
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

struct Magick_PathClosePath_Wrapper: Magick::PathClosePath
{
    Magick_PathClosePath_Wrapper(PyObject* py_self_):
        Magick::PathClosePath(), py_self(py_self_) {}


    PyObject* py_self;
};


}// namespace 


// Module ======================================================================
void Export_pyste_src_PathClosePath()
{
    class_< Magick::PathClosePath, boost::noncopyable, Magick_PathClosePath_Wrapper >("PathClosePath", init<  >())
    ;

}

