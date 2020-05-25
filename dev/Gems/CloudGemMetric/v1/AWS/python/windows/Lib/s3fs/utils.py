from contextlib import contextmanager
import sys


@contextmanager
def ignoring(*exceptions):
    try:
        yield
    except exceptions:
        pass


def title_case(string):
    """
    TitleCases a given string.

    Parameters
    ----------
    string : underscore separated string
    """
    return ''.join(x.capitalize() for x in string.split('_'))


class ParamKwargsHelper(object):
    """
    Utility class to help extract the subset of keys that an s3 method is
    actually using

    Parameters
    ----------
    s3 : boto S3FileSystem
    """
    _kwarg_cache = {}

    def __init__(self, s3):
        self.s3 = s3

    def _get_valid_keys(self, model_name):
        if model_name not in self._kwarg_cache:
            model = self.s3.meta.service_model.operation_model(model_name)
            valid_keys = set(model.input_shape.members.keys())
            self._kwarg_cache[model_name] = valid_keys
        return self._kwarg_cache[model_name]

    def filter_dict(self, method_name, d):
        model_name = title_case(method_name)
        valid_keys = self._get_valid_keys(model_name)
        if isinstance(d, SSEParams):
            d = d.to_kwargs()
        return {k: v for k, v in d.items() if k in valid_keys}


class SSEParams(object):

    def __init__(self, server_side_encryption=None, sse_customer_algorithm=None,
                 sse_customer_key=None, sse_kms_key_id=None):
        self.ServerSideEncryption = server_side_encryption
        self.SSECustomerAlgorithm = sse_customer_algorithm
        self.SSECustomerKey = sse_customer_key
        self.SSEKMSKeyId = sse_kms_key_id

    def to_kwargs(self):
        return {k: v for k, v in self.__dict__.items() if v is not None}
