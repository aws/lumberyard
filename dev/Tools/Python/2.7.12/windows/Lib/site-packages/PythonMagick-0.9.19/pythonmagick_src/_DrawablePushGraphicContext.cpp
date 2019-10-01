
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

struct Magick_DrawablePushGraphicContext_Wrapper: Magick::DrawablePushGraphicContext
{
    Magick_DrawablePushGraphicContext_Wrapper(PyObject* py_self_):
        Magick::DrawablePushGraphicContext(), py_self(py_self_) {}


    PyObject* py_self;
};


}// namespace 


// Module ======================================================================
void Export_pyste_src_DrawablePushGraphicContext()
{
    class_< Magick::DrawablePushGraphicContext, boost::noncopyable, Magick_DrawablePushGraphicContext_Wrapper >("DrawablePushGraphicContext", init<  >())
    ;
implicitly_convertible<Magick::DrawablePushGraphicContext,Magick::Drawable>();
}

