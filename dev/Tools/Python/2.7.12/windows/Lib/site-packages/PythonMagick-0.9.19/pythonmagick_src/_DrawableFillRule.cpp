
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

struct Magick_DrawableFillRule_Wrapper: Magick::DrawableFillRule
{
    Magick_DrawableFillRule_Wrapper(PyObject* py_self_, const MagickCore::FillRule p0):
        Magick::DrawableFillRule(p0), py_self(py_self_) {}


    PyObject* py_self;
};


}// namespace 


// Module ======================================================================
void Export_pyste_src_DrawableFillRule()
{
    class_< Magick::DrawableFillRule, boost::noncopyable, Magick_DrawableFillRule_Wrapper >("DrawableFillRule", init< const MagickCore::FillRule >())
        .def("fillRule", (void (Magick::DrawableFillRule::*)(const MagickCore::FillRule) )&Magick::DrawableFillRule::fillRule)
        .def("fillRule", (MagickCore::FillRule (Magick::DrawableFillRule::*)() const)&Magick::DrawableFillRule::fillRule)
    ;
implicitly_convertible<Magick::DrawableFillRule,Magick::Drawable>();
}

