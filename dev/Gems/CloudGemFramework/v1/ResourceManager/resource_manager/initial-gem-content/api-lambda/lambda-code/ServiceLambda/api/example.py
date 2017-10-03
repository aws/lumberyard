from __future__ import print_function

import service

# import errors
#
# raise errors.ClientError(message) - results in HTTP 400 response with message
# raise errors.ForbiddenRequestError(message) - results in 403 response with message
# raise errors.NotFoundError(message) - results in HTTP 404 response with message
#
# Any other exception results in HTTP 500 with a generic internal service error message.

@service.api
def get(request):
    # TODO: implement...
    print('example.get called')
    return { 'ExamplePropertyA': 'hello', 'ExamplePropertyB': 42 }


@service.api
def post(request, data):
    # TODO: implement...
    print('example.post called with', data)
    return { 'ExamplePropertyA': 'world', 'ExamplePropertyB': 420 }
