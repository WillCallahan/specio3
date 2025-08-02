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

py::list read_spc_py(const std::string &path) {
    auto spectra_map = spc_reader::read_spc(path);
    py::list out;

    for (const auto &item : spectra_map) {
        const auto &spectrum = item.second;
        ssize_t n = static_cast<ssize_t>(spectrum.first.size());

        py::array_t<double> x_arr(n);
        py::array_t<double> y_arr(n);

        auto x_buf = x_arr.mutable_unchecked<1>();
        auto y_buf = y_arr.mutable_unchecked<1>();

        for (ssize_t i = 0; i < n; ++i) {
            x_buf(i) = spectrum.first[i];   // x values
            y_buf(i) = spectrum.second[i];  // y values
        }

        out.append(py::make_tuple(x_arr, y_arr));
    }
    return out;
}

PYBIND11_MODULE(_specio3, m) {
    m.doc() = "SPC spectral reader exposed to Python (returns list of (x,y) numpy arrays)";
    m.def("read_spc", &read_spc_py,
          py::arg("path"),
          R"pbdoc(
            Read a single .spc file and return a list of (x,y) numpy array tuples.
          )pbdoc");
}
