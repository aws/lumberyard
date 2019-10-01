
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

struct Magick_PathQuadraticCurvetoRel_Wrapper: Magick::PathQuadraticCurvetoRel
{
    Magick_PathQuadraticCurvetoRel_Wrapper(PyObject* py_self_, const Magick::PathQuadraticCurvetoArgs& p0):
        Magick::PathQuadraticCurvetoRel(p0), py_self(py_self_) {}

    Magick_PathQuadraticCurvetoRel_Wrapper(PyObject* py_self_, const Magick::PathQuadraticCurvetoArgsList& p0):
        Magick::PathQuadraticCurvetoRel(p0), py_self(py_self_) {}

    Magick_PathQuadraticCurvetoRel_Wrapper(PyObject* py_self_, const Magick::PathQuadraticCurvetoRel& p0):
        Magick::PathQuadraticCurvetoRel(p0), py_self(py_self_) {}


    PyObject* py_self;
};


}// namespace 


// Module ======================================================================
void Export_pyste_src_PathQuadraticCurvetoRel()
{
    class_< Magick::PathQuadraticCurvetoRel, Magick_PathQuadraticCurvetoRel_Wrapper >("PathQuadraticCurvetoRel", init< const Magick::PathQuadraticCurvetoArgs& >())
        .def(init< const Magick::PathQuadraticCurvetoArgsList& >())
        .def(init< const Magick::PathQuadraticCurvetoRel& >())
    ;

}

