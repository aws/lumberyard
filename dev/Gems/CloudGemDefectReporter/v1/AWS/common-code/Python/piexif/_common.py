import struct

from ._exceptions import InvalidImageDataError


def split_into_segments(data):
    """Slices JPEG meta data into a list from JPEG binary data.
    """
    if data[0:2] != b"\xff\xd8":
        raise InvalidImageDataError("Given data isn't JPEG.")

    head = 2
    segments = [b"\xff\xd8"]
    while 1:
        if data[head: head + 2] == b"\xff\xda":
            segments.append(data[head:])
            break
        else:
            length = struct.unpack(">H", data[head + 2: head + 4])[0]
            endPoint = head + length + 2
            seg = data[head: endPoint]
            segments.append(seg)
            head = endPoint

        if (head >= len(data)):
            raise InvalidImageDataError("Wrong JPEG data.")
    return segments

def read_exif_from_file(filename):
    """Slices JPEG meta data into a list from JPEG binary data.
    """
    f = open(filename, "rb")
    data = f.read(6)

    if data[0:2] != b"\xff\xd8":
        raise InvalidImageDataError("Given data isn't JPEG.")

    head = data[2:6]
    HEAD_LENGTH = 4
    exif = None
    while 1:
        length = struct.unpack(">H", head[2: 4])[0]

        if head[:2] == b"\xff\xe1":
            segment_data = f.read(length - 2)
            exif = head + segment_data
            break
        elif head[0:1] == b"\xff":
            f.read(length - 2)
            head = f.read(HEAD_LENGTH)
        else:
            break

    f.close()
    return exif

def get_exif_seg(segments):
    """Returns Exif from JPEG meta data list
    """
    for seg in segments:
        if seg[0:2] == b"\xff\xe1" and seg[4:10] == b"Exif\x00\x00":
            return seg
    return None


def merge_segments(segments, exif=b""):
    """Merges Exif with APP0 and APP1 manipulations.
    """
    if segments[1][0:2] == b"\xff\xe0" and \
       segments[2][0:2] == b"\xff\xe1" and \
       segments[2][4:10] == b"Exif\x00\x00":
        if exif:
            segments[2] = exif
            segments.pop(1)
        elif exif is None:
            segments.pop(2)
        else:
            segments.pop(1)
    elif segments[1][0:2] == b"\xff\xe0":
        if exif:
            segments[1] = exif
    elif segments[1][0:2] == b"\xff\xe1" and \
         segments[1][4:10] == b"Exif\x00\x00":
        if exif:
            segments[1] = exif
        elif exif is None:
            segments.pop(1)
    else:
        if exif:
            segments.insert(1, exif)
    return b"".join(segments)
