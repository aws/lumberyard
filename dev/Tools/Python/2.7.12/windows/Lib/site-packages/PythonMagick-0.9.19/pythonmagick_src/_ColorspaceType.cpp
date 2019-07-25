
// Boost Includes =============================================================
#include <boost/python.hpp>
#include <boost/cstdint.hpp>

// Includes ===================================================================
#include <Magick++/Include.h>

// Using ======================================================================
using namespace boost::python;

// Module =====================================================================
void Export_pyste_src_ColorspaceType()
{
    enum_< MagickCore::ColorspaceType >("ColorspaceType")
        .value("UndefinedColorspace", MagickCore::UndefinedColorspace)
        .value("RGBColorspace", MagickCore::RGBColorspace)
        .value("GRAYColorspace", MagickCore::GRAYColorspace)
        .value("TransparentColorspace", MagickCore::TransparentColorspace)
        .value("OHTAColorspace", MagickCore::OHTAColorspace)
        .value("LabColorspace", MagickCore::LabColorspace)
        .value("XYZColorspace", MagickCore::XYZColorspace)
        .value("YCbCrColorspace", MagickCore::YCbCrColorspace)
        .value("YCCColorspace", MagickCore::YCCColorspace)
        .value("YIQColorspace", MagickCore::YIQColorspace)
        .value("YPbPrColorspace", MagickCore::YPbPrColorspace)
        .value("YUVColorspace", MagickCore::YUVColorspace)
        .value("CMYKColorspace", MagickCore::CMYKColorspace)
        .value("sRGBColorspace", MagickCore::sRGBColorspace)
        .value("HSBColorspace", MagickCore::HSBColorspace)
        .value("HSLColorspace", MagickCore::HSLColorspace)
        .value("HWBColorspace", MagickCore::HWBColorspace)
#if MagickLibVersion < 0x700
        .value("Rec601LumaColorspace", MagickCore::Rec601LumaColorspace)
#endif
        .value("Rec601YCbCrColorspace", MagickCore::Rec601YCbCrColorspace)
#if MagickLibVersion < 0x700
        .value("Rec709LumaColorspace", MagickCore::Rec709LumaColorspace)
#endif
        .value("Rec709YCbCrColorspace", MagickCore::Rec709YCbCrColorspace)
        .value("LogColorspace", MagickCore::LogColorspace)
        .value("CMYColorspace", MagickCore::CMYColorspace)
        .value("LuvColorspace", MagickCore::LuvColorspace)
        .value("HCLColorspace", MagickCore::HCLColorspace)
    ;
}
