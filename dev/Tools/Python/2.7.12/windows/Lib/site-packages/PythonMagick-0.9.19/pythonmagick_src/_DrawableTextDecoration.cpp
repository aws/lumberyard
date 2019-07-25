
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

struct Magick_DrawableTextDecoration_Wrapper: Magick::DrawableTextDecoration
{
    Magick_DrawableTextDecoration_Wrapper(PyObject* py_self_, MagickCore::DecorationType p0):
        Magick::DrawableTextDecoration(p0), py_self(py_self_) {}

    Magick_DrawableTextDecoration_Wrapper(PyObject* py_self_, const Magick::DrawableTextDecoration& p0):
        Magick::DrawableTextDecoration(p0), py_self(py_self_) {}


    PyObject* py_self;
};


}// namespace 


// Module ======================================================================
void Export_pyste_src_DrawableTextDecoration()
{
    class_< Magick::DrawableTextDecoration, Magick_DrawableTextDecoration_Wrapper >("DrawableTextDecoration", init< MagickCore::DecorationType >())
        .def(init< const Magick::DrawableTextDecoration& >())
        .def("decoration", (void (Magick::DrawableTextDecoration::*)(MagickCore::DecorationType) )&Magick::DrawableTextDecoration::decoration)
        .def("decoration", (MagickCore::DecorationType (Magick::DrawableTextDecoration::*)() const)&Magick::DrawableTextDecoration::decoration)
    ;
implicitly_convertible<Magick::DrawableTextDecoration,Magick::Drawable>();
}

