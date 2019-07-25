
// Boost Includes ==============================================================
#include <boost/python.hpp>
#include <boost/cstdint.hpp>

// Includes ====================================================================
#include <Magick++/Drawable.h>

// Declarations ================================================================
#include <Magick++.h>

// Using =======================================================================
using namespace boost::python;

// Module ======================================================================
void Export_pyste_src_PathArcArgs()
{
    class_< Magick::PathArcArgs >("PathArcArgs", init<  >())
        .def(init< double, double, double, bool, bool, double, double >())
        .def(init< const Magick::PathArcArgs& >())
        .def("radiusX", (void (Magick::PathArcArgs::*)(double) )&Magick::PathArcArgs::radiusX)
        .def("radiusX", (double (Magick::PathArcArgs::*)() const)&Magick::PathArcArgs::radiusX)
        .def("radiusY", (void (Magick::PathArcArgs::*)(double) )&Magick::PathArcArgs::radiusY)
        .def("radiusY", (double (Magick::PathArcArgs::*)() const)&Magick::PathArcArgs::radiusY)
        .def("xAxisRotation", (void (Magick::PathArcArgs::*)(double) )&Magick::PathArcArgs::xAxisRotation)
        .def("xAxisRotation", (double (Magick::PathArcArgs::*)() const)&Magick::PathArcArgs::xAxisRotation)
        .def("largeArcFlag", (void (Magick::PathArcArgs::*)(bool) )&Magick::PathArcArgs::largeArcFlag)
        .def("largeArcFlag", (bool (Magick::PathArcArgs::*)() const)&Magick::PathArcArgs::largeArcFlag)
        .def("sweepFlag", (void (Magick::PathArcArgs::*)(bool) )&Magick::PathArcArgs::sweepFlag)
        .def("sweepFlag", (bool (Magick::PathArcArgs::*)() const)&Magick::PathArcArgs::sweepFlag)
        .def("x", (void (Magick::PathArcArgs::*)(double) )&Magick::PathArcArgs::x)
        .def("x", (double (Magick::PathArcArgs::*)() const)&Magick::PathArcArgs::x)
        .def("y", (void (Magick::PathArcArgs::*)(double) )&Magick::PathArcArgs::y)
        .def("y", (double (Magick::PathArcArgs::*)() const)&Magick::PathArcArgs::y)
        .def( self != self )
        .def( self > self )
        .def( self <= self )
        .def( self >= self )
        .def( self < self )
        .def( self == self )
    ;

}

