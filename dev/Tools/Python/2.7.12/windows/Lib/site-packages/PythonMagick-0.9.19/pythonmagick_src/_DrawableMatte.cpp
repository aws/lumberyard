
// Boost Includes ==============================================================
#include <boost/python.hpp>
#include <boost/cstdint.hpp>

// Includes ====================================================================
#include <Magick++/Drawable.h>

// Declarations ================================================================
#include <Magick++.h>

// Using =======================================================================
using namespace boost::python;

#if MagickLibVersion < 0x700
namespace  {


struct Magick_DrawableMatte_Wrapper: Magick::DrawableMatte
{
    Magick_DrawableMatte_Wrapper(PyObject* py_self_, double p0, double p1, MagickCore::PaintMethod p2):
        Magick::DrawableMatte(p0, p1, p2), py_self(py_self_) {}


    PyObject* py_self;
};


}// namespace 


// Module ======================================================================
void Export_pyste_src_DrawableMatte()
{
    class_< Magick::DrawableMatte, boost::noncopyable, Magick_DrawableMatte_Wrapper >("DrawableMatte", init< double, double, MagickCore::PaintMethod >())
        .def("x", (void (Magick::DrawableMatte::*)(double) )&Magick::DrawableMatte::x)
        .def("x", (double (Magick::DrawableMatte::*)() const)&Magick::DrawableMatte::x)
        .def("y", (void (Magick::DrawableMatte::*)(double) )&Magick::DrawableMatte::y)
        .def("y", (double (Magick::DrawableMatte::*)() const)&Magick::DrawableMatte::y)
        .def("paintMethod", (void (Magick::DrawableMatte::*)(MagickCore::PaintMethod) )&Magick::DrawableMatte::paintMethod)
        .def("paintMethod", (MagickCore::PaintMethod (Magick::DrawableMatte::*)() const)&Magick::DrawableMatte::paintMethod)
    ;
implicitly_convertible<Magick::DrawableMatte,Magick::Drawable>();
}
#else
namespace  {


struct Magick_DrawableAlpha_Wrapper: Magick::DrawableAlpha
{
    Magick_DrawableAlpha_Wrapper(PyObject* py_self_, double p0, double p1, MagickCore::PaintMethod p2):
        Magick::DrawableAlpha(p0, p1, p2), py_self(py_self_) {}


    PyObject* py_self;
};


}// namespace 


// Module ======================================================================
void Export_pyste_src_DrawableAlpha()
{
    class_< Magick::DrawableAlpha, boost::noncopyable, Magick_DrawableAlpha_Wrapper >("DrawableAlpha", init< double, double, MagickCore::PaintMethod >())
        .def("x", (void (Magick::DrawableAlpha::*)(double) )&Magick::DrawableAlpha::x)
        .def("x", (double (Magick::DrawableAlpha::*)() const)&Magick::DrawableAlpha::x)
        .def("y", (void (Magick::DrawableAlpha::*)(double) )&Magick::DrawableAlpha::y)
        .def("y", (double (Magick::DrawableAlpha::*)() const)&Magick::DrawableAlpha::y)
        .def("paintMethod", (void (Magick::DrawableAlpha::*)(MagickCore::PaintMethod) )&Magick::DrawableAlpha::paintMethod)
        .def("paintMethod", (MagickCore::PaintMethod (Magick::DrawableAlpha::*)() const)&Magick::DrawableAlpha::paintMethod)
    ;
implicitly_convertible<Magick::DrawableAlpha,Magick::Drawable>();
}
#endif
