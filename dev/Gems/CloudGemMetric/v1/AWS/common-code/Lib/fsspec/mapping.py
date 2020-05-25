from collections.abc import MutableMapping
from .registry import get_filesystem_class
from .core import split_protocol


class FSMap(MutableMapping):
    """Wrap a FileSystem instance as a mutable wrapping.

    The keys of the mapping become files under the given root, and the
    values (which must be bytes) the contents of those files.

    Parameters
    ----------
    root: string
        prefix for all the files
    fs: FileSystem instance
    check: bool (=True)
        performs a touch at the location, to check for write access.

    Examples
    --------
    >>> fs = FileSystem(**parameters) # doctest: +SKIP
    >>> d = FSMap('my-data/path/', fs) # doctest: +SKIP
    or, more likely
    >>> d = fs.get_mapper('my-data/path/')

    >>> d['loc1'] = b'Hello World' # doctest: +SKIP
    >>> list(d.keys()) # doctest: +SKIP
    ['loc1']
    >>> d['loc1'] # doctest: +SKIP
    b'Hello World'
    """

    def __init__(self, root, fs, check=False, create=False):
        self.fs = fs
        self.root = fs._strip_protocol(root).rstrip(
            "/"
        )  # we join on '/' in _key_to_str
        if create:
            if not self.fs.exists(root):
                self.fs.mkdir(root)
        if check:
            if not self.fs.exists(root):
                raise ValueError(
                    "Path %s does not exist. Create "
                    " with the ``create=True`` keyword" % root
                )
            self.fs.touch(root + "/a")
            self.fs.rm(root + "/a")

    def clear(self):
        """Remove all keys below root - empties out mapping
        """
        try:
            self.fs.rm(self.root, True)
            self.fs.mkdir(self.root)
        except:  # noqa: E722
            pass

    def _key_to_str(self, key):
        """Generate full path for the key"""
        if isinstance(key, (tuple, list)):
            key = str(tuple(key))
        else:
            key = str(key)
        return "/".join([self.root, key]) if self.root else key

    def _str_to_key(self, s):
        """Strip path of to leave key name"""
        return s[len(self.root) :].lstrip("/")

    def __getitem__(self, key, default=None):
        """Retrieve data"""
        k = self._key_to_str(key)
        try:
            result = self.fs.cat(k)
        except FileNotFoundError:
            if default is not None:
                return default
            raise KeyError(key)
        return result

    def pop(self, key, default=None):
        result = self.__getitem__(key, default)
        try:
            del self[key]
        except KeyError:
            pass
        return result

    def __setitem__(self, key, value):
        """Store value in key"""
        key = self._key_to_str(key)
        self.fs.mkdirs(self.fs._parent(key), exist_ok=True)
        with self.fs.open(key, "wb") as f:
            f.write(value)

    def __iter__(self):
        return (self._str_to_key(x) for x in self.fs.find(self.root))

    def __len__(self):
        return len(self.fs.find(self.root))

    def __delitem__(self, key):
        """Remove key"""
        try:
            self.fs.rm(self._key_to_str(key))
        except:  # noqa: E722
            raise KeyError

    def __contains__(self, key):
        """Does key exist in mapping?"""
        path = self._key_to_str(key)
        return self.fs.exists(path) and self.fs.isfile(path)

    def __getstate__(self):
        """Mapping should be pickleable"""
        # TODO: replace with reduce to reinstantiate?
        return self.fs, self.root

    def __setstate__(self, state):
        fs, root = state
        self.fs = fs
        self.root = root


def get_mapper(url, check=False, create=False, **kwargs):
    """Create key-value interface for given URL and options

    The URL will be of the form "protocol://location" and point to the root
    of the mapper required. All keys will be file-names below this location,
    and their values the contents of each key.

    Parameters
    ----------
    url: str
        Root URL of mapping
    check: bool
        Whether to attempt to read from the location before instantiation, to
        check that the mapping does exist
    create: bool
        Whether to make the directory corresponding to the root before
        instantiating

    Returns
    -------
    ``FSMap`` instance, the dict-like key-value store.
    """
    protocol, path = split_protocol(url)
    cls = get_filesystem_class(protocol)
    fs = cls(**kwargs)
    # Removing protocol here - could defer to each open() on the backend
    return FSMap(url, fs, check, create)
