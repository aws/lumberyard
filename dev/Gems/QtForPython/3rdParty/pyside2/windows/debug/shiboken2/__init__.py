__version__ = "5.12.4"
__version_info__ = (5, 12, 4, "", "")

# PYSIDE-932: Python 2 cannot import 'zipfile' for embedding while being imported, itself.
# We simply pre-load all imports for the signature extension.
# Also, PyInstaller seems not always to be reliable in finding modules.
# We explicitly import everything that is needed:
import sys
import os
import zipfile
import base64
import marshal
import io
import contextlib
import textwrap
import traceback
import types
import struct
import re
import tempfile
import keyword
import functools
if sys.version_info[0] == 3:
    # PyInstaller seems to sometimes fail:
    import typing

from .shiboken2 import *

# Trigger signature initialization.
type.__signature__
