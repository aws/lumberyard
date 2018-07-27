
from collections import MutableMapping
import os

from .core import S3FileSystem, split_path


class S3Map(MutableMapping):
    """Wrap an S3FileSystem as a mutable wrapping.

    The keys of the mapping become files under the given root, and the
    values (which must be bytes) the contents of those files.

    Parameters
    ----------
    root : string
        prefix for all the files (perhaps just a bucket name)
    s3 : S3FileSystem
    check : bool (=True)
        performs a touch at the location, to check writeability.

    Examples
    --------
    >>> s3 = s3fs.S3FileSystem() # doctest: +SKIP
    >>> d = MapWrapping('mybucket/mapstore/', s3=s3) # doctest: +SKIP
    >>> d['loc1'] = b'Hello World' # doctest: +SKIP
    >>> list(d.keys()) # doctest: +SKIP
    ['loc1']
    >>> d['loc1'] # doctest: +SKIP
    b'Hello World'
    """

    def __init__(self, root, s3=None, check=False, create=False):
        self.s3 = s3 or S3FileSystem.current()
        self.root = root
        if check:
            self.s3.touch(root+'/a')
            self.s3.rm(root+'/a')
        else:
            bucket = split_path(root)[0]
            if create:
                self.s3.mkdir(bucket)
            elif not self.s3.exists(bucket):
                raise ValueError("Bucket %s does not exist."
                        " Create bucket with the ``create=True`` keyword" %
                        bucket)

    def clear(self):
        """Remove all keys below root - empties out mapping
        """
        try:
            self.s3.rm(self.root, recursive=True)
        except (IOError, OSError):
            # ignore non-existence of root
            pass

    def _key_to_str(self, key):
        if isinstance(key, (tuple, list)):
            key = str(tuple(key))
        else:
            key = str(key)
        return '/'.join([self.root, key])

    def __getitem__(self, key):
        key = self._key_to_str(key)
        try:
            with self.s3.open(key, 'rb') as f:
                result = f.read()
        except (IOError, OSError):
            raise KeyError(key)
        return result

    def __setitem__(self, key, value):
        key = self._key_to_str(key)
        with self.s3.open(key, 'wb') as f:
            f.write(value)

    def keys(self):
        return (x[len(self.root) + 1:] for x in self.s3.walk(self.root))

    def __iter__(self):
        return self.keys()

    def __delitem__(self, key):
        self.s3.rm(self._key_to_str(key))

    def __contains__(self, key):
        return self.s3.exists(self._key_to_str(key))

    def __len__(self):
        return sum(1 for _ in self.keys())
