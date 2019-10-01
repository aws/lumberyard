
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

struct Magick_DrawableTranslation_Wrapper: Magick::DrawableTranslation
{
    Magick_DrawableTranslation_Wrapper(PyObject* py_self_, double p0, double p1):
        Magick::DrawableTranslation(p0, p1), py_self(py_self_) {}


    PyObject* py_self;
};


}// namespace 


// Module ======================================================================
void Export_pyste_src_DrawableTranslation()
{
    class_< Magick::DrawableTranslation, boost::noncopyable, Magick_DrawableTranslation_Wrapper >("DrawableTranslation", init< double, double >())
        .def("x", (void (Magick::DrawableTranslation::*)(double) )&Magick::DrawableTranslation::x)
        .def("x", (double (Magick::DrawableTranslation::*)() const)&Magick::DrawableTranslation::x)
        .def("y", (void (Magick::DrawableTranslation::*)(double) )&Magick::DrawableTranslation::y)
        .def("y", (double (Magick::DrawableTranslation::*)() const)&Magick::DrawableTranslation::y)
    ;
implicitly_convertible<Magick::DrawableTranslation,Magick::Drawable>();
}

