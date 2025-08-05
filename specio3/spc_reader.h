#pragma once

#include <pybind11/pybind11.h>
#include <vector>
#include <string>
#include <fstream>

namespace py = pybind11;

// Forward declarations
struct Subfile;
struct SPCFile;

/**
 * Helper function to convert file offset to human-readable string.
 *
 * @param o Stream offset to convert
 * @return String representation of the offset
 */
std::string human_offset(std::streamoff o);

/**
 * Template helper function to read little-endian values from buffer.
 * Assumes host is little-endian; add byte-swapping if needed for big-endian hosts.
 *
 * @tparam T Type to read (uint16_t, uint32_t, float, double, etc.)
 * @param buffer Pointer to buffer containing the data
 * @return The value read from buffer in host byte order
 */
template <typename T>
T read_le(const char* buffer);

/**
 * Structure representing a single spectrum subfile.
 * Contains X and Y data vectors along with Z-axis metadata.
 */
struct Subfile {
    std::vector<double> x;      ///< X-axis values (wavelength, frequency, etc.)
    std::vector<double> y;      ///< Y-axis values (intensity, absorbance, etc.)
    float z_start = 0;          ///< Starting Z-axis value for this subfile
    float z_end = 0;            ///< Ending Z-axis value for this subfile
};

/**
 * Structure representing a complete SPC file with all metadata and spectral data.
 * Supports various SPC format variants including single/multi-file and different data types.
 */
struct SPCFile {
    // File format flags
    bool is_multifile = false;  ///< True if file contains multiple spectra
    bool is_xy = false;         ///< True if file contains explicit X-axis data
    bool is_xyxy = false;       ///< True if each subfile has its own X-axis data
    bool y_in_16bit = false;    ///< True if Y values are stored as 16-bit integers

    // Global file metadata
    uint32_t num_points = 0;    ///< Number of data points per spectrum (if not XYXY)
    uint32_t num_subfiles = 0;  ///< Number of spectra in the file
    double first_x = 0;         ///< First X-axis value (for Y-only files)
    double last_x = 0;          ///< Last X-axis value (for Y-only files)

    // Spectral data and log information
    std::vector<Subfile> subfiles;  ///< Vector of all spectra in the file
    std::string log_text;           ///< Optional log text from the file
};

/**
 * Apply Y-axis scaling for 32-bit integer values according to SPC specification.
 * Uses the exponent byte to scale raw integer values to floating point.
 *
 * @param integer_y Raw 32-bit integer Y value from file
 * @param exponent_byte Signed exponent byte from header (-128 indicates float data)
 * @param is_16bit Whether this is 16-bit data (affects divisor calculation)
 * @return Scaled floating point Y value
 */
double apply_y_scaling_uint32(uint32_t integer_y, int8_t exponent_byte, bool is_16bit);

/**
 * Apply Y-axis scaling for 16-bit integer values according to SPC specification.
 * Uses the exponent byte to scale raw integer values to floating point.
 *
 * @param integer_y Raw 16-bit integer Y value from file
 * @param exponent_byte Signed exponent byte from header (-128 indicates float data)
 * @return Scaled floating point Y value
 */
double apply_y_scaling_uint16(uint16_t integer_y, int8_t exponent_byte);

/**
 * Read and parse a complete SPC file into memory.
 * Handles all SPC format variants including single/multi-file, Y-only/XY/XYXY formats,
 * and different data precision levels (16-bit/32-bit integers, floats).
 *
 * @param filename Path to the SPC file to read
 * @return SPCFile structure containing all parsed data and metadata
 * @throws std::runtime_error if file cannot be opened, read, or contains invalid data
 * @throws std::runtime_error if file format is unsupported or corrupted
 */
SPCFile read_spc_impl(const std::string& filename);

/**
 * Convert SPCFile structure to Python dictionary for pybind11 bindings.
 * Creates a nested dictionary structure containing all file metadata and spectral data
 * that can be easily accessed from Python code.
 *
 * The returned dictionary contains:
 * - File format flags (is_multifile, is_xy, is_xyxy, y_in_16bit)
 * - Global metadata (num_points, num_subfiles, first_x, last_x)
 * - Log text string
 * - List of subfiles, each containing x/y vectors and z_start/z_end values
 *
 * @param spc SPCFile structure to convert
 * @return Python dictionary containing all SPC file data
 */
py::dict to_pydict(const SPCFile& spc);
