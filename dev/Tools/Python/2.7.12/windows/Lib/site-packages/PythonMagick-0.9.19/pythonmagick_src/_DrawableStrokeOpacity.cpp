
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

struct Magick_DrawableStrokeOpacity_Wrapper: Magick::DrawableStrokeOpacity
{
    Magick_DrawableStrokeOpacity_Wrapper(PyObject* py_self_, double p0):
        Magick::DrawableStrokeOpacity(p0), py_self(py_self_) {}


    PyObject* py_self;
};


}// namespace 


// Module ======================================================================
void Export_pyste_src_DrawableStrokeOpacity()
{
    class_< Magick::DrawableStrokeOpacity, boost::noncopyable, Magick_DrawableStrokeOpacity_Wrapper >("DrawableStrokeOpacity", init< double >())
        .def("opacity", (void (Magick::DrawableStrokeOpacity::*)(double) )&Magick::DrawableStrokeOpacity::opacity)
        .def("opacity", (double (Magick::DrawableStrokeOpacity::*)() const)&Magick::DrawableStrokeOpacity::opacity)
    ;
implicitly_convertible<Magick::DrawableStrokeOpacity,Magick::Drawable>();
}

