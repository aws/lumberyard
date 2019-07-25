
// Boost Includes ==============================================================
#include <boost/python.hpp>
#include <boost/cstdint.hpp>

// Includes ====================================================================
#include <Magick++/Include.h>

// Using =======================================================================
using namespace boost::python;

// Module ======================================================================
void Export_pyste_src_CompressionType()
{
    enum_< MagickCore::CompressionType >("CompressionType")
        .value("JPEG2000Compression", MagickCore::JPEG2000Compression)
        .value("DXT5Compression", MagickCore::DXT5Compression)
        .value("LZWCompression", MagickCore::LZWCompression)
        .value("DXT3Compression", MagickCore::DXT3Compression)
        .value("RLECompression", MagickCore::RLECompression)
        .value("Group4Compression", MagickCore::Group4Compression)
        .value("NoCompression", MagickCore::NoCompression)
        .value("LosslessJPEGCompression", MagickCore::LosslessJPEGCompression)
        .value("ZipCompression", MagickCore::ZipCompression)
        .value("BZipCompression", MagickCore::BZipCompression)
        .value("DXT1Compression", MagickCore::DXT1Compression)
        .value("JPEGCompression", MagickCore::JPEGCompression)
        .value("UndefinedCompression", MagickCore::UndefinedCompression)
        .value("FaxCompression", MagickCore::FaxCompression)
    ;

}

