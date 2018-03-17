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

import unittest
import mock

import requests
import requests_aws4auth

from cgf_service_client.path import Path
from cgf_service_client import error

class UnitTest_CloudGemFramework_ServiceClient_Path(unittest.TestCase):

    def test_init_stores_args(self):

        target = Path('http://example.com/base', Foo = 10, Bar = 20)

        self.assertEquals(target.url, 'http://example.com/base')
        self.assertEquals(target.config.Foo, 10)
        self.assertEquals(target.config.Bar, 20)


    def test_init_removes_slash_if_url_ends_with_slash(self):

        target = Path('http://example.com/base/')

        self.assertEquals(target.url, 'http://example.com/base')


    def test_str(self):
        target = Path('http://example.com/base')
        self.assertEquals(str(target), 'http://example.com/base')


    def test_apply(self):

        target = Path('http://example.com/a/b')

        self.assertEquals(target.apply('c', 'd').url, 'http://example.com/a/b/c/d')
        self.assertEquals(target.apply('c/d').url, 'http://example.com/a/b/c/d')
        self.assertEquals(target.apply('/root').url, 'http://example.com/root')
        self.assertEquals(target.apply('..', 'c', 'd').url, 'http://example.com/a/c/d')
        self.assertEquals(target.apply('../c/d').url, 'http://example.com/a/c/d')
        self.assertEquals(target.apply('x/y/../../c/d').url, 'http://example.com/a/b/c/d')
        self.assertEquals(target.apply('..', '..').url, 'http://example.com')
        self.assertEquals(target.apply('../..').url, 'http://example.com')
        self.assertEquals(target.apply('..', '..', 'c', 'd').url, 'http://example.com/c/d')
        self.assertEquals(target.apply('../../c/d').url, 'http://example.com/c/d')

        with self.assertRaises(ValueError):
            target.apply('..', '..', '..')

        with self.assertRaises(ValueError):
            target.apply('../../..')


    def test_configure(self):

        target = Path('http://example.com/a/b', A = 10, B = 20)

        self.assertEquals(target.configure(B = 30, C = 40).config.DATA, { 'A': 10, 'B': 30, 'C': 40 })
        self.assertEquals(target.config.DATA, { 'A': 10, 'B': 20 })

     
    def test_navigate(self):

        target = Path('http://example.com/a/b')

        self.assertEquals(target.navigate('c').url, 'http://example.com/a/b/c')
        self.assertEquals(target.navigate('c', 'http://quoted').url, 'http://example.com/a/b/c/http%3A%2F%2Fquoted')


    def test_eq(self):

        self.assertEqual(Path('http://example.com'), Path('http://example.com'))
        self.assertNotEqual(Path('http://example.com'), Path('http://example.com/a'))

        self.assertEqual(Path('http://example.com', A = 10), Path('http://example.com', A = 10))
        self.assertNotEqual(Path('http://example.com', A = 10), Path('http://example.com', A = 20))


    def test_GET(self):

        target = Path('http://example.com/a/b')

        with mock.patch.object(target, '_Path__service_request') as mock_service_request:

            response = target.GET(A = 10, B = 20)

            mock_service_request.assert_called_once_with(requests.get, params = { 'A': 10, 'B': 20 })
            self.assertEquals(response, mock_service_request.return_value)


    def test_PUT(self):

        target = Path('http://example.com/a/b')

        with mock.patch.object(target, '_Path__service_request') as mock_service_request:

            body = { 'x': 'y' }

            response = target.PUT(body, A = 10, B = 20)

            mock_service_request.assert_called_once_with(requests.put, body, params = { 'A': 10, 'B': 20 })
            self.assertEquals(response, mock_service_request.return_value)


    def test_POST(self):

        target = Path('http://example.com/a/b')

        with mock.patch.object(target, '_Path__service_request') as mock_service_request:

            body = { 'x': 'y' }

            response = target.POST(body, A = 10, B = 20)

            mock_service_request.assert_called_once_with(requests.post, body, params = { 'A': 10, 'B': 20 })
            self.assertEquals(response, mock_service_request.return_value)


    def test_DELETE(self):

        target = Path('http://example.com/a/b')

        with mock.patch.object(target, '_Path__service_request') as mock_service_request:

            response = target.DELETE(A = 10, B = 20)

            mock_service_request.assert_called_once_with(requests.delete, params = { 'A': 10, 'B': 20 })
            self.assertEquals(response, mock_service_request.return_value)


    def test_service_request_without_session(self):

        target = Path('http://example/com/a/b')

        mock_method = mock.MagicMock()
        mock_method.return_value.json.return_value = {'result': {'x': 'y'}}
        mock_method.return_value.status_code = 200
        mock_body = mock.MagicMock()
        mock_params = mock.MagicMock()

        response = target._Path__service_request(mock_method, mock_body, mock_params)

        mock_method.assert_called_once_with('http://example/com/a/b', auth = None, json = mock_body, params = mock_params)
        self.assertEquals(response.DATA, {'x': 'y'})


    @mock.patch('requests_aws4auth.AWS4Auth')
    def test_service_request_with_session(self, mock_AWS4Auth):

        mock_session = mock.MagicMock()

        target = Path('http://example/com/a/b', session = mock_session)

        mock_method = mock.MagicMock()
        mock_method.return_value.json.return_value = {'result': {'x': 'y'}}
        mock_method.return_value.status_code = 200
        mock_body = mock.MagicMock()
        mock_params = mock.MagicMock()

        response = target._Path__service_request(mock_method, mock_body, mock_params)

        mock_AWS4Auth.assert_called_once_with(
            mock_session.get_credentials.return_value.get_frozen_credentials.return_value.access_key,
            mock_session.get_credentials.return_value.get_frozen_credentials.return_value.secret_key,
            mock_session.region_name,
            'execute-api',
            session_token = mock_session.get_credentials().get_frozen_credentials().token
        )

        mock_method.assert_called_once_with('http://example/com/a/b', auth = mock_AWS4Auth.return_value, json = mock_body, params = mock_params)
        self.assertEquals(response.DATA, {'x': 'y'})


    @mock.patch('requests_aws4auth.AWS4Auth')
    def test_service_request_with_role_arn(self, mock_AWS4Auth):

        mock_session = mock.MagicMock()
        mock_role_arn = mock.MagicMock()

        mock_session.client.return_value.assume_role.return_value = {
            'Credentials': {
                'AccessKeyId': mock.MagicMock(),
                'SecretAccessKey': mock.MagicMock(),
                'SessionToken': mock.MagicMock()
            }
        }

        target = Path('http://example/com/a/b', session = mock_session, role_arn = mock_role_arn)

        mock_method = mock.MagicMock()
        mock_method.return_value.json.return_value = {'result': {'x': 'y'}}
        mock_method.return_value.status_code = 200
        mock_body = mock.MagicMock()
        mock_params = mock.MagicMock()

        response = target._Path__service_request(mock_method, mock_body, mock_params)

        mock_session.client.assert_called_once_with('sts')
        mock_session.client.return_value.assume_role.assert_called_once_with(
            RoleArn = mock_role_arn,
            RoleSessionName = 'cgf_service_client'
        )

        mock_AWS4Auth.assert_called_once_with(
            mock_session.client.return_value.assume_role.return_value['Credentials']['AccessKeyId'],
            mock_session.client.return_value.assume_role.return_value['Credentials']['SecretAccessKey'],
            mock_session.region_name,
            'execute-api',
            session_token = mock_session.client.return_value.assume_role.return_value['Credentials']['SessionToken']
        )

        mock_method.assert_called_once_with('http://example/com/a/b', auth = mock_AWS4Auth.return_value, json = mock_body, params = mock_params)
        self.assertEquals(response.DATA, {'x': 'y'})


    def test_service_request_with_400_status_code(self):

        target = Path('http://example/com/a/b')

        mock_method = mock.MagicMock()
        mock_method.return_value.json.return_value = {'result': {'x': 'y'}}
        mock_method.return_value.status_code = 400
        mock_body = mock.MagicMock()
        mock_params = mock.MagicMock()

        with self.assertRaises(error.ClientError):
            target._Path__service_request(mock_method, mock_body, mock_params)


    def test_service_request_with_401_status_code(self):

        target = Path('http://example/com/a/b')

        mock_method = mock.MagicMock()
        mock_method.return_value.json.return_value = {'result': {'x': 'y'}}
        mock_method.return_value.status_code = 401
        mock_body = mock.MagicMock()
        mock_params = mock.MagicMock()

        with self.assertRaises(error.NotAllowedError):
            target._Path__service_request(mock_method, mock_body, mock_params)


    def test_service_request_with_404_status_code(self):

        target = Path('http://example/com/a/b')

        mock_method = mock.MagicMock()
        mock_method.return_value.json.return_value = {'result': {'x': 'y'}}
        mock_method.return_value.status_code = 404
        mock_body = mock.MagicMock()
        mock_params = mock.MagicMock()

        with self.assertRaises(error.NotFoundError):
            target._Path__service_request(mock_method, mock_body, mock_params)


    def test_service_request_with_500_status_code(self):

        target = Path('http://example/com/a/b')

        mock_method = mock.MagicMock()
        mock_method.return_value.json.return_value = {'result': {'x': 'y'}}
        mock_method.return_value.status_code = 500
        mock_body = mock.MagicMock()
        mock_params = mock.MagicMock()

        with self.assertRaises(error.ServerError):
            target._Path__service_request(mock_method, mock_body, mock_params)


    def test_service_request_with_999_status_code(self):

        target = Path('http://example/com/a/b')

        mock_method = mock.MagicMock()
        mock_method.return_value.json.return_value = {'result': {'x': 'y'}}
        mock_method.return_value.status_code = 999
        mock_body = mock.MagicMock()
        mock_params = mock.MagicMock()

        with self.assertRaises(error.HttpError):
            target._Path__service_request(mock_method, mock_body, mock_params)



if __name__ == '__main__':
    unittest.main()
