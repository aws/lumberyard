
import array
from contextlib import contextmanager
import os
import tempfile
import shutil
import sys

PY2 = sys.version_info[0] == 2
PY3 = sys.version_info[0] == 3


@contextmanager
def ignoring(*exceptions):
    try:
        yield
    except exceptions:
        pass


@contextmanager
def tmpfile(extension='', dir=None):
    extension = '.' + extension.lstrip('.')
    handle, filename = tempfile.mkstemp(extension, dir=dir)
    os.close(handle)
    os.remove(filename)

    try:
        yield filename
    finally:
        if os.path.exists(filename):
            if os.path.isdir(filename):
                shutil.rmtree(filename)
            else:
                with ignoring(OSError):
                    os.remove(filename)


def seek_delimiter(file, delimiter, blocksize):
    """ Seek current file to next byte after a delimiter bytestring

    This seeks the file to the next byte following the delimiter.  It does
    not return anything.  Use ``file.tell()`` to see location afterwards.

    Parameters
    ----------
    file: a file
    delimiter: bytes
        a delimiter like ``b'\n'`` or message sentinel
    blocksize: int
        Number of bytes to read from the file at once.
    """

    if file.tell() == 0:
        return

    last = b''
    while True:
        current = file.read(blocksize)
        if not current:
            return
        full = last + current
        try:
            i = full.index(delimiter)
            file.seek(file.tell() - (len(full) - i) + len(delimiter))
            return
        except ValueError:
            pass
        last = full[-len(delimiter):]



def read_block(f, offset, length, delimiter=None):
    """ Read a block of bytes from a file

    Parameters
    ----------
    fn: string
        Path to filename on S3
    offset: int
        Byte offset to start read
    length: int
        Number of bytes to read
    delimiter: bytes (optional)
        Ensure reading starts and stops at delimiter bytestring

    If using the ``delimiter=`` keyword argument we ensure that the read
    starts and stops at delimiter boundaries that follow the locations
    ``offset`` and ``offset + length``.  If ``offset`` is zero then we
    start at zero.  The bytestring returned WILL include the
    terminating delimiter string.

    Examples
    --------

    >>> from io import BytesIO  # doctest: +SKIP
    >>> f = BytesIO(b'Alice, 100\\nBob, 200\\nCharlie, 300')  # doctest: +SKIP
    >>> read_block(f, 0, 13)  # doctest: +SKIP
    b'Alice, 100\\nBo'

    >>> read_block(f, 0, 13, delimiter=b'\\n')  # doctest: +SKIP
    b'Alice, 100\\nBob, 200\\n'

    >>> read_block(f, 10, 10, delimiter=b'\\n')  # doctest: +SKIP
    b'Bob, 200\\nCharlie, 300'
    """
    if delimiter:
        f.seek(offset)
        seek_delimiter(f, delimiter, 2**16)
        start = f.tell()
        length -= start - offset

        f.seek(start + length)
        seek_delimiter(f, delimiter, 2**16)
        end = f.tell()
        eof = not f.read(1)

        offset = start
        length = end - start

    f.seek(offset)
    bytes = f.read(length)
    return bytes


def raises(exc, lamda):
    try:
        lamda()
        return False
    except exc:
        return True


def ensure_writable(b):
    if PY2 and isinstance(b, array.array):
        return b.tostring()
    return b


def title_case(string):
    """
    TitleCases a given string.

    Parameters
    ----------
    string : underscore seperated string
    """
    return ''.join([x.capitalize() for x in string.split('_')])


class ParamKwargsHelper(object):
    """
    Utility class to help extract the subset of keys that an s3 method is
    actually using

    Parameters
    ----------
    s3 : boto S3FileSystem
    """
    _kwarg_cache = {}

    def __init__(self, s3):
        self.s3 = s3

    def _get_valid_keys(self, model_name):
        if model_name not in self._kwarg_cache:
            model = self.s3.meta.service_model.operation_model(model_name)
            valid_keys = set(model.input_shape.members.keys())
            self._kwarg_cache[model_name] = valid_keys
        return self._kwarg_cache[model_name]

    def filter_dict(self, method_name, d):
        model_name = title_case(method_name)
        valid_keys = self._get_valid_keys(model_name)
        if isinstance(d, SSEParams):
            d = d.to_kwargs()
        return {k: v for k, v in d.items() if k in valid_keys}


class SSEParams(object):

    def __init__(self, server_side_encryption=None, sse_customer_algorithm=None,
                 sse_customer_key=None, sse_kms_key_id=None):
        self.ServerSideEncryption = server_side_encryption
        self.SSECustomerAlgorithm = sse_customer_algorithm
        self.SSECustomerKey = sse_customer_key
        self.SSEKMSKeyId = sse_kms_key_id

    def to_kwargs(self):
        return {k: v for k, v in self.__dict__.items() if v is not None}