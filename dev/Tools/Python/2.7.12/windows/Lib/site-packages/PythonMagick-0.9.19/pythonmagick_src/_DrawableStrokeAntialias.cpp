
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

struct Magick_DrawableStrokeAntialias_Wrapper: Magick::DrawableStrokeAntialias
{
    Magick_DrawableStrokeAntialias_Wrapper(PyObject* py_self_, bool p0):
        Magick::DrawableStrokeAntialias(p0), py_self(py_self_) {}


    PyObject* py_self;
};


}// namespace 


// Module ======================================================================
void Export_pyste_src_DrawableStrokeAntialias()
{
    class_< Magick::DrawableStrokeAntialias, boost::noncopyable, Magick_DrawableStrokeAntialias_Wrapper >("DrawableStrokeAntialias", init< bool >())
        .def("flag", (void (Magick::DrawableStrokeAntialias::*)(bool) )&Magick::DrawableStrokeAntialias::flag)
        .def("flag", (bool (Magick::DrawableStrokeAntialias::*)() const)&Magick::DrawableStrokeAntialias::flag)
    ;
implicitly_convertible<Magick::DrawableStrokeAntialias,Magick::Drawable>();
}

