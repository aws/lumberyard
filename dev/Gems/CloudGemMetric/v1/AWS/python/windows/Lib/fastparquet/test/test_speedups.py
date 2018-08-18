# -*- coding: utf-8 -*-

import struct

import pytest

import numpy as np

import pandas as pd
import pandas.util.testing as tm

from fastparquet.speedups import (
    array_encode_utf8, array_decode_utf8,
    pack_byte_array, unpack_byte_array
    )
from fastparquet.util import PY2, PY3

strings = [u"abc", u"a\x00c", u"héhé", u"プログラミング"]


def test_array_encode_utf8():
    arr = np.array(strings, dtype='object')
    expected = [s.encode('utf-8') for s in strings]
    got = array_encode_utf8(arr)

    assert got.dtype == np.dtype('object')
    assert list(got) == expected

    ser = pd.Series(arr)
    got = array_encode_utf8(ser)

    assert got.dtype == np.dtype('object')
    assert list(got) == expected

    # Wrong array type
    arr = np.array(strings, dtype='U')
    with pytest.raises((TypeError, ValueError)):
        array_encode_utf8(arr)

    # Disabled for v2
    if PY3:
        # Non-encodable string (lone surrogate)
        # on py2 this works anyway
        invalid_string = u"\uDE80"
        arr = np.array(strings + [invalid_string], dtype='object')
        with pytest.raises(UnicodeEncodeError):
            array_encode_utf8(arr)

    # Wrong object type
    arr = np.array([b"foo"], dtype='object')
    with pytest.raises(TypeError):
        array_encode_utf8(arr)


def test_array_decode_utf8():
    bytestrings = [s.encode('utf-8') for s in strings]

    arr = np.array(bytestrings, dtype='object')
    expected = list(strings)
    got = array_decode_utf8(arr)

    assert got.dtype == np.dtype('object')
    assert list(got) == expected

    # Non-decodable string
    arr = np.array(bytestrings + [b"\x00\xff"], dtype='object')
    with pytest.raises(UnicodeDecodeError):
        array_decode_utf8(arr)

    # Wrong array type
    arr = np.array(bytestrings, dtype='S')
    with pytest.raises((TypeError, ValueError)):
        array_decode_utf8(arr)

    # Wrong object type
    arr = np.array([u"foo"], dtype='object')
    with pytest.raises(TypeError):
        array_decode_utf8(arr)


def test_pack_byte_array():
    bytestrings = [b"foo", b"bar\x00" * 256 + b"z"]

    expected = b''.join(struct.pack('<L', len(b)) + b
                        for b in bytestrings)

    b = pack_byte_array(bytestrings)
    assert b == expected

    b = pack_byte_array([])
    assert b == b''

    with pytest.raises(TypeError):
        pack_byte_array(tuple(bytestrings))

    with pytest.raises(TypeError):
        pack_byte_array(bytestrings + [u"foo"])

    with pytest.raises(TypeError):
        pack_byte_array(b"foo")


def test_unpack_byte_array():
    bytestrings = [b"foo", b"bar\x00" * 256 + b"z"]

    packed = b''.join(struct.pack('<L', len(b)) + b
                      for b in bytestrings)

    seq = unpack_byte_array(packed, len(bytestrings))
    assert seq == bytestrings

    def check_invalid_length(b, n):
        with pytest.raises(RuntimeError):
            unpack_byte_array(b, n)

    check_invalid_length(packed, len(bytestrings) + 1)
    check_invalid_length(packed[:-1], len(bytestrings))
    check_invalid_length(packed[:-1], len(bytestrings))

    # Extra bytes silently ignored
    seq = unpack_byte_array(packed, len(bytestrings) - 1)
    assert seq == bytestrings[:-1]
    seq = unpack_byte_array(packed + b'\x00', len(bytestrings))
    assert seq == bytestrings
    seq = unpack_byte_array(packed + b'\x01\x02\x03\x04', len(bytestrings))
    assert seq == bytestrings
