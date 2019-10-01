
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

struct Magick_DrawableFillOpacity_Wrapper: Magick::DrawableFillOpacity
{
    Magick_DrawableFillOpacity_Wrapper(PyObject* py_self_, double p0):
        Magick::DrawableFillOpacity(p0), py_self(py_self_) {}


    PyObject* py_self;
};


}// namespace 


// Module ======================================================================
void Export_pyste_src_DrawableFillOpacity()
{
    class_< Magick::DrawableFillOpacity, boost::noncopyable, Magick_DrawableFillOpacity_Wrapper >("DrawableFillOpacity", init< double >())
        .def("opacity", (void (Magick::DrawableFillOpacity::*)(double) )&Magick::DrawableFillOpacity::opacity)
        .def("opacity", (double (Magick::DrawableFillOpacity::*)() const)&Magick::DrawableFillOpacity::opacity)
    ;
implicitly_convertible<Magick::DrawableFillOpacity,Magick::Drawable>();
}

