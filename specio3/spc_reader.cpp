#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <fstream>
#include <vector>
#include <string>
#include <stdexcept>
#include <cstring>
#include <sstream>

namespace py = pybind11;

static std::string human_offset(std::streamoff o) {
    std::ostringstream ss;
    ss << o;
    return ss.str();
}

// Helpers for reading little-endian values (assuming host is little-endian; if not, you need byte-swapping)
template <typename T>
T read_le(const char* buffer) {
    T v;
    std::memcpy(&v, buffer, sizeof(T));
    return v;
}

struct Subfile {
    std::vector<double> x;
    std::vector<double> y;
    float z_start = 0;
    float z_end = 0;
};

struct SPCFile {
    bool is_multifile = false;
    bool is_xy = false;
    bool is_xyxy = false;
    bool y_in_16bit = false;
    uint32_t num_points = 0;
    uint32_t num_subfiles = 0;
    double first_x = 0;
    double last_x = 0;
    std::vector<Subfile> subfiles;
    std::string log_text;
};

static double apply_y_scaling_uint32(uint32_t integer_y, int8_t exponent_byte, bool is_16bit) {
    double divisor = is_16bit ? static_cast<double>(1ULL << 16) : static_cast<double>(1ULL << 32);
    double scale = std::pow(2.0, static_cast<int>(exponent_byte));
    return (scale * static_cast<double>(integer_y)) / divisor;
}

static double apply_y_scaling_uint16(uint16_t integer_y, int8_t exponent_byte) {
    constexpr auto divisor = static_cast<double>(1ULL << 16);
    const double scale = std::pow(2.0, static_cast<int>(exponent_byte));
    return (scale * static_cast<double>(integer_y)) / divisor;
}

SPCFile read_spc_impl(const std::string& filename) {
    std::ifstream f(filename, std::ios::binary);
    if (!f) {
        throw std::runtime_error("Unable to open file: " + filename);
    }

    SPCFile out;

    // Read main header (512 bytes)
    std::vector<char> mainhdr_buf(512);
    f.read(mainhdr_buf.data(), 512);
    if (f.gcount() != 512) {
        throw std::runtime_error("Failed to read full main header (expected 512 bytes, got " + std::to_string(f.gcount()) + ")");
    }

    // Parse fields from main header.
    // NOTE: Offsets are based on the specification PDF. If your version differs adjust offsets accordingly.
    auto file_type_flag = static_cast<uint8_t>(mainhdr_buf[0]);
    out.is_multifile = (file_type_flag & 0x10) != 0;
    out.is_xy = (file_type_flag & 0x80) != 0;
    bool has_per_subfile_x = (file_type_flag & 0x40) != 0; // indicates XYXY multifile when combined
    out.is_xyxy = out.is_multifile && out.is_xy && has_per_subfile_x;
    out.y_in_16bit = (file_type_flag & 0x01) != 0;

    // Global exponent for Y (signed). Spec uses 0x80 (-128) to denote float.
    int8_t global_exponent_y = static_cast<int8_t>(mainhdr_buf[5]);

    // Number of points (if not XYXY) at offset 4, little-endian 32-bit
    out.num_points = read_le<uint32_t>(mainhdr_buf.data() + 4);
    // First X at offset 8 (double), Last X at offset 16
    out.first_x = read_le<double>(mainhdr_buf.data() + 8);
    out.last_x = read_le<double>(mainhdr_buf.data() + 16);
    // Number of subfiles: offset 23 according to earlier parsing logic; if wrong, adjust per your spec.
    out.num_subfiles = read_le<uint32_t>(mainhdr_buf.data() + 23);
    auto log_block_offset = read_le<uint32_t>(mainhdr_buf.data() + 246);

    // Shared X array (for XY / XYY) if applicable
    std::vector<double> shared_x;
    // Defensive: ensure out.num_points is initialized before use
    // (It is set above from the file header, but some static analyzers may warn)
    // If you want to silence such warnings, you can add an assert:
    if (out.is_xy && out.num_points == 0) {
        throw std::runtime_error("num_points is zero but expected >0 for XY-type file.");
    }
    if (!out.is_xyxy) {
        if (out.is_xy) {
            // Shared X array of floats comes immediately after main header
            shared_x.resize(out.num_points);
            for (uint32_t i = 0; i < out.num_points; ++i) {
                float xv;
                f.read(reinterpret_cast<char*>(&xv), sizeof(float));
                if (!f) {
                    throw std::runtime_error("Failed reading shared X array at point " + std::to_string(i) +
                                             ", file offset " + human_offset(f.tellg()));
                }
                shared_x[i] = static_cast<double>(xv);
            }
            if (!shared_x.empty()) {
                out.first_x = shared_x.front();
                out.last_x = shared_x.back();
            }
        }
    }

    // Read all subheaders (each is 32 bytes)
    struct SubHeaderRaw {
        uint8_t subfile_flags;       // 1 byte
        int8_t subfile_exponent;     // 1 byte (signed; -128 == float Y)
        uint16_t subfile_index;      // 2 bytes
        float z_start;               // 4 bytes
        float z_end;                 // 4 bytes
        float noise;                // 4 bytes
        uint32_t num_points_xyxy;    // 4 bytes
        uint32_t num_coadded_scans;  // 4 bytes
        float w_axis_value;          // 4 bytes
        char reserved[4];            // 4 bytes
    };
    static_assert(sizeof(SubHeaderRaw) == 32, "SubheaderRaw must be 32 bytes");

    std::vector<SubHeaderRaw> subhdrs;
    subhdrs.reserve(out.num_subfiles);
    for (uint32_t i = 0; i < out.num_subfiles; ++i) {
        SubHeaderRaw sh;
        f.read(reinterpret_cast<char*>(&sh), sizeof(SubHeaderRaw));
        if (!f) {
            std::ostringstream err;
            err << "Failed reading subheader " << i << " at offset " << human_offset(f.tellg());
            throw std::runtime_error(err.str());
        }
        subhdrs.push_back(sh);
    }

    // Determine if global Y is float
    bool global_float_y = (global_exponent_y == static_cast<int8_t>(-128));

    // Prepare subfiles container
    out.subfiles.resize(out.num_subfiles);
    for (uint32_t si = 0; si < out.num_subfiles; ++si) {
        const SubHeaderRaw& sh = subhdrs[si];
        Subfile& s = out.subfiles[si];
        s.z_start = sh.z_start;
        s.z_end = sh.z_end;

        bool subfile_float_y = (sh.subfile_exponent == static_cast<int8_t>(-128));
        int8_t exponent_to_use = subfile_float_y ? 0 : sh.subfile_exponent;
        bool use_float_y = subfile_float_y || global_float_y;

        uint32_t this_num_points = out.num_points;
        if (out.is_xyxy) {
            this_num_points = sh.num_points_xyxy;
        }

        if (this_num_points == 0) {
            std::ostringstream err;
            err << "Subfile " << si << " has zero points (this_num_points==0)";
            throw std::runtime_error(err.str());
        }

        // X axis per subfile
        if (out.is_xyxy) {
            s.x.resize(this_num_points);
            for (uint32_t i = 0; i < this_num_points; ++i) {
                float xv;
                f.read(reinterpret_cast<char*>(&xv), sizeof(float));
                if (!f) {
                    std::ostringstream err;
                    err << "Failed reading XYXY subfile X data for subfile " << si << " at point " << i
                        << " offset " << human_offset(f.tellg());
                    throw std::runtime_error(err.str());
                }
                s.x[i] = static_cast<double>(xv);
            }
        } else if (out.is_xy) {
            s.x = shared_x;
        } else {
            // Y-only: generate linearly spaced X
            s.x.resize(out.num_points);
            if (out.num_points > 1) {
                double step = (out.last_x - out.first_x) / static_cast<double>(out.num_points - 1);
                for (uint32_t i = 0; i < out.num_points; ++i) {
                    s.x[i] = out.first_x + step * i;
                }
            } else {
                s.x[0] = out.first_x;
            }
        }

        // Y values
        s.y.resize(this_num_points);
        if (use_float_y) {
            for (uint32_t i = 0; i < this_num_points; ++i) {
                float yv;
                f.read(reinterpret_cast<char*>(&yv), sizeof(float));
                if (!f) {
                    std::ostringstream err;
                    err << "Failed reading float Y values for subfile " << si << " at point " << i
                        << " offset " << human_offset(f.tellg());
                    throw std::runtime_error(err.str());
                }
                s.y[i] = static_cast<double>(yv);
            }
        } else {
            if (out.y_in_16bit) {
                for (uint32_t i = 0; i < this_num_points; ++i) {
                    uint16_t iv;
                    f.read(reinterpret_cast<char*>(&iv), sizeof(uint16_t));
                    if (!f) {
                        std::ostringstream err;
                        err << "Failed reading 16-bit integer Y for subfile " << si << " at point " << i
                            << " offset " << human_offset(f.tellg());
                        throw std::runtime_error(err.str());
                    }
                    s.y[i] = apply_y_scaling_uint16(iv, exponent_to_use);
                }
            } else {
                for (uint32_t i = 0; i < this_num_points; ++i) {
                    uint32_t iv;
                    f.read(reinterpret_cast<char*>(&iv), sizeof(uint32_t));
                    if (!f) {
                        std::ostringstream err;
                        err << "Failed reading 32-bit integer Y for subfile " << si << " at point " << i
                            << " offset " << human_offset(f.tellg());
                        throw std::runtime_error(err.str());
                    }
                    s.y[i] = apply_y_scaling_uint32(iv, exponent_to_use, false);
                }
            }
        }
    }

    // Read log text if present
    if (log_block_offset != 0) {
        f.seekg(static_cast<std::streamoff>(log_block_offset), std::ios::beg);
        if (!f) {
            throw std::runtime_error("Failed to seek to log block offset: " + std::to_string(log_block_offset));
        }

        // Minimal log header reading (assuming fixed layout)
        struct LogHeaderRaw {
            uint32_t log_block_size;
            uint32_t memory_block_size;
            uint32_t offset_to_text;
            uint32_t binary_log_size;
            uint32_t disk_area_size;
            char reserved[44];
        };
        static_assert(sizeof(LogHeaderRaw) == 64, "Unexpected log header size");
        LogHeaderRaw loghdr;
        f.read(reinterpret_cast<char*>(&loghdr), sizeof(LogHeaderRaw));
        if (!f) {
            throw std::runtime_error("Failed reading log header at offset " + human_offset(f.tellg()));
        }

        uint32_t text_offset_within_log = loghdr.offset_to_text;
        if (text_offset_within_log != 0 && loghdr.log_block_size > text_offset_within_log) {
            uint32_t ascii_log_size = loghdr.log_block_size - text_offset_within_log;
            f.seekg(static_cast<std::streamoff>(log_block_offset + text_offset_within_log), std::ios::beg);
            std::vector<char> logtext_buf(ascii_log_size);
            f.read(logtext_buf.data(), ascii_log_size);
            size_t actually_read = f.gcount();
            if (actually_read > 0) {
                out.log_text.assign(logtext_buf.data(), actually_read);
            }
        }
    }

    return out;
}

py::dict to_pydict(const SPCFile& spc) {
    py::dict d;
    d["is_multifile"] = spc.is_multifile;
    d["is_xy"] = spc.is_xy;
    d["is_xyxy"] = spc.is_xyxy;
    d["y_in_16bit"] = spc.y_in_16bit;
    d["num_points"] = spc.num_points;
    d["num_subfiles"] = spc.num_subfiles;
    d["first_x"] = spc.first_x;
    d["last_x"] = spc.last_x;
    d["log_text"] = spc.log_text;

    py::list subs;
    for (const auto&[x, y, z_start, z_end] : spc.subfiles) {
        py::dict sd;
        sd["x"] = x;
        sd["y"] = y;
        sd["z_start"] = z_start;
        sd["z_end"] = z_end;
        subs.append(sd);
    }
    d["subfiles"] = subs;
    return d;
}
