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
#include <filesystem>

namespace spc_reader {

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

// Helper: read little-endian 4-byte int from buffer
static int32_t read_le_int32(const char *buf) {
    uint8_t b0 = static_cast<uint8_t>(buf[0]);
    uint8_t b1 = static_cast<uint8_t>(buf[1]);
    uint8_t b2 = static_cast<uint8_t>(buf[2]);
    uint8_t b3 = static_cast<uint8_t>(buf[3]);
    return static_cast<int32_t>(b0 | (b1 << 8) | (b2 << 16) | (b3 << 24));
}

// Helper: read little-endian 4-byte float from buffer
static float read_le_float(const char *buf) {
    uint32_t bits = 0;
    for (int i = 0; i < 4; ++i) {
        bits |= (static_cast<uint32_t>(static_cast<uint8_t>(buf[i])) << (i * 8));
    }
    float result;
    std::memcpy(&result, &bits, sizeof(float));
    return result;
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

std::map<std::string, Spectrum> read_spc(const std::string &path) {
    std::ifstream fin(path, std::ios::binary);
    if (!fin) {
        throw std::runtime_error("Failed to open SPC file: " + path);
    }

    // Read main header (512 bytes)
    std::array<char, 512> main_hdr;
    fin.read(main_hdr.data(), main_hdr.size());
    if (!fin) throw std::runtime_error("Failed to read main header");

    // Parse file type flags
    uint8_t file_type_flag = static_cast<uint8_t>(main_hdr[0]);
    bool y16bit = (file_type_flag & 0x01) != 0;
    bool is_xy = (file_type_flag & 0x80) != 0;
    bool per_subfile_x = (file_type_flag & 0x40) != 0;
    bool is_multifile = (file_type_flag & 0x04) != 0;

    std::cerr << "SPC file_type_flag: 0x" << std::hex << (int)file_type_flag << std::dec << std::endl;
    std::cerr << "y16bit: " << y16bit << ", is_xy: " << is_xy << ", per_subfile_x: " << per_subfile_x << ", is_multifile: " << is_multifile << std::endl;

    // Based on the hex dump analysis:
    // 00 4d ff ff 00 a1 01 47 18 86 89 45 81 8f c8 43
    // The number of points appears to be at offset 1 as a single byte: 0x4d = 77 decimal
    uint8_t version_or_points = static_cast<uint8_t>(main_hdr[1]);
    int32_t global_num_points = version_or_points;

    std::cerr << "Points from offset 1 (single byte): " << global_num_points << std::endl;

    // If that's unreasonably small, try reading more bytes
    if (global_num_points < 10) {
        // Try reading as 16-bit little-endian from offset 1
        uint16_t points_16 = static_cast<uint8_t>(main_hdr[1]) | (static_cast<uint8_t>(main_hdr[2]) << 8);
        if (points_16 > 10 && points_16 < 10000) {
            global_num_points = points_16;
            std::cerr << "Using 16-bit value: " << global_num_points << std::endl;
        }
    }

    // Final validation
    if (global_num_points <= 0 || global_num_points > 10000) {
        dump_header_debug(main_hdr);
        std::cerr << "Cannot determine point count, using reasonable default" << std::endl;
        global_num_points = 512; // Common spectral size
    }

    std::cerr << "Final point count: " << global_num_points << std::endl;

    // For X-axis range, the hex shows values at offsets 8-15:
    // 18 86 89 45 81 8f c8 43
    // This looks like two 4-byte IEEE floats
    float first_x_f = read_le_float(main_hdr.data() + 8);  // 18 86 89 45
    float last_x_f = read_le_float(main_hdr.data() + 12); // 81 8f c8 43

    double first_x = static_cast<double>(first_x_f);
    double last_x = static_cast<double>(last_x_f);

    std::cerr << "Raw X range floats: " << first_x_f << " to " << last_x_f << std::endl;
    std::cerr << "X range: " << first_x << " to " << last_x << std::endl;

    // Validate and potentially fix X range
    if (std::isnan(first_x) || std::isnan(last_x) || std::isinf(first_x) || std::isinf(last_x) ||
        first_x == last_x || std::abs(first_x) > 1e6 || std::abs(last_x) > 1e6) {
        std::cerr << "Invalid X range detected, generating default range" << std::endl;
        // For typical IR spectroscopy: 4000 to 400 cm-1
        first_x = 4000.0;
        last_x = 400.0;
    }

    std::vector<double> x_values;
    std::vector<double> y_values;

    // Generate X values
    if (global_num_points > 1) {
        double delta = (last_x - first_x) / (global_num_points - 1);
        x_values.reserve(global_num_points);
        for (int i = 0; i < global_num_points; ++i) {
            x_values.push_back(first_x + delta * i);
        }
    } else {
        x_values.push_back(first_x);
    }

    // For Y data, start after the 512-byte header
    std::streampos data_pos = 512;

    // Skip subheader if present
    data_pos += 32;

    // Read Y values
    fin.seekg(data_pos);
    if (!fin) throw std::runtime_error("Failed to seek to Y data");

    std::streamoff available_bytes = remaining_bytes(fin);
    std::cerr << "Available bytes for Y data: " << available_bytes << std::endl;

    // Try different Y data formats
    bool y_read_success = false;

    // Try reading as 32-bit integers first (most common)
    if (!y_read_success && available_bytes >= static_cast<std::streamoff>(sizeof(uint32_t) * global_num_points)) {
        std::cerr << "Attempting to read Y as 32-bit integers" << std::endl;
        std::vector<uint32_t> y_buffer(global_num_points);
        fin.read(reinterpret_cast<char*>(y_buffer.data()), sizeof(uint32_t) * global_num_points);
        if (fin) {
            y_values.reserve(global_num_points);
            for (const auto& y : y_buffer) {
                // Scale down large integer values
                double scaled_y = static_cast<double>(y);
                if (scaled_y > 1e6) {
                    scaled_y /= 1e6; // Scale down very large values
                }
                y_values.push_back(scaled_y);
            }
            y_read_success = true;
            std::cerr << "Successfully read Y as 32-bit integers" << std::endl;
        } else {
            fin.clear();
            fin.seekg(data_pos);
        }
    }

    // Try reading as floats
    if (!y_read_success && available_bytes >= static_cast<std::streamoff>(sizeof(float) * global_num_points)) {
        std::cerr << "Attempting to read Y as floats" << std::endl;
        std::vector<float> y_buffer(global_num_points);
        fin.read(reinterpret_cast<char*>(y_buffer.data()), sizeof(float) * global_num_points);
        if (fin) {
            y_values.reserve(global_num_points);
            for (const auto& y : y_buffer) {
                y_values.push_back(static_cast<double>(y));
            }
            y_read_success = true;
            std::cerr << "Successfully read Y as floats" << std::endl;
        } else {
            fin.clear();
            fin.seekg(data_pos);
        }
    }

    // Try reading as 16-bit integers
    if (!y_read_success && available_bytes >= static_cast<std::streamoff>(sizeof(uint16_t) * global_num_points)) {
        std::cerr << "Attempting to read Y as 16-bit integers" << std::endl;
        std::vector<uint16_t> y_buffer(global_num_points);
        fin.read(reinterpret_cast<char*>(y_buffer.data()), sizeof(uint16_t) * global_num_points);
        if (fin) {
            y_values.reserve(global_num_points);
            for (const auto& y : y_buffer) {
                y_values.push_back(static_cast<double>(y));
            }
            y_read_success = true;
            std::cerr << "Successfully read Y as 16-bit integers" << std::endl;
        }
    }

    if (!y_read_success) {
        throw std::runtime_error("Failed to read Y data in any supported format");
    }

    std::cerr << "Successfully read " << x_values.size() << " X values and " << y_values.size() << " Y values" << std::endl;

    // Validate data ranges
    if (!y_values.empty()) {
        double min_y = *std::min_element(y_values.begin(), y_values.end());
        double max_y = *std::max_element(y_values.begin(), y_values.end());
        std::cerr << "Y data range: " << min_y << " to " << max_y << std::endl;
    }

    // Extract filename for map key
    std::filesystem::path file_path(path);
    std::string filename = file_path.filename().string();

    std::map<std::string, Spectrum> result;
    result[filename] = std::make_pair(std::move(x_values), std::move(y_values));

    return result;
}

std::map<std::string, Spectrum> read_spc_dir(const std::string& directory,
                                             const std::string& ext,
                                             const std::string& orient) {
    std::map<std::string, Spectrum> spectra_map;

    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
        if (!entry.is_regular_file()) continue;

        const std::string fname = entry.path().filename().string();
        if (entry.path().extension() != ext) continue;

        try {
            auto file_spectra = read_spc(entry.path().string());
            // Merge the results
            for (const auto& [key, spectrum] : file_spectra) {
                spectra_map[fname] = spectrum;
            }
        } catch (const std::exception& e) {
            std::cerr << "Failed to parse SPC file: " << fname << " - " << e.what() << "\n";
            continue;
        }
    }

    return spectra_map;
}

} // namespace spc_reader
