import os
import pickle
from fastparquet import ParquetFile, parquet_thrift
from fastparquet.test.util import TEST_DATA
from fastparquet.schema import schema_tree


def test_serialize():
    fn = os.path.join(TEST_DATA, "nation.impala.parquet")
    pf = ParquetFile(fn)
    fmd2 = pickle.loads(pickle.dumps(pf.fmd))
    schema_tree(fmd2.schema)  # because we added fake fields when loading pf
    assert fmd2 == pf.fmd

    rg = pf.row_groups[0]
    rg2 = pickle.loads(pickle.dumps(rg))
    assert rg == rg2