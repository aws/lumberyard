"""test_read.py - unit and integration tests for reading parquet data."""

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals

from itertools import product
import numpy as np
import os

import pandas as pd
import pytest

import fastparquet
from fastparquet import writer, core

from fastparquet.test.util import TEST_DATA, s3, tempdir


def test_header_magic_bytes(tempdir):
    """Test reading the header magic bytes."""
    fn = os.path.join(tempdir, 'temp.parq')
    with open(fn, 'wb') as f:
        f.write(b"PAR1_some_bogus_data")
    with pytest.raises(fastparquet.ParquetException):
        p = fastparquet.ParquetFile(fn, verify=True)


@pytest.mark.parametrize("size", [1, 4, 12, 20])
def test_read_footer_fail(tempdir, size):
    """Test reading the footer."""
    import struct
    fn = os.path.join(TEST_DATA, "nation.impala.parquet")
    fout = os.path.join(tempdir, "temp.parquet")
    with open(fn, 'rb') as f1:
        with open(fout, 'wb') as f2:
            f1.seek(-8, 2)
            head_size = struct.unpack('<i', f1.read(4))[0]
            f1.seek(-(head_size + 8), 2)
            block = f1.read(head_size)
            f2.write(b'0' * 25)  # padding
            f2.write(block[:-size])
            f2.write(f1.read())
    with pytest.raises(TypeError):
        p = fastparquet.ParquetFile(fout)


def test_read_footer():
    """Test reading the footer."""
    p = fastparquet.ParquetFile(os.path.join(TEST_DATA, "nation.impala.parquet"))
    snames = {"schema", "n_regionkey", "n_name", "n_nationkey", "n_comment"}
    assert {s.name for s in p._schema} == snames
    assert set(p.columns) == snames - {"schema"}

files = [os.path.join(TEST_DATA, p) for p in
         ["gzip-nation.impala.parquet", "nation.dict.parquet",
          "nation.impala.parquet", "nation.plain.parquet",
          "snappy-nation.impala.parquet"]]
csvfile = os.path.join(TEST_DATA, "nation.csv")
cols = ["n_nationkey", "n_name", "n_regionkey", "n_comment"]
expected = pd.read_csv(csvfile, delimiter="|", index_col=0, names=cols)


def test_read_s3(s3):
    s3fs = pytest.importorskip('s3fs')
    s3 = s3fs.S3FileSystem()
    myopen = s3.open
    pf = fastparquet.ParquetFile(TEST_DATA+'/split/_metadata', open_with=myopen)
    df = pf.to_pandas()
    assert df.shape == (2000, 3)
    assert (df.cat.value_counts() == [1000, 1000]).all()


@pytest.mark.parametrize("parquet_file", files)
def test_file_csv(parquet_file):
    """Test the various file times
    """
    p = fastparquet.ParquetFile(parquet_file)
    data = p.to_pandas()
    if 'comment_col' in data.columns:
        mapping = {'comment_col': "n_comment", 'name': 'n_name',
                   'nation_key': 'n_nationkey', 'region_key': 'n_regionkey'}
        data.columns = [mapping[k] for k in data.columns]
    data.set_index('n_nationkey', inplace=True)

    for col in cols[1:]:
        if isinstance(data[col][0], bytes):
            data[col] = data[col].str.decode('utf8')
        assert (data[col] == expected[col]).all()


def test_null_int():
    """Test reading a file that contains null records."""
    p = fastparquet.ParquetFile(os.path.join(TEST_DATA, "test-null.parquet"))
    data = p.to_pandas()
    expected = pd.DataFrame([{"foo": 1, "bar": 2}, {"foo": 1, "bar": None}])
    for col in data:
        assert (data[col] == expected[col])[~expected[col].isnull()].all()
        assert sum(data[col].isnull()) == sum(expected[col].isnull())


def test_converted_type_null():
    """Test reading a file that contains null records for a plain column that
     is converted to utf-8."""
    p = fastparquet.ParquetFile(os.path.join(TEST_DATA,
                                         "test-converted-type-null.parquet"))
    data = p.to_pandas()
    expected = pd.DataFrame([{"foo": "bar"}, {"foo": None}])
    for col in data:
        if isinstance(data[col][0], bytes):
            # Remove when re-implemented converted types
            data[col] = data[col].str.decode('utf8')
        assert (data[col] == expected[col])[~expected[col].isnull()].all()
        assert sum(data[col].isnull()) == sum(expected[col].isnull())


def test_null_plain_dictionary():
    """Test reading a file that contains null records for a plain dictionary
     column."""
    p = fastparquet.ParquetFile(os.path.join(TEST_DATA,
                                         "test-null-dictionary.parquet"))
    data = p.to_pandas()
    expected = pd.DataFrame([{"foo": None}] + [{"foo": "bar"},
                             {"foo": "baz"}] * 3)
    for col in data:
        if isinstance(data[col][1], bytes):
            # Remove when re-implemented converted types
            data[col] = data[col].str.decode('utf8')
        assert (data[col] == expected[col])[~expected[col].isnull()].all()
        assert sum(data[col].isnull()) == sum(expected[col].isnull())


def test_dir_partition():
    """Test creation of categories from directory structure"""
    x = np.arange(2000)
    df = pd.DataFrame({
        'num': x,
        'cat': pd.Series(np.array(['fred', 'freda'])[x%2], dtype='category'),
        'catnum': pd.Series(np.array([1, 2, 3])[x%3], dtype='category')})
    pf = fastparquet.ParquetFile(os.path.join(TEST_DATA, "split"))
    out = pf.to_pandas()
    for cat, catnum in product(['fred', 'freda'], [1, 2, 3]):
        assert (df.num[(df.cat == cat) & (df.catnum == catnum)].tolist()) ==\
                out.num[(out.cat == cat) & (out.catnum == catnum)].tolist()
    assert out.cat.dtype == 'category'
    assert out.catnum.dtype == 'category'
    assert out.catnum.cat.categories.dtype == 'int64'


def test_stat_filters():
    path = os.path.join(TEST_DATA, 'split')
    pf = fastparquet.ParquetFile(path)
    base_shape = len(pf.to_pandas())

    filters = [('num', '>', 0)]
    assert len(pf.to_pandas(filters=filters)) == base_shape

    filters = [('num', '<', 0)]
    assert len(pf.to_pandas(filters=filters)) == 0

    filters = [('num', '>', 500)]
    assert 0 < len(pf.to_pandas(filters=filters)) < base_shape

    filters = [('num', '>', 1500)]
    assert 0 < len(pf.to_pandas(filters=filters)) < base_shape

    filters = [('num', '>', 2000)]
    assert len(pf.to_pandas(filters=filters)) == 0

    filters = [('num', '>=', 1999)]
    assert 0 < len(pf.to_pandas(filters=filters)) < base_shape

    filters = [('num', '!=', 1000)]
    assert len(pf.to_pandas(filters=filters)) == base_shape

    filters = [('num', 'in', [-1, -2])]
    assert len(pf.to_pandas(filters=filters)) == 0

    filters = [('num', 'not in', [-1, -2])]
    assert len(pf.to_pandas(filters=filters)) == base_shape

    filters = [('num', 'in', [0])]
    l = len(pf.to_pandas(filters=filters))
    assert 0 < l < base_shape

    filters = [('num', 'in', [0, 1500])]
    assert l < len(pf.to_pandas(filters=filters)) < base_shape

    filters = [('num', 'in', [-1, 1999])]
    l = len(pf.to_pandas(filters=filters))
    assert 0 < l < base_shape


def test_cat_filters():
    path = os.path.join(TEST_DATA, 'split')
    pf = fastparquet.ParquetFile(path)
    base_shape = len(pf.to_pandas())

    filters = [('cat', '==', 'freda')]
    assert len(pf.to_pandas(filters=filters)) == 1000

    filters = [('cat', '!=', 'freda')]
    assert len(pf.to_pandas(filters=filters)) == 1000

    filters = [('cat', 'in', ['fred', 'freda'])]
    assert 0 < len(pf.to_pandas(filters=filters)) == 2000

    filters = [('cat', 'not in', ['fred', 'frederick'])]
    assert 0 < len(pf.to_pandas(filters=filters)) == 1000

    filters = [('catnum', '==', 2000)]
    assert len(pf.to_pandas(filters=filters)) == 0

    filters = [('catnum', '>=', 2)]
    assert 0 < len(pf.to_pandas(filters=filters)) == 1333

    filters = [('catnum', '>=', 1)]
    assert len(pf.to_pandas(filters=filters)) == base_shape

    filters = [('catnum', 'in', [0, 1])]
    assert len(pf.to_pandas(filters=filters)) == 667

    filters = [('catnum', 'not in', [1, 2, 3])]
    assert len(pf.to_pandas(filters=filters)) == 0

    filters = [('cat', '==', 'freda'), ('catnum', '>=', 2.5)]
    assert len(pf.to_pandas(filters=filters)) == 333

    filters = [('cat', '==', 'freda'), ('catnum', '!=', 2.5)]
    assert len(pf.to_pandas(filters=filters)) == 1000


def test_statistics(tempdir):
    s = pd.Series([b'a', b'b', b'c']*20)
    df = pd.DataFrame({'a': s, 'b': s.astype('category'),
                       'c': s.astype('category').cat.as_ordered()})
    fastparquet.write(tempdir, df, file_scheme='hive')
    pf = fastparquet.ParquetFile(tempdir)
    stat = pf.statistics
    assert stat['max']['a'] == [b'c']
    assert stat['min']['a'] == [b'a']
    assert stat['max']['b'] == [None]
    assert stat['min']['b'] == [None]
    assert stat['max']['c'] == [b'c']
    assert stat['min']['c'] == [b'a']


def test_grab_cats(tempdir):
    s = pd.Series(['a', 'c', 'b']*20)
    df = pd.DataFrame({'a': s, 'b': s.astype('category'),
                       'c': s.astype('category').cat.as_ordered()})
    fastparquet.write(tempdir, df, file_scheme='hive')
    pf = fastparquet.ParquetFile(tempdir)
    cats = pf.grab_cats(['b', 'c'])
    assert (cats['b'] == df.b.cat.categories).all()
    assert (cats['c'] == df.c.cat.categories).all()


def test_index(tempdir):
    s = pd.Series(['a', 'c', 'b']*20)
    df = pd.DataFrame({'a': s, 'b': s.astype('category'),
                       'c': range(60, 0, -1)})

    for column in df:
        d2 = df.set_index(column)
        fastparquet.write(tempdir, d2, file_scheme='hive', write_index=True)
        pf = fastparquet.ParquetFile(tempdir)
        out = pf.to_pandas(index=column, categories=['b'])
        pd.util.testing.assert_frame_equal(out, d2, check_categorical=False)


def test_skip_length():
    class MockIO:
        loc = 0
    for num in [1, 63, 64, 64*127, 64*128, 63*128**2, 64*128**2]:
        block, _ = writer.make_definitions(np.zeros(num), True)
        MockIO.loc = 0
        core.skip_definition_bytes(MockIO, num)
        assert len(block) == MockIO.loc


def test_timestamp96():
    pf = fastparquet.ParquetFile(os.path.join(TEST_DATA, 'mr_times.parq'))
    out = pf.to_pandas()
    expected = pd.to_datetime(
            ["2016-08-01 23:08:01", "2016-08-02 23:08:02",
             "2016-08-03 23:08:03", "2016-08-04 23:08:04",
             "2016-08-05 23:08:04", "2016-08-06 23:08:05",
             "2016-08-07 23:08:06", "2016-08-08 23:08:07",
             "2016-08-09 23:08:08", "2016-08-10 23:08:09"])
    assert (out['date_added'] == expected).all()


def test_bad_catsize(tempdir):
    df = pd.DataFrame({'a': pd.Categorical([str(i) for i in range(1024)])})
    fastparquet.write(tempdir, df, file_scheme='hive')
    pf = fastparquet.ParquetFile(tempdir)
    assert pf.categories == {'a': 1024}
    with pytest.raises(RuntimeError):
        pf.to_pandas(categories={'a': 2})


def test_null_sizes(tempdir):
    df = pd.DataFrame({'a': [True, None], 'b': [3000, np.nan]}, dtype="O")
    fastparquet.write(tempdir, df, has_nulls=True, file_scheme='hive')
    pf = fastparquet.ParquetFile(tempdir)
    assert pf.dtypes['a'] == 'float16'
    assert pf.dtypes['b'] == 'float64'


def test_multi_index(tempdir):
    r = pd.date_range('2000', '2000-01-03')
    df = pd.DataFrame({'a': r, 'b': [1, 3, 3], 'c': [1.0, np.nan, 3]})
    df = df.set_index(['a', 'b'])
    fastparquet.write(tempdir, df, has_nulls=True, file_scheme='hive')
    dg = fastparquet.ParquetFile(tempdir).to_pandas()
    assert dg.shape == (3, 1)
    assert len(dg.index.levels) == 2
    assert dg.index.levels[0].name == 'a'
    assert dg.index.levels[0].dtype == '<M8[ns]'
    assert dg.index.levels[1].name == 'b'
    assert dg.index.levels[1].dtype == np.int64


def test_multi_index_category(tempdir):
    r = pd.date_range('2000', '2000-01-03')
    df = pd.DataFrame({'a': r, 'b': ['X', 'X', 'L'], 'c': [1.0, np.nan, 3]})
    df['b'] = df['b'].astype('category')
    df = df.set_index(['a', 'b'])
    fastparquet.write(tempdir, df, has_nulls=True, file_scheme='hive')
    dg = fastparquet.ParquetFile(tempdir).to_pandas()
    assert dg.shape == (3, 1)
    assert len(dg.index.levels) == 2
    assert dg.index.levels[0].name == 'a'
    assert dg.index.levels[0].dtype == '<M8[ns]'
    assert dg.index.levels[1].name == 'b'
    assert dg.equals(df)

def test_no_columns(tempdir):
    # https://github.com/dask/fastparquet/issues/361
    # Create a non-empty DataFrame, then select no columns. That way we get
    # _some_ rows, _no_ columns.
    #
    # df = pd.DataFrame({"A": [1, 2]})[[]]
    # fastparquet.write("test-data/no_columns.parquet", df)
    pf = fastparquet.ParquetFile(os.path.join(TEST_DATA, "no_columns.parquet"))
    assert pf.count == 2
    assert pf.columns == []
    result = pf.to_pandas()
    expected = pd.DataFrame({"A": [1, 2]})[[]]
    assert len(result) == 2
    pd.testing.assert_frame_equal(result, expected)

def test_map_multipage(tempdir):
    pf = fastparquet.ParquetFile(os.path.join(TEST_DATA, "map-test.snappy.parquet"))
    assert pf.count == 3551
    df = pf.to_pandas()
    first_row_keys = [u'FoxNews.com', u'News Network', u'mobile technology', u'broadcast', u'sustainability',
                      u'collective intelligence', u'radio', u'business law', u'LLC', u'telecommunications',
                      u'FOX News Network']
    last_row_keys = [u'protests', u'gas mask', u'Pot & Painting Party', u'Denver', u'New Year', u'Anderson Cooper',
                     u'gas mask bonk', u'digital media', u'marijuana leaf earrings', u'Screengrab', u'gas mask bongs',
                     u'Randi Kaye', u'Lee Rogers', u'Andy Cohen', u'CNN', u'Times Square', u'Colorado', u'opera',
                     u'slavery', u'Kathy Griffin', u'marijuana cigarette', u'executive producer']

    assert len(df) == 3551
    assert sorted(df["topics"].iloc[0].keys()) == sorted(first_row_keys)
    assert sorted(df["topics"].iloc[-1].keys()) == sorted(last_row_keys)
    assert df.isnull().sum().sum() == 0 # ensure every row got converted

def test_map_last_row_split(tempdir):
    pf = fastparquet.ParquetFile(os.path.join(TEST_DATA, "test-map-last-row-split.parquet"))
    assert pf.count == 2428
    df = pf.to_pandas()
    # file has 3 pages - rows at index 1210 and 2427 are split in-between neighboring pages
    first_split_row_keys = [u'White House', u'State Department', u'Tatverd\xe4chtige', u'financial economics',
                            u'Hezbollah', u'Bashar Assad', u'break-down', u'paper', u'radio', u'musicals',
                            u'Vladimir Putin', u'Hill two', u'The New York Times and Washington Post', u'tweet',
                            u'guest bedroom', u'Susie Tompkins Buell', u'private law', u'Tammy Bruce',
                            u'Obama Presidential Library', u'Fox News', u'President Trump', u'John Kerry',
                            u'Vanity Fair', u'government', u'Josh Meyer', u'The Hill', u'Esprit Clothing',
                            u'Rainer Wendt', u'Fitness', u'u.n.', u'David Brock', u'fleas', u'Trump', u'WORKOUT',
                            u'Washington', u'Brandenburg Gate', u'Lisa Bloom', u'festgenommen', u'journalist',
                            u'Kolleg', u'Middle East', u'financial markets', u'gym equipment', u'weight training',
                            u'reference', u'Solche Taten', u'digital radio', u'Stephen l. Miller', u'Belleon Body',
                            u'harassment', u'East', u'investment', u'creatures', u'Islamic Republic', u'New Year',
                            u'New York City', u'Media Research center', u'Neue Osnabruecker Zeitung daily newspaper',
                            u'Berlin', u'gegen diese Taten vorgehen', u'safety', u'Jarrett Blanc', u'Tehran',
                            u'America', u'Black Lives Matter', u'pussy hats',
                            u'wurden bislang leider vereinzelt sexuelle \xdcbergriffe gemeldet', u'Roger Cohen',
                            u'u.s.', u'Donald Trump', u'Emily Shire', u'hardline', u'common law', u'animal workouts',
                            u'Hamas', u'operas', u'New York Times', u'Amanda Hess', u'Adrian Carrasquillo',
                            u'Lukas Mikelionis', u'Koi', u'TOUGHEST MUDDER', u'Middle Eastern', u'Erik Wemple',
                            u'Associated Press', u'Iran', u'out-of-pocket expenses', u'Neue Osnabruecker Zeitung',
                            u'lizards', u'Carlos Leon', u'Polizei Berlin Einsatz', u'Russia', u'Russian',
                            u'Berlin Wall', u'Obama', u'The Times', u'The New York Post', u'Mark Halperin',
                            u'learning programs', u'NBC', u'American', u'Jeff Bell',
                            u'Heat Street and National Review Online', u'Dan Merica', u'Tel Aviv',
                            u'Wielding Money', u'anxiety', u'Bell', u'Twitter', u'Hillary Clinton',
                            u'physical exercise', u'Fellow Times', u'property', u'Paul Krugman', u'FoxNews.com',
                            u'Times Square New Year', u'Mika Brzezinksi', u'Ayatollah Ali Khamenei', u'Nikki Haley',
                            u'Obama Library', u'internet-based works', u'Quadriga', u'Washington Post',
                            u'Angela Merkel', u'Manhattan', u'United Nations', u'information', u'Israel',
                            u'Wir haben zivile', u'administration', u'United States', u'Maya Kosoff', u'Germany',
                            u'donor', u'television terminology', u'Bloom', u'The Washington Post', u'Jack Shafer',
                            u'Bei den Veranstaltungen', u'singles', u'uprising', u'reporting', u'AP',
                            u'Fox News Opinion', u'celebrity lawyer', u'Dan Gainor', u'CNN', u'Syria',
                            u'business law', u'inspiration', u'regime', u'Politico', u'Democratic Party',
                            u'The New York Times', u'websites', u'socio-economics', u'Jerusalem']
    second_split_row_keys = [u'Stockton University', u'Walter Montelione', u'law enforcement', u'shooting',
                             u'international incidents', u'NYE', u'Linda Kologi', u'criminal law',
                             u'Long Branch Police Department', u'Kaitlyn Schallhorn', u'Brittany Kologi', u'suspect',
                             u'teenager', u'Monmouth County', u'television terminology', u'Fox News', u'Long Branch',
                             u'Monmouth County prosecutor\u2019s Office', u'Galloway Township', u'Dave Farmer',
                             u'Steven Kologi jr.', u'u.s.', u'incident', u'WCBS-TV', u'Christopher j. Gramiccioni',
                             u"Diane D'Amico", u'New Jersey', u'shooter', u'maritime incidents',
                             u'Monmouth County Prosecutor', u'Steven Kologi', u'Bryan Llenas', u'Mary Schultz',
                             u'NJ.com', u'n.j.', u'Veronica Mass']
    assert len(df) == 2428
    assert sorted(df["topics"].iloc[1210].keys()) == sorted(first_split_row_keys)
    assert sorted(df["topics"].iloc[2427].keys()) == sorted(second_split_row_keys)
    assert df.isnull().sum().sum() == 0


def test_truncated_decimal():
    # protect against numpy truncation of fixed-length-bytes
    pf = fastparquet.ParquetFile(os.path.join(TEST_DATA, "decimals.parquet"))
    df = pf.to_pandas()
    expected = pd.Series(
        [93, 155, 102, 80, 85.5, 109, 105, 139, 91, 105],
        name='weight measure:WEIGHT(KG, 0)')
    out = df['weight measure:WEIGHT(KG, 0)']
    assert np.allclose(expected, out)
