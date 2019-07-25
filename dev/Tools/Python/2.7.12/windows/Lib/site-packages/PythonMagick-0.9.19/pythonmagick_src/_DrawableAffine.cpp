
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

struct Magick_DrawableAffine_Wrapper: Magick::DrawableAffine
{
    Magick_DrawableAffine_Wrapper(PyObject* py_self_, double p0, double p1, double p2, double p3, double p4, double p5):
        Magick::DrawableAffine(p0, p1, p2, p3, p4, p5), py_self(py_self_) {}

    Magick_DrawableAffine_Wrapper(PyObject* py_self_):
        Magick::DrawableAffine(), py_self(py_self_) {}


    PyObject* py_self;
};


}// namespace 


// Module ======================================================================
void Export_pyste_src_DrawableAffine()
{
    class_< Magick::DrawableAffine, boost::noncopyable, Magick_DrawableAffine_Wrapper >("DrawableAffine", init<  >())
        .def(init< double, double, double, double, double, double >())
        .def("sx", (void (Magick::DrawableAffine::*)(const double) )&Magick::DrawableAffine::sx)
        .def("sx", (double (Magick::DrawableAffine::*)() const)&Magick::DrawableAffine::sx)
        .def("sy", (void (Magick::DrawableAffine::*)(const double) )&Magick::DrawableAffine::sy)
        .def("sy", (double (Magick::DrawableAffine::*)() const)&Magick::DrawableAffine::sy)
        .def("rx", (void (Magick::DrawableAffine::*)(const double) )&Magick::DrawableAffine::rx)
        .def("rx", (double (Magick::DrawableAffine::*)() const)&Magick::DrawableAffine::rx)
        .def("ry", (void (Magick::DrawableAffine::*)(const double) )&Magick::DrawableAffine::ry)
        .def("ry", (double (Magick::DrawableAffine::*)() const)&Magick::DrawableAffine::ry)
        .def("tx", (void (Magick::DrawableAffine::*)(const double) )&Magick::DrawableAffine::tx)
        .def("tx", (double (Magick::DrawableAffine::*)() const)&Magick::DrawableAffine::tx)
        .def("ty", (void (Magick::DrawableAffine::*)(const double) )&Magick::DrawableAffine::ty)
        .def("ty", (double (Magick::DrawableAffine::*)() const)&Magick::DrawableAffine::ty)
    ;
implicitly_convertible<Magick::DrawableAffine,Magick::Drawable>();
}

