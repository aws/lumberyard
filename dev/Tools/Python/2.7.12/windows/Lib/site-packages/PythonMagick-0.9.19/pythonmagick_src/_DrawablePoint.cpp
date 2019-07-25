
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

struct Magick_DrawablePoint_Wrapper: Magick::DrawablePoint
{
    Magick_DrawablePoint_Wrapper(PyObject* py_self_, double p0, double p1):
        Magick::DrawablePoint(p0, p1), py_self(py_self_) {}


    PyObject* py_self;
};


}// namespace 


// Module ======================================================================
void Export_pyste_src_DrawablePoint()
{
    class_< Magick::DrawablePoint, boost::noncopyable, Magick_DrawablePoint_Wrapper >("DrawablePoint", init< double, double >())
        .def("x", (void (Magick::DrawablePoint::*)(double) )&Magick::DrawablePoint::x)
        .def("x", (double (Magick::DrawablePoint::*)() const)&Magick::DrawablePoint::x)
        .def("y", (void (Magick::DrawablePoint::*)(double) )&Magick::DrawablePoint::y)
        .def("y", (double (Magick::DrawablePoint::*)() const)&Magick::DrawablePoint::y)
    ;
implicitly_convertible<Magick::DrawablePoint,Magick::Drawable>();
}

