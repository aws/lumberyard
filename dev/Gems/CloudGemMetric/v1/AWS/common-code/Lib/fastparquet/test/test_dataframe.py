import pandas as pd
from fastparquet.dataframe import empty


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
