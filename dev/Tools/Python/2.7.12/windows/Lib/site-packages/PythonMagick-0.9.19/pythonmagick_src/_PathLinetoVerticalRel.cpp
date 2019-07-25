
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

struct Magick_PathLinetoVerticalRel_Wrapper: Magick::PathLinetoVerticalRel
{
    Magick_PathLinetoVerticalRel_Wrapper(PyObject* py_self_, double p0):
        Magick::PathLinetoVerticalRel(p0), py_self(py_self_) {}


    PyObject* py_self;
};


}// namespace 


// Module ======================================================================
void Export_pyste_src_PathLinetoVerticalRel()
{
    class_< Magick::PathLinetoVerticalRel, boost::noncopyable, Magick_PathLinetoVerticalRel_Wrapper >("PathLinetoVerticalRel", init< double >())
        .def("y", (void (Magick::PathLinetoVerticalRel::*)(double) )&Magick::PathLinetoVerticalRel::y)
        .def("y", (double (Magick::PathLinetoVerticalRel::*)() const)&Magick::PathLinetoVerticalRel::y)
    ;

}

