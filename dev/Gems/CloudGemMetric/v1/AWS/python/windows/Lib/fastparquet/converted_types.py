# -#- coding: utf-8 -#-
"""
Deal with parquet logical types (aka converted types), higher-order things built from primitive types.

The implementations in this class are pure python for the widest compatibility,
but they're not necessarily the most performant.
"""

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import codecs
import datetime
import json
import logging
import numba
import numpy as np
import binascii

import sys

from .thrift_structures import parquet_thrift
from .util import PY2
from .speedups import array_decode_utf8

logger = logging.getLogger('parquet')  # pylint: disable=invalid-name

try:
    from bson import BSON
    unbson = BSON.decode
    tobson = BSON.encode
except ImportError:
    try:
        import bson
        unbson = bson.loads
        tobson = bson.dumps
    except:
        def unbson(x):
            raise ImportError("BSON not found")
        def tobson(x):
            raise ImportError("BSON not found")

DAYS_TO_MILLIS = 86400000000000
"""Number of millis in a day. Used to convert a Date to a date"""
nat = np.datetime64('NaT').view('int64')

simple = {parquet_thrift.Type.INT32: np.dtype('int32'),
          parquet_thrift.Type.INT64: np.dtype('int64'),
          parquet_thrift.Type.FLOAT: np.dtype('float32'),
          parquet_thrift.Type.DOUBLE: np.dtype('float64'),
          parquet_thrift.Type.BOOLEAN: np.dtype('bool'),
          parquet_thrift.Type.INT96: np.dtype('S12'),
          parquet_thrift.Type.BYTE_ARRAY: np.dtype("O"),
          parquet_thrift.Type.FIXED_LEN_BYTE_ARRAY: np.dtype("O")}
complex = {parquet_thrift.ConvertedType.UTF8: np.dtype("O"),
           parquet_thrift.ConvertedType.DECIMAL: np.dtype('float64'),
           parquet_thrift.ConvertedType.UINT_8: np.dtype('uint8'),
           parquet_thrift.ConvertedType.UINT_16: np.dtype('uint16'),
           parquet_thrift.ConvertedType.UINT_32: np.dtype('uint32'),
           parquet_thrift.ConvertedType.UINT_64: np.dtype('uint64'),
           parquet_thrift.ConvertedType.INT_8: np.dtype('int8'),
           parquet_thrift.ConvertedType.INT_16: np.dtype('int16'),
           parquet_thrift.ConvertedType.INT_32: np.dtype('int32'),
           parquet_thrift.ConvertedType.INT_64: np.dtype('int64'),
           parquet_thrift.ConvertedType.TIME_MILLIS: np.dtype('<m8[ns]'),
           parquet_thrift.ConvertedType.DATE: np.dtype('<M8[ns]'),
           parquet_thrift.ConvertedType.TIMESTAMP_MILLIS: np.dtype('<M8[ns]'),
           parquet_thrift.ConvertedType.TIME_MICROS: np.dtype('<m8[ns]'),
           parquet_thrift.ConvertedType.TIMESTAMP_MICROS: np.dtype('<M8[ns]')
           }


def typemap(se):
    """Get the final dtype - no actual conversion"""
    if se.converted_type is None:
        if se.type in simple:
            return simple[se.type]
        else:
            return np.dtype("S%i" % se.type_length)
    if se.converted_type in complex:
        return complex[se.converted_type]
    return np.dtype("O")


def convert(data, se, timestamp96=True):
    """Convert known types from primitive to rich.

    Parameters
    ----------
    data: pandas series of primitive type
    se: a schema element.
    timestamp96: convert int96 as if it were written by mr-parquet
    """
    ctype = se.converted_type
    if se.type == parquet_thrift.Type.INT96 and timestamp96:
        data2 = data.view([('ns', 'i8'), ('day', 'i4')])
        return ((data2['day'] - 2440588) * 86400000000000 +
                data2['ns']).view('M8[ns]')
    if ctype is None:
        return data
    if ctype == parquet_thrift.ConvertedType.UTF8:
        if isinstance(data, list) or data.dtype != "O":
            data = np.asarray(data, dtype="O")
        return array_decode_utf8(data)
    if ctype == parquet_thrift.ConvertedType.DECIMAL:
        scale_factor = 10**-se.scale
        if data.dtype.kind in ['i', 'f']:
            return data * scale_factor
        else:  # byte-string
            # NB: general but slow method
            # could optimize when data.dtype.itemsize <= 8
            if PY2:
                def from_bytes(d):
                    return int(binascii.b2a_hex(d), 16) if len(d) else 0
                by = data.tostring()
                its = data.dtype.itemsize
                return np.array([
                    from_bytes(by[i * its:(i + 1) * its]) * scale_factor
                    for i in range(len(data))
                ])
            else:
                # NB: `from_bytes` may be py>=3.4 only
                return np.array([
                    int.from_bytes(
                        data.data[i:i + 1], byteorder='big', signed=True
                    ) * scale_factor
                    for i in range(len(data))
                ])
    elif ctype == parquet_thrift.ConvertedType.DATE:
        return (data * DAYS_TO_MILLIS).view('datetime64[ns]')
    elif ctype == parquet_thrift.ConvertedType.TIME_MILLIS:
        out = np.empty(len(data), dtype='int64')
        if sys.platform == 'win32':
            data = data.astype('int64')
        time_shift(data, out, 1000000)
        return out.view('timedelta64[ns]')
    elif ctype == parquet_thrift.ConvertedType.TIMESTAMP_MILLIS:
        out = np.empty_like(data)
        time_shift(data, out, 1000000)
        return out.view('datetime64[ns]')
    elif ctype == parquet_thrift.ConvertedType.TIME_MICROS:
        out = np.empty_like(data)
        time_shift(data, out)
        return out.view('timedelta64[ns]')
    elif ctype == parquet_thrift.ConvertedType.TIMESTAMP_MICROS:
        out = np.empty_like(data)
        time_shift(data, out)
        return out.view('datetime64[ns]')
    elif ctype == parquet_thrift.ConvertedType.UINT_8:
        return data.astype(np.uint8)
    elif ctype == parquet_thrift.ConvertedType.UINT_16:
        return data.astype(np.uint16)
    elif ctype == parquet_thrift.ConvertedType.UINT_32:
        return data.astype(np.uint32)
    elif ctype == parquet_thrift.ConvertedType.UINT_64:
        return data.astype(np.uint64)
    elif ctype == parquet_thrift.ConvertedType.INT_8:
        return data.astype(np.int8)
    elif ctype == parquet_thrift.ConvertedType.INT_16:
        return data.astype(np.int16)
    elif ctype == parquet_thrift.ConvertedType.INT_32:
        return data.astype(np.int32)
    elif ctype == parquet_thrift.ConvertedType.INT_64:
        return data.astype(np.int64)
    elif ctype == parquet_thrift.ConvertedType.JSON:
        if isinstance(data, list) or data.dtype != "O":
            out = np.empty(len(data), dtype="O")
        else:
            out = data
        out[:] = [json.loads(d.decode('utf8')) for d in data]
        return out
    elif ctype == parquet_thrift.ConvertedType.BSON:
        if isinstance(data, list) or data.dtype != "O":
            out = np.empty(len(data), dtype="O")
        else:
            out = data
        out[:] = [unbson(d) for d in data]
        return out
    elif ctype == parquet_thrift.ConvertedType.INTERVAL:
        # for those that understand, output is month, day, ms
        # maybe should convert to timedelta
        return data.view('<u4').reshape((len(data), -1))
    else:
        logger.info("Converted type '%s'' not handled",
                    parquet_thrift.ConvertedType._VALUES_TO_NAMES[ctype])  # pylint:disable=protected-access
    return data


@numba.njit(nogil=True)
def time_shift(indata, outdata, factor=1000):  # pragma: no cover
    for i in range(len(indata)):
        if indata[i] == nat:
            outdata[i] = nat
        else:
            outdata[i] = indata[i] * factor
