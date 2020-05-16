"""parquet - read parquet files."""
from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals

from collections import OrderedDict
import json
import re
import six
import struct
import warnings

import numpy as np
from fastparquet.util import join_path

from .core import read_thrift
from .thrift_structures import parquet_thrift
from . import core, schema, converted_types, encoding, dataframe
from .util import (default_open, ParquetException, val_to_num,
                   ensure_bytes, check_column_names, metadata_from_many,
                   ex_from_sep, get_file_scheme, STR_TYPE, groupby_types,
                   unique_everseen)


class ParquetFile(object):
    """The metadata of a parquet file or collection

    Reads the metadata (row-groups and schema definition) and provides
    methods to extract the data from the files.

    Note that when reading parquet files partitioned using directories
    (i.e. using the hive/drill scheme), an attempt is made to coerce
    the partition values to a number, datetime or timedelta. Fastparquet
    cannot read a hive/drill parquet file with partition names which coerce
    to the same value, such as "0.7" and ".7".

    Parameters
    ----------
    fn: path/URL string or list of paths
        Location of the data. If a directory, will attempt to read a file
        "_metadata" within that directory. If a list of paths, will assume
        that they make up a single parquet data set. This parameter can also
        be any file-like object, in which case this must be a single-file
        dataset.
    verify: bool [False]
        test file start/end byte markers
    open_with: function
        With the signature `func(path, mode)`, returns a context which
        evaluated to a file open for reading. Defaults to the built-in `open`.
    root: str
        If passing a list of files, the top directory of the data-set may
        be ambiguous for partitioning where the upmost field has only one
        value. Use this to specify the data'set root directory, if required.

    Attributes
    ----------
    cats: dict
        Columns derived from hive/drill directory information, with known
        values for each column.
    categories: list
        Columns marked as categorical in the extra metadata (meaning the
        data must have come from pandas).
    columns: list of str
        The data columns available
    count: int
        Total number of rows
    dtypes: dict
        Expected output types for each column
    file_scheme: str
        'simple': all row groups are within the same file; 'hive': all row
        groups are in other files; 'mixed': row groups in this file and others
        too; 'empty': no row groups at all.
    info: dict
        Combination of some of the other attributes
    key_value_metadata: list
        Additional information about this data's origin, e.g., pandas
        description.
    row_groups: list
        Thrift objects for each row group
    schema: schema.SchemaHelper
        print this for a representation of the column structure
    selfmade: bool
        If this file was created by fastparquet
    statistics: dict
        Max/min/count of each column chunk
    """
    def __init__(self, fn, verify=False, open_with=default_open,
                 root=False, sep=None):
        if isinstance(fn, (tuple, list)):
            basepath, fmd = metadata_from_many(fn, verify_schema=verify,
                                               open_with=open_with, root=root)
            if basepath:
                self.fn = join_path(basepath, '_metadata')  # effective file
            else:
                self.fn = '_metadata'
            self.fmd = fmd
            self._set_attrs()
        elif hasattr(fn, 'read'):
            # file-like
            self._parse_header(fn, verify)
            if self.file_scheme not in ['simple', 'empty']:
                raise ValueError('Cannot use file-like input '
                                 'with multi-file data')
            open_with = lambda *args, **kwargs: fn
            self.fn = None
        else:
            try:
                fn2 = join_path(fn, '_metadata')
                self.fn = fn2
                with open_with(fn2, 'rb') as f:
                    self._parse_header(f, verify)
                fn = fn2
            except (IOError, OSError):
                self.fn = join_path(fn)
                with open_with(fn, 'rb') as f:
                    self._parse_header(f, verify)
        self.open = open_with
        self.sep = sep
        self._statistics = None

    def _parse_header(self, f, verify=True):
        try:
            f.seek(0)
            if verify:
                assert f.read(4) == b'PAR1'
            f.seek(-8, 2)
            head_size = struct.unpack('<i', f.read(4))[0]
            if verify:
                assert f.read() == b'PAR1'
        except (AssertionError, struct.error):
            raise ParquetException('File parse failed: %s' % self.fn)

        f.seek(-(head_size+8), 2)
        try:
            fmd = read_thrift(f, parquet_thrift.FileMetaData)
        except Exception:
            raise ParquetException('Metadata parse failed: %s' %
                                   self.fn)
        self.head_size = head_size
        self.fmd = fmd
        self._set_attrs()

    def _set_attrs(self):
        fmd = self.fmd
        self.version = fmd.version
        self._schema = fmd.schema
        self.row_groups = fmd.row_groups or []
        self.key_value_metadata = {k.key: k.value
                                   for k in fmd.key_value_metadata or []}
        self.created_by = fmd.created_by
        self.schema = schema.SchemaHelper(self._schema)
        self.selfmade = self.created_by.split(' ', 1)[0] == "fastparquet-python" if self.created_by is not None else False
        files = [rg.columns[0].file_path
                 for rg in self.row_groups
                 if rg.columns]
        self.file_scheme = get_file_scheme(files)
        self._read_partitions()
        self._dtypes()

    @property
    def helper(self):
        return self.schema

    @property
    def columns(self):
        """ Column names """
        return [c for c, i in self._schema[0].children.items()
                if len(getattr(i, 'children', [])) == 0
                or i.converted_type in [parquet_thrift.ConvertedType.LIST,
                                        parquet_thrift.ConvertedType.MAP]]

    @property
    def statistics(self):
        if self._statistics is None:
            self._statistics = statistics(self)
        return self._statistics

    def _read_partitions(self):
        paths = (
            col.file_path or "" 
            for rg in self.row_groups
            for col in rg.columns
        )
        self.cats = paths_to_cats(paths, self.file_scheme)

    def row_group_filename(self, rg):
        if rg.columns and rg.columns[0].file_path:
            base = re.sub(r'_metadata(/)?$', '', self.fn).rstrip('/')
            if base:
                return join_path(base, rg.columns[0].file_path)
            else:
                return rg.columns[0].file_path
        else:
            return self.fn

    def read_row_group_file(self, rg, columns, categories, index=None,
                            assign=None):
        """ Open file for reading, and process it as a row-group """
        categories = self.check_categories(categories)
        fn = self.row_group_filename(rg)
        ret = False
        if assign is None:
            df, assign = self.pre_allocate(
                    rg.num_rows, columns, categories, index)
            ret = True
        core.read_row_group_file(
                fn, rg, columns, categories, self.schema, self.cats,
                open=self.open, selfmade=self.selfmade, index=index,
                assign=assign, scheme=self.file_scheme)
        if ret:
            return df

    def read_row_group(self, rg, columns, categories, infile=None,
                       index=None, assign=None):
        """
        Access row-group in a file and read some columns into a data-frame.
        """
        categories = self.check_categories(categories)
        ret = False
        if assign is None:
            df, assign = self.pre_allocate(rg.num_rows, columns,
                                           categories, index)
            ret = True
        core.read_row_group(
                infile, rg, columns, categories, self.schema, self.cats,
                self.selfmade, index=index, assign=assign,
                scheme=self.file_scheme)
        if ret:
            return df

    def grab_cats(self, columns, row_group_index=0):
        """ Read dictionaries of first row_group

        Used to correctly create metadata for categorical dask dataframes.
        Could be used to check that the same dictionary is used throughout
        the data.

        Parameters
        ----------
        columns: list
            Column names to load
        row_group_index: int (0)
            Row group to load from

        Returns
        -------
        {column: [list, of, values]}
        """
        if len(columns) == 0:
            return {}
        rg = self.row_groups[row_group_index]
        ofname = self.row_group_filename(rg)
        out = {}

        with self.open(ofname, 'rb') as f:
            for column in rg.columns:
                name = ".".join(column.meta_data.path_in_schema)
                if name not in columns:
                    continue
                out[name] = core.read_col(column, self.schema, f,
                                          grab_dict=True)
        return out

    def filter_row_groups(self, filters):
        """
        Select row groups using set of filters

        Parameters
        ----------
        filters: list of tuples
            See ``filter_out_cats`` and ``filter_out_stats``

        Returns
        -------
        Filtered list of row groups
        """
        return [rg for rg in self.row_groups if
                not(filter_out_stats(rg, filters, self.schema)) and
                not(filter_out_cats(rg, filters))]

    def iter_row_groups(self, columns=None, categories=None, filters=[],
                        index=None):
        """
        Read data from parquet into a Pandas dataframe.

        Parameters
        ----------
        columns: list of names or `None`
            Column to load (see `ParquetFile.columns`). Any columns in the
            data not in this list will be ignored. If `None`, read all columns.
        categories: list, dict or `None`
            If a column is encoded using dictionary encoding in every row-group
            and its name is also in this list, it will generate a Pandas
            Category-type column, potentially saving memory and time. If a
            dict {col: int}, the value indicates the number of categories,
            so that the optimal data-dtype can be allocated. If ``None``,
            will automatically set *if* the data was written by fastparquet.
        filters: list of tuples
            To filter out (i.e., not read) some of the row-groups.
            (This is not row-level filtering)
            Filter syntax: [(column, op, val), ...],
            where op is [==, >, >=, <, <=, !=, in, not in]
        index: string or list of strings or False or None
            Column(s) to assign to the (multi-)index. If None, index is
            inferred from the metadata (if this was originally pandas data); if
            the metadata does not exist or index is False, index is simple
            sequential integers.
        assign: dict {cols: array}
            Pre-allocated memory to write to. If None, will allocate memory
            here.

        Returns
        -------
        Generator yielding one Pandas data-frame per row-group
        """
        index = self._get_index(index)
        columns = columns or self.columns
        if index:
            columns += [i for i in index if i not in columns]
        check_column_names(self.columns, columns, categories)
        rgs = self.filter_row_groups(filters)
        if all(column.file_path is None for rg in self.row_groups
               for column in rg.columns):
            with self.open(self.fn, 'rb') as f:
                for rg in rgs:
                    df, views = self.pre_allocate(rg.num_rows, columns,
                                                  categories, index)
                    self.read_row_group(rg, columns, categories, infile=f,
                                        index=index, assign=views)
                    yield df
        else:
            for rg in rgs:
                df, views = self.pre_allocate(rg.num_rows, columns,
                                              categories, index)
                self.read_row_group_file(rg, columns, categories, index,
                                         assign=views)
                yield df

    def _get_index(self, index=None):
        if index is None:
            index = [i for i in self.pandas_metadata.get('index_columns', [])
                     if isinstance(i, six.text_type)]
        if isinstance(index, STR_TYPE):
            index = [index]
        return index

    def to_pandas(self, columns=None, categories=None, filters=[],
                  index=None):
        """
        Read data from parquet into a Pandas dataframe.

        Parameters
        ----------
        columns: list of names or `None`
            Column to load (see `ParquetFile.columns`). Any columns in the
            data not in this list will be ignored. If `None`, read all columns.
        categories: list, dict or `None`
            If a column is encoded using dictionary encoding in every row-group
            and its name is also in this list, it will generate a Pandas
            Category-type column, potentially saving memory and time. If a
            dict {col: int}, the value indicates the number of categories,
            so that the optimal data-dtype can be allocated. If ``None``,
            will automatically set *if* the data was written from pandas.
        filters: list of tuples
            To filter out (i.e., not read) some of the row-groups.
            (This is not row-level filtering)
            Filter syntax: [(column, op, val), ...],
            where op is [==, >, >=, <, <=, !=, in, not in]
        index: string or list of strings or False or None
            Column(s) to assign to the (multi-)index. If None, index is
            inferred from the metadata (if this was originally pandas data); if
            the metadata does not exist or index is False, index is simple
            sequential integers.

        Returns
        -------
        Pandas data-frame
        """
        rgs = self.filter_row_groups(filters)
        size = sum(rg.num_rows for rg in rgs)
        index = self._get_index(index)
        if columns is not None:
            columns = columns[:]
        else:
            columns = self.columns
        if index:
            columns += [i for i in index if i not in columns]
        check_column_names(self.columns + list(self.cats), columns, categories)
        df, views = self.pre_allocate(size, columns, categories, index)
        start = 0
        if self.file_scheme == 'simple':
            with self.open(self.fn, 'rb') as f:
                for rg in rgs:
                    parts = {name: (v if name.endswith('-catdef')
                                    else v[start:start + rg.num_rows])
                             for (name, v) in views.items()}
                    self.read_row_group(rg, columns, categories, infile=f,
                                        index=index, assign=parts)
                    start += rg.num_rows
        else:
            for rg in rgs:
                parts = {name: (v if name.endswith('-catdef')
                                else v[start:start + rg.num_rows])
                         for (name, v) in views.items()}
                self.read_row_group_file(rg, columns, categories, index,
                                         assign=parts)
                start += rg.num_rows
        return df

    def pre_allocate(self, size, columns, categories, index):
        categories = self.check_categories(categories)
        df, arrs = _pre_allocate(size, columns, categories, index, self.cats,
                                 self._dtypes(categories), self.tz)
        i_no_name = re.compile(r"__index_level_\d+__")
        if self.has_pandas_metadata:
            md = self.pandas_metadata
            if md.get('column_indexes', False):
                names = [(c['name'] if isinstance(c, dict) else c)
                         for c in md['column_indexes']]
                names = [None if n is None or i_no_name.match(n) else n
                         for n in names]
                df.columns.names = names
            if md.get('index_columns', False) and not (index or index is False):
                if len(md['index_columns']) == 1:
                    ic = md['index_columns'][0]
                    if isinstance(ic, dict) and ic['kind'] == 'range':
                        from pandas import RangeIndex
                        df.index = RangeIndex(
                            start=ic['start'],
                            stop=ic['start'] + size * ic['step'] + 1,
                            step=ic['step']
                        )[:size]
                names = [(c['name'] if isinstance(c, dict) else c)
                         for c in md['index_columns']]
                names = [None if n is None or i_no_name.match(n) else n
                         for n in names]
                df.index.names = names
        return df, arrs

    @property
    def count(self):
        """ Total number of rows """
        return sum(rg.num_rows for rg in self.row_groups)

    @property
    def info(self):
        """ Some metadata details """
        return {'name': self.fn, 'columns': self.columns,
                'partitions': list(self.cats), 'rows': self.count}

    def check_categories(self, cats):
        categ = self.categories
        if not self.has_pandas_metadata:
            return cats or {}
        if cats is None:
            return categ or {}
        if any(c not in categ for c in cats):
            raise TypeError('Attempt to load column as categorical that was'
                            ' not categorical in the original pandas data')
        return cats

    @property
    def has_pandas_metadata(self):
        if self.fmd.key_value_metadata is None:
            return False
        return bool(self.key_value_metadata.get('pandas', False))

    @property
    def pandas_metadata(self):
        if self.has_pandas_metadata:
            return json.loads(self.key_value_metadata['pandas'])
        else:
            return {}

    @property
    def categories(self):
        if self.has_pandas_metadata:
            metadata = self.pandas_metadata
            cats = {m['name']: m['metadata']['num_categories'] for m in
                    metadata['columns'] if m['pandas_type'] == 'categorical'}
            return cats
        # old track
        vals = self.key_value_metadata.get("fastparquet.cats", None)
        if vals:
            return json.loads(vals)
        else:
            return {}

    def _dtypes(self, categories=None):
        """ Implied types of the columns in the schema """
        import pandas as pd
        if self.has_pandas_metadata:
            md = self.pandas_metadata['columns']
            tz = {c['name']: c['metadata']['timezone'] for c in md
                  if (c.get('metadata', {}) or {}).get('timezone', None)}
        else:
            tz = None
        self.tz = tz
        categories = self.check_categories(categories)
        dtype = OrderedDict((name, (converted_types.typemap(f)
                            if f.num_children in [None, 0] else np.dtype("O")))
                            for name, f in self.schema.root.children.items()
                            if getattr(f, 'isflat', False) is False)
        for i, (col, dt) in enumerate(dtype.copy().items()):
            if dt.kind in ['i', 'b']:
                # int/bool columns that may have nulls become float columns
                num_nulls = 0
                for rg in self.row_groups:
                    chunk = rg.columns[i]
                    if chunk.meta_data.statistics is None:
                        num_nulls = True
                        break
                    if chunk.meta_data.statistics.null_count is None:
                        num_nulls = True
                        break
                    if chunk.meta_data.statistics.null_count:
                        num_nulls = True
                        break
                if num_nulls:
                    if dtype[col].itemsize == 1:
                        dtype[col] = np.dtype('f2')
                    elif dtype[col].itemsize == 2:
                        dtype[col] = np.dtype('f4')
                    else:
                        dtype[col] = np.dtype('f8')
            elif dt.kind == "M":
                if tz is not None and tz.get(col, False):
                    dtype[col] = pd.Series([], dtype='M8[ns]'
                                           ).dt.tz_localize(tz[col]).dtype
            elif dt == 'S12':
                dtype[col] = 'M8[ns]'
        for field in categories:
            dtype[field] = 'category'
        for cat in self.cats:
            dtype[cat] = "category"
        self.dtypes = dtype
        return dtype

    def __str__(self):
        return "<Parquet File: %s>" % self.info

    __repr__ = __str__


def _pre_allocate(size, columns, categories, index, cs, dt, tz=None):
    index = [index] if isinstance(index, six.text_type) else (index or [])
    cols = [c for c in columns if c not in index]
    categories = categories or {}
    cats = cs.copy()
    if isinstance(categories, dict):
        cats.update(categories)

    def get_type(name):
        if name in categories:
            return 'category'
        return dt.get(name, None)

    dtypes = [get_type(c) for c in cols]
    index_types = [get_type(i) for i in index]
    cols.extend(cs)
    dtypes.extend(['category'] * len(cs))
    df, views = dataframe.empty(dtypes, size, cols=cols, index_names=index,
                                index_types=index_types, cats=cats, timezones=tz)
    return df, views


def paths_to_cats(paths, file_scheme):
    """
    Extract categorical fields and labels from hive- or drill-style paths.

    Parameters
    ----------
    paths (Iterable[str]): file paths relative to root
    file_scheme (str): 

    Returns
    -------
    cats (OrderedDict[str, List[Any]]): a dict of field names and their values
    """
    if file_scheme in ['simple', 'flat', 'other']:
        cats = {}
        return cats

    cats = OrderedDict()
    raw_cats = OrderedDict()
    s = ex_from_sep('/')
    paths = unique_everseen(paths)
    if file_scheme == 'hive':
        partitions = unique_everseen(
            (k, v)
            for path in paths
            for k, v in s.findall(path)
        )
        for key, val in partitions:
            cats.setdefault(key, set()).add(val_to_num(val))
            raw_cats.setdefault(key, set()).add(val)
    else:
        i_val = unique_everseen(
            (i, val)
            for path in paths
            for i, val in enumerate(path.split('/')[:-1])
        )
        for i, val in i_val:
            key = 'dir%i' % i
            cats.setdefault(key, set()).add(val_to_num(val))
            raw_cats.setdefault(key, set()).add(val)

    for key, v in cats.items():
        # Check that no partition names map to the same value after transformation by val_to_num
        raw = raw_cats[key]
        if len(v) != len(raw):
            conflicts_by_value = OrderedDict()
            for raw_val in raw_cats[key]:
                conflicts_by_value.setdefault(val_to_num(raw_val), set()).add(raw_val)
            conflicts = [c for k in conflicts_by_value.values() if len(k) > 1 for c in k]
            raise ValueError("Partition names map to the same value: %s" % conflicts)
        vals_by_type = groupby_types(v)

        # Check that all partition names map to the same type after transformation by val_to_num
        if len(vals_by_type) > 1:
            examples = [x[0] for x in vals_by_type.values()]
            warnings.warn("Partition names coerce to values of different types, e.g. %s" % examples)

    cats = OrderedDict([(key, list(v)) for key, v in cats.items()])
    return cats


def filter_out_stats(rg, filters, schema):
    """
    According to the filters, should this row-group be excluded

    Considers the statistics included in the metadata of this row-group

    Parameters
    ----------
    rg: thrift RowGroup structure
    filters: list of 3-tuples
        Structure of each tuple: (column, op, value) where op is one of
        ['==', '!=', '<', '<=', '>', '>=', 'in', 'not in'] and value is
        appropriate for the column in question

    Returns
    -------
    True or False
    """
    if rg.num_rows == 0:
        # always ignore empty row-groups, don't bother loading
        return True
    if len(filters) == 0:
        return False
    for column in rg.columns:
        vmax, vmin = None, None
        name = ".".join(column.meta_data.path_in_schema)
        app_filters = [f[1:] for f in filters if f[0] == name]
        for op, val in app_filters:
            se = schema.schema_element(name)
            if column.meta_data.statistics is not None:
                s = column.meta_data.statistics
                if s.max is not None:
                    b = ensure_bytes(s.max)
                    vmax = encoding.read_plain(b, column.meta_data.type, 1)
                    if se.converted_type is not None:
                        vmax = converted_types.convert(vmax, se)
                if s.min is not None:
                    b = ensure_bytes(s.min)
                    vmin = encoding.read_plain(b, column.meta_data.type, 1)
                    if se.converted_type is not None:
                        vmin = converted_types.convert(vmin, se)
                if filter_val(op, val, vmin, vmax):
                    return True
    return False


def statistics(obj):
    """
    Return per-column statistics for a ParquerFile

    Parameters
    ----------
    obj: ParquetFile

    Returns
    -------
    dictionary mapping stats (min, max, distinct_count, null_count) to column
    names to lists of values.  ``None``s used if no statistics found.

    Examples
    --------
    >>> statistics(my_parquet_file)
    {'min': {'x': [1, 4], 'y': [5, 3]},
     'max': {'x': [2, 6], 'y': [8, 6]},
     'distinct_count': {'x': [None, None], 'y': [None, None]},
     'null_count': {'x': [0, 3], 'y': [0, 0]}}
    """
    if isinstance(obj, parquet_thrift.ColumnChunk):
        md = obj.meta_data
        s = obj.meta_data.statistics
        rv = {}
        if not s:
            return rv
        if s.max is not None:
            try:
                if md.type == parquet_thrift.Type.BYTE_ARRAY:
                    rv['max'] = ensure_bytes(s.max)
                else:
                    rv['max'] = encoding.read_plain(ensure_bytes(s.max),
                                                    md.type, 1)[0]
            except:
                rv['max'] = None
        if s.min is not None:
            try:
                if md.type == parquet_thrift.Type.BYTE_ARRAY:
                    rv['min'] = ensure_bytes(s.min)
                else:
                    rv['min'] = encoding.read_plain(ensure_bytes(s.min),
                                                    md.type, 1)[0]
            except:
                rv['min'] = None
        if s.null_count is not None:
            rv['null_count'] = s.null_count
        if s.distinct_count is not None:
            rv['distinct_count'] = s.distinct_count
        return rv

    if isinstance(obj, parquet_thrift.RowGroup):
        return {'.'.join(c.meta_data.path_in_schema): statistics(c)
                for c in obj.columns}

    if isinstance(obj, ParquetFile):
        L = list(map(statistics, obj.row_groups))
        d = {n: {col: [item.get(col, {}).get(n, None) for item in L]
                 for col in obj.columns}
             for n in ['min', 'max', 'null_count', 'distinct_count']}
        if not L:
            return d
        schema = obj.schema
        for col in obj.row_groups[0].columns:
            column = '.'.join(col.meta_data.path_in_schema)
            se = schema.schema_element(col.meta_data.path_in_schema)
            if (se.converted_type is not None
                    or se.type == parquet_thrift.Type.INT96):
                dtype = 'S12' if se.type == parquet_thrift.Type.INT96 else None
                for name in ['min', 'max']:
                    try:
                        d[name][column] = (
                            [None] if d[name][column] is None
                            or None in d[name][column]
                            else list(converted_types.convert(
                                np.array(d[name][column], dtype), se))
                        )
                    except (KeyError, ValueError):
                        # catch no stat and bad conversions
                        d[name][column] = [None]
        return d


def sorted_partitioned_columns(pf, filters=None):
    """
    The columns that are known to be sorted partition-by-partition

    They may not be sorted within each partition, but all elements in one
    row group are strictly greater than all elements in previous row groups.

    Examples
    --------
    >>> sorted_partitioned_columns(pf)
    {'id': {'min': [1, 5, 10], 'max': [4, 9, 20]}}

    Returns
    -------
    A set of column names

    See Also
    --------
    statistics
    """
    s = statistics(pf)
    if (filters is not None) & (filters != []):
        idx_list = [i for i, rg in enumerate(pf.row_groups) if
                    not(filter_out_stats(rg, filters, pf.schema)) and
                    not(filter_out_cats(rg, filters))]
        for stat in s.keys():
            for col in s[stat].keys():
                s[stat][col] = [s[stat][col][i] for i in idx_list]
    columns = pf.columns
    out = dict()
    for c in columns:
        min, max = s['min'][c], s['max'][c]
        if any(x is None for x in min + max):
            continue
        try:
            if (sorted(min) == min and
                    sorted(max) == max and
                    all(mx < mn for mx, mn in zip(max[:-1], min[1:]))):
                out[c] = {'min': min, 'max': max}
        except TypeError:
            # because some types, e.g., dicts cannot be sorted/compared
            continue
    return out


def filter_out_cats(rg, filters):
    """
    According to the filters, should this row-group be excluded

    Considers the partitioning category applicable to this row-group

    Parameters
    ----------
    rg: thrift RowGroup structure
    filters: list of 3-tuples
        Structure of each tuple: (column, op, value) where op is one of
        ['==', '!=', '<', '<=', '>', '>=', 'in', 'not in'] and value is
        appropriate for the column in question

    Returns
    -------
    True or False
    """
    # TODO: fix for Drill
    if len(filters) == 0 or rg.columns[0].file_path is None:
        return False
    s = ex_from_sep('/')
    partitions = s.findall(rg.columns[0].file_path)
    pairs = [(p[0], p[1]) for p in partitions]
    for cat, v in pairs:

        app_filters = [f[1:] for f in filters if f[0] == cat]
        for op, val in app_filters:
            tstr = six.string_types + (six.text_type, )
            if isinstance(val, tstr) or (isinstance(val, (tuple, list)) and
                                         all(isinstance(x, tstr) for x in val)):
                v0 = v
            else:
                v0 = val_to_num(v)
            if filter_val(op, val, v0, v0):
                return True
    return False


def filter_val(op, val, vmin=None, vmax=None):
    """
    Perform value comparison for filtering

    op: ['==', '!=', '<', '<=', '>', '>=', 'in', 'not in']
    val: appropriate value
    vmin, vmax: the range to compare within

    Returns
    -------
    True or False
    """
    vmin = _handle_np_array(vmin)
    vmax = _handle_np_array(vmax)
    if op == 'in':
        return filter_in(val, vmin, vmax)
    if op == 'not in':
        return filter_not_in(val, vmin, vmax)
    if vmax is not None:
        if op in ['==', '>='] and val > vmax:
            return True
        if op == '>' and val >= vmax:
            return True
    if vmin is not None:
        if op in ['==', '<='] and val < vmin:
            return True
        if op == '<' and val <= vmin:
            return True
    if (op == '!=' and vmax is not None and vmin is not None and
            vmax == vmin and val == vmax):
        return True

    # keep this row_group
    return False


def _handle_np_array(v):
    if v is not None and isinstance(v, np.ndarray):
        v = v[0]
    return v


def filter_in(values, vmin=None, vmax=None):
    """
    Handles 'in' filters

    op: ['in', 'not in']
    values: iterable of values
    vmin, vmax: the range to compare within

    Returns
    -------
    True or False
    """
    if len(values) == 0:
        return True
    if vmax == vmin and vmax is not None and vmax not in values:
        return True
    if vmin is None and vmax is None:
        return False

    sorted_values = sorted(values)
    if vmin is None and vmax is not None:
        return sorted_values[0] > vmax
    elif vmax is None and vmin is not None:
        return sorted_values[-1] < vmin

    vmin_insert = np.searchsorted(sorted_values, vmin, side='left')
    vmax_insert = np.searchsorted(sorted_values, vmax, side='right')

    # if the indexes are equal, then there are no values within the range
    return vmin_insert == vmax_insert


def filter_not_in(values, vmin=None, vmax=None):
    """
    Handles 'not in' filters

    op: ['in', 'not in']
    values: iterable of values
    vmin, vmax: the range to compare within

    Returns
    -------
    True or False
    """
    if len(values) == 0:
        return False
    if vmax is not None and vmax in values:
        return True
    elif vmin is not None and vmin in values:
        return True
    else:
        return False
