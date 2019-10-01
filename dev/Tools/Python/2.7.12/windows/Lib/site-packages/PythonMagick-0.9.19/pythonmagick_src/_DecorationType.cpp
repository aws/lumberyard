
// Boost Includes ==============================================================
#include <boost/python.hpp>
#include <boost/cstdint.hpp>

// Includes ====================================================================
#include <Magick++/Include.h>

// Using =======================================================================
using namespace boost::python;

// Module ======================================================================
void Export_pyste_src_DecorationType()
{
    enum_< MagickCore::DecorationType >("DecorationType")
        .value("OverlineDecoration", MagickCore::OverlineDecoration)
        .value("UnderlineDecoration", MagickCore::UnderlineDecoration)
        .value("LineThroughDecoration", MagickCore::LineThroughDecoration)
        .value("UndefinedDecoration", MagickCore::UndefinedDecoration)
        .value("NoDecoration", MagickCore::NoDecoration)
    ;

}

