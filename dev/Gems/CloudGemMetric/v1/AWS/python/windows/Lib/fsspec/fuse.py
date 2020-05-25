from __future__ import print_function
import os
import stat
from errno import ENOENT, EIO
from fuse import Operations, FuseOSError
import threading
import time
from fuse import FUSE


class FUSEr(Operations):
    def __init__(self, fs, path):
        self.fs = fs
        self.cache = {}
        self.root = path.rstrip("/") + "/"
        self.counter = 0

    def getattr(self, path, fh=None):
        path = "".join([self.root, path.lstrip("/")]).rstrip("/")
        try:
            info = self.fs.info(path)
        except FileNotFoundError:
            raise FuseOSError(ENOENT)
        data = {"st_uid": 1000, "st_gid": 1000}
        perm = 0o777

        if info["type"] != "file":
            data["st_mode"] = stat.S_IFDIR | perm
            data["st_size"] = 0
            data["st_blksize"] = 0
        else:
            data["st_mode"] = stat.S_IFREG | perm
            data["st_size"] = info["size"]
            data["st_blksize"] = 5 * 2 ** 20
            data["st_nlink"] = 1
        data["st_atime"] = time.time()
        data["st_ctime"] = time.time()
        data["st_mtime"] = time.time()
        return data

    def readdir(self, path, fh):
        path = "".join([self.root, path.lstrip("/")])
        files = self.fs.ls(path, False)
        files = [os.path.basename(f.rstrip("/")) for f in files]
        return [".", ".."] + files

    def mkdir(self, path, mode):
        path = "".join([self.root, path.lstrip("/")])
        self.fs.mkdir(path)
        return 0

    def rmdir(self, path):
        path = "".join([self.root, path.lstrip("/")])
        self.fs.rmdir(path)
        return 0

    def read(self, path, size, offset, fh):
        f = self.cache[fh]
        f.seek(offset)
        out = f.read(size)
        return out

    def write(self, path, data, offset, fh):
        f = self.cache[fh]
        f.write(data)
        return len(data)

    def create(self, path, flags, fi=None):
        fn = "".join([self.root, path.lstrip("/")])
        f = self.fs.open(fn, "wb")
        self.cache[self.counter] = f
        self.counter += 1
        return self.counter - 1

    def open(self, path, flags):
        fn = "".join([self.root, path.lstrip("/")])
        if flags % 2 == 0:
            # read
            mode = "rb"
        else:
            # write/create
            mode = "wb"
        self.cache[self.counter] = self.fs.open(fn, mode)
        self.counter += 1
        return self.counter - 1

    def truncate(self, path, length, fh=None):
        fn = "".join([self.root, path.lstrip("/")])
        if length != 0:
            raise NotImplementedError
        # maybe should be no-op since open with write sets size to zero anyway
        self.fs.touch(fn)

    def unlink(self, path):
        fn = "".join([self.root, path.lstrip("/")])
        try:
            self.fs.rm(fn, False)
        except (IOError, FileNotFoundError):
            raise FuseOSError(EIO)

    def release(self, path, fh):
        try:
            if fh in self.cache:
                f = self.cache[fh]
                f.close()
                self.cache.pop(fh)
        except Exception as e:
            print(e)
        return 0

    def chmod(self, path, mode):
        raise NotImplementedError


def run(fs, path, mount_point, foreground=True, threads=False):
    """ Mount stuff in a local directory

    This uses fusepy to make it appear as if a given path on an fsspec
    instance is in fact resident within the local file-system.

    This requires that fusepy by installed, and that FUSE be available on
    the system (typically requiring a package to be installed with
    apt, yum, brew, etc.).

    Parameters
    ----------
    fs: file-system instance
        From one of the compatible implementations
    path: str
        Location on that file-system to regard as the root directory to
        mount. Note that you typically should include the terminating "/"
        character.
    mount_point: str
        An empty directory on the local file-system where the contents of
        the remote path will appear
    foreground: bool
        Whether or not calling this function will block. Operation will
        typically be more stable if True.
    threads: bool
        Whether or not to create threads when responding to file operations
        within the mounter directory. Operation will typically be more
        stable if False.

    """
    func = lambda: FUSE(
        FUSEr(fs, path), mount_point, nothreads=not threads, foreground=True
    )
    if foreground is False:
        th = threading.Thread(target=func)
        th.daemon = True
        th.start()
        return th
    else:  # pragma: no cover
        try:
            func()
        except KeyboardInterrupt:
            pass
