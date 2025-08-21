# specio3

[![CI/CD Pipeline](https://github.com/WillCallahan/specio3/workflows/CI/CD%20Pipeline/badge.svg)](https://github.com/WillCallahan/specio3/actions)
[![PyPI version](https://badge.fury.io/py/specio3.svg)](https://badge.fury.io/py/specio3)
[![Python versions](https://img.shields.io/pypi/pyversions/specio3.svg)](https://pypi.org/project/specio3/)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Code style: black](https://img.shields.io/badge/code%20style-black-000000.svg)](https://github.com/psf/black)

A high-performance Python library for reading Galactic SPC (Spectral Data Collection) files. Built with C++ for speed and wrapped with pybind11 for seamless Python integration.

## Features

- **🚀 Fast C++ Implementation**: High-performance spectral file parsing
- **📊 Multiple SPC Formats**: Supports single-spectrum and multi-spectrum files
- **🔧 Format Variants**: Handles Y-only, XY, XYY, and XYXY file types
- **🐍 Pythonic API**: Clean, well-typed interface with comprehensive documentation
- **📈 NumPy Integration**: Returns data as numpy arrays for easy analysis
- **🛡️ Robust Error Handling**: Comprehensive validation and clear error messages
- **🔄 Cross-Platform**: Works on Linux, macOS, and Windows
- **📦 Easy Installation**: Available on PyPI with pre-built wheels

## Installation

### From PyPI (recommended)

```bash
pip install specio3
```

### From source

```bash
git clone https://github.com/WillCallahan/specio3.git
cd specio3
pip install .
```

### Development installation

```bash
git clone https://github.com/WillCallahan/specio3.git
cd specio3
poetry install
```

## Quick Start

```python
import specio3
import matplotlib.pyplot as plt

# Read an SPC file
spectra = specio3.read_spc('your_file.spc')

# Get the first spectrum
x_values, y_values = spectra[0]

# Plot the spectrum
plt.figure(figsize=(10, 6))
plt.plot(x_values, y_values)
plt.xlabel('Wavenumber (cm⁻¹)')
plt.ylabel('Intensity')
plt.title('Spectral Data')
plt.show()

print(f"Loaded {len(spectra)} spectra")
print(f"First spectrum has {len(x_values)} data points")
```

## API Reference

### `read_spc(path: str) -> List[Tuple[NDArray[np.float64], NDArray[np.float64]]]`

Read SPC spectral file and return list of (x,y) arrays.

**Parameters:**

- `path` (str): Path to the SPC file to read

**Returns:**

- `List[Tuple[NDArray[np.float64], NDArray[np.float64]]]`: List of (x_array, y_array) tuples

**Raises:**

- `FileNotFoundError`: If the specified file does not exist
- `RuntimeError`: If the file cannot be read or has an unsupported format
- `PermissionError`: If the file cannot be read due to insufficient permissions

## Supported SPC File Types

| Format | Description          | X-axis         | Y-axis         | Multiple Spectra |
|--------|----------------------|----------------|----------------|------------------|
| Y-only | Evenly spaced X-axis | ✅ Generated    | ✅ Stored       | ✅                |
| XY     | Explicit X values    | ✅ Stored       | ✅ Stored       | ❌                |
| XYY    | Shared X-axis        | ✅ Shared       | ✅ Per spectrum | ✅                |
| XYXY   | Per-spectrum X-axis  | ✅ Per spectrum | ✅ Per spectrum | ✅                |

## Advanced Usage

### Processing Multiple Spectra

```python
import specio3
import numpy as np

# Read multi-spectrum file
spectra = specio3.read_spc('multi_spectrum.spc')

# Process each spectrum
for i, (x, y) in enumerate(spectra):
    # Find peak
    peak_idx = np.argmax(y)
    peak_wavenumber = x[peak_idx]
    peak_intensity = y[peak_idx]

    print(f"Spectrum {i}: Peak at {peak_wavenumber:.2f} cm⁻¹ with intensity {peak_intensity:.6f}")
```

### Error Handling

```python
import specio3

try:
    spectra = specio3.read_spc('example.spc')
    print(f"Successfully loaded {len(spectra)} spectra")
except FileNotFoundError:
    print("SPC file not found")
except RuntimeError as e:
    print(f"Error reading SPC file: {e}")
except Exception as e:
    print(f"Unexpected error: {e}")
```

### Integration with Scientific Libraries

```python
import specio3
import numpy as np
import pandas as pd
from scipy import signal

# Read spectrum
spectra = specio3.read_spc('example.spc')
x, y = spectra[0]

# Create DataFrame for analysis
df = pd.DataFrame({'wavenumber': x, 'intensity': y})

# Apply smoothing
y_smooth = signal.savgol_filter(y, window_length=5, polyorder=2)

# Find peaks
peaks, _ = signal.find_peaks(y_smooth, height=0.1)
peak_wavenumbers = x[peaks]

print(f"Found {len(peaks)} peaks at: {peak_wavenumbers}")
```

## Performance

specio3 is designed for high performance:

- **C++ Core**: Spectral data parsing implemented in optimized C++
- **Memory Efficient**: Minimal memory overhead with direct numpy array creation
- **Fast I/O**: Efficient binary file reading with proper buffering
- **Type Safety**: Full type hints for optimal IDE support and static analysis

### Benchmarks

Performance benchmarks on real SPC files from the test suite (macOS M1, 12 CPU cores):

| File Category | File Size | Data Points | Load Time | Throughput | Memory Usage |
|---------------|-----------|-------------|-----------|------------|--------------|
| Medium files  | ~131 KB   | ~33,000     | ~2.0 ms   | ~65 MB/s   | ~0.1 MB      |
| Large files   | ~258 KB   | ~66,000     | ~4.0 ms   | ~64 MB/s   | ~0.2 MB      |

#### Comparison with spectrochempy

specio3 significantly outperforms the reference implementation from [spectrochempy](https://github.com/spectrochempy/spectrochempy):

| File | Size | specio3 | spectrochempy | Speedup |
|------|------|---------|---------------|---------|
| 103b4anh.spc | 131 KB | 2.4 ms | 8.1 ms | **3.4x faster** |
| 087b4ana.spc | 258 KB | 4.1 ms | 14.2 ms | **3.4x faster** |
| 040b4ana.spc | 131 KB | 2.0 ms | 7.8 ms | **3.9x faster** |

**Average performance: 3.6x faster than spectrochempy**

*Run `python scripts/benchmark.py` to reproduce these results on your system.*

## Contributing

We welcome contributions! Please see our [Contributing Guidelines](CONTRIBUTING.md) for details.

### Development Setup

1. Clone the repository:

```bash
git clone https://github.com/WillCallahan/specio3.git
cd specio3
```

2. Install Poetry (if not already installed):

```bash
curl -sSL https://install.python-poetry.org | python3 -
```

3. Install dependencies:

```bash
poetry install
```

4. Build the C++ extension:

```bash
poetry run rebuild
```

5. Run tests:

```bash
poetry run test
```

### Running Tests

```bash
# Run all tests
poetry run test

# Run with coverage
poetry run coverage

# Run specific test file
poetry run pytest tests/spc_reader_test.py -v
```

## Building from Source

### Requirements

- Python 3.10+
- C++14 compatible compiler
- CMake (for building wheels)
- NumPy
- pybind11

### Build Process

```bash
# Install build dependencies
pip install build wheel

# Build source distribution and wheel
python -m build

# Install locally
pip install dist/specio3-*.whl
```

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Changelog

### v0.1.2 (2025-08-08)

- **Performance**: Optimized GitHub Actions wheel builds (5.6x faster)
- **Security**: Comprehensive security audit and vulnerability fixes
- **Organization**: Improved project structure with docs/ and scripts/ directories
- **Testing**: Enhanced test reliability and removed .DS_Store from test output
- **CI/CD**: Fixed wheel platform tags and deployment issues

### v0.1.0 (2025-08-02)

- Initial release
- Support for major SPC file variants
- High-performance C++ implementation
- Complete Python API with type hints
- Comprehensive test suite
- Multi-platform support (Linux, macOS, Windows)

## Acknowledgments

- Built with [pybind11](https://github.com/pybind/pybind11) for Python-C++ bindings
- Uses [NumPy](https://numpy.org/) for efficient array operations
- Developed with [Poetry](https://python-poetry.org/) for dependency management
- Tested with Spectral Data from the [EPA Spectra](https://www3.epa.gov/ttn/emc/ftir/data.html) data set
- Performance benchmarks compared against [spectrochempy](https://github.com/spectrochempy/spectrochempy) - the reference Python implementation for SPC file reading

## Project Structure

```
specio3/
├── specio3/           # Main Python package
│   ├── __init__.py    # Package initialization and public API
│   ├── __main__.py    # Command-line interface
│   ├── *.cpp          # C++ implementation files
│   └── *.h            # C++ header files
├── tests/             # Test suite
│   ├── data/          # Test SPC files
│   └── *.py           # Test modules
├── scripts/           # Utility scripts
│   ├── benchmark.py   # Performance benchmarking
│   └── benchmark_comparison.py  # Comparison with other libraries
├── docs/              # Documentation
│   ├── spc-specification.pdf    # SPC file format specification
│   └── SECURITY_AUDIT.md        # Security analysis and recommendations
└── README.md          # This file
```

## Support

- **Documentation**: [Full API documentation](https://WillCallahan.github.io/specio3/)
- **Issues**: [GitHub Issues](https://github.com/WillCallahan/specio3/issues)
- **Discussions**: [GitHub Discussions](https://github.com/WillCallahan/specio3/discussions)

---

**Made with ❤️ for the spectroscopy community**
