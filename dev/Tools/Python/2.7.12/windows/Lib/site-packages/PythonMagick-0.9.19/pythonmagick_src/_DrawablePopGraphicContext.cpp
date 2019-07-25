
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

struct Magick_DrawablePopGraphicContext_Wrapper: Magick::DrawablePopGraphicContext
{
    Magick_DrawablePopGraphicContext_Wrapper(PyObject* py_self_):
        Magick::DrawablePopGraphicContext(), py_self(py_self_) {}


    PyObject* py_self;
};


}// namespace 


// Module ======================================================================
void Export_pyste_src_DrawablePopGraphicContext()
{
    class_< Magick::DrawablePopGraphicContext, boost::noncopyable, Magick_DrawablePopGraphicContext_Wrapper >("DrawablePopGraphicContext", init<  >())
    ;
implicitly_convertible<Magick::DrawablePopGraphicContext,Magick::Drawable>();
}

