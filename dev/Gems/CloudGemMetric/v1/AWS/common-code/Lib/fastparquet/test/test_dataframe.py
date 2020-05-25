import distutils
import warnings

import pytest
import pandas as pd

from fastparquet.dataframe import empty


if distutils.version.LooseVersion(pd.__version__) >= "0.24.0":
    DatetimeTZDtype = pd.DatetimeTZDtype
else:
    DatetimeTZDtype = pd.api.types.DatetimeTZDtype


def test_empty():
    n = 100
    df, views = empty('category', size=n, cols=['c'])
    assert df.shape == (n, 1)
    assert df.dtypes.tolist() == ['category']
    assert views['c'].dtype == 'int16'

    df, views = empty('category', size=n, cols=['c'], cats={'c': 2**20})
    assert df.shape == (n, 1)
    assert df.dtypes.tolist() == ['category']
    assert views['c'].dtype == 'int32'

    df, views = empty('category', size=n, cols=['c'],
                      cats={'c': ['one', 'two']})
    views['c'][0] = 1
    assert df.c[:2].tolist() == ['two', 'one']

    df, views = empty('i4,i8,f8,f8,O', size=n,
                      cols=['i4', 'i8', 'f8_1', 'f8_2', 'O'])
    assert df.shape == (n, 5)
    assert len(views) == 5


def test_empty_tz():
    warnings.simplefilter("error", DeprecationWarning)

    with pytest.warns(None) as e:
        empty([DatetimeTZDtype(unit="ns", tz="UTC")], 10, cols=['a'],
              timezones={'a': 'UTC'})

    assert len(e) == 0, e


def test_timestamps():
    z = 'US/Eastern'

    # single column
    df, views = empty('M8', 100, cols=['t'])
    assert df.t.dt.tz is None
    views['t'].dtype.kind == "M"

    df, views = empty('M8', 100, cols=['t'], timezones={'t': z})
    assert df.t.dt.tz.zone == z
    views['t'].dtype.kind == "M"

    # one time column, one normal
    df, views = empty('M8,i', 100, cols=['t', 'i'], timezones={'t': z})
    assert df.t.dt.tz.zone == z
    views['t'].dtype.kind == "M"
    views['i'].dtype.kind == 'i'

    # no effect of timezones= on non-time column
    df, views = empty('M8,i', 100, cols=['t', 'i'], timezones={'t': z, 'i': z})
    assert df.t.dt.tz.zone == z
    assert df.i.dtype.kind == 'i'
    views['t'].dtype.kind == "M"
    views['i'].dtype.kind == 'i'

    # multi-timezones
    z2 = 'US/Central'
    df, views = empty('M8,M8', 100, cols=['t1', 't2'], timezones={'t1': z,
                                                                  't2': z})
    assert df.t1.dt.tz.zone == z
    assert df.t2.dt.tz.zone == z

    df, views = empty('M8,M8', 100, cols=['t1', 't2'], timezones={'t1': z})
    assert df.t1.dt.tz.zone == z
    assert df.t2.dt.tz is None

    df, views = empty('M8,M8', 100, cols=['t1', 't2'], timezones={'t1': z,
                                                                  't2': 'UTC'})
    assert df.t1.dt.tz.zone == z
    assert df.t2.dt.tz.zone == 'UTC'

    df, views = empty('M8,M8', 100, cols=['t1', 't2'], timezones={'t1': z,
                                                                  't2': z2})
    assert df.t1.dt.tz.zone == z
    assert df.t2.dt.tz.zone == z2
