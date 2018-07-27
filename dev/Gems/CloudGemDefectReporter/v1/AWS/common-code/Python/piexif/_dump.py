import copy
import numbers
import struct

from ._common import *
from ._exif import *


TIFF_HEADER_LENGTH = 8


def dump(exif_dict_original):
    """
    py:function:: piexif.load(data)

    Return exif as bytes.

    :param dict exif: Exif data({"0th":dict, "Exif":dict, "GPS":dict, "Interop":dict, "1st":dict, "thumbnail":bytes})
    :return: Exif
    :rtype: bytes
    """
    exif_dict = copy.deepcopy(exif_dict_original)
    header = b"Exif\x00\x00\x4d\x4d\x00\x2a\x00\x00\x00\x08"
    exif_is = False
    gps_is = False
    interop_is = False
    first_is = False

    if "0th" in exif_dict:
        zeroth_ifd = exif_dict["0th"]
    else:
        zeroth_ifd = {}

    if (("Exif" in exif_dict) and len(exif_dict["Exif"]) or
          ("Interop" in exif_dict) and len(exif_dict["Interop"]) ):
        zeroth_ifd[ImageIFD.ExifTag] = 1
        exif_is = True
        exif_ifd = exif_dict["Exif"]
        if ("Interop" in exif_dict) and len(exif_dict["Interop"]):
            exif_ifd[ExifIFD. InteroperabilityTag] = 1
            interop_is = True
            interop_ifd = exif_dict["Interop"]
        elif ExifIFD. InteroperabilityTag in exif_ifd:
            exif_ifd.pop(ExifIFD.InteroperabilityTag)
    elif ImageIFD.ExifTag in zeroth_ifd:
        zeroth_ifd.pop(ImageIFD.ExifTag)

    if ("GPS" in exif_dict) and len(exif_dict["GPS"]):
        zeroth_ifd[ImageIFD.GPSTag] = 1
        gps_is = True
        gps_ifd = exif_dict["GPS"]
    elif ImageIFD.GPSTag in zeroth_ifd:
        zeroth_ifd.pop(ImageIFD.GPSTag)

    if (("1st" in exif_dict) and
            ("thumbnail" in exif_dict) and
            (exif_dict["thumbnail"] is not None)):
        first_is = True
        exif_dict["1st"][ImageIFD.JPEGInterchangeFormat] = 1
        exif_dict["1st"][ImageIFD.JPEGInterchangeFormatLength] = 1
        first_ifd = exif_dict["1st"]

    zeroth_set = _dict_to_bytes(zeroth_ifd, "0th", 0)
    zeroth_length = (len(zeroth_set[0]) + exif_is * 12 + gps_is * 12 + 4 +
                     len(zeroth_set[1]))

    if exif_is:
        exif_set = _dict_to_bytes(exif_ifd, "Exif", zeroth_length)
        exif_length = len(exif_set[0]) + interop_is * 12 + len(exif_set[1])
    else:
        exif_bytes = b""
        exif_length = 0
    if gps_is:
        gps_set = _dict_to_bytes(gps_ifd, "GPS", zeroth_length + exif_length)
        gps_bytes = b"".join(gps_set)
        gps_length = len(gps_bytes)
    else:
        gps_bytes = b""
        gps_length = 0
    if interop_is:
        offset = zeroth_length + exif_length + gps_length
        interop_set = _dict_to_bytes(interop_ifd, "Interop", offset)
        interop_bytes = b"".join(interop_set)
        interop_length = len(interop_bytes)
    else:
        interop_bytes = b""
        interop_length = 0
    if first_is:
        offset = zeroth_length + exif_length + gps_length + interop_length
        first_set = _dict_to_bytes(first_ifd, "1st", offset)
        thumbnail = _get_thumbnail(exif_dict["thumbnail"])
        thumbnail_max_size = 64000
        if len(thumbnail) > thumbnail_max_size:
            raise ValueError("Given thumbnail is too large. max 64kB")
    else:
        first_bytes = b""
    if exif_is:
        pointer_value = TIFF_HEADER_LENGTH + zeroth_length
        pointer_str = struct.pack(">I", pointer_value)
        key = ImageIFD.ExifTag
        key_str = struct.pack(">H", key)
        type_str = struct.pack(">H", TYPES.Long)
        length_str = struct.pack(">I", 1)
        exif_pointer = key_str + type_str + length_str + pointer_str
    else:
        exif_pointer = b""
    if gps_is:
        pointer_value = TIFF_HEADER_LENGTH + zeroth_length + exif_length
        pointer_str = struct.pack(">I", pointer_value)
        key = ImageIFD.GPSTag
        key_str = struct.pack(">H", key)
        type_str = struct.pack(">H", TYPES.Long)
        length_str = struct.pack(">I", 1)
        gps_pointer = key_str + type_str + length_str + pointer_str
    else:
        gps_pointer = b""
    if interop_is:
        pointer_value = (TIFF_HEADER_LENGTH +
                         zeroth_length + exif_length + gps_length)
        pointer_str = struct.pack(">I", pointer_value)
        key = ExifIFD.InteroperabilityTag
        key_str = struct.pack(">H", key)
        type_str = struct.pack(">H", TYPES.Long)
        length_str = struct.pack(">I", 1)
        interop_pointer = key_str + type_str + length_str + pointer_str
    else:
        interop_pointer = b""
    if first_is:
        pointer_value = (TIFF_HEADER_LENGTH + zeroth_length +
                         exif_length + gps_length + interop_length)
        first_ifd_pointer = struct.pack(">L", pointer_value)
        thumbnail_pointer = (pointer_value + len(first_set[0]) + 24 +
                             4 + len(first_set[1]))
        thumbnail_p_bytes = (b"\x02\x01\x00\x04\x00\x00\x00\x01" +
                             struct.pack(">L", thumbnail_pointer))
        thumbnail_length_bytes = (b"\x02\x02\x00\x04\x00\x00\x00\x01" +
                                  struct.pack(">L", len(thumbnail)))
        first_bytes = (first_set[0] + thumbnail_p_bytes +
                       thumbnail_length_bytes + b"\x00\x00\x00\x00" +
                       first_set[1] + thumbnail)
    else:
        first_ifd_pointer = b"\x00\x00\x00\x00"

    zeroth_bytes = (zeroth_set[0] + exif_pointer + gps_pointer +
                    first_ifd_pointer + zeroth_set[1])
    if exif_is:
        exif_bytes = exif_set[0] + interop_pointer + exif_set[1]

    return (header + zeroth_bytes + exif_bytes + gps_bytes +
            interop_bytes + first_bytes)


def _get_thumbnail(jpeg):
    segments = split_into_segments(jpeg)
    while (b"\xff\xe0" <= segments[1][0:2] <= b"\xff\xef"):
        segments.pop(1)
    thumbnail = b"".join(segments)
    return thumbnail


def _pack_byte(*args):
    return struct.pack("B" * len(args), *args)

def _pack_signed_byte(*args):
    return struct.pack("b" * len(args), *args)

def _pack_short(*args):
    return struct.pack(">" + "H" * len(args), *args)

def _pack_signed_short(*args):
    return struct.pack(">" + "h" * len(args), *args)

def _pack_long(*args):
    return struct.pack(">" + "L" * len(args), *args)

def _pack_slong(*args):
    return struct.pack(">" + "l" * len(args), *args)

def _pack_float(*args):
    return struct.pack(">" + "f" * len(args), *args)

def _pack_double(*args):
    return struct.pack(">" + "d" * len(args), *args)


def _value_to_bytes(raw_value, value_type, offset):
    four_bytes_over = b""
    value_str = b""

    if value_type == TYPES.Byte:
        length = len(raw_value)
        if length <= 4:
            value_str = (_pack_byte(*raw_value) +
                            b"\x00" * (4 - length))
        else:
            value_str = struct.pack(">I", offset)
            four_bytes_over = _pack_byte(*raw_value)
    elif value_type == TYPES.Short:
        length = len(raw_value)
        if length <= 2:
            value_str = (_pack_short(*raw_value) +
                            b"\x00\x00" * (2 - length))
        else:
            value_str = struct.pack(">I", offset)
            four_bytes_over = _pack_short(*raw_value)
    elif value_type == TYPES.Long:
        length = len(raw_value)
        if length <= 1:
            value_str = _pack_long(*raw_value)
        else:
            value_str = struct.pack(">I", offset)
            four_bytes_over = _pack_long(*raw_value)
    elif value_type == TYPES.SLong:
        length = len(raw_value)
        if length <= 1:
            value_str = _pack_slong(*raw_value)
        else:
            value_str = struct.pack(">I", offset)
            four_bytes_over = _pack_slong(*raw_value)
    elif value_type == TYPES.Ascii:
        try:
            new_value = raw_value.encode("latin1") + b"\x00"
        except:
            try:
                new_value = raw_value + b"\x00"
            except TypeError:
                raise ValueError("Got invalid type to convert.")
        length = len(new_value)
        if length > 4:
            value_str = struct.pack(">I", offset)
            four_bytes_over = new_value
        else:
            value_str = new_value + b"\x00" * (4 - length)
    elif value_type == TYPES.Rational:
        if isinstance(raw_value[0], numbers.Integral):
            length = 1
            num, den = raw_value
            new_value = struct.pack(">L", num) + struct.pack(">L", den)
        elif isinstance(raw_value[0], tuple):
            length = len(raw_value)
            new_value = b""
            for n, val in enumerate(raw_value):
                num, den = val
                new_value += (struct.pack(">L", num) +
                                struct.pack(">L", den))
        value_str = struct.pack(">I", offset)
        four_bytes_over = new_value
    elif value_type == TYPES.SRational:
        if isinstance(raw_value[0], numbers.Integral):
            length = 1
            num, den = raw_value
            new_value = struct.pack(">l", num) + struct.pack(">l", den)
        elif isinstance(raw_value[0], tuple):
            length = len(raw_value)
            new_value = b""
            for n, val in enumerate(raw_value):
                num, den = val
                new_value += (struct.pack(">l", num) +
                                struct.pack(">l", den))
        value_str = struct.pack(">I", offset)
        four_bytes_over = new_value
    elif value_type == TYPES.Undefined:
        length = len(raw_value)
        if length > 4:
            value_str = struct.pack(">I", offset)
            try:
                four_bytes_over = b"" + raw_value
            except TypeError:
                raise ValueError("Got invalid type to convert.")
        else:
            try:
                value_str = raw_value + b"\x00" * (4 - length)
            except TypeError:
                raise ValueError("Got invalid type to convert.")
    elif value_type == TYPES.SByte: # Signed Byte
        length = len(raw_value)
        if length <= 4:
            value_str = (_pack_signed_byte(*raw_value) +
                            b"\x00" * (4 - length))
        else:
            value_str = struct.pack(">I", offset)
            four_bytes_over = _pack_signed_byte(*raw_value)
    elif value_type == TYPES.SShort: # Signed Short
        length = len(raw_value)
        if length <= 2:
            value_str = (_pack_signed_short(*raw_value) +
                            b"\x00\x00" * (2 - length))
        else:
            value_str = struct.pack(">I", offset)
            four_bytes_over = _pack_signed_short(*raw_value)
    elif value_type == TYPES.Float:
        length = len(raw_value)
        if length <= 1:
            value_str = _pack_float(*raw_value)
        else:
            value_str = struct.pack(">I", offset)
            four_bytes_over = _pack_float(*raw_value)
    elif value_type == TYPES.DFloat: # Double
        length = len(raw_value)
        value_str = struct.pack(">I", offset)
        four_bytes_over = _pack_double(*raw_value)

    length_str = struct.pack(">I", length)
    return length_str, value_str, four_bytes_over

def _dict_to_bytes(ifd_dict, ifd, ifd_offset):
    tag_count = len(ifd_dict)
    entry_header = struct.pack(">H", tag_count)
    if ifd in ("0th", "1st"):
        entries_length = 2 + tag_count * 12 + 4
    else:
        entries_length = 2 + tag_count * 12
    entries = b""
    values = b""

    for n, key in enumerate(sorted(ifd_dict)):
        if (ifd == "0th") and (key in (ImageIFD.ExifTag, ImageIFD.GPSTag)):
            continue
        elif (ifd == "Exif") and (key == ExifIFD.InteroperabilityTag):
            continue
        elif (ifd == "1st") and (key in (ImageIFD.JPEGInterchangeFormat, ImageIFD.JPEGInterchangeFormatLength)):
            continue

        raw_value = ifd_dict[key]
        key_str = struct.pack(">H", key)
        value_type = TAGS[ifd][key]["type"]
        type_str = struct.pack(">H", value_type)
        four_bytes_over = b""

        if isinstance(raw_value, numbers.Integral) or isinstance(raw_value, float):
            raw_value = (raw_value,)
        offset = TIFF_HEADER_LENGTH + entries_length + ifd_offset + len(values)

        try:
            length_str, value_str, four_bytes_over = _value_to_bytes(raw_value,
                                                                     value_type,
                                                                     offset)
        except ValueError:
            raise ValueError(
                '"dump" got wrong type of exif value.\n' +
                '{0} in {1} IFD. Got as {2}.'.format(key, ifd, type(ifd_dict[key]))
            )

        entries += key_str + type_str + length_str + value_str
        values += four_bytes_over
    return (entry_header + entries, values)
