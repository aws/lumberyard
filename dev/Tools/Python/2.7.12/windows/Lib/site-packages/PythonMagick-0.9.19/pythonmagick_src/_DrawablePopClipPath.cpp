
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

struct Magick_DrawablePopClipPath_Wrapper: Magick::DrawablePopClipPath
{
    Magick_DrawablePopClipPath_Wrapper(PyObject* py_self_):
        Magick::DrawablePopClipPath(), py_self(py_self_) {}


    PyObject* py_self;
};


}// namespace 


// Module ======================================================================
void Export_pyste_src_DrawablePopClipPath()
{
    class_< Magick::DrawablePopClipPath, boost::noncopyable, Magick_DrawablePopClipPath_Wrapper >("DrawablePopClipPath", init<  >())
    ;
implicitly_convertible<Magick::DrawablePopClipPath,Magick::Drawable>();
}

