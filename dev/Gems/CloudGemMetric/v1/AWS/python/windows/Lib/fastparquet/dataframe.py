import re
from collections import OrderedDict
from distutils.version import LooseVersion
import numpy as np
from pandas.core.internals import BlockManager
from pandas import (
    Categorical, DataFrame, Series,
    CategoricalIndex, RangeIndex, Index, MultiIndex,
    __version__ as pdver
)
from pandas.api.types import is_categorical_dtype
import six
import warnings
from .util import STR_TYPE


class Dummy(object):
    pass


def empty(types, size, cats=None, cols=None, index_types=None, index_names=None,
          timezones=None):
    """
    Create empty DataFrame to assign into

    In the simplest case, will return a Pandas dataframe of the given size,
    with columns of the given names and types. The second return value `views`
    is a dictionary of numpy arrays into which you can assign values that
    show up in the dataframe.

    For categorical columns, you get two views to assign into: if the
    column name is "col", you get both "col" (the category codes) and
    "col-catdef" (the category labels).

    For a single categorical index, you should use the `.set_categories`
    method of the appropriate "-catdef" columns, passing an Index of values

    ``views['index-catdef'].set_categories(pd.Index(newvalues), fastpath=True)``

    Multi-indexes work a lot like categoricals, even if the types of each
    index are not themselves categories, and will also have "-catdef" entries
    in the views. However, these will be Dummy instances, providing only a
    ``.set_categories`` method, to be used as above.

    Parameters
    ----------
    types: like np record structure, 'i4,u2,f4,f2,f4,M8,m8', or using tuples
        applies to non-categorical columns. If there are only categorical
        columns, an empty string of None will do.
    size: int
        Number of rows to allocate
    cats: dict {col: labels}
        Location and labels for categorical columns, e.g., {1: ['mary', 'mo]}
        will create column index 1 (inserted amongst the numerical columns)
        with two possible values. If labels is an integers, `{'col': 5}`,
        will generate temporary labels using range. If None, or column name
        is missing, will assume 16-bit integers (a reasonable default).
    cols: list of labels
        assigned column names, including categorical ones.
    index_types: list of str
        For one of more index columns, make them have this type. See general
        description, above, for caveats about multi-indexing. If None, the
        index will be the default RangeIndex.
    index_names: list of str
        Names of the index column(s), if using
    timezones: dict {col: timezone_str}
        for timestamp type columns, apply this timezone to the pandas series;
        the numpy view will be UTC.

    Returns
    -------
    - dataframe with correct shape and data-types
    - list of numpy views, in order, of the columns of the dataframe. Assign
        to this.
    """
    views = {}
    timezones = timezones or {}

    if isinstance(types, STR_TYPE):
        types = types.split(',')
    cols = cols if cols is not None else range(len(types))

    def cat(col):
        if cats is None or col not in cats:
            return RangeIndex(0, 2**14)
        elif isinstance(cats[col], int):
            return RangeIndex(0, cats[col])
        else:  # explicit labels list
            return cats[col]

    df = OrderedDict()
    for t, col in zip(types, cols):
        if str(t) == 'category':
            df[six.text_type(col)] = Categorical([], categories=cat(col),
                                                 fastpath=True)
        else:
            if hasattr(t, 'base'):
                # funky pandas not-dtype
                t = t.base
            d = np.empty(0, dtype=t)
            if d.dtype.kind == "M" and six.text_type(col) in timezones:
                try:
                    d = Series(d).dt.tz_localize(timezones[six.text_type(col)])
                except:
                    warnings.warn("Inferring time-zone from %s in column %s "
                                  "failed, using time-zone-agnostic"
                                  "" % (timezones[six.text_type(col)], col))
            df[six.text_type(col)] = d

    df = DataFrame(df)
    if not index_types:
        index = RangeIndex(size)
    elif len(index_types) == 1:
        t, col = index_types[0], index_names[0]
        if col is None:
            raise ValueError('If using an index, must give an index name')
        if str(t) == 'category':
            c = Categorical([], categories=cat(col), fastpath=True)
            vals = np.zeros(size, dtype=c.codes.dtype)
            index = CategoricalIndex(c)
            index._data._codes = vals
            views[col] = vals
            views[col+'-catdef'] = index._data
        else:
            if hasattr(t, 'base'):
                # funky pandas not-dtype
                t = t.base
            d = np.empty(size, dtype=t)
            if d.dtype.kind == "M" and six.text_type(col) in timezones:
                try:
                    d = Series(d).dt.tz_localize(timezones[six.text_type(col)])
                except:
                    warnings.warn("Inferring time-zone from %s in column %s "
                                  "failed, using time-zone-agnostic"
                                  "" % (timezones[six.text_type(col)], col))
            index = Index(d)
            views[col] = index.values
    else:
        index = MultiIndex([[]], [[]])
        # index = MultiIndex.from_arrays(indexes)
        index._levels = list()
        index._labels = list()
        index._codes = list()
        index._names = list(index_names)
        for i, col in enumerate(index_names):
            index._levels.append(Index([None]))

            def set_cats(values, i=i, col=col, **kwargs):
                values.name = col
                if index._levels[i][0] is None:
                    index._levels[i] = values
                elif not index._levels[i].equals(values):
                    raise RuntimeError("Different dictionaries encountered"
                                       " while building categorical")

            x = Dummy()
            x._set_categories = set_cats

            d = np.zeros(size, dtype=int)
            if LooseVersion(pdver) >= LooseVersion("0.24.0"):
                index._codes = list(index._codes) + [d]
            else:
                index._labels.append(d)
            views[col] = d
            views[col+'-catdef'] = x

    axes = [df._data.axes[0], index]

    # allocate and create blocks
    blocks = []
    for block in df._data.blocks:
        if block.is_categorical:
            categories = block.values.categories
            code = np.zeros(shape=size, dtype=block.values.codes.dtype)
            values = Categorical(values=code, categories=categories,
                                 fastpath=True)
            new_block = block.make_block_same_class(values=values)
        elif getattr(block.dtype, 'tz', None):
            new_shape = (size, )
            values = np.empty(shape=new_shape, dtype='M8[ns]')
            new_block = block.make_block_same_class(
                type(block.values)(values, dtype=block.values.dtype)
            )
        else:
            new_shape = (block.values.shape[0], size)
            values = np.empty(shape=new_shape, dtype=block.values.dtype)
            new_block = block.make_block_same_class(values=values)

        blocks.append(new_block)

    # create block manager
    df = DataFrame(BlockManager(blocks, axes))

    # create views
    for block in df._data.blocks:
        dtype = block.dtype
        inds = block.mgr_locs.indexer
        if isinstance(inds, slice):
            inds = list(range(inds.start, inds.stop, inds.step))
        for i, ind in enumerate(inds):
            col = df.columns[ind]
            if is_categorical_dtype(dtype):
                views[col] = block.values._codes
                views[col+'-catdef'] = block.values
            elif getattr(block.dtype, 'tz', None):
                views[col] = np.asarray(block.values, dtype='M8[ns]')
            else:
                views[col] = block.values[i]

    if index_names:
        df.index.names = [
            None if re.match(r'__index_level_\d+__', n) else n
            for n in index_names
        ]
    return df, views
