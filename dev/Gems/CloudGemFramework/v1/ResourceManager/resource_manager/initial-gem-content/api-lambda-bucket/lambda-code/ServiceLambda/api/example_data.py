import service
import errors
import bucket_data

@service.api
def list(request, start = None, count = None):
    keys = bucket_data.list(start, count)
    return {
        'Keys': [ { 'Key': key } for key in keys ]
    }


@service.api
def create(request, data):
    __validate_data(data)
    key = bucket_data.create(data)
    return {
        'Key': key
    }


@service.api
def read(request, key):
    data = bucket_data.read(key)
    if data is None:
        raise errors.NotFoundError('No data with key {} was found.'.format(key))
    return data


@service.api
def update(request, key, data):
    __validate_data(data)
    if not bucket_data.update(key, data):
        raise errors.NotFoundError('No data with key {} was found.'.format(key))


@service.api
def delete(request, key):
    if not bucket_data.delete(key):
        raise errors.NotFoundError('No data with key {} was found.'.format(key))


def __validate_data(data):

    if not isinstance(data.get('ExamplePropertyA', None), basestring):
        raise errors.ClientError('Property ExamplePropertyA in provided data is missing or is not a string.')

    if not isinstance(data.get('ExamplePropertyB', None), int):
        raise errors.ClientError('Property ExamplePropertyB in provided data is missing or is not an integer.')
