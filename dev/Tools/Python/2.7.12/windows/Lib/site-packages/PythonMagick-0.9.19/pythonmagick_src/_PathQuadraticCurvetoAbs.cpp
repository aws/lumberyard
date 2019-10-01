
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

struct Magick_PathQuadraticCurvetoAbs_Wrapper: Magick::PathQuadraticCurvetoAbs
{
    Magick_PathQuadraticCurvetoAbs_Wrapper(PyObject* py_self_, const Magick::PathQuadraticCurvetoArgs& p0):
        Magick::PathQuadraticCurvetoAbs(p0), py_self(py_self_) {}

    Magick_PathQuadraticCurvetoAbs_Wrapper(PyObject* py_self_, const Magick::PathQuadraticCurvetoArgsList& p0):
        Magick::PathQuadraticCurvetoAbs(p0), py_self(py_self_) {}

    Magick_PathQuadraticCurvetoAbs_Wrapper(PyObject* py_self_, const Magick::PathQuadraticCurvetoAbs& p0):
        Magick::PathQuadraticCurvetoAbs(p0), py_self(py_self_) {}


    PyObject* py_self;
};


}// namespace 


// Module ======================================================================
void Export_pyste_src_PathQuadraticCurvetoAbs()
{
    class_< Magick::PathQuadraticCurvetoAbs, Magick_PathQuadraticCurvetoAbs_Wrapper >("PathQuadraticCurvetoAbs", init< const Magick::PathQuadraticCurvetoArgs& >())
        .def(init< const Magick::PathQuadraticCurvetoArgsList& >())
        .def(init< const Magick::PathQuadraticCurvetoAbs& >())
    ;

}

