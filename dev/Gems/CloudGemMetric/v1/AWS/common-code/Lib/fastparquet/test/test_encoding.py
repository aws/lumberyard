"""test_encoding.py - tests for deserializing parquet data."""
import array
import io
import numpy as np
import struct

import fastparquet.encoding
from fastparquet import parquet_thrift


def test_int32():
    """Test reading bytes containing int32 data."""
    assert 999 == fastparquet.encoding.read_plain(
             struct.pack(b"<i", 999),
             parquet_thrift.Type.INT32, 1)

def test_int64():
    """Test reading bytes containing int64 data."""
    assert 999 == fastparquet.encoding.read_plain(
             struct.pack(b"<q", 999),
             parquet_thrift.Type.INT64, 1)

def test_int96():
    """Test reading bytes containing int96 data."""
    assert b'\x00\x00\x00\x00\x00\x00\x00\x00\xe7\x03\x00\x00' == fastparquet.encoding.read_plain(
             struct.pack(b"<qi", 0, 999),
             parquet_thrift.Type.INT96, 1)

def test_float():
    """Test reading bytes containing float data."""
    assert (9.99 - fastparquet.encoding.read_plain(
             struct.pack(b"<f", 9.99),
             parquet_thrift.Type.FLOAT, 1)) < 0.01

def test_double():
    """Test reading bytes containing double data."""
    assert (9.99 - fastparquet.encoding.read_plain(
             struct.pack(b"<d", 9.99),
             parquet_thrift.Type.DOUBLE, 1)) < 0.01

def test_fixed():
    """Test reading bytes containing fixed bytes data."""
    data = b"foobar"
    assert data[:3] == fastparquet.encoding.read_plain(
            data, parquet_thrift.Type.FIXED_LEN_BYTE_ARRAY, -1, 3)[0]
    assert data[3:] == fastparquet.encoding.read_plain(
            data, parquet_thrift.Type.FIXED_LEN_BYTE_ARRAY, -1, 3)[1]

def test_boolean():
    """Test reading bytes containing boolean data."""
    data = 0b1101
    d = struct.pack(b"<i", data)
    assert ([True, False, True, True] == fastparquet.encoding.read_plain(
            d, parquet_thrift.Type.BOOLEAN, 4)).all(0)


def testFourByteValue():
    """Test reading a run with a single four-byte value."""
    fo = fastparquet.encoding.Numpy8(np.fromstring(struct.pack(b"<i", 1 << 30), np.uint8))
    o = fastparquet.encoding.Numpy32(np.empty(10, np.uint32))
    fastparquet.encoding.read_rle(fo, 2 << 1, 30, o)
    assert ([1 << 30] * 2 == o.so_far()).all()


def testSingleByte():
    """Test reading a single byte value."""
    fo = fastparquet.encoding.Numpy8(np.fromstring(struct.pack(b"<i", 0x7F), np.uint8))
    out = fastparquet.encoding.read_unsigned_var_int(fo)
    assert 0x7F == out

def testFourByte():
    """Test reading a four byte value."""
    fo = fastparquet.encoding.Numpy8(np.fromstring(struct.pack(b"<BBBB", 0xFF, 0xFF, 0xFF, 0x7F), np.uint8))
    out = fastparquet.encoding.read_unsigned_var_int(fo)
    assert 0x0FFFFFFF == out


def testFromExample():
    """Test a simple example."""
    raw_data_in = [0b10001000, 0b11000110, 0b11111010]
    encoded_bitstring = b'\x88\xc6\xfa'
    fo = fastparquet.encoding.Numpy8(np.fromstring(encoded_bitstring, np.uint8))
    count = 8
    o = fastparquet.encoding.Numpy32(np.empty(count, np.uint32))
    fastparquet.encoding.read_bitpacked(fo, count, 3, o)
    assert (list(range(8)) == o.so_far()).all()


def testWidths():
    """Test all possible widths for a single byte."""
    assert 0 == fastparquet.encoding.width_from_max_int(0)
    assert 1 == fastparquet.encoding.width_from_max_int(1)
    assert 2 == fastparquet.encoding.width_from_max_int(2)
    assert 2 == fastparquet.encoding.width_from_max_int(3)
    assert 3 == fastparquet.encoding.width_from_max_int(4)
    assert 3 == fastparquet.encoding.width_from_max_int(5)
    assert 3 == fastparquet.encoding.width_from_max_int(6)
    assert 3 == fastparquet.encoding.width_from_max_int(7)
    assert 4 == fastparquet.encoding.width_from_max_int(8)
    assert 4 == fastparquet.encoding.width_from_max_int(15)
    assert 5 == fastparquet.encoding.width_from_max_int(16)
    assert 5 == fastparquet.encoding.width_from_max_int(31)
    assert 6 == fastparquet.encoding.width_from_max_int(32)
    assert 6 == fastparquet.encoding.width_from_max_int(63)
    assert 7 == fastparquet.encoding.width_from_max_int(64)
    assert 7 == fastparquet.encoding.width_from_max_int(127)
    assert 8 == fastparquet.encoding.width_from_max_int(128)
    assert 8 == fastparquet.encoding.width_from_max_int(255)
