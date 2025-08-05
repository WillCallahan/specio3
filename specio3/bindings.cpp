#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>
#include "spc_reader.h"

// Windows compatibility: MSVC doesn't have ssize_t
#ifdef _WIN32
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;
#endif

namespace py = pybind11;

PYBIND11_MODULE(_specio3, m) {
    m.doc() = "SPC file reader with corrected subheader and exponent handling";

    m.def("read_spc", [](const std::string& filename) {
        SPCFile spc;
        try {
            spc = read_spc_impl(filename);
        } catch (const std::exception& e) {
            std::ostringstream msg;
            msg << "Error in read_spc_impl: " << e.what();
            throw std::runtime_error(msg.str());
        }
        return to_pydict(spc);
    }, py::arg("filename"), "Read an SPC file and return its contents as a Python dict");
}
