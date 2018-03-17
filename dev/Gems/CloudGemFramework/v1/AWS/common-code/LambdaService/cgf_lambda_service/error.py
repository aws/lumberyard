#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#

'''All exception types other than ClientError and ForbiddenRequestError will result in
the generic "internal service error" message being returned to the client. This prevents
messages from being returned to the client that could expose an exploit to an hacker.

Note that the service expects API Gateway to be configured to look for the "Client Error:"
or "Forbidden:" prefixes on the message to determine if the error is an client error,
forbidden error, or an internal service error. The exception types below simply prefix
the provided message.

The API Gateway configuration will return an HTTP 400 response for client errors,
an HTTP 403 response for forbidden errors, and an HTTP 500 respose for
internal service errors. In all cases the response body will be:

    {
        "errorMessage": "...",
        "errorType": "..."
    }

For client errors and forbidden errors, the errorType property will be the exception type name.
You can extend ClientError or ForbiddenRequestError to define your own custom error type codes.
This can be used to allow the client to take corrective actions based on the error type,
rather than trying to parse the error message.

For internal service errors, errorType will always be "ServiceError".
'''


class ClientError(RuntimeError):
    '''Exception type raised when a client makes an invalid request.
    This exception type prefixes the provided message with "Client Error: ".
    See documentation above for more information.
    '''
    def __init__(self, message):
        super(ClientError, self).__init__('Client Error: ' + message)


class ForbiddenRequestError(RuntimeError):
    '''Exception type raised when a client makes a request that is forbidden.
    This exception type prefixes the provided message with "Forbidden:".
    See documentation above for more information.
    '''
    def __init__(self, message='Access to the requested resource is not allowed.'):
        super(ForbiddenRequestError, self).__init__('Forbidden: ' + message)


class NotFoundError(RuntimeError):
    '''Exception type raised when a client requests an entity that does not exist.
    This exception type prefixes the provided message with "Not Found: ".
    See documentation above for more information.'''

    def __init__(self, message = 'The resource you requested was not found.'):
        super(NotFoundError, self).__init__('Not Found: ' + message)
