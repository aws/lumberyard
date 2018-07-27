from PIL import Image


class WxScreen(object):

    '''based on: http://stackoverflow.com/questions/69645/take-a-screenshot-via-a-python-script-linux
    '''
    name = 'wx'
    childprocess = False

    def __init__(self):
        import wx
        self.wx = wx
        self.app = None

    def grab(self, bbox=None):
        wx = self.wx
        if not self.app:
            self.app = wx.App()
        screen = wx.ScreenDC()
        size = screen.GetSize()
        bmp = wx.EmptyBitmap(size[0], size[1])
        mem = wx.MemoryDC(bmp)
        mem.Blit(0, 0, size[0], size[1], screen, 0, 0)
        del mem
        myWxImage = wx.ImageFromBitmap(bmp)
        im = Image.new('RGB', (myWxImage.GetWidth(), myWxImage.GetHeight()))
        if hasattr(Image, 'frombytes'):
            # for Pillow
            im.frombytes(myWxImage.GetData())
        else:
            # for PIL
            im.fromstring(myWxImage.GetData())
        if bbox:
            im = im.crop(bbox)
        return im

    def grab_to_file(self, filename):
        # bmp.SaveFile('screenshot.png', wx.BITMAP_TYPE_PNG)
        im = self.grab()
        im.save(filename)

    def backend_version(self):
        return self.wx.__version__
