
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

struct Magick_DrawablePushClipPath_Wrapper: Magick::DrawablePushClipPath
{
    Magick_DrawablePushClipPath_Wrapper(PyObject* py_self_, const std::string& p0):
        Magick::DrawablePushClipPath(p0), py_self(py_self_) {}

    Magick_DrawablePushClipPath_Wrapper(PyObject* py_self_, const Magick::DrawablePushClipPath& p0):
        Magick::DrawablePushClipPath(p0), py_self(py_self_) {}


    PyObject* py_self;
};


}// namespace 


// Module ======================================================================
void Export_pyste_src_DrawablePushClipPath()
{
    class_< Magick::DrawablePushClipPath, Magick_DrawablePushClipPath_Wrapper >("DrawablePushClipPath", init< const std::string& >())
        .def(init< const Magick::DrawablePushClipPath& >())
    ;
implicitly_convertible<Magick::DrawablePushClipPath,Magick::Drawable>();
}

