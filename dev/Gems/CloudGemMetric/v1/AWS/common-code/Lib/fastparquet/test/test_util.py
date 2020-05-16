import os
import pandas as pd
import pytest

from fastparquet.util import (analyse_paths, get_file_scheme, val_to_num,
                              join_path, groupby_types, get_column_metadata)


def test_analyse_paths():
    file_list = ['a', 'b']
    base, out = analyse_paths(file_list)
    assert (base, out) == ('', ['a', 'b'])

    file_list = ['c/a', 'c/b']
    base, out = analyse_paths(file_list)
    assert (base, out) == ('c', ['a', 'b'])

    file_list = ['c/d/a', 'c/d/b']
    base, out = analyse_paths(file_list)
    assert (base, out) == ('c/d', ['a', 'b'])

    file_list = ['/c/d/a', '/c/d/b']
    base, out = analyse_paths(file_list)
    assert (base, out) == ('/c/d', ['a', 'b'])

    file_list = ['c/cat=1/a', 'c/cat=2/b', 'c/cat=1/c']
    base, out = analyse_paths(file_list)
    assert (base, out) == ('c', ['cat=1/a', 'cat=2/b', 'cat=1/c'])

    file_list = ['c\\cat=1\\a', 'c\\cat=2\\b', 'c\\cat=1\\c']
    temp, os.sep = os.sep, '\\'  # We must trick linux into thinking this is windows for this test to work
    base, out = analyse_paths(file_list)
    os.sep = temp
    assert (base, out) == ('c', ['cat=1/a', 'cat=2/b', 'cat=1/c'])


def test_empty():
    assert join_path("test", ""), "test"


def test_parents():
    assert join_path("test", "../../..") == "../.."

    with pytest.raises(Exception):
        join_path("/test", "../../..")
    with pytest.raises(Exception):
        join_path("/test", "../..")


def test_abs_and_rel_paths():
    assert join_path('/', 'this/is/a/test/') == '/this/is/a/test'
    assert join_path('.', 'this/is/a/test/') == 'this/is/a/test'
    assert join_path('', 'this/is/a/test/') == 'this/is/a/test'
    assert join_path('/test', '.') == '/test'
    assert join_path('/test', '..', 'this') == '/this'
    assert join_path('/test', '../this') == '/this'


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
    assert val_to_num('-20') == -20
    assert val_to_num(7) == 7
    assert val_to_num(0.7) == 0.7
    assert val_to_num(0) == 0
    assert val_to_num('NOW') == 'NOW'
    assert val_to_num('now') == 'now'
    assert val_to_num('TODAY') == 'TODAY'
    assert val_to_num('') == ''
    assert val_to_num('nan') == 'nan'
    assert val_to_num('NaN') == 'NaN'
    assert val_to_num('2018-10-10') == pd.to_datetime('2018-10-10')
    assert val_to_num('2018-10-09') == pd.to_datetime('2018-10-09')
    assert val_to_num('2017-12') == pd.to_datetime('2017-12')
    assert val_to_num('5e+6') == 5e6
    assert val_to_num('5e-6') == 5e-6
    assert val_to_num('0xabc') == '0xabc'
    assert val_to_num('hello world') == 'hello world'
    # The following tests document an idiosyncrasy of val_to_num which is difficult
    # to avoid while timedeltas are supported.
    assert val_to_num('50+20') == pd.to_timedelta('50+20')
    assert val_to_num('50-20') == pd.to_timedelta('50-20')


def test_groupby_types():
    assert len(groupby_types([1, 2, 3])) == 1
    assert len(groupby_types(["1", "2", "3.0"])) == 1
    assert len(groupby_types([1, 2, 3.0])) == 2
    assert len(groupby_types([1, "2", "3.0"])) == 2 
    assert len(groupby_types([pd.to_datetime("2000"), "2000"])) == 2


def test_bad_tz():
    idx = pd.date_range('2012-01-01', periods=3, tz='dateutil/Europe/London')
    with pytest.raises(ValueError):
        get_column_metadata(idx, 'tz')
