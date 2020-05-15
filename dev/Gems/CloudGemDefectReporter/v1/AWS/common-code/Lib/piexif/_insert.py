import io
import struct

from ._common import *
from ._exceptions import InvalidImageDataError


def insert(exif, image, new_file=None):
    """
    py:function:: piexif.insert(exif_bytes, filename)

    Insert exif into JPEG.

    :param bytes exif_bytes: Exif as bytes
    :param str filename: JPEG
    """
    if exif[0:6] != b"\x45\x78\x69\x66\x00\x00":
        raise ValueError("Given data is not exif data")
    exif = b"\xff\xe1" + struct.pack(">H", len(exif) + 2) + exif

    output_file = False
    if image[0:2] == b"\xff\xd8":
        image_data = image
    else:
        with open(image, 'rb') as f:
            image_data = f.read()
        if image_data[0:2] != b"\xff\xd8":
            raise InvalidImageDataError
        output_file = True
    segments = split_into_segments(image_data)
    new_data = merge_segments(segments, exif)

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
        raise ValueError("Give a 3rd argment to 'insert' to output file")