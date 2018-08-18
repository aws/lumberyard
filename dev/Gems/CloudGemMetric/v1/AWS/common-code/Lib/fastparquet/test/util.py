from itertools import product
import os
import pytest
import tempfile
import shutil

TEST_DATA = "test-data"


@pytest.yield_fixture()
def s3():
    s3fs = pytest.importorskip('s3fs')
    moto = pytest.importorskip('moto')
    m = moto.mock_s3()
    m.start()
    s3 = s3fs.S3FileSystem()
    s3.mkdir(TEST_DATA)
    paths = []
    for cat, catnum in product(('fred', 'freda'), ('1', '2', '3')):
        path = os.sep.join([TEST_DATA, 'split', 'cat=' + cat,
                            'catnum=' + catnum])
        files = os.listdir(path)
        for fn in files:
            full_path = os.path.join(path, fn)
            s3.put(full_path, full_path)
            paths.append(full_path)
    path = os.path.join(TEST_DATA, 'split')
    files = os.listdir(path)
    for fn in files:
        full_path = os.path.join(path, fn)
        if os.path.isdir(full_path):
            continue
        s3.put(full_path, full_path)
        paths.append(full_path)
    yield s3
    for path in paths:
        s3.rm(path)


@pytest.fixture()
def sql():
    pyspark = pytest.importorskip("pyspark")
    sc = pyspark.SparkContext.getOrCreate()
    sql = pyspark.SQLContext(sc)
    return sql


@pytest.yield_fixture()
def tempdir():
    d = tempfile.mkdtemp()
    yield d
    if os.path.exists(d):
        shutil.rmtree(d, ignore_errors=True)
