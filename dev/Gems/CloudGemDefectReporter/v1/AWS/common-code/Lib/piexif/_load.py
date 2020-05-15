import struct
import sys

from ._common import *
from ._exceptions import InvalidImageDataError
from ._exif import *
from piexif import _webp

LITTLE_ENDIAN = b"\x49\x49"


def load(input_data, key_is_name=False):
    """
    py:function:: piexif.load(filename)

    Return exif data as dict. Keys(IFD name), be contained, are "0th", "Exif", "GPS", "Interop", "1st", and "thumbnail". Without "thumbnail", the value is dict(tag name/tag value). "thumbnail" value is JPEG as bytes.

    :param str filename: JPEG or TIFF
    :return: Exif data({"0th":dict, "Exif":dict, "GPS":dict, "Interop":dict, "1st":dict, "thumbnail":bytes})
    :rtype: dict
    """
    exif_dict = {"0th":{},
                 "Exif":{},
                 "GPS":{},
                 "Interop":{},
                 "1st":{},
                 "thumbnail":None}
    exifReader = _ExifReader(input_data)
    if exifReader.tiftag is None:
        return exif_dict

    if exifReader.tiftag[0:2] == LITTLE_ENDIAN:
        exifReader.endian_mark = "<"
    else:
        exifReader.endian_mark = ">"

    pointer = struct.unpack(exifReader.endian_mark + "L",
                            exifReader.tiftag[4:8])[0]
    exif_dict["0th"] = exifReader.get_ifd_dict(pointer, "0th")
    first_ifd_pointer = exif_dict["0th"].pop("first_ifd_pointer")
    if ImageIFD.ExifTag in exif_dict["0th"]:
        pointer = exif_dict["0th"][ImageIFD.ExifTag]
        exif_dict["Exif"] = exifReader.get_ifd_dict(pointer, "Exif")
    if ImageIFD.GPSTag in exif_dict["0th"]:
        pointer = exif_dict["0th"][ImageIFD.GPSTag]
        exif_dict["GPS"] = exifReader.get_ifd_dict(pointer, "GPS")
    if ExifIFD.InteroperabilityTag in exif_dict["Exif"]:
        pointer = exif_dict["Exif"][ExifIFD.InteroperabilityTag]
        exif_dict["Interop"] = exifReader.get_ifd_dict(pointer, "Interop")
    if first_ifd_pointer != b"\x00\x00\x00\x00":
        pointer = struct.unpack(exifReader.endian_mark + "L",
                                first_ifd_pointer)[0]
        exif_dict["1st"] = exifReader.get_ifd_dict(pointer, "1st")
        if (ImageIFD.JPEGInterchangeFormat in exif_dict["1st"] and
            ImageIFD.JPEGInterchangeFormatLength in exif_dict["1st"]):
            end = (exif_dict["1st"][ImageIFD.JPEGInterchangeFormat] +
                   exif_dict["1st"][ImageIFD.JPEGInterchangeFormatLength])
            thumb = exifReader.tiftag[exif_dict["1st"][ImageIFD.JPEGInterchangeFormat]:end]
            exif_dict["thumbnail"] = thumb

    if key_is_name:
        exif_dict = _get_key_name_dict(exif_dict)
    return exif_dict


class _ExifReader(object):
    def __init__(self, data):
        # Prevents "UnicodeWarning: Unicode equal comparison failed" warnings on Python 2
        maybe_image = sys.version_info >= (3,0,0) or isinstance(data, str)

        if maybe_image and data[0:2] == b"\xff\xd8":  # JPEG
            segments = split_into_segments(data)
            app1 = get_exif_seg(segments)
            if app1:
                self.tiftag = app1[10:]
            else:
                self.tiftag = None
        elif maybe_image and data[0:2] in (b"\x49\x49", b"\x4d\x4d"):  # TIFF
            self.tiftag = data
        elif maybe_image and data[0:4] == b"RIFF" and data[8:12] == b"WEBP":
            self.tiftag = _webp.get_exif(data)
        elif maybe_image and data[0:4] == b"Exif":  # Exif
            self.tiftag = data[6:]
        else:
            with open(data, 'rb') as f:
                magic_number = f.read(2)
            if magic_number == b"\xff\xd8":  # JPEG
                app1 = read_exif_from_file(data)
                if app1:
                    self.tiftag = app1[10:]
                else:
                    self.tiftag = None
            elif magic_number in (b"\x49\x49", b"\x4d\x4d"):  # TIFF
                with open(data, 'rb') as f:
                    self.tiftag = f.read()
            else:
                with open(data, 'rb') as f:
                    header = f.read(12)
                if header[0:4] == b"RIFF"and header[8:12] == b"WEBP":
                    with open(data, 'rb') as f:
                        file_data = f.read()
                    self.tiftag = _webp.get_exif(file_data)
                else:
                    raise InvalidImageDataError("Given file is neither JPEG nor TIFF.")

    def get_ifd_dict(self, pointer, ifd_name, read_unknown=False):
        ifd_dict = {}
        tag_count = struct.unpack(self.endian_mark + "H",
                                  self.tiftag[pointer: pointer+2])[0]
        offset = pointer + 2
        if ifd_name in ["0th", "1st"]:
            t = "Image"
        else:
            t = ifd_name
        p_and_value = []
        for x in range(tag_count):
            pointer = offset + 12 * x
            tag = struct.unpack(self.endian_mark + "H",
                       self.tiftag[pointer: pointer+2])[0]
            value_type = struct.unpack(self.endian_mark + "H",
                         self.tiftag[pointer + 2: pointer + 4])[0]
            value_num = struct.unpack(self.endian_mark + "L",
                                      self.tiftag[pointer + 4: pointer + 8]
                                      )[0]
            value = self.tiftag[pointer+8: pointer+12]
            p_and_value.append((pointer, value_type, value_num, value))
            v_set = (value_type, value_num, value, tag)
            if tag in TAGS[t]:
                ifd_dict[tag] = self.convert_value(v_set)
            elif read_unknown:
                ifd_dict[tag] = (v_set[0], v_set[1], v_set[2], self.tiftag)
            #else:
            #    pass

        if ifd_name == "0th":
            pointer = offset + 12 * tag_count
            ifd_dict["first_ifd_pointer"] = self.tiftag[pointer:pointer + 4]
        return ifd_dict

    def convert_value(self, val):
        data = None
        t = val[0]
        length = val[1]
        value = val[2]

        if t == TYPES.Byte: # BYTE
            if length > 4:
                pointer = struct.unpack(self.endian_mark + "L", value)[0]
                data = struct.unpack("B" * length,
                                     self.tiftag[pointer: pointer + length])
            else:
                data = struct.unpack("B" * length, value[0:length])
        elif t == TYPES.Ascii: # ASCII
            if length > 4:
                pointer = struct.unpack(self.endian_mark + "L", value)[0]
                data = self.tiftag[pointer: pointer+length - 1]
            else:
                data = value[0: length - 1]
        elif t == TYPES.Short: # SHORT
            if length > 2:
                pointer = struct.unpack(self.endian_mark + "L", value)[0]
                data = struct.unpack(self.endian_mark + "H" * length,
                                     self.tiftag[pointer: pointer+length*2])
            else:
                data = struct.unpack(self.endian_mark + "H" * length,
                                     value[0:length * 2])
        elif t == TYPES.Long: # LONG
            if length > 1:
                pointer = struct.unpack(self.endian_mark + "L", value)[0]
                data = struct.unpack(self.endian_mark + "L" * length,
                                     self.tiftag[pointer: pointer+length*4])
            else:
                data = struct.unpack(self.endian_mark + "L" * length,
                                     value)
        elif t == TYPES.Rational: # RATIONAL
            pointer = struct.unpack(self.endian_mark + "L", value)[0]
            if length > 1:
                data = tuple(
                    (struct.unpack(self.endian_mark + "L",
                                   self.tiftag[pointer + x * 8:
                                       pointer + 4 + x * 8])[0],
                     struct.unpack(self.endian_mark + "L",
                                   self.tiftag[pointer + 4 + x * 8:
                                       pointer + 8 + x * 8])[0])
                    for x in range(length)
                )
            else:
                data = (struct.unpack(self.endian_mark + "L",
                                      self.tiftag[pointer: pointer + 4])[0],
                        struct.unpack(self.endian_mark + "L",
                                      self.tiftag[pointer + 4: pointer + 8]
                                      )[0])
        elif t == TYPES.SByte: # SIGNED BYTES
            if length > 4:
                pointer = struct.unpack(self.endian_mark + "L", value)[0]
                data = struct.unpack("b" * length,
                                     self.tiftag[pointer: pointer + length])
            else:
                data = struct.unpack("b" * length, value[0:length])
        elif t == TYPES.Undefined: # UNDEFINED BYTES
            if length > 4:
                pointer = struct.unpack(self.endian_mark + "L", value)[0]
                data = self.tiftag[pointer: pointer+length]
            else:
                data = value[0:length]
        elif t == TYPES.SShort: # SIGNED SHORT
            if length > 2:
                pointer = struct.unpack(self.endian_mark + "L", value)[0]
                data = struct.unpack(self.endian_mark + "h" * length,
                                     self.tiftag[pointer: pointer+length*2])
            else:
                data = struct.unpack(self.endian_mark + "h" * length,
                                     value[0:length * 2])
        elif t == TYPES.SLong: # SLONG
            if length > 1:
                pointer = struct.unpack(self.endian_mark + "L", value)[0]
                data = struct.unpack(self.endian_mark + "l" * length,
                                     self.tiftag[pointer: pointer+length*4])
            else:
                data = struct.unpack(self.endian_mark + "l" * length,
                                     value)
        elif t == TYPES.SRational: # SRATIONAL
            pointer = struct.unpack(self.endian_mark + "L", value)[0]
            if length > 1:
                data = tuple(
                  (struct.unpack(self.endian_mark + "l",
                   self.tiftag[pointer + x * 8: pointer + 4 + x * 8])[0],
                   struct.unpack(self.endian_mark + "l",
                   self.tiftag[pointer + 4 + x * 8: pointer + 8 + x * 8])[0])
                  for x in range(length)
                )
            else:
                data = (struct.unpack(self.endian_mark + "l",
                                      self.tiftag[pointer: pointer + 4])[0],
                        struct.unpack(self.endian_mark + "l",
                                      self.tiftag[pointer + 4: pointer + 8]
                                      )[0])
        elif t == TYPES.Float: # FLOAT
            if length > 1:
                pointer = struct.unpack(self.endian_mark + "L", value)[0]
                data = struct.unpack(self.endian_mark + "f" * length,
                                     self.tiftag[pointer: pointer+length*4])
            else:
                data = struct.unpack(self.endian_mark + "f" * length,
                                     value)
        elif t == TYPES.DFloat: # DOUBLE
            pointer = struct.unpack(self.endian_mark + "L", value)[0]
            data = struct.unpack(self.endian_mark + "d" * length,
                                    self.tiftag[pointer: pointer+length*8])
        else:
            raise ValueError("Exif might be wrong. Got incorrect value " +
                             "type to decode.\n" +
                             "tag: " + str(val[3]) + "\ntype: " + str(t))

        if isinstance(data, tuple) and (len(data) == 1):
            return data[0]
        else:
            return data


def _get_key_name_dict(exif_dict):
    new_dict = {
        "0th":{TAGS["Image"][n]["name"]:value for n, value in exif_dict["0th"].items()},
        "Exif":{TAGS["Exif"][n]["name"]:value for n, value in exif_dict["Exif"].items()},
        "1st":{TAGS["Image"][n]["name"]:value for n, value in exif_dict["1st"].items()},
        "GPS":{TAGS["GPS"][n]["name"]:value for n, value in exif_dict["GPS"].items()},
        "Interop":{TAGS["Interop"][n]["name"]:value for n, value in exif_dict["Interop"].items()},
        "thumbnail":exif_dict["thumbnail"],
    }
    return new_dict