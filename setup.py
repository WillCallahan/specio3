import sys

from pybind11.setup_helpers import Pybind11Extension, build_ext
from setuptools import setup

# Configure the compiler for C++11 or later
extra_compile_args = []
extra_link_args = []

# Add appropriate flags based on the platform
if sys.platform == 'darwin':  # macOS
    extra_compile_args = ['-std=c++17', '-O3']
    extra_link_args = ['-stdlib=libc++']

ext_modules = [
    Pybind11Extension(
        "specio3._specio3",  # Top-level module
        ["specio3/spc_reader.cpp", "specio3/bindings.cpp"],
        # include_dirs = [pybind11.get_include(), pybind11.get_include(user = True)],
        language = "c++",
        extra_compile_args = extra_compile_args,
        extra_link_args = extra_link_args,
    ),
]

if __name__ == "__main__":
    setup(
        name = "specio3",
        version = "0.1.0",
        packages = ["specio3"],
        cmdclass = {"build_ext": build_ext},
        zip_safe = False,
        ext_modules = ext_modules,
    )
