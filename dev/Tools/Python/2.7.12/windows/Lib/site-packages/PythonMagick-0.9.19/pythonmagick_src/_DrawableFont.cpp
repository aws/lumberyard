
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

struct Magick_DrawableFont_Wrapper: Magick::DrawableFont
{
    Magick_DrawableFont_Wrapper(PyObject* py_self_, const std::string& p0):
        Magick::DrawableFont(p0), py_self(py_self_) {}

    Magick_DrawableFont_Wrapper(PyObject* py_self_, const std::string& p0, MagickCore::StyleType p1, const long unsigned int p2, MagickCore::StretchType p3):
        Magick::DrawableFont(p0, p1, p2, p3), py_self(py_self_) {}

    Magick_DrawableFont_Wrapper(PyObject* py_self_, const Magick::DrawableFont& p0):
        Magick::DrawableFont(p0), py_self(py_self_) {}


    PyObject* py_self;
};


}// namespace 


// Module ======================================================================
void Export_pyste_src_DrawableFont()
{
    class_< Magick::DrawableFont, Magick_DrawableFont_Wrapper >("DrawableFont", init< const std::string& >())
        .def(init< const std::string&, MagickCore::StyleType, const long unsigned int, MagickCore::StretchType >())
        .def(init< const Magick::DrawableFont& >())
        .def("font", (void (Magick::DrawableFont::*)(const std::string&) )&Magick::DrawableFont::font)
        .def("font", (std::string (Magick::DrawableFont::*)() const)&Magick::DrawableFont::font)
    ;
implicitly_convertible<Magick::DrawableFont,Magick::Drawable>();
}

