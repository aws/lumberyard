from PIL import Image
import io

# def _coder(im):
#     if im:
#         data = {
#             'pixels': im.tobytes(),
#             'size': im.size,
#             'mode': im.mode,
#         }
#         return data
#  
#  
# def _decoder(data):
#     if data:
#         im = Image.frombytes(data['mode'], data['size'], data['pixels'])
#         return im

def _coder(im):
    if im:
        b=io.BytesIO()
        im.save(b, format='png')
        data = b.getvalue()
        return data
 
 
def _decoder(data):
    if data:
        b=io.BytesIO(data)
        im = Image.open(b)
        return im

codec = (_coder, _decoder)