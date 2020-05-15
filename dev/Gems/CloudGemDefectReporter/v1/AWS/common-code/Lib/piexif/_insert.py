import io
import struct
import sys

from ._common import *
from ._exceptions import InvalidImageDataError
from piexif import _webp

def insert(exif, image, new_file=None):
    """
    py:function:: piexif.insert(exif_bytes, filename)

    Insert exif into JPEG.

    :param bytes exif_bytes: Exif as bytes
    :param str filename: JPEG
    """
    if exif[0:6] != b"\x45\x78\x69\x66\x00\x00":
        raise ValueError("Given data is not exif data")

    output_file = False
    # Prevents "UnicodeWarning: Unicode equal comparison failed" warnings on Python 2
    maybe_image = sys.version_info >= (3,0,0) or isinstance(image, str)

    if maybe_image and image[0:2] == b"\xff\xd8":
        image_data = image
        file_type = "jpeg"
    elif maybe_image and image[0:4] == b"RIFF" and image[8:12] == b"WEBP":
        image_data = image
        file_type = "webp"
    else:
        with open(image, 'rb') as f:
            image_data = f.read()
        if image_data[0:2] == b"\xff\xd8":
            file_type = "jpeg"
        elif image_data[0:4] == b"RIFF" and image_data[8:12] == b"WEBP":
            file_type = "webp"
        else:
            raise InvalidImageDataError
        output_file = True

    if file_type == "jpeg":
        exif = b"\xff\xe1" + struct.pack(">H", len(exif) + 2) + exif
        segments = split_into_segments(image_data)
        new_data = merge_segments(segments, exif)
    elif file_type == "webp":
        exif = exif[6:]
        new_data = _webp.insert(image_data, exif)

    if isinstance(new_file, io.BytesIO):
        new_file.write(new_data)
        new_file.seek(0)
    elif new_file:
        with open(new_file, "wb+") as f:
            f.write(new_data)
    elif output_file:
        with open(image, "wb+") as f:
            f.write(new_data)
    else:
        raise ValueError("Give a 3rd argument to 'insert' to output file")