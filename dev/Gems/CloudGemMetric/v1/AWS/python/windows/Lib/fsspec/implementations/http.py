from __future__ import print_function, division, absolute_import

import re
import requests
from urllib.parse import urlparse
from fsspec import AbstractFileSystem
from fsspec.spec import AbstractBufferedFile
from fsspec.utils import tokenize, DEFAULT_BLOCK_SIZE
from ..caching import AllBytes

# https://stackoverflow.com/a/15926317/3821154
ex = re.compile(r"""<a\s+(?:[^>]*?\s+)?href=(["'])(.*?)\1""")
ex2 = re.compile(r"""(http[s]?://[-a-zA-Z0-9@:%_+.~#?&/=]+)""")


class HTTPFileSystem(AbstractFileSystem):
    """
    Simple File-System for fetching data via HTTP(S)

    ``ls()`` is implemented by loading the parent page and doing a regex
    match on the result. If simple_link=True, anything of the form
    "http(s)://server.com/stuff?thing=other"; otherwise only links within
    HTML href tags will be used.
    """

    sep = "/"

    def __init__(
        self,
        simple_links=True,
        block_size=None,
        same_scheme=True,
        size_policy=None,
        **storage_options
    ):
        """
        Parameters
        ----------
        block_size: int
            Blocks to read bytes; if 0, will default to raw requests file-like
            objects instead of HTTPFile instances
        simple_links: bool
            If True, will consider both HTML <a> tags and anything that looks
            like a URL; if False, will consider only the former.
        same_scheme: True
            When doing ls/glob, if this is True, only consider paths that have
            http/https matching the input URLs.
        size_policy: this argument is deprecated
        storage_options: key-value
            May be credentials, e.g., `{'auth': ('username', 'pword')}` or any
            other parameters passed on to requests
        """
        AbstractFileSystem.__init__(self)
        self.block_size = block_size if block_size is not None else DEFAULT_BLOCK_SIZE
        self.simple_links = simple_links
        self.same_schema = same_scheme
        self.kwargs = storage_options
        self.session = requests.Session()

    @classmethod
    def _strip_protocol(cls, path):
        """ For HTTP, we always want to keep the full URL
        """
        return path

    # TODO: override get

    def ls(self, url, detail=True):
        # ignoring URL-encoded arguments
        r = self.session.get(url, **self.kwargs)
        if self.simple_links:
            links = ex2.findall(r.text) + ex.findall(r.text)
        else:
            links = ex.findall(r.text)
        out = set()
        parts = urlparse(url)
        for l in links:
            if isinstance(l, tuple):
                l = l[1]
            if l.startswith("http"):
                if self.same_schema:
                    if l.split(":", 1)[0] == url.split(":", 1)[0]:
                        out.add(l)
                elif l.replace("https", "http").startswith(
                    url.replace("https", "http")
                ):
                    # allowed to cross http <-> https
                    out.add(l)
            elif l.startswith("/") and len(l) > 1:
                out.add(parts.scheme + "://" + parts.netloc + l)
            else:
                if l not in ["..", "../"]:
                    # Ignore FTP-like "parent"
                    out.add("/".join([url.rstrip("/"), l.lstrip("/")]))
        if not out and url.endswith("/"):
            return self.ls(url.rstrip("/"), detail=True)
        if detail:
            return [
                {
                    "name": u,
                    "size": None,
                    "type": "directory" if u.endswith("/") else "file",
                }
                for u in out
            ]
        else:
            return list(sorted(out))

    def cat(self, url):
        r = self.session.get(url, **self.kwargs)
        r.raise_for_status()
        return r.content

    def mkdirs(self, url):
        """Make any intermediate directories to make path writable"""
        raise NotImplementedError

    def exists(self, path):
        kwargs = self.kwargs.copy()
        kwargs["stream"] = True
        try:
            r = self.session.get(path, **kwargs)
            r.close()
            return r.ok
        except requests.HTTPError:
            return False

    def _open(
        self,
        path,
        mode="rb",
        block_size=None,
        autocommit=None,  # XXX: This differs from the base class.
        cache_options=None,
        **kwargs
    ):
        """Make a file-like object

        Parameters
        ----------
        path: str
            Full URL with protocol
        mode: string
            must be "rb"
        block_size: int or None
            Bytes to download in one request; use instance value if None. If
            zero, will return a streaming Requests file-like instance.
        kwargs: key-value
            Any other parameters, passed to requests calls
        """
        if mode != "rb":
            raise NotImplementedError
        block_size = block_size if block_size is not None else self.block_size
        kw = self.kwargs.copy()
        kw.update(kwargs)  # this does nothing?
        if block_size:
            return HTTPFile(
                self,
                path,
                self.session,
                block_size,
                mode=mode,
                cache_options=cache_options,
                **kw
            )
        else:
            kw["stream"] = True
            r = self.session.get(path, **kw)
            r.raise_for_status()
            r.raw.decode_content = True
            return r.raw

    def ukey(self, url):
        """Unique identifier; assume HTTP files are static, unchanging"""
        return tokenize(url, self.kwargs, self.protocol)

    def info(self, url, **kwargs):
        """Get info of URL

        Tries to access location via HEAD, and then GET methods, but does
        not fetch the data.

        It is possible that the server does not supply any size information, in
        which case size will be given as None (and certain operations on the
        corresponding file will not work).
        """
        size = False
        for policy in ["head", "get"]:
            try:
                size = file_size(url, self.session, policy, **self.kwargs)
                if size:
                    break
            except Exception:
                pass
        else:
            # get failed, so conclude URL does not exist
            if size is False:
                raise FileNotFoundError(url)
        return {"name": url, "size": size or None, "type": "file"}


class HTTPFile(AbstractBufferedFile):
    """
    A file-like object pointing to a remove HTTP(S) resource

    Supports only reading, with read-ahead of a predermined block-size.

    In the case that the server does not supply the filesize, only reading of
    the complete file in one go is supported.

    Parameters
    ----------
    url: str
        Full URL of the remote resource, including the protocol
    session: requests.Session or None
        All calls will be made within this session, to avoid restarting
        connections where the server allows this
    block_size: int or None
        The amount of read-ahead to do, in bytes. Default is 5MB, or the value
        configured for the FileSystem creating this file
    size: None or int
        If given, this is the size of the file in bytes, and we don't attempt
        to call the server to find the value.
    kwargs: all other key-values are passed to requests calls.
    """

    def __init__(
        self,
        fs,
        url,
        session=None,
        block_size=None,
        mode="rb",
        cache_type="bytes",
        cache_options=None,
        size=None,
        **kwargs
    ):
        if mode != "rb":
            raise NotImplementedError("File mode not supported")
        self.url = url
        self.session = session if session is not None else requests.Session()
        if size is not None:
            self.details = {"name": url, "size": size, "type": "file"}
        super().__init__(
            fs=fs,
            path=url,
            mode=mode,
            block_size=block_size,
            cache_type=cache_type,
            cache_options=cache_options,
            **kwargs
        )
        self.cache.size = self.size or self.blocksize

    def read(self, length=-1):
        """Read bytes from file

        Parameters
        ----------
        length: int
            Read up to this many bytes. If negative, read all content to end of
            file. If the server has not supplied the filesize, attempting to
            read only part of the data will raise a ValueError.
        """
        if (
            (length < 0 and self.loc == 0)
            or (length > (self.size or length))  # explicit read all
            or (  # read more than there is
                self.size and self.size < self.blocksize
            )  # all fits in one block anyway
        ):
            self._fetch_all()
        if self.size is None:
            if length < 0:
                self._fetch_all()
        else:
            length = min(self.size - self.loc, length)
        return super().read(length)

    def _fetch_all(self):
        """Read whole file in one shot, without caching

        This is only called when position is still at zero,
        and read() is called without a byte-count.
        """
        if not isinstance(self.cache, AllBytes):
            r = self.session.get(self.url, **self.kwargs)
            r.raise_for_status()
            out = r.content
            self.cache = AllBytes(out)
            self.size = len(out)

    def _fetch_range(self, start, end):
        """Download a block of data

        The expectation is that the server returns only the requested bytes,
        with HTTP code 206. If this is not the case, we first check the headers,
        and then stream the output - if the data size is bigger than we
        requested, an exception is raised.
        """
        kwargs = self.kwargs.copy()
        headers = kwargs.pop("headers", {})
        headers["Range"] = "bytes=%i-%i" % (start, end - 1)
        r = self.session.get(self.url, headers=headers, stream=True, **kwargs)
        if r.status_code == 416:
            # range request outside file
            return b""
        r.raise_for_status()
        if r.status_code == 206:
            # partial content, as expected
            out = r.content
        elif "Content-Length" in r.headers:
            cl = int(r.headers["Content-Length"])
            if cl <= end - start:
                # data size OK
                out = r.content
            else:
                raise ValueError(
                    "Got more bytes (%i) than requested (%i)" % (cl, end - start)
                )
        else:
            cl = 0
            out = []
            for chunk in r.iter_content(chunk_size=2 ** 20):
                # data size unknown, let's see if it goes too big
                if chunk:
                    out.append(chunk)
                    cl += len(chunk)
                    if cl > end - start:
                        raise ValueError(
                            "Got more bytes so far (>%i) than requested (%i)"
                            % (cl, end - start)
                        )
                else:
                    break
            out = b"".join(out)
        return out


def file_size(url, session=None, size_policy="head", **kwargs):
    """Call HEAD on the server to get file size

    Default operation is to explicitly allow redirects and use encoding
    'identity' (no compression) to get the true size of the target.
    """
    kwargs = kwargs.copy()
    ar = kwargs.pop("allow_redirects", True)
    head = kwargs.get("headers", {}).copy()
    head["Accept-Encoding"] = "identity"
    session = session or requests.Session()
    if size_policy == "head":
        r = session.head(url, allow_redirects=ar, **kwargs)
    elif size_policy == "get":
        kwargs["stream"] = True
        r = session.get(url, allow_redirects=ar, **kwargs)
    else:
        raise TypeError('size_policy must be "head" or "get", got %s' "" % size_policy)
    if "Content-Length" in r.headers:
        return int(r.headers["Content-Length"])
    elif "Content-Range" in r.headers:
        return int(r.headers["Content-Range"].split("/")[1])
