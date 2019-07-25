
// Boost Includes ==============================================================
#include <boost/python.hpp>
#include <boost/cstdint.hpp>

// Includes ====================================================================
#include <Magick++/Drawable.h>

// Declarations ================================================================
#include <Magick++.h>

// Using =======================================================================
using namespace boost::python;

namespace  {

struct Magick_PathSmoothQuadraticCurvetoRel_Wrapper: Magick::PathSmoothQuadraticCurvetoRel
{
    Magick_PathSmoothQuadraticCurvetoRel_Wrapper(PyObject* py_self_, const Magick::Coordinate& p0):
        Magick::PathSmoothQuadraticCurvetoRel(p0), py_self(py_self_) {}

    Magick_PathSmoothQuadraticCurvetoRel_Wrapper(PyObject* py_self_, const Magick::CoordinateList& p0):
        Magick::PathSmoothQuadraticCurvetoRel(p0), py_self(py_self_) {}

    Magick_PathSmoothQuadraticCurvetoRel_Wrapper(PyObject* py_self_, const Magick::PathSmoothQuadraticCurvetoRel& p0):
        Magick::PathSmoothQuadraticCurvetoRel(p0), py_self(py_self_) {}


    PyObject* py_self;
};


}// namespace 


// Module ======================================================================
void Export_pyste_src_PathSmoothQuadraticCurvetoRel()
{
    class_< Magick::PathSmoothQuadraticCurvetoRel, Magick_PathSmoothQuadraticCurvetoRel_Wrapper >("PathSmoothQuadraticCurvetoRel", init< const Magick::Coordinate& >())
        .def(init< const Magick::CoordinateList& >())
        .def(init< const Magick::PathSmoothQuadraticCurvetoRel& >())
    ;

}

