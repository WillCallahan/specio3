---
name: Bug report
about: Create a report to help us improve specio3
title: '[BUG] '
labels: bug
assignees: ''

---

**Describe the bug**
A clear and concise description of what the bug is.

**To Reproduce**
Steps to reproduce the behavior:
1. Import specio3
2. Call function with parameters '...'
3. Use file '...'
4. See error

**Expected behavior**
A clear and concise description of what you expected to happen.

**Error message/traceback**
If applicable, paste the full error message or traceback here:
```
Paste error message here
```

**Sample file**
If the issue is related to a specific SPC file:
- [ ] I can share the problematic SPC file
- [ ] The file contains sensitive data and cannot be shared
- [ ] The issue occurs with multiple files

**Environment (please complete the following information):**
- OS: [e.g. Windows 10, macOS 14.6, Ubuntu 22.04]
- Python version: [e.g. 3.10.12]
- specio3 version: [e.g. 0.1.0]
- Installation method: [e.g. pip, conda, from source]

**File information (if applicable)**
If the issue is with a specific SPC file, please provide:
- File size: [e.g. 45 KB]
- File type: [e.g. single spectrum, multi-spectrum]
- Source instrument/software: [e.g. Thermo OMNIC, if known]

**Additional context**
Add any other context about the problem here. Include any workarounds you've found.

**Debugging information**
If you can run the following code, please paste the output:
```python
import specio3
import sys
import platform

print(f"Python: {sys.version}")
print(f"Platform: {platform.platform()}")
print(f"specio3 functions: {dir(specio3)}")

# If you can load the problematic file:
try:
    data = specio3.read_spc("your_file.spc")
    print(f"Successfully loaded {len(data)} spectra")
    for i, (x, y) in enumerate(data[:3]):  # First 3 spectra only
        print(f"Spectrum {i}: {len(x)} points, X range: {x[0]:.3f} to {x[-1]:.3f}")
except Exception as e:
    print(f"Error: {e}")
```
