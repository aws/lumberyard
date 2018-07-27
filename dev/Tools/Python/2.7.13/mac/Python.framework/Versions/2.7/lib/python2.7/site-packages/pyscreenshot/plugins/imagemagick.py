from easyprocess import EasyProcess
from easyprocess import extract_version
from PIL import Image
import tempfile

PROGRAM = 'import'
# http://www.imagemagick.org/


class ImagemagickWrapper(object):
    name = 'imagemagick'
    childprocess = True

    def __init__(self):
        EasyProcess([PROGRAM, '-version']).check_installed()

    def grab(self, bbox=None):
        f = tempfile.NamedTemporaryFile(
            suffix='.png', prefix='pyscreenshot_imagemagick_')
        filename = f.name
        self.grab_to_file(filename, bbox=bbox)
        im = Image.open(filename)
        # if bbox:
        #    im = im.crop(bbox)
        return im

    def grab_to_file(self, filename, bbox=None):
        command = 'import -silent -window root '
        if bbox:
            command += " -crop '%sx%s+%s+%s' " % (
                bbox[2] - bbox[0], bbox[3] - bbox[1], bbox[0], bbox[1])
        command += filename
        EasyProcess(command).call()

    def backend_version(self):
        return extract_version(EasyProcess([PROGRAM, '-version']).call().stdout.replace('-', ' '))
