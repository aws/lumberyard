
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

struct Magick_PathLinetoHorizontalAbs_Wrapper: Magick::PathLinetoHorizontalAbs
{
    Magick_PathLinetoHorizontalAbs_Wrapper(PyObject* py_self_, double p0):
        Magick::PathLinetoHorizontalAbs(p0), py_self(py_self_) {}


    PyObject* py_self;
};


}// namespace 


// Module ======================================================================
void Export_pyste_src_PathLinetoHorizontalAbs()
{
    class_< Magick::PathLinetoHorizontalAbs, boost::noncopyable, Magick_PathLinetoHorizontalAbs_Wrapper >("PathLinetoHorizontalAbs", init< double >())
        .def("x", (void (Magick::PathLinetoHorizontalAbs::*)(double) )&Magick::PathLinetoHorizontalAbs::x)
        .def("x", (double (Magick::PathLinetoHorizontalAbs::*)() const)&Magick::PathLinetoHorizontalAbs::x)
    ;

}

