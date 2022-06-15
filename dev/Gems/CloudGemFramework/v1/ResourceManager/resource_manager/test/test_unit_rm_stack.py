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
import datetime
import unittest
from unittest import mock

import boto3
import botocore
from botocore.stub import Stubber

from resource_manager.errors import HandledError
from resource_manager.stack import Monitor
from resource_manager.stack import StackContext
from resource_manager.stack import StackOperationException

# Constants for tests
TEST_STACK_NAME = "TestStackName"
TEST_REGION = "TestRegion"
TEST_UUID = "90ce8630-ec89-11ea-930d-0a7bf50fa3c2"
TEST_ACCOUNT = "0123456789"
TEST_STACK_ID = f"arn:aws:cloudformation:{TEST_REGION}:{TEST_ACCOUNT}:stack/{TEST_STACK_NAME}/{TEST_UUID}"

# boto3.client requires a region even for stubbers. Actual region is not important
# Removes reliance on .aws setting or environment variable
TEST_STUB_REGION = 'us-east-1'


def _get_cf_stubbed_client(method: str, response, expected_params: dict):
    """Generate a CF subber"""
    client = boto3.client('cloudformation', region_name=TEST_STUB_REGION)
    cf_stubber = Stubber(client)
    cf_stubber.add_response(method, response, expected_params)
    cf_stubber.activate()
    return client


def _get_cf_stubbed_client_with_error(method: str, service_error_code: str, service_message: str = None):
    client = boto3.client('cloudformation', region_name=TEST_STUB_REGION)
    cf_stubber = Stubber(client)
    cf_stubber.add_client_error(
        method=method,
        service_error_code=service_error_code,
        service_message=service_message
    )
    cf_stubber.activate()
    return client


def _set_up_context(capabilities=None, mock_client=None):
    """
    Set up the Context object that drives everything from a CloudCanvas POV

    :param capabilities: A list of CloudFormation capabilities to mock
    :return: A context object that can be configured in the test
    """
    if capabilities is None:
        capabilities = []

    context = mock.MagicMock()
    context.config = mock.MagicMock()
    context.stack = mock.MagicMock()
    context.stack.confirm_stack_operation.return_value = capabilities

    context.config.local_project_settings = mock.MagicMock()

    context.config.project_template_aggregator = mock.MagicMock()
    context.config.project_template_aggregator.effective_template = {}

    context.aws = mock.MagicMock()
    context.aws.client.return_value = mock.MagicMock() if mock_client is None else mock_client
    return context


class UnitTest_StackOperationException(unittest.TestCase):
    def test_create(self):
        # GIVEN
        expected_resources = mock.MagicMock()
        expected_message = "EXPECTED_MESSAGE"

        # WHEN
        oe = StackOperationException(message=expected_message, failed_resources=expected_resources)

        # THEN
        self.assertEqual(oe.failed_resources, expected_resources)
        self.assertEqual(str(oe), expected_message)


class UnitTest_StackContext(unittest.TestCase):

    def test_context_create(self):
        # GIVEN
        context = _set_up_context()

        stack_context = StackContext(context)
        stack_context.initialize(args={})

        # THEN
        self.assertIsNotNone(stack_context)

    def test_context_name_exists(self):
        # GIVEN
        expected_params = {'StackName': TEST_STACK_NAME}
        response = self.__build_simple_describe_stack_response(stack_status='COMPLETED')

        client = _get_cf_stubbed_client(method='describe_stacks',
                                        response=response,
                                        expected_params=expected_params)

        context = _set_up_context(mock_client=client)
        stack_context = StackContext(context)

        # WHEN
        exists = stack_context.name_exists(stack_name=TEST_STACK_NAME, region=TEST_REGION)

        # THEN
        self.assertTrue(exists)

    def test_context_name_exists_is_false_for_deleted_stacks(self):
        # GIVEN
        expected_params = {'StackName': TEST_STACK_NAME}
        expected_status = 'DELETE_COMPLETE'
        response = self.__build_simple_describe_stack_response(stack_status=expected_status)

        client = _get_cf_stubbed_client(method='describe_stacks', response=response,
                                        expected_params=expected_params)

        context = _set_up_context(mock_client=client)
        stack_context = StackContext(context)

        # WHEN
        exists = stack_context.name_exists(stack_name=TEST_STACK_NAME, region=TEST_REGION)

        # THEN
        self.assertFalse(exists)

    def test_context_name_exists_is_false_for_validation_errors(self):
        # GIVEN
        client = _get_cf_stubbed_client_with_error(
            method='describe_stacks',
            service_error_code="ValidationError")

        context = _set_up_context(mock_client=client)
        stack_context = StackContext(context)

        # WHEN
        exists = stack_context.name_exists(stack_name=TEST_STACK_NAME, region=TEST_REGION)

        # THEN
        self.assertFalse(exists)

    def test_context_get_stack_status(self):
        # GIVEN
        expected_status = 'COMPLETED'
        expected_params = {'StackName': TEST_STACK_ID}
        response = self.__build_simple_describe_stack_response(stack_status=expected_status)

        client = _get_cf_stubbed_client(method='describe_stacks', response=response,
                                        expected_params=expected_params)

        context = _set_up_context(mock_client=client)
        stack_context = StackContext(context)

        # WHEN
        status = stack_context.get_stack_status(stack_id=TEST_STACK_ID)

        # THEN
        self.assertEqual(status, expected_status)

    def test_context_get_stack_status_client_failure(self):
        # GIVEN
        client = _get_cf_stubbed_client_with_error(
            method='describe_stacks',
            service_error_code="ClientError",
            service_message="Stack with id {TEST_STACK_ID} does not exist")

        context = _set_up_context(mock_client=client)
        stack_context = StackContext(context)

        # WHEN
        with self.assertRaises(botocore.exceptions.ClientError):
            stack_context.get_stack_status(stack_id=TEST_STACK_ID)

    def test_context_get_stack_status_validation_failure(self):
        # GIVEN
        client = _get_cf_stubbed_client_with_error(
            method='describe_stacks',
            service_error_code="ValidationError",
            service_message=f"Stack with id {TEST_STACK_ID} does not exist")

        context = _set_up_context(mock_client=client)
        stack_context = StackContext(context)

        # WHEN
        status = stack_context.get_stack_status(stack_id=TEST_STACK_ID)

        # THEN
        self.assertIsNone(status)

    def test_context_describe_stack(self):
        # GIVEN
        expected_params = {'StackName': TEST_STACK_ID}
        expected_status = 'COMPLETED'
        response = self.__build_simple_describe_stack_response(stack_status=expected_status)

        client = _get_cf_stubbed_client(
            method='describe_stacks',
            response=response,
            expected_params=expected_params)

        context = _set_up_context(mock_client=client)
        stack_context = StackContext(context)

        # WHEN
        response = stack_context.describe_stack(stack_id=TEST_STACK_ID)

        # THEN
        self.assertEqual(response['StackId'], TEST_STACK_ID)
        self.assertEqual(response['StackName'], TEST_STACK_NAME)
        self.assertEqual(response['StackStatus'], expected_status)

    def test_context_describe_stack_with_optional_and_validation_error(self):
        # GIVEN
        client = _get_cf_stubbed_client_with_error(
            method='describe_stacks',
            service_error_code='ValidationError'
        )

        context = _set_up_context(mock_client=client)
        stack_context = StackContext(context)

        # WHEN
        response = stack_context.describe_stack(stack_id=TEST_STACK_ID, optional=True)

        # THEN
        self.assertIsNone(response)

    def test_context_describe_stack_with_validation_error(self):
        # GIVEN
        client = _get_cf_stubbed_client_with_error(
            method='describe_stacks',
            service_error_code='ValidationError'
        )

        context = _set_up_context(mock_client=client)
        stack_context = StackContext(context)

        # WHEN
        with self.assertRaises(HandledError):
            stack_context.describe_stack(stack_id=TEST_STACK_ID, optional=False)

    def test_context_describe_stack_with_and_access_denied(self):
        # GIVEN
        client = _get_cf_stubbed_client_with_error(
            method='describe_stacks',
            service_error_code='AccessDenied'
        )

        context = _set_up_context(mock_client=client)
        stack_context = StackContext(context)

        # WHEN
        response = stack_context.describe_stack(stack_id=TEST_STACK_ID, optional=False)

        # THEN
        self.assertEqual(response['StackStatus'], 'UNKNOWN')
        self.assertEqual(response['StackStatusReason'], 'Access denied.')

    def test_context_get_current_template(self):
        # GIVEN
        expected_params = {'StackName': TEST_STACK_ID}
        expected_body = str({'AWSTemplateFormatVersion': '2010-09-09'})
        response = {
            "TemplateBody": expected_body
        }

        client = _get_cf_stubbed_client(
            method='get_template',
            response=response,
            expected_params=expected_params)

        context = _set_up_context(mock_client=client)
        stack_context = StackContext(context)

        # WHEN
        response = stack_context.get_current_template(stack_id=TEST_STACK_ID)

        # THEN
        self.assertEqual(response, expected_body)

    def test_context_get_current_template_with_client_error(self):
        # GIVEN}
        client = _get_cf_stubbed_client_with_error(
            method='get_template',
            service_error_code='ValidationError')

        context = _set_up_context(mock_client=client)
        stack_context = StackContext(context)

        # WHEN
        with self.assertRaises(HandledError):
            stack_context.get_current_template(stack_id=TEST_STACK_ID)

    @staticmethod
    def __build_simple_describe_stack_response(stack_status=None):
        expected_stack_status = "COMPLETED" if stack_status is None else stack_status
        expected_update_time = datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S')

        response = UnitTest_StackContext.__build_describes_stack_response(
            expected_status=expected_stack_status,
            expected_name=TEST_STACK_NAME,
            expected_stack_id=TEST_STACK_ID,
            expected_update_time=expected_update_time)
        return response

    @staticmethod
    def __build_describes_stack_response(expected_name, expected_stack_id, expected_status, expected_update_time):
        return {
            'Stacks': [
                {
                    'StackId': expected_stack_id,
                    'StackName': expected_name,
                    'CreationTime': expected_update_time,
                    'LastUpdatedTime': expected_update_time,
                    'StackStatus': expected_status,
                },
            ]
        }


class UnitTest_Monitor(unittest.TestCase):
    """Unit tests for the Monitor object"""

    def test_create(self):
        # GIVEN
        events_response = self._build_stack_events()
        resource_response = self._build_stack_resource_response()
        expected_params = {'StackName': TEST_STACK_ID}
        client = boto3.client('cloudformation', region_name=TEST_STUB_REGION)
        cf_stubber = Stubber(client)
        cf_stubber.add_response('describe_stack_events', events_response, expected_params)
        cf_stubber.add_response('describe_stack_resources', resource_response, expected_params)
        cf_stubber.activate()
        context = _set_up_context(mock_client=client)

        # WHEN
        mon = Monitor(context=context, stack_id=TEST_STACK_ID, operation='OPERATION')

        # THEN
        self.assertEqual(mon.stack_id, TEST_STACK_ID)
        self.assertEqual(mon.operation, 'OPERATION')
        self.assertEqual(mon.client, client)
        self.assertEqual(len(mon.events_seen), 1, "Expected to see 1 event")

    @staticmethod
    def _build_stack_resource_response():
        """ Sample describe_stack_resources response"""
        return {
            "StackResources": [
                {
                    "StackName": TEST_STACK_NAME,
                    "StackId": TEST_STACK_ID,
                    "LogicalResourceId": "bucket",
                    "PhysicalResourceId": "my-stack-bucket-1vc62xmplgguf",
                    "ResourceType": "AWS::S3::Bucket",
                    "Timestamp": "2019-10-02T04:34:11.345Z",
                    "ResourceStatus": "CREATE_COMPLETE",
                    "DriftInformation": {
                        "StackResourceDriftStatus": "IN_SYNC"
                    }
                }
            ]
        }

    @staticmethod
    def _build_stack_events():
        return {
            "StackEvents": [
                {
                    "StackId": TEST_STACK_ID,
                    "EventId": "4e1516d0-e4d6-xmpl-b94f-0a51958a168c",
                    "StackName": "my-stack",
                    "LogicalResourceId": "my-stack",
                    "PhysicalResourceId": "arn:aws:cloudformation:us-west-2:123456789012:stack/my-stack/d0a825a0-e4cd-xmpl-b9fb-061c69e99204",
                    "ResourceType": "AWS::CloudFormation::Stack",
                    "Timestamp": "2019-10-02T05:34:29.556Z",
                    "ResourceStatus": "UPDATE_COMPLETE"
                }
            ],
        }
