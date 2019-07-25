
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

struct Magick_PathCurvetoRel_Wrapper: Magick::PathCurvetoRel
{
    Magick_PathCurvetoRel_Wrapper(PyObject* py_self_, const Magick::PathCurvetoArgs& p0):
        Magick::PathCurvetoRel(p0), py_self(py_self_) {}

    Magick_PathCurvetoRel_Wrapper(PyObject* py_self_, const Magick::PathCurveToArgsList& p0):
        Magick::PathCurvetoRel(p0), py_self(py_self_) {}

    Magick_PathCurvetoRel_Wrapper(PyObject* py_self_, const Magick::PathCurvetoRel& p0):
        Magick::PathCurvetoRel(p0), py_self(py_self_) {}


    PyObject* py_self;
};


}// namespace 


// Module ======================================================================
void Export_pyste_src_PathCurvetoRel()
{
    class_< Magick::PathCurvetoRel, Magick_PathCurvetoRel_Wrapper >("PathCurvetoRel", init< const Magick::PathCurvetoArgs& >())
        .def(init< const Magick::PathCurveToArgsList& >())
        .def(init< const Magick::PathCurvetoRel& >())
    ;

}

