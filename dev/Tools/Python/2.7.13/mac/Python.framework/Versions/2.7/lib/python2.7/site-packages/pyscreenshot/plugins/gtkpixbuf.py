from PIL import Image
import tempfile


class GtkPixbufWrapper(object):
    name = 'pygtk'
    childprocess = False

    def __init__(self):
        import gtk
        self.gtk = gtk

    def grab(self, bbox=None):
        f = tempfile.NamedTemporaryFile(
            suffix='.png', prefix='pyscreenshot_gtkpixbuf_')
        filename = f.name
        self.grab_to_file(filename, bbox)
        im = Image.open(filename)
        return im

    def grab_to_file(self, filename, bbox=None):
        '''
        based on: http://stackoverflow.com/questions/69645/take-a-screenshot-via-a-python-script-linux

        http://www.pygtk.org/docs/pygtk/class-gdkpixbuf.html

        only "jpeg" or "png"
        '''

        w = self.gtk.gdk.get_default_root_window()
#       Capture the whole screen.
        if bbox == None:
            sz = w.get_size()
            pb = self.gtk.gdk.Pixbuf(
                self.gtk.gdk.COLORSPACE_RGB, False, 8, sz[0], sz[1])  # 24bit RGB
            pb = pb.get_from_drawable(
                w, w.get_colormap(), 0, 0, 0, 0, sz[0], sz[1])
#       Only capture what we need. The smaller the capture, the faster.
        else:
            sz = [bbox[2] - bbox[0], bbox[3] - bbox[1]]
            pb = self.gtk.gdk.Pixbuf(
                self.gtk.gdk.COLORSPACE_RGB, False, 8, sz[0], sz[1])
            pb = pb.get_from_drawable(
                w, w.get_colormap(), bbox[0], bbox[1], 0, 0, sz[0], sz[1])
        assert pb
        ftype = 'png'
        if filename.endswith('.jpeg'):
            ftype = 'jpeg'

        pb.save(filename, ftype)

    def backend_version(self):
        return '.'.join(map(str, self.gtk.ver))
