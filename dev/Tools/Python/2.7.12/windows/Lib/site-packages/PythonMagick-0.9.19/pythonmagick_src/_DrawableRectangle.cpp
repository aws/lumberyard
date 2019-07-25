
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

struct Magick_DrawableRectangle_Wrapper: Magick::DrawableRectangle
{
    Magick_DrawableRectangle_Wrapper(PyObject* py_self_, double p0, double p1, double p2, double p3):
        Magick::DrawableRectangle(p0, p1, p2, p3), py_self(py_self_) {}


    PyObject* py_self;
};


}// namespace 


// Module ======================================================================
void Export_pyste_src_DrawableRectangle()
{
    class_< Magick::DrawableRectangle, boost::noncopyable, Magick_DrawableRectangle_Wrapper >("DrawableRectangle", init< double, double, double, double >())
        .def("upperLeftX", (void (Magick::DrawableRectangle::*)(double) )&Magick::DrawableRectangle::upperLeftX)
        .def("upperLeftX", (double (Magick::DrawableRectangle::*)() const)&Magick::DrawableRectangle::upperLeftX)
        .def("upperLeftY", (void (Magick::DrawableRectangle::*)(double) )&Magick::DrawableRectangle::upperLeftY)
        .def("upperLeftY", (double (Magick::DrawableRectangle::*)() const)&Magick::DrawableRectangle::upperLeftY)
        .def("lowerRightX", (void (Magick::DrawableRectangle::*)(double) )&Magick::DrawableRectangle::lowerRightX)
        .def("lowerRightX", (double (Magick::DrawableRectangle::*)() const)&Magick::DrawableRectangle::lowerRightX)
        .def("lowerRightY", (void (Magick::DrawableRectangle::*)(double) )&Magick::DrawableRectangle::lowerRightY)
        .def("lowerRightY", (double (Magick::DrawableRectangle::*)() const)&Magick::DrawableRectangle::lowerRightY)
    ;
implicitly_convertible<Magick::DrawableRectangle,Magick::Drawable>();
}

