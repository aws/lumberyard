# -*- coding: utf-8 -*-
import logging
import os
import socket
import time
from typing import Tuple, Optional

from fsspec import AbstractFileSystem
from fsspec.spec import AbstractBufferedFile

from fsspec.utils import infer_storage_options
from fsspec.utils import tokenize

import botocore
import botocore.session
from botocore.client import Config
from botocore.exceptions import ClientError, ParamValidationError, BotoCoreError

from s3fs.errors import translate_boto_error
from s3fs.utils import ParamKwargsHelper

logger = logging.getLogger('s3fs')

if "S3FS_LOGGING_LEVEL" in os.environ:
    handle = logging.StreamHandler()
    formatter = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s '
                                  '- %(message)s')
    handle.setFormatter(formatter)
    logger.addHandler(handle)
    logger.setLevel(os.environ["S3FS_LOGGING_LEVEL"])

S3_RETRYABLE_ERRORS = (
    socket.timeout,
)

_VALID_FILE_MODES = {'r', 'w', 'a', 'rb', 'wb', 'ab'}


key_acls = {'private', 'public-read', 'public-read-write',
            'authenticated-read', 'aws-exec-read', 'bucket-owner-read',
            'bucket-owner-full-control'}
buck_acls = {'private', 'public-read', 'public-read-write',
             'authenticated-read'}


def version_id_kw(version_id):
    """Helper to make versionId kwargs.

    Not all boto3 methods accept a None / empty versionId so dictionary expansion solves
    that problem.
    """
    if version_id:
        return {'VersionId': version_id}
    else:
        return {}


def _coalesce_version_id(*args):
    """Helper to coalesce a list of version_ids down to one"""
    version_ids = set(args)
    if None in version_ids:
        version_ids.remove(None)
    if len(version_ids) > 1:
        raise ValueError(
            "Cannot coalesce version_ids where more than one are defined,"
            " {}".format(version_ids))
    elif len(version_ids) == 0:
        return None
    else:
        return version_ids.pop()


class S3FileSystem(AbstractFileSystem):
    """
    Access S3 as if it were a file system.

    This exposes a filesystem-like API (ls, cp, open, etc.) on top of S3
    storage.

    Provide credentials either explicitly (``key=``, ``secret=``) or depend
    on boto's credential methods. See botocore documentation for more
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
    s3_additional_kwargs : dict of parameters that are used when calling s3 api
        methods. Typically used for things like "ServerSideEncryption".
    client_kwargs : dict of parameters for the botocore client
    requester_pays : bool (False)
        If RequesterPays buckets are supported.
    default_block_size: int (None)
        If given, the default block size value used for ``open()``, if no
        specific value is given at all time. The built-in default is 5MB.
    default_fill_cache : Bool (True)
        Whether to use cache filling with open by default. Refer to
        ``S3File.open``.
    default_cache_type : string ('bytes')
        If given, the default cache_type value used for ``open()``. Set to "none"
        if no caching is desired. See fsspec's documentation for other available
        cache_type values. Default cache_type is 'bytes'.
    version_aware : bool (False)
        Whether to support bucket versioning.  If enable this will require the
        user to have the necessary IAM permissions for dealing with versioned
        objects.
    config_kwargs : dict of parameters passed to ``botocore.client.Config``
    kwargs : other parameters for core session
    session : botocore Session object to be used for all connections.
         This session will be used inplace of creating a new session inside S3FileSystem.

    The following parameters are passed on to fsspec:

    skip_instance_cache: to control reuse of instances
    use_listings_cache, listings_expiry_time, max_paths: to control reuse of directory listings

    Examples
    --------
    >>> s3 = S3FileSystem(anon=False)  # doctest: +SKIP
    >>> s3.ls('my-bucket/')  # doctest: +SKIP
    ['my-file.txt']

    >>> with s3.open('my-bucket/my-file.txt', mode='rb') as f:  # doctest: +SKIP
    ...     print(f.read())  # doctest: +SKIP
    b'Hello, world!'
    """
    root_marker = ""
    connect_timeout = 5
    retries = 5
    read_timeout = 15
    default_block_size = 5 * 2**20
    protocol = ['s3', 's3a']
    _extra_tokenize_attributes = ('default_block_size',)

    def __init__(self, anon=False, key=None, secret=None, token=None,
                 use_ssl=True, client_kwargs=None, requester_pays=False,
                 default_block_size=None, default_fill_cache=True,
                 default_cache_type='bytes', version_aware=False, config_kwargs=None,
                 s3_additional_kwargs=None, session=None, username=None,
                 password=None, **kwargs):
        if key and username:
            raise KeyError('Supply either key or username, not both')
        if secret and password:
            raise KeyError('Supply secret or password, not both')
        if username:
            key = username
        if password:
            secret = password

        self.anon = anon
        self.session = None
        self.passed_in_session = session
        if self.passed_in_session:
            self.session = self.passed_in_session
        self.key = key
        self.secret = secret
        self.token = token
        self.kwargs = kwargs
        super_kwargs = {k: kwargs.pop(k)
                        for k in ['use_listings_cache', 'listings_expiry_time', 'max_paths']
                        if k in kwargs}  # passed to fsspec superclass

        if client_kwargs is None:
            client_kwargs = {}
        if config_kwargs is None:
            config_kwargs = {}
        self.default_block_size = default_block_size or self.default_block_size
        self.default_fill_cache = default_fill_cache
        self.default_cache_type = default_cache_type
        self.version_aware = version_aware
        self.client_kwargs = client_kwargs
        self.config_kwargs = config_kwargs
        self.req_kw = {'RequestPayer': 'requester'} if requester_pays else {}
        self.s3_additional_kwargs = s3_additional_kwargs or {}
        self.use_ssl = use_ssl
        self.s3 = self.connect()
        self._kwargs_helper = ParamKwargsHelper(self.s3)
        super().__init__(**super_kwargs)

    def _filter_kwargs(self, s3_method, kwargs):
        return self._kwargs_helper.filter_dict(s3_method.__name__, kwargs)

    def _call_s3(self, method, *akwarglist, **kwargs):
        kw2 = kwargs.copy()
        kw2.pop('Body', None)
        logger.debug("CALL: %s - %s - %s" % (method.__name__, akwarglist, kw2))
        additional_kwargs = self._get_s3_method_kwargs(method, *akwarglist,
                                                       **kwargs)
        return method(**additional_kwargs)

    def _get_s3_method_kwargs(self, method, *akwarglist, **kwargs):
        additional_kwargs = self.s3_additional_kwargs.copy()
        for akwargs in akwarglist:
            additional_kwargs.update(akwargs)
        # Add the normal kwargs in
        additional_kwargs.update(kwargs)
        # filter all kwargs
        return self._filter_kwargs(method, additional_kwargs)

    @staticmethod
    def _get_kwargs_from_urls(urlpath):
        """
        When we have a urlpath that contains a ?versionId= query assume that we want to use version_aware mode for
        the filesystem.
        """
        url_storage_opts = infer_storage_options(urlpath)
        url_query = url_storage_opts.get("url_query")
        out = {}
        if url_query is not None:
            from urllib.parse import parse_qs
            parsed = parse_qs(url_query)
            if 'versionId' in parsed:
                out['version_aware'] = True
        return out

    def split_path(self, path) -> Tuple[str, str, Optional[str]]:
        """
        Normalise S3 path string into bucket and key.

        Parameters
        ----------
        path : string
            Input path, like `s3://mybucket/path/to/file`

        Examples
        --------
        >>> split_path("s3://mybucket/path/to/file")
        ['mybucket', 'path/to/file', None]

        >>> split_path("s3://mybucket/path/to/versioned_file?versionId=some_version_id")
        ['mybucket', 'path/to/versioned_file', 'some_version_id']
        """
        path = self._strip_protocol(path)
        path = path.lstrip('/')
        if '/' not in path:
            return path, "", None
        else:
            bucket, keypart = path.split('/', 1)
            key, _, version_id = keypart.partition('?versionId=')
            return bucket, key, version_id if self.version_aware and version_id else None

    def _prepare_config_kwargs(self):
        config_kwargs = self.config_kwargs.copy()
        if "connect_timeout" not in config_kwargs.keys():
            config_kwargs['connect_timeout'] = self.connect_timeout
        if "read_timeout" not in config_kwargs.keys():
            config_kwargs['read_timeout'] = self.read_timeout
        return config_kwargs

    def connect(self, refresh=True):
        """
        Establish S3 connection object.

        Parameters
        ----------
        refresh : bool
            Whether to create new session/client, even if a previous one with
            the same parameters already exists. If False (default), an
            existing one will be used if possible
        """
        if refresh is False:
            # back compat: we store whole FS instance now
            return self.s3
        anon, key, secret, kwargs, ckwargs, token, ssl = (
            self.anon, self.key, self.secret, self.kwargs,
            self.client_kwargs, self.token, self.use_ssl)

        if not self.passed_in_session:
            self.session = botocore.session.Session(**self.kwargs)

        logger.debug("Setting up s3fs instance")

        config_kwargs = self._prepare_config_kwargs()
        if self.anon:
            from botocore import UNSIGNED
            conf = Config(signature_version=UNSIGNED, **config_kwargs)
            self.s3 = self.session.create_client('s3', config=conf, use_ssl=ssl,
                                        **self.client_kwargs)
        else:
            conf = Config(**config_kwargs)
            self.s3 = self.session.create_client('s3', aws_access_key_id=self.key, aws_secret_access_key=self.secret, aws_session_token=self.token, config=conf, use_ssl=ssl,
                                        **self.client_kwargs)
        return self.s3

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
        sts = self.session.create_client('sts')
        cred = sts.get_session_token(DurationSeconds=exp)['Credentials']
        return {'key': cred['AccessKeyId'], 'secret': cred['SecretAccessKey'],
                'token': cred['SessionToken'], 'anon': False}

    def _open(self, path, mode='rb', block_size=None, acl='', version_id=None,
              fill_cache=None, cache_type=None, autocommit=True, requester_pays=None, **kwargs):
        """ Open a file for reading or writing

        Parameters
        ----------
        path: string
            Path of file on S3
        mode: string
            One of 'r', 'w', 'a', 'rb', 'wb', or 'ab'. These have the same meaning
            as they do for the built-in `open` function.
        block_size: int
            Size of data-node blocks if reading
        fill_cache: bool
            If seeking to new a part of the file beyond the current buffer,
            with this True, the buffer will be filled between the sections to
            best support random access. When reading only a few specific chunks
            out of a file, performance may be better if False.
        acl: str
            Canned ACL to set when writing
        version_id : str
            Explicit version of the object to open.  This requires that the s3
            filesystem is version aware and bucket versioning is enabled on the
            relevant bucket.
        encoding : str
            The encoding to use if opening the file in text mode. The platform's
            default text encoding is used if not given.
        cache_type : str
            See fsspec's documentation for available cache_type values. Set to "none"
            if no caching is desired. If None, defaults to ``self.default_cache_type``.
        requester_pays : bool (optional)
            If RequesterPays buckets are supported.  If None, defaults to the
            value used when creating the S3FileSystem (which defaults to False.)
        kwargs: dict-like
            Additional parameters used for s3 methods.  Typically used for
            ServerSideEncryption.
        """
        if block_size is None:
            block_size = self.default_block_size
        if fill_cache is None:
            fill_cache = self.default_fill_cache
        if requester_pays is None:
            requester_pays = bool(self.req_kw)

        acl = acl or self.s3_additional_kwargs.get('ACL', '')
        kw = self.s3_additional_kwargs.copy()
        kw.update(kwargs)
        if not self.version_aware and version_id:
            raise ValueError("version_id cannot be specified if the filesystem "
                             "is not version aware")

        if cache_type is None:
            cache_type = self.default_cache_type

        return S3File(self, path, mode, block_size=block_size, acl=acl,
                      version_id=version_id, fill_cache=fill_cache,
                      s3_additional_kwargs=kw, cache_type=cache_type,
                      autocommit=autocommit, requester_pays=requester_pays)

    def _lsdir(self, path, refresh=False, max_items=None):
        bucket, prefix, _ = self.split_path(path)
        prefix = prefix + '/' if prefix else ""
        if path not in self.dircache or refresh:
            try:
                logger.debug("Get directory listing page for %s" % path)
                pag = self.s3.get_paginator('list_objects_v2')
                config = {}
                if max_items is not None:
                    config.update(MaxItems=max_items, PageSize=2 * max_items)
                it = pag.paginate(Bucket=bucket, Prefix=prefix, Delimiter='/',
                                  PaginationConfig=config, **self.req_kw)
                files = []
                dircache = []
                for i in it:
                    dircache.extend(i.get('CommonPrefixes', []))
                    for c in i.get('Contents', []):
                        c['type'] = 'file'
                        c['size'] = c['Size']
                        files.append(c)
                if dircache:
                    files.extend([{'Key': l['Prefix'][:-1], 'Size': 0,
                                  'StorageClass': "DIRECTORY",
                                   'type': 'directory', 'size': 0}
                                  for l in dircache])
                for f in files:
                    f['Key'] = '/'.join([bucket, f['Key']])
                    f['name'] = f['Key']
            except ClientError as e:
                raise translate_boto_error(e)

            self.dircache[path] = files
            return files
        return self.dircache[path]

    def mkdir(self, path, acl="", **kwargs):
        path = self._strip_protocol(path).rstrip('/')
        if not self._parent(path):
            if acl and acl not in buck_acls:
                raise ValueError('ACL not in %s', buck_acls)
            try:
                params = {"Bucket": path, 'ACL': acl}
                region_name = (kwargs.get("region_name", None) or
                               self.client_kwargs.get("region_name", None))
                if region_name:
                    params['CreateBucketConfiguration'] = {
                        'LocationConstraint': region_name
                    }
                self.s3.create_bucket(**params)
                self.invalidate_cache('')
                self.invalidate_cache(path)
            except ClientError as e:
                raise translate_boto_error(e)
            except ParamValidationError as e:
                raise ValueError('Bucket create failed %r: %s' % (path, e))

    def rmdir(self, path):
        path = self._strip_protocol(path).rstrip('/')
        if not self._parent(path):
            try:
                self.s3.delete_bucket(Bucket=path)
            except ClientError as e:
                raise translate_boto_error(e)
            self.invalidate_cache(path)
            self.invalidate_cache('')

    def _lsbuckets(self, refresh=False):
        if '' not in self.dircache or refresh:
            if self.anon:
                # cannot list buckets if not logged in
                return []
            try:
                files = self.s3.list_buckets()['Buckets']
            except ClientError:
                # listbucket permission missing
                return []
            for f in files:
                f['Key'] = f['Name']
                f['Size'] = 0
                f['StorageClass'] = 'BUCKET'
                f['size'] = 0
                f['type'] = 'directory'
                f['name'] = f['Name']
                del f['Name']
            self.dircache[''] = files
            return files
        return self.dircache['']

    def _ls(self, path, refresh=False):
        """ List files in given bucket, or list of buckets.

        Listing is cached unless `refresh=True`.

        Note: only your buckets associated with the login will be listed by
        `ls('')`, not any public buckets (even if already accessed).

        Parameters
        ----------
        path : string/bytes
            location at which to list files
        refresh : bool (=False)
            if False, look in local cache for file details first
        """
        path = self._strip_protocol(path)
        if path in ['', '/']:
            return self._lsbuckets(refresh)
        else:
            return self._lsdir(path, refresh)

    def exists(self, path):
        if path in ['', '/']:
            # the root always exists, even if anon
            return True
        bucket, key, version_id = self.split_path(path)
        if key:
            return super().exists(path)
        else:
            try:
                self.ls(path)
                return True
            except FileNotFoundError:
                return False

    def touch(self, path, truncate=True, data=None, **kwargs):
        """Create empty file or truncate"""
        bucket, key, version_id = self.split_path(path)
        if version_id:
            raise ValueError("S3 does not support touching existing versions of files")
        if not truncate and self.exists(path):
            raise ValueError("S3 does not support touching existent files")
        try:
            self._call_s3(self.s3.put_object, kwargs, Bucket=bucket, Key=key)
        except ClientError as ex:
            raise translate_boto_error(ex)
        self.invalidate_cache(self._parent(path))

    def info(self, path, version_id=None, refresh=False):
        path = self._strip_protocol(path)
        if path in ['/', '']:
            return {'name': path, 'size': 0, 'type': 'directory'}
        kwargs = self.kwargs.copy()
        if version_id is not None:
            if not self.version_aware:
                raise ValueError("version_id cannot be specified if the "
                                 "filesystem is not version aware")
        bucket, key, path_version_id = self.split_path(path)
        version_id = _coalesce_version_id(path_version_id, version_id)
        if self.version_aware or (key and self._ls_from_cache(path) is None) or refresh:
            try:
                out = self._call_s3(self.s3.head_object, kwargs, Bucket=bucket,
                                    Key=key, **version_id_kw(version_id), **self.req_kw)
                return {
                    'ETag': out['ETag'],
                    'Key': '/'.join([bucket, key]),
                    'LastModified': out['LastModified'],
                    'Size': out['ContentLength'],
                    'size': out['ContentLength'],
                    'name': '/'.join([bucket, key]),
                    'type': 'file',
                    'StorageClass': "STANDARD",
                    'VersionId': out.get('VersionId')
                }
            except ClientError as e:
                ee = translate_boto_error(e)
                # This could have failed since the thing we are looking for is a prefix.
                if isinstance(ee, FileNotFoundError):
                    return super(S3FileSystem, self).info(path)
                else:
                    raise ee
            except ParamValidationError as e:
                raise ValueError('Failed to head path %r: %s' % (path, e))
        return super().info(path)
    
    def checksum(self, path, refresh=False):
        """
        Unique value for current version of file

        If the checksum is the same from one moment to another, the contents
        are guaranteed to be the same. If the checksum changes, the contents
        *might* have changed.

        Parameters
        ----------
        path : string/bytes
            path of file to get checksum for
        refresh : bool (=False)
            if False, look in local cache for file details first
        
        """

        info = self.info(path, refresh=refresh)
        
        if info["type"] != 'directory':
            return int(info["ETag"].strip('"'), 16)
        else:
            return int(tokenize(info), 16)
        

    def isdir(self, path):
        path = self._strip_protocol(path).strip("/")
        # Send buckets to super
        if "/" not in path:
            return super(S3FileSystem, self).isdir(path)

        if path in self.dircache:
            for fp in self.dircache[path]:
                # For files the dircache can contain itself.
                # If it contains anything other than itself it is a directory.
                if fp["name"] != path:
                    return True
            return False

        parent = self._parent(path)
        if parent in self.dircache:
            for f in self.dircache[parent]:
                if f["name"] == path:
                    # If we find ourselves return whether we are a directory
                    return f["type"] == "directory"
            return False

        # This only returns things within the path and NOT the path object itself
        return bool(self._lsdir(path))

    def ls(self, path, detail=False, refresh=False, **kwargs):
        """ List single "directory" with or without details

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
        path = self._strip_protocol(path).rstrip('/')
        files = self._ls(path, refresh=refresh)
        if not files:
            files = self._ls(self._parent(path), refresh=refresh)
            files = [o for o in files if o['name'].rstrip('/') == path
                     and o['type'] != 'directory']
        if detail:
            return files
        else:
            return list(sorted(set([f['name'] for f in files])))

    def object_version_info(self, path, **kwargs):
        if not self.version_aware:
            raise ValueError("version specific functionality is disabled for "
                             "non-version aware filesystems")
        bucket, key, _ = self.split_path(path)
        kwargs = {}
        out = {'IsTruncated': True}
        versions = []
        while out['IsTruncated']:
            out = self._call_s3(self.s3.list_object_versions, kwargs,
                                Bucket=bucket, Prefix=key, **self.req_kw)
            versions.extend(out['Versions'])
            kwargs['VersionIdMarker'] = out.get('NextVersionIdMarker', '')
        return versions

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
        bucket, key, version_id = self.split_path(path)
        if refresh or path not in self._metadata_cache:
            response = self._call_s3(self.s3.head_object,
                                     kwargs,
                                     Bucket=bucket,
                                     Key=key,
                                     **version_id_kw(version_id),
                                     **self.req_kw)
            self._metadata_cache[path] = response['Metadata']

        return self._metadata_cache[path]

    def get_tags(self, path):
        """Retrieve tag key/values for the given path

        Returns
        -------
        {str: str}
        """
        bucket, key, version_id = self.split_path(path)
        response = self._call_s3(self.s3.get_object_tagging,
                                 Bucket=bucket, Key=key, **version_id_kw(version_id))
        return {v['Key']: v['Value'] for v in response['TagSet']}

    def put_tags(self, path, tags, mode='o'):
        """Set tags for given existing key

        Tags are a str:str mapping that can be attached to any key, see
        https://docs.aws.amazon.com/awsaccountbilling/latest/aboutv2/allocation-tag-restrictions.html

        This is similar to, but distinct from, key metadata, which is usually
        set at key creation time.

        Parameters
        ----------
        path: str
            Existing key to attach tags to
        tags: dict str, str
            Tags to apply.
        mode:
            One of 'o' or 'm'
            'o': Will over-write any existing tags.
            'm': Will merge in new tags with existing tags.  Incurs two remote
            calls.
        """
        bucket, key, version_id = self.split_path(path)

        if mode == 'm':
            existing_tags = self.get_tags(path=path)
            existing_tags.update(tags)
            new_tags = [{'Key': k, 'Value': v}
                        for k, v in existing_tags.items()]
        elif mode == 'o':
            new_tags = [{'Key': k, 'Value': v} for k, v in tags.items()]
        else:
            raise ValueError("Mode must be {'o', 'm'}, not %s" % mode)

        tag = {'TagSet': new_tags}
        self._call_s3(self.s3.put_object_tagging,
                      Bucket=bucket, Key=key, Tagging=tag, **version_id_kw(version_id))

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

        Attributes have to be of the form documented in the
        `Metadata Reference`_.

        Parameters
        ----------
        kw_args : key-value pairs like field="value", where the values must be
            strings. Does not alter existing fields, unless
            the field appears here - if the value is None, delete the
            field.
        copy_kwargs : dict, optional
            dictionary of additional params to use for the underlying
            s3.copy_object.

        Examples
        --------
        >>> mys3file.setxattr(attribute_1='value1', attribute_2='value2')  # doctest: +SKIP
        # Example for use with copy_args
        >>> mys3file.setxattr(copy_kwargs={'ContentType': 'application/pdf'},
        ...     attribute_1='value1')  # doctest: +SKIP


        .. Metadata Reference:
        http://docs.aws.amazon.com/AmazonS3/latest/dev/UsingMetadata.html#object-metadata
        """

        bucket, key, version_id = self.split_path(path)
        metadata = self.metadata(path)
        metadata.update(**kw_args)
        copy_kwargs = copy_kwargs or {}

        # remove all keys that are None
        for kw_key in kw_args:
            if kw_args[kw_key] is None:
                metadata.pop(kw_key, None)

        src = {'Bucket': bucket, 'Key': key}
        if version_id:
            src['VersionId'] = version_id

        self._call_s3(
            self.s3.copy_object,
            copy_kwargs,
            CopySource=src,
            Bucket=bucket,
            Key=key,
            Metadata=metadata,
            MetadataDirective='REPLACE',
        )

        # refresh metadata
        self._metadata_cache[path] = metadata

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
        bucket, key, version_id = self.split_path(path)
        if key:
            if acl not in key_acls:
                raise ValueError('ACL not in %s', key_acls)
            self._call_s3(self.s3.put_object_acl,
                          kwargs, Bucket=bucket, Key=key, ACL=acl, **version_id_kw(version_id))
        else:
            if acl not in buck_acls:
                raise ValueError('ACL not in %s', buck_acls)
            self._call_s3(self.s3.put_bucket_acl,
                          kwargs, Bucket=bucket, ACL=acl)

    def url(self, path, expires=3600, **kwargs):
        """ Generate presigned URL to access path by HTTP

        Parameters
        ----------
        path : string
            the key path we are interested in
        expires : int
            the number of seconds this signature will be good for.
        """
        bucket, key, version_id = self.split_path(path)
        return self.s3.generate_presigned_url(
            ClientMethod='get_object',
            Params=dict(Bucket=bucket, Key=key, **version_id_kw(version_id), **kwargs),
            ExpiresIn=expires)

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
        bucket, key, version_id = self.split_path(path)
        if version_id:
            raise ValueError("Cannot write to an explicit versioned file!")
        mpu = self._call_s3(
            self.s3.create_multipart_upload,
            kwargs,
            Bucket=bucket,
            Key=key
        )
        # TODO: Make this support versions?
        out = [self._call_s3(
            self.s3.upload_part_copy,
            kwargs,
            Bucket=bucket, Key=key, UploadId=mpu['UploadId'],
            CopySource=f, PartNumber=i + 1)
            for (i, f) in enumerate(filelist)]
        parts = [{'PartNumber': i + 1, 'ETag': o['CopyPartResult']['ETag']} for
                 (i, o) in enumerate(out)]
        part_info = {'Parts': parts}
        self.s3.complete_multipart_upload(Bucket=bucket, Key=key,
                                          UploadId=mpu['UploadId'],
                                          MultipartUpload=part_info)
        self.invalidate_cache(path)

    def copy_basic(self, path1, path2, **kwargs):
        """ Copy file between locations on S3

        Not allowed where the origin is >5GB - use copy_managed
        """
        buc1, key1, ver1 = self.split_path(path1)
        buc2, key2, ver2 = self.split_path(path2)
        if ver2:
            raise ValueError("Cannot copy to a versioned file!")
        try:
            copy_src = {'Bucket': buc1, 'Key': key1}
            if ver1:
                copy_src['VersionId'] = ver1
            self._call_s3(
                self.s3.copy_object,
                kwargs,
                Bucket=buc2, Key=key2, CopySource=copy_src
            )
        except ClientError as e:
            raise translate_boto_error(e)
        except ParamValidationError as e:
            raise ValueError('Copy failed (%r -> %r): %s' % (path1, path2, e))
        self.invalidate_cache(path2)

    def copy_managed(self, path1, path2, block=5 * 2**30, **kwargs):
        """Copy file between locations on S3 as multi-part

        block: int
            The size of the pieces, must be larger than 5MB and at most 5GB.
            Smaller blocks mean more calls, only useful for testing.
        """
        if block < 5*2**20 or block > 5*2**30:
            raise ValueError('Copy block size must be 5MB<=block<=5GB')
        size = self.info(path1)['size']
        bucket, key, version = self.split_path(path2)
        mpu = self._call_s3(
            self.s3.create_multipart_upload,
            Bucket=bucket, Key=key, **kwargs)
        parts = []
        for i, offset in enumerate(range(0, size + 1, block)):
            for attempt in range(self.retries + 1):
                try:
                    brange = "bytes=%i-%i" % (
                            offset, min(offset + block - 1, size))
                    out = self._call_s3(
                        self.s3.upload_part_copy,
                        Bucket=bucket,
                        Key=key,
                        PartNumber=i + 1,
                        UploadId=mpu['UploadId'],
                        CopySource=path1,
                        CopySourceRange=brange)
                    break
                except S3_RETRYABLE_ERRORS as exc:
                    if attempt < self.retries:
                        logger.debug('Exception %r on S3 write, retrying', exc,
                                     exc_info=True)
                    time.sleep(1.7 ** attempt * 0.1)
                except Exception as exc:
                    raise IOError('Write failed: %r' % exc)
            parts.append({'PartNumber': i + 1,
                          'ETag': out['CopyPartResult']['ETag']})
        self._call_s3(
            self.s3.complete_multipart_upload,
            Bucket=bucket,
            Key=key,
            UploadId=mpu['UploadId'],
            MultipartUpload={'Parts': parts})
        self.invalidate_cache(path2)

    def copy(self, path1, path2, **kwargs):
        gb5 = 5 * 2**30
        path1 = self._strip_protocol(path1)
        size = self.info(path1)['size']
        if size <= gb5:
            # simple copy allowed for <5GB
            self.copy_basic(path1, path2, **kwargs)
        else:
            # serial multipart copy
            self.copy_managed(path1, path2, **kwargs)

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
        buckets = {self.split_path(path)[0] for path in pathlist}
        if len(buckets) > 1:
            raise ValueError("Bulk delete files should refer to only one "
                             "bucket")
        bucket = buckets.pop()
        if len(pathlist) > 1000:
            for i in range((len(pathlist) // 1000) + 1):
                self.bulk_delete(pathlist[i * 1000:(i + 1) * 1000])
            return
        delete_keys = {'Objects': [{'Key': self.split_path(path)[1]} for path
                                   in pathlist]}
        for path in pathlist:
            self.invalidate_cache(self._parent(path))
        try:
            self._call_s3(
                self.s3.delete_objects,
                kwargs,
                Bucket=bucket, Delete=delete_keys)
        except ClientError as e:
            raise translate_boto_error(e)

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
        # TODO: explicit version deletes?
        bucket, key, _ = self.split_path(path)
        if recursive:
            files = self.find(path, maxdepth=None)
            if key and not files:
                raise FileNotFoundError(path)
            self.bulk_delete(files, **kwargs)
            if not key:
                self.rmdir(bucket)
            return
        if key:
            if not self.exists(path):
                raise FileNotFoundError(path)
            try:
                self._call_s3(
                    self.s3.delete_object, kwargs, Bucket=bucket, Key=key)
            except ClientError as e:
                raise translate_boto_error(e)
            self.invalidate_cache(self._parent(path))
        else:
            if self.exists(bucket):
                try:
                    self.s3.delete_bucket(Bucket=bucket)
                except BotoCoreError as e:
                    raise IOError('Delete bucket %r failed: %s' % (bucket, e))
                self.invalidate_cache(bucket)
                self.invalidate_cache('')
            else:
                raise FileNotFoundError(path)

    def invalidate_cache(self, path=None):
        if path is None:
            self.dircache.clear()
        else:
            path = self._strip_protocol(path)
            self.dircache.pop(path, None)
            while path:
                self.dircache.pop(path, None)
                path = self._parent(path)

    def walk(self, path, maxdepth=None, **kwargs):
        if path in ['', '*'] + ['{}://'.format(p) for p in self.protocol]:
            raise ValueError('Cannot crawl all of S3')
        return super().walk(path, maxdepth=maxdepth, **kwargs)


class S3File(AbstractBufferedFile):
    """
    Open S3 key as a file. Data is only loaded and cached on demand.

    Parameters
    ----------
    s3 : S3FileSystem
        botocore connection
    path : string
        S3 bucket/key to access
    mode : str
        One of 'rb', 'wb', 'ab'. These have the same meaning
        as they do for the built-in `open` function.
    block_size : int
        read-ahead size for finding delimiters
    fill_cache : bool
        If seeking to new a part of the file beyond the current buffer,
        with this True, the buffer will be filled between the sections to
        best support random access. When reading only a few specific chunks
        out of a file, performance may be better if False.
    acl: str
        Canned ACL to apply
    version_id : str
        Optional version to read the file at.  If not specified this will
        default to the current version of the object.  This is only used for
        reading.
    requester_pays : bool (False)
        If RequesterPays buckets are supported.

    Examples
    --------
    >>> s3 = S3FileSystem()  # doctest: +SKIP
    >>> with s3.open('my-bucket/my-file.txt', mode='rb') as f:  # doctest: +SKIP
    ...     ...  # doctest: +SKIP

    See Also
    --------
    S3FileSystem.open: used to create ``S3File`` objects

    """
    retries = 5
    part_min = 5 * 2 ** 20
    part_max = 5 * 2 ** 30

    def __init__(self, s3, path, mode='rb', block_size=5 * 2 ** 20, acl="",
                 version_id=None, fill_cache=True, s3_additional_kwargs=None,
                 autocommit=True, cache_type='bytes', requester_pays=False):
        bucket, key, path_version_id = s3.split_path(path)
        if not key:
            raise ValueError('Attempt to open non key-like path: %s' % path)
        self.bucket = bucket
        self.key = key
        self.version_id = _coalesce_version_id(version_id, path_version_id)
        self.acl = acl
        if self.acl and self.acl not in key_acls:
            raise ValueError('ACL not in %s', key_acls)
        self.mpu = None
        self.parts = None
        self.fill_cache = fill_cache
        self.s3_additional_kwargs = s3_additional_kwargs or {}
        self.req_kw = {'RequestPayer': 'requester'} if requester_pays else {}
        super().__init__(s3, path, mode, block_size, autocommit=autocommit,
                         cache_type=cache_type)
        self.s3 = self.fs  # compatibility
        if self.writable():
            if block_size < 5 * 2 ** 20:
                raise ValueError('Block size must be >=5MB')
        else:
            if version_id and self.fs.version_aware:
                self.version_id = version_id
                self.details = self.fs.info(self.path, version_id=version_id)
                self.size = self.details['size']
            elif self.fs.version_aware:
                self.version_id = self.details.get('VersionId')
                # In this case we have not managed to get the VersionId out of details and
                # we should invalidate the cache and perform a full head_object since it
                # has likely been partially populated by ls.
                if self.version_id is None:
                    self.fs.invalidate_cache(self.path)
                    self.details = self.fs.info(self.path)
                    self.version_id = self.details.get('VersionId')

        # when not using autocommit we want to have transactional state to manage
        self.append_block = False

        if 'a' in mode and s3.exists(path):
            loc = s3.info(path)['size']
            if loc < 5 * 2 ** 20:
                # existing file too small for multi-upload: download
                self.write(self.fs.cat(self.path))
            else:
                self.append_block = True
            self.loc = loc

    def _call_s3(self, method, *kwarglist, **kwargs):
        return self.fs._call_s3(method, self.s3_additional_kwargs, *kwarglist,
                                **kwargs)

    def _initiate_upload(self):
        if self.autocommit and not self.append_block and self.tell() < self.blocksize:
            # only happens when closing small file, use on-shot PUT
            return
        logger.debug("Initiate upload for %s" % self)
        self.parts = []
        try:
            self.mpu = self._call_s3(
                self.fs.s3.create_multipart_upload,
                Bucket=self.bucket, Key=self.key, ACL=self.acl)
        except ClientError as e:
            raise translate_boto_error(e)
        except ParamValidationError as e:
            raise ValueError('Initiating write to %r failed: %s' % (self.path, e))

        if self.append_block:
            # use existing data in key when appending,
            # and block is big enough
            out = self.fs._call_s3(
                self.fs.s3.upload_part_copy,
                self.s3_additional_kwargs,
                Bucket=self.bucket,
                Key=self.key,
                PartNumber=1,
                UploadId=self.mpu['UploadId'],
                CopySource=self.path)
            self.parts.append({'PartNumber': 1,
                               'ETag': out['CopyPartResult']['ETag']})

    def metadata(self, refresh=False, **kwargs):
        """ Return metadata of file.
        See :func:`~s3fs.S3Filesystem.metadata`.

        Metadata is cached unless `refresh=True`.
        """
        return self.fs.metadata(self.path, refresh, **kwargs)

    def getxattr(self, xattr_name, **kwargs):
        """ Get an attribute from the metadata.
        See :func:`~s3fs.S3Filesystem.getxattr`.

        Examples
        --------
        >>> mys3file.getxattr('attribute_1')  # doctest: +SKIP
        'value_1'
        """
        return self.fs.getxattr(self.path, xattr_name, **kwargs)

    def setxattr(self, copy_kwargs=None, **kwargs):
        """ Set metadata.
        See :func:`~s3fs.S3Filesystem.setxattr`.

        Examples
        --------
        >>> mys3file.setxattr(attribute_1='value1', attribute_2='value2')  # doctest: +SKIP
        """
        if self.writable():
            raise NotImplementedError('cannot update metadata while file '
                                      'is open for writing')
        return self.fs.setxattr(self.path, copy_kwargs=copy_kwargs, **kwargs)

    def url(self, **kwargs):
        """ HTTP URL to read this file (if it already exists)
        """
        return self.fs.url(self.path, **kwargs)

    def _fetch_range(self, start, end):
        return _fetch_range(self.fs.s3, self.bucket, self.key, self.version_id, start, end, req_kw=self.req_kw)

    def _upload_chunk(self, final=False):
        bucket, key, _ = self.fs.split_path(self.path)
        logger.debug("Upload for %s, final=%s, loc=%s, buffer loc=%s" % (
            self, final, self.loc, self.buffer.tell()
        ))
        if self.autocommit and not self.append_block and final and self.tell() < self.blocksize:
            # only happens when closing small file, use on-shot PUT
            data1 = False
        else:
            self.buffer.seek(0)
            (data0, data1) = (None, self.buffer.read(self.blocksize))

        while data1:
            (data0, data1) = (data1, self.buffer.read(self.blocksize))
            data1_size = len(data1)

            if 0 < data1_size < self.blocksize:
                remainder = data0 + data1
                remainder_size = self.blocksize + data1_size

                if remainder_size <= self.part_max:
                    (data0, data1) = (remainder, None)
                else:
                    partition = remainder_size // 2
                    (data0, data1) = (remainder[:partition], remainder[partition:])

            part = len(self.parts) + 1
            logger.debug("Upload chunk %s, %s" % (self, part))

            for attempt in range(self.retries + 1):
                try:
                    out = self._call_s3(
                        self.fs.s3.upload_part,
                        Bucket=bucket,
                        PartNumber=part, UploadId=self.mpu['UploadId'],
                        Body=data0, Key=key)
                    break
                except S3_RETRYABLE_ERRORS as exc:
                    if attempt < self.retries:
                        logger.debug('Exception %r on S3 write, retrying', exc,
                                     exc_info=True)
                    time.sleep(1.7**attempt * 0.1)
                except Exception as exc:
                    raise IOError('Write failed: %r' % exc)
            else:
                raise IOError('Write failed after %i retries' % self.retries)

            self.parts.append({'PartNumber': part, 'ETag': out['ETag']})

        if self.autocommit and final:
            self.commit()
        return not final

    def commit(self):
        logger.debug("Commit %s" % self)
        if self.tell() == 0:
            if self.buffer is not None:
                logger.debug("Empty file committed %s" % self)
                self._abort_mpu()
                self.fs.touch(self.path)
        elif not self.parts:
            if self.buffer is not None:
                logger.debug("One-shot upload of %s" % self)
                self.buffer.seek(0)
                data = self.buffer.read()
                write_result = self._call_s3(
                    self.fs.s3.put_object,
                    Key=self.key, Bucket=self.bucket, Body=data, **self.kwargs
                )
                if self.fs.version_aware:
                    self.version_id = write_result.get('VersionId')
            else:
                raise RuntimeError
        else:
            logger.debug("Complete multi-part upload for %s " % self)
            part_info = {'Parts': self.parts}
            write_result = self._call_s3(
                self.fs.s3.complete_multipart_upload,
                Bucket=self.bucket,
                Key=self.key,
                UploadId=self.mpu['UploadId'],
                MultipartUpload=part_info)
            if self.fs.version_aware:
                self.version_id = write_result.get('VersionId')

        # complex cache invalidation, since file's appearance can cause several
        # directories
        self.buffer = None
        parts = self.path.split('/')
        path = parts[0]
        for p in parts[1:]:
            if path in self.fs.dircache and not [
                    True for f in self.fs.dircache[path]
                    if f['name'] == path + '/' + p]:
                self.fs.invalidate_cache(path)
            path = path + '/' + p

    def discard(self):
        self._abort_mpu()
        self.buffer = None  # file becomes unusable

    def _abort_mpu(self):
        if self.mpu:
            self._call_s3(
                self.fs.s3.abort_multipart_upload,
                Bucket=self.bucket,
                Key=self.key,
                UploadId=self.mpu['UploadId'],
            )
            self.mpu = None


def _fetch_range(client, bucket, key, version_id, start, end, max_attempts=10,
                 req_kw=None):
    if req_kw is None:
        req_kw = {}
    if start == end:
        # When these match, we would make a request with `range=start-end - 1`
        # According to RFC2616, servers are supposed to ignore the Range
        # field when it's invalid like this. S3 does ignore it, moto doesn't.
        # To avoid differences in behavior under mocking, we just avoid
        # making these requests. It's hoped that since we're being called
        # from a caching object, this won't end up mattering.
        logger.debug(
            'skip fetch for negative range - bucket=%s,key=%s,start=%d,end=%d',
            bucket, key, start, end
        )
        return b''
    logger.debug("Fetch: %s/%s, %s-%s", bucket, key, start, end)
    for i in range(max_attempts):
        try:
            resp = client.get_object(Bucket=bucket, Key=key,
                                     Range='bytes=%i-%i' % (start, end - 1),
                                     **version_id_kw(version_id),
                                     **req_kw)
            return resp['Body'].read()
        except S3_RETRYABLE_ERRORS as e:
            logger.debug('Exception %r on S3 download, retrying', e,
                         exc_info=True)
            time.sleep(1.7**i * 0.1)
            continue
        except ConnectionError as e:
            logger.debug('ConnectionError %r on S3 download, retrying', e,
                         exc_info=True)
            time.sleep(1.7**i * 0.1)
            continue
        except ClientError as e:
            if e.response['Error'].get('Code', 'Unknown') in ['416',
                                                              'InvalidRange']:
                return b''
            raise translate_boto_error(e)
        except Exception as e:
            if 'time' in str(e).lower():  # Actual exception type changes often
                continue
            else:
                raise
    raise RuntimeError("Max number of S3 retries exceeded")
