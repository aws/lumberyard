
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

struct Magick_DrawableLine_Wrapper: Magick::DrawableLine
{
    Magick_DrawableLine_Wrapper(PyObject* py_self_, double p0, double p1, double p2, double p3):
        Magick::DrawableLine(p0, p1, p2, p3), py_self(py_self_) {}


    PyObject* py_self;
};


}// namespace 


// Module ======================================================================
void Export_pyste_src_DrawableLine()
{
    class_< Magick::DrawableLine, boost::noncopyable, Magick_DrawableLine_Wrapper >("DrawableLine", init< double, double, double, double >())
        .def("startX", (void (Magick::DrawableLine::*)(double) )&Magick::DrawableLine::startX)
        .def("startX", (double (Magick::DrawableLine::*)() const)&Magick::DrawableLine::startX)
        .def("startY", (void (Magick::DrawableLine::*)(double) )&Magick::DrawableLine::startY)
        .def("startY", (double (Magick::DrawableLine::*)() const)&Magick::DrawableLine::startY)
        .def("endX", (void (Magick::DrawableLine::*)(double) )&Magick::DrawableLine::endX)
        .def("endX", (double (Magick::DrawableLine::*)() const)&Magick::DrawableLine::endX)
        .def("endY", (void (Magick::DrawableLine::*)(double) )&Magick::DrawableLine::endY)
        .def("endY", (double (Magick::DrawableLine::*)() const)&Magick::DrawableLine::endY)
    ;
implicitly_convertible<Magick::DrawableLine,Magick::Drawable>();
}

