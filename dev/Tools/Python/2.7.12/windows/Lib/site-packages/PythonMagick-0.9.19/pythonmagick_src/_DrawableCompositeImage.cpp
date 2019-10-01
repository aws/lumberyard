
// Boost Includes ==============================================================
#include <boost/python.hpp>
#include <boost/cstdint.hpp>

// Includes ====================================================================
#include <Magick++/Drawable.h>

// Declarations ================================================================
#include <Magick++.h>

// Using =======================================================================
using namespace boost::python;

namespace  {

struct Magick_DrawableCompositeImage_Wrapper: Magick::DrawableCompositeImage
{
    Magick_DrawableCompositeImage_Wrapper(PyObject* py_self_, double p0, double p1, const std::string& p2):
        Magick::DrawableCompositeImage(p0, p1, p2), py_self(py_self_) {}

    Magick_DrawableCompositeImage_Wrapper(PyObject* py_self_, double p0, double p1, const Magick::Image& p2):
        Magick::DrawableCompositeImage(p0, p1, p2), py_self(py_self_) {}

    Magick_DrawableCompositeImage_Wrapper(PyObject* py_self_, double p0, double p1, double p2, double p3, const std::string& p4):
        Magick::DrawableCompositeImage(p0, p1, p2, p3, p4), py_self(py_self_) {}

    Magick_DrawableCompositeImage_Wrapper(PyObject* py_self_, double p0, double p1, double p2, double p3, const Magick::Image& p4):
        Magick::DrawableCompositeImage(p0, p1, p2, p3, p4), py_self(py_self_) {}

    Magick_DrawableCompositeImage_Wrapper(PyObject* py_self_, double p0, double p1, double p2, double p3, const std::string& p4, MagickCore::CompositeOperator p5):
        Magick::DrawableCompositeImage(p0, p1, p2, p3, p4, p5), py_self(py_self_) {}

    Magick_DrawableCompositeImage_Wrapper(PyObject* py_self_, double p0, double p1, double p2, double p3, const Magick::Image& p4, MagickCore::CompositeOperator p5):
        Magick::DrawableCompositeImage(p0, p1, p2, p3, p4, p5), py_self(py_self_) {}

    Magick_DrawableCompositeImage_Wrapper(PyObject* py_self_, const Magick::DrawableCompositeImage& p0):
        Magick::DrawableCompositeImage(p0), py_self(py_self_) {}


    PyObject* py_self;
};


}// namespace 


// Module ======================================================================
void Export_pyste_src_DrawableCompositeImage()
{
    class_< Magick::DrawableCompositeImage, Magick_DrawableCompositeImage_Wrapper >("DrawableCompositeImage", init< const Magick::DrawableCompositeImage& >())
        .def(init< double, double, const std::string& >())
        .def(init< double, double, const Magick::Image& >())
        .def(init< double, double, double, double, const std::string& >())
        .def(init< double, double, double, double, const Magick::Image& >())
        .def(init< double, double, double, double, const std::string&, MagickCore::CompositeOperator >())
        .def(init< double, double, double, double, const Magick::Image&, MagickCore::CompositeOperator >())
        .def("composition", (void (Magick::DrawableCompositeImage::*)(MagickCore::CompositeOperator) )&Magick::DrawableCompositeImage::composition)
        .def("composition", (MagickCore::CompositeOperator (Magick::DrawableCompositeImage::*)() const)&Magick::DrawableCompositeImage::composition)
        .def("filename", (void (Magick::DrawableCompositeImage::*)(const std::string&) )&Magick::DrawableCompositeImage::filename)
        .def("filename", (std::string (Magick::DrawableCompositeImage::*)() const)&Magick::DrawableCompositeImage::filename)
        .def("x", (void (Magick::DrawableCompositeImage::*)(double) )&Magick::DrawableCompositeImage::x)
        .def("x", (double (Magick::DrawableCompositeImage::*)() const)&Magick::DrawableCompositeImage::x)
        .def("y", (void (Magick::DrawableCompositeImage::*)(double) )&Magick::DrawableCompositeImage::y)
        .def("y", (double (Magick::DrawableCompositeImage::*)() const)&Magick::DrawableCompositeImage::y)
        .def("width", (void (Magick::DrawableCompositeImage::*)(double) )&Magick::DrawableCompositeImage::width)
        .def("width", (double (Magick::DrawableCompositeImage::*)() const)&Magick::DrawableCompositeImage::width)
        .def("height", (void (Magick::DrawableCompositeImage::*)(double) )&Magick::DrawableCompositeImage::height)
        .def("height", (double (Magick::DrawableCompositeImage::*)() const)&Magick::DrawableCompositeImage::height)
        .def("image", (void (Magick::DrawableCompositeImage::*)(const Magick::Image&) )&Magick::DrawableCompositeImage::image)
        .def("image", (Magick::Image (Magick::DrawableCompositeImage::*)() const)&Magick::DrawableCompositeImage::image)
        .def("magick", (void (Magick::DrawableCompositeImage::*)(std::string) )&Magick::DrawableCompositeImage::magick)
        .def("magick", (std::string (Magick::DrawableCompositeImage::*)() )&Magick::DrawableCompositeImage::magick)
    ;
implicitly_convertible<Magick::DrawableCompositeImage,Magick::Drawable>();
}

