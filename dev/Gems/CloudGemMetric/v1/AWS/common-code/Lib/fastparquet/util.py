import ast
import copy
import os
import os.path
import pandas as pd
import re
import six
import numbers
from collections import defaultdict
from distutils.version import LooseVersion
import itertools
import pandas

try:
    from pandas.api.types import is_categorical_dtype
except ImportError:
    # Pandas <= 0.18.1
    from pandas.core.common import is_categorical_dtype

PY2 = six.PY2
PY3 = six.PY3
PANDAS_VERSION = LooseVersion(pandas.__version__)
STR_TYPE = six.string_types[0]  # 'str' for Python3, 'basestring' for Python2
created_by = "fastparquet-python version 1.0.0 (build 111)"


class ParquetException(Exception):
    """Generic Exception related to unexpected data format when
     reading parquet file."""
    pass


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
    """Parse a string as a number, date or timedelta if possible"""
    if isinstance(x, numbers.Real):
        return x
    if x in ['now', 'NOW', 'TODAY', '']:
        return x
    if type(x) == str and x.lower() == 'nan':
        return x
    if x == "True":
        return True
    if x == "False":
        return False
    try:
        return int(x, base=10)
    except:
        pass
    try:
        return float(x)
    except:
        pass
    try:
        return pd.to_datetime(x)
    except:
        pass
    try:
        # TODO: determine the valid usecases for this, then try to limit the set
        #  ofstrings which may get inadvertently converted to timedeltas
        return pd.to_timedelta(x)
    except:
        return x

if PY2:
    def ensure_bytes(s):
        return s.encode('utf-8') if isinstance(s, unicode) else s
else:
    def ensure_bytes(s):
        return s.encode('utf-8') if isinstance(s, str) else s


def check_column_names(columns, *args):
    """Ensure that parameters listing column names have corresponding columns"""
    for arg in args:
        if isinstance(arg, (tuple, list)):
            missing = set(arg) - set(columns)
            if missing:
                raise ValueError("Following columns were requested but are "
                                 "not available: %s.\n"
                                 "All requested columns: %s\n"
                                 "Available columns: %s"
                                 "" % (missing, arg, columns))


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

    if all(isinstance(pf, api.ParquetFile) for pf in file_list):
        pfs = file_list
        file_list = [pf.fn for pf in pfs]
    elif all(not isinstance(pf, api.ParquetFile) for pf in file_list):
        pfs = [api.ParquetFile(fn, open_with=open_with) for fn in file_list]
    else:
        raise ValueError("Merge requires all PaquetFile instances or none")
    basepath, file_list = analyse_paths(file_list, root=root)

    if verify_schema:
        for pf in pfs[1:]:
            if pf._schema != pfs[0]._schema:
                raise ValueError('Incompatible schemas')

    fmd = copy.copy(pfs[0].fmd)  # we inherit "created by" field
    fmd.row_groups = []

    for pf, fn in zip(pfs, file_list):
        if pf.file_scheme not in ['simple', 'empty']:
            for rg in pf.row_groups:
                rg = copy.copy(rg)
                rg.columns = [copy.copy(c) for c in rg.columns]
                for chunk in rg.columns:
                    chunk.file_path = '/'.join([fn, chunk.file_path])
                fmd.row_groups.append(rg)

        else:
            for rg in pf.row_groups:
                rg = copy.copy(rg)
                rg.columns = [copy.copy(c) for c in rg.columns]
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


def analyse_paths(file_list, root=False):
    """Consolidate list of file-paths into  parquet relative paths"""
    path_parts_list = [join_path(fn).split('/') for fn in file_list]
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
        basepath = join_path(root).split('/')
        l = len(basepath)
        assert all(p[:l] == basepath for p in path_parts_list
                   ), "All paths must begin with the given root"
    l = len(basepath)
    out_list = []
    for path_parts in path_parts_list:
        out_list.append('/'.join(path_parts[l:]))  # use '/'.join() instead of join_path to be consistent with split('/')

    return '/'.join(basepath), out_list  # use '/'.join() instead of join_path to be consistent with split('/')


def infer_dtype(column):
    try:
        return pd.api.types.infer_dtype(column, skipna=False)
    except AttributeError:
        return pd.lib.infer_dtype(column)


def groupby_types(iterable):
    groups = defaultdict(list)
    for x in iterable:
        groups[type(x)].append(x)
    return groups


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
        try:
            pd.Series([pd.to_datetime('now')]).dt.tz_localize(str(dtype.tz))
            extra_metadata = {'timezone': str(dtype.tz)}
        except Exception:
            raise ValueError("Time-zone information could not be serialised: "
                             "%s, please use another" % str(dtype.tz))
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
        'field_name': name,
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


def get_file_scheme(paths):
    """For the given row groups, figure out if the partitioning scheme

    Parameters
    ----------
    paths: list of str
        normally from row_group.columns[0].file_path

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
    parts = [p.split('/') for p in paths]
    lens = [len(p) for p in parts]
    if len(set(lens)) > 1:
        return 'other'
    if set(lens) == {1}:
        return 'flat'
    s = ex_from_sep('/')
    dirs = [p.rsplit('/', 1)[0] for p in paths]
    matches = [s.findall(d) for d in dirs]
    if all(len(m) == (l - 1) for (m, l) in
           zip(matches, lens)):
        keys = (tuple(m[0] for m in parts) for parts in matches)
        if len(set(keys)) == 1:
            return 'hive'
    return 'drill'


def join_path(*path):
    def scrub(i, p):
        # Convert path to standard form
        # this means windows path separators are converted to linux
        p = p.replace(os.sep, "/")
        if p == "":  # empty path is assumed to be a relative path
            return "."
        if p[-1] == '/':  # trailing slashes are not allowed
            p = p[:-1]
        if i > 0 and p[0] == '/':  # only the first path can start with /
            p = p[1:]
        return p

    abs_prefix = ''
    if path and path[0]:
        if path[0][0] == '/':
            abs_prefix = '/'
            path = list(path)
            path[0] = path[0][1:]
        elif os.sep == '\\' and path[0][1:].startswith(':/'):
            # If windows, then look for the "c:/" prefix
            abs_prefix = path[0][0:3]
            path = list(path)
            path[0] = path[0][3:]

    scrubbed = []
    for i, p in enumerate(path):
        scrubbed.extend(scrub(i, p).split("/"))
    simpler = []
    for s in scrubbed:
        if s == ".":
            pass
        elif s == "..":
            if simpler:
                if simpler[-1] == '..':
                    simpler.append(s)
                else:
                    simpler.pop()
            elif abs_prefix:
                raise Exception("can not get parent of root")
            else:
                simpler.append(s)
        else:
            simpler.append(s)

    if not simpler:
        if abs_prefix:
            joined = abs_prefix
        else:
            joined = "."
    else:
        joined = abs_prefix + ('/'.join(simpler))
    return joined


if PY2:
    filterfalse = itertools.ifilterfalse
else:
    filterfalse = itertools.filterfalse


def unique_everseen(iterable, key=None):
    """List unique elements, preserving order. Remember all elements ever seen.

    unique_everseen('AAAABBBCCDAABBB') --> A B C D
    unique_everseen('ABBCcAD', str.lower) --> A B C D
    """
    
    seen = set()
    seen_add = seen.add
    if key is None:
        for element in filterfalse(seen.__contains__, iterable):
            seen_add(element)
            yield element
    else:
        for element in iterable:
            k = key(element)
            if k not in seen:
                seen_add(k)
                yield element
