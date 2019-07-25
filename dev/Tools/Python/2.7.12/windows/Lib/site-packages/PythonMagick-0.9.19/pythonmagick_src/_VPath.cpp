
// Boost Includes ==============================================================
#include <boost/python.hpp>
#include <boost/cstdint.hpp>

// Includes ====================================================================
#include <Magick++/Drawable.h>

// Declarations ================================================================
#include <Magick++.h>

// Using =======================================================================
using namespace boost::python;

// Module ======================================================================
void Export_pyste_src_VPath()
{
    class_< Magick::VPath >("VPath", init<  >())
        .def(init< const Magick::VPathBase& >())
        .def(init< const Magick::VPath& >())
    ;

}

