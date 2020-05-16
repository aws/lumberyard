"""encoding.py - methods for reading parquet encoded data blocks."""

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import array
import numba
import numpy as np

from .speedups import unpack_byte_array
from .thrift_structures import parquet_thrift
from .util import byte_buffer


@numba.njit(nogil=True)
def unpack_boolean(data, out):
    for i in range(len(data)):
        d = data[i]
        for j in range(8):
            out[i * 8 + j] = d & 1
            d >>= 1


def read_plain_boolean(raw_bytes, count):
    """Read `count` booleans using the plain encoding."""
    data = np.frombuffer(raw_bytes, dtype='uint8')
    padded = len(raw_bytes) * 8
    out = np.empty(padded, dtype=bool)
    unpack_boolean(data, out)
    return out[:count]


DECODE_TYPEMAP = {
    parquet_thrift.Type.INT32: np.int32,
    parquet_thrift.Type.INT64: np.int64,
    parquet_thrift.Type.INT96: np.dtype('S12'),
    parquet_thrift.Type.FLOAT: np.float32,
    parquet_thrift.Type.DOUBLE: np.float64,
}


def read_plain(raw_bytes, type_, count, width=0):
    if type_ in DECODE_TYPEMAP:
        dtype = DECODE_TYPEMAP[type_]
        return np.frombuffer(byte_buffer(raw_bytes), dtype=dtype, count=count)
    if type_ == parquet_thrift.Type.FIXED_LEN_BYTE_ARRAY:
        if count == 1:
            width = len(raw_bytes)
        dtype = np.dtype('S%i' % width)
        return np.frombuffer(byte_buffer(raw_bytes), dtype=dtype, count=count)
    if type_ == parquet_thrift.Type.BOOLEAN:
        return read_plain_boolean(raw_bytes, count)
    # variable byte arrays (rare)
    try:
        return np.array(unpack_byte_array(raw_bytes, count), dtype='O')
    except RuntimeError:
        if count == 1:
            # e.g., for statistics
            return np.array([raw_bytes], dtype='O')
        else:
            raise


@numba.jit(nogil=True)
def read_unsigned_var_int(file_obj):  # pragma: no cover
    """Read a value using the unsigned, variable int encoding.
    file-obj is a NumpyIO of bytes; avoids struct to allow numba-jit
    """
    result = 0
    shift = 0
    while True:
        byte = file_obj.read_byte()
        result |= ((byte & 0x7F) << shift)
        if (byte & 0x80) == 0:
            break
        shift += 7
    return result


@numba.njit(nogil=True)
def read_rle(file_obj, header, bit_width, o):  # pragma: no cover
    """Read a run-length encoded run from the given fo with the given header and bit_width.

    The count is determined from the header and the width is used to grab the
    value that's repeated. Yields the value repeated count times.
    """
    count = header >> 1
    width = (bit_width + 7) // 8
    zero = np.zeros(4, dtype=np.int8)
    data = file_obj.read(width)
    zero[:len(data)] = data
    value = zero.view(np.int32)
    o.write_many(value, count)


@numba.njit(nogil=True)
def width_from_max_int(value):  # pragma: no cover
    """Convert the value specified to a bit_width."""
    for i in range(0, 64):
        if value == 0:
            return i
        value >>= 1


@numba.njit(nogil=True)
def _mask_for_bits(i):  # pragma: no cover
    """Generate a mask to grab `i` bits from an int value."""
    return (1 << i) - 1


@numba.njit(nogil=True)
def read_bitpacked(file_obj, header, width, o):  # pragma: no cover
    """
    Read values packed into width-bits each (which can be >8)

    file_obj is a NumbaIO array (int8)
    o is an output NumbaIO array (int32)
    """
    num_groups = header >> 1
    count = num_groups * 8
    byte_count = (width * count) // 8
    raw_bytes = file_obj.read(byte_count)
    mask = _mask_for_bits(width)
    current_byte = 0
    data = raw_bytes[current_byte]
    bits_wnd_l = 8
    bits_wnd_r = 0
    total = byte_count * 8
    while total >= width:
        # NOTE zero-padding could produce extra zero-values
        if bits_wnd_r >= 8:
            bits_wnd_r -= 8
            bits_wnd_l -= 8
            data >>= 8
        elif bits_wnd_l - bits_wnd_r >= width:
            o.write_byte((data >> bits_wnd_r) & mask)
            total -= width
            bits_wnd_r += width
        elif current_byte + 1 < byte_count:
            current_byte += 1
            data |= (raw_bytes[current_byte] << bits_wnd_l)
            bits_wnd_l += 8


@numba.njit(nogil=True)
def read_rle_bit_packed_hybrid(io_obj, width, length=None, o=None):  # pragma: no cover
    """Read values from `io_obj` using the rel/bit-packed hybrid encoding.

    If length is not specified, then a 32-bit int is read first to grab the
    length of the encoded data.

    file-obj is a NumpyIO of bytes; o if an output NumpyIO of int32
    """
    if length is None:
        length = read_length(io_obj)
    start = io_obj.loc
    while io_obj.loc-start < length and o.loc < o.len:
        header = read_unsigned_var_int(io_obj)
        if header & 1 == 0:
            read_rle(io_obj, header, width, o)
        else:
            read_bitpacked(io_obj, header, width, o)
    io_obj.loc = start + length
    return o.so_far()


@numba.njit(nogil=True)
def read_length(file_obj):  # pragma: no cover
    """ Numpy trick to get a 32-bit length from four bytes

    Equivalent to struct.unpack('<i'), but suitable for numba-jit
    """
    sub = file_obj.read(4)
    return sub[0] + sub[1]*256 + sub[2]*256*256 + sub[3]*256*256*256


class NumpyIO(object):  # pragma: no cover
    """
    Read or write from a numpy arra like a file object

    This class is numba-jit-able (for specific dtypes)
    """
    def __init__(self, data):
        self.data = data
        self.len = data.size
        self.loc = 0

    def read(self, x):
        self.loc += x
        return self.data[self.loc-x:self.loc]

    def read_byte(self):
        self.loc += 1
        return self.data[self.loc-1]

    def write(self, d):
        l = len(d)
        self.loc += l
        self.data[self.loc-l:self.loc] = d

    def write_byte(self, b):
        if self.loc >= self.len:
            # ignore attempt to write past end of buffer
            return
        self.data[self.loc] = b
        self.loc += 1

    def write_many(self, b, count):
        self.data[self.loc:self.loc+count] = b
        self.loc += count

    def tell(self):
        return self.loc

    def so_far(self):
        """ In write mode, the data we have gathered until now
        """
        return self.data[:self.loc]

spec8 = [('data', numba.uint8[:]), ('loc', numba.int64), ('len', numba.int64)]
Numpy8 = numba.jitclass(spec8)(NumpyIO)
spec32 = [('data', numba.uint32[:]), ('loc', numba.int64), ('len', numba.int64)]
Numpy32 = numba.jitclass(spec32)(NumpyIO)


def _assemble_objects(assign, defi, rep, val, dic, d, null, null_val, max_defi, prev_i):
    """Dremel-assembly of arrays of values into lists

    Parameters
    ----------
    assign: array dtype O
        To insert lists into
    defi: int array
        Definition levels, max 3
    rep: int array
        Repetition levels, max 1
    dic: array of labels or None
        Applied if d is True
    d: bool
        Whether to dereference dict values
    null: bool
        Can an entry be None?
    null_val: bool
        can list elements be None
    max_defi: int
        value of definition level that corresponds to non-null
    prev_i: int
        1 + index where the last row in the previous page was inserted (0 if first page)
    """
    ## TODO: good case for cython
    if d:
        # dereference dict values
        val = dic[val]
    i = prev_i
    vali = 0
    part = []
    started = False
    have_null = False
    if defi is None:
        defi = value_maker(max_defi)
    for de, re in zip(defi, rep):
        if not re:
            # new row - save what we have
            if started:
                assign[i] = None if have_null else part
                part = []
                i += 1
            else:
                # first time: no row to save yet, unless it's a row continued from previous page
                if vali > 0:
                    assign[i - 1].extend(part) # add the items to previous row
                    part = []
                    # don't increment i since we only filled i-1
                started = True
        if de == max_defi:
            # append real value to current item
            part.append(val[vali])
            vali += 1
        elif de > null:
            # append null to current item
            part.append(None)
        # next object is None as opposed to an object
        have_null = de == 0 and null
    if started: # normal case - add the leftovers to the next row
        assign[i] = None if have_null else part
    else: # can only happen if the only elements in this page are the continuation of the last row from previous page
        assign[i - 1].extend(part)
    return i


def value_maker(val):
    while True:
        yield val
