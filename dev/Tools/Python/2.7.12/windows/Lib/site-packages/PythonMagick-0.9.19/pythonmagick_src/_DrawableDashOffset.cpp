
// Boost Includes ==============================================================
#include <boost/python.hpp>
#include <boost/cstdint.hpp>

// Includes ====================================================================
#include <Magick++/Drawable.h>

// Declarations ================================================================
#include <Magick++.h>

// Using =======================================================================
using namespace boost::python;

#if MagickLibVersion < 0x700
namespace  {

struct Magick_DrawableDashOffset_Wrapper: Magick::DrawableDashOffset
{
    Magick_DrawableDashOffset_Wrapper(PyObject* py_self_, const double p0):
        Magick::DrawableDashOffset(p0), py_self(py_self_) {}


    PyObject* py_self;
};


}// namespace 


// Module ======================================================================
void Export_pyste_src_DrawableDashOffset()
{
    class_< Magick::DrawableDashOffset, boost::noncopyable, Magick_DrawableDashOffset_Wrapper >("DrawableDashOffset", init< const double >())
        .def("offset", (void (Magick::DrawableDashOffset::*)(const double) )&Magick::DrawableDashOffset::offset)
        .def("offset", (double (Magick::DrawableDashOffset::*)() const)&Magick::DrawableDashOffset::offset)
    ;
implicitly_convertible<Magick::DrawableDashOffset,Magick::Drawable>();
}
#else
namespace  {

struct Magick_DrawableStrokeDashOffset_Wrapper: Magick::DrawableStrokeDashOffset
{
    Magick_DrawableStrokeDashOffset_Wrapper(PyObject* py_self_, const double p0):
        Magick::DrawableStrokeDashOffset(p0), py_self(py_self_) {}


    PyObject* py_self;
};


}// namespace 


// Module ======================================================================
void Export_pyste_src_DrawableStrokeDashOffset()
{
    class_< Magick::DrawableStrokeDashOffset, boost::noncopyable, Magick_DrawableStrokeDashOffset_Wrapper >("DrawableStrokeDashOffset", init< const double >())
        .def("offset", (void (Magick::DrawableStrokeDashOffset::*)(const double) )&Magick::DrawableStrokeDashOffset::offset)
        .def("offset", (double (Magick::DrawableStrokeDashOffset::*)() const)&Magick::DrawableStrokeDashOffset::offset)
    ;
implicitly_convertible<Magick::DrawableStrokeDashOffset,Magick::Drawable>();
}
#endif
