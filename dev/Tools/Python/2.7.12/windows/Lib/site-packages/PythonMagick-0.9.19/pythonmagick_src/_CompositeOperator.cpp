
// Boost Includes ==============================================================
#include <boost/python.hpp>
#include <boost/cstdint.hpp>

// Includes ====================================================================
#include <Magick++/Include.h>

// Using =======================================================================
using namespace boost::python;

// Module ======================================================================
void Export_pyste_src_CompositeOperator()
{
    enum_< MagickCore::CompositeOperator >("CompositeOperator")
        .value("SrcAtopCompositeOp", MagickCore::SrcAtopCompositeOp)
        .value("SaturateCompositeOp", MagickCore::SaturateCompositeOp)
        .value("CopyCyanCompositeOp", MagickCore::CopyCyanCompositeOp)
        .value("BumpmapCompositeOp", MagickCore::BumpmapCompositeOp)
        .value("ExclusionCompositeOp", MagickCore::ExclusionCompositeOp)
        .value("SrcOutCompositeOp", MagickCore::SrcOutCompositeOp)
        .value("CopyBlackCompositeOp", MagickCore::CopyBlackCompositeOp)
        .value("ScreenCompositeOp", MagickCore::ScreenCompositeOp)
        .value("NoCompositeOp", MagickCore::NoCompositeOp)
        .value("HardLightCompositeOp", MagickCore::HardLightCompositeOp)
        .value("DstInCompositeOp", MagickCore::DstInCompositeOp)
        .value("LuminizeCompositeOp", MagickCore::LuminizeCompositeOp)
        .value("DifferenceCompositeOp", MagickCore::DifferenceCompositeOp)
        .value("BlendCompositeOp", MagickCore::BlendCompositeOp)
        .value("DisplaceCompositeOp", MagickCore::DisplaceCompositeOp)
        .value("DarkenCompositeOp", MagickCore::DarkenCompositeOp)
        .value("OverlayCompositeOp", MagickCore::OverlayCompositeOp)
        .value("CopyYellowCompositeOp", MagickCore::CopyYellowCompositeOp)
#if MagickLibVersion < 0x700
        .value("MinusCompositeOp", MagickCore::MinusCompositeOp)
#else
        .value("MinusDstCompositeOp", MagickCore::MinusDstCompositeOp)
        .value("MinusSrcCompositeOp", MagickCore::MinusSrcCompositeOp)
#endif
        .value("UndefinedCompositeOp", MagickCore::UndefinedCompositeOp)
        .value("HueCompositeOp", MagickCore::HueCompositeOp)
        .value("DstOutCompositeOp", MagickCore::DstOutCompositeOp)
        .value("CopyMagentaCompositeOp", MagickCore::CopyMagentaCompositeOp)
        .value("DstAtopCompositeOp", MagickCore::DstAtopCompositeOp)
        .value("ModulateCompositeOp", MagickCore::ModulateCompositeOp)
        .value("ThresholdCompositeOp", MagickCore::ThresholdCompositeOp)
        .value("OutCompositeOp", MagickCore::OutCompositeOp)
        .value("LinearLightCompositeOp", MagickCore::LinearLightCompositeOp)
        .value("ChangeMaskCompositeOp", MagickCore::ChangeMaskCompositeOp)
        .value("SrcInCompositeOp", MagickCore::SrcInCompositeOp)
        .value("CopyCompositeOp", MagickCore::CopyCompositeOp)
        .value("DstOverCompositeOp", MagickCore::DstOverCompositeOp)
#if MagickLibVersion < 0x700
        .value("CopyOpacityCompositeOp", MagickCore::CopyOpacityCompositeOp)
#else
        .value("CopyAlphaCompositeOp", MagickCore::CopyAlphaCompositeOp)
#endif
        .value("ColorBurnCompositeOp", MagickCore::ColorBurnCompositeOp)
        .value("DstCompositeOp", MagickCore::DstCompositeOp)
        .value("CopyBlueCompositeOp", MagickCore::CopyBlueCompositeOp)
        .value("DissolveCompositeOp", MagickCore::DissolveCompositeOp)
        .value("MultiplyCompositeOp", MagickCore::MultiplyCompositeOp)
#if MagickLibVersion < 0x700
        .value("DivideCompositeOp", MagickCore::DivideCompositeOp)
#else
        .value("DivideDstCompositeOp", MagickCore::DivideDstCompositeOp)
        .value("DivideSrcCompositeOp", MagickCore::DivideSrcCompositeOp)
#endif
        .value("ColorDodgeCompositeOp", MagickCore::ColorDodgeCompositeOp)
        .value("SrcOverCompositeOp", MagickCore::SrcOverCompositeOp)
        .value("AtopCompositeOp", MagickCore::AtopCompositeOp)
        .value("SoftLightCompositeOp", MagickCore::SoftLightCompositeOp)
#if MagickLibVersion < 0x700
        .value("AddCompositeOp", MagickCore::AddCompositeOp)
#else
        .value("ModulusAddCompositeOp", MagickCore::ModulusAddCompositeOp)
#endif
        .value("OverCompositeOp", MagickCore::OverCompositeOp)
        .value("SrcCompositeOp", MagickCore::SrcCompositeOp)
        .value("ClearCompositeOp", MagickCore::ClearCompositeOp)
        .value("InCompositeOp", MagickCore::InCompositeOp)
        .value("PlusCompositeOp", MagickCore::PlusCompositeOp)
        .value("CopyGreenCompositeOp", MagickCore::CopyGreenCompositeOp)
        .value("LightenCompositeOp", MagickCore::LightenCompositeOp)
        .value("ReplaceCompositeOp", MagickCore::ReplaceCompositeOp)
#if MagickLibVersion < 0x700
        .value("SubtractCompositeOp", MagickCore::SubtractCompositeOp)
#else
        .value("ModulusSubtractCompositeOp", MagickCore::ModulusSubtractCompositeOp)
#endif
        .value("ColorizeCompositeOp", MagickCore::ColorizeCompositeOp)
        .value("CopyRedCompositeOp", MagickCore::CopyRedCompositeOp)
        .value("XorCompositeOp", MagickCore::XorCompositeOp)
    ;

}

