import paramiko
from stat import S_ISDIR, S_ISLNK
import types
import uuid
from .. import AbstractFileSystem
from ..utils import infer_storage_options


class SFTPFileSystem(AbstractFileSystem):
    """Files over SFTP/SSH

    Peer-to-peer filesystem over SSH using paramiko.

    Note: if using this with the ``open`` or ``open_files``, with full URLs,
    there is no way to tell if a path is relative, so all paths are assumed
    to be absolute.
    """

    protocol = "sftp", "ssh"

    def __init__(self, host, **ssh_kwargs):
        """

        Parameters
        ----------
        host: str
            Hostname or IP as a string
        temppath: str
            Location on the server to put files, when within a transaction
        ssh_kwargs: dict
            Parameters passed on to connection. See details in
            http://docs.paramiko.org/en/2.4/api/client.html#paramiko.client.SSHClient.connect
            May include port, username, password...
        """
        if self._cached:
            return
        super(SFTPFileSystem, self).__init__(**ssh_kwargs)
        self.temppath = ssh_kwargs.pop("temppath", "/tmp")
        self.host = host
        self.ssh_kwargs = ssh_kwargs
        self._connect()

    def _connect(self):
        self.client = paramiko.SSHClient()
        self.client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
        self.client.connect(self.host, **self.ssh_kwargs)
        self.ftp = self.client.open_sftp()

    @classmethod
    def _strip_protocol(cls, path):
        return infer_storage_options(path)["path"]

    @staticmethod
    def _get_kwargs_from_urls(urlpath):
        out = infer_storage_options(urlpath)
        out.pop("path", None)
        out.pop("protocol", None)
        return out

    def mkdir(self, path, mode=511):
        self.ftp.mkdir(path, mode)

    def makedirs(self, path, exist_ok=False, mode=511):
        if self.exists(path) and not exist_ok:
            raise FileExistsError("File exists: {}".format(path))

        parts = path.split("/")
        path = ""

        for part in parts:
            path += "/" + part
            if not self.exists(path):
                self.mkdir(path, mode)

    def rmdir(self, path):
        self.ftp.rmdir(path)

    def info(self, path):
        s = self.ftp.stat(path)
        if S_ISDIR(s.st_mode):
            t = "directory"
        elif S_ISLNK(s.st_mode):
            t = "link"
        else:
            t = "file"
        return {
            "name": path + "/" if t == "directory" else path,
            "size": s.st_size,
            "type": t,
            "uid": s.st_uid,
            "gui": s.st_gid,
            "time": s.st_atime,
            "mtime": s.st_mtime,
        }

    def ls(self, path, detail=False):
        out = ["/".join([path.rstrip("/"), p]) for p in self.ftp.listdir(path)]
        out = [self.info(o) for o in out]
        if detail:
            return out
        return sorted([p["name"] for p in out])

    def put(self, lpath, rpath):
        self.ftp.put(lpath, rpath)

    def get(self, rpath, lpath):
        self.ftp.get(rpath, lpath)

    def _open(self, path, mode="rb", block_size=None, **kwargs):
        """
        block_size: int or None
            If 0, no buffering, if 1, line buffering, if >1, buffer that many
            bytes, if None use default from paramiko.
        """
        if kwargs.get("autocommit", True) is False:
            # writes to temporary file, move on commit
            path2 = "{}/{}".format(self.temppath, uuid.uuid4())
            f = self.ftp.open(path2, mode, bufsize=block_size if block_size else -1)
            f.temppath = path2
            f.targetpath = path
            f.fs = self
            f.commit = types.MethodType(commit_a_file, f)
            f.discard = types.MethodType(discard_a_file, f)
        else:
            f = self.ftp.open(path, mode, bufsize=block_size if block_size else -1)
        return f

    def _rm(self, path):
        if self.isdir(path):
            self.ftp.rmdir(path)
        else:
            self.ftp.remove(path)

    def mv(self, old, new):
        self.ftp.posix_rename(old, new)


def commit_a_file(self):
    self.fs.mv(self.temppath, self.targetpath)


def discard_a_file(self):
    self.fs._rm(self.temppath)
