
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

struct Magick_DrawableViewbox_Wrapper: Magick::DrawableViewbox
{
    Magick_DrawableViewbox_Wrapper(PyObject* py_self_, ::ssize_t p0, ::ssize_t p1, ::ssize_t p2, ::ssize_t p3):
        Magick::DrawableViewbox(p0, p1, p2, p3), py_self(py_self_) {}


    PyObject* py_self;
};


}// namespace 


// Module ======================================================================
void Export_pyste_src_DrawableViewbox()
{
    class_< Magick::DrawableViewbox, boost::noncopyable, Magick_DrawableViewbox_Wrapper >("DrawableViewbox", init< ::ssize_t, ::ssize_t, ::ssize_t, ::ssize_t >())
        .def("x1", (void (Magick::DrawableViewbox::*)(::ssize_t) )&Magick::DrawableViewbox::x1)
        .def("x1", (::ssize_t (Magick::DrawableViewbox::*)() const)&Magick::DrawableViewbox::x1)
        .def("y1", (void (Magick::DrawableViewbox::*)(::ssize_t) )&Magick::DrawableViewbox::y1)
        .def("y1", (::ssize_t (Magick::DrawableViewbox::*)() const)&Magick::DrawableViewbox::y1)
        .def("x2", (void (Magick::DrawableViewbox::*)(::ssize_t) )&Magick::DrawableViewbox::x2)
        .def("x2", (::ssize_t (Magick::DrawableViewbox::*)() const)&Magick::DrawableViewbox::x2)
        .def("y2", (void (Magick::DrawableViewbox::*)(::ssize_t) )&Magick::DrawableViewbox::y2)
        .def("y2", (::ssize_t (Magick::DrawableViewbox::*)() const)&Magick::DrawableViewbox::y2)
    ;
implicitly_convertible<Magick::DrawableViewbox,Magick::Drawable>();
}

