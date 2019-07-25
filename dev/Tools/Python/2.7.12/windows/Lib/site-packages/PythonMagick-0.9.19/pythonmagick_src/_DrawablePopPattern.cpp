
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

struct Magick_DrawablePopPattern_Wrapper: Magick::DrawablePopPattern
{
    Magick_DrawablePopPattern_Wrapper(PyObject* py_self_):
        Magick::DrawablePopPattern(), py_self(py_self_) {}


    PyObject* py_self;
};


}// namespace 


// Module ======================================================================
void Export_pyste_src_DrawablePopPattern()
{
    class_< Magick::DrawablePopPattern, boost::noncopyable, Magick_DrawablePopPattern_Wrapper >("DrawablePopPattern", init<  >())
    ;
implicitly_convertible<Magick::DrawablePopPattern,Magick::Drawable>();
}

