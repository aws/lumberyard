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
# $Revision: #4 $

class HttpError(RuntimeError):

    @classmethod
    def for_response(cls, response):
        code = response.status_code
        if code >= 200 and code <= 299:
            return
        if code >= 400 and code <= 499:
            if code == 404:
                raise NotFoundError(code, response.text)
            if code in [401, 403]:
                raise NotAllowedError(code, response.text)
            raise ClientError(code, response.text)
        if code >= 500 and code <= 599:
            raise ServerError(code, response.text)
        raise HttpError(code, response.text)

    def __init__(self, code, message):
        super(HttpError, self).__init__(message + 'HTTP {}'.format(code))
        self.__code = code

    @property
    def code(self):
        return self.__code


class ClientError(HttpError):

    def __init__(self, code, message):
        super(ClientError, self).__init__(code, message)


class NotFoundError(ClientError):

    def __init__(self, code, message):
        super(NotFoundError, self).__init__(code, message)


class NotAllowedError(ClientError):

    def __init__(self, code, message):
        super(NotAllowedError, self).__init__(code, message)

    
class ServerError(HttpError):

    def __init__(self, code, message):
        super(ServerError, self).__init__(code, message)



