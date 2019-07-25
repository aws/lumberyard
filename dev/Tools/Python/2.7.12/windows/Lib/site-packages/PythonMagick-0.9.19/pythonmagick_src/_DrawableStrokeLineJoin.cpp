
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

struct Magick_DrawableStrokeLineJoin_Wrapper: Magick::DrawableStrokeLineJoin
{
    Magick_DrawableStrokeLineJoin_Wrapper(PyObject* py_self_, MagickCore::LineJoin p0):
        Magick::DrawableStrokeLineJoin(p0), py_self(py_self_) {}


    PyObject* py_self;
};


}// namespace 


// Module ======================================================================
void Export_pyste_src_DrawableStrokeLineJoin()
{
    class_< Magick::DrawableStrokeLineJoin, boost::noncopyable, Magick_DrawableStrokeLineJoin_Wrapper >("DrawableStrokeLineJoin", init< MagickCore::LineJoin >())
        .def("linejoin", (void (Magick::DrawableStrokeLineJoin::*)(MagickCore::LineJoin) )&Magick::DrawableStrokeLineJoin::linejoin)
        .def("linejoin", (MagickCore::LineJoin (Magick::DrawableStrokeLineJoin::*)() const)&Magick::DrawableStrokeLineJoin::linejoin)
    ;
implicitly_convertible<Magick::DrawableStrokeLineJoin,Magick::Drawable>();
}

