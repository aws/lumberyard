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
# $Revision: #1 $

import unittest
from unittest import mock
from six.moves import http_client

from cgf_utils import custom_resource_response


class UnitTest_CloudGemFramework_ProjectResourceHandler_custom_resource_response(unittest.TestCase):

    def test_succeed(self):

        event = {
            'StackId': 'test-stack-id',
            'RequestId': 'test-request-id',
            'LogicalResourceId': 'test-logical-resource-id',
            'ResponseURL': 'https://test-host/test-path/test-path?test-arg=test-value'
        }

        context = {
        }

        data = {
            'test-data-key': 'test-data-value'
        }

        physical_resource_id = 'test-physical-resource-id'

        with mock.patch('six.moves.http_client.HTTPSConnection') as mock_HTTPSConnection:

            mock_connection = mock_HTTPSConnection.return_value
            mock_getresponse = mock_connection.getresponse

            mock_response = mock.MagicMock()
            mock_response.status = http_client.OK
            mock_getresponse.return_value = mock_response

            mock_request = mock_connection.request

            custom_resource_response.succeed(event, context, data, physical_resource_id)

            mock_HTTPSConnection.assert_called_once_with('test-host')
            expected_params = '{"Status": "SUCCESS", "PhysicalResourceId": "test-physical-resource-id", "StackId": "test-stack-id", ' \
                              '"RequestId": "test-request-id", "LogicalResourceId": "test-logical-resource-id", "Data": {"test-data-key": "test-data-value"}}'
            mock_request.assert_called_with('PUT', '/test-path/test-path?test-arg=test-value', expected_params)

    def test_failed_without_physical_resource_id(self):

        event = {
            'StackId': 'test-stack-id',
            'RequestId': 'test-request-id',
            'LogicalResourceId': 'test-logical-resource-id',
            'ResponseURL': 'https://test-host/test-path'
        }

        context = {
        }

        reason = 'test-reason'

        with mock.patch('six.moves.http_client.HTTPSConnection') as mock_HTTPSConnection:

            mock_connection = mock_HTTPSConnection.return_value
            mock_getresponse = mock_connection.getresponse

            mock_response = mock.MagicMock()
            mock_response.status = http_client.OK
            mock_getresponse.return_value = mock_response

            mock_request = mock_connection.request

            custom_resource_response.fail(event, context, reason)

            mock_HTTPSConnection.assert_called_once_with('test-host')
            expected_params = '{"Status": "FAILED", "StackId": "test-stack-id", "RequestId": "test-request-id", ' \
                              '"LogicalResourceId": "test-logical-resource-id", "PhysicalResourceId": "test-request-id", "Reason": "test-reason"}'
            mock_request.assert_called_with('PUT', '/test-path', expected_params)

    def test_failed_with_physical_resource_id(self):

        event = {
            'StackId': 'test-stack-id',
            'RequestId': 'test-request-id',
            'LogicalResourceId': 'test-logical-resource-id',
            'PhysicalResourceId': 'test-physical-resource-id',
            'ResponseURL': 'https://test-host/test-path'
        }

        context = {
        }

        reason = 'test-reason'

        with mock.patch('six.moves.http_client.HTTPSConnection') as mock_HTTPSConnection:

            mock_connection = mock_HTTPSConnection.return_value
            mock_getresponse = mock_connection.getresponse

            mock_response = mock.MagicMock()
            mock_response.status = http_client.OK
            mock_getresponse.return_value = mock_response

            mock_request = mock_connection.request

            custom_resource_response.fail(event, context, reason)

            mock_HTTPSConnection.assert_called_once_with('test-host')
            expected_params = '{"Status": "FAILED", "StackId": "test-stack-id", "RequestId": "test-request-id", "LogicalResourceId": ' \
                              '"test-logical-resource-id", "PhysicalResourceId": "test-physical-resource-id", "Reason": "test-reason"}'

            mock_request.assert_called_with('PUT', '/test-path', expected_params)

    def test_retry(self):

        event = {
            'StackId': 'test-stack-id',
            'RequestId': 'test-request-id',
            'LogicalResourceId': 'test-logical-resource-id',
            'PhysicalResourceId': 'test-physical-resource-id',
            'ResponseURL': 'https://test-host/test-path'
        }

        context = {
        }

        reason = 'test-reason'

        with mock.patch('six.moves.http_client.HTTPSConnection') as mock_HTTPSConnection:

            mock_connection = mock_HTTPSConnection.return_value
            mock_getresponse = mock_connection.getresponse

            mock_failed_response = mock.MagicMock()
            mock_failed_response.status = http_client.INTERNAL_SERVER_ERROR
            mock_ok_response = mock.MagicMock()
            mock_ok_response.status = http_client.OK
            mock_exception_response = RuntimeError('test')
            mock_getresponse.side_effect = [mock_failed_response, mock_exception_response, mock_ok_response]

            mock_request = mock_connection.request

            custom_resource_response.fail(event, context, reason)

            mock_HTTPSConnection.assert_called_with('test-host')
            mock_HTTPSConnection.assert_called_with('test-host')
            mock_HTTPSConnection.assert_called_with('test-host')
            self.assertEqual(mock_HTTPSConnection.call_count, 3)

            expected_parameters = '{"Status": "FAILED", "StackId": "test-stack-id", "RequestId": "test-request-id", ' \
                                  '"LogicalResourceId": "test-logical-resource-id", "PhysicalResourceId": "test-physical-resource-id", "Reason": "test-reason"}'

            mock_request.assert_called_with('PUT', '/test-path', expected_parameters)
            mock_request.assert_called_with('PUT', '/test-path', expected_parameters)
            mock_request.assert_called_with('PUT', '/test-path', expected_parameters)
            self.assertEqual(mock_request.call_count, 3)

