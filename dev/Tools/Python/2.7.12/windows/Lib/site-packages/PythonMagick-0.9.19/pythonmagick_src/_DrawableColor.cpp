
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

struct Magick_DrawableColor_Wrapper: Magick::DrawableColor
{
    Magick_DrawableColor_Wrapper(PyObject* py_self_, double p0, double p1, MagickCore::PaintMethod p2):
        Magick::DrawableColor(p0, p1, p2), py_self(py_self_) {}


    PyObject* py_self;
};


}// namespace 


// Module ======================================================================
void Export_pyste_src_DrawableColor()
{
    class_< Magick::DrawableColor, boost::noncopyable, Magick_DrawableColor_Wrapper >("DrawableColor", init< double, double, MagickCore::PaintMethod >())
        .def("x", (void (Magick::DrawableColor::*)(double) )&Magick::DrawableColor::x)
        .def("x", (double (Magick::DrawableColor::*)() const)&Magick::DrawableColor::x)
        .def("y", (void (Magick::DrawableColor::*)(double) )&Magick::DrawableColor::y)
        .def("y", (double (Magick::DrawableColor::*)() const)&Magick::DrawableColor::y)
        .def("paintMethod", (void (Magick::DrawableColor::*)(MagickCore::PaintMethod) )&Magick::DrawableColor::paintMethod)
        .def("paintMethod", (MagickCore::PaintMethod (Magick::DrawableColor::*)() const)&Magick::DrawableColor::paintMethod)
    ;
implicitly_convertible<Magick::DrawableColor,Magick::Drawable>();
}

