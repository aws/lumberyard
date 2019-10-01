
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

struct Magick_PathMovetoRel_Wrapper: Magick::PathMovetoRel
{
    Magick_PathMovetoRel_Wrapper(PyObject* py_self_, const Magick::Coordinate& p0):
        Magick::PathMovetoRel(p0), py_self(py_self_) {}

    Magick_PathMovetoRel_Wrapper(PyObject* py_self_, const Magick::CoordinateList& p0):
        Magick::PathMovetoRel(p0), py_self(py_self_) {}

    Magick_PathMovetoRel_Wrapper(PyObject* py_self_, const Magick::PathMovetoRel& p0):
        Magick::PathMovetoRel(p0), py_self(py_self_) {}


    PyObject* py_self;
};


}// namespace 


// Module ======================================================================
void Export_pyste_src_PathMovetoRel()
{
    class_< Magick::PathMovetoRel, Magick_PathMovetoRel_Wrapper >("PathMovetoRel", init< const Magick::Coordinate& >())
        .def(init< const Magick::CoordinateList& >())
        .def(init< const Magick::PathMovetoRel& >())
    ;

}

