
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

struct Magick_DrawableCircle_Wrapper: Magick::DrawableCircle
{
    Magick_DrawableCircle_Wrapper(PyObject* py_self_, double p0, double p1, double p2, double p3):
        Magick::DrawableCircle(p0, p1, p2, p3), py_self(py_self_) {}


    PyObject* py_self;
};


}// namespace 


// Module ======================================================================
void Export_pyste_src_DrawableCircle()
{
    class_< Magick::DrawableCircle, boost::noncopyable, Magick_DrawableCircle_Wrapper >("DrawableCircle", init< double, double, double, double >())
        .def("originX", (void (Magick::DrawableCircle::*)(double) )&Magick::DrawableCircle::originX)
        .def("originX", (double (Magick::DrawableCircle::*)() const)&Magick::DrawableCircle::originX)
        .def("originY", (void (Magick::DrawableCircle::*)(double) )&Magick::DrawableCircle::originY)
        .def("originY", (double (Magick::DrawableCircle::*)() const)&Magick::DrawableCircle::originY)
        .def("perimX", (void (Magick::DrawableCircle::*)(double) )&Magick::DrawableCircle::perimX)
        .def("perimX", (double (Magick::DrawableCircle::*)() const)&Magick::DrawableCircle::perimX)
        .def("perimY", (void (Magick::DrawableCircle::*)(double) )&Magick::DrawableCircle::perimY)
        .def("perimY", (double (Magick::DrawableCircle::*)() const)&Magick::DrawableCircle::perimY)
    ;
implicitly_convertible<Magick::DrawableCircle,Magick::Drawable>();
}

