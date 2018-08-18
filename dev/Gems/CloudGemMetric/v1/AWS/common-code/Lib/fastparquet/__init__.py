"""parquet - read parquet files."""
from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals

from .thrift_structures import parquet_thrift
from .core import read_thrift
from .writer import write
from . import core, schema, converted_types, api
from .api import ParquetFile
from .util import ParquetException

__version__ = "0.1.3"
