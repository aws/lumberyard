
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

struct Magick_DrawableRoundRectangle_Wrapper: Magick::DrawableRoundRectangle
{
    Magick_DrawableRoundRectangle_Wrapper(PyObject* py_self_, double p0, double p1, double p2, double p3, double p4, double p5):
        Magick::DrawableRoundRectangle(p0, p1, p2, p3, p4, p5), py_self(py_self_) {}


    PyObject* py_self;
};


}// namespace 


// Module ======================================================================
void Export_pyste_src_DrawableRoundRectangle()
{
    class_< Magick::DrawableRoundRectangle, boost::noncopyable, Magick_DrawableRoundRectangle_Wrapper >("DrawableRoundRectangle", init< double, double, double, double, double, double >())
        .def("centerX", (void (Magick::DrawableRoundRectangle::*)(double) )&Magick::DrawableRoundRectangle::centerX)
        .def("centerX", (double (Magick::DrawableRoundRectangle::*)() const)&Magick::DrawableRoundRectangle::centerX)
        .def("centerY", (void (Magick::DrawableRoundRectangle::*)(double) )&Magick::DrawableRoundRectangle::centerY)
        .def("centerY", (double (Magick::DrawableRoundRectangle::*)() const)&Magick::DrawableRoundRectangle::centerY)
        .def("width", (void (Magick::DrawableRoundRectangle::*)(double) )&Magick::DrawableRoundRectangle::width)
        .def("width", (double (Magick::DrawableRoundRectangle::*)() const)&Magick::DrawableRoundRectangle::width)
        .def("hight", (void (Magick::DrawableRoundRectangle::*)(double) )&Magick::DrawableRoundRectangle::hight)
        .def("hight", (double (Magick::DrawableRoundRectangle::*)() const)&Magick::DrawableRoundRectangle::hight)
        .def("cornerWidth", (void (Magick::DrawableRoundRectangle::*)(double) )&Magick::DrawableRoundRectangle::cornerWidth)
        .def("cornerWidth", (double (Magick::DrawableRoundRectangle::*)() const)&Magick::DrawableRoundRectangle::cornerWidth)
        .def("cornerHeight", (void (Magick::DrawableRoundRectangle::*)(double) )&Magick::DrawableRoundRectangle::cornerHeight)
        .def("cornerHeight", (double (Magick::DrawableRoundRectangle::*)() const)&Magick::DrawableRoundRectangle::cornerHeight)
    ;
implicitly_convertible<Magick::DrawableRoundRectangle,Magick::Drawable>();
}

