# -*- coding: utf-8 -*-
"""test_converted_types.py - tests for decoding data to their logical data types."""
from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals

import datetime
from decimal import Decimal
import numpy as np
import pandas as pd
import pytest

from fastparquet import parquet_thrift as pt
from fastparquet.converted_types import convert
from fastparquet.util import PY2


def test_int32():
    """Test decimal data stored as int32."""
    schema = pt.SchemaElement(
        type=pt.Type.INT32,
        name="test",
        converted_type=pt.ConvertedType.DECIMAL,
        scale=10,
        precision=9
    )

    assert (convert(pd.Series([9876543210]), schema)[0] - 9.87654321) < 0.01


def test_date():
    """Test int32 encoding a date."""
    schema = pt.SchemaElement(
        type=pt.Type.INT32,
        name="test",
        converted_type=pt.ConvertedType.DATE,
    )
    days = (datetime.date(2004, 11, 3) - datetime.date(1970, 1, 1)).days
    assert (convert(pd.Series([days]), schema)[0] ==
            pd.to_datetime([datetime.date(2004, 11, 3)]))


def test_time_millis():
    """Test int32 encoding a timedelta in millis."""
    schema = pt.SchemaElement(
        type=pt.Type.INT32,
        name="test",
        converted_type=pt.ConvertedType.TIME_MILLIS,
    )
    assert (convert(np.array([731888], dtype='int32'), schema)[0] ==
            np.array([731888], dtype='timedelta64[ms]'))


def test_timestamp_millis():
    """Test int64 encoding a datetime."""
    schema = pt.SchemaElement(
        type=pt.Type.INT64,
        name="test",
        converted_type=pt.ConvertedType.TIMESTAMP_MILLIS,
    )
    assert (convert(np.array([1099511625014], dtype='int64'), schema)[0] ==
            np.array(datetime.datetime(2004, 11, 3, 19, 53, 45, 14 * 1000),
                dtype='datetime64[ns]'))


def test_utf8():
    """Test bytes representing utf-8 string."""
    schema = pt.SchemaElement(
        type=pt.Type.BYTE_ARRAY,
        name="test",
        converted_type=pt.ConvertedType.UTF8
    )
    data = b'\xc3\x96rd\xc3\xb6g'
    assert convert(pd.Series([data]), schema)[0] == u"Ördög"


def test_json():
    """Test bytes representing json."""
    schema = pt.SchemaElement(
        type=pt.Type.BYTE_ARRAY,
        name="test",
        converted_type=pt.ConvertedType.JSON
    )
    assert convert(pd.Series([b'{"foo": ["bar", "\\ud83d\\udc7e"]}']),
                          schema)[0] == {'foo': ['bar', u'👾']}


@pytest.mark.skipif(PY2,reason='BSON unicode may not be supported in Python 2')
def test_bson():
    """Test bytes representing bson."""
    bson = pytest.importorskip('bson')
    schema = pt.SchemaElement(
        type=pt.Type.BYTE_ARRAY,
        name="test",
        converted_type=pt.ConvertedType.BSON
    )
    assert convert(pd.Series(
            [b'&\x00\x00\x00\x04foo\x00\x1c\x00\x00\x00\x020'
             b'\x00\x04\x00\x00\x00bar\x00\x021\x00\x05\x00'
             b'\x00\x00\xf0\x9f\x91\xbe\x00\x00\x00']),
            schema)[0] == {'foo': ['bar', u'👾']}


def test_uint16():
    """Test decoding int32 as uint16."""
    schema = pt.SchemaElement(
        type=pt.Type.INT32,
        name="test",
        converted_type=pt.ConvertedType.UINT_16
    )
    assert convert(pd.Series([-3]), schema)[0] == 65533


def test_uint32():
    """Test decoding int32 as uint32."""
    schema = pt.SchemaElement(
        type=pt.Type.INT32,
        name="test",
        converted_type=pt.ConvertedType.UINT_32
    )
    assert convert(pd.Series([-6884376]), schema)[0] == 4288082920


def test_uint64():
    """Test decoding int64 as uint64."""
    schema = pt.SchemaElement(
        type=pt.Type.INT64,
        name="test",
        converted_type=pt.ConvertedType.UINT_64
    )
    assert convert(pd.Series([-6884376]), schema)[0] == 18446744073702667240


def test_big_decimal():
    schema = pt.SchemaElement(
        type=pt.Type.FIXED_LEN_BYTE_ARRAY,
        name="test",
        converted_type=pt.ConvertedType.DECIMAL,
        type_length=32,
        scale=1,
        precision=38
    )
    data = np.array([
    b'', b'\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x1e\\',
    b'\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x1d\\',
    b'\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\r{',
    b'\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x19)'],
            dtype='|S32')
    assert np.isclose(convert(data, schema),
                      np.array([0., 777.2, 751.6, 345.1, 644.1])).all()
