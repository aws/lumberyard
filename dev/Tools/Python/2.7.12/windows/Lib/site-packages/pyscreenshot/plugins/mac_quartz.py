# Javier Escalada Gomez
#
# from:
# https://stackoverflow.com/questions/4524723/take-screenshot-in-python-on-mac-os-x

from PIL import Image
import tempfile


class MacQuartzWrapper(object):
    name = 'mac_quartz'
    childprocess = False

    def __init__(self):
        import Quartz
        import LaunchServices
        from Cocoa import NSURL
        import Quartz.CoreGraphics as CG
        self.Quartz = Quartz
        self.LaunchServices = LaunchServices
        self.NSURL = NSURL
        self.CG = CG

    def grab(self, bbox=None):
        f = tempfile.NamedTemporaryFile(
            suffix='.png', prefix='pyscreenshot_screencapture_')
        filename = f.name
        self.grab_to_file(filename, bbox=bbox)
        im = Image.open(filename)
        return im

    def grab_to_file(self, filename, bbox=None, dpi=72):
        # FIXME: Should query dpi from somewhere, e.g for retina displays

        region = self.CG.CGRectMake(*bbox) if bbox else self.CG.CGRectInfinite

        # Create screenshot as CGImage
        image = self.CG.CGWindowListCreateImage(region,
                                                self.CG.kCGWindowListOptionOnScreenOnly, self.CG.kCGNullWindowID,
                                                self.CG.kCGWindowImageDefault)

        # XXX: Can add more types:
        # https://developer.apple.com/library/mac/documentation/MobileCoreServices/Reference/UTTypeRef/Reference/reference.html#//apple_ref/doc/uid/TP40008771
        file_type = self.LaunchServices.kUTTypePNG
        if filename.endswith('.jpeg'):
            file_type = self.LaunchServices.kUTTypeJPEG
        elif filename.endswith('.tiff'):
            file_type = self.LaunchServices.kUTTypeTIFF
        elif filename.endswith('.bmp'):
            file_type = self.LaunchServices.kUTTypeBMP
        elif filename.endswith('.gif'):
            file_type = self.LaunchServices.kUTTypeGIF
        elif filename.endswith('.pdf'):
            file_type = self.LaunchServices.kUTTypePDF

        url = self.NSURL.fileURLWithPath_(filename)

        dest = self.Quartz.CGImageDestinationCreateWithURL(url, file_type,
                                                           # 1 image in file
                                                           1,
                                                           None)

        properties = {
            self.Quartz.kCGImagePropertyDPIWidth: dpi,
            self.Quartz.kCGImagePropertyDPIHeight: dpi,
        }

        # Add the image to the destination, characterizing the image with
        # the properties dictionary.
        self.Quartz.CGImageDestinationAddImage(dest, image, properties)

        # When all the images (only 1 in this example) are added to the destination,
        # finalize the CGImageDestination object.
        self.Quartz.CGImageDestinationFinalize(dest)

    def backend_version(self):
        # TODO:
        return 'not implemented'
