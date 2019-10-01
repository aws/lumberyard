
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

struct Magick_DrawableMiterLimit_Wrapper: Magick::DrawableMiterLimit
{
    Magick_DrawableMiterLimit_Wrapper(PyObject* py_self_, size_t p0):
        Magick::DrawableMiterLimit(p0), py_self(py_self_) {}


    PyObject* py_self;
};


}// namespace 


// Module ======================================================================
void Export_pyste_src_DrawableMiterLimit()
{
    class_< Magick::DrawableMiterLimit, boost::noncopyable, Magick_DrawableMiterLimit_Wrapper >("DrawableMiterLimit", init< size_t >())
        .def("miterlimit", (void (Magick::DrawableMiterLimit::*)(size_t) )&Magick::DrawableMiterLimit::miterlimit)
        .def("miterlimit", (size_t (Magick::DrawableMiterLimit::*)() const)&Magick::DrawableMiterLimit::miterlimit)
    ;
implicitly_convertible<Magick::DrawableMiterLimit,Magick::Drawable>();
}

