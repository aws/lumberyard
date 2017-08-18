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

class ClientError(RuntimeError):
    '''Exception type raised when a client makes an invalid request. All other exception
    types will result in the generic "internal service error" message being returned to
    the client. This prevents messages from being returned to the client that could expose
    an exploit to an hacker.

    Note that the service expects API Gateway to be configured to look for the "Client Error"
    prefix on the message to determine if the error is an client error or an internal service
    error. This exception type simply prefixes the provided message with "Client Error: ".

    The API Gateway configuration will return an HTTP 400 response for client errors and 
    an HTTP 500 respose for internal service errors. In both cases the response body will be:

        {
            "errorMessage": "...",
            "errorType": "..."
        }

    For client errors, the errorType property will be the exception type name. You can extend
    ClientError to define your own custom error type codes. This can be used to allow the 
    client to take corrective actions based on the error type, rather than trying to parse
    the error message.

    For internal service errors, errorType will always be "ServiceError".
    '''
    def __init__(self, message):
        super(ClientError, self).__init__('Client Error: ' + message)
        
class InternalRequestError(RuntimeError):
    def __init__(self):
        super(InternalRequestError, self).__init__('Internal request error')

    def __init__(self, message_string):
        super(InternalRequestError, self).__init__('Internal error:' + message_string)
