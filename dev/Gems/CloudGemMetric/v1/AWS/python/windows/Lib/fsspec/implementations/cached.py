import time
import pickle
import logging
import os
import hashlib
import tempfile
import inspect
from fsspec import AbstractFileSystem, filesystem
from fsspec.spec import AbstractBufferedFile
from fsspec.core import MMapCache, BaseCache

logger = logging.getLogger("fsspec")


class CachingFileSystem(AbstractFileSystem):
    """Locally caching filesystem, layer over any other FS

    This class implements chunk-wise local storage of remote files, for quick
    access after the initial download. The files are stored in a given
    directory with random hashes for the filenames. If no directory is given,
    a temporary one is used, which should be cleaned up by the OS after the
    process ends. The files themselves as sparse (as implemented in
    MMapCache), so only the data which is accessed takes up space.

    Restrictions:

    - the block-size must be the same for each access of a given file, unless
      all blocks of the file have already been read
    - caching can only be applied to file-systems which produce files
      derived from fsspec.spec.AbstractBufferedFile ; LocalFileSystem is also
      allowed, for testing
    """

    protocol = ("blockcache", "cached")

    def __init__(
        self,
        target_protocol=None,
        cache_storage="TMP",
        cache_check=10,
        check_files=False,
        expiry_time=604800,
        target_options=None,
        fs=None,
        same_names=False,
        **kwargs
    ):
        """

        Parameters
        ----------
        target_protocol: str (optional)
            Target filesystem protocol. Provide either this or ``fs``.
        cache_storage: str or list(str)
            Location to store files. If "TMP", this is a temporary directory,
            and will be cleaned up by the OS when this process ends (or later).
            If a list, each location will be tried in the order given, but
            only the last will be considered writable.
        cache_check: int
            Number of seconds between reload of cache metadata
        check_files: bool
            Whether to explicitly see if the UID of the remote file matches
            the stored one before using. Warning: some file systems such as
            HTTP cannot reliably give a unique hash of the contents of some
            path, so be sure to set this option to False.
        expiry_time: int
            The time in seconds after which a local copy is considered useless.
            Set to falsy to prevent expiry. The default is equivalent to one
            week.
        target_options: dict or None
            Passed to the instantiation of the FS, if fs is None.
        fs: filesystem instance
            The target filesystem to run against. Provide this or ``protocol``.
        same_names: bool (optional)
            By default, target URLs are hashed, so that files from different backends
            with the same basename do not conflict. If this is true, the original
            basename is used.
        """
        super().__init__(**kwargs)
        if not (fs is None) ^ (target_protocol is None):
            raise ValueError(
                "Please provide one of filesystem instance (fs) or"
                " remote_protocol, not both"
            )
        if cache_storage == "TMP":
            storage = [tempfile.mkdtemp()]
        else:
            if isinstance(cache_storage, str):
                storage = [cache_storage]
            else:
                storage = cache_storage
        os.makedirs(storage[-1], exist_ok=True)
        self.storage = storage
        self.kwargs = target_options or {}
        self.cache_check = cache_check
        self.check_files = check_files
        self.expiry = expiry_time
        self.same_names = same_names
        self.target_protocol = (
            target_protocol if isinstance(target_protocol, str) else fs.protocol
        )
        self.load_cache()
        self.fs = fs if fs is not None else filesystem(target_protocol, **self.kwargs)

    def load_cache(self):
        """Read set of stored blocks from file"""
        cached_files = []
        for storage in self.storage:
            fn = os.path.join(storage, "cache")
            if os.path.exists(fn):
                with open(fn, "rb") as f:
                    # TODO: consolidate blocks here
                    cached_files.append(pickle.load(f))
            else:
                os.makedirs(storage, exist_ok=True)
                cached_files.append({})
        self.cached_files = cached_files or [{}]
        self.last_cache = time.time()

    def save_cache(self):
        """Save set of stored blocks from file"""
        fn = os.path.join(self.storage[-1], "cache")
        # TODO: a file lock could be used to ensure file does not change
        #  between re-read and write; but occasional duplicated reads ok.
        cache = self.cached_files[-1]
        if os.path.exists(fn):
            with open(fn, "rb") as f:
                cached_files = pickle.load(f)
            for k, c in cached_files.items():
                if c["blocks"] is not True:
                    if cache[k]["blocks"] is True:
                        c["blocks"] = True
                    else:
                        c["blocks"] = set(c["blocks"]).union(cache[k]["blocks"])

            # Files can be added to cache after it was written once
            for k, c in cache.items():
                if k not in cached_files:
                    cached_files[k] = c
        else:
            cached_files = cache
        cache = {k: v.copy() for k, v in cached_files.items()}
        for c in cache.values():
            if isinstance(c["blocks"], set):
                c["blocks"] = list(c["blocks"])
        fn2 = tempfile.mktemp()
        with open(fn2, "wb") as f:
            pickle.dump(cache, f)
        os.replace(fn2, fn)

    def _check_cache(self):
        """Reload caches if time elapsed or any disappeared"""
        if not self.cache_check:
            # explicitly told not to bother checking
            return
        timecond = time.time() - self.last_cache > self.cache_check
        existcond = all(os.path.exists(storage) for storage in self.storage)
        if timecond or not existcond:
            self.load_cache()

    def _check_file(self, path):
        """Is path in cache and still valid"""
        self._check_cache()
        if not path.startswith(self.target_protocol):
            store_path = self.target_protocol + "://" + path
            path = self.fs._strip_protocol(store_path)
        for storage, cache in zip(self.storage, self.cached_files):
            if path not in cache:
                continue
            detail = cache[path].copy()
            if self.check_files:
                if detail["uid"] != self.fs.ukey(path):
                    continue
            if self.expiry:
                if detail["time"] - time.time() > self.expiry:
                    continue
            fn = os.path.join(storage, detail["fn"])
            if os.path.exists(fn):
                return detail, fn
        return False, None

    def _open(
        self,
        path,
        mode="rb",
        block_size=None,
        autocommit=True,
        cache_options=None,
        **kwargs
    ):
        """Wrap the target _open

        If the whole file exists in the cache, just open it locally and
        return that.

        Otherwise, open the file on the target FS, and make it have a mmap
        cache pointing to the location which we determine, in our cache.
        The ``blocks`` instance is shared, so as the mmap cache instance
        updates, so does the entry in our ``cached_files`` attribute.
        We monkey-patch this file, so that when it closes, we call
        ``close_and_update`` to save the state of the blocks.
        """
        path = self._strip_protocol(path)

        if not path.startswith(self.target_protocol):
            store_path = self.target_protocol + "://" + path
        else:
            store_path = path
        path = self.fs._strip_protocol(store_path)
        if "r" not in mode:
            return self.fs._open(
                path,
                mode=mode,
                block_size=block_size,
                autocommit=autocommit,
                cache_options=cache_options,
                **kwargs
            )
        detail, fn = self._check_file(store_path)
        if detail:
            # file is in cache
            hash, blocks = detail["fn"], detail["blocks"]
            if blocks is True:
                # stored file is complete
                logger.debug("Opening local copy of %s" % path)
                return open(fn, mode)
            # TODO: action where partial file exists in read-only cache
            logger.debug("Opening partially cached copy of %s" % path)
        else:
            hash = hash_name(path, self.same_names)
            fn = os.path.join(self.storage[-1], hash)
            blocks = set()
            detail = {
                "fn": hash,
                "blocks": blocks,
                "time": time.time(),
                "uid": self.fs.ukey(path),
            }
            self.cached_files[-1][store_path] = detail
            logger.debug("Creating local sparse file for %s" % path)

        # call target filesystems open
        f = self.fs._open(
            path,
            mode=mode,
            block_size=block_size,
            autocommit=autocommit,
            cache_options=cache_options,
            cache_type=None,
            **kwargs
        )
        if "blocksize" in detail:
            if detail["blocksize"] != f.blocksize:
                raise ValueError(
                    "Cached file must be reopened with same block"
                    "size as original (old: %i, new %i)"
                    "" % (detail["blocksize"], f.blocksize)
                )
        else:
            detail["blocksize"] = f.blocksize
        f.cache = MMapCache(f.blocksize, f._fetch_range, f.size, fn, blocks)
        close = f.close
        f.close = lambda: self.close_and_update(f, close)
        self.save_cache()
        return f

    def close_and_update(self, f, close):
        """Called when a file is closing, so store the set of blocks"""
        path = self._strip_protocol(f.path)

        if not path.startswith(self.target_protocol):
            store_path = self.target_protocol + "://" + path
        c = self.cached_files[-1][store_path]
        if c["blocks"] is not True and len(["blocks"]) * f.blocksize >= f.size:
            c["blocks"] = True
        self.save_cache()
        close()

    def __getattribute__(self, item):
        if item in [
            "load_cache",
            "_open",
            "save_cache",
            "close_and_update",
            "__init__",
            "__getattribute__",
            "__reduce__",
            "open",
            "cat",
            "get",
            "read_block",
            "tail",
            "head",
            "_check_file",
            "_check_cache",
        ]:
            # all the methods defined in this class. Note `open` here, since
            # it calls `_open`, but is actually in superclass
            return lambda *args, **kw: getattr(type(self), item)(self, *args, **kw)
        if item in ["__reduce_ex__"]:
            raise AttributeError
        if item in ["_strip_protocol"]:
            # class methods
            return lambda *args, **kw: getattr(type(self), item)(*args, **kw)
        if item in ["_cache"]:
            # class attributes
            return getattr(type(self), item)
        if item == "__class__":
            return type(self)
        d = object.__getattribute__(self, "__dict__")
        fs = d.get("fs", None)  # fs is not immediately defined
        if item in d:
            return d[item]
        elif fs is not None:
            if item in fs.__dict__:
                # attribute of instance
                return fs.__dict__[item]
            # attributed belonging to the target filesystem
            cls = type(fs)
            m = getattr(cls, item)
            if inspect.isfunction(m) and (
                not hasattr(m, "__self__") or m.__self__ is None
            ):
                # instance method
                return m.__get__(fs, cls)
            return m  # class method or attribute
        else:
            # attributes of the superclass, while target is being set up
            return super().__getattribute__(item)


class WholeFileCacheFileSystem(CachingFileSystem):
    """Caches whole remote files on first access

    This class is intended as a layer over any other file system, and
    will make a local copy of each file accessed, so that all subsequent
    reads are local. This is similar to ``CachingFileSystem``, but without
    the block-wise functionality and so can work even when sparse files
    are not allowed. See its docstring for definition of the init
    arguments.

    The class still needs access to the remote store for listing files,
    and may refresh cached files.
    """

    protocol = "filecache"

    def _open(self, path, mode="rb", **kwargs):
        path = self._strip_protocol(path)

        if not path.startswith(self.target_protocol):
            store_path = self.target_protocol + "://" + path
        else:
            store_path = path
        path = self.fs._strip_protocol(store_path)
        if "r" not in mode:
            return self.fs._open(path, mode=mode, **kwargs)
        detail, fn = self._check_file(store_path)
        if detail:
            hash, blocks = detail["fn"], detail["blocks"]
            if blocks is True:
                logger.debug("Opening local copy of %s" % path)
                return open(fn, mode)
            else:
                raise ValueError(
                    "Attempt to open partially cached file %s"
                    "as a wholly cached file" % path
                )
        else:
            hash = hash_name(path, self.same_names)
            fn = os.path.join(self.storage[-1], hash)
            blocks = True
            detail = {
                "fn": hash,
                "blocks": blocks,
                "time": time.time(),
                "uid": self.fs.ukey(path),
            }
            self.cached_files[-1][store_path] = detail
            logger.debug("Copying %s to local cache" % path)
        kwargs["mode"] = mode

        # call target filesystems open
        # TODO: why not just use fs.get ??
        f = self.fs._open(path, **kwargs)
        with open(fn, "wb") as f2:
            if isinstance(f, AbstractBufferedFile):
                # want no type of caching if just downloading whole thing
                f.cache = BaseCache(0, f.cache.fetcher, f.size)
            if getattr(f, "blocksize", 0) and f.size:
                # opportunity to parallelise here
                data = True
                while data:
                    data = f.read(f.blocksize)
                    f2.write(data)
            else:
                # this only applies to HTTP, should instead use streaming
                f2.write(f.read())
        self.save_cache()
        return self._open(path, mode)


class SimpleCacheFileSystem(CachingFileSystem):
    """Caches whole remote files on first access

    This class is intended as a layer over any other file system, and
    will make a local copy of each file accessed, so that all subsequent
    reads are local. This implementation only copies whole files, and
    does not keep any metadata about the download time or file details.
    It is therefore safer to use in multi-threaded/concurrent situations.

    """

    protocol = "simplecache"

    def __init__(self, **kwargs):
        kw = kwargs.copy()
        for key in ["cache_check", "expiry_time", "check_files"]:
            kw[key] = False
        super().__init__(**kw)
        for storage in self.storage:
            if not os.path.exists(storage):
                os.makedirs(storage, exist_ok=True)
        self.cached_files = [{}]

    def _check_file(self, path):
        sha = hash_name(path, self.same_names)
        for storage in self.storage:
            fn = os.path.join(storage, sha)
            if os.path.exists(fn):
                return fn

    def save_cache(self):
        pass

    def load_cache(self):
        pass

    def _open(self, path, mode="rb", **kwargs):
        path = self._strip_protocol(path)

        if not path.startswith(self.target_protocol):
            store_path = self.target_protocol + "://" + path
        else:
            store_path = path
        path = self.fs._strip_protocol(store_path)
        if "r" not in mode:
            return self.fs._open(path, mode=mode, **kwargs)
        fn = self._check_file(path)
        if fn:
            return open(fn, mode)

        sha = hash_name(path, self.same_names)
        fn = os.path.join(self.storage[-1], sha)
        logger.debug("Copying %s to local cache" % path)
        kwargs["mode"] = mode

        with self.fs._open(path, **kwargs) as f, open(fn, "wb") as f2:
            if isinstance(f, AbstractBufferedFile):
                # want no type of caching if just downloading whole thing
                f.cache = BaseCache(0, f.cache.fetcher, f.size)
            if getattr(f, "blocksize", 0) and f.size:
                # opportunity to parallelise here
                data = True
                while data:
                    data = f.read(f.blocksize)
                    f2.write(data)
            else:
                # this only applies to HTTP, should instead use streaming
                f2.write(f.read())
        return self._open(path, mode)


def hash_name(path, same_name):
    if same_name:
        hash = os.path.basename(path)
    else:
        hash = hashlib.sha256(path.encode()).hexdigest()
    return hash
