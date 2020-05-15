import io
import os
import shutil
import posixpath
import re
import tempfile
from fsspec import AbstractFileSystem
from fsspec.utils import stringify_path


class LocalFileSystem(AbstractFileSystem):
    """Interface to files on local storage

    Parameters
    ----------
    auto_mkdirs: bool
        Whether, when opening a file, the directory containing it should
        be created (if it doesn't already exist). This is assumed by pyarrow
        code.
    """

    root_marker = "/"
    protocol = "file"

    def __init__(self, auto_mkdir=False, **kwargs):
        super().__init__(**kwargs)
        self.auto_mkdir = auto_mkdir

    def mkdir(self, path, create_parents=True, **kwargs):
        path = self._strip_protocol(path)
        if create_parents:
            self.makedirs(path, exist_ok=True)
        else:
            os.mkdir(path, **kwargs)

    def makedirs(self, path, exist_ok=False):
        path = self._strip_protocol(path)
        os.makedirs(path, exist_ok=exist_ok)

    def rmdir(self, path):
        os.rmdir(path)

    def ls(self, path, detail=False):
        path = self._strip_protocol(path)
        paths = [posixpath.join(path, f) for f in os.listdir(path)]
        if detail:
            return [self.info(f) for f in paths]
        else:
            return paths

    def glob(self, path, **kargs):
        path = self._strip_protocol(path)
        return super().glob(path)

    def info(self, path, **kwargs):
        path = self._strip_protocol(path)
        out = os.stat(path, follow_symlinks=False)
        dest = False
        if os.path.islink(path):
            t = "link"
            dest = os.readlink(path)
        elif os.path.isdir(path):
            t = "directory"
        elif os.path.isfile(path):
            t = "file"
        else:
            t = "other"
        result = {"name": path, "size": out.st_size, "type": t, "created": out.st_ctime}
        for field in ["mode", "uid", "gid", "mtime"]:
            result[field] = getattr(out, "st_" + field)
        if dest:
            result["destination"] = dest
            try:
                out2 = os.stat(path, follow_symlinks=True)
                result["size"] = out2.st_size
            except IOError:
                result["size"] = 0
        return result

    def copy(self, path1, path2, **kwargs):
        shutil.copyfile(path1, path2)

    def get(self, path1, path2, **kwargs):
        if kwargs.get("recursive"):
            return super(LocalFileSystem, self).get(path1, path2, **kwargs)
        else:
            return self.copy(path1, path2, **kwargs)

    def put(self, path1, path2, **kwargs):
        if kwargs.get("recursive"):
            return super(LocalFileSystem, self).put(path1, path2, **kwargs)
        else:
            return self.copy(path1, path2, **kwargs)

    def mv(self, path1, path2, **kwargs):
        os.rename(path1, path2)

    def rm(self, path, recursive=False, maxdepth=None):
        if recursive and self.isdir(path):
            shutil.rmtree(path)
        else:
            os.remove(path)

    def _open(self, path, mode="rb", block_size=None, **kwargs):
        path = self._strip_protocol(path)
        if self.auto_mkdir:
            self.makedirs(self._parent(path), exist_ok=True)
        return LocalFileOpener(path, mode, fs=self, **kwargs)

    def touch(self, path, **kwargs):
        path = self._strip_protocol(path)
        if self.exists(path):
            os.utime(path, None)
        else:
            open(path, "a").close()

    @classmethod
    def _parent(cls, path):
        path = cls._strip_protocol(path).rstrip("/")
        if "/" in path:
            return path.rsplit("/", 1)[0]
        else:
            return cls.root_marker

    @classmethod
    def _strip_protocol(cls, path):
        path = stringify_path(path)
        if path.startswith("file://"):
            path = path[7:]
        path = os.path.expanduser(path)
        return make_path_posix(path)

    def _isfilestore(self):
        # Inheriting from DaskFileSystem makes this False (S3, etc. were)
        # the original motivation. But we are a posix-like file system.
        # See https://github.com/dask/dask/issues/5526
        return True


def make_path_posix(path, sep=os.sep):
    """ Make path generic """
    if re.match("/[A-Za-z]:", path):
        # for windows file URI like "file:///C:/folder/file"
        # or "file:///C:\\dir\\file"
        path = path[1:]
    if path.startswith("\\\\"):
        # special case for windows UNC/DFS-style paths, do nothing,
        # jsut flip the slashes around (case below does not work!)
        return path.replace("\\", "/")
    if path.startswith("\\") or re.match("[\\\\]*[A-Za-z]:", path):
        # windows full path "\\server\\path" or "C:\\local\\path"
        return path.lstrip("\\").replace("\\", "/").replace("//", "/")
    if (
        sep not in path
        and "/" not in path
        or (sep == "/" and not path.startswith("/"))
        or (sep == "\\" and ":" not in path)
    ):
        # relative path like "path" or "rel\\path" (win) or rel/path"
        path = os.path.abspath(path)
        if os.sep == "\\":
            # abspath made some more '\\' separators
            return make_path_posix(path, sep)
    return path


class LocalFileOpener(object):
    def __init__(self, path, mode, autocommit=True, fs=None, **kwargs):
        self.path = path
        self.mode = mode
        self.fs = fs
        self.f = None
        self.autocommit = autocommit
        self.blocksize = io.DEFAULT_BUFFER_SIZE
        self._open()

    def _open(self):
        if self.f is None or self.f.closed:
            if self.autocommit or "w" not in self.mode:
                self.f = open(self.path, mode=self.mode)
            else:
                # TODO: check if path is writable?
                i, name = tempfile.mkstemp()
                os.close(i)  # we want normal open and normal buffered file
                self.temp = name
                self.f = open(name, mode=self.mode)
            if "w" not in self.mode:
                self.details = self.fs.info(self.path)
                self.size = self.details["size"]
                self.f.size = self.size

    def _fetch_range(self, start, end):
        # probably only used by cached FS
        if "r" not in self.mode:
            raise ValueError
        self._open()
        self.f.seek(start)
        return self.f.read(end - start)

    def __setstate__(self, state):
        if "r" in state["mode"]:
            loc = self.state.pop("loc")
            self._open()
            self.f.seek(loc)
        else:
            self.f = None
        self.__dict__.update(state)

    def __getstate__(self):
        d = self.__dict__.copy()
        d.pop("f")
        if "r" in self.mode:
            d["loc"] = self.f.tell()
        else:
            if not self.f.closed:
                raise ValueError("Cannot serialise open write-mode local file")
        return d

    def commit(self):
        if self.autocommit:
            raise RuntimeError("Can only commit if not already set to autocommit")
        os.replace(self.temp, self.path)

    def discard(self):
        if self.autocommit:
            raise RuntimeError("Cannot discard if set to autocommit")
        os.remove(self.temp)

    def __fspath__(self):
        # uniquely among fsspec implementations, this is a real, local path
        return self.path

    def __getattr__(self, item):
        return getattr(self.f, item)

    def __enter__(self):
        self._incontext = True
        return self.f.__enter__()

    def __exit__(self, exc_type, exc_value, traceback):
        self._incontext = False
        self.f.__exit__(exc_type, exc_value, traceback)
