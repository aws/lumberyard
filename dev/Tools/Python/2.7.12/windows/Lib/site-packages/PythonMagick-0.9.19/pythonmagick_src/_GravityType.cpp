
// Boost Includes ==============================================================
#include <boost/python.hpp>
#include <boost/cstdint.hpp>

// Includes ====================================================================
#include <Magick++/Include.h>

// Using =======================================================================
using namespace boost::python;

// Module ======================================================================
void Export_pyste_src_GravityType()
{
    enum_< MagickCore::GravityType >("GravityType")
        .value("SouthEastGravity", MagickCore::SouthEastGravity)
        .value("UndefinedGravity", MagickCore::UndefinedGravity)
        .value("CenterGravity", MagickCore::CenterGravity)
        .value("SouthWestGravity", MagickCore::SouthWestGravity)
#if MagickLibVersion < 0x700
        .value("StaticGravity", MagickCore::StaticGravity)
#endif
        .value("SouthGravity", MagickCore::SouthGravity)
        .value("ForgetGravity", MagickCore::ForgetGravity)
        .value("EastGravity", MagickCore::EastGravity)
        .value("NorthGravity", MagickCore::NorthGravity)
        .value("NorthWestGravity", MagickCore::NorthWestGravity)
        .value("NorthEastGravity", MagickCore::NorthEastGravity)
        .value("WestGravity", MagickCore::WestGravity)
    ;

}

