
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

struct Magick_PathLinetoHorizontalRel_Wrapper: Magick::PathLinetoHorizontalRel
{
    Magick_PathLinetoHorizontalRel_Wrapper(PyObject* py_self_, double p0):
        Magick::PathLinetoHorizontalRel(p0), py_self(py_self_) {}


    PyObject* py_self;
};


}// namespace 


// Module ======================================================================
void Export_pyste_src_PathLinetoHorizontalRel()
{
    class_< Magick::PathLinetoHorizontalRel, boost::noncopyable, Magick_PathLinetoHorizontalRel_Wrapper >("PathLinetoHorizontalRel", init< double >())
        .def("x", (void (Magick::PathLinetoHorizontalRel::*)(double) )&Magick::PathLinetoHorizontalRel::x)
        .def("x", (double (Magick::PathLinetoHorizontalRel::*)() const)&Magick::PathLinetoHorizontalRel::x)
    ;

}

