#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include "spc_reader.h"

// Windows compatibility: MSVC doesn't have ssize_t
#ifdef _WIN32
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;
#endif

namespace py = pybind11;

py::list read_spc_py(const std::string &path) {
    auto spectra = read_spc_multi(path);
    py::list out;
    for (const auto &spec : spectra) {
        const auto &x = spec.first;
        const auto &y = spec.second;
        ssize_t n = static_cast<ssize_t>(x.size());
        py::array_t<double> x_arr(n);
        py::array_t<double> y_arr(n);
        auto x_buf = x_arr.mutable_unchecked<1>();
        auto y_buf = y_arr.mutable_unchecked<1>();
        for (ssize_t i = 0; i < n; ++i) {
            x_buf(i) = x[i];
            y_buf(i) = y[i];
        }
        out.append(py::make_tuple(x_arr, y_arr));
    }
    return out;
}

PYBIND11_MODULE(_specio3, m) {
    m.doc() = "SPC spectral reader exposed to Python (returns list of (x,y) numpy arrays)";
    m.def("read_spc", &read_spc_py, "Read SPC file and return list of (x,y) arrays");
}
