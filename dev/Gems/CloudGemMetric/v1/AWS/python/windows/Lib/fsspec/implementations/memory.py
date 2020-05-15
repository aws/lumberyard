from __future__ import print_function, division, absolute_import

from io import BytesIO
from fsspec import AbstractFileSystem
import logging

logger = logging.Logger("fsspec.memoryfs")


class MemoryFileSystem(AbstractFileSystem):
    """A filesystem based on a dict of BytesIO objects"""

    store = {}  # global
    pseudo_dirs = []
    protocol = "memory"
    root_marker = ""

    def ls(self, path, detail=False, **kwargs):
        if path in self.store:
            # there is a key with this exact name, but could also be directory
            out = [
                {
                    "name": path,
                    "size": self.store[path].getbuffer().nbytes,
                    "type": "file",
                }
            ]
        else:
            out = []
        path = path.strip("/").lstrip("/")
        paths = set()
        for p2 in self.store:
            has_slash = "/" if p2.startswith("/") else ""
            p = p2.lstrip("/")
            if "/" in p:
                root = p.rsplit("/", 1)[0]
            else:
                root = ""
            if root == path:
                out.append(
                    {
                        "name": has_slash + p,
                        "size": self.store[p2].getbuffer().nbytes,
                        "type": "file",
                    }
                )
            elif path and all(
                (a == b) for a, b in zip(path.split("/"), p.strip("/").split("/"))
            ):
                # implicit directory
                ppath = "/".join(p.split("/")[: len(path.split("/")) + 1])
                if ppath not in paths:
                    out.append(
                        {
                            "name": has_slash + ppath + "/",
                            "size": 0,
                            "type": "directory",
                        }
                    )
                    paths.add(ppath)
            elif all(
                (a == b)
                for a, b in zip(path.split("/"), [""] + p.strip("/").split("/"))
            ):
                # root directory entry
                ppath = p.rstrip("/").split("/", 1)[0]
                if ppath not in paths:
                    out.append(
                        {
                            "name": has_slash + ppath + "/",
                            "size": 0,
                            "type": "directory",
                        }
                    )
                    paths.add(ppath)
        for p2 in self.pseudo_dirs:
            if self._parent(p2).strip("/").rstrip("/") == path:
                out.append({"name": p2 + "/", "size": 0, "type": "directory"})
        if detail:
            return out
        return sorted([f["name"] for f in out])

    def mkdir(self, path):
        path = path.rstrip("/")
        if path not in self.pseudo_dirs:
            self.pseudo_dirs.append(path)

    def rmdir(self, path):
        path = path.rstrip("/")
        if path in self.pseudo_dirs:
            if self.ls(path) == []:
                self.pseudo_dirs.remove(path)
            else:
                raise OSError("Directory %s not empty" % path)
        else:
            raise FileNotFoundError(path)

    def exists(self, path):
        return path in self.store

    def _open(
        self,
        path,
        mode="rb",
        block_size=None,
        autocommit=True,
        cache_options=None,
        **kwargs
    ):
        if mode in ["rb", "ab", "rb+"]:
            if path in self.store:
                f = self.store[path]
                if mode == "rb":
                    f.seek(0)
                else:
                    f.seek(0, 2)
                return f
            else:
                raise FileNotFoundError(path)
        if mode == "wb":
            m = MemoryFile(self, path)
            if not self._intrans:
                m.commit()
            return m

    def copy(self, path1, path2, **kwargs):
        self.store[path2] = MemoryFile(self, path2, self.store[path1].getbuffer())

    def cat(self, path):
        try:
            return self.store[path].getvalue()
        except KeyError:
            raise FileNotFoundError(path)

    def _rm(self, path):
        del self.store[path]

    def size(self, path):
        """Size in bytes of the file at path"""
        if path not in self.store:
            raise FileNotFoundError(path)
        return self.store[path].getbuffer().nbytes


class MemoryFile(BytesIO):
    """A BytesIO which can't close and works as a context manager

    Can initialise with data

    No need to provide fs, path if auto-committing (default)
    """

    def __init__(self, fs, path, data=None):
        self.fs = fs
        self.path = path
        if data:
            self.write(data)
            self.size = len(data)
            self.seek(0)

    def __enter__(self):
        return self

    def close(self):
        self.size = self.seek(0, 2)

    def discard(self):
        pass

    def commit(self):
        self.fs.store[self.path] = self
