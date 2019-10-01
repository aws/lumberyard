
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

struct Magick_DrawableSkewY_Wrapper: Magick::DrawableSkewY
{
    Magick_DrawableSkewY_Wrapper(PyObject* py_self_, double p0):
        Magick::DrawableSkewY(p0), py_self(py_self_) {}


    PyObject* py_self;
};


}// namespace 


// Module ======================================================================
void Export_pyste_src_DrawableSkewY()
{
    class_< Magick::DrawableSkewY, boost::noncopyable, Magick_DrawableSkewY_Wrapper >("DrawableSkewY", init< double >())
        .def("angle", (void (Magick::DrawableSkewY::*)(double) )&Magick::DrawableSkewY::angle)
        .def("angle", (double (Magick::DrawableSkewY::*)() const)&Magick::DrawableSkewY::angle)
    ;
implicitly_convertible<Magick::DrawableSkewY,Magick::Drawable>();
}

