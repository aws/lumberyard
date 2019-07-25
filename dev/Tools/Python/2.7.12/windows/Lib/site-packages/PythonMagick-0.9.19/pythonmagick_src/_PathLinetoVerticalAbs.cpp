
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

struct Magick_PathLinetoVerticalAbs_Wrapper: Magick::PathLinetoVerticalAbs
{
    Magick_PathLinetoVerticalAbs_Wrapper(PyObject* py_self_, double p0):
        Magick::PathLinetoVerticalAbs(p0), py_self(py_self_) {}


    PyObject* py_self;
};


}// namespace 


// Module ======================================================================
void Export_pyste_src_PathLinetoVerticalAbs()
{
    class_< Magick::PathLinetoVerticalAbs, boost::noncopyable, Magick_PathLinetoVerticalAbs_Wrapper >("PathLinetoVerticalAbs", init< double >())
        .def("y", (void (Magick::PathLinetoVerticalAbs::*)(double) )&Magick::PathLinetoVerticalAbs::y)
        .def("y", (double (Magick::PathLinetoVerticalAbs::*)() const)&Magick::PathLinetoVerticalAbs::y)
    ;

}

