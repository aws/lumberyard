
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

struct Magick_DrawablePolygon_Wrapper: Magick::DrawablePolygon
{
    Magick_DrawablePolygon_Wrapper(PyObject* py_self_, const Magick::CoordinateList& p0):
        Magick::DrawablePolygon(p0), py_self(py_self_) {}

    Magick_DrawablePolygon_Wrapper(PyObject* py_self_, const Magick::DrawablePolygon& p0):
        Magick::DrawablePolygon(p0), py_self(py_self_) {}


    PyObject* py_self;
};


}// namespace 


// Module ======================================================================
void Export_pyste_src_DrawablePolygon()
{
    class_< Magick::DrawablePolygon, Magick_DrawablePolygon_Wrapper >("DrawablePolygon", init< const Magick::CoordinateList& >())
        .def(init< const Magick::DrawablePolygon& >())
    ;
implicitly_convertible<Magick::DrawablePolygon,Magick::Drawable>();
}

