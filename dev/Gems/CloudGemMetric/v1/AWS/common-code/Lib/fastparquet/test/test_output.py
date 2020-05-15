# -*- coding: utf-8 -*-
from __future__ import unicode_literals

import datetime
import numpy as np
import os
import pandas as pd
import pandas.util.testing as tm
from fastparquet import ParquetFile
from fastparquet import write, parquet_thrift
from fastparquet import writer, encoding
from pandas.testing import assert_frame_equal
import pytest

from fastparquet.util import default_mkdirs
from fastparquet.test.util import s3, tempdir, sql

TEST_DATA = "test-data"


def test_uvarint():
    values = np.random.randint(0, 15000, size=100)
    o = encoding.Numpy8(np.zeros(30, dtype=np.uint8))
    for v in values:
        o.loc = 0
        writer.encode_unsigned_varint(v, o)
        o.loc = 0
        out = encoding.read_unsigned_var_int(o)
        assert v == out


def test_bitpack():
    for _ in range(10):
        values = np.random.randint(0, 15000, size=np.random.randint(10, 100),
                                   dtype=np.int32)
        width = encoding.width_from_max_int(values.max())
        o = encoding.Numpy8(np.zeros(900, dtype=np.uint8))
        writer.encode_bitpacked(values, width, o)
        o.loc = 0
        head = encoding.read_unsigned_var_int(o)
        out = encoding.Numpy32(np.zeros(300, dtype=np.int32))
        encoding.read_bitpacked(o, head, width, out)
        assert (values == out.so_far()[:len(values)]).all()
        assert out.so_far()[len(values):].sum() == 0  # zero padding
        assert out.loc - len(values) < 8


def test_length():
    lengths = np.random.randint(0, 15000, size=100)
    o = encoding.Numpy8(np.zeros(900, dtype=np.uint8))
    for l in lengths:
        o.loc = 0
        writer.write_length(l, o)
        o.loc = 0
        out = encoding.read_length(o)
        assert l == out


def test_rle_bp():
    for _ in range(10):
        values = np.random.randint(0, 15000, size=np.random.randint(10, 100),
                                   dtype=np.int32)
        out = encoding.Numpy32(np.empty(len(values) + 5, dtype=np.int32))
        o = encoding.Numpy8(np.zeros(900, dtype=np.uint8))
        width = encoding.width_from_max_int(values.max())

        # without length
        writer.encode_rle_bp(values, width, o)
        l = o.loc
        o.loc = 0

        encoding.read_rle_bit_packed_hybrid(o, width, length=l, o=out)
        assert (out.so_far()[:len(values)] == values).all()


def test_roundtrip_s3(s3):
    data = pd.DataFrame({'i32': np.arange(1000, dtype=np.int32),
                         'i64': np.arange(1000, dtype=np.int64),
                         'f': np.arange(1000, dtype=np.float64),
                         'bhello': np.random.choice([b'hello', b'you',
                            b'people'], size=1000).astype("O")})
    data['hello'] = data.bhello.str.decode('utf8')
    data['bcat'] = data.bhello.astype('category')
    data.loc[100, 'f'] = np.nan
    data['cat'] = data.hello.astype('category')
    noop = lambda x: True
    myopen = s3.open
    write(TEST_DATA+'/temp_parq', data, file_scheme='hive',
          row_group_offsets=[0, 500], open_with=myopen, mkdirs=noop)
    myopen = s3.open
    pf = ParquetFile(TEST_DATA+'/temp_parq', open_with=myopen)
    df = pf.to_pandas(categories=['cat', 'bcat'])
    for col in data:
        assert (df[col] == data[col])[~df[col].isnull()].all()


@pytest.mark.parametrize('scheme', ['simple', 'hive'])
@pytest.mark.parametrize('row_groups', [[0], [0, 500]])
@pytest.mark.parametrize('comp', [None, 'GZIP', 'SNAPPY'])
def test_roundtrip(tempdir, scheme, row_groups, comp):
    data = pd.DataFrame({'i32': np.arange(1000, dtype=np.int32),
                         'i64': np.arange(1000, dtype=np.int64),
                         'u64': np.arange(1000, dtype=np.uint64),
                         'f': np.arange(1000, dtype=np.float64),
                         'bhello': np.random.choice([b'hello', b'you',
                            b'people'], size=1000).astype("O")})
    data['a'] = np.array([b'a', b'b', b'c', b'd', b'e']*200, dtype="S1")
    data['aa'] = data['a'].map(lambda x: 2*x).astype("S2")
    data['hello'] = data.bhello.str.decode('utf8')
    data['bcat'] = data.bhello.astype('category')
    data['cat'] = data.hello.astype('category')
    fname = os.path.join(tempdir, 'test.parquet')
    write(fname, data, file_scheme=scheme, row_group_offsets=row_groups,
          compression=comp)

    r = ParquetFile(fname)

    df = r.to_pandas()

    assert data.cat.dtype == 'category'

    for col in r.columns:
        assert (df[col] == data[col]).all()
        # tests https://github.com/dask/fastparquet/issues/250
        assert isinstance(data[col][0], type(df[col][0]))


def test_bad_coltype(tempdir):
    df = pd.DataFrame({'0': [1, 2], (0, 1): [3, 4]})
    fn = os.path.join(tempdir, 'temp.parq')
    with pytest.raises((ValueError, TypeError)) as e:
        write(fn, df)
        assert "tuple" in str(e.value)


def test_bad_col(tempdir):
    df = pd.DataFrame({'x': [1, 2]})
    fn = os.path.join(tempdir, 'temp.parq')
    with pytest.raises(ValueError) as e:
        write(fn, df, has_nulls=['y'])


@pytest.mark.parametrize('scheme', ('simple', 'hive'))
def test_roundtrip_complex(tempdir, scheme,):
    import datetime
    data = pd.DataFrame({'ui32': np.arange(1000, dtype=np.uint32),
                         'i16': np.arange(1000, dtype=np.int16),
                         'ui8': np.array([1, 2, 3, 4]*250, dtype=np.uint8),
                         'f16': np.arange(1000, dtype=np.float16),
                         'dicts': [{'oi': 'you'}] * 1000,
                         't': [datetime.datetime.now()] * 1000,
                         'td': [datetime.timedelta(seconds=1)] * 1000,
                         'bool': np.random.choice([True, False], size=1000)
                         })
    data.loc[100, 't'] = None

    fname = os.path.join(tempdir, 'test.parquet')
    write(fname, data, file_scheme=scheme)

    r = ParquetFile(fname)

    df = r.to_pandas()
    for col in r.columns:
        assert (df[col] == data[col])[~data[col].isnull()].all()


@pytest.mark.parametrize('df', [
    pd.util.testing.makeMixedDataFrame(),
    pd.DataFrame({'x': pd.date_range('3/6/2012 00:00',
                  periods=10, freq='H', tz='Europe/London')}),
    pd.DataFrame({'x': pd.date_range('3/6/2012 00:00',
                  periods=10, freq='H', tz='Europe/Berlin')}),
    pd.DataFrame({'x': pd.date_range('3/6/2012 00:00',
                  periods=10, freq='H', tz='UTC')})
    ])
def test_datetime_roundtrip(tempdir, df, capsys):
    fname = os.path.join(tempdir, 'test.parquet')
    w = False
    if 'x' in df and 'Europe/' in str(df.x.dtype.tz):
        with pytest.warns(UserWarning) as w:
            write(fname, df)
    else:
        write(fname, df)
    r = ParquetFile(fname)

    if w:
        assert any("UTC" in str(wm.message) for wm in w.list)

    df2 = r.to_pandas()

    pd.util.testing.assert_frame_equal(df, df2, check_categorical=False)


def test_nulls_roundtrip(tempdir):
    fname = os.path.join(tempdir, 'temp.parq')
    data = pd.DataFrame({'o': np.random.choice(['hello', 'world', None],
                                               size=1000)})
    data['cat'] = data['o'].astype('category')
    writer.write(fname, data, has_nulls=['o', 'cat'])

    r = ParquetFile(fname)
    df = r.to_pandas()
    for col in r.columns:
        assert (df[col] == data[col])[~data[col].isnull()].all()
        assert (data[col].isnull() == df[col].isnull()).all()


def test_make_definitions_with_nulls():
    for _ in range(10):
        out = np.empty(1000, dtype=np.int32)
        o = encoding.Numpy32(out)
        data = pd.Series(np.random.choice([True, None],
                                          size=np.random.randint(1, 1000)))
        out, d2 = writer.make_definitions(data, False)
        i = encoding.Numpy8(np.frombuffer(out, dtype=np.uint8))
        encoding.read_rle_bit_packed_hybrid(i, 1, length=None, o=o)
        out = o.so_far()[:len(data)]
        assert (out == ~data.isnull()).sum()


def test_make_definitions_without_nulls():
    for _ in range(100):
        out = np.empty(10000, dtype=np.int32)
        o = encoding.Numpy32(out)
        data = pd.Series([True] * np.random.randint(1, 10000))
        out, d2 = writer.make_definitions(data, True)

        l = len(data) << 1
        p = 1
        while l > 127:
            l >>= 7
            p += 1
        assert len(out) == 4 + p + 1  # "length", num_count, value

        i = encoding.Numpy8(np.frombuffer(out, dtype=np.uint8))
        encoding.read_rle_bit_packed_hybrid(i, 1, length=None, o=o)
        out = o.so_far()
        assert (out == ~data.isnull()).sum()

    # class mock:
    #     def is_required(self, *args):
    #         return False
    #     def max_definition_level(self, *args):
    #         return 1
    #     def __getattr__(self, item):
    #         return None
    # halper, metadata = mock(), mock()


def test_empty_row_group(tempdir):
    fname = os.path.join(tempdir, 'temp.parq')
    data = pd.DataFrame({'o': np.random.choice(['hello', 'world'],
                                               size=1000)})
    writer.write(fname, data, row_group_offsets=[0, 900, 1800])
    pf = ParquetFile(fname)
    assert len(pf.row_groups) == 2


@pytest.mark.skip()
def test_write_delta(tempdir):
    fname = os.path.join(tempdir, 'temp.parq')
    data = pd.DataFrame({'i1': np.arange(10, dtype=np.int32) + 2,
                         'i2': np.cumsum(np.random.randint(
                                 0, 5, size=10)).astype(np.int32) + 2})
    writer.write(fname, data, encoding="DELTA_BINARY_PACKED")

    df = sql.read.parquet(fname)
    ddf = df.toPandas()
    for col in data:
        assert (ddf[col] == data[col])[~ddf[col].isnull()].all()


def test_int_rowgroups(tempdir):
    df = pd.DataFrame({'a': [1]*100})
    fname = os.path.join(tempdir, 'test.parq')
    writer.write(fname, df, row_group_offsets=30)
    r = ParquetFile(fname)
    assert [rg.num_rows for rg in r.row_groups] == [25, 25, 25, 25]
    writer.write(fname, df, row_group_offsets=33)
    r = ParquetFile(fname)
    assert [rg.num_rows for rg in r.row_groups] == [25, 25, 25, 25]
    writer.write(fname, df, row_group_offsets=34)
    r = ParquetFile(fname)
    assert [rg.num_rows for rg in r.row_groups] == [34, 34, 32]
    writer.write(fname, df, row_group_offsets=35)
    r = ParquetFile(fname)
    assert [rg.num_rows for rg in r.row_groups] == [34, 34, 32]


@pytest.mark.parametrize('scheme', ['hive', 'drill'])
def test_groups_roundtrip(tempdir, scheme):
    df = pd.DataFrame({'a': np.random.choice(['a', 'b', None], size=1000),
                       'b': np.random.randint(0, 64000, size=1000),
                       'c': np.random.choice([True, False], size=1000)})
    writer.write(tempdir, df, partition_on=['a', 'c'], file_scheme=scheme)

    r = ParquetFile(tempdir)
    assert r.columns == ['b']
    out = r.to_pandas()
    if scheme == 'drill':
        assert set(r.cats) == {'dir0', 'dir1'}
        assert set(out.columns) == {'b', 'dir0', 'dir1'}
        out.rename(columns={'dir0': 'a', 'dir1': 'c'}, inplace=True)

    for i, row in out.iterrows():
        assert row.b in list(df[(df.a == row.a) & (df.c == row.c)].b)

    writer.write(tempdir, df, row_group_offsets=[0, 50], partition_on=['a', 'c'],
                 file_scheme=scheme)

    r = ParquetFile(tempdir)
    assert r.count == sum(~df.a.isnull())
    assert len(r.row_groups) == 8
    out = r.to_pandas()

    if scheme == 'drill':
        assert set(out.columns) == {'b', 'dir0', 'dir1'}
        out.rename(columns={'dir0': 'a', 'dir1': 'c'}, inplace=True)
    for i, row in out.iterrows():
        assert row.b in list(df[(df.a==row.a)&(df.c==row.c)].b)


def test_groups_iterable(tempdir):
    df = pd.DataFrame({'a': np.random.choice(['aaa', 'bbb', None], size=1000),
                       'b': np.random.randint(0, 64000, size=1000),
                       'c': np.random.choice([True, False], size=1000)})
    writer.write(tempdir, df, partition_on=['a'], file_scheme='hive')

    r = ParquetFile(tempdir)
    assert r.columns == ['b', 'c']
    out = r.to_pandas()

    for i, row in out.iterrows():
        assert row.b in list(df[(df.a==row.a)&(df.c==row.c)].b)



def test_empty_groupby(tempdir):
    df = pd.DataFrame({'a': np.random.choice(['a', 'b', None], size=1000),
                       'b': np.random.randint(0, 64000, size=1000),
                       'c': np.random.choice([True, False], size=1000)})
    df.loc[499:, 'c'] = True  # no False in second half
    writer.write(tempdir, df, partition_on=['a', 'c'], file_scheme='hive',
                 row_group_offsets=[0, 500])
    r = ParquetFile(tempdir)
    assert r.count == sum(~df.a.isnull())
    assert len(r.row_groups) == 6
    out = r.to_pandas()

    for i, row in out.iterrows():
        assert row.b in list(df[(df.a==row.a)&(df.c==row.c)].b)


def test_too_many_partition_columns(tempdir):
    df = pd.DataFrame({'a': np.random.choice(['a', 'b', 'c'], size=1000),
                       'c': np.random.choice([True, False], size=1000)})
    with pytest.raises(ValueError) as ve:
        writer.write(tempdir, df, partition_on=['a', 'c'], file_scheme='hive')
    assert "Cannot include all columns" in str(ve.value)


def test_read_partitioned_and_write_with_empty_partions(tempdir):
    df = pd.DataFrame({'a': np.random.choice(['a', 'b', 'c'], size=1000),
                       'c': np.random.choice([True, False], size=1000)})

    writer.write(tempdir, df, partition_on=['a'], file_scheme='hive')
    df_filtered = ParquetFile(tempdir).to_pandas(
                                            filters=[('a', '==', 'b')]
                                            )

    writer.write(tempdir, df_filtered, partition_on=['a'], file_scheme='hive')

    df_loaded = ParquetFile(tempdir).to_pandas()

    tm.assert_frame_equal(df_filtered, df_loaded, check_categorical=False)


@pytest.mark.parametrize('compression', ['GZIP',
                                         'gzip',
                                         None,
                                         {'x': 'GZIP'},
                                         {'y': 'gzip', 'x': None}])
def test_write_compression_dict(tempdir, compression):
    df = pd.DataFrame({'x': [1, 2, 3],
                       'y': [1., 2., 3.]})
    fn = os.path.join(tempdir, 'tmp.parq')
    writer.write(fn, df, compression=compression)
    r = ParquetFile(fn)
    df2 = r.to_pandas()

    tm.assert_frame_equal(df, df2, check_categorical=False)


def test_write_compression_schema(tempdir):
    df = pd.DataFrame({'x': [1, 2, 3],
                       'y': [1., 2., 3.]})
    fn = os.path.join(tempdir, 'tmp.parq')
    writer.write(fn, df, compression={'x': 'gzip'})
    r = ParquetFile(fn)

    assert all(c.meta_data.codec for row in r.row_groups
                                 for c in row.columns
                                 if c.meta_data.path_in_schema == ['x'])
    assert not any(c.meta_data.codec for row in r.row_groups
                                 for c in row.columns
                                 if c.meta_data.path_in_schema == ['y'])


def test_index(tempdir):
    import json
    fn = os.path.join(tempdir, 'tmp.parq')
    df = pd.DataFrame({'x': [1, 2, 3],
                       'y': [1., 2., 3.]},
                       index=pd.Index([10, 20, 30], name='z'))

    writer.write(fn, df)

    pf = ParquetFile(fn)
    assert set(pf.columns) == {'x', 'y', 'z'}
    meta = json.loads(pf.key_value_metadata['pandas'])
    assert meta['index_columns'] == ['z']
    out = pf.to_pandas()
    assert out.index.name == 'z'
    pd.util.testing.assert_frame_equal(df, out)
    out = pf.to_pandas(index=False)
    assert out.index.name is None
    assert (out.index == range(3)).all()
    assert (out.z == df.index).all()


def test_duplicate_columns(tempdir):
    fn = os.path.join(tempdir, 'tmp.parq')
    df = pd.DataFrame(np.arange(12).reshape(4, 3), columns=list('aaa'))
    with pytest.raises(ValueError) as e:
        write(fn, df)
    assert 'duplicate' in str(e.value)


@pytest.mark.parametrize('cmp', [None, 'gzip'])
def test_cmd_bytesize(tempdir, cmp):
    from fastparquet import core
    fn = os.path.join(tempdir, 'tmp.parq')
    df = pd.DataFrame({'s': ['a', 'b']}, dtype='category')
    write(fn, df, compression=cmp)
    pf = ParquetFile(fn)
    chunk = pf.row_groups[0].columns[0]
    cmd = chunk.meta_data
    csize = cmd.total_compressed_size
    f = open(fn, 'rb')
    f.seek(cmd.dictionary_page_offset)
    ph = core.read_thrift(f, parquet_thrift.PageHeader)
    c1 = ph.compressed_page_size
    f.seek(c1, 1)
    ph = core.read_thrift(f, parquet_thrift.PageHeader)
    c2 = ph.compressed_page_size
    f.seek(c2, 1)
    assert csize == f.tell() - cmd.dictionary_page_offset


def test_dotted_column(tempdir):
    fn = os.path.join(tempdir, 'tmp.parq')
    df = pd.DataFrame({'x.y': [1, 2, 3],
                       'y': [1., 2., 3.]})

    writer.write(fn, df)

    out = ParquetFile(fn).to_pandas()
    assert list(out.columns) == ['x.y', 'y']


def test_naive_index(tempdir):
    fn = os.path.join(tempdir, 'tmp.parq')
    df = pd.DataFrame({'x': [1, 2, 3],
                       'y': [1., 2., 3.]})

    writer.write(fn, df)
    r = ParquetFile(fn)

    assert set(r.columns) == {'x', 'y'}

    writer.write(fn, df, write_index=True)
    r = ParquetFile(fn)

    assert set(r.columns) == {'x', 'y', 'index'}


def test_text_convert(tempdir):
    df = pd.DataFrame({'a': [u'π'] * 100,
                       'b': [b'a'] * 100})
    fn = os.path.join(tempdir, 'tmp.parq')

    write(fn, df, fixed_text={'a': 2, 'b': 1})
    pf = ParquetFile(fn)
    assert pf._schema[1].type == parquet_thrift.Type.FIXED_LEN_BYTE_ARRAY
    assert pf._schema[1].type_length == 2
    assert pf._schema[2].type == parquet_thrift.Type.FIXED_LEN_BYTE_ARRAY
    assert pf._schema[2].type_length == 1
    assert pf.statistics['max']['a'] == [u'π']
    df2 = pf.to_pandas()
    tm.assert_frame_equal(df, df2, check_categorical=False)

    write(fn, df)
    pf = ParquetFile(fn)
    assert pf._schema[1].type == parquet_thrift.Type.BYTE_ARRAY
    assert pf._schema[2].type == parquet_thrift.Type.BYTE_ARRAY
    assert pf.statistics['max']['a'] == [u'π']
    df2 = pf.to_pandas()
    tm.assert_frame_equal(df, df2, check_categorical=False)

    write(fn, df, fixed_text={'a': 2})
    pf = ParquetFile(fn)
    assert pf._schema[1].type == parquet_thrift.Type.FIXED_LEN_BYTE_ARRAY
    assert pf._schema[2].type == parquet_thrift.Type.BYTE_ARRAY
    assert pf.statistics['max']['a'] == [u'π']
    df2 = pf.to_pandas()
    tm.assert_frame_equal(df, df2, check_categorical=False)


def test_null_time(tempdir):
    """Test reading a file that contains null records."""
    tmp = str(tempdir)
    expected = pd.DataFrame({"t": [np.timedelta64(), np.timedelta64('NaT')]})
    fn = os.path.join(tmp, "test-time-null.parquet")

    # with NaT
    write(fn, expected, has_nulls=False)
    p = ParquetFile(fn)
    data = p.to_pandas()
    assert (data['t'] == expected['t'])[~expected['t'].isnull()].all()
    assert sum(data['t'].isnull()) == sum(expected['t'].isnull())

    # with NULL
    write(fn, expected, has_nulls=True)
    p = ParquetFile(fn)
    data = p.to_pandas()
    assert (data['t'] == expected['t'])[~expected['t'].isnull()].all()
    assert sum(data['t'].isnull()) == sum(expected['t'].isnull())


def test_auto_null(tempdir):
    tmp = str(tempdir)
    df = pd.DataFrame({'a': [1, 2, 3, 0],
                       'aa': [1, 2, 3, None],
                       'b': [1., 2., 3., np.nan],
                       'c': pd.to_timedelta([1, 2, 3, np.nan], unit='ms'),
                       'd': ['a', 'b', 'c', None],
                       'f': [True, False, True, True],
                       'ff': [True, False, None, True]})
    df['e'] = df['d'].astype('category')
    df['bb'] = df['b'].astype('object')
    df['aaa'] = df['a'].astype('object')
    object_cols = ['d', 'ff', 'bb', 'aaa']
    test_cols = list(set(df) - set(object_cols)) + ['d']
    fn = os.path.join(tmp, "test.parq")

    with pytest.raises(ValueError):
        write(fn, df, has_nulls=False)

    write(fn, df, has_nulls=True)
    pf = ParquetFile(fn)
    for col in pf._schema[1:]:
        assert col.repetition_type == parquet_thrift.FieldRepetitionType.OPTIONAL
    df2 = pf.to_pandas(categories=['e'])

    tm.assert_frame_equal(df[test_cols], df2[test_cols], check_categorical=False)
    tm.assert_frame_equal(df[['ff']].astype('float16'), df2[['ff']])
    tm.assert_frame_equal(df[['bb']].astype('float64'), df2[['bb']])
    tm.assert_frame_equal(df[['aaa']].astype('int64'), df2[['aaa']])

    # not giving any value same as has_nulls=True
    write(fn, df)
    pf = ParquetFile(fn)
    for col in pf._schema[1:]:
        assert col.repetition_type == parquet_thrift.FieldRepetitionType.OPTIONAL
    df2 = pf.to_pandas(categories=['e'])

    tm.assert_frame_equal(df[test_cols], df2[test_cols], check_categorical=False)
    tm.assert_frame_equal(df[['ff']].astype('float16'), df2[['ff']])
    tm.assert_frame_equal(df[['bb']].astype('float64'), df2[['bb']])
    tm.assert_frame_equal(df[['aaa']].astype('int64'), df2[['aaa']])

    # 'infer' is new recommended auto-null
    write(fn, df, has_nulls='infer')
    pf = ParquetFile(fn)
    for col in pf._schema[1:]:
        if col.name in object_cols:
            assert col.repetition_type == parquet_thrift.FieldRepetitionType.OPTIONAL
        else:
            assert col.repetition_type == parquet_thrift.FieldRepetitionType.REQUIRED
    df2 = pf.to_pandas()
    tm.assert_frame_equal(df[test_cols], df2[test_cols], check_categorical=False)
    tm.assert_frame_equal(df[['ff']].astype('float16'), df2[['ff']])
    tm.assert_frame_equal(df[['bb']].astype('float64'), df2[['bb']])
    tm.assert_frame_equal(df[['aaa']].astype('int64'), df2[['aaa']])

    # nut legacy None still works
    write(fn, df, has_nulls=None)
    pf = ParquetFile(fn)
    for col in pf._schema[1:]:
        if col.name in object_cols:
            assert col.repetition_type == parquet_thrift.FieldRepetitionType.OPTIONAL
        else:
            assert col.repetition_type == parquet_thrift.FieldRepetitionType.REQUIRED
    df2 = pf.to_pandas()
    tm.assert_frame_equal(df[test_cols], df2[test_cols], check_categorical=False)
    tm.assert_frame_equal(df[['ff']].astype('float16'), df2[['ff']])
    tm.assert_frame_equal(df[['bb']].astype('float64'), df2[['bb']])
    tm.assert_frame_equal(df[['aaa']].astype('int64'), df2[['aaa']])


@pytest.mark.parametrize('n', (10, 127, 2**8 + 1, 2**16 + 1))
def test_many_categories(tempdir, n):
    tmp = str(tempdir)
    cats = np.arange(n)
    codes = np.random.randint(0, n, size=1000000)
    df = pd.DataFrame({'x': pd.Categorical.from_codes(codes, cats), 'y': 1})
    fn = os.path.join(tmp, "test.parq")

    write(fn, df, has_nulls=False)
    pf = ParquetFile(fn)
    out = pf.to_pandas(categories={'x': n})

    tm.assert_frame_equal(df, out, check_categorical=False)

    df.set_index('x', inplace=True)
    write(fn, df, has_nulls=False, write_index=True)
    pf = ParquetFile(fn)
    out = pf.to_pandas(categories={'x': n}, index='x')

    assert (out.index == df.index).all()
    assert (out.y == df.y).all()


def test_autocat(tempdir):
    tmp = str(tempdir)
    fn = os.path.join(tmp, "test.parq")
    data = pd.DataFrame({'o': pd.Categorical(
        np.random.choice(['hello', 'world'], size=1000))})
    write(fn, data)
    pf = ParquetFile(fn)
    assert 'o' in pf.categories
    assert pf.categories['o'] == 2
    assert str(pf.dtypes['o']) == 'category'
    out = pf.to_pandas()
    assert out.dtypes['o'] == 'category'
    out = pf.to_pandas(categories={})
    assert str(out.dtypes['o']) != 'category'
    out = pf.to_pandas(categories=['o'])
    assert out.dtypes['o'].kind == 'O'
    out = pf.to_pandas(categories={'o': 2})
    assert out.dtypes['o'].kind == 'O'


@pytest.mark.parametrize('row_groups', ([0], [0, 2]))
@pytest.mark.parametrize('dirs', (['', ''], ['cat=1', 'cat=2']))
def test_merge(tempdir, dirs, row_groups):
    fn = str(tempdir)

    default_mkdirs(os.path.join(fn, dirs[0]))
    df0 = pd.DataFrame({'a': [1, 2, 3, 4]})
    fn0 = os.sep.join([fn, dirs[0], 'out0.parq'])
    write(fn0, df0, row_group_offsets=row_groups)

    default_mkdirs(os.path.join(fn, dirs[1]))
    df1 = pd.DataFrame({'a': [5, 6, 7, 8]})
    fn1 = os.sep.join([fn, dirs[1], 'out1.parq'])
    write(fn1, df1, row_group_offsets=row_groups)

    # with file-names
    pf = writer.merge([fn0, fn1])
    assert len(pf.row_groups) == 2 * len(row_groups)
    out = pf.to_pandas().a.tolist()
    assert out == [1, 2, 3, 4, 5, 6, 7, 8]
    if "cat=1" in dirs:
        assert 'cat' in pf.cats

    # with instances
    pf = writer.merge([ParquetFile(fn0), ParquetFile(fn1)])
    assert len(pf.row_groups) == 2 * len(row_groups)
    out = pf.to_pandas().a.tolist()
    assert out == [1, 2, 3, 4, 5, 6, 7, 8]
    if "cat=1" in dirs:
        assert 'cat' in pf.cats


def test_merge_s3(tempdir, s3):
    fn = str(tempdir)

    df0 = pd.DataFrame({'a': [1, 2, 3, 4]})
    fn0 = TEST_DATA + '/out0.parq'
    write(fn0, df0, open_with=s3.open)

    df1 = pd.DataFrame({'a': [5, 6, 7, 8]})
    fn1 = TEST_DATA + '/out1.parq'
    write(fn1, df1, open_with=s3.open)

    # with file-names
    pf = writer.merge([fn0, fn1], open_with=s3.open)
    assert len(pf.row_groups) == 2
    out = pf.to_pandas().a.tolist()
    assert out == [1, 2, 3, 4, 5, 6, 7, 8]


def test_merge_fail(tempdir):
    fn = str(tempdir)

    df0 = pd.DataFrame({'a': [1, 2, 3, 4]})
    fn0 = os.sep.join([fn, 'out0.parq'])
    write(fn0, df0)

    df1 = pd.DataFrame({'a': ['a', 'b', 'c']})
    fn1 = os.sep.join([fn, 'out1.parq'])
    write(fn1, df1)

    with pytest.raises(ValueError) as e:
        writer.merge([fn0, fn1])
    assert 'schemas' in str(e.value)


def test_append_simple(tempdir):
    fn = os.path.join(str(tempdir), 'test.parq')
    df = pd.DataFrame({'a': [1, 2, 3, 0],
                       'b': ['a', 'a', 'b', 'b']})
    write(fn, df, write_index=False)
    write(fn, df, append=True, write_index=False)

    pf = ParquetFile(fn)
    expected = pd.concat([df, df], ignore_index=True)
    pd.util.testing.assert_frame_equal(pf.to_pandas(), expected,
                                       check_categorical=False)


@pytest.mark.parametrize('scheme', ('hive', 'simple'))
def test_append_empty(tempdir, scheme):
    fn = os.path.join(str(tempdir), 'test.parq')
    df = pd.DataFrame({'a': [1, 2, 3, 0],
                       'b': ['a', 'a', 'b', 'b']})
    write(fn, df.head(0), write_index=False, file_scheme=scheme)
    pf = ParquetFile(fn)
    assert pf.count == 0
    assert pf.file_scheme == 'empty'
    write(fn, df, append=True, write_index=False, file_scheme=scheme)

    pf = ParquetFile(fn)
    pd.util.testing.assert_frame_equal(pf.to_pandas(), df,
                                       check_categorical=False)



@pytest.mark.parametrize('row_groups', ([0], [0, 2]))
@pytest.mark.parametrize('partition', ([], ['b']))
def test_append(tempdir, row_groups, partition):
    fn = str(tempdir)
    df0 = pd.DataFrame({'a': [1, 2, 3, 0],
                        'b': ['a', 'b', 'a', 'b'],
                        'c': True})
    df1 = pd.DataFrame({'a': [4, 5, 6, 7],
                        'b': ['a', 'b', 'a', 'b'],
                        'c': False})
    write(fn, df0, partition_on=partition, file_scheme='hive',
          row_group_offsets=row_groups)
    write(fn, df1, partition_on=partition, file_scheme='hive',
          row_group_offsets=row_groups, append=True)

    pf = ParquetFile(fn)

    expected = pd.concat([df0, df1], ignore_index=True)

    assert len(pf.row_groups) == 2 * len(row_groups) * (len(partition) + 1)
    items_out = {tuple(row[1])
                 for row in pf.to_pandas()[['a', 'b', 'c']].iterrows()}
    items_in = {tuple(row[1])
                for row in expected.iterrows()}
    assert items_in == items_out


def test_append_fail(tempdir):
    fn = str(tempdir)
    df0 = pd.DataFrame({'a': [1, 2, 3, 0],
                        'b': ['a', 'b', 'a', 'b'],
                        'c': True})
    df1 = pd.DataFrame({'a': [4, 5, 6, 7],
                        'b': ['a', 'b', 'a', 'b'],
                        'c': False})
    write(fn, df0, file_scheme='hive')
    with pytest.raises(ValueError) as e:
        write(fn, df1, file_scheme='simple', append=True)
    assert 'existing file scheme' in str(e.value)

    fn2 = os.path.join(fn, 'temp.parq')
    write(fn2, df0, file_scheme='simple')
    with pytest.raises(ValueError) as e:
        write(fn2, df1, file_scheme='hive', append=True)
    assert 'existing file scheme' in str(e.value)


def test_append_w_partitioning(tempdir):
    fn = str(tempdir)
    df = pd.DataFrame({'a': np.random.choice([1, 2, 3], size=50),
                       'b': np.random.choice(['hello', 'world'], size=50),
                       'c': np.random.randint(50, size=50)})
    write(fn, df, file_scheme='hive', partition_on=['a', 'b'])
    write(fn, df, file_scheme='hive', partition_on=['a', 'b'], append=True)
    write(fn, df, file_scheme='hive', partition_on=['a', 'b'], append=True)
    write(fn, df, file_scheme='hive', partition_on=['a', 'b'], append=True)
    pf = ParquetFile(fn)
    out = pf.to_pandas()
    assert len(out) == 200
    assert sorted(out.a)[::4] == sorted(df.a)
    with pytest.raises(ValueError):
        write(fn, df, file_scheme='hive', partition_on=['a'], append=True)
    with pytest.raises(ValueError):
        write(fn, df, file_scheme='hive', partition_on=['b', 'a'], append=True)


def test_bad_object_encoding(tempdir):
    df = pd.DataFrame({'x': ['a', 'ab']})
    with pytest.raises(ValueError) as e:
        write(str(tempdir), df, object_encoding='utf-8')
    assert "utf-8" in str(e.value)


def test_empty_dataframe(tempdir):
    df = pd.DataFrame({'a': [], 'b': []}, dtype=int)
    fn = os.path.join(str(tempdir), 'test.parquet')
    write(fn, df)
    pf = ParquetFile(fn)
    out = pf.to_pandas()
    assert pf.count == 0
    assert len(out) == 0
    assert (out.columns == df.columns).all()
    assert pf.statistics


def test_hasnulls_ordering(tempdir):
    fname = os.path.join(tempdir, 'temp.parq')
    data = pd.DataFrame({'a': np.random.rand(100),
                         'b': np.random.rand(100),
                         'c': np.random.rand(100)})
    writer.write(fname, data, has_nulls=['a', 'c'])

    r = ParquetFile(fname)
    assert r._schema[1].name == 'a'
    assert r._schema[1].repetition_type == 1
    assert r._schema[2].name == 'b'
    assert r._schema[2].repetition_type == 0
    assert r._schema[3].name == 'c'
    assert r._schema[3].repetition_type == 1


def test_cats_in_part_files(tempdir):
    df = pd.DataFrame({'a': pd.Categorical(['a', 'b'] * 100)})
    writer.write(tempdir, df, file_scheme='hive', row_group_offsets=50)
    import glob
    files = glob.glob(os.path.join(tempdir, 'part*'))
    pf = ParquetFile(tempdir)
    assert len(pf.row_groups) == 4
    kv = pf.fmd.key_value_metadata
    assert kv
    for f in files:
        pf = ParquetFile(f)
        assert pf.fmd.key_value_metadata == kv
        assert len(pf.row_groups) == 1
    out = pd.concat([ParquetFile(f).to_pandas() for f in files],
                    ignore_index=True)
    pd.util.testing.assert_frame_equal(df, out)


def test_cats_and_nulls(tempdir):
    df = pd.DataFrame({'x': pd.Categorical([1, 2, 1])})
    fn = os.path.join(tempdir, 'temp.parq')
    write(fn, df)
    pf = ParquetFile(fn)
    assert not pf.schema.is_required('x')
    out = pf.to_pandas()
    assert out.dtypes['x'] == 'category'
    assert out.x.tolist() == [1, 2, 1]


def test_consolidate_cats(tempdir):
    import json
    df = pd.DataFrame({'x': pd.Categorical([1, 2, 1])})
    fn = os.path.join(tempdir, 'temp.parq')
    write(fn, df)
    pf = ParquetFile(fn)
    assert 2 == json.loads(pf.fmd.key_value_metadata[0].value)['columns'][0][
        'metadata']['num_categories']
    start = pf.row_groups[0].columns[0].meta_data.key_value_metadata[0].value
    assert start == '2'
    pf.row_groups[0].columns[0].meta_data.key_value_metadata[0].value = '5'
    writer.consolidate_categories(pf.fmd)
    assert 5 == json.loads(pf.fmd.key_value_metadata[0].value)['columns'][0][
        'metadata']['num_categories']


def test_bad_object_encoding(tempdir):
    df = pd.DataFrame({'a': [b'00']})
    with pytest.raises(ValueError) as e:
        write(tempdir, df, file_scheme='hive', object_encoding='utf8')
    assert "UTF8" in str(e.value)
    assert "bytes" in str(e.value)
    assert '"a"' in str(e.value)

    df = pd.DataFrame({'a': [0, "hello", 0]})
    with pytest.raises(ValueError) as e:
        write(tempdir, df, file_scheme='hive', object_encoding='int')
    assert "INT64" in str(e.value)
    assert "primitive" in str(e.value)
    assert '"a"' in str(e.value)

def test_object_encoding_int32(tempdir):
    df = pd.DataFrame({'a': ['15', None, '2']})
    fn = os.path.join(tempdir, 'temp.parq')
    write(fn, df, object_encoding={'a': 'int32'})
    pf = ParquetFile(fn)
    assert pf._schema[1].type == parquet_thrift.Type.INT32
    assert not pf.schema.is_required('a')
