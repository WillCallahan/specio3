#pragma once

#include <string>
#include <vector>
#include <utility>

/// One spectrum's X and Y arrays.
using Spectrum = std::pair<std::vector<double>, std::vector<double>>;

/// Reads an SPC file (single or multifile, common variants) and returns all spectra.
/// Throws std::runtime_error on unsupported formats or I/O errors.
std::vector<Spectrum> read_spc_multi(const std::string &path);
