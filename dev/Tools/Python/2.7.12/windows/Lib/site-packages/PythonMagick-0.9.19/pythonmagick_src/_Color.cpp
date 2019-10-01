
// Boost Includes ==============================================================
#include <boost/python.hpp>
#include <boost/cstdint.hpp>

// Includes ====================================================================
#include <Magick++/Color.h>

// Using =======================================================================
using namespace boost::python;

// Module ======================================================================
void Export_pyste_src_Color()
{
    class_< Magick::Color >("Color", init<  >())
        .def(init< MagickCore::Quantum, MagickCore::Quantum, MagickCore::Quantum >())
        .def(init< MagickCore::Quantum, MagickCore::Quantum, MagickCore::Quantum, MagickCore::Quantum >())
        .def(init< const std::string& >())
        .def(init< const char* >())
        .def(init< const Magick::Color& >())
#if MagickLibVersion < 0x700
        .def(init< const MagickCore::PixelPacket& >())
        .def("redQuantum", (void (Magick::Color::*)(MagickCore::Quantum) )&Magick::Color::redQuantum)
        .def("redQuantum", (MagickCore::Quantum (Magick::Color::*)() const)&Magick::Color::redQuantum)
        .def("greenQuantum", (void (Magick::Color::*)(MagickCore::Quantum) )&Magick::Color::greenQuantum)
        .def("greenQuantum", (MagickCore::Quantum (Magick::Color::*)() const)&Magick::Color::greenQuantum)
        .def("blueQuantum", (void (Magick::Color::*)(MagickCore::Quantum) )&Magick::Color::blueQuantum)
        .def("blueQuantum", (MagickCore::Quantum (Magick::Color::*)() const)&Magick::Color::blueQuantum)
        .def("alphaQuantum", (void (Magick::Color::*)(MagickCore::Quantum) )&Magick::Color::alphaQuantum)
        .def("alphaQuantum", (MagickCore::Quantum (Magick::Color::*)() const)&Magick::Color::alphaQuantum)
        .def("alpha", (void (Magick::Color::*)(double) )&Magick::Color::alpha)
        .def("alpha", (double (Magick::Color::*)() const)&Magick::Color::alpha)
        .def("intensity", &Magick::Color::intensity)
        .def("scaleDoubleToQuantum", &Magick::Color::scaleDoubleToQuantum)
        .def("scaleQuantumToDouble", (double (*)(const MagickCore::Quantum))&Magick::Color::scaleQuantumToDouble)
        .def("scaleQuantumToDouble", (double (*)(const double))&Magick::Color::scaleQuantumToDouble)
        .staticmethod("scaleDoubleToQuantum")
        .staticmethod("scaleQuantumToDouble")
        .def("to_MagickCore_PixelPacket", &Magick::Color::operator MagickCore::PixelPacket)
#else
        .def(init< const MagickCore::PixelInfo& >())
        .def("quantumRed", (void (Magick::Color::*)(MagickCore::Quantum) )&Magick::Color::quantumRed)
        .def("quantumRed", (MagickCore::Quantum (Magick::Color::*)() const)&Magick::Color::quantumRed)
        .def("quantumGreen", (void (Magick::Color::*)(MagickCore::Quantum) )&Magick::Color::quantumGreen)
        .def("quantumGreen", (MagickCore::Quantum (Magick::Color::*)() const)&Magick::Color::quantumGreen)
        .def("quantumBlue", (void (Magick::Color::*)(MagickCore::Quantum) )&Magick::Color::quantumBlue)
        .def("quantumBlue", (MagickCore::Quantum (Magick::Color::*)() const)&Magick::Color::quantumBlue)
        .def("quantumAlpha", (void (Magick::Color::*)(MagickCore::Quantum) )&Magick::Color::quantumAlpha)
        .def("quantumAlpha", (MagickCore::Quantum (Magick::Color::*)() const)&Magick::Color::quantumAlpha)
        .def("to_MagickCore_PixelInfo", &Magick::Color::operator MagickCore::PixelInfo)
#endif
        .def("isValid", (void (Magick::Color::*)(bool) )&Magick::Color::isValid)
        .def("isValid", (bool (Magick::Color::*)() const)&Magick::Color::isValid)
        .def( self > self )
        .def( self < self )
        .def( self == self )
        .def( self != self )
        .def( self <= self )
        .def( self >= self )
        .def("to_std_string", &Magick::Color::operator std::string)
    ;

implicitly_convertible<std::string,Magick::Color>();}

