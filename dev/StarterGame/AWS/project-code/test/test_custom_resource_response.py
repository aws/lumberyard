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

import custom_resource_response

class TestCustomResourceResponse(unittest.TestCase):

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

            mock_HTTPSConnection.assert_called_with('test-host')
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

            mock_HTTPSConnection.assert_called_with('test-host')
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

            mock_HTTPSConnection.assert_called_with('test-host')
            mock_request.assert_called_with('PUT', '/test-path', '{"Status": "FAILED", "StackId": "test-stack-id", "Reason": "test-reason", "PhysicalResourceId": "test-physical-resource-id", "RequestId": "test-request-id", "LogicalResourceId": "test-logical-resource-id"}')


