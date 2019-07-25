
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

struct Magick_DrawablePushPattern_Wrapper: Magick::DrawablePushPattern
{
    Magick_DrawablePushPattern_Wrapper(PyObject* py_self_, const std::string& p0, long int p1, long int p2, long int p3, long int p4):
        Magick::DrawablePushPattern(p0, p1, p2, p3, p4), py_self(py_self_) {}

    Magick_DrawablePushPattern_Wrapper(PyObject* py_self_, const Magick::DrawablePushPattern& p0):
        Magick::DrawablePushPattern(p0), py_self(py_self_) {}


    PyObject* py_self;
};


}// namespace 


// Module ======================================================================
void Export_pyste_src_DrawablePushPattern()
{
    class_< Magick::DrawablePushPattern, Magick_DrawablePushPattern_Wrapper >("DrawablePushPattern", init< const Magick::DrawablePushPattern& >())
        .def(init< const std::string&, long int, long int, long int, long int >())
    ;
implicitly_convertible<Magick::DrawablePushPattern,Magick::Drawable>();
}

