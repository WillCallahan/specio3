#include "spc_reader.h"
#include <array>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <cstring>
#include <cstdint>
#include <cmath>
#include <iostream>
#include <iomanip>

// Helper to get remaining bytes from current position
static std::streamoff remaining_bytes(std::ifstream &fin) {
    std::streampos cur = fin.tellg();
    fin.seekg(0, std::ios::end);
    std::streampos end = fin.tellg();
    fin.seekg(cur);
    return static_cast<std::streamoff>(end - cur);
}

// Dump the first N bytes of the header for debugging (hex)
static void dump_header_debug(const std::array<char, 512> &hdr, size_t n = 64) {
    std::cerr << "SPC header (first " << n << " bytes):\n";
    for (size_t i = 0; i < n; ++i) {
        if (i % 16 == 0) std::cerr << std::setw(4) << std::setfill('0') << i << ": ";
        std::cerr << std::hex << std::setw(2) << std::setfill('0')
                  << (static_cast<uint8_t>(hdr[i]) & 0xFF) << ' ';
        if ((i + 1) % 16 == 0) std::cerr << '\n';
    }
    std::cerr << std::dec << "\n";
}

static std::vector<double> read_Y_uint32(std::ifstream &fin, int32_t points, uint8_t exponent,
                                         const std::string &context) {
    std::vector<double> y_values;
    if (exponent == 0x80) {
        size_t expect_bytes = sizeof(float) * static_cast<size_t>(points);
        if (remaining_bytes(fin) < static_cast<std::streamoff>(expect_bytes)) {
            std::ostringstream oss;
            oss << "Not enough bytes for float Y for " << context << ": need " << expect_bytes
                << " have " << remaining_bytes(fin);
            throw std::runtime_error(oss.str());
        }
        std::vector<float> ybuf(points);
        fin.read(reinterpret_cast<char *>(ybuf.data()), expect_bytes);
        if (fin.gcount() != static_cast<std::streamsize>(expect_bytes)) {
            std::ostringstream oss;
            oss << "Failed to read float Y values for " << context << " (got " << fin.gcount()
                << " bytes, expected " << expect_bytes << ")";
            throw std::runtime_error(oss.str());
        }
        y_values.reserve(points);
        for (int i = 0; i < points; ++i) {
            y_values.push_back(static_cast<double>(ybuf[i]));
        }
    } else {
        size_t expect_bytes = sizeof(uint32_t) * static_cast<size_t>(points);
        if (remaining_bytes(fin) < static_cast<std::streamoff>(expect_bytes)) {
            std::ostringstream oss;
            oss << "Not enough bytes for scaled uint32 Y for " << context << ": need " << expect_bytes
                << " have " << remaining_bytes(fin);
            throw std::runtime_error(oss.str());
        }
        std::vector<uint32_t> ybuf(points);
        fin.read(reinterpret_cast<char *>(ybuf.data()), expect_bytes);
        if (fin.gcount() != static_cast<std::streamsize>(expect_bytes)) {
            std::ostringstream oss;
            oss << "Failed to read scaled integer Y values for " << context << " (got " << fin.gcount()
                << " bytes, expected " << expect_bytes << ")";
            throw std::runtime_error(oss.str());
        }
        y_values.reserve(points);
        for (int i = 0; i < points; ++i) {
            double normalized = static_cast<double>(ybuf[i]) / 4294967296.0; // 2^32
            y_values.push_back(std::ldexp(normalized, static_cast<int>(exponent)));
        }
    }
    return y_values;
}

static std::vector<double> read_Y_uint16(std::ifstream &fin, int32_t points, uint8_t exponent,
                                         const std::string &context) {
    std::vector<double> y_values;
    if (exponent == 0x80) {
        size_t expect_bytes = sizeof(uint16_t) * static_cast<size_t>(points);
        if (remaining_bytes(fin) < static_cast<std::streamoff>(expect_bytes)) {
            std::ostringstream oss;
            oss << "Not enough bytes for uint16 Y for " << context << ": need " << expect_bytes
                << " have " << remaining_bytes(fin);
            throw std::runtime_error(oss.str());
        }
        std::vector<uint16_t> ybuf(points);
        fin.read(reinterpret_cast<char *>(ybuf.data()), expect_bytes);
        if (fin.gcount() != static_cast<std::streamsize>(expect_bytes)) {
            std::ostringstream oss;
            oss << "Failed to read uint16 Y values for " << context << " (got " << fin.gcount()
                << " bytes, expected " << expect_bytes << ")";
            throw std::runtime_error(oss.str());
        }
        y_values.reserve(points);
        for (int i = 0; i < points; ++i) {
            y_values.push_back(static_cast<double>(ybuf[i]));
        }
    } else {
        size_t expect_bytes = sizeof(uint16_t) * static_cast<size_t>(points);
        if (remaining_bytes(fin) < static_cast<std::streamoff>(expect_bytes)) {
            std::ostringstream oss;
            oss << "Not enough bytes for scaled uint16 Y for " << context << ": need " << expect_bytes
                << " have " << remaining_bytes(fin);
            throw std::runtime_error(oss.str());
        }
        std::vector<uint16_t> ybuf(points);
        fin.read(reinterpret_cast<char *>(ybuf.data()), expect_bytes);
        if (fin.gcount() != static_cast<std::streamsize>(expect_bytes)) {
            std::ostringstream oss;
            oss << "Failed to read scaled uint16 Y values for " << context << " (got " << fin.gcount()
                << " bytes, expected " << expect_bytes << ")";
            throw std::runtime_error(oss.str());
        }
        y_values.reserve(points);
        for (int i = 0; i < points; ++i) {
            double normalized = static_cast<double>(ybuf[i]) / 65536.0; // 2^16
            y_values.push_back(std::ldexp(normalized, static_cast<int>(exponent)));
        }
    }
    return y_values;
}

// Helper: read little-endian 4-byte int from buffer
static int32_t read_le_int32(const char *buf) {
    uint8_t b0 = static_cast<uint8_t>(buf[0]);
    uint8_t b1 = static_cast<uint8_t>(buf[1]);
    uint8_t b2 = static_cast<uint8_t>(buf[2]);
    uint8_t b3 = static_cast<uint8_t>(buf[3]);
    return static_cast<int32_t>(b0 | (b1 << 8) | (b2 << 16) | (b3 << 24));
}

// Helper: read little-endian 8-byte double from buffer
static double read_le_double(const char *buf) {
    uint64_t bits = 0;
    for (int i = 0; i < 8; ++i) {
        bits |= (static_cast<uint64_t>(static_cast<uint8_t>(buf[i])) << (i * 8));
    }
    double result;
    std::memcpy(&result, &bits, sizeof(double));
    return result;
}

std::vector<Spectrum> read_spc_multi(const std::string &path) {
    std::ifstream fin(path, std::ios::binary);
    if (!fin) {
        throw std::runtime_error("Failed to open SPC file: " + path);
    }

    // Read main header
    std::array<char, 512> main_hdr;
    fin.read(main_hdr.data(), main_hdr.size());
    if (!fin) throw std::runtime_error("Failed to read main header");

    // Define the constant here where it's used
    const int32_t MAX_REASONABLE_POINTS = 10'000'000; // arbitrary guard

    // Parse primary flags
    uint8_t file_type_flag = static_cast<uint8_t>(main_hdr[0]);
    bool y16bit = (file_type_flag & 0x01) != 0;
    bool is_xy = (file_type_flag & 0x80) != 0;
    bool per_subfile_x = (file_type_flag & 0x40) != 0; // XYXY vs XYY
    // Always read as little-endian - number of points is at offset 1 (bytes 1-4)

    // Try different interpretations of the point count field
    // First, try reading as 32-bit from offset 1
    int32_t raw_points = read_le_int32(main_hdr.data() + 1);
    int32_t global_num_points = 0;

    // Check if this looks like a reasonable value
    if (raw_points > 0 && raw_points < MAX_REASONABLE_POINTS) {
        global_num_points = raw_points;
        std::cerr << "Using 32-bit point count: " << global_num_points << std::endl;
    } else {
        // Try reading just the first byte at offset 1
        global_num_points = static_cast<uint8_t>(main_hdr[1]);
        std::cerr << "32-bit value unreasonable (" << raw_points << "), trying 8-bit: " << global_num_points << std::endl;

        // If that's too small, try 16-bit little-endian from offset 1
        if (global_num_points < 10) {
            uint16_t points_16 = static_cast<uint8_t>(main_hdr[1]) | (static_cast<uint8_t>(main_hdr[2]) << 8);
            if (points_16 > 0 && points_16 < MAX_REASONABLE_POINTS) {
                global_num_points = points_16;
                std::cerr << "8-bit too small, using 16-bit: " << global_num_points << std::endl;
            }
        }
    }

    // Handle special cases in SPC format
    if (global_num_points < 0) {
        // Some SPC files use negative values for special encoding
        global_num_points = -global_num_points;
        std::cerr << "Warning: Negative point count converted to positive: " << global_num_points << std::endl;
    }

    // Read num_subfiles as 2-byte little-endian at offset 0x1E (30)
    uint16_t raw_num_subfiles = static_cast<uint8_t>(main_hdr[30]) | (static_cast<uint8_t>(main_hdr[31]) << 8);
    bool is_multifile = (file_type_flag & 0x04) != 0;
    int32_t num_subfiles = is_multifile ? raw_num_subfiles : 1;

    if (global_num_points <= 0) {
        std::ostringstream oss;
        oss << "Invalid number of points in SPC header: " << global_num_points;
        dump_header_debug(main_hdr);
        throw std::runtime_error(oss.str());
    }
    // Sanity cap: if ridiculously large, fail early with diagnostics
    if (!is_multifile && global_num_points > MAX_REASONABLE_POINTS) {
        std::ostringstream oss;
        oss << "Unreasonable point count for single-spectrum SPC: " << global_num_points
            << " (file size likely too small). Header may be corrupt or multifile detection failed.";
        dump_header_debug(main_hdr);
        throw std::runtime_error(oss.str());
    }

    // Read first_x and last_x as little-endian doubles
    double first_x = read_le_double(main_hdr.data() + 8);
    double last_x = read_le_double(main_hdr.data() + 16);

    std::vector<Spectrum> result;
    std::vector<double> shared_x;

    std::streampos cursor = static_cast<std::streampos>(512);

    // Debug: print header info
    std::cerr << "SPC file_type_flag: 0x" << std::hex << (int)file_type_flag << std::dec << std::endl;
    std::cerr << "is_xy: " << is_xy << ", per_subfile_x: " << per_subfile_x << ", is_multifile: " << is_multifile << std::endl;
    std::cerr << "raw_num_subfiles: " << raw_num_subfiles << ", global_num_points: " << global_num_points << std::endl;

    if (is_multifile) {
        if (is_xy && !per_subfile_x) {
            // XYY multifile: shared explicit X after main header
            fin.seekg(cursor);
            std::vector<float> xbuf(global_num_points);
            fin.read(reinterpret_cast<char *>(xbuf.data()), sizeof(float) * global_num_points);
            if (!fin) throw std::runtime_error("Failed to read shared X axis for XYY multifile");
            shared_x.reserve(global_num_points);
            for (int i = 0; i < global_num_points; ++i) {
                shared_x.push_back(static_cast<double>(xbuf[i]));
            }
            cursor = static_cast<std::streampos>(512 + sizeof(float) * global_num_points);
        } else if (!is_xy) {
            // Y-only multifile: linear X
            double delta = (global_num_points > 1) ? (last_x - first_x) / (global_num_points - 1) : 0.0;
            shared_x.resize(global_num_points);
            for (int i = 0; i < global_num_points; ++i) {
                shared_x[i] = first_x + delta * i;
            }
            cursor = static_cast<std::streampos>(512);
        } else {
            // XYXY multifile: per-subfile X, nothing global
            cursor = static_cast<std::streampos>(512);
        }

        for (int sf = 0; sf < num_subfiles; ++sf) {
            fin.seekg(cursor);
            if (!fin) throw std::runtime_error("Seek failed to subfile header");

            std::array<char, 32> subhdr;
            fin.read(subhdr.data(), subhdr.size());
            if (!fin) throw std::runtime_error("Failed to read subfile header");

            if (sf == 0) {
                std::cerr << "Subfile 0 subhdr raw: ";
                for (int i = 0; i < 32; ++i) std::cerr << std::hex << (static_cast<uint8_t>(subhdr[i]) & 0xFF) << ' ';
                std::cerr << std::dec << std::endl;
            }

            uint8_t subfile_exponent = static_cast<uint8_t>(subhdr[1]);
            int32_t subfile_num_points = global_num_points;
            if (is_xy && per_subfile_x) {
                // Per SPC format, number of points is at offset 2 in subheader
                subfile_num_points = read_le_int32(subhdr.data() + 2);
                // Debug: print raw bytes and value
                std::cerr << "Subfile " << sf << " num_points (raw): ";
                for (int i = 0; i < 4; ++i) {
                    std::cerr << std::hex << (static_cast<uint8_t>(subhdr[2 + i]) & 0xFF) << ' ';
                }
                std::cerr << std::dec << " => " << subfile_num_points << std::endl;
                if (subfile_num_points <= 0 || subfile_num_points > MAX_REASONABLE_POINTS) {
                    std::ostringstream oss;
                    oss << "Invalid per-subfile point count in XYXY multifile: " << subfile_num_points;
                    dump_header_debug(main_hdr);
                    throw std::runtime_error(oss.str());
                }
            }

            cursor += static_cast<std::streamoff>(32);

            std::vector<double> x_values;
            std::vector<double> y_values;

            // X axis
            if (is_xy && per_subfile_x) {
                // XYXY: read local X array
                fin.seekg(cursor);
                std::vector<float> xbuf(subfile_num_points);
                fin.read(reinterpret_cast<char *>(xbuf.data()), sizeof(float) * subfile_num_points);
                if (!fin) throw std::runtime_error("Failed to read local X for subfile");
                x_values.reserve(subfile_num_points);
                for (int i = 0; i < subfile_num_points; ++i) {
                    x_values.push_back(static_cast<double>(xbuf[i]));
                }
                cursor += static_cast<std::streamoff>(sizeof(float) * subfile_num_points);
            } else {
                // Shared X (Y-only or XYY)
                x_values = shared_x;
            }

            // Y values
            fin.seekg(cursor);
            if (!fin) throw std::runtime_error("Seek to Y data failed for subfile");

            if (y16bit) {
                y_values = read_Y_uint16(fin, subfile_num_points, subfile_exponent,
                                         "subfile " + std::to_string(sf));
                cursor += static_cast<std::streamoff>(sizeof(uint16_t) * static_cast<size_t>(subfile_num_points));
            } else {
                y_values = read_Y_uint32(fin, subfile_num_points, subfile_exponent,
                                         "subfile " + std::to_string(sf));
                cursor += static_cast<std::streamoff>(
                    (subfile_exponent == 0x80 ? sizeof(float) : sizeof(uint32_t)) * static_cast<size_t>(subfile_num_points));
            }

            if (x_values.empty()) {
                double delta = (global_num_points > 1) ? (last_x - first_x) / (global_num_points - 1) : 0.0;
                x_values.resize(global_num_points);
                for (int i = 0; i < global_num_points; ++i) {
                    x_values[i] = first_x + delta * i;
                }
            }

            result.emplace_back(std::move(x_values), std::move(y_values));
        }
    } else {
        // Single-spectrum file
        std::vector<double> x_values;
        std::vector<double> y_values;

        if (is_xy) {
            fin.seekg(cursor);
            std::vector<float> xbuf(global_num_points);
            fin.read(reinterpret_cast<char *>(xbuf.data()), sizeof(float) * global_num_points);
            if (!fin) throw std::runtime_error("Failed to read explicit X axis");
            x_values.reserve(global_num_points);
            for (int i = 0; i < global_num_points; ++i) {
                x_values.push_back(static_cast<double>(xbuf[i]));
            }
            cursor = static_cast<std::streampos>(512 + sizeof(float) * global_num_points);
        } else {
            double delta = (global_num_points > 1) ? (last_x - first_x) / (global_num_points - 1) : 0.0;
            x_values.resize(global_num_points);
            for (int i = 0; i < global_num_points; ++i) {
                x_values[i] = first_x + delta * i;
            }
            cursor = static_cast<std::streampos>(512);
        }

        // Subheader
        fin.seekg(cursor);
        std::array<char, 32> subhdr;
        fin.read(subhdr.data(), subhdr.size());
        if (!fin) throw std::runtime_error("Failed to read subheader for single file");
        uint8_t subfile_exponent = static_cast<uint8_t>(subhdr[1]);
        cursor += static_cast<std::streamoff>(32);

        // Y data
        fin.seekg(cursor);
        if (!fin) throw std::runtime_error("Seek to Y data failed for single file");

        if (y16bit) {
            y_values = read_Y_uint16(fin, global_num_points, subfile_exponent, "single file");
        } else {
            y_values = read_Y_uint32(fin, global_num_points, subfile_exponent, "single file");
        }

        result.emplace_back(std::move(x_values), std::move(y_values));
    }

    return result;
}
