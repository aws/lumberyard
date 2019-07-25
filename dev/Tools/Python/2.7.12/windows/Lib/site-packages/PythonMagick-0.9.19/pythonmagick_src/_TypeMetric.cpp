
// Boost Includes ==============================================================
#include <boost/python.hpp>
#include <boost/cstdint.hpp>

// Includes ====================================================================
#include <Magick++/TypeMetric.h>

// Using =======================================================================
using namespace boost::python;

// Module ======================================================================
void Export_pyste_src_TypeMetric()
{
    class_< Magick::TypeMetric, boost::noncopyable >("TypeMetric", init<  >())
        .def("ascent", &Magick::TypeMetric::ascent)
        .def("descent", &Magick::TypeMetric::descent)
        .def("textWidth", &Magick::TypeMetric::textWidth)
        .def("textHeight", &Magick::TypeMetric::textHeight)
        .def("maxHorizontalAdvance", &Magick::TypeMetric::maxHorizontalAdvance)
    ;

}

