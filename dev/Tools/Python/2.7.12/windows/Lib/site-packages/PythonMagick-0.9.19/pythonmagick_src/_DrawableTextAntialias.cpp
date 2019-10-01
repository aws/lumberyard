
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

struct Magick_DrawableTextAntialias_Wrapper: Magick::DrawableTextAntialias
{
    Magick_DrawableTextAntialias_Wrapper(PyObject* py_self_, bool p0):
        Magick::DrawableTextAntialias(p0), py_self(py_self_) {}

    Magick_DrawableTextAntialias_Wrapper(PyObject* py_self_, const Magick::DrawableTextAntialias& p0):
        Magick::DrawableTextAntialias(p0), py_self(py_self_) {}


    PyObject* py_self;
};


}// namespace 


// Module ======================================================================
void Export_pyste_src_DrawableTextAntialias()
{
    class_< Magick::DrawableTextAntialias, Magick_DrawableTextAntialias_Wrapper >("DrawableTextAntialias", init< bool >())
        .def(init< const Magick::DrawableTextAntialias& >())
        .def("flag", (void (Magick::DrawableTextAntialias::*)(bool) )&Magick::DrawableTextAntialias::flag)
        .def("flag", (bool (Magick::DrawableTextAntialias::*)() const)&Magick::DrawableTextAntialias::flag)
    ;
implicitly_convertible<Magick::DrawableTextAntialias,Magick::Drawable>();
}

