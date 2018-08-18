"""parquet - read parquet files."""
from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals

from collections import OrderedDict
import json
import os
import re
import six
import struct

import numpy as np

from .core import read_thrift
from .thrift_structures import parquet_thrift
from . import core, schema, converted_types, encoding, dataframe
from .util import (default_open, ParquetException, val_to_num,
                   ensure_bytes, check_column_names, metadata_from_many,
                   ex_from_sep, get_file_scheme)


class ParquetFile(object):
    """The metadata of a parquet file or collection

    Reads the metadata (row-groups and schema definition) and provides
    methods to extract the data from the files.

    Parameters
    ----------
    fn: path/URL string or list of paths
        Location of the data. If a directory, will attempt to read a file
        "_metadata" within that directory. If a list of paths, will assume
        that they make up a single parquet data set.
    verify: bool [False]
        test file start/end byte markers
    open_with: function
        With the signature `func(path, mode)`, returns a context which
        evaluated to a file open for reading. Defaults to the built-in `open`.
    sep: string [`os.sep`]
        Path separator to use, if data is in multiple files.
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
        too; 'empty': no row grops at all.
    info: dict
        Combination of some of the other attributes
    key_value_metadata: list
        Additional information about this data's origin, e.g., pandas
        description.
    row_groups: list
        Thrift objects for each row group
    schema: schema.SchemaHelper
        print this for a representation of the column structure
    self_made: bool
        If this file was created by fastparquet
    statistics: dict
        Max/min/count of each column chunk
    """
    def __init__(self, fn, verify=False, open_with=default_open,
                 sep=os.sep, root=False):
        self.sep = sep
        if isinstance(fn, (tuple, list)):
            basepath, fmd = metadata_from_many(fn, verify_schema=verify,
                                               open_with=open_with, root=root)
            if basepath:
                self.fn = sep.join([basepath, '_metadata'])  # effective file
            else:
                self.fn = '_metadata'
            self.fmd = fmd
            self._set_attrs()
        else:
            try:
                fn2 = sep.join([fn, '_metadata'])
                self.fn = fn2
                with open_with(fn2, 'rb') as f:
                    self._parse_header(f, verify)
                fn = fn2
            except (IOError, OSError):
                self.fn = fn
                with open_with(fn, 'rb') as f:
                    self._parse_header(f, verify)
        self.open = open_with

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
        self.group_files = {}
        for i, rg in enumerate(self.row_groups):
            for chunk in rg.columns:
                self.group_files.setdefault(i, set()).add(chunk.file_path)
        self.schema = schema.SchemaHelper(self._schema)
        self.selfmade = self.created_by.split(' ', 1)[0] == "fastparquet-python"
        self.file_scheme = get_file_scheme([rg.columns[0].file_path
                                           for rg in self.row_groups], self.sep)
        self._read_partitions()
        self._dtypes()

    @ property
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
        return statistics(self)

    def _read_partitions(self):
        if self.file_scheme in ['simple', 'flat', 'other']:
            self.cats = {}
            return
        cats = OrderedDict()
        for rg in self.row_groups:
            for col in rg.columns:
                s = ex_from_sep(self.sep)
                path = col.file_path or ""
                if self.file_scheme == 'hive':
                    partitions = s.findall(path)
                    for key, val in partitions:
                        cats.setdefault(key, set()).add(val)
                else:
                    for i, val in enumerate(col.file_path.split(self.sep)[:-1]):
                        key = 'dir%i' % i
                        cats.setdefault(key, set()).add(val)
        self.cats = OrderedDict([(key, list([val_to_num(x) for x in v]))
                                for key, v in cats.items()])

    def row_group_filename(self, rg):
        if rg.columns[0].file_path:
            base = self.fn.replace('_metadata', '').rstrip(self.sep)
            if base:
                return self.sep.join([base, rg.columns[0].file_path])
            else:
                return rg.columns[0].file_path
        else:
            return self.fn

    def read_row_group_file(self, rg, columns, categories, index=None,
                            assign=None):
        """ Open file for reading, and process it as a row-group """
        if categories is None:
            categories = self.categories
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
        if categories is None:
            categories = self.categories
        ret = False
        if assign is None:
            df, assign = self.pre_allocate(rg.num_rows, columns,
                                           categories, index)
            ret = True
        core.read_row_group(
                infile, rg, columns, categories, self.schema, self.cats,
                self.selfmade, index=index, assign=assign, sep=self.sep,
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
                not(filter_out_cats(rg, filters, self.sep))]

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
        index: string or None
            Column to assign to the index. If None, index is inferred from the
            metadata (if this was originally pandas data); if the metadata does
            not exist or index is False, index is simple sequential integers.
        assign: dict {cols: array}
            Pre-allocated memory to write to. If None, will allocate memory
            here.

        Returns
        -------
        Generator yielding one Pandas data-frame per row-group
        """
        if index is None:
            index = self._get_index(index)
        columns = columns or self.columns
        if index and index not in columns:
            columns.append(index)
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
            index = json.loads(self.key_value_metadata.get('pandas', '{}')).get(
                'index_columns', [])
            if len(index) > 1:
                raise NotImplementedError('multi-index not yet supported, '
                                          'use index=False')
            if index:
                return index[0]
            else:
                return None
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
            will automatically set *if* the data was written by fastparquet.
        filters: list of tuples
            To filter out (i.e., not read) some of the row-groups.
            (This is not row-level filtering)
            Filter syntax: [(column, op, val), ...],
            where op is [==, >, >=, <, <=, !=, in, not in]
        index: string or None
            Column to assign to the index. If None, index is inferred from the
            metadata (if this was originally pandas data); if the metadata does
            not exist or index is False, index is simple sequential integers.

        Returns
        -------
        Pandas data-frame
        """
        rgs = self.filter_row_groups(filters)
        size = sum(rg.num_rows for rg in rgs)
        if index is None:
            index = self._get_index(index)
        columns = columns or self.columns
        if index and index not in columns:
            columns.append(index)
        check_column_names(self.columns, columns, categories)
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
        if categories is None:
            categories = self.categories
        return _pre_allocate(size, columns, categories, index, self.cats,
                             self._dtypes(categories))

    @property
    def count(self):
        """ Total number of rows """
        return sum(rg.num_rows for rg in self.row_groups)

    @property
    def info(self):
        """ Some metadata details """
        return {'name': self.fn, 'columns': self.columns,
                'partitions': list(self.cats), 'rows': self.count}

    @property
    def categories(self):
        if self.fmd.key_value_metadata is None:
            return {}
        vals = self.key_value_metadata.get('pandas', None)
        if vals:
            metadata = json.loads(vals)
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
        if categories is None:
            categories = self.categories
        dtype = {name: (converted_types.typemap(f)
                          if f.num_children in [None, 0] else np.dtype("O"))
                 for name, f in self.schema.root.children.items()}
        for col, dt in dtype.copy().items():
            if dt.kind in ['i', 'b']:
                # int/bool columns that may have nulls become float columns
                num_nulls = 0
                for rg in self.row_groups:
                    chunks = [c for c in rg.columns
                              if '.'.join(c.meta_data.path_in_schema) == col]
                    for chunk in chunks:
                        if chunk.meta_data.statistics is None:
                            num_nulls = True
                            break
                        if chunk.meta_data.statistics.null_count is None:
                            num_nulls = True
                            break
                        num_nulls += chunk.meta_data.statistics.null_count
                if num_nulls:
                    if dtype[col].itemsize == 1:
                        dtype[col] = np.dtype('f2')
                    elif dtype[col].itemsize == 2:
                        dtype[col] = np.dtype('f4')
                    else:
                        dtype[col] = np.dtype('f8')
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


def _pre_allocate(size, columns, categories, index, cs, dt):
    cols = [c for c in columns if index != c]
    categories = categories or {}
    cats = cs.copy()
    if isinstance(categories, dict):
        cats.update(categories)

    def get_type(name):
        if name in categories:
            return 'category'
        return dt.get(name, None)

    dtypes = [get_type(c) for c in cols]
    index_type = get_type(index)
    cols.extend(cs)
    dtypes.extend(['category'] * len(cs))
    df, views = dataframe.empty(dtypes, size, cols=cols, index_name=index,
                                index_type=index_type, cats=cats)
    if index and re.match(r'__index_level_\d+__', index):
        df.index.name = None
    return df, views


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
                out = filter_val(op, val, vmin, vmax)
                if out is True:
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
            if se.converted_type is not None:
                for name in ['min', 'max']:
                    try:
                        d[name][column] = (
                            [None] if d[name][column] is None
                            or None in d[name][column]
                            else list(converted_types.convert(
                                np.array(d[name][column]), se))
                        )
                    except (KeyError, ValueError):
                        # catch no stat and bad conversions
                        d[name][column] = [None]
        return d


def sorted_partitioned_columns(pf):
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


def filter_out_cats(rg, filters, sep='/'):
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
    s = ex_from_sep(sep)
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
            out = filter_val(op, val, v0, v0)
            if out is True:
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
    if (op == 'in' and vmax is not None and vmin is not None and
            vmax == vmin and vmax not in val):
        return True
    if vmax is not None:
        if isinstance(vmax, np.ndarray):
            vmax = vmax[0]
        if op in ['==', '>='] and val > vmax:
            return True
        if op == '>' and val >= vmax:
            return True
        if op == 'in' and min(val) > vmax:
            return True
    if vmin is not None:
        if isinstance(vmin, np.ndarray):
            vmin = vmin[0]
        if op in ['==', '<='] and val < vmin:
            return True
        if op == '<' and val <= vmin:
            return True
        if op == 'in' and max(val) < vmin:
            return True
    if (op == '!=' and vmax is not None and vmin is not None and
            vmax == vmin and val == vmax):
        return True
    if (op == 'not in' and vmax is not None and vmin is not None and
            vmax == vmin and vmax in val):
        return True

    # keep this row_group
    return False
