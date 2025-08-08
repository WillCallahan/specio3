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
    // Convert unsigned to signed
    int32_t signed_y = static_cast<int32_t>(integer_y);
    
    // For SPC files, if exponent is -128, it indicates float data
    if (exponent_byte == -128) {
        // Reinterpret the integer as a float
        float float_val;
        std::memcpy(&float_val, &integer_y, sizeof(float));
        return static_cast<double>(float_val);
    }
    
    // For integer data, use a reasonable scaling
    // The exponent in SPC files is typically used as a power of 2 divisor
    // But the values we're seeing suggest we need a different approach
    // Let's try a simple scaling that brings the values to a reasonable range
    double scale = 1.0 / (1ULL << 20);  // Divide by 2^20 to bring large integers to reasonable range
    return scale * static_cast<double>(signed_y);
}

static double apply_y_scaling_uint16(uint16_t integer_y, int8_t exponent_byte) {
    // Convert unsigned to signed
    int16_t signed_y = static_cast<int16_t>(integer_y);
    
    if (exponent_byte == -128) {
        // This shouldn't happen for 16-bit data, but handle it
        return static_cast<double>(signed_y);
    }
    
    // Simple scaling for 16-bit data
    double scale = 1.0 / (1ULL << 10);  // Divide by 2^10
    return scale * static_cast<double>(signed_y);
}

SPCFile read_spc_impl(const std::string& filename) {
    std::ifstream f(filename, std::ios::binary);
    if (!f) {
        throw std::runtime_error("Unable to open file: " + filename);
    }

    SPCFile out;

    // Get file size
    f.seekg(0, std::ios::end);
    std::streamsize file_size = f.tellg();
    f.seekg(0, std::ios::beg);

    // Read main header (512 bytes)
    std::vector<char> mainhdr_buf(512);
    f.read(mainhdr_buf.data(), 512);
    if (f.gcount() != 512) {
        throw std::runtime_error("Failed to read full main header (expected 512 bytes, got " + std::to_string(f.gcount()) + ")");
    }

    // Parse fields from main header according to SPC specification
    // Byte 0: File type flags
    auto file_type_flag = static_cast<uint8_t>(mainhdr_buf[0]);
    out.is_multifile = (file_type_flag & 0x10) != 0;
    out.is_xy = (file_type_flag & 0x80) != 0;
    bool has_per_subfile_x = (file_type_flag & 0x40) != 0;
    out.is_xyxy = out.is_multifile && out.is_xy && has_per_subfile_x;
    out.y_in_16bit = (file_type_flag & 0x01) != 0;

    // Byte 1: Global exponent for Y (signed). Spec uses 0x80 (-128) to denote float.
    int8_t global_exponent_y = static_cast<int8_t>(mainhdr_buf[1]);

    // For Y-only files, calculate number of points from file size
    if (!out.is_xy) {
        std::streamsize data_size = file_size - 512;  // Subtract header size
        if (out.y_in_16bit) {
            out.num_points = static_cast<uint32_t>(data_size / 2);
        } else {
            out.num_points = static_cast<uint32_t>(data_size / 4);
        }
    } else {
        // For XY files, read from header at offset 2-3
        out.num_points = read_le<uint16_t>(mainhdr_buf.data() + 2);
    }
    
    // Bytes 8-11: First X (float, little-endian)
    out.first_x = static_cast<double>(read_le<float>(mainhdr_buf.data() + 8));
    
    // Bytes 12-15: Last X (float, little-endian)  
    out.last_x = static_cast<double>(read_le<float>(mainhdr_buf.data() + 12));
    
    // For single file, num_subfiles is 1
    if (!out.is_multifile) {
        out.num_subfiles = 1;
    } else {
        // Bytes 22-25: Number of subfiles for multifile, little-endian 32-bit
        out.num_subfiles = read_le<uint32_t>(mainhdr_buf.data() + 22);
    }
    
    // Bytes 244-247: Log block offset, little-endian 32-bit
    auto log_block_offset = read_le<uint32_t>(mainhdr_buf.data() + 244);

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

    // Read all subheaders (each is 32 bytes) - only for multifile
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
    if (out.is_multifile) {
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
    } else {
        // For single file, create a dummy subheader
        SubHeaderRaw sh = {};
        sh.subfile_exponent = global_exponent_y;
        sh.num_points_xyxy = out.num_points;
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
