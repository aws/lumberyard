
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

struct Magick_DrawableRotation_Wrapper: Magick::DrawableRotation
{
    Magick_DrawableRotation_Wrapper(PyObject* py_self_, double p0):
        Magick::DrawableRotation(p0), py_self(py_self_) {}


    PyObject* py_self;
};


}// namespace 


// Module ======================================================================
void Export_pyste_src_DrawableRotation()
{
    class_< Magick::DrawableRotation, boost::noncopyable, Magick_DrawableRotation_Wrapper >("DrawableRotation", init< double >())
        .def("angle", (void (Magick::DrawableRotation::*)(double) )&Magick::DrawableRotation::angle)
        .def("angle", (double (Magick::DrawableRotation::*)() const)&Magick::DrawableRotation::angle)
    ;
implicitly_convertible<Magick::DrawableRotation,Magick::Drawable>();
}

