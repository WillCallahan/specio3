#pragma once

#include <vector>
#include <string>
#include <utility>
#include <map>

namespace spc_reader {

// Type alias for a spectrum: pair of (x_values, y_values)
using Spectrum = std::pair<std::vector<double>, std::vector<double>>;

/**
 * Read SPC spectral file and return map of filename to spectrum.
 *
 * @param path Path to the SPC file to read
 * @return Map with filename as key and spectrum as (x_array, y_array) pair
 * @throws std::runtime_error if file cannot be read or has invalid format
 */
std::map<std::string, Spectrum> read_spc(const std::string &path);

/**
 * Read multiple SPC files from a directory.
 *
 * @param directory Path to directory containing SPC files
 * @param ext File extension to look for (default ".spc")
 * @param orient Orientation (currently unused, for compatibility)
 * @return Map of filename to spectrum pairs
 */
std::map<std::string, Spectrum> read_spc_dir(const std::string& directory,
                                             const std::string& ext = ".spc",
                                             const std::string& orient = "Row");

} // namespace spc_reader
