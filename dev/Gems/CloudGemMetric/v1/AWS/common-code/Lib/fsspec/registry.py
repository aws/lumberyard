import importlib
from distutils.version import LooseVersion

__all__ = ["registry", "get_filesystem_class", "default"]

# mapping protocol: implementation class object
registry = {}
default = "file"

# protocols mapped to the class which implements them. This dict can
# be dynamically updated.
known_implementations = {
    "file": {"class": "fsspec.implementations.local.LocalFileSystem"},
    "memory": {"class": "fsspec.implementations.memory.MemoryFileSystem"},
    "dropbox": {
        "class": "dropboxdrivefs.DropboxDriveFileSystem",
        "err": (
            'DropboxFileSystem requires "dropboxdrivefs",'
            '"requests" and "dropbox" to be installed'
        ),
    },
    "http": {
        "class": "fsspec.implementations.http.HTTPFileSystem",
        "err": 'HTTPFileSystem requires "requests" to be installed',
    },
    "https": {
        "class": "fsspec.implementations.http.HTTPFileSystem",
        "err": 'HTTPFileSystem requires "requests" to be installed',
    },
    "zip": {"class": "fsspec.implementations.zip.ZipFileSystem"},
    "gcs": {
        "class": "gcsfs.GCSFileSystem",
        "err": "Please install gcsfs to access Google Storage",
    },
    "gs": {
        "class": "gcsfs.GCSFileSystem",
        "err": "Please install gcsfs to access Google Storage",
    },
    "sftp": {
        "class": "fsspec.implementations.sftp.SFTPFileSystem",
        "err": 'SFTPFileSystem requires "paramiko" to be installed',
    },
    "ssh": {
        "class": "fsspec.implementations.sftp.SFTPFileSystem",
        "err": 'SFTPFileSystem requires "paramiko" to be installed',
    },
    "ftp": {"class": "fsspec.implementations.ftp.FTPFileSystem"},
    "hdfs": {
        "class": "fsspec.implementations.hdfs.PyArrowHDFS",
        "err": "pyarrow and local java libraries required for HDFS",
    },
    "webhdfs": {
        "class": "fsspec.implementations.webhdfs.WebHDFS",
        "err": 'webHDFS access requires "requests" to be installed',
    },
    "s3": {"class": "s3fs.S3FileSystem", "err": "Install s3fs to access S3"},
    "adl": {
        "class": "adlfs.AzureDatalakeFileSystem",
        "err": "Install adlfs to access Azure Datalake Gen1",
    },
    "abfs": {
        "class": "adlfs.AzureBlobFileSystem",
        "err": "Install adlfs to access Azure Datalake Gen2 and Azure Blob Storage",
    },
    "cached": {"class": "fsspec.implementations.cached.CachingFileSystem"},
    "blockcache": {"class": "fsspec.implementations.cached.CachingFileSystem"},
    "filecache": {"class": "fsspec.implementations.cached.WholeFileCacheFileSystem"},
    "simplecache": {"class": "fsspec.implementations.cached.SimpleCacheFileSystem"},
    "dask": {
        "class": "fsspec.implementations.dask.DaskWorkerFileSystem",
        "err": "Install dask distributed to access worker file system",
    },
    "github": {
        "class": "fsspec.implementations.github.GithubFileSystem",
        "err": "Install the requests package to use the github FS",
    },
}

minversions = {"s3fs": LooseVersion("0.3.0"), "gcsfs": LooseVersion("0.3.0")}


def get_filesystem_class(protocol):
    """Fetch named protocol implementation from the registry

    The dict ``known_implementations`` maps protocol names to the locations
    of classes implementing the corresponding file-system. When used for the
    first time, appropriate imports will happen and the class will be placed in
    the registry. All subsequent calls will fetch directly from the registry.

    Some protocol implementations require additional dependencies, and so the
    import may fail. In this case, the string in the "err" field of the
    ``known_implementations`` will be given as the error message.
    """
    if not protocol:
        protocol = default

    if protocol not in registry:
        if protocol not in known_implementations:
            raise ValueError("Protocol not known: %s" % protocol)
        bit = known_implementations[protocol]
        registry[protocol] = _import_class(bit["class"])
    cls = registry[protocol]
    if getattr(cls, "protocol", None) in ("abstract", None):
        cls.protocol = protocol

    return cls


def _import_class(cls, minv=None):
    mod, name = cls.rsplit(".", 1)
    minv = minv or minversions
    minversion = minv.get(mod, None)

    mod = importlib.import_module(mod)
    if minversion:
        version = getattr(mod, "__version__", None)
        if version and LooseVersion(version) < minversion:
            raise RuntimeError(
                "'{}={}' is installed, but version '{}' or "
                "higher is required".format(mod.__name__, version, minversion)
            )
    return getattr(mod, name)


def filesystem(protocol, **storage_options):
    """Instantiate filesystems for given protocol and arguments

    ``storage_options`` are specific to the protocol being chosen, and are
    passed directly to the class.
    """
    cls = get_filesystem_class(protocol)
    return cls(**storage_options)
