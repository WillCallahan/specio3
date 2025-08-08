"""SPC spectral file reader with type hints."""
from typing import List, Tuple
import numpy as np
from numpy.typing import NDArray
from ._specio3 import read_spc as _read_spc

def read_spc(path: str) -> List[Tuple[NDArray[np.float64], NDArray[np.float64]]]:
    """
    Read SPC spectral file and return list of (x,y) arrays.

    This function reads Galactic SPC files (both single-spectrum and multi-spectrum)
    and returns the spectral data as numpy arrays. Supports various SPC format
    variants including Y-only, XY, and multifile formats.

    Parameters
    ----------
    path : str
        Path to the SPC file to read. Must be a valid file path with read permissions.

    Returns
    -------
    List[Tuple[NDArray[np.float64], NDArray[np.float64]]]
        List of (x_array, y_array) tuples, where each tuple represents one spectrum.

        - x_array : 1D numpy array of float64 values representing the X-axis (e.g., wavelength, frequency)
        - y_array : 1D numpy array of float64 values representing the Y-axis (e.g., intensity, absorbance)

        Both arrays have the same length and are guaranteed to be non-empty.
        For single-spectrum files, the list contains one tuple.
        For multi-spectrum files, the list contains multiple tuples.

    Raises
    ------
    FileNotFoundError
        If the specified file does not exist or cannot be accessed.
    RuntimeError
        If the file cannot be read, has an unsupported format, contains invalid data,
        or if arrays have mismatched lengths.
    PermissionError
        If the file exists but cannot be read due to insufficient permissions.

    See Also
    --------
    numpy.loadtxt : For reading simple text-based spectral files
    scipy.io : For other scientific data formats

    Notes
    -----
    The function supports the following SPC file variants:

    - Single-spectrum Y-only files (evenly spaced X-axis)
    - Single-spectrum XY files (explicit X-axis values)
    - Multi-spectrum files with shared X-axis
    - Multi-spectrum files with per-spectrum X-axis (XYXY format)

    All data is converted to float64 precision for consistency.

    Examples
    --------
    Read a single-spectrum SPC file:

    >>> import specio3
    >>> spectra = specio3.read_spc('single_spectrum.spc')
    >>> x_vals, y_vals = spectra[0]  # First (and only) spectrum
    >>> print(f"Spectrum has {len(x_vals)} points")
    Spectrum has 1024 points

    Read a multi-spectrum SPC file:

    >>> spectra = specio3.read_spc('multi_spectrum.spc')
    >>> print(f"File contains {len(spectra)} spectra")
    File contains 5 spectra
    >>> for i, (x, y) in enumerate(spectra):
    ...     print(f"Spectrum {i}: {len(x)} points, range {x[0]:.1f}-{x[-1]:.1f}")
    Spectrum 0: 512 points, range 400.0-700.0
    Spectrum 1: 512 points, range 400.0-700.0
    ...

    Process the spectral data:

    >>> import numpy as np
    >>> import matplotlib.pyplot as plt
    >>> spectra = specio3.read_spc('example.spc')
    >>> x, y = spectra[0]
    >>> max_intensity = np.max(y)
    >>> peak_wavelength = x[np.argmax(y)]
    >>> plt.plot(x, y)
    >>> plt.xlabel('Wavelength (nm)')
    >>> plt.ylabel('Intensity')
    >>> plt.show()
    """
    result_dict = _read_spc(path)

    # Validate the result structure
    if not isinstance(result_dict, dict):
        raise RuntimeError("Expected a dictionary from the SPC file reader.")

    if 'subfiles' not in result_dict:
        raise RuntimeError("No subfiles found in the SPC file.")

    subfiles = result_dict['subfiles']
    if not subfiles:
        raise RuntimeError("No spectra found in the SPC file.")

    # Convert to list of (x, y) tuples with numpy arrays
    result = []
    for i, subfile in enumerate(subfiles):
        if not isinstance(subfile, dict) or 'x' not in subfile or 'y' not in subfile:
            raise RuntimeError(f"Spectrum {i}: Expected a dictionary with 'x' and 'y' keys.")

        x_data = subfile['x']
        y_data = subfile['y']
        
        # Convert to numpy arrays
        x_arr = np.array(x_data, dtype=np.float64)
        y_arr = np.array(y_data, dtype=np.float64)

        if len(x_arr) != len(y_arr):
            raise RuntimeError(f"Spectrum {i}: x and y arrays have different lengths ({len(x_arr)} vs {len(y_arr)}).")

        if len(x_arr) == 0:
            raise RuntimeError(f"Spectrum {i}: Empty spectrum data.")

        result.append((x_arr, y_arr))

    return result

__all__ = ['read_spc']