
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

struct Magick_DrawableGravity_Wrapper: Magick::DrawableGravity
{
    Magick_DrawableGravity_Wrapper(PyObject* py_self_, MagickCore::GravityType p0):
        Magick::DrawableGravity(p0), py_self(py_self_) {}


    PyObject* py_self;
};


}// namespace 


// Module ======================================================================
void Export_pyste_src_DrawableGravity()
{
    class_< Magick::DrawableGravity, boost::noncopyable, Magick_DrawableGravity_Wrapper >("DrawableGravity", init< MagickCore::GravityType >())
        .def("gravity", (void (Magick::DrawableGravity::*)(MagickCore::GravityType) )&Magick::DrawableGravity::gravity)
        .def("gravity", (MagickCore::GravityType (Magick::DrawableGravity::*)() const)&Magick::DrawableGravity::gravity)
    ;
implicitly_convertible<Magick::DrawableGravity,Magick::Drawable>();
}

