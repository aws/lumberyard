from ..spec import AbstractFileSystem
from ..utils import infer_storage_options
from pyarrow.hdfs import HadoopFileSystem


class PyArrowHDFS(AbstractFileSystem):
    """Adapted version of Arrow's HadoopFileSystem

    This is a very simple wrapper over pa.hdfs.HadoopFileSystem, which
    passes on all calls to the underlying class.
    """

    def __init__(
        self,
        host="default",
        port=0,
        user=None,
        kerb_ticket=None,
        driver="libhdfs",
        extra_conf=None,
        **kwargs
    ):
        """

        Parameters
        ----------
        host: str
            Hostname, IP or "default" to try to read from Hadoop config
        port: int
            Port to connect on, or default from Hadoop config if 0
        user: str or None
            If given, connect as this username
        kerb_ticket: str or None
            If given, use this ticket for authentication
        driver: 'libhdfs' or 'libhdfs3'
            Binary driver; libhdfs if the JNI library and default
        extra_conf: None or dict
            Passed on to HadoopFileSystem
        """
        if self._cached:
            return
        AbstractFileSystem.__init__(self, **kwargs)
        self.pars = (host, port, user, kerb_ticket, driver, extra_conf)
        self.pahdfs = HadoopFileSystem(
            host=host,
            port=port,
            user=user,
            kerb_ticket=kerb_ticket,
            driver=driver,
            extra_conf=extra_conf,
        )

    def _open(
        self,
        path,
        mode="rb",
        block_size=None,
        autocommit=True,
        cache_options=None,
        **kwargs
    ):
        """

        Parameters
        ----------
        path: str
            Location of file; should start with '/'
        mode: str
        block_size: int
            Hadoop block size, e.g., 2**26
        autocommit: True
            Transactions are not yet implemented for HDFS; errors if not True
        kwargs: dict or None
            Hadoop config parameters

        Returns
        -------
        HDFSFile file-like instance
        """

        return HDFSFile(
            self,
            path,
            mode,
            block_size=block_size,
            autocommit=autocommit,
            cache_options=cache_options,
            **kwargs
        )

    def __reduce_ex__(self, protocol):
        return PyArrowHDFS, self.pars

    def ls(self, path, detail=True):
        out = self.pahdfs.ls(path, detail)
        if detail:
            for p in out:
                p["type"] = p["kind"]
                p["name"] = self._strip_protocol(p["name"])
        else:
            out = [self._strip_protocol(p) for p in out]
        return out

    @staticmethod
    def _get_kwargs_from_urls(path):
        ops = infer_storage_options(path)
        out = {}
        if ops.get("host", None):
            out["host"] = ops["host"]
        if ops.get("username", None):
            out["user"] = ops["username"]
        if ops.get("port", None):
            out["port"] = ops["port"]
        return out

    @classmethod
    def _strip_protocol(cls, path):
        ops = infer_storage_options(path)
        return ops["path"]

    def __getattribute__(self, item):
        if item in [
            "_open",
            "__init__",
            "__getattribute__",
            "__reduce_ex__",
            "open",
            "ls",
            "makedirs",
        ]:
            # all the methods defined in this class. Note `open` here, since
            # it calls `_open`, but is actually in superclass
            return lambda *args, **kw: getattr(PyArrowHDFS, item)(self, *args, **kw)
        if item == "__class__":
            return PyArrowHDFS
        d = object.__getattribute__(self, "__dict__")
        pahdfs = d.get("pahdfs", None)  # fs is not immediately defined
        if pahdfs is not None and item in [
            "chmod",
            "chown",
            "user",
            "df",
            "disk_usage",
            "download",
            "driver",
            "exists",
            "extra_conf",
            "get_capacity",
            "get_space_used",
            "host",
            "is_open",
            "kerb_ticket",
            "strip_protocol",
            "mkdir",
            "mv",
            "port",
            "get_capacity",
            "get_space_used",
            "df",
            "chmod",
            "chown",
            "disk_usage",
            "download",
            "upload",
            "_get_kwargs_from_urls",
            "read_parquet",
            "rm",
            "stat",
            "upload",
        ]:
            return getattr(pahdfs, item)
        else:
            # attributes of the superclass, while target is being set up
            return super().__getattribute__(item)


class HDFSFile(object):
    """Wrapper around arrow's HdfsFile

    Allows seek beyond EOF and (eventually) commit/discard
    """

    def __init__(
        self,
        fs,
        path,
        mode,
        block_size,
        autocommit=True,
        cache_type="readahead",
        cache_options=None,
        **kwargs
    ):
        # TODO: Inherit from AbstractBufferedFile?
        if not autocommit:
            raise NotImplementedError(
                "HDFSFile cannot be opened with 'autocommit=False'."
            )

        self.fs = fs
        self.path = path
        self.mode = mode
        self.block_size = block_size
        self.fh = fs.pahdfs.open(path, mode, block_size, **kwargs)
        if self.fh.readable():
            self.seek_size = self.size()

    def seek(self, loc, whence=0):
        if whence == 0 and self.readable():
            loc = min(loc, self.seek_size)
        return self.fh.seek(loc, whence)

    def __getattr__(self, item):
        return getattr(self.fh, item)

    def __reduce_ex__(self, protocol):
        return HDFSFile, (self.fs, self.path, self.mode, self.block_size)

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.close()
