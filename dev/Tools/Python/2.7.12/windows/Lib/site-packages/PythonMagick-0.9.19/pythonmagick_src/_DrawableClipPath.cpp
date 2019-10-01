
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

struct Magick_DrawableClipPath_Wrapper: Magick::DrawableClipPath
{
    Magick_DrawableClipPath_Wrapper(PyObject* py_self_, const std::string& p0):
        Magick::DrawableClipPath(p0), py_self(py_self_) {}

    Magick_DrawableClipPath_Wrapper(PyObject* py_self_, const Magick::DrawableClipPath& p0):
        Magick::DrawableClipPath(p0), py_self(py_self_) {}


    PyObject* py_self;
};


}// namespace 


// Module ======================================================================
void Export_pyste_src_DrawableClipPath()
{
    class_< Magick::DrawableClipPath, Magick_DrawableClipPath_Wrapper >("DrawableClipPath", init< const std::string& >())
        .def(init< const Magick::DrawableClipPath& >())
        .def("clip_path", (void (Magick::DrawableClipPath::*)(const std::string&) )&Magick::DrawableClipPath::clip_path)
        .def("clip_path", (std::string (Magick::DrawableClipPath::*)() const)&Magick::DrawableClipPath::clip_path)
    ;
implicitly_convertible<Magick::DrawableClipPath,Magick::Drawable>();
}

