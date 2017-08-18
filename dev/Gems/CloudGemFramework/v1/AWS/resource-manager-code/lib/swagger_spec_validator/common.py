import contextlib
import sys

try:
    import simplejson as json
except ImportError:
    import json
import six
from six.moves.urllib import request

TIMEOUT_SEC = 1


def wrap_exception(method):
    def wrapper(*args, **kwargs):
        try:
            return method(*args, **kwargs)
        except Exception as e:
            six.reraise(
                SwaggerValidationError,
                SwaggerValidationError(str(e)),
                sys.exc_info()[2])
    return wrapper


def load_json(url):
    with contextlib.closing(request.urlopen(url, timeout=TIMEOUT_SEC)) as fh:
        return json.loads(fh.read().decode('utf-8'))


class SwaggerValidationError(Exception):
    """Exception raised in case of a validation error."""
    pass
