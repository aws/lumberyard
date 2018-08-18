import numpy as np
from pandas.core.index import _ensure_index, CategoricalIndex, Index
from pandas.core.internals import BlockManager, _block_shape
from pandas.core.generic import NDFrame
from pandas.core.frame import DataFrame
from pandas.core.index import RangeIndex, Index
from pandas.core.categorical import Categorical, CategoricalDtype
try:
    from pandas.api.types import is_categorical_dtype
except ImportError:
    # Pandas <= 0.18.1
    from pandas.core.common import is_categorical_dtype
from .util import STR_TYPE


def empty(types, size, cats=None, cols=None, index_type=None, index_name=None):
    """
    Create empty DataFrame to assign into

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

    Returns
    -------
    - dataframe with correct shape and data-types
    - list of numpy views, in order, of the columns of the dataframe. Assign
        to this.
    """
    df = DataFrame()
    views = {}

    cols = cols if cols is not None else range(cols)
    if isinstance(types, STR_TYPE):
        types = types.split(',')
    for t, col in zip(types, cols):
        if str(t) == 'category':
            if cats is None or col not in cats:
                df[str(col)] = Categorical(
                        [], categories=RangeIndex(0, 2**14),
                        fastpath=True)
            elif isinstance(cats[col], int):
                df[str(col)] = Categorical(
                        [], categories=RangeIndex(0, cats[col]),
                        fastpath=True)
            else:  # explicit labels list
                df[str(col)] = Categorical([], categories=cats[col],
                                           fastpath=True)
        else:
            df[str(col)] = np.empty(0, dtype=t)

    if index_type is not None and index_type is not False:
        if index_name is None:
            raise ValueError('If using an index, must give an index name')
        if str(index_type) == 'category':
            if cats is None or index_name not in cats:
                c = Categorical(
                        [], categories=RangeIndex(0, 2**14),
                        fastpath=True)
            elif isinstance(cats[index_name], int):
                c = Categorical(
                        [], categories=RangeIndex(0, cats[index_name]),
                        fastpath=True)
            else:  # explicit labels list
                c = Categorical([], categories=cats[index_name],
                                           fastpath=True)
            print(cats, index_name, c)
            vals = np.empty(size, dtype=c.codes.dtype)
            index = CategoricalIndex(c)
            index._data._codes = vals
            views[index_name] = vals
        else:
            index = Index(np.empty(size, dtype=index_type))
            views[index_name] = index.values

        axes = [df._data.axes[0], index]
    else:
        axes = [df._data.axes[0], RangeIndex(size)]

    # allocate and create blocks
    blocks = []
    for block in df._data.blocks:
        if block.is_categorical:
            categories = block.values.categories
            code = np.zeros(shape=size, dtype=block.values.codes.dtype)
            values = Categorical(values=code, categories=categories,
                                 fastpath=True)
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
            else:
                views[col] = block.values[i]

    if index_name is not None and index_name is not False:
        df.index.name = index_name
    if str(index_type) == 'category':
        views[index_name+'-catdef'] = df._data.axes[1].values
    return df, views
