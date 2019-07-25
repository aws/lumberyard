
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
void Export_pyste_src_Drawable()
{
    class_< Magick::Drawable >("Drawable", init<  >() )
        .def(init< const Magick::DrawableBase& >() )
        .def(init< const Magick::Drawable& >() )
#if MagickLibVersion < 0x700
        .def( self != self )
        .def( self == self )
        .def( self < self )
        .def( self > self )
        .def( self <= self )
        .def( self >= self )
#endif
    ;

}

