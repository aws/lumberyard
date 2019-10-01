
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

struct Magick_PathMovetoAbs_Wrapper: Magick::PathMovetoAbs
{
    Magick_PathMovetoAbs_Wrapper(PyObject* py_self_, const Magick::Coordinate& p0):
        Magick::PathMovetoAbs(p0), py_self(py_self_) {}

    Magick_PathMovetoAbs_Wrapper(PyObject* py_self_, const Magick::CoordinateList& p0):
        Magick::PathMovetoAbs(p0), py_self(py_self_) {}

    Magick_PathMovetoAbs_Wrapper(PyObject* py_self_, const Magick::PathMovetoAbs& p0):
        Magick::PathMovetoAbs(p0), py_self(py_self_) {}


    PyObject* py_self;
};


}// namespace 


// Module ======================================================================
void Export_pyste_src_PathMovetoAbs()
{
    class_< Magick::PathMovetoAbs, Magick_PathMovetoAbs_Wrapper >("PathMovetoAbs", init< const Magick::Coordinate& >())
        .def(init< const Magick::CoordinateList& >())
        .def(init< const Magick::PathMovetoAbs& >())
    ;

}

