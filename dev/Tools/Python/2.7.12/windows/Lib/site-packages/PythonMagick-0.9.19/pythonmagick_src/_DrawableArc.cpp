
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

struct Magick_DrawableArc_Wrapper: Magick::DrawableArc
{
    Magick_DrawableArc_Wrapper(PyObject* py_self_, double p0, double p1, double p2, double p3, double p4, double p5):
        Magick::DrawableArc(p0, p1, p2, p3, p4, p5), py_self(py_self_) {}


    PyObject* py_self;
};


}// namespace 


// Module ======================================================================
void Export_pyste_src_DrawableArc()
{
    class_< Magick::DrawableArc, boost::noncopyable, Magick_DrawableArc_Wrapper >("DrawableArc", init< double, double, double, double, double, double >())
        .def("startX", (void (Magick::DrawableArc::*)(double) )&Magick::DrawableArc::startX)
        .def("startX", (double (Magick::DrawableArc::*)() const)&Magick::DrawableArc::startX)
        .def("startY", (void (Magick::DrawableArc::*)(double) )&Magick::DrawableArc::startY)
        .def("startY", (double (Magick::DrawableArc::*)() const)&Magick::DrawableArc::startY)
        .def("endX", (void (Magick::DrawableArc::*)(double) )&Magick::DrawableArc::endX)
        .def("endX", (double (Magick::DrawableArc::*)() const)&Magick::DrawableArc::endX)
        .def("endY", (void (Magick::DrawableArc::*)(double) )&Magick::DrawableArc::endY)
        .def("endY", (double (Magick::DrawableArc::*)() const)&Magick::DrawableArc::endY)
        .def("startDegrees", (void (Magick::DrawableArc::*)(double) )&Magick::DrawableArc::startDegrees)
        .def("startDegrees", (double (Magick::DrawableArc::*)() const)&Magick::DrawableArc::startDegrees)
        .def("endDegrees", (void (Magick::DrawableArc::*)(double) )&Magick::DrawableArc::endDegrees)
        .def("endDegrees", (double (Magick::DrawableArc::*)() const)&Magick::DrawableArc::endDegrees)
    ;
implicitly_convertible<Magick::DrawableArc,Magick::Drawable>();
}

