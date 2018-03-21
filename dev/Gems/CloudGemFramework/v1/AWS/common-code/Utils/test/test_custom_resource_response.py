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
import mock
import httplib

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

        physical_resource_id = 'test-physical-resoruce-id'

        with mock.patch('httplib.HTTPSConnection') as mock_HTTPSConnection:

            mock_connection = mock_HTTPSConnection.return_value
            mock_getresponse = mock_connection.getresponse

            mock_response = mock.MagicMock()
            mock_response.status = httplib.OK
            mock_getresponse.return_value = mock_response

            mock_request = mock_connection.request

            custom_resource_response.succeed(event, context, data, physical_resource_id)

            mock_HTTPSConnection.assert_called_once_with('test-host')
            mock_request.assert_called_with('PUT', '/test-path/test-path?test-arg=test-value', '{"Status": "SUCCESS", "StackId": "test-stack-id", "PhysicalResourceId": "test-physical-resoruce-id", "RequestId": "test-request-id", "Data": {"test-data-key": "test-data-value"}, "LogicalResourceId": "test-logical-resource-id"}')


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

        with mock.patch('httplib.HTTPSConnection') as mock_HTTPSConnection:

            mock_connection = mock_HTTPSConnection.return_value
            mock_getresponse = mock_connection.getresponse

            mock_response = mock.MagicMock()
            mock_response.status = httplib.OK
            mock_getresponse.return_value = mock_response

            mock_request = mock_connection.request

            custom_resource_response.fail(event, context, reason)

            mock_HTTPSConnection.assert_called_once_with('test-host')
            mock_request.assert_called_with('PUT', '/test-path', '{"Status": "FAILED", "StackId": "test-stack-id", "Reason": "test-reason", "PhysicalResourceId": "test-request-id", "RequestId": "test-request-id", "LogicalResourceId": "test-logical-resource-id"}')


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

        with mock.patch('httplib.HTTPSConnection') as mock_HTTPSConnection:

            mock_connection = mock_HTTPSConnection.return_value
            mock_getresponse = mock_connection.getresponse

            mock_response = mock.MagicMock()
            mock_response.status = httplib.OK
            mock_getresponse.return_value = mock_response

            mock_request = mock_connection.request

            custom_resource_response.fail(event, context, reason)

            mock_HTTPSConnection.assert_called_once_with('test-host')
            mock_request.assert_called_with('PUT', '/test-path', '{"Status": "FAILED", "StackId": "test-stack-id", "Reason": "test-reason", "PhysicalResourceId": "test-physical-resource-id", "RequestId": "test-request-id", "LogicalResourceId": "test-logical-resource-id"}')

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

        with mock.patch('httplib.HTTPSConnection') as mock_HTTPSConnection:

            mock_connection = mock_HTTPSConnection.return_value
            mock_getresponse = mock_connection.getresponse

            mock_failed_response = mock.MagicMock()
            mock_failed_response.status = httplib.INTERNAL_SERVER_ERROR
            mock_ok_response = mock.MagicMock()
            mock_ok_response.status = httplib.OK
            mock_exception_response = RuntimeError('test')
            mock_getresponse.side_effect = [ mock_failed_response, mock_exception_response, mock_ok_response ]

            mock_request = mock_connection.request

            custom_resource_response.fail(event, context, reason)

            mock_HTTPSConnection.assert_called_with('test-host')
            mock_HTTPSConnection.assert_called_with('test-host')
            mock_HTTPSConnection.assert_called_with('test-host')
            self.assertEquals(mock_HTTPSConnection.call_count, 3)

            mock_request.assert_called_with('PUT', '/test-path', '{"Status": "FAILED", "StackId": "test-stack-id", "Reason": "test-reason", "PhysicalResourceId": "test-physical-resource-id", "RequestId": "test-request-id", "LogicalResourceId": "test-logical-resource-id"}')
            mock_request.assert_called_with('PUT', '/test-path', '{"Status": "FAILED", "StackId": "test-stack-id", "Reason": "test-reason", "PhysicalResourceId": "test-physical-resource-id", "RequestId": "test-request-id", "LogicalResourceId": "test-logical-resource-id"}')
            mock_request.assert_called_with('PUT', '/test-path', '{"Status": "FAILED", "StackId": "test-stack-id", "Reason": "test-reason", "PhysicalResourceId": "test-physical-resource-id", "RequestId": "test-request-id", "LogicalResourceId": "test-logical-resource-id"}')
            self.assertEquals(mock_request.call_count, 3)

