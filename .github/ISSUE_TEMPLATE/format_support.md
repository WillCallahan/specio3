---
name: File format support
about: Request support for a specific SPC file format or report format-related issues
title: '[FORMAT] '
labels: format-support
assignees: ''

---

**File format details**
- SPC format version: [e.g. unknown, if known specify]
- Instrument/Software: [e.g. Thermo OMNIC, PerkinElmer Spectrum, etc.]
- File characteristics:
  - [ ] Single spectrum
  - [ ] Multi-spectrum
  - [ ] XY data (explicit X-axis)
  - [ ] Y-only data (calculated X-axis)
  - [ ] 16-bit Y data
  - [ ] 32-bit Y data
  - [ ] Float Y data

**Current behavior**
What happens when you try to read the file with specio3:
- [ ] File loads but data appears incorrect
- [ ] File fails to load with error
- [ ] File loads partially
- [ ] Other: [describe]

**Error message (if applicable)**
```
Paste any error messages here
```

**Sample file**
- [ ] I can provide a sample file for testing
- [ ] File contains sensitive data but I can provide a sanitized version
- [ ] Cannot share file due to confidentiality

**Expected behavior**
Describe what you expected to happen and how the data should be interpreted.

**File information**
If you can share details about the file structure:
- File size: [e.g. 2.3 MB]
- Number of spectra: [e.g. 1, 50, unknown]
- Spectral range: [e.g. 4000-400 cm⁻¹, 200-800 nm]
- Number of data points per spectrum: [e.g. 1024, 2048]

**Additional context**
Any other software that can successfully read this file format, or additional details about the file structure.
