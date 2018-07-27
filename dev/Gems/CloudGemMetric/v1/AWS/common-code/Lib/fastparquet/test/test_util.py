import pytest

from fastparquet.util import (thrift_copy, analyse_paths, get_file_scheme,
                              val_to_num)
from fastparquet import parquet_thrift


def test_analyse_paths():
    file_list = ['a', 'b']
    base, out = analyse_paths(file_list, '/')
    assert (base, out) == ('', ['a', 'b'])

    file_list = ['c/a', 'c/b']
    base, out = analyse_paths(file_list, '/')
    assert (base, out) == ('c', ['a', 'b'])

    file_list = ['c/d/a', 'c/d/b']
    base, out = analyse_paths(file_list, '/')
    assert (base, out) == ('c/d', ['a', 'b'])

    file_list = ['c/cat=1/a', 'c/cat=2/b', 'c/cat=1/c']
    base, out = analyse_paths(file_list, '/')
    assert (base, out) == ('c', ['cat=1/a', 'cat=2/b', 'cat=1/c'])

    file_list = ['c\\cat=1\\a', 'c\\cat=2\\b', 'c\\cat=1\\c']
    base, out = analyse_paths(file_list, '\\')
    assert (base, out) == ('c', ['cat=1\\a', 'cat=2\\b', 'cat=1\\c'])


def test_thrift_copy():
    fmd = parquet_thrift.FileMetaData()
    rg0 = parquet_thrift.RowGroup()
    rg0.num_rows = 5
    rg1 = parquet_thrift.RowGroup()
    rg1.num_rows = 15
    fmd.row_groups = [rg0, rg1]

    fmd2 = thrift_copy(fmd)

    assert fmd is not fmd2
    assert fmd == fmd2
    assert fmd2.row_groups[0] is not rg0
    rg0.num_rows = 25
    assert fmd2.row_groups[0].num_rows == 5


def test_file_scheme():
    paths = [None, None]
    assert get_file_scheme(paths) == 'simple'
    paths = []
    assert get_file_scheme(paths) == 'empty'  # this is pointless
    paths = ['file']
    assert get_file_scheme(paths) == 'flat'
    paths = ['file', 'file']
    assert get_file_scheme(paths) == 'flat'
    paths = ['a=1/b=2/file', 'a=2/b=1/file']
    assert get_file_scheme(paths) == 'hive'
    paths = ['a=1/z=2/file', 'a=2/b=6/file']  # note key names do not match
    assert get_file_scheme(paths) == 'drill'
    paths = ['a=1/b=2/file', 'a=2/b/file']
    assert get_file_scheme(paths) == 'drill'
    paths = ['a/b/c/file', 'a/b/file']
    assert get_file_scheme(paths) == 'other'


def test_val_to_num():
    assert val_to_num('7') == 7
    assert val_to_num('.7') == .7
    assert val_to_num('0.7') == .7
    assert val_to_num('07') == 7
    assert val_to_num('0') == 0
    assert val_to_num('00') == 0
