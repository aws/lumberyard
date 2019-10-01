
// Boost Includes ==============================================================
#include <boost/python.hpp>
#include <boost/cstdint.hpp>

// Includes ====================================================================
#include <Magick++/Drawable.h>

// Using =======================================================================
using namespace boost::python;

// Module ======================================================================
void Export_pyste_src_Coordinate()
{
    class_< Magick::Coordinate, boost::noncopyable >("Coordinate", init<  >())
        .def(init< double, double >())
        .def("x", (void (Magick::Coordinate::*)(double) )&Magick::Coordinate::x)
        .def("x", (double (Magick::Coordinate::*)() const)&Magick::Coordinate::x)
        .def("y", (void (Magick::Coordinate::*)(double) )&Magick::Coordinate::y)
        .def("y", (double (Magick::Coordinate::*)() const)&Magick::Coordinate::y)
        .def( self >= self )
        .def( self <= self )
        .def( self > self )
        .def( self < self )
        .def( self == self )
        .def( self != self )
    ;

}

