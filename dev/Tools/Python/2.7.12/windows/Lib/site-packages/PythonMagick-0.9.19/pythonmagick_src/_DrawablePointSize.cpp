
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

struct Magick_DrawablePointSize_Wrapper: Magick::DrawablePointSize
{
    Magick_DrawablePointSize_Wrapper(PyObject* py_self_, double p0):
        Magick::DrawablePointSize(p0), py_self(py_self_) {}


    PyObject* py_self;
};


}// namespace 


// Module ======================================================================
void Export_pyste_src_DrawablePointSize()
{
    class_< Magick::DrawablePointSize, boost::noncopyable, Magick_DrawablePointSize_Wrapper >("DrawablePointSize", init< double >())
        .def("pointSize", (void (Magick::DrawablePointSize::*)(double) )&Magick::DrawablePointSize::pointSize)
        .def("pointSize", (double (Magick::DrawablePointSize::*)() const)&Magick::DrawablePointSize::pointSize)
    ;
implicitly_convertible<Magick::DrawablePointSize,Magick::Drawable>();
}

