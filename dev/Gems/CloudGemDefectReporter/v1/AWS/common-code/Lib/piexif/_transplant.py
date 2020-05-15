import io

from ._common import *


def transplant(exif_src, image, new_file=None):
    """
    py:function:: piexif.transplant(filename1, filename2)

    Transplant exif from filename1 to filename2.

    :param str filename1: JPEG
    :param str filename2: JPEG
    """
    if exif_src[0:2] == b"\xff\xd8":
        src_data = exif_src
    else:
        with open(exif_src, 'rb') as f:
            src_data = f.read()
    segments = split_into_segments(src_data)
    exif = get_exif_seg(segments)
    if exif is None:
        raise ValueError("not found exif in input")

    output_file = False
    if image[0:2] == b"\xff\xd8":
        image_data = image
    else:
        with open(image, 'rb') as f:
            image_data = f.read()
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
        raise ValueError("Give a 3rd argument to 'transplant' to output file")