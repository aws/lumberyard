
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

struct Magick_DrawableStrokeLineCap_Wrapper: Magick::DrawableStrokeLineCap
{
    Magick_DrawableStrokeLineCap_Wrapper(PyObject* py_self_, MagickCore::LineCap p0):
        Magick::DrawableStrokeLineCap(p0), py_self(py_self_) {}


    PyObject* py_self;
};


}// namespace 


// Module ======================================================================
void Export_pyste_src_DrawableStrokeLineCap()
{
    class_< Magick::DrawableStrokeLineCap, boost::noncopyable, Magick_DrawableStrokeLineCap_Wrapper >("DrawableStrokeLineCap", init< MagickCore::LineCap >())
        .def("linecap", (void (Magick::DrawableStrokeLineCap::*)(MagickCore::LineCap) )&Magick::DrawableStrokeLineCap::linecap)
        .def("linecap", (MagickCore::LineCap (Magick::DrawableStrokeLineCap::*)() const)&Magick::DrawableStrokeLineCap::linecap)
    ;
implicitly_convertible<Magick::DrawableStrokeLineCap,Magick::Drawable>();
}

