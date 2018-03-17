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

from __future__ import print_function

import urlparse
import urllib

from copy import copy

from cgf_utils.data_utils import Data
from cgf_service_client import error

import requests
import requests_aws4auth


class Path(object):
    '''Provides an HTTP client optimized for use with REST style sevice apis
    authenticted using AWS IAM credentials.

    Usage Examples

      client = cgf_service_client.for_url('https://example.com/base')
      
      # GET https://example.com/base
      content = client.GET() # content = { 'Greeting': 'Hello World' }
      print(content.Greeting) # use attribute name instead of ['']

      # GET http://example.com/base/foo
      new_client = client.navigate('foo')
      content = new_client.GET()
        - or - 
      content = client.navigate('foo').GET()

      # GET http://example.com/base/a/b/c/d
      content = client.navigate('a', 'b', 'c', 'd').GET()

      # GET http://example.com/base/http%3A%2F%2Fquoted
      content = client.navigate('http://quoted').GET()

      # GET http://example.com/base?foo=10&bar=20
      content = client.GET(foo=10, bar=20)

      # POST http://example.com/base with JSON body
      result = client.POST({'foo': { 'bar': 10 } })
        - or - 
      body = cgf_service_client.Data()
      body.foo.bar = 10
      result = client.POST(body)

      # POST http://example.com/base?foo=10&bar=20 with JSON body
      result = client.POST({'foo': 'bar'}, foo=10, bar=20)

      # PUT http://example.com/base with JSON body
      result = client.PUT({'foo': 'bar'})

      # DELETE http://example.com/base
      client.DELETE()

      # GET http://example.com/base/foo/bar as relative url
      content = client.GET() # content = { 'RelativeUrl': 'bar' }
      relative_content = client.apply(content.RelativeUrl).GET()




  Error Handling
        
      HTTP status codes in the range 200-299 will cause the request method (GET,
      PUT, etc.) to return the response content normally. Any other status code
      will result in one of the following exceptiosn being thrown.

          HttpError                
            ClientError            400-499
              NotAllowedError      401, 403
              NotFoundError        404
            ServerError            500-599

    '''

    def __init__(self, url, **kwargs):
        '''Initializes a path object.

        Arguments:

          - url: the url represented by the Path object.

          - kwargs: the configuration used by the Path object. The following 
          configuration properties are supported:

            - session: a boto3.Session object that has the aws credentials and
            region used to sign the request. If not present, the request is not
            signed.

            - role_arn: identifies a role to be assumed before making the request.
            The credentials provided by the session property are used to assume 
            the role, and the requesting credentials are used to make the request.

            - role_session_name: the session name used when assuming the role. If
            not set, then 'cgf_service_client' is used.

            - verbose: enables logging of requests and responses.

        '''

        # TODO: add support for a "spec" configuration item. A spec is a description 
        # of the child paths and operations supported by this path. A spec can be
        # generated from a swagger api description, 'compiled' represtantion of that
        # data.
        #
        # Such data isn't that useful in production envrionments but can be invaluable
        # during development. For example, the Path object could popluate itself with
        # attributes representing the expected paths, and delete the attributes 
        # representing unsupported operations. This would allow you to use a REPL 
        # prompt to easily explore an api.
        #
        # It could also be used to validate request and response data, which helps 
        # detect and isoate issues early in development. However, this verification
        # usually isn't worth the overhead in production, after everything is working. 
        # 
        # A spec for an api would look something like the following. It is basically
        # a tree where the keys are the allowed attributes at each location. Each
        # node can be annotated with metadata denoted using $key$. 
        #
        #   {
        #       "foo": {
        #           "{param_name}": {
        #               "$type$": "string",
        #               "bar": {
        #                   "POST": {
        #                       "$body$": {                    <-- denormalized type definition
        #                           "$type$": "MyType",
        #                           "MyProp": {
        #                               "$type$": "string"
        #                           }
        #                       },
        #                       "MyQueryParam": {
        #                           "$type$": "integer"
        #                       },
        #                       "$result$": {                  <-- denormalized type definition
        #                           "$type$": "MyType",
        #                           "MyProp": {
        #                               "$type$": "string"
        #                           }
        #                       }
        #                   }
        #               }
        #           }
        #       }
        #   }
        # 
        # A key feature of this approach is the denormalization of the type data, seen
        # in this example where the POST operation returns the same type of data as is 
        # provied in the body. This allows each child path to be passed child nodes from
        # the spec without worrying about looksing track of type definitinos. In practice 
        # the same object would be refeferenced from multiple locations the in spec, so
        # there isn't a lot of memory being used.
        #
        # Some special handling of '..' would be needed if you want spec data to flow up
        # the path. Currently we don't keep track of the parent of the path, meaning:
        #  
        #    assertFalse( foo.bar.APPLY('..') is foo )
        #
        # This can lead to some unexpected behavior where configuration is concerned:
        #
        #    foo = Path(url, A = 1)
        #    assertEquals(foo.CONFIG.A, 1)
        #    bar = foo.APPLY('bar', A = 2)
        #    assertEquals(bar.CONFIG.A, 2)
        #    foo2 = bar.APPLY('..')
        #    assertEquals(foo2.CONFIG.A, 2)
        #    assertEquals(foo2.URL, foo.URL)
        #    assertNotEquals(foo2.CONFIG.A, foo.CONFIG.A)
        #    assertNotEquals(foo2, foo)
        #

        if url.endswith('/'):
            url = url[:-1]

        self.__url = url

        # Apply passes along it's current config using the __config key word argument
        # so that it can be reused. If that arg isn't present, kwargs is the config.
        self.__config = kwargs.get('__config', Data(kwargs))


    @property
    def url(self):
        '''The URL represented by this Path object.'''
        return self.__url


    @property
    def config(self):
        '''The configuration used by this Path object.'''
        return self.__config


    def GET(self, **kwargs):
        '''Perform an HTTP GET request using the Path object's URL.
        
        Arguments:

          - **kwargs: the query string paramters used in the request.

        Returns:

          - Value decoded from returned JSON

        Raises:

          - cgf_service_client.HttpError, or one if it's subclasses, if an HTTP
          error occurs.
        '''
        return self.__service_request(requests.get, params = kwargs)


    def POST(self, body, **kwargs):
        '''Perform an HTTP POST request using the Path object's URL.
        
        Arguments:

          - body: value that will be encoded as JSON and sent with the request.

          - **kwargs: the query string paramters used in the request.

        Returns:

          - Value decoded from returned JSON

        Raises:

          - cgf_service_client.HttpError, or one if it's subclasses, if an HTTP
          error occurs.
        '''
        return self.__service_request(requests.post, body, params = kwargs)


    def PUT(self, body, **kwargs):
        '''Perform an HTTP PUT request using the Path object's URL.
        
        Arguments:

          - body: value that will be encoded as JSON and sent with the request.

          - **kwargs: the query string paramters used in the request.

        Returns:

          - Value decoded from returned JSON

        Raises:

          - cgf_service_client.HttpError, or one if it's subclasses, if an HTTP
          error occurs.
        '''
        return self.__service_request(requests.put, body, params = kwargs)


    def DELETE(self, body, **kwargs):
        '''Perform an HTTP POST request using the Path object's URL.
        
        Arguments:

          - **kwargs: the query string paramters used in the request.

        Returns:

          - Value decoded from returned JSON

        Raises:

          - cgf_service_client.HttpError, or one if it's subclasses, if an HTTP
          error occurs.
        '''
        return self.__service_request(requests.delete, body, params = kwargs)

    def navigate(self, *args):
        '''Create a client object for an url that is this client's url with path
        chagnes applied.

        Arguments:

          *args -- positional arguments are joined with the current url to produce
          the new client's url. The values provided are url quoted (reserved url
          characters are replaced with codes) before being appended to to url. Use 
          apply if you don't want the arguments url quoated.

        Returns a new client object for the specified url.
        '''
        return self.apply(*[urllib.quote_plus(item) for item in args])

    def apply(self, *args):
        '''Create a new client object for an url that is this client's url with 
        url path changes applied.

        Arguments:

          *args -- positional arguments are joined with the current url produce the 
          new client's url. A value starting with / will reset the path, keeping
          the scheme and host. A value starting with a scheme will will reset the
          entire url. A value of .. will move up one level in the path.
          
          The values provided should already have url quoting applied if that is
          desired. Use navigate instead to apply url quoting automatically.

        Returns a new client object for the specified url.
        '''
        
        new_url = self.__url
        for arg in args:
            expanded_args = arg.split('/')
            if arg.startswith('/'):
                expanded_args[0] = '/' + expanded_args[0]
            for arg in expanded_args:
                if arg == '..' and new_url.count('/') <= 2:
                    raise ValueError('Path segment .. cannot be applied to the root: {}'.format(self.__url))
                new_url = urlparse.urljoin(new_url + '/', arg)
                if new_url.endswith('/'):
                    new_url = new_url[:-1]
        
        return Path(new_url, __config = self.__config)


    def configure(self, **kwargs):
        '''Create a new client object with a modified configuration.

        Arguments:

          **kwargs -- key word arguments are used to update the current
          configuration to produce the a configuration.
        '''

        new_config = copy(self.__config.DATA)
        new_config.update(kwargs)

        return Path(self.__url, **new_config)


    def __eq__(self, other):
        return isinstance(other, Path)           \
            and other.__url == self.__url        \
            and other.__config == self.__config


    def __str__(self):
        return self.__url


    def __repr__(self):
        return 'Path({})'.format(self.__url)
    

    def __service_request(self, method, body = None, params = None):
        '''Submits a network requests and waits for a response.

        Arguments:

            method - the functin to be called to execute the request.

            body - the json content submited with the request

            params - query string parameters sent with the request

        Returns:

            The json content from response wrapped by a Data object. The returned 
            object will have a PATH attribute that is this path object and a RESPONSE 
            attribute that is the response object for the http request. The Data
            object will be read only.

        '''

        if self.__config.session:
            if self.__config.role_arn:

                sts = self.__config.session.client('sts')

                res = sts.assume_role(
                    RoleArn = self.__config.role_arn, 
                    RoleSessionName = self.__config.role_session_name or 'cgf_service_client'
                )

                access_key = res['Credentials']['AccessKeyId']
                secret_key = res['Credentials']['SecretAccessKey']
                session_token = res['Credentials']['SessionToken']
                auth_description = self.__config.role_arn

            else:       
                
                session_credentials = self.__config.session.get_credentials().get_frozen_credentials()
                
                access_key = session_credentials.access_key
                secret_key = session_credentials.secret_key
                session_token = session_credentials.token
                auth_description = 'session'

            auth = requests_aws4auth.AWS4Auth(
                access_key, 
                secret_key, 
                self.__config.session.region_name, 
                'execute-api',
                session_token=session_token
            )

        else:

            auth = None
            auth_description = None
        
        if self.__config.verbose:
            print('HTTP', method.__name__.upper(), self.__url, params, body, auth_description)

        response = method(self.__url, params=params, json=body, auth=auth)

        if self.__config.verbose:
            print('  -->', response.status_code, response.text)

        error.HttpError.for_response(response)

        if response.content:

            result = response.json()

            # TODO: the generated api gateway lambda displatch mapping injects this 'result' thing
            # in there, it shouldn't do this because it breaks the contract defined by the swagger.
            # To fix it, we need to:
            #
            # 1) change swagger_processor.lambda_dispatch to return the response provided by the
            # lambda directly. Problem is that breaks the additional_response_template_content feature,
            # which can probabally be removed all together.
            #
            # 2) change the c++ client generator handle the missing result property
            #
            # 3) change cgp to handle the missing result property
            #
            # 4) remove the following code
            #
            result = result.get('result', None) 
            if result is None:
                return RuntimeError('Service response did not include the stupid result property.')
            #
            # END TODO

            return Data(result, read_only = True, PATH = self, RESPONSE = response)

        else:

            return None
