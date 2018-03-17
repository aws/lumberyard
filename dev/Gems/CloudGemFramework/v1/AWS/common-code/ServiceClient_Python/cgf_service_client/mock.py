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

# Python
import functools
import imp
import sys

# import the real mock module even though this module is also named mock
# sys.path[1:] removes '.' from the path, so the local module isn't found
f, pathname, desc = imp.find_module('mock', sys.path[1:])
real_mock = imp.load_module('real_mock', f, pathname, desc)
if f: f.close() 

# ServiceClient
import cgf_service_client

# Utils
from cgf_utils.data_utils import Data


def patch():
    '''Patch the cgf_service_client system. Can be used as either a function decorator 
    or a context object. Examples:
    
      @cgf_service_client.mock.patch()
      def test_foo(self, mock_service_client):
        
        mock_client = mock_service_client.for_url('test-base-url').navigate('xyz', 'bar')
        mock_client.GET.return_value = { 'Return': 'Data' }

        do_something_that_uses_service_client()

        mock_client.GET.assert_called_once_with(query_param = 10)


    def test_bar(self):

        with cgf_service_client.mock.patch() as mock_service_client:

            mock_client = mock_service_client.for_url('test-base-url').navigate('xyz', 'bar')
            mock_client.GET.return_value = { 'Return': 'Data' }

            do_something_that_uses_service_client()

            mock_client.GET.assert_called_once_with(query_param = 10)
    
    '''
    return MockServiceClient()

class MockServiceClient(object):


    def __init__(self):
        # We maintain a mapping from url to MockPathContext objects. Each
        # unique url gets an unique context. The context provides the
        # MagicMock used to mock the http methods (GET, etc.) in all Path 
        # objects initialied with that url.
        self.__context_map = {}


    # when used as a function decorator
    def __call__(self, func):

        my_self = self

        @functools.wraps(func)
        def wrapper(*args, **kwargs):
            with my_self:
                new_args = args + (my_self,)
                return func(*new_args, **kwargs)

        return wrapper


    # when used as a context object
    def __enter__(self):
        self.__patchers = self.__make_patchers()
        for patcher in self.__patchers: 
            patcher.start()
        return self


    # when used as a context object
    def __exit__(self, *args, **kwargs):
        if self.__patchers:
            for patcher in self.__patchers:
                patcher.stop()


    def __make_patchers(self):

        # Method that will be calld to create new instances of Path objcets
        def new_path(url, **kwargs):

            # Get the context for the url, creting them as necessary.
            mock_context = self.__context_map.get(url)
            if mock_context is None:
                mock_context = MockPathContext(url)
                self.__context_map[url] = mock_context

            # Pass the context to a new mock path object.
            return MockPath(mock_context, url, **kwargs)

        # Patch the Path constructor method so that it calls the method above instead.
        return (
            real_mock.patch('cgf_service_client.path.Path', side_effect = new_path),
            real_mock.patch('cgf_service_client.Path', side_effect = new_path)
        )


    def for_url(self, url):
        return cgf_service_client.Path(url) # should be patched by now, so a Mockpath is created


class MockPathContext(object):

    def __init__(self, url):

        # Each url gets a MockPathContext, and for each of those we keep track of 
        # the config object used to make each request and provide the shared 
        # MagicMock used for http methods on that url.

        self.__get_call_configs = []
        self.__get_method_mock = real_mock.MagicMock(name = 'GET ' + url)

        self.__put_call_configs = []
        self.__put_method_mock = real_mock.MagicMock(name = 'PUT ' + url)

        self.__post_call_configs = []
        self.__post_method_mock = real_mock.MagicMock(name = 'POST ' + url)

        self.__delete_call_configs = []
        self.__delete_method_mock = real_mock.MagicMock(name = 'DELETE ' + url)


    def bind(self, mock_path):
        
        # Override the http methods on the provided MockPath object with an 
        # HttpMethodMock object. That object will forward all calls to the 
        # mock to the method mock we provide, except __call__, which it
        # overrides to capture the CONFIG from the MockPath object each 
        # time the mock is invoked.

        setattr(mock_path, 'GET', HttpMethodMock(mock_path, self.__get_call_configs, self.__get_method_mock))
        setattr(mock_path, 'PUT', HttpMethodMock(mock_path, self.__put_call_configs, self.__put_method_mock))
        setattr(mock_path, 'POST', HttpMethodMock(mock_path, self.__post_call_configs, self.__post_method_mock))
        setattr(mock_path, 'DELETE', HttpMethodMock(mock_path, self.__delete_call_configs, self.__delete_method_mock))


class HttpMethodMock(object):


    def __init__(self, mock_path, call_configs, shared_mock):

        # completely wrap the shared_mock
        # from https://stackoverflow.com/questions/1443129/completely-wrap-an-object-in-python

        self.__dict__ = shared_mock.__dict__

        self.__class__ = type(
            shared_mock.__class__.__name__,
            (self.__class__, shared_mock.__class__),
            {}
        )

        self.__mock_path = mock_path
        self.__call_configs = call_configs
        self.__shared_mock = shared_mock
        self.__return_data = None


    def _get_child_mock(self, **kw):
        # Override so that any mock objects created by the mock are real MagicMock 
        # objects, not our warapper.
        return real_mock.MagicMock(**kw)


    @property
    def return_json(self):
        '''Dict that will be wrapped by a Data object and returned when the method is called,
        or a list of dict that will be returned by a sequence of calls.'''
        return self.__return_data


    @return_json.setter
    def return_json(self, value):
        self.__return_data = value
        if isinstance(value, list):
            self.side_effect = [ Data(entry, read_only = True, PATH = self.__mock_path, RESPONSE = real_mock.MagicMock(name='response')) for entry in value ]
        else:
            self.return_value = Data(value, read_only = True, PATH = self.__mock_path, RESPONSE = real_mock.MagicMock(name='response')) 


    def __call__(self, *args, **kwargs):
        self.__call_configs.append(self.__mock_path.config)
        result = self.__shared_mock(*args, **kwargs)
        return result


    @property
    def mock_call_configs(self):
        return self.__call_configs


    def assert_all_calls_have_config_containing(self, **kwargs):
        for call_config in self.__call_configs:
            self.__assert_call_config_contains(call_config, kwargs)


    def __assert_call_config_contains(self, call_config, kwargs):

        for k, v in kwargs.iteritems():
            
            assert k in call_config.DATA, 'Expected {} configuration to include a value for {}, but there is no value.'.format(
                self._mock_name,
                k
            )

            assert v == call_config.DATA[k], 'Expected {} configuration {} value to be {}, but there it was {}.'.format(
                self._mock_name,
                k,
                v,
                call_config.DATA[k]
            )



class MockPath(cgf_service_client.Path):

    def __init__(self, mock_context, url, **kwargs):
        super(MockPath, self).__init__(url, **kwargs)
        mock_context.bind(self)

    def __repr__(self):
        return 'MockPath({})'.format(str(self))

    