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

from mock import MagicMock

import mock
import functools

import boto3

def s3_get_object_response(body_content):
    mock_body = MagicMock()
    mock_body.read.return_value = body_content
    return { 'Body': mock_body }


def patch_client(service, method, **kwargs):
    '''Creates a MagicMock object for a boto3.client method.

    Args:

        service - the name of the AWS service passed to boto3.client.

        method - the name of the method on the service client to mock.

        new (named) - like mock.patch(new=DEFAULT), specifies the mock
        object instead of createing a MagicMock.

        reload (named) - a module or list of modules that will be reloaded
        after the patch is applied. Useful for when a module caches a 
        client object globally.

        **kwargs - like mock.patch(**kwargs), provides arguments passed
        to the MagicMock constructor.

    Returns:

        A callable object that can be used as a decorator or context 
        object (just like the return value of mock.patch).

        When used as a decorator, the mock method object is passed as
        an argument to the decorated method. When used as a context
        object, the mock method object is the value provided by the 
        with statement.

        In addition to the usual MagicMock methods, the provided mock 
        method object supports the following methods:

            assert_client_created_with( **kwargs ) - verify that the
            boto3.client method was called with the specified key 
            word arguments.

    Example:

        import mock_aws

        BODY_CONTENT = '...'

        @mock_aws.patch_client('s3', 'get_object', return_value = mock_aws.s3_get_object_response(BODY_CONTENT))
        def test_foo(self, mock_get_object):
            
            s3 = boto3.client('s3')   
            s3.get_object(Bucket='...', Key='...')

            mock_get_object.assert_called_with(Bucket='...', Key='...')

    '''

    #return mock.patch('boto3.client', new = MockClientFactory(service, method, kwargs))

    return __ClientPatchContextDecorator(service, method, kwargs)


class __ClientPatchContextDecorator(object):

    class MockClientFactory(MagicMock):

        def __init__(self):

            super(MagicMock, self).__init__()

            self.__mock_clients = {}

            def __side_effect(*args, **kwargs):

                if not args:
                    raise AssertionError('boto3.client called without service type argument.')

                service = args[0]

                return self.get_mock_client(service)

            self.side_effect = __side_effect


        def get_mock_client(self, service):
            mock_client = self.__mock_clients.get(service)
            if not mock_client:
                mock_client = MagicMock(name=service)
                self.__mock_clients[service] = mock_client
            return mock_client


        def add_service_method_mock(self, service, method, kwargs):

            mock_client = self.get_mock_client(service)

            mock_method = kwargs.get('new')
            if not mock_method:
                mock_method = MagicMock(**kwargs)

            setattr(mock_method, 'client_factory', self)

            mock_client.attach_mock(mock_method, method)

            return mock_method


    def __init__(self, service, method, kwargs):
        self.__service = service
        self.__method = method
        self.__reload = kwargs.pop('reload', [])
        self.__kwargs = kwargs
        self.__client_factory_patcher = None

        if not isinstance(self.__reload, list):
            self.__reload = [ self.__reload ]


    def __call__(self, func):

        if not hasattr(func, 'mock_client_factory'):
            mock_client_factory = self.MockClientFactory()
            setattr(func, 'mock_client_factory', mock_client_factory)
        else:
            mock_client_factory = getattr(func, 'mock_client_factory')

        # The call to patcher(func) below causes mock_method to be added into 
        # the function's arguments in the right location relative to any other 
        # patch decorators that are on this function. Without this, mock_method 
        # ends up before or after all the other patch generated parameters, not 
        # interspesed between them as would be expected.

        mock_client = mock_client_factory.get_mock_client(self.__service)
        patcher = mock.patch.object(mock_client, self.__method, client_factory=mock_client_factory, **self.__kwargs)
        patcher_func = patcher(func)

        @functools.wraps(func)
        def wrapper(*args, **kwargs):

            if not isinstance(boto3.client, self.MockClientFactory):
                self.__client_factory_patcher = mock.patch.object(boto3, 'client', new = mock_client_factory)
                self.__client_factory_patcher.start()

            for module in self.__reload:
                reload(module)

            result = patcher_func(*args, **kwargs)

            if self.__client_factory_patcher:
                self.__client_factory_patcher.stop()

            return result

        return wrapper


    def __enter__(self):

        if not isinstance(boto3.client, self.MockClientFactory):
            self.__client_factory_patcher = mock.patch.object(boto3, 'client', new = self.MockClientFactory())
            self.__client_factory_patcher.start()

        for module in self.__reload:
            reload(module)

        mock_client_factory = boto3.client
        mock_client = mock_client_factory.get_mock_client(self.__service)
        self.__method_patcher = mock.patch.object(mock_client, self.__method, client_factory=mock_client_factory, **self.__kwargs)
        return self.__method_patcher.start()


    def __exit__(self, *args, **kwargs):

        if self.__method_patcher:
            self.__method_patcher.stop()

        if self.__client_factory_patcher:
            self.__client_factory_patcher.stop()

    
    