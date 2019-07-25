
// Boost Includes ==============================================================
#include <boost/python.hpp>
#include <boost/cstdint.hpp>

// Includes ====================================================================
#include <Magick++/Image.h>

// Using =======================================================================
using namespace boost::python;

// Declarations ================================================================
namespace  {

BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(Magick_Image_adaptiveThreshold_overloads_2_3, adaptiveThreshold, 2, 3)

BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(Magick_Image_blur_overloads_0_2, blur, 0, 2)

BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(Magick_Image_border_overloads_0_1, border, 0, 1)

BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(Magick_Image_charcoal_overloads_0_2, charcoal, 0, 2)

BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(Magick_Image_composite_overloads_3_4, composite, 3, 4)

BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(Magick_Image_composite_overloads_2_3, composite, 2, 3)

BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(Magick_Image_edge_overloads_0_1, edge, 0, 1)

BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(Magick_Image_emboss_overloads_0_2, emboss, 0, 2)

BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(Magick_Image_frame_overloads_0_1, frame, 0, 1)

BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(Magick_Image_frame_overloads_2_4, frame, 2, 4)

BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(Magick_Image_map_overloads_1_2, map, 1, 2)

BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(Magick_Image_medianFilter_overloads_0_1, medianFilter, 0, 1)

BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(Magick_Image_negate_overloads_0_1, negate, 0, 1)

BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(Magick_Image_oilPaint_overloads_0_1, oilPaint, 0, 1)

BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(Magick_Image_quantize_overloads_0_1, quantize, 0, 1)

BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(Magick_Image_raise_overloads_0_2, raise, 0, 2)

BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(Magick_Image_segment_overloads_0_2, segment, 0, 2)

BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(Magick_Image_shade_overloads_0_3, shade, 0, 3)

BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(Magick_Image_sharpen_overloads_0_2, sharpen, 0, 2)

BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(Magick_Image_solarize_overloads_0_1, solarize, 0, 1)

BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(Magick_Image_spread_overloads_0_1, spread, 0, 1)

BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(Magick_Image_wave_overloads_0_2, wave, 0, 2)

BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(Magick_Image_signature_overloads_0_1, signature, 0, 1)


}// namespace 


// Module ======================================================================
void Export_pyste_src_Image()
{
    class_< Magick::Image >("Image", init<  >())
        .def(init< const std::string& >())
        .def(init< const Magick::Geometry&, const Magick::Color& >())
        .def(init< const Magick::Blob& >())
        .def(init< const Magick::Blob&, const Magick::Geometry& >())
        .def(init< const Magick::Blob&, const Magick::Geometry&, const size_t >())
        .def(init< const Magick::Blob&, const Magick::Geometry&, const size_t, const std::string& >())
        .def(init< const Magick::Blob&, const Magick::Geometry&, const std::string& >())
        .def(init< const size_t, const size_t, const std::string&, const MagickCore::StorageType, const char* >())
        .def(init< const Magick::Image& >())
        .def(init< MagickCore::Image* >())
        .def("adaptiveThreshold", &Magick::Image::adaptiveThreshold, Magick_Image_adaptiveThreshold_overloads_2_3())
        .def("addNoise", &Magick::Image::addNoise)
        .def("affineTransform", &Magick::Image::affineTransform)
        .def("annotate", (void (Magick::Image::*)(const std::string&, const Magick::Geometry&) )&Magick::Image::annotate)
        .def("annotate", (void (Magick::Image::*)(const std::string&, const Magick::Geometry&, const MagickCore::GravityType) )&Magick::Image::annotate)
        .def("annotate", (void (Magick::Image::*)(const std::string&, const Magick::Geometry&, const MagickCore::GravityType, const double) )&Magick::Image::annotate)
        .def("annotate", (void (Magick::Image::*)(const std::string&, const MagickCore::GravityType) )&Magick::Image::annotate)
        .def("blur", &Magick::Image::blur, Magick_Image_blur_overloads_0_2())
        .def("border", &Magick::Image::border, Magick_Image_border_overloads_0_1())
        .def("channel", &Magick::Image::channel)
        .def("channelDepth", (void (Magick::Image::*)(const MagickCore::ChannelType, const size_t) )&Magick::Image::channelDepth)
        .def("channelDepth", (size_t (Magick::Image::*)(const MagickCore::ChannelType) )&Magick::Image::channelDepth)
        .def("charcoal", &Magick::Image::charcoal, Magick_Image_charcoal_overloads_0_2())
        .def("chop", &Magick::Image::chop)
        .def("colorize", (void (Magick::Image::*)(const unsigned int, const unsigned int, const unsigned int, const Magick::Color&) )&Magick::Image::colorize)
        .def("colorize", (void (Magick::Image::*)(const unsigned int, const Magick::Color&) )&Magick::Image::colorize)
        .def("comment", (void (Magick::Image::*)(const std::string&) )&Magick::Image::comment)
#if MagickLibVersion < 0x700
        .def("compare", (bool (Magick::Image::*)(const Magick::Image&) )&Magick::Image::compare)
#else
        .def("compare", (bool (Magick::Image::*)(const Magick::Image&) const)&Magick::Image::compare)
#endif
        .def("composite", (void (Magick::Image::*)(const Magick::Image&, const ::ssize_t, const ::ssize_t, const MagickCore::CompositeOperator) )&Magick::Image::composite, Magick_Image_composite_overloads_3_4())
        .def("composite", (void (Magick::Image::*)(const Magick::Image&, const Magick::Geometry&, const MagickCore::CompositeOperator) )&Magick::Image::composite, Magick_Image_composite_overloads_2_3())
        .def("composite", (void (Magick::Image::*)(const Magick::Image&, const MagickCore::GravityType, const MagickCore::CompositeOperator) )&Magick::Image::composite, Magick_Image_composite_overloads_2_3())
        .def("contrast", &Magick::Image::contrast)
        .def("convolve", &Magick::Image::convolve)
        .def("crop", &Magick::Image::crop)
        .def("cycleColormap", &Magick::Image::cycleColormap)
        .def("despeckle", &Magick::Image::despeckle)
        .def("display", &Magick::Image::display)
        .def("draw", (void (Magick::Image::*)(const Magick::Drawable&) )&Magick::Image::draw)
        .def("draw", (void (Magick::Image::*)(const Magick::Drawable&) )&Magick::Image::draw)
#if MagickLibVersion < 0x700
        .def("draw", (void (Magick::Image::*)(const std::list<Magick::Drawable,std::allocator<Magick::Drawable> >&) )&Magick::Image::draw)
#else
        .def("draw", (void (Magick::Image::*)(const std::vector<Magick::Drawable>&) )&Magick::Image::draw)
#endif
        .def("edge", &Magick::Image::edge, Magick_Image_edge_overloads_0_1())
        .def("emboss", &Magick::Image::emboss, Magick_Image_emboss_overloads_0_2())
        .def("enhance", &Magick::Image::enhance)
        .def("equalize", &Magick::Image::equalize)
        .def("erase", &Magick::Image::erase)
        .def("flip", &Magick::Image::flip)
#if MagickLibVersion < 0x700
        .def("floodFillColor", (void (Magick::Image::*)(const ::ssize_t, const ::ssize_t, const Magick::Color&) )&Magick::Image::floodFillColor)
#else
        .def("floodFillColor", (void (Magick::Image::*)(const ::ssize_t, const ::ssize_t, const Magick::Color&, const bool) )&Magick::Image::floodFillColor)
#endif
#if MagickLibVersion < 0x700
        .def("floodFillColor", (void (Magick::Image::*)(const Magick::Geometry&, const Magick::Color&) )&Magick::Image::floodFillColor)
#else
        .def("floodFillColor", (void (Magick::Image::*)(const Magick::Geometry&, const Magick::Color&, const bool) )&Magick::Image::floodFillColor)
#endif
#if MagickLibVersion < 0x700
        .def("floodFillColor", (void (Magick::Image::*)(const ::ssize_t, const ::ssize_t, const Magick::Color&, const Magick::Color&) )&Magick::Image::floodFillColor)
#else
        .def("floodFillColor", (void (Magick::Image::*)(const ::ssize_t, const ::ssize_t, const Magick::Color&, const Magick::Color&,const bool) )&Magick::Image::floodFillColor)
#endif
#if MagickLibVersion < 0x700
        .def("floodFillColor", (void (Magick::Image::*)(const Magick::Geometry&, const Magick::Color&, const Magick::Color&) )&Magick::Image::floodFillColor)
#else
        .def("floodFillColor", (void (Magick::Image::*)(const Magick::Geometry&, const Magick::Color&, const Magick::Color&, const bool) )&Magick::Image::floodFillColor)
#endif
#if MagickLibVersion < 0x700
        .def("floodFillOpacity", (void (Magick::Image::*)(const ::ssize_t, const ::ssize_t, const unsigned int, const bool) ) &Magick::Image::floodFillOpacity)
#else
        .def("floodFillAlpha", (void (Magick::Image::*)(const ::ssize_t, const ::ssize_t, const unsigned int, const bool) ) &Magick::Image::floodFillAlpha)
#endif
#if MagickLibVersion < 0x700
        .def("floodFillOpacity", (void (Magick::Image::*)(const ::ssize_t, const ::ssize_t, const unsigned int, const Magick::PaintMethod) ) &Magick::Image::floodFillOpacity)
#else
        .def("floodFillAlpha", (void (Magick::Image::*)(const ::ssize_t, const ::ssize_t, const unsigned int, const Magick::Color&, const bool) ) &Magick::Image::floodFillAlpha)
#endif
#if MagickLibVersion < 0x700
        .def("floodFillOpacity", (void (Magick::Image::*)(const ::ssize_t, const ::ssize_t, const unsigned int, const Magick::Color&, const bool) ) &Magick::Image::floodFillOpacity)
#endif
#if MagickLibVersion < 0x700
        .def("floodFillTexture", (void (Magick::Image::*)(const ::ssize_t, const ::ssize_t, const Magick::Image&) )&Magick::Image::floodFillTexture)
#else
        .def("floodFillTexture", (void (Magick::Image::*)(const ::ssize_t, const ::ssize_t, const Magick::Image&,const bool) )&Magick::Image::floodFillTexture)
#endif
#if MagickLibVersion < 0x700
        .def("floodFillTexture", (void (Magick::Image::*)(const Magick::Geometry&, const Magick::Image&) )&Magick::Image::floodFillTexture)
#else
        .def("floodFillTexture", (void (Magick::Image::*)(const Magick::Geometry&, const Magick::Image&,const bool) )&Magick::Image::floodFillTexture)
#endif
#if MagickLibVersion < 0x700
        .def("floodFillTexture", (void (Magick::Image::*)(const ::ssize_t, const ::ssize_t, const Magick::Image&, const Magick::Color&) )&Magick::Image::floodFillTexture)
#else
        .def("floodFillTexture", (void (Magick::Image::*)(const ::ssize_t, const ::ssize_t, const Magick::Image&, const Magick::Color&, const bool) )&Magick::Image::floodFillTexture)
#endif
#if MagickLibVersion < 0x700
        .def("floodFillTexture", (void (Magick::Image::*)(const Magick::Geometry&, const Magick::Image&, const Magick::Color&) )&Magick::Image::floodFillTexture)
#else
        .def("floodFillTexture", (void (Magick::Image::*)(const Magick::Geometry&, const Magick::Image&, const Magick::Color&,const bool) )&Magick::Image::floodFillTexture)
#endif
        .def("flop", &Magick::Image::flop)
        .def("frame", (void (Magick::Image::*)(const Magick::Geometry&) )&Magick::Image::frame, Magick_Image_frame_overloads_0_1())
        .def("frame", (void (Magick::Image::*)(const size_t, const size_t, const ::ssize_t, const ::ssize_t) )&Magick::Image::frame, Magick_Image_frame_overloads_2_4())
        .def("gamma", (void (Magick::Image::*)(const double) )&Magick::Image::gamma)
        .def("gamma", (void (Magick::Image::*)(const double, const double, const double) )&Magick::Image::gamma)
        .def("gaussianBlur", &Magick::Image::gaussianBlur)
        .def("implode", &Magick::Image::implode)
        .def("label", (void (Magick::Image::*)(const std::string&) )&Magick::Image::label)
        .def("magnify", &Magick::Image::magnify)
        .def("map", &Magick::Image::map, Magick_Image_map_overloads_1_2())
#if MagickLibVersion < 0x700
        .def("matteFloodfill", &Magick::Image::matteFloodfill)
#endif
        .def("medianFilter", &Magick::Image::medianFilter, Magick_Image_medianFilter_overloads_0_1())
        .def("minify", &Magick::Image::minify)
        .def("modulate", &Magick::Image::modulate)
        .def("negate", &Magick::Image::negate, Magick_Image_negate_overloads_0_1())
        .def("normalize", &Magick::Image::normalize)
        .def("oilPaint", &Magick::Image::oilPaint, Magick_Image_oilPaint_overloads_0_1())
#if MagickLibVersion < 0x700
        .def("opacity", &Magick::Image::opacity)
#endif
        .def("opaque", &Magick::Image::opaque)
        .def("ping", (void (Magick::Image::*)(const std::string&) )&Magick::Image::ping)
        .def("ping", (void (Magick::Image::*)(const Magick::Blob&) )&Magick::Image::ping)
        .def("quantize", &Magick::Image::quantize, Magick_Image_quantize_overloads_0_1())
        .def("process", &Magick::Image::process)
        .def("raise", &Magick::Image::raise, Magick_Image_raise_overloads_0_2())
        .def("read", (void (Magick::Image::*)(const std::string&) )&Magick::Image::read)
        .def("read", (void (Magick::Image::*)(const Magick::Geometry&, const std::string&) )&Magick::Image::read)
        .def("read", (void (Magick::Image::*)(const Magick::Blob&) )&Magick::Image::read)
        .def("read", (void (Magick::Image::*)(const Magick::Blob&, const Magick::Geometry&) )&Magick::Image::read)
        .def("read", (void (Magick::Image::*)(const Magick::Blob&, const Magick::Geometry&, const size_t) )&Magick::Image::read)
        .def("read", (void (Magick::Image::*)(const Magick::Blob&, const Magick::Geometry&, const size_t, const std::string&) )&Magick::Image::read)
        .def("read", (void (Magick::Image::*)(const Magick::Blob&, const Magick::Geometry&, const std::string&) )&Magick::Image::read)
        .def("reduceNoise", (void (Magick::Image::*)() )&Magick::Image::reduceNoise)
#if MagickLibVersion < 0x700
        .def("reduceNoise", (void (Magick::Image::*)(const double) )&Magick::Image::reduceNoise)
#else
        .def("reduceNoise", (void (Magick::Image::*)(const size_t) )&Magick::Image::reduceNoise)
#endif
        .def("resize", &Magick::Image::resize)
        .def("roll", (void (Magick::Image::*)(const Magick::Geometry&) )&Magick::Image::roll)
        .def("roll", (void (Magick::Image::*)(const size_t, const size_t) )&Magick::Image::roll)
        .def("rotate", &Magick::Image::rotate)
        .def("sample", &Magick::Image::sample)
        .def("scale", &Magick::Image::scale)
        .def("segment", &Magick::Image::segment, Magick_Image_segment_overloads_0_2())
        .def("shade", &Magick::Image::shade, Magick_Image_shade_overloads_0_3())
        .def("sharpen", &Magick::Image::sharpen, Magick_Image_sharpen_overloads_0_2())
        .def("shave", &Magick::Image::shave)
        .def("shear", &Magick::Image::shear)
        .def("solarize", &Magick::Image::solarize, Magick_Image_solarize_overloads_0_1())
        .def("spread", &Magick::Image::spread, Magick_Image_spread_overloads_0_1())
        .def("stegano", &Magick::Image::stegano)
        .def("stereo", &Magick::Image::stereo)
        .def("strip", &Magick::Image::strip)
        .def("swirl", &Magick::Image::swirl)
        .def("texture", &Magick::Image::texture)
        .def("threshold", &Magick::Image::threshold)
#if MagickLibVersion < 0x700
        .def("transform", (void (Magick::Image::*)(const Magick::Geometry&) )&Magick::Image::transform)
        .def("transform", (void (Magick::Image::*)(const Magick::Geometry&, const Magick::Geometry&) )&Magick::Image::transform)
#endif
        .def("transparent", &Magick::Image::transparent)
        .def("trim", &Magick::Image::trim)
        .def("type", (void (Magick::Image::*)(const MagickCore::ImageType) )&Magick::Image::type)
        .def("unsharpmask", &Magick::Image::unsharpmask)
        .def("wave", &Magick::Image::wave, Magick_Image_wave_overloads_0_2())
        .def("write", (void (Magick::Image::*)(const std::string&) )&Magick::Image::write)
        .def("write", (void (Magick::Image::*)(Magick::Blob*) )&Magick::Image::write)
        .def("write", (void (Magick::Image::*)(Magick::Blob*, const std::string&) )&Magick::Image::write)
        .def("write", (void (Magick::Image::*)(Magick::Blob*, const std::string&, const size_t) )&Magick::Image::write)
        .def("zoom", &Magick::Image::zoom)
        .def("adjoin", (void (Magick::Image::*)(const bool) )&Magick::Image::adjoin)
        .def("adjoin", (bool (Magick::Image::*)() const)&Magick::Image::adjoin)
#if MagickLibVersion < 0x700
        .def("antiAlias", (void (Magick::Image::*)(const bool) )&Magick::Image::antiAlias)
        .def("antiAlias", (bool (Magick::Image::*)() const)&Magick::Image::antiAlias)
#endif
        .def("animationDelay", (void (Magick::Image::*)(const size_t) )&Magick::Image::animationDelay)
        .def("animationDelay", (size_t (Magick::Image::*)() const)&Magick::Image::animationDelay)
        .def("animationIterations", (void (Magick::Image::*)(const size_t) )&Magick::Image::animationIterations)
        .def("animationIterations", (size_t (Magick::Image::*)() const)&Magick::Image::animationIterations)
        .def("attribute", (void (Magick::Image::*)(const std::string, const std::string) )&Magick::Image::attribute)
        .def("attribute", (std::string (Magick::Image::*)(const std::string) const )&Magick::Image::attribute)
        .def("backgroundColor", (void (Magick::Image::*)(const Magick::Color&) )&Magick::Image::backgroundColor)
        .def("backgroundColor", (Magick::Color (Magick::Image::*)() const)&Magick::Image::backgroundColor)
        .def("backgroundTexture", (void (Magick::Image::*)(const std::string&) )&Magick::Image::backgroundTexture)
        .def("backgroundTexture", (std::string (Magick::Image::*)() const)&Magick::Image::backgroundTexture)
        .def("baseColumns", &Magick::Image::baseColumns)
        .def("baseFilename", &Magick::Image::baseFilename)
        .def("baseRows", &Magick::Image::baseRows)
        .def("borderColor", (void (Magick::Image::*)(const Magick::Color&) )&Magick::Image::borderColor)
        .def("borderColor", (Magick::Color (Magick::Image::*)() const)&Magick::Image::borderColor)
        .def("boundingBox", &Magick::Image::boundingBox)
        .def("boxColor", (void (Magick::Image::*)(const Magick::Color&) )&Magick::Image::boxColor)
        .def("boxColor", (Magick::Color (Magick::Image::*)() const)&Magick::Image::boxColor)
#if MagickLibVersion < 0x700
        .def("cacheThreshold", &Magick::Image::cacheThreshold)
#endif
#if MagickLibVersion < 0x700
        .def("chromaBluePrimary", (void (Magick::Image::*)(const double, const double) )&Magick::Image::chromaBluePrimary)
        .def("chromaBluePrimary", (void (Magick::Image::*)(double*, double*) const)&Magick::Image::chromaBluePrimary)
#else
        .def("chromaBluePrimary", (void (Magick::Image::*)(const double, const double,const double) )&Magick::Image::chromaBluePrimary)
        .def("chromaBluePrimary", (void (Magick::Image::*)(double*, double*,double*) const)&Magick::Image::chromaBluePrimary)
#endif
#if MagickLibVersion < 0x700
        .def("chromaGreenPrimary", (void (Magick::Image::*)(const double, const double) )&Magick::Image::chromaGreenPrimary)
        .def("chromaGreenPrimary", (void (Magick::Image::*)(double*, double*) const)&Magick::Image::chromaGreenPrimary)
#else
        .def("chromaGreenPrimary", (void (Magick::Image::*)(const double, const double,const double) )&Magick::Image::chromaGreenPrimary)
        .def("chromaGreenPrimary", (void (Magick::Image::*)(double*, double*,double *) const)&Magick::Image::chromaGreenPrimary)
#endif
#if MagickLibVersion < 0x700
        .def("chromaRedPrimary", (void (Magick::Image::*)(const double, const double) )&Magick::Image::chromaRedPrimary)
        .def("chromaRedPrimary", (void (Magick::Image::*)(double*, double*) const)&Magick::Image::chromaRedPrimary)
#else
        .def("chromaRedPrimary", (void (Magick::Image::*)(const double, const double, const double) )&Magick::Image::chromaRedPrimary)
        .def("chromaRedPrimary", (void (Magick::Image::*)(double*, double*,double *) const)&Magick::Image::chromaRedPrimary)
#endif
#if MagickLibVersion < 0x700
        .def("chromaWhitePoint", (void (Magick::Image::*)(const double, const double) )&Magick::Image::chromaWhitePoint)
        .def("chromaWhitePoint", (void (Magick::Image::*)(double*, double*) const)&Magick::Image::chromaWhitePoint)
#else
        .def("chromaWhitePoint", (void (Magick::Image::*)(const double, const double, const double) )&Magick::Image::chromaWhitePoint)
        .def("chromaWhitePoint", (void (Magick::Image::*)(double*, double*, double *) const)&Magick::Image::chromaWhitePoint)
#endif
        .def("classType", (void (Magick::Image::*)(const MagickCore::ClassType) )&Magick::Image::classType)
        .def("classType", (MagickCore::ClassType (Magick::Image::*)() const)&Magick::Image::classType)
#if MagickLibVersion < 0x700
        .def("clipMask", (void (Magick::Image::*)(const Magick::Image&) )&Magick::Image::clipMask)
        .def("clipMask", (Magick::Image (Magick::Image::*)() const)&Magick::Image::clipMask)
#endif
        .def("colorFuzz", (void (Magick::Image::*)(const double) )&Magick::Image::colorFuzz)
        .def("colorFuzz", (double (Magick::Image::*)() const)&Magick::Image::colorFuzz)
        .def("colorMap", (void (Magick::Image::*)(const size_t, const Magick::Color&) )&Magick::Image::colorMap)
        .def("colorMap", (Magick::Color (Magick::Image::*)(const size_t) const)&Magick::Image::colorMap)
        .def("colorMapSize", (void (Magick::Image::*)(const size_t) )&Magick::Image::colorMapSize)
        .def("colorMapSize", (size_t (Magick::Image::*)() const)&Magick::Image::colorMapSize)
        .def("colorSpace", (void (Magick::Image::*)(const MagickCore::ColorspaceType) )&Magick::Image::colorSpace)
        .def("colorSpace", (MagickCore::ColorspaceType (Magick::Image::*)() const)&Magick::Image::colorSpace)
        .def("columns", &Magick::Image::columns)
        .def("comment", (std::string (Magick::Image::*)() const)&Magick::Image::comment)
        .def("compose", (void (Magick::Image::*)(const MagickCore::CompositeOperator) )&Magick::Image::compose)
        .def("compose", (MagickCore::CompositeOperator (Magick::Image::*)() const)&Magick::Image::compose)
        .def("compressType", (void (Magick::Image::*)(const MagickCore::CompressionType) )&Magick::Image::compressType)
        .def("compressType", (MagickCore::CompressionType (Magick::Image::*)() const)&Magick::Image::compressType)
        .def("debug", (void (Magick::Image::*)(const bool) )&Magick::Image::debug)
        .def("debug", (bool (Magick::Image::*)() const)&Magick::Image::debug)
        .def("defineValue", (void (Magick::Image::*)(const std::string&, const std::string&, const std::string&) )&Magick::Image::defineValue)
        .def("defineValue", (std::string (Magick::Image::*)(const std::string&, const std::string&) const)&Magick::Image::defineValue)
        .def("defineSet", (void (Magick::Image::*)(const std::string&, const std::string&, bool) )&Magick::Image::defineSet)
        .def("defineSet", (bool (Magick::Image::*)(const std::string&, const std::string&) const)&Magick::Image::defineSet)
#if MagickLibVersion < 0x700
        .def("density", (void (Magick::Image::*)(const Magick::Geometry&) )&Magick::Image::density)
        .def("density", (Magick::Geometry (Magick::Image::*)() const)&Magick::Image::density)
#else
        .def("density", (void (Magick::Image::*)(const Magick::Point&) )&Magick::Image::density)
        .def("density", (Magick::Point (Magick::Image::*)() const)&Magick::Image::density)
#endif
        .def("depth", (void (Magick::Image::*)(const size_t) )&Magick::Image::depth)
        .def("depth", (size_t (Magick::Image::*)() const)&Magick::Image::depth)
        .def("directory", &Magick::Image::directory)
        .def("endian", (void (Magick::Image::*)(const MagickCore::EndianType) )&Magick::Image::endian)
        .def("endian", (MagickCore::EndianType (Magick::Image::*)() const)&Magick::Image::endian)
        .def("fileName", (void (Magick::Image::*)(const std::string&) )&Magick::Image::fileName)
        .def("fileName", (std::string (Magick::Image::*)() const)&Magick::Image::fileName)
        .def("fileSize", &Magick::Image::fileSize)
        .def("fillColor", (void (Magick::Image::*)(const Magick::Color&) )&Magick::Image::fillColor)
        .def("fillColor", (Magick::Color (Magick::Image::*)() const)&Magick::Image::fillColor)
        .def("fillRule", (void (Magick::Image::*)(const MagickCore::FillRule&) )&Magick::Image::fillRule)
        .def("fillRule", (MagickCore::FillRule (Magick::Image::*)() const)&Magick::Image::fillRule)
        .def("fillPattern", (void (Magick::Image::*)(const Magick::Image&) )&Magick::Image::fillPattern)
        .def("fillPattern", (Magick::Image (Magick::Image::*)() const)&Magick::Image::fillPattern)
#if MagickLibVersion < 0x700
        .def("filterType", (void (Magick::Image::*)(const MagickCore::FilterTypes) )&Magick::Image::filterType)
        .def("filterType", (MagickCore::FilterTypes (Magick::Image::*)() const)&Magick::Image::filterType)
#else
        .def("filterType", (void (Magick::Image::*)(const MagickCore::FilterType) )&Magick::Image::filterType)
        .def("filterType", (MagickCore::FilterType (Magick::Image::*)() const)&Magick::Image::filterType)
#endif
        .def("font", (void (Magick::Image::*)(const std::string&) )&Magick::Image::font)
        .def("font", (std::string (Magick::Image::*)() const)&Magick::Image::font)
        .def("fontPointsize", (void (Magick::Image::*)(const double) )&Magick::Image::fontPointsize)
        .def("fontPointsize", (double (Magick::Image::*)() const)&Magick::Image::fontPointsize)
        .def("fontTypeMetrics", &Magick::Image::fontTypeMetrics)
        .def("format", &Magick::Image::format)
        .def("gamma", (double (Magick::Image::*)() const)&Magick::Image::gamma)
        .def("geometry", &Magick::Image::geometry)
#if MagickLibVersion < 0x700
        .def("gifDisposeMethod", (void (Magick::Image::*)(const size_t) )&Magick::Image::gifDisposeMethod)
        .def("gifDisposeMethod", (size_t (Magick::Image::*)() const)&Magick::Image::gifDisposeMethod)
#else
        .def("gifDisposeMethod", (void (Magick::Image::*)(const MagickCore::DisposeType) )&Magick::Image::gifDisposeMethod)
        .def("gifDisposeMethod", (MagickCore::DisposeType (Magick::Image::*)() const)&Magick::Image::gifDisposeMethod)
#endif
        .def("iccColorProfile", (void (Magick::Image::*)(const Magick::Blob&) )&Magick::Image::iccColorProfile)
        .def("iccColorProfile", (Magick::Blob (Magick::Image::*)() const)&Magick::Image::iccColorProfile)
        .def("interlaceType", (void (Magick::Image::*)(const MagickCore::InterlaceType) )&Magick::Image::interlaceType)
        .def("interlaceType", (MagickCore::InterlaceType (Magick::Image::*)() const)&Magick::Image::interlaceType)
        .def("iptcProfile", (void (Magick::Image::*)(const Magick::Blob&) )&Magick::Image::iptcProfile)
        .def("iptcProfile", (Magick::Blob (Magick::Image::*)() const)&Magick::Image::iptcProfile)
        .def("isValid", (void (Magick::Image::*)(const bool) )&Magick::Image::isValid)
        .def("isValid", (bool (Magick::Image::*)() const)&Magick::Image::isValid)
        .def("label", (std::string (Magick::Image::*)() const)&Magick::Image::label)
#if MagickLibVersion < 0x700
        .def("lineWidth", (void (Magick::Image::*)(const double) )&Magick::Image::lineWidth)
        .def("lineWidth", (double (Magick::Image::*)() const)&Magick::Image::lineWidth)
#endif
        .def("magick", (void (Magick::Image::*)(const std::string&) )&Magick::Image::magick)
        .def("magick", (std::string (Magick::Image::*)() const)&Magick::Image::magick)
#if MagickLibVersion < 0x700
        .def("matte", (void (Magick::Image::*)(const bool) )&Magick::Image::matte)
        .def("matte", (bool (Magick::Image::*)() const)&Magick::Image::matte)
#else
        .def("alpha", (void (Magick::Image::*)(const bool) )&Magick::Image::alpha)
        .def("alpha", (bool (Magick::Image::*)() const)&Magick::Image::alpha)
#endif
#if (MagickLibVersion >= 0x700) && (MagickLibVersion < 0x705)
        .def("alphaColor", (void (Magick::Image::*)(const Magick::Color&) )&Magick::Image::alphaColor)
        .def("alphaColor", (Magick::Color (Magick::Image::*)() const)&Magick::Image::alphaColor)
#else
        .def("matteColor", (void (Magick::Image::*)(const Magick::Color&) )&Magick::Image::matteColor)
        .def("matteColor", (Magick::Color (Magick::Image::*)() const)&Magick::Image::matteColor)
#endif
        .def("meanErrorPerPixel", &Magick::Image::meanErrorPerPixel)
        .def("modulusDepth", (void (Magick::Image::*)(const size_t) )&Magick::Image::modulusDepth)
        .def("modulusDepth", (size_t (Magick::Image::*)() const)&Magick::Image::modulusDepth)
        .def("montageGeometry", &Magick::Image::montageGeometry)
        .def("monochrome", (void (Magick::Image::*)(const bool) )&Magick::Image::monochrome)
        .def("monochrome", (bool (Magick::Image::*)() const)&Magick::Image::monochrome)
        .def("normalizedMaxError", &Magick::Image::normalizedMaxError)
        .def("normalizedMeanError", &Magick::Image::normalizedMeanError)
        .def("page", (void (Magick::Image::*)(const Magick::Geometry&) )&Magick::Image::page)
        .def("page", (Magick::Geometry (Magick::Image::*)() const)&Magick::Image::page)
#if MagickLibVersion < 0x700
        .def("penColor", (void (Magick::Image::*)(const Magick::Color&) )&Magick::Image::penColor)
        .def("penColor", (Magick::Color (Magick::Image::*)() const)&Magick::Image::penColor)
        .def("penTexture", (void (Magick::Image::*)(const Magick::Image&) )&Magick::Image::penTexture)
        .def("penTexture", (Magick::Image (Magick::Image::*)() const)&Magick::Image::penTexture)
#endif
        .def("pixelColor", (void (Magick::Image::*)(const ::ssize_t, const ::ssize_t, const Magick::Color&) )&Magick::Image::pixelColor)
        .def("pixelColor", (Magick::Color (Magick::Image::*)(const ::ssize_t, const ::ssize_t) const)&Magick::Image::pixelColor)
        .def("profile", (void (Magick::Image::*)(const std::string, const Magick::Blob&) )&Magick::Image::profile)
        .def("profile", (Magick::Blob (Magick::Image::*)(const std::string) const)&Magick::Image::profile)
        .def("quality", (void (Magick::Image::*)(const size_t) )&Magick::Image::quality)
        .def("quality", (size_t (Magick::Image::*)() const)&Magick::Image::quality)
        .def("quantizeColors", (void (Magick::Image::*)(const size_t) )&Magick::Image::quantizeColors)
        .def("quantizeColors", (size_t (Magick::Image::*)() const)&Magick::Image::quantizeColors)
        .def("quantizeColorSpace", (void (Magick::Image::*)(const MagickCore::ColorspaceType) )&Magick::Image::quantizeColorSpace)
        .def("quantizeColorSpace", (MagickCore::ColorspaceType (Magick::Image::*)() const)&Magick::Image::quantizeColorSpace)
        .def("quantizeDither", (void (Magick::Image::*)(const bool) )&Magick::Image::quantizeDither)
        .def("quantizeDither", (bool (Magick::Image::*)() const)&Magick::Image::quantizeDither)
        .def("quantizeTreeDepth", (void (Magick::Image::*)(const size_t) )&Magick::Image::quantizeTreeDepth)
        .def("quantizeTreeDepth", (size_t (Magick::Image::*)() const)&Magick::Image::quantizeTreeDepth)
        .def("renderingIntent", (void (Magick::Image::*)(const MagickCore::RenderingIntent) )&Magick::Image::renderingIntent)
        .def("renderingIntent", (MagickCore::RenderingIntent (Magick::Image::*)() const)&Magick::Image::renderingIntent)
        .def("resolutionUnits", (void (Magick::Image::*)(const MagickCore::ResolutionType) )&Magick::Image::resolutionUnits)
        .def("resolutionUnits", (MagickCore::ResolutionType (Magick::Image::*)() const)&Magick::Image::resolutionUnits)
        .def("rows", &Magick::Image::rows)
        .def("scene", (void (Magick::Image::*)(const size_t) )&Magick::Image::scene)
        .def("scene", (size_t (Magick::Image::*)() const)&Magick::Image::scene)
        .def("signature", &Magick::Image::signature, Magick_Image_signature_overloads_0_1())
        .def("size", (void (Magick::Image::*)(const Magick::Geometry&) )&Magick::Image::size)
        .def("size", (Magick::Geometry (Magick::Image::*)() const)&Magick::Image::size)
        .def("statistics", &Magick::Image::statistics)
        .def("strokeAntiAlias", (void (Magick::Image::*)(const bool) )&Magick::Image::strokeAntiAlias)
        .def("strokeAntiAlias", (bool (Magick::Image::*)() const)&Magick::Image::strokeAntiAlias)
        .def("strokeColor", (void (Magick::Image::*)(const Magick::Color&) )&Magick::Image::strokeColor)
        .def("strokeColor", (Magick::Color (Magick::Image::*)() const)&Magick::Image::strokeColor)
        .def("strokeDashOffset", (void (Magick::Image::*)(const double) )&Magick::Image::strokeDashOffset)
        .def("strokeDashOffset", (double (Magick::Image::*)() const)&Magick::Image::strokeDashOffset)
        .def("strokeLineCap", (void (Magick::Image::*)(const MagickCore::LineCap) )&Magick::Image::strokeLineCap)
        .def("strokeLineCap", (MagickCore::LineCap (Magick::Image::*)() const)&Magick::Image::strokeLineCap)
        .def("strokeLineJoin", (void (Magick::Image::*)(const MagickCore::LineJoin) )&Magick::Image::strokeLineJoin)
        .def("strokeLineJoin", (MagickCore::LineJoin (Magick::Image::*)() const)&Magick::Image::strokeLineJoin)
        .def("strokeMiterLimit", (void (Magick::Image::*)(const size_t) )&Magick::Image::strokeMiterLimit)
        .def("strokeMiterLimit", (size_t (Magick::Image::*)() const)&Magick::Image::strokeMiterLimit)
        .def("strokePattern", (void (Magick::Image::*)(const Magick::Image&) )&Magick::Image::strokePattern)
        .def("strokePattern", (Magick::Image (Magick::Image::*)() const)&Magick::Image::strokePattern)
        .def("strokeWidth", (void (Magick::Image::*)(const double) )&Magick::Image::strokeWidth)
        .def("strokeWidth", (double (Magick::Image::*)() const)&Magick::Image::strokeWidth)
        .def("subImage", (void (Magick::Image::*)(const size_t) )&Magick::Image::subImage)
        .def("subImage", (size_t (Magick::Image::*)() const)&Magick::Image::subImage)
        .def("subRange", (void (Magick::Image::*)(const size_t) )&Magick::Image::subRange)
        .def("subRange", (size_t (Magick::Image::*)() const)&Magick::Image::subRange)
        .def("textEncoding", (void (Magick::Image::*)(const std::string&) )&Magick::Image::textEncoding)
        .def("textEncoding", (std::string (Magick::Image::*)() const)&Magick::Image::textEncoding)
#if MagickLibVersion < 0x700
        .def("tileName", (void (Magick::Image::*)(const std::string&) )&Magick::Image::tileName)
        .def("tileName", (std::string (Magick::Image::*)() const)&Magick::Image::tileName)
#endif
        .def("totalColors", &Magick::Image::totalColors)
        .def("transformOrigin", &Magick::Image::transformOrigin)
        .def("transformRotation", &Magick::Image::transformRotation)
        .def("transformReset", &Magick::Image::transformReset)
        .def("transformScale", &Magick::Image::transformScale)
        .def("transformSkewX", &Magick::Image::transformSkewX)
        .def("transformSkewY", &Magick::Image::transformSkewY)
        .def("verbose", (void (Magick::Image::*)(const bool) )&Magick::Image::verbose)
        .def("verbose", (bool (Magick::Image::*)() const)&Magick::Image::verbose)
#if MagickLibVersion < 0x700
        .def("view", (void (Magick::Image::*)(const std::string&) )&Magick::Image::view)
        .def("view", (std::string (Magick::Image::*)() const)&Magick::Image::view)
#endif
        .def("x11Display", (void (Magick::Image::*)(const std::string&) )&Magick::Image::x11Display)
        .def("x11Display", (std::string (Magick::Image::*)() const)&Magick::Image::x11Display)
        .def("xResolution", &Magick::Image::xResolution)
        .def("yResolution", &Magick::Image::yResolution)
        .def("syncPixels", &Magick::Image::syncPixels)
        .def("readPixels", &Magick::Image::readPixels)
        .def("writePixels", &Magick::Image::writePixels)
        .def("modifyImage", &Magick::Image::modifyImage)
#if MagickLibVersion < 0x700
        .def("throwImageException", &Magick::Image::throwImageException)
        .staticmethod("cacheThreshold")
#endif
        .def( self == self )
        .def( self != self )
        .def( self > self )
        .def( self < self )
        .def( self >= self )
        .def( self <= self )
    ;

}

