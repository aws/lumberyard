# -*- coding: utf-8 -*-
import io
import logging
import os
import re
import socket
from hashlib import md5

import boto3
import boto3.compat
from botocore.client import Config
from botocore.exceptions import ClientError, ParamValidationError

from s3fs.utils import ParamKwargsHelper
from .utils import read_block, raises, ensure_writable

logger = logging.getLogger(__name__)

logging.getLogger('boto3').setLevel(logging.WARNING)
logging.getLogger('botocore').setLevel(logging.WARNING)

try:
    from boto3.s3.transfer import S3_RETRYABLE_ERRORS
except ImportError:
    S3_RETRYABLE_ERRORS = (
        socket.timeout,
    )

try:
    FileNotFoundError
except NameError:
    class FileNotFoundError(IOError):
        pass


def tokenize(*args, **kwargs):
    """ Deterministic token

    >>> tokenize('Hello') == tokenize('Hello')
    True
    """
    if kwargs:
        args = args + (kwargs,)
    return md5(str(tuple(args)).encode()).hexdigest()


def split_path(path):
    """
    Normalise S3 path string into bucket and key.

    Parameters
    ----------
    path : string
        Input path, like `s3://mybucket/path/to/file`

    Examples
    --------
    >>> split_path("s3://mybucket/path/to/file")
    ['mybucket', 'path/to/file']
    """
    if path.startswith('s3://'):
        path = path[5:]
    if '/' not in path:
        return path, ""
    else:
        return path.split('/', 1)

key_acls = {'private', 'public-read', 'public-read-write',
            'authenticated-read', 'aws-exec-read', 'bucket-owner-read',
            'bucket-owner-full-control'}
buck_acls = {'private', 'public-read', 'public-read-write',
             'authenticated-read'}


class S3FileSystem(object):
    """
    Access S3 as if it were a file system.

    This exposes a filesystem-like API (ls, cp, open, etc.) on top of S3
    storage.

    Provide credentials either explicitly (``key=``, ``secret=``) or depend
    on boto's credential methods. See boto3 documentation for more
    information. If no credentials are available, use ``anon=True``.

    Parameters
    ----------
    anon : bool (False)
        Whether to use anonymous connection (public buckets only). If False,
        uses the key/secret given, or boto's credential resolver (environment
        variables, config files, EC2 IAM server, in that order)
    key : string (None)
        If not anonymous, use this access key ID, if specified
    secret : string (None)
        If not anonymous, use this secret access key, if specified
    token : string (None)
        If not anonymous, use this security token, if specified
    use_ssl : bool (True)
        Whether to use SSL in connections to S3; may be faster without, but
        insecure
    s3_additional_kwargs : dict of parameters that are used when calling s3 api methods.
           Typically used for things like "ServerSideEncryption".
    client_kwargs : dict of parameters for the boto3 client
    requester_pays : bool (False)
        If RequesterPays buckets are supported.
    default_block_size: None, int
        If given, the default block size value used for ``open()``, if no
        specific value is given at all time. The built-in default is 5MB.
    default_fill_cache : Bool (True)
        Whether to use cache filling with open by default. Refer to
        ``S3File.open``.
    config_kwargs : dict of parameters passed to ``botocore.client.Config``
    kwargs : other parameters for boto3 session

    Examples
    --------
    >>> s3 = S3FileSystem(anon=False)  # doctest: +SKIP
    >>> s3.ls('my-bucket/')  # doctest: +SKIP
    ['my-file.txt']

    >>> with s3.open('my-bucket/my-file.txt', mode='rb') as f:  # doctest: +SKIP
    ...     print(f.read())  # doctest: +SKIP
    b'Hello, world!'
    """
    _conn = {}
    _singleton = [None]
    connect_timeout = 5
    read_timeout = 15
    default_block_size = 5 * 2**20

    def __init__(self, anon=False, key=None, secret=None, token=None,
                 use_ssl=True, client_kwargs=None, requester_pays=False,
                 default_block_size=None, default_fill_cache=True,
                 config_kwargs=None, s3_additional_kwargs=None, **kwargs):
        self.anon = anon
        self.session = None
        self.key = key
        self.secret = secret
        self.token = token
        self.kwargs = kwargs

        if client_kwargs is None:
            client_kwargs = {}
        if default_block_size is not None:
            self.default_block_size = default_block_size
        if config_kwargs is None:
            config_kwargs = {}
        self.default_fill_cache = default_fill_cache
        self.client_kwargs = client_kwargs
        self.config_kwargs = config_kwargs
        self.dirs = {}
        self.req_kw = {'RequestPayer': 'requester'} if requester_pays else {}
        self.s3_additional_kwargs = s3_additional_kwargs or {}
        self.use_ssl = use_ssl
        self.s3 = self.connect()
        self._kwargs_helper = ParamKwargsHelper(self.s3)
        self._singleton[0] = self

    def _filter_kwargs(self, s3_method, kwargs):
        return self._kwargs_helper.filter_dict(s3_method.__name__, kwargs)

    def _call_s3(self, method, *akwarglist, **kwargs):
        additional_kwargs = self.s3_additional_kwargs.copy()
        for akwargs in akwarglist:
            additional_kwargs.update(akwargs)
        # Add the normal kwargs in
        additional_kwargs.update(kwargs)
        # filter all kwargs
        additional_kwargs = self._filter_kwargs(method, additional_kwargs)
        return method(**additional_kwargs)

    @classmethod
    def current(cls):
        """ Return the most recently created S3FileSystem

        If no S3FileSystem has been created, then create one
        """
        if not cls._singleton[0]:
            return cls()
        else:
            return cls._singleton[0]

    def connect(self, refresh=False):
        """
        Establish S3 connection object.

        Parameters
        ----------
        refresh : bool (True)
            Whether to use cached filelists, if already read
        """
        anon, key, secret, kwargs, ckwargs, token, ssl = (
              self.anon, self.key, self.secret, self.kwargs,
              self.client_kwargs, self.token, self.use_ssl)

        # Include the current PID in the connection key so that different
        # SSL connections are made for each process.
        tok = tokenize(anon, key, secret, kwargs, ckwargs, token,
                       ssl, os.getpid())
        if refresh:
            self._conn.pop(tok, None)
        if tok not in self._conn:
            logger.debug("Open S3 connection.  Anonymous: %s", self.anon)
            if self.anon:
                from botocore import UNSIGNED
                conf = Config(connect_timeout=self.connect_timeout,
                              read_timeout=self.read_timeout,
                              signature_version=UNSIGNED, **self.config_kwargs)
                self.session = boto3.Session(**self.kwargs)
            else:
                conf = Config(connect_timeout=self.connect_timeout,
                              read_timeout=self.read_timeout,
                              **self.config_kwargs)
                self.session = boto3.Session(self.key, self.secret, self.token,
                                             **self.kwargs)
            s3 = self.session.client('s3', config=conf, use_ssl=ssl,
                                     **self.client_kwargs)
            self._conn[tok] = (s3, self.session)
        else:
            s3, session = self._conn[tok]
            self.session = session
        return s3

    def get_delegated_s3pars(self, exp=3600):
        """Get temporary credentials from STS, appropriate for sending across a
        network. Only relevant where the key/secret were explicitly provided.

        Parameters
        ----------
        exp : int
            Time in seconds that credentials are good for

        Returns
        -------
        dict of parameters
        """
        if self.anon:
            return {'anon': True}
        if self.token:  # already has temporary cred
            return {'key': self.key, 'secret': self.secret, 'token': self.token,
                    'anon': False}
        if self.key is None or self.secret is None:  # automatic credentials
            return {'anon': False}
        sts = self.session.client('sts')
        cred = sts.get_session_token(DurationSeconds=exp)['Credentials']
        return {'key': cred['AccessKeyId'], 'secret': cred['SecretAccessKey'],
                'token': cred['SessionToken'], 'anon': False}

    def __getstate__(self):
        d = self.__dict__.copy()
        del d['s3']
        del d['session']
        del d['_kwargs_helper']
        logger.debug("Serialize with state: %s", d)
        return d

    def __setstate__(self, state):
        self.__dict__.update(state)
        self._conn = {}
        self.s3 = self.connect()
        self._kwargs_helper = ParamKwargsHelper(self.s3)

    def open(self, path, mode='rb', block_size=None, acl='',
             fill_cache=None, **kwargs):
        """ Open a file for reading or writing

        Parameters
        ----------
        path: string
            Path of file on S3
        mode: string
            One of 'rb' or 'wb'
        block_size: int
            Size of data-node blocks if reading
        fill_cache: bool
            If seeking to new a part of the file beyond the current buffer,
            with this True, the buffer will be filled between the sections to
            best support random access. When reading only a few specific chunks
            out of a file, performance may be better if False.
        acl: str
            Canned ACL to set when writing
        kwargs: dict-like
            Additional parameters used for s3 methods.  Typically used for ServerSideEncryption.
        """
        if block_size is None:
            block_size = self.default_block_size
        if fill_cache is None:
            fill_cache = self.default_fill_cache
        if 'b' not in mode:
            raise NotImplementedError("Text mode not supported, use mode='%s'"
                                      " and manage bytes" % (mode[0] + 'b'))
        return S3File(self, path, mode, block_size=block_size, acl=acl,
                      fill_cache=fill_cache, s3_additional_kwargs=kwargs)

    def _lsdir(self, path, refresh=False):
        if path.startswith('s3://'):
            path = path[len('s3://'):]
        path = path.rstrip('/')
        bucket, prefix = split_path(path)
        prefix = prefix + '/' if prefix else ""
        if path not in self.dirs or refresh:
            try:
                pag = self.s3.get_paginator('list_objects_v2')
                it = pag.paginate(Bucket=bucket, Prefix=prefix, Delimiter='/',
                                  **self.req_kw)
                files = []
                dirs = None
                for i in it:
                    dirs = dirs or i.get('CommonPrefixes', None)
                    files.extend(i.get('Contents', []))
                if dirs:
                    files.extend([{'Key': l['Prefix'][:-1], 'Size': 0,
                                  'StorageClass': "DIRECTORY"} for l in dirs])
                files = [f for f in files if len(f['Key']) > len(prefix)]
                for f in files:
                    f['Key'] = '/'.join([bucket, f['Key']])
            except ClientError:
                # path not accessible
                files = []
            self.dirs[path] = files
        return self.dirs[path]

    def _lsbuckets(self, refresh=False):
        if '' not in self.dirs or refresh:
            if self.anon:
                # cannot list buckets if not logged in
                return []
            files = self.s3.list_buckets()['Buckets']
            for f in files:
                f['Key'] = f['Name']
                f['Size'] = 0
                f['StorageClass'] = 'BUCKET'
                del f['Name']
            self.dirs[''] = files
        return self.dirs['']

    def _ls(self, path, refresh=False):
        """ List files in given bucket, or list of buckets.

        Listing is cached unless `refresh=True`.

        Note: only your buckets associated with the login will be listed by
        `ls('')`, not any public buckets (even if already accessed).

        Parameters
        ----------
        path : string/bytes
            location at which to list files
        detail : bool (=True)
            if True, each list item is a dict of file properties;
            otherwise, returns list of filenames
        refresh : bool (=False)
            if False, look in local cache for file details first
        kwargs : dict
            additional arguments passed on
        """
        if path.startswith('s3://'):
            path = path[len('s3://'):]
        if path in ['', '/']:
            return self._lsbuckets(refresh)
        else:
            return self._lsdir(path, refresh)

    def ls(self, path, detail=False, refresh=False, **kwargs):
        """ List single "directory" with or without details """
        if path.startswith('s3://'):
            path = path[len('s3://'):]
        path = path.rstrip('/')
        files = self._ls(path, refresh=refresh)
        if not files:
            if split_path(path)[1]:
                files = [self.info(path, **kwargs)]
            elif path:
                raise FileNotFoundError(path)
        if detail:
            return files
        else:
            return [f['Key'] for f in files]

    def info(self, path, refresh=False, **kwargs):
        """ Detail on the specific file pointed to by path.

        Gets details only for a specific key, directories/buckets cannot be
        used with info.
        """
        parent = path.rsplit('/', 1)[0]
        files = self._lsdir(parent, refresh=refresh)
        files = [f for f in files if f['Key'] == path and f['StorageClass'] not
                 in ['DIRECTORY', 'BUCKET']]
        if len(files) == 1:
            return files[0]
        else:
            try:
                bucket, key = split_path(path)
                out = self._call_s3(self.s3.head_object,
                                    kwargs, Bucket=bucket, Key=key, **self.req_kw)
                out = {'ETag': out['ETag'], 'Key': '/'.join([bucket, key]),
                       'LastModified': out['LastModified'],
                       'Size': out['ContentLength'], 'StorageClass': "STANDARD"}
                return out
            except (ClientError, ParamValidationError):
                raise FileNotFoundError(path)

    _metadata_cache = {}

    def metadata(self, path, refresh=False, **kwargs):
        """ Return metadata of path.

        Metadata is cached unless `refresh=True`.

        Parameters
        ----------
        path : string/bytes
            filename to get metadata for
        refresh : bool (=False)
            if False, look in local cache for file metadata first
        """
        bucket, key = split_path(path)

        if refresh or path not in self._metadata_cache:
            response = self._call_s3(self.s3.head_object,
                                     kwargs,
                                     Bucket=bucket,
                                     Key=key,
                                     **self.req_kw)
            self._metadata_cache[path] = response['Metadata']

        return self._metadata_cache[path]

    def getxattr(self, path, attr_name, **kwargs):
        """ Get an attribute from the metadata.

        Examples
        --------
        >>> mys3fs.getxattr('mykey', 'attribute_1')  # doctest: +SKIP
        'value_1'
        """
        xattr = self.metadata(path, **kwargs)
        if attr_name in xattr:
            return xattr[attr_name]
        return None

    def setxattr(self, path, copy_kwargs=None, **kw_args):
        """ Set metadata.

        Attributes have to be of the form documented in the `Metadata Reference`_.

        Parameters
        ---------
        kw_args : key-value pairs like field="value", where the values must be strings. Does not alter existing fields,
            unless the field appears here - if the value is None, delete the field.
        copy_args : dict, optional
            dictionary of additional params to use for the underlying s3.copy_object.

        Examples
        --------
        >>> mys3file.setxattr(attribute_1='value1', attribute_2='value2')  # doctest: +SKIP
        # Example for use with copy_args
        >>> mys3file.setxattr(copy_kwargs={'ContentType': 'application/pdf'}, attribute_1='value1')  # doctest: +SKIP


        .. Metadata Reference:
        http://docs.aws.amazon.com/AmazonS3/latest/dev/UsingMetadata.html#object-metadata
        """

        bucket, key = split_path(path)
        metadata = self.metadata(path)
        metadata.update(**kw_args)
        copy_kwargs = copy_kwargs or {}

        # remove all keys that are None
        for kw_key in kw_args:
            if kw_args[kw_key] is None:
                metadata.pop(kw_key, None)

        self._call_s3(
            self.s3.copy_object,
            copy_kwargs,
            CopySource="{}/{}".format(bucket, key),
            Bucket=bucket,
            Key=key,
            Metadata=metadata,
            MetadataDirective='REPLACE',
            )

        # refresh metadata
        self._metadata_cache[path] = metadata

    def _walk(self, path, refresh=False):
        if path.startswith('s3://'):
            path = path[len('s3://'):]
        if path in ['', '/']:
            raise ValueError('Cannot walk all of S3')
        filenames = self._ls(path, refresh=refresh)[:]
        for f in filenames[:]:
            if f['StorageClass'] == 'DIRECTORY':
                filenames.extend(self._walk(f['Key'], refresh))
        return [f for f in filenames if f['StorageClass'] not in
                ['BUCKET', 'DIRECTORY']]

    def walk(self, path, refresh=False):
        """ Return all real keys below path """
        return [f['Key'] for f in self._walk(path, refresh)]

    def chmod(self, path, acl, **kwargs):
        """ Set Access Control on a bucket/key

        See http://docs.aws.amazon.com/AmazonS3/latest/dev/acl-overview.html#canned-acl

        Parameters
        ----------
        path : string
            the object to set
        acl : string
            the value of ACL to apply
        """
        bucket, key = split_path(path)
        if key:
            if acl not in key_acls:
                raise ValueError('ACL not in %s', key_acls)
            self._call_s3(self.s3.put_object_acl,
                          kwargs, Bucket=bucket, Key=key, ACL=acl)
        else:
            if acl not in buck_acls:
                raise ValueError('ACL not in %s', buck_acls)
            self._call_s3(self.s3.put_bucket_acl,
                          kwargs, Bucket=bucket, ACL=acl)

    def glob(self, path):
        """
        Find files by glob-matching.

        Note that the bucket part of the path must not contain a "*"
        """
        path0 = path
        if path.startswith('s3://'):
            path = path[len('s3://'):]
        path = path.rstrip('/')
        bucket, key = split_path(path)
        if "*" in bucket:
            raise ValueError('Bucket cannot contain a "*"')
        if '*' not in path:
            path = path.rstrip('/') + '/*'
        if '/' in path[:path.index('*')]:
            ind = path[:path.index('*')].rindex('/')
            root = path[:ind + 1]
        else:
            root = '/'
        allfiles = self.walk(root)
        pattern = re.compile("^" + path.replace('//', '/')
                             .rstrip('/')
                             .replace('*', '[^/]*')
                             .replace('?', '.') + "$")
        out = [f for f in allfiles if re.match(pattern,
               f.replace('//', '/').rstrip('/'))]
        if not out:
            out = self.ls(path0)
        return out

    def du(self, path, total=False, deep=False, **kwargs):
        """ Bytes in keys at path """
        if deep:
            files = self.walk(path)
            files = [self.info(f, **kwargs) for f in files]
        else:
            files = self.ls(path, detail=True)
        if total:
            return sum(f.get('Size', 0) for f in files)
        else:
            return {p['Key']: p['Size'] for p in files}

    def exists(self, path):
        """ Does such a file/directory exist? """
        bucket, key = split_path(path)
        if key or bucket not in self.ls(''):
            return not raises(FileNotFoundError, lambda: self.ls(path))
        else:
            return True

    def cat(self, path, **kwargs):
        """ Returns contents of file """
        with self.open(path, 'rb', **kwargs) as f:
            return f.read()

    def tail(self, path, size=1024, **kwargs):
        """ Return last bytes of file """
        length = self.info(path, **kwargs)['Size']
        if size > length:
            return self.cat(path, **kwargs)
        with self.open(path, 'rb', **kwargs) as f:
            f.seek(length - size)
            return f.read(size)

    def head(self, path, size=1024, **kwargs):
        """ Return first bytes of file """
        with self.open(path, 'rb', block_size=size, **kwargs) as f:
            return f.read(size)

    def url(self, path, expires=3600, **kwargs):
        """ Generate presigned URL to access path by HTTP

        Parameters
        ----------
        path : string
            the key path we are interested in
        expires : int
            the number of seconds this signature will be good for.
        """
        bucket, key = split_path(path)
        return self.s3.generate_presigned_url(ClientMethod='get_object',
                                       Params=dict(Bucket=bucket, Key=key, **kwargs),
                                       ExpiresIn=expires)

    def get(self, path, filename, **kwargs):
        """ Stream data from file at path to local filename """
        with self.open(path, 'rb', **kwargs) as f:
            with open(filename, 'wb') as f2:
                while True:
                    data = f.read(f.blocksize)
                    if len(data) == 0:
                        break
                    f2.write(data)

    def put(self, filename, path, **kwargs):
        """ Stream data from local filename to file at path """
        with open(filename, 'rb') as f:
            with self.open(path, 'wb', **kwargs) as f2:
                while True:
                    data = f.read(f2.blocksize)
                    if len(data) == 0:
                        break
                    f2.write(data)

    def mkdir(self, path, acl="", **kwargs):
        """ Make new bucket or empty key """
        self.touch(path, acl=acl, **kwargs)

    def rmdir(self, path, **kwargs):
        """ Remove empty key or bucket """
        bucket, key = split_path(path)
        if (key and self.info(path)['Size'] == 0) or not key:
            self.rm(path, **kwargs)
        else:
            raise IOError('Path is not directory-like', path)

    def mv(self, path1, path2, **kwargs):
        """ Move file between locations on S3 """
        self.copy(path1, path2, **kwargs)
        self.rm(path1)

    def merge(self, path, filelist, **kwargs):
        """ Create single S3 file from list of S3 files

        Uses multi-part, no data is downloaded. The original files are
        not deleted.

        Parameters
        ----------
        path : str
            The final file to produce
        filelist : list of str
            The paths, in order, to assemble into the final file.
        """
        bucket, key = split_path(path)
        mpu = self._call_s3(
            self.s3.create_multipart_upload,
            kwargs,
            Bucket=bucket,
            Key=key
            )
        out = [self._call_s3(
                    self.s3.upload_part_copy,
                    kwargs,
                    Bucket=bucket, Key=key, UploadId=mpu['UploadId'],
                    CopySource=f, PartNumber=i+1) for (i, f) in enumerate(filelist)]
        parts = [{'PartNumber': i+1, 'ETag': o['CopyPartResult']['ETag']} for (i, o) in enumerate(out)]
        part_info = {'Parts': parts}
        self.s3.complete_multipart_upload(Bucket=bucket, Key=key,
                    UploadId=mpu['UploadId'], MultipartUpload=part_info)
        self.invalidate_cache(path)

    def copy(self, path1, path2, **kwargs):
        """ Copy file between locations on S3 """
        buc1, key1 = split_path(path1)
        buc2, key2 = split_path(path2)
        try:
            self._call_s3(
                self.s3.copy_object,
                kwargs,
                Bucket=buc2, Key=key2, CopySource='/'.join([buc1, key1])
                )
        except (ClientError, ParamValidationError):
            raise IOError('Copy failed', (path1, path2))
        self.invalidate_cache(path2)

    def bulk_delete(self, pathlist, **kwargs):
        """
        Remove multiple keys with one call

        Parameters
        ----------
        pathlist : listof strings
            The keys to remove, must all be in the same bucket.
        """
        if not pathlist:
            return
        buckets = {split_path(path)[0] for path in pathlist}
        if len(buckets) > 1:
            raise ValueError("Bulk delete files should refer to only one bucket")
        bucket = buckets.pop()
        if len(pathlist) > 1000:
            for i in range((len(pathlist) // 1000) + 1):
                self.bulk_delete(pathlist[i*1000:(i+1)*1000])
            return
        delete_keys = {'Objects': [{'Key' : split_path(path)[1]} for path
                                   in pathlist]}
        try:
            self._call_s3(
                self.s3.delete_objects,
                kwargs,
                Bucket=bucket, Delete=delete_keys)
            for path in pathlist:
                self.invalidate_cache(path)
        except ClientError:
            raise IOError('Bulk delete failed')

    def rm(self, path, recursive=False, **kwargs):
        """
        Remove keys and/or bucket.

        Parameters
        ----------
        path : string
            The location to remove.
        recursive : bool (True)
            Whether to remove also all entries below, i.e., which are returned
            by `walk()`.
        """
        if not self.exists(path):
            raise FileNotFoundError(path)
        if recursive:
            self.invalidate_cache(path)
            self.bulk_delete(self.walk(path), **kwargs)
            if not self.exists(path):
                return
        bucket, key = split_path(path)
        if key:
            try:
                self._call_s3(
                    self.s3.delete_object, kwargs, Bucket=bucket, Key=key)
            except ClientError:
                raise IOError('Delete key failed', (bucket, key))
            self.invalidate_cache(path)
        else:
            if not self.s3.list_objects(Bucket=bucket).get('Contents'):
                try:
                    self.s3.delete_bucket(Bucket=bucket)
                except ClientError:
                    raise IOError('Delete bucket failed', bucket)
                self.invalidate_cache(bucket)
                self.invalidate_cache('')
            else:
                raise IOError('Not empty', path)

    def invalidate_cache(self, path=None):
        if path is None:
            self.dirs.clear()
        else:
            self.dirs.pop(path, None)
            parent = path.rsplit('/', 1)[0]
            self.dirs.pop(parent, None)

    def touch(self, path, acl="", **kwargs):
        """
        Create empty key

        If path is a bucket only, attempt to create bucket.
        """
        bucket, key = split_path(path)
        if key:
            if acl and acl not in key_acls:
                raise ValueError('ACL not in %s', key_acls)
            self._call_s3(
                self.s3.put_object, kwargs,
                Bucket=bucket, Key=key, ACL=acl)
            self.invalidate_cache(path)
        else:
            if acl and acl not in buck_acls:
                raise ValueError('ACL not in %s', buck_acls)
            try:
                self.s3.create_bucket(Bucket=bucket, ACL=acl)
                self.invalidate_cache('')
                self.invalidate_cache(bucket)
            except (ClientError, ParamValidationError):
                raise IOError('Bucket create failed', path)

    def read_block(self, fn, offset, length, delimiter=None, **kwargs):
        """ Read a block of bytes from an S3 file

        Starting at ``offset`` of the file, read ``length`` bytes.  If
        ``delimiter`` is set then we ensure that the read starts and stops at
        delimiter boundaries that follow the locations ``offset`` and ``offset
        + length``.  If ``offset`` is zero then we start at zero.  The
        bytestring returned WILL include the end delimiter string.

        If offset+length is beyond the eof, reads to eof.

        Parameters
        ----------
        fn: string
            Path to filename on S3
        offset: int
            Byte offset to start read
        length: int
            Number of bytes to read
        delimiter: bytes (optional)
            Ensure reading starts and stops at delimiter bytestring

        Examples
        --------
        >>> s3.read_block('data/file.csv', 0, 13)  # doctest: +SKIP
        b'Alice, 100\\nBo'
        >>> s3.read_block('data/file.csv', 0, 13, delimiter=b'\\n')  # doctest: +SKIP
        b'Alice, 100\\nBob, 200\\n'

        Use ``length=None`` to read to the end of the file.
        >>> s3.read_block('data/file.csv', 0, None, delimiter=b'\\n')  # doctest: +SKIP
        b'Alice, 100\\nBob, 200\\nCharlie, 300'

        See Also
        --------
        distributed.utils.read_block
        """
        with self.open(fn, 'rb', **kwargs) as f:
            size = f.info()['Size']
            if length is None:
                length = size
            if offset + length > size:
                length = size - offset
            bytes = read_block(f, offset, length, delimiter)
        return bytes


class S3File(object):
    """
    Open S3 key as a file. Data is only loaded and cached on demand.

    Parameters
    ----------
    s3 : boto3 connection
    path : string
        S3 bucket/key to access
    block_size : int
        read-ahead size for finding delimiters
    fill_cache: bool
        If seeking to new a part of the file beyond the current buffer,
        with this True, the buffer will be filled between the sections to
        best support random access. When reading only a few specific chunks
        out of a file, performance may be better if False.
    acl: str
        Canned ACL to apply

    Examples
    --------
    >>> s3 = S3FileSystem()  # doctest: +SKIP
    >>> with s3.open('my-bucket/my-file.txt', mode='rb') as f:  # doctest: +SKIP
    ...     ...  # doctest: +SKIP

    See Also
    --------
    S3FileSystem.open: used to create ``S3File`` objects
    """

    def __init__(self, s3, path, mode='rb', block_size=5 * 2 ** 20, acl="",
                 fill_cache=True, s3_additional_kwargs=None):
        self.mode = mode
        if mode not in {'rb', 'wb', 'ab'}:
            raise NotImplementedError("File mode must be {'rb', 'wb', 'ab'}, not %s" % mode)
        if path.startswith('s3://'):
            path = path[len('s3://'):]
        self.path = path
        bucket, key = split_path(path)
        self.s3 = s3
        self.bucket = bucket
        self.key = key
        self.blocksize = block_size
        self.cache = b""
        self.loc = 0
        self.start = None
        self.end = None
        self.closed = False
        self.trim = True
        self.mpu = None
        self.acl = acl
        self.fill_cache = fill_cache
        self.s3_additional_kwargs = s3_additional_kwargs or {}
        if mode in {'wb', 'ab'}:
            if acl and acl not in key_acls:
                raise ValueError('ACL not in %s', key_acls)
            self.buffer = io.BytesIO()
            self.parts = []
            self.size = 0
            if block_size < 5 * 2 ** 20:
                raise ValueError('Block size must be >=5MB')
            self.forced = False
            if mode == 'ab' and s3.exists(path):
                self.size = s3.info(path)['Size']
                if self.size < 5*2**20:
                    # existing file too small for multi-upload: download
                    self.write(s3.cat(path))
                else:
                    try:
                        self.mpu = self.s3._call_s3(
                            self.s3.s3.create_multipart_upload,
                            self.s3_additional_kwargs,
                            Bucket=bucket, Key=key, ACL=acl)
                    except (ClientError, ParamValidationError) as e:
                        raise IOError('Open for write failed', path, e)
                    self.loc = self.size
                    out = self.s3._call_s3(
                        self.s3.s3.upload_part_copy,
                        self.s3_additional_kwargs,
                        Bucket=self.bucket,
                        Key=self.key, PartNumber=1,
                        UploadId=self.mpu['UploadId'],
                        CopySource=path)
                    self.parts.append({'PartNumber': 1,
                                       'ETag': out['CopyPartResult']['ETag']})
        else:
            try:
                self.size = self.info()['Size']
            except (ClientError, ParamValidationError):
                raise IOError("File not accessible", path)

    def _call_s3(self, method, *kwarglist, **kwargs):
        return self.s3._call_s3(method, self.s3_additional_kwargs, *kwarglist, **kwargs)

    def info(self, **kwargs):
        """ File information about this path """
        return self.s3.info(self.path, **kwargs)

    def metadata(self, refresh=False, **kwargs):
        """ Return metadata of file.
        See :func:`~s3fs.S3Filesystem.metadata`.

        Metadata is cached unless `refresh=True`.
        """
        return self.s3.metadata(self.path, refresh, **kwargs)

    def getxattr(self, xattr_name, **kwargs):
        """ Get an attribute from the metadata.
        See :func:`~s3fs.S3Filesystem.getxattr`.

        Examples
        --------
        >>> mys3file.getxattr('attribute_1')  # doctest: +SKIP
        'value_1'
        """
        return self.s3.getxattr(self.path, xattr_name, **kwargs)

    def setxattr(self, copy_kwargs=None, **kwargs):
        """ Set metadata.
        See :func:`~s3fs.S3Filesystem.setxattr`.

        Examples
        --------
        >>> mys3file.setxattr(attribute_1='value1', attribute_2='value2')  # doctest: +SKIP
        """
        if self.mode != 'rb':
            raise NotImplementedError('cannot update metadata while file is open for writing')
        return self.s3.setxattr(self.path, copy_kwargs=copy_kwargs, **kwargs)

    def url(self, **kwargs):
        """ HTTP URL to read this file (if it already exists)
        """
        return self.s3.url(self.path, **kwargs)

    def tell(self):
        """ Current file location """
        return self.loc

    def seek(self, loc, whence=0):
        """ Set current file location

        Parameters
        ----------
        loc : int
            byte location
        whence : {0, 1, 2}
            from start of file, current location or end of file, resp.
        """
        if not self.mode == 'rb':
            raise ValueError('Seek only available in read mode')
        if whence == 0:
            nloc = loc
        elif whence == 1:
            nloc = self.loc + loc
        elif whence == 2:
            nloc = self.size + loc
        else:
            raise ValueError(
                "invalid whence (%s, should be 0, 1 or 2)" % whence)
        if nloc < 0:
            raise ValueError('Seek before start of file')
        self.loc = nloc
        return self.loc

    def readline(self, length=-1):
        """
        Read and return a line from the stream.

        If length is specified, at most size bytes will be read.
        """
        self._fetch(self.loc, self.loc + 1)
        while True:
            found = self.cache[self.loc - self.start:].find(b'\n') + 1
            if length > 0 and found > length:
                return self.read(length)
            if found:
                return self.read(found)
            if self.end > self.size:
                return self.read(length)
            self._fetch(self.start, self.end + self.blocksize)

    def __next__(self):
        out = self.readline()
        if not out:
            raise StopIteration
        return out

    next = __next__

    def __iter__(self):
        return self

    def readlines(self):
        """ Return all lines in a file as a list """
        return list(self)

    def _fetch(self, start, end):
        if self.start is None and self.end is None:
            # First read
            self.start = start
            self.end = end + self.blocksize
            self.cache = _fetch_range(self.s3.s3, self.bucket, self.key,
                                      start, self.end, req_kw=self.s3.req_kw)
        if start < self.start:
            if not self.fill_cache and end + self.blocksize < self.start:
                self.start, self.end = None, None
                return self._fetch(start, end)
            new = _fetch_range(self.s3.s3, self.bucket, self.key,
                               start, self.start, req_kw=self.s3.req_kw)
            self.start = start
            self.cache = new + self.cache
        if end > self.end:
            if self.end > self.size:
                return
            if not self.fill_cache and start > self.end:
                self.start, self.end = None, None
                return self._fetch(start, end)
            new = _fetch_range(self.s3.s3, self.bucket, self.key,
                               self.end, end + self.blocksize,
                               req_kw=self.s3.req_kw)
            self.end = end + self.blocksize
            self.cache = self.cache + new

    def read(self, length=-1):
        """
        Return data from cache, or fetch pieces as necessary

        Parameters
        ----------
        length : int (-1)
            Number of bytes to read; if <0, all remaining bytes.
        """
        if self.mode != 'rb':
            raise ValueError('File not in read mode')
        if length < 0:
            length = self.size
        if self.closed:
            raise ValueError('I/O operation on closed file.')
        self._fetch(self.loc, self.loc + length)
        out = self.cache[self.loc - self.start:
                         self.loc - self.start + length]
        self.loc += len(out)
        if self.trim:
            num = (self.loc - self.start) // self.blocksize - 1
            if num > 0:
                self.start += self.blocksize * num
                self.cache = self.cache[self.blocksize * num:]
        return out

    def write(self, data):
        """
        Write data to buffer.

        Buffer only sent to S3 on close() or if buffer is greater than or equal to blocksize.

        Parameters
        ----------
        data : bytes
            Set of bytes to be written.
        """
        if self.mode not in {'wb', 'ab'}:
            raise ValueError('File not in write mode')
        if self.closed:
            raise ValueError('I/O operation on closed file.')
        out = self.buffer.write(ensure_writable(data))
        self.loc += out
        if self.buffer.tell() >= self.blocksize:
            self.flush()
        return out

    def flush(self, force=False, retries=10):
        """
        Write buffered data to S3.

        Uploads the current buffer, if it is larger than the block-size. If
        the buffer is smaller than the block-size, this is a no-op.

        Due to S3 multi-upload policy, you can only safely force flush to S3
        when you are finished writing.

        Parameters
        ----------
        force : bool
            When closing, write the last block even if it is smaller than
            blocks are allowed to be.
        """
        if self.mode in {'wb', 'ab'} and not self.closed:
            if self.buffer.tell() < self.blocksize and not force:
                # ignore if not enough data for a block and not closing
                return
            if self.buffer.tell() == 0:
                # no data in the buffer to write
                return
            if force and self.forced:
                raise ValueError("Force flush cannot be called more than once")
            if force:
                self.forced = True

            self.buffer.seek(0)
            part = len(self.parts) + 1
            i = 0

            try:
                self.mpu = self.mpu or self._call_s3(
                    self.s3.s3.create_multipart_upload,
                    Bucket=self.bucket, Key=self.key, ACL=self.acl)
            except (ClientError, ParamValidationError) as e:
                raise IOError('Initiating write failed: %s' % self.path, e)

            while True:
                try:
                    out = self._call_s3(
                        self.s3.s3.upload_part,
                        Bucket=self.bucket,
                        PartNumber=part, UploadId=self.mpu['UploadId'],
                        Body=self.buffer.read(), Key=self.key)
                    break
                except S3_RETRYABLE_ERRORS:
                    if i < retries:
                        logger.debug('Exception %e on S3 write, retrying',
                                     exc_info=True)
                        i += 1
                        continue
                    else:
                        raise IOError('Write failed after %i retries' % retries,
                                      self)
                except Exception as e:
                    raise IOError('Write failed', self, e)
            self.parts.append({'PartNumber': part, 'ETag': out['ETag']})
            self.buffer = io.BytesIO()

    def close(self):
        """ Close file

        If in write mode, key is only finalized upon close, and key will then
        be available to other processes.
        """
        if self.closed:
            return
        self.cache = None
        if self.mode in {'wb', 'ab'}:
            if self.parts:
                self.flush(force=True)
                part_info = {'Parts': self.parts}
                self._call_s3(
                    self.s3.s3.complete_multipart_upload,
                    Bucket=self.bucket,
                    Key=self.key,
                    UploadId=self.mpu['UploadId'],
                    MultipartUpload=part_info)
            else:
                self.buffer.seek(0)
                try:
                    self._call_s3(
                        self.s3.s3.put_object,
                        Bucket=self.bucket, Key=self.key,
                        Body=self.buffer.read(), ACL=self.acl)
                except (ClientError, ParamValidationError) as e:
                    raise IOError('Write failed: %s' % self.path, e)
            self.s3.invalidate_cache(self.path)
        self.closed = True

    def readable(self):
        """Return whether the S3File was opened for reading"""
        return self.mode == 'rb'

    def seekable(self):
        """Return whether the S3File is seekable (only in read mode)"""
        return self.readable()

    def writable(self):
        """Return whether the S3File was opened for writing"""
        return self.mode in {'wb', 'ab'}

    def __del__(self):
        self.close()

    def __str__(self):
        return "<S3File %s/%s>" % (self.bucket, self.key)

    __repr__ = __str__

    def __enter__(self):
        return self

    def __exit__(self, *args):
        self.close()


def _fetch_range(client, bucket, key, start, end, max_attempts=10,
                 req_kw=None):
    if req_kw is None:
        req_kw = {}
    logger.debug("Fetch: %s/%s, %s-%s", bucket, key, start, end)
    for i in range(max_attempts):
        try:
            resp = client.get_object(Bucket=bucket, Key=key,
                                     Range='bytes=%i-%i' % (start, end - 1),
                                     **req_kw)
            return resp['Body'].read()
        except S3_RETRYABLE_ERRORS as e:
            logger.debug('Exception %e on S3 download, retrying', e,
                         exc_info=True)
            continue
        except ClientError as e:
            if e.response['Error'].get('Code', 'Unknown') in ['416',
                                                              'InvalidRange']:
                return b''
            else:
                raise
        except Exception as e:
            if 'time' in str(e).lower():  # Actual exception type changes often
                continue
            else:
                raise
    raise RuntimeError("Max number of S3 retries exceeded")
