
// Boost Includes ==============================================================
#include <boost/python.hpp>
#include <boost/cstdint.hpp>

// Includes ====================================================================
#include <Magick++/Pixels.h>

// Using =======================================================================
using namespace boost::python;

// Module ======================================================================
void Export_pyste_src_Pixels()
{
    class_< Magick::Pixels, boost::noncopyable >("Pixels", init< Magick::Image& >())
        .def("sync", &Magick::Pixels::sync)
        .def("x", &Magick::Pixels::x)
        .def("y", &Magick::Pixels::y)
        .def("columns", &Magick::Pixels::columns)
        .def("rows", &Magick::Pixels::rows)
    ;

}

