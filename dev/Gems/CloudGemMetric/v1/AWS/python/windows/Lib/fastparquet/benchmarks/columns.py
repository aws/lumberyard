from contextlib import contextmanager
import os
import numpy as np
import pandas as pd
import time
from fastparquet import write, ParquetFile
from dask.utils import tmpdir


@contextmanager
def measure(name, result):
    t0 = time.time()
    yield
    t1 = time.time()
    result[name] = round((t1 - t0) * 1000, 3)


def time_column():
    with tmpdir() as tempdir:
        result = {}
        fn = os.path.join(tempdir, 'temp.parq')
        n = 10000000
        r = np.random.randint(-1e10, 1e10, n)
        d = pd.DataFrame({'w': pd.Categorical(np.random.choice(
                ['hi', 'you', 'people'], size=n)),
                          'x': r.view('timedelta64[ns]'),
                          'y': r / np.random.randint(1, 1000, size=n),
                          'z': np.random.randint(0, 127, size=n,
                                                 dtype=np.uint8)})

        for col in d.columns:
            df = d[[col]]
            write(fn, df)
            with measure('%s: write, no nulls' % d.dtypes[col], result):
                write(fn, df, has_nulls=False)

            pf = ParquetFile(fn)
            pf.to_pandas(categories={'w': 3})  # warm-up

            with measure('%s: read, no nulls' % d.dtypes[col], result):
                pf.to_pandas(categories={'w': 3})

            with measure('%s: write, no nulls, has_null=True' % d.dtypes[col], result):
                write(fn, df, has_nulls=True)

            pf = ParquetFile(fn)
            pf.to_pandas(categories={'w': 3})  # warm-up

            with measure('%s: read, no nulls, has_null=True' % d.dtypes[col], result):
                pf.to_pandas(categories={'w': 3})

            if d.dtypes[col].kind == 'm':
                d.loc[n//2, col] = pd.to_datetime('NaT')
            elif d.dtypes[col].kind == 'f':
                d.loc[n//2, col] = np.nan
            elif d.dtypes[col].kind in ['i', 'u']:
                continue
            else:
                d.loc[n//2, col] = None
            with measure('%s: write, with null, has_null=True' % d.dtypes[col], result):
                write(fn, df, has_nulls=True)

            pf = ParquetFile(fn)
            pf.to_pandas(categories={'w': 3})  # warm-up

            with measure('%s: read, with null, has_null=True' % d.dtypes[col], result):
                pf.to_pandas(categories={'w': 3})

            with measure('%s: write, with null, has_null=False' % d.dtypes[col], result):
                write(fn, df, has_nulls=False)

            pf = ParquetFile(fn)
            pf.to_pandas(categories={'w': 3})  # warm-up

            with measure('%s: read, with null, has_null=False' % d.dtypes[col], result):
                pf.to_pandas(categories={'w': 3})

        return result


def time_text():
    with tmpdir() as tempdir:
        result = {}
        fn = os.path.join(tempdir, 'temp.parq')
        n = 1000000
        d = pd.DataFrame({
            'a': np.random.choice(['hi', 'you', 'people'], size=n),
            'b': np.random.choice([b'hi', b'you', b'people'], size=n)})

        for col in d.columns:
            for fixed in [None, 6]:
                df = d[[col]]
                if isinstance(df.iloc[0, 0], bytes):
                    t = "bytes"
                else:
                    t = 'utf8'
                write(fn, df)
                with measure('%s: write, fixed: %s' % (t, fixed), result):
                    write(fn, df, has_nulls=False, write_index=False,
                          fixed_text={col: fixed}, object_encoding=t)

                pf = ParquetFile(fn)
                pf.to_pandas()  # warm-up

                with measure('%s: read, fixed: %s' % (t, fixed), result):
                    pf.to_pandas()
        return result


if __name__ == '__main__':
    result = {}
    for f in [time_column, time_text]:
        result.update(f())
    for k in sorted(result):
        print(k, result[k])


def time_find_nulls(N=10000000):
    x = np.random.random(N)
    df = pd.DataFrame({'x': x})
    result = {}
    run_find_nulls(df, result)
    df.loc[N//2, 'x'] = np.nan
    run_find_nulls(df, result)
    df.loc[:, 'x'] = np.nan
    df.loc[N//2, 'x'] = np.random.random()
    run_find_nulls(df, result)
    df.loc[N//2, 'x'] = np.nan
    run_find_nulls(df, result)

    x = np.random.randint(0, 2**30, N)
    df = pd.DataFrame({'x': x})
    run_find_nulls(df, result)

    df = pd.DataFrame({'x': x.view('datetime64[s]')})
    run_find_nulls(df, result)
    v = df.loc[N//2, 'x']
    df.loc[N//2, 'x'] = pd.to_datetime('NaT')
    run_find_nulls(df, result)
    df.loc[:, 'x'] = pd.to_datetime('NaT')
    df.loc[N//2, 'x'] = v
    run_find_nulls(df, result)
    df.loc[:, 'x'] = pd.to_datetime('NaT')
    run_find_nulls(df, result)

    [(k + (v, )) for k, v in result.items()]
    df = pd.DataFrame(out, columns=('type', 'nvalid', 'op', 'time'))
    df.groupby(('type', 'nvalid', 'op')).sum()
    return df


def run_find_nulls(df, res):
    nvalid = (df.x == df.x).sum()
    with measure((df.x.dtype.kind, nvalid, 'notnull'), res):
        df.x.notnull()
    with measure((df.x.dtype.kind, nvalid, 'notnull,sum'), res):
        df.x.notnull().sum()
    with measure((df.x.dtype.kind, nvalid, 'notnull,any'), res):
        df.x.notnull().any()
    with measure((df.x.dtype.kind, nvalid, 'notnull,all'), res):
        df.x.notnull().all()
    with measure((df.x.dtype.kind, nvalid, 'count'), res):
        df.x.count()

