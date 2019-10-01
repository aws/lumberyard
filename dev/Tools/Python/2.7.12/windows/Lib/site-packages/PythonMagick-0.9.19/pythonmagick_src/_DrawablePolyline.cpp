
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

struct Magick_DrawablePolyline_Wrapper: Magick::DrawablePolyline
{
    Magick_DrawablePolyline_Wrapper(PyObject* py_self_, const Magick::CoordinateList& p0):
        Magick::DrawablePolyline(p0), py_self(py_self_) {}

    Magick_DrawablePolyline_Wrapper(PyObject* py_self_, const Magick::DrawablePolyline& p0):
        Magick::DrawablePolyline(p0), py_self(py_self_) {}


    PyObject* py_self;
};


}// namespace 


// Module ======================================================================
void Export_pyste_src_DrawablePolyline()
{
    class_< Magick::DrawablePolyline, Magick_DrawablePolyline_Wrapper >("DrawablePolyline", init< const Magick::CoordinateList& >())
        .def(init< const Magick::DrawablePolyline& >())
    ;
implicitly_convertible<Magick::DrawablePolyline,Magick::Drawable>();
}

