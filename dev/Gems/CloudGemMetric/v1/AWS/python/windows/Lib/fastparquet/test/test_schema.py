import os

import numpy as np
from pandas import DataFrame
import pytest

from fastparquet import ParquetFile
from fastparquet import write
from fastparquet.test.util import tempdir


def _generate_random_dataframe(n_rows=1000):
    # borrowing approach from test/test_output.py, test_roundtrip_s3 function
    data_dict = dict(
        i32=np.arange(n_rows, dtype=np.int32),
        i64=np.arange(n_rows, dtype=np.int64),
        f32=np.arange(n_rows, dtype=np.float32),
        f64=np.arange(n_rows, dtype=np.float64),
        obj=np.random.choice(["some", "random", "words"],
                             size=n_rows).astype("O")
    )
    data = DataFrame(data_dict)
    return data


def _convert_to_parquet(dfs, tempdir, path_prefix):
    parquet_files = {}
    for name, df in dfs.items():
        path_base = "{}_{}.parquet".format(path_prefix, name)
        path = os.path.join(tempdir, path_base)
        write(path, df)
        parquet_files[name] = ParquetFile(path)
    return parquet_files


def test_schema_eq(tempdir):
    dfs = {key: _generate_random_dataframe() for key in ("A", "B")}
    parquet_files = _convert_to_parquet(dfs, tempdir, "test_scheme_eq")
    assert parquet_files["A"].schema == parquet_files["B"].schema

def test_schema_ne_subset(tempdir):
    # schemas don't match, one is subset of other
    dfs = {key: _generate_random_dataframe() for key in ("A", "B")}
    dfs["B"].drop("i32", axis="columns", inplace=True)
    parquet_files = _convert_to_parquet(dfs, tempdir, "test_scheme_ne_subset")
    assert parquet_files["A"].schema != parquet_files["B"].schema

def test_schema_ne_renamed(tempdir):
    # schemas don't match, at least one name doesn't match (dtypes match)
    dfs = {key: _generate_random_dataframe() for key in ("A", "B")}
    dfs["B"].rename({"i32": "new_name"}, axis="columns", inplace=True)
    parquet_files = _convert_to_parquet(dfs, tempdir, "test_scheme_ne_renamed")
    assert parquet_files["A"].schema != parquet_files["B"].schema

def test_schema_ne_converted(tempdir):
    # schemas don't match, at least one dtype doesn't match (names match)
    dfs = {key: _generate_random_dataframe() for key in ("A", "B")}
    dfs["B"]["i32"] = dfs["B"]["i32"].astype(np.int64)
    parquet_files = _convert_to_parquet(dfs, tempdir, "test_scheme_ne_convert")
    assert parquet_files["A"].schema != parquet_files["B"].schema

def test_schema_ne_different_order(tempdir):
    # schemas don't match, column order is different
    dfs = {key: _generate_random_dataframe() for key in ("A", "B")}
    dfs["B"] = dfs["B"][dfs["B"].columns[::-1]]
    parquet_files = _convert_to_parquet(dfs, tempdir, "test_scheme_ne_order")
    assert parquet_files["A"].schema != parquet_files["B"].schema
