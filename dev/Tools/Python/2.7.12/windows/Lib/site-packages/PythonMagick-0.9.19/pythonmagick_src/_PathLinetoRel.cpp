
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

struct Magick_PathLinetoRel_Wrapper: Magick::PathLinetoRel
{
    Magick_PathLinetoRel_Wrapper(PyObject* py_self_, const Magick::Coordinate& p0):
        Magick::PathLinetoRel(p0), py_self(py_self_) {}

    Magick_PathLinetoRel_Wrapper(PyObject* py_self_, const Magick::CoordinateList& p0):
        Magick::PathLinetoRel(p0), py_self(py_self_) {}

    Magick_PathLinetoRel_Wrapper(PyObject* py_self_, const Magick::PathLinetoRel& p0):
        Magick::PathLinetoRel(p0), py_self(py_self_) {}


    PyObject* py_self;
};


}// namespace 


// Module ======================================================================
void Export_pyste_src_PathLinetoRel()
{
    class_< Magick::PathLinetoRel, Magick_PathLinetoRel_Wrapper >("PathLinetoRel", init< const Magick::Coordinate& >())
        .def(init< const Magick::CoordinateList& >())
        .def(init< const Magick::PathLinetoRel& >())
    ;

}

