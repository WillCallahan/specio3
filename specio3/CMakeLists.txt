cmake_minimum_required(VERSION 3.31)
project(specio3)

set(CMAKE_CXX_STANDARD 17)

include_directories(.)

# Set Python3_EXECUTABLE to Poetry virtualenv Python
set(Python3_EXECUTABLE "/Users/william/Library/Caches/pypoetry/virtualenvs/specio3-q6-wVR9k-py3.12/bin/python")
find_package(Python3 COMPONENTS Interpreter Development REQUIRED)

# Add pybind11 include directory manually (header-only usage)
include_directories("/Users/william/Library/Caches/pypoetry/virtualenvs/specio3-q6-wVR9k-py3.12/lib/python3.12/site-packages/pybind11/include")

# For numpy include dir
execute_process(
        COMMAND ${Python3_EXECUTABLE} -c "import numpy; print(numpy.get_include())"
        OUTPUT_VARIABLE NumPy_INCLUDE_DIR
        OUTPUT_STRIP_TRAILING_WHITESPACE
)

include_directories(${NumPy_INCLUDE_DIR})

add_library(_specio3 MODULE bindings.cpp spc_reader.cpp)
