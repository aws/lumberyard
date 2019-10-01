
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

struct Magick_DrawablePath_Wrapper: Magick::DrawablePath
{
    Magick_DrawablePath_Wrapper(PyObject* py_self_, const Magick::VPathList& p0):
        Magick::DrawablePath(p0), py_self(py_self_) {}

    Magick_DrawablePath_Wrapper(PyObject* py_self_, const Magick::DrawablePath& p0):
        Magick::DrawablePath(p0), py_self(py_self_) {}


    PyObject* py_self;
};


}// namespace 


// Module ======================================================================
void Export_pyste_src_DrawablePath()
{
    class_< Magick::DrawablePath, Magick_DrawablePath_Wrapper >("DrawablePath", init< const Magick::VPathList& >())
        .def(init< const Magick::DrawablePath& >())
    ;
implicitly_convertible<Magick::DrawablePath,Magick::Drawable>();
}

