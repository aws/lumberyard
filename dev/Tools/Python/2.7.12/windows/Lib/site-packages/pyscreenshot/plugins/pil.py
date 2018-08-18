from PIL import Image


class PilWrapper(object):
    """windows only."""
    name = 'pil'
    childprocess = False

    def __init__(self):
        from PIL import ImageGrab  # windows only
        self.ImageGrab = ImageGrab

    def grab(self, bbox=None):
        return self.ImageGrab.grab(bbox)

    def grab_to_file(self, filename):
        im = self.grab()
        im.save(filename)

    def backend_version(self):
        return Image.VERSION
