# -*- coding: utf-8 -*-
"""JIRA utils used internally."""
from __future__ import unicode_literals
import threading

from jira.resilientsession import raise_on_error


class CaseInsensitiveDict(dict):
    """A case-insensitive ``dict``-like object.

    Implements all methods and operations of
    ``collections.MutableMapping`` as well as dict's ``copy``. Also
    provides ``lower_items``.

    All keys are expected to be strings. The structure remembers the
    case of the last key to be set, and ``iter(instance)``,
    ``keys()``, ``items()``, ``iterkeys()``, and ``iteritems()``
    will contain case-sensitive keys. However, querying and contains
    testing is case insensitive::

        cid = CaseInsensitiveDict()
        cid['Accept'] = 'application/json'
        cid['aCCEPT'] == 'application/json'  # True
        list(cid) == ['Accept']  # True

    For example, ``headers['content-encoding']`` will return the
    value of a ``'Content-Encoding'`` response header, regardless
    of how the header name was originally stored.

    If the constructor, ``.update``, or equality comparison
    operations are given keys that have equal ``.lower()``s, the
    behavior is undefined.

    """

    def __init__(self, *args, **kw):
        super(CaseInsensitiveDict, self).__init__(*args, **kw)

        self.itemlist = {}
        for key, value in super(CaseInsensitiveDict, self).items():
            if key != key.lower():
                self[key.lower()] = value
                self.pop(key, None)

        # self.itemlist[key.lower()] = value

    def __setitem__(self, key, value):
        """Overwrite [] implementation."""
        super(CaseInsensitiveDict, self).__setitem__(key.lower(), value)

    # def __iter__(self):
    #    return iter(self.itemlist)

    # def keys(self):
    #    return self.itemlist

    # def values(self):
    #    return [self[key] for key in self]

    # def itervalues(self):
    #    return (self[key] for key in self)


def threaded_requests(requests):
    for fn, url, request_args in requests:
        th = threading.Thread(
            target=fn, args=(url,), kwargs=request_args, name=url,
        )
        th.start()

    for th in threading.enumerate():
        if th.name.startswith('http'):
            th.join()


def json_loads(r):
    raise_on_error(r)
    try:
        return r.json()
    except ValueError:
        # json.loads() fails with empty bodies
        if not r.text:
            return {}
        raise
