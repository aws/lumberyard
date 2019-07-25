
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

struct Magick_PathLinetoAbs_Wrapper: Magick::PathLinetoAbs
{
    Magick_PathLinetoAbs_Wrapper(PyObject* py_self_, const Magick::Coordinate& p0):
        Magick::PathLinetoAbs(p0), py_self(py_self_) {}

    Magick_PathLinetoAbs_Wrapper(PyObject* py_self_, const Magick::CoordinateList& p0):
        Magick::PathLinetoAbs(p0), py_self(py_self_) {}

    Magick_PathLinetoAbs_Wrapper(PyObject* py_self_, const Magick::PathLinetoAbs& p0):
        Magick::PathLinetoAbs(p0), py_self(py_self_) {}


    PyObject* py_self;
};


}// namespace 


// Module ======================================================================
void Export_pyste_src_PathLinetoAbs()
{
    class_< Magick::PathLinetoAbs, Magick_PathLinetoAbs_Wrapper >("PathLinetoAbs", init< const Magick::Coordinate& >())
        .def(init< const Magick::CoordinateList& >())
        .def(init< const Magick::PathLinetoAbs& >())
    ;

}

