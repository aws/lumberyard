
import datetime as dt
import os
import shutil
import string
import sys

import numpy as np
import pandas as pd
import pytest
try:
    from pandas.tslib import Timestamp
except ImportError:
    from pandas import Timestamp
from six import PY2

from fastparquet import write, ParquetFile
from fastparquet.test.util import tempdir


def frame_symbol_dtTrade_type_strike(days=1 * 252,
                       start_date=dt.datetime(2005, 1, 1, hour=0, minute=0, second=0),
                       symbols=['SPY', 'FB', 'TLT'],
                       numbercolumns=1):
    base = start_date
    date_list = [base + dt.timedelta(days=x) for x in range(0, days)]
    tuple_list = []
    for x in symbols:
        for y in date_list:
            tuple_list.append((x, y.year, y))
    index = pd.MultiIndex.from_tuples(tuple_list, names=('symbol', 'year', 'dtTrade'))
    np.random.seed(seed=0)
    df = pd.DataFrame(np.random.randn(index.size, numbercolumns),
                      index=index, columns=[x for x in string.ascii_uppercase[0:numbercolumns]])
    return df

@pytest.mark.parametrize('input_symbols,input_days,file_scheme,input_columns,'
                         'partitions,filters',
                         [
                             (['NOW', 'SPY', 'VIX'], 2 * 252, 'hive', 2,
                              ['symbol', 'year'], [('symbol', '==', 'SPY')]),
                             (['now', 'SPY', 'VIX'], 2 * 252, 'hive', 2,
                              ['symbol', 'year'], [('symbol', '==', 'SPY')]),
                             (['TODAY', 'SPY', 'VIX'], 2 * 252, 'hive', 2,
                              ['symbol', 'year'], [('symbol', '==', 'SPY')]),
                             (['VIX*', 'SPY', 'VIX'], 2 * 252, 'hive', 2,
                              ['symbol', 'year'], [('symbol', '==', 'SPY')]),
                             (['QQQ*', 'SPY', 'VIX'], 2 * 252, 'hive', 2,
                              ['symbol', 'year'], [('symbol', '==', 'SPY')]),
                             (['QQQ!', 'SPY', 'VIX'], 2 * 252, 'hive', 2,
                              ['symbol', 'year'], [('symbol', '==', 'SPY')]),
                             (['Q%QQ', 'SPY', 'VIX'], 2 * 252, 'hive', 2,
                              ['symbol', 'year'], [('symbol', '==', 'SPY')]),
                             (['NOW', 'SPY', 'VIX'], 10, 'hive', 2,
                              ['symbol', 'dtTrade'], [('symbol', '==', 'SPY')]),
                             (['NOW', 'SPY', 'VIX'], 10, 'hive', 2,
                              ['symbol', 'dtTrade'],
                              [('dtTrade', '==',
                                '2005-01-02T00:00:00.000000000')]),
                             (['NOW', 'SPY', 'VIX'], 10, 'hive', 2,
                              ['symbol', 'dtTrade'],
                              [('dtTrade', '==',
                                Timestamp('2005-01-01 00:00:00'))]),
                         ]
                         )
def test_frame_write_read_verify(tempdir, input_symbols, input_days,
                                 file_scheme,
                                 input_columns, partitions, filters):
    if os.name == 'nt':
        pytest.xfail("Partitioning folder names contain special characters which are not supported on Windows")

    # Generate Temp Director for parquet Files
    fdir = str(tempdir)
    fname = os.path.join(fdir, 'test')

    # Generate Test Input Frame
    input_df = frame_symbol_dtTrade_type_strike(days=input_days,
                                                symbols=input_symbols,
                                                numbercolumns=input_columns)
    input_df.reset_index(inplace=True)
    write(fname, input_df, partition_on=partitions, file_scheme=file_scheme,
          compression='SNAPPY')

    # Read Back Whole Parquet Structure
    output_df = ParquetFile(fname).to_pandas()
    for col in output_df.columns:
        assert col in input_df.columns.values
    assert len(input_df) == len(output_df)

    # Read with filters
    filtered_output_df = ParquetFile(fname).to_pandas(filters=filters)

    # Filter Input Frame to Match What Should Be Expected from parquet read
    # Handle either string or non-string inputs / works for timestamps
    filterStrings = []
    for name, operator, value in filters:
        if isinstance(value, str):
            value = "'{}'".format(value)
        else:
            value = value.__repr__()
        filterStrings.append("{} {} {}".format(name, operator, value))
    filters_expression = " and ".join(filterStrings)
    filtered_input_df = input_df.query(filters_expression)

    # Check to Ensure Columns Match
    for col in filtered_output_df.columns:
        assert col in filtered_input_df.columns.values
    # Check to Ensure Number of Rows Match
    assert len(filtered_input_df) == len(filtered_output_df)

    # Clean Up
    shutil.rmtree(fdir, ignore_errors=True)
