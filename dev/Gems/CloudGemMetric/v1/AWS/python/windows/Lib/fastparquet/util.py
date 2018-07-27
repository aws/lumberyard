import ast
import os
import os.path
import pandas as pd
import re
import six

from .thrift_structures import thrift_copy
try:
    from pandas.api.types import is_categorical_dtype
except ImportError:
    # Pandas <= 0.18.1
    from pandas.core.common import is_categorical_dtype

PY2 = six.PY2
PY3 = six.PY3
STR_TYPE = six.string_types[0]  # 'str' for Python3, 'basestring' for Python2
created_by = "fastparquet-python version 1.0.0 (build 111)"


class ParquetException(Exception):
    """Generic Exception related to unexpected data format when
     reading parquet file."""
    pass


def sep_from_open(opener):
    if opener is default_open:
        return os.sep
    else:
        return '/'


if PY2:
    def default_mkdirs(f):
        if not os.path.exists(f):
            os.makedirs(f)
else:
    def default_mkdirs(f):
        os.makedirs(f, exist_ok=True)


def default_open(f, mode='rb'):
    return open(f, mode)


def val_to_num(x):
    if x in ['NOW', 'TODAY']:
        return x
    if set(x) == {'0'}:
        # special case for values like "000"
        return 0
    try:
        return ast.literal_eval(x.lstrip('0'))
    except:
        pass
    try:
        return pd.to_datetime(x)
    except:
        pass
    try:
        return pd.to_timedelta(x)
    except:
        return x


if PY2:
    def ensure_bytes(s):
        return s.encode('utf-8') if isinstance(s, unicode) else s
else:
    def ensure_bytes(s):
        return s.encode('utf-8') if isinstance(s, str) else s


def index_like(index):
    """
    Does index look like a default range index?
    """
    return not (isinstance(index, pd.RangeIndex) and
                index._start == 0 and
                index._stop == len(index) and
                index._step == 1 and index.name is None)


def check_column_names(columns, *args):
    """Ensure that parameters listing column names have corresponding columns"""
    for arg in args:
        if isinstance(arg, (tuple, list)):
            if set(arg) - set(columns):
                raise ValueError("Column name not in list.\n"
                                 "Requested %s\n"
                                 "Allowed %s" % (arg, columns))


def byte_buffer(raw_bytes):
    return buffer(raw_bytes) if PY2 else memoryview(raw_bytes)


def metadata_from_many(file_list, verify_schema=False, open_with=default_open,
                       root=False):
    """
    Given list of parquet files, make a FileMetaData that points to them

    Parameters
    ----------
    file_list: list of paths of parquet files
    verify_schema: bool (False)
        Whether to assert that the schemas in each file are identical
    open_with: function
        Use this to open each path.
    root: str
        Top of the dataset's directory tree, for cases where it can't be
        automatically inferred.

    Returns
    -------
    basepath: the root path that other paths are relative to
    fmd: metadata thrift structure
    """
    from fastparquet import api
    sep = sep_from_open(open_with)
    if all(isinstance(pf, api.ParquetFile) for pf in file_list):
        pfs = file_list
        file_list = [pf.fn for pf in pfs]
    elif all(not isinstance(pf, api.ParquetFile) for pf in file_list):
        pfs = [api.ParquetFile(fn, open_with=open_with) for fn in file_list]
    else:
        raise ValueError("Merge requires all PaquetFile instances or none")
    basepath, file_list = analyse_paths(file_list, sep, root=root)

    if verify_schema:
        for pf in pfs[1:]:
            if pf._schema != pfs[0]._schema:
                raise ValueError('Incompatible schemas')

    fmd = thrift_copy(pfs[0].fmd)  # we inherit "created by" field
    fmd.row_groups = []

    for pf, fn in zip(pfs, file_list):
        if pf.file_scheme not in ['simple', 'empty']:
            # should remove 'empty' datasets up front? Get ignored on load
            # anyway.
            raise ValueError('Cannot merge multi-file input', fn)
        for rg in pf.row_groups:
            rg = thrift_copy(rg)
            for chunk in rg.columns:
                chunk.file_path = fn
            fmd.row_groups.append(rg)

    fmd.num_rows = sum(rg.num_rows for rg in fmd.row_groups)
    return basepath, fmd

# simple cache to avoid re compile every time
seps = {}


def ex_from_sep(sep):
    """Generate regex for category folder matching"""
    if sep not in seps:
        if sep in r'\^$.|?*+()[]':
            s = re.compile(r"([a-zA-Z_0-9]+)=([^\{}]+)".format(sep))
        else:
            s = re.compile("([a-zA-Z_0-9]+)=([^{}]+)".format(sep))
        seps[sep] = s
    return seps[sep]


def analyse_paths(file_list, sep=os.sep, root=False):
    """Consolidate list of file-paths into  parquet relative paths"""
    path_parts_list = [fn.split(sep) for fn in file_list]
    if root is False:
        basepath = path_parts_list[0][:-1]
        for i, path_parts in enumerate(path_parts_list):
            j = len(path_parts) - 1
            for k, (base_part, path_part) in enumerate(
                    zip(basepath, path_parts)):
                if base_part != path_part:
                    j = k
                    break
            basepath = basepath[:j]
        l = len(basepath)

    else:
        basepath = root.rstrip(sep).split(sep)
        l = len(basepath)
        assert all(p[:l] == basepath for p in path_parts_list
                   ), "All paths must begin with the given root"
    l = len(basepath)
    out_list = []
    for path_parts in path_parts_list:
        out_list.append(sep.join(path_parts[l:]))

    return sep.join(basepath), out_list


def infer_dtype(column):
    try:
        return pd.api.types.infer_dtype(column)
    except AttributeError:
        return pd.lib.infer_dtype(column)


def get_column_metadata(column, name):
    """Produce pandas column metadata block"""
    # from pyarrow.pandas_compat
    # https://github.com/apache/arrow/blob/master/python/pyarrow/pandas_compat.py
    inferred_dtype = infer_dtype(column)
    dtype = column.dtype

    if is_categorical_dtype(dtype):
        extra_metadata = {
            'num_categories': len(column.cat.categories),
            'ordered': column.cat.ordered,
        }
        dtype = column.cat.codes.dtype
    elif hasattr(dtype, 'tz'):
        extra_metadata = {'timezone': str(dtype.tz)}
    else:
        extra_metadata = None

    if not isinstance(name, six.string_types):
        raise TypeError(
            'Column name must be a string. Got column {} of type {}'.format(
                name, type(name).__name__
            )
        )

    return {
        'name': name,
        'pandas_type': {
            'string': 'bytes' if PY2 else 'unicode',
            'datetime64': (
                'datetimetz' if hasattr(dtype, 'tz')
                else 'datetime'
            ),
            'integer': str(dtype),
            'floating': str(dtype),
        }.get(inferred_dtype, inferred_dtype),
        'numpy_type': get_numpy_type(dtype),
        'metadata': extra_metadata,
    }


def get_numpy_type(dtype):
    if is_categorical_dtype(dtype):
        return 'category'
    else:
        return str(dtype)


def get_file_scheme(paths, sep='/'):
    """For the given row groups, figure out if the partitioning scheme

    Parameters
    ----------
    paths: list of str
        normally from row_group.columns[0].file_path
    sep: str
        path separator such as '/'

    Returns
    -------
    'empty': no rgs at all
    'simple': all rgs in a single file
    'flat': multiple files in one directory
    'hive': directories are all `key=value`; all files are at the same
        directory depth
    'drill': assume directory names are labels, and field names are of the
        form dir0, dir1; all files are at the same directory depth
    'other': none of the above, assume no partitioning
    """
    if not paths:
        return 'empty'
    if set(paths) == {None}:
        return 'simple'
    if None in paths:
        return 'other'
    parts = [p.split(sep) for p in paths]
    lens = [len(p) for p in parts]
    if len(set(lens)) > 1:
        return 'other'
    if set(lens) == {1}:
        return 'flat'
    s = ex_from_sep(sep)
    dirs = [p.rsplit(sep, 1)[0] for p in paths]
    matches = [s.findall(d) for d in dirs]
    if all(len(m) == (l - 1) for (m, l) in
           zip(matches, lens)):
        keys = (tuple(m[0] for m in parts) for parts in matches)
        if len(set(keys)) == 1:
            return 'hive'
    return 'drill'
