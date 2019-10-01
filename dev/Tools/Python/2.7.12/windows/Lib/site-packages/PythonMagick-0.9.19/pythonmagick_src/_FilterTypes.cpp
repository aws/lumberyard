
// Boost Includes ==============================================================
#include <boost/python.hpp>
#include <boost/cstdint.hpp>

// Includes ====================================================================
#include <Magick++/Include.h>

// Using =======================================================================
using namespace boost::python;

// Module ======================================================================
#if MagickLibVersion < 0x700
void Export_pyste_src_FilterTypes()
{
    enum_< MagickCore::FilterTypes >("FilterTypes")
#else
void Export_pyste_src_FilterType()
{
    enum_< MagickCore::FilterType >("FilterType")
#endif
        .value("BesselFilter", MagickCore::BesselFilter)
        .value("QuadraticFilter", MagickCore::QuadraticFilter)
        .value("BartlettFilter", MagickCore::BartlettFilter)
        .value("CatromFilter", MagickCore::CatromFilter)
        .value("TriangleFilter", MagickCore::TriangleFilter)
        .value("SincFilter", MagickCore::SincFilter)
        .value("BohmanFilter", MagickCore::BohmanFilter)
        .value("BoxFilter", MagickCore::BoxFilter)
        .value("CubicFilter", MagickCore::CubicFilter)
        .value("KaiserFilter", MagickCore::KaiserFilter)
        .value("HammingFilter", MagickCore::HammingFilter)
        .value("ParzenFilter", MagickCore::ParzenFilter)
        .value("SentinelFilter", MagickCore::SentinelFilter)
        .value("LanczosFilter", MagickCore::LanczosFilter)
        .value("WelshFilter", MagickCore::WelshFilter)
        .value("MitchellFilter", MagickCore::MitchellFilter)
        .value("BlackmanFilter", MagickCore::BlackmanFilter)
        .value("GaussianFilter", MagickCore::GaussianFilter)
        .value("HanningFilter", MagickCore::HanningFilter)
        .value("PointFilter", MagickCore::PointFilter)
        .value("HermiteFilter", MagickCore::HermiteFilter)
        .value("LagrangeFilter", MagickCore::LagrangeFilter)
        .value("UndefinedFilter", MagickCore::UndefinedFilter)
    ;

}

