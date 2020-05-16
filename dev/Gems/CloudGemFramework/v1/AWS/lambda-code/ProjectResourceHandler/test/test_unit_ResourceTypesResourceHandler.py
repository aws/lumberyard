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
import json
from unittest import mock
from unittest.mock import MagicMock

import boto3
from botocore.stub import Stubber, ANY
from botocore.exceptions import ClientError


from .test_case import ResourceHandlerTestCase
from cgf_utils import custom_resource_response
from resource_manager_common import stack_info
from resource_manager_common import constant

import ResourceTypesResourceHandler


# Specific unit tests to test setting up lambda config overrides on custom resource lambdas
class UnitTest_CloudGemFramework_ResourceTypeResourcesHandler_ResourceTypeLambdaConfig(ResourceHandlerTestCase):
    """
    Unit tests for the Lambda Config overrides that can be set on CoreResourceTypes

    Note: Test uses an AWS boto3 stubber to call lambda: https://botocore.amazonaws.com/v1/documentation/api/latest/reference/stubber.html
    - Has the advantage that it validates params and captures and tests the output
    - Has the disadvantage that it's much slower than straight mocking
    """
    PARENT_STACK = MagicMock()
    PARENT_STACK.stack_name = "Parent"

    MOCK_S3_CODE_BUCKET = 'arn:aws:s3:{}:{}:bucket/path'.format(ResourceHandlerTestCase.REGION, ResourceHandlerTestCase.ACCOUNT)

    RESOURCE_GROUP_INFO = MagicMock()
    RESOURCE_GROUP_INFO.is_project_stack = True
    RESOURCE_GROUP_INFO.project_stack = MagicMock()
    RESOURCE_GROUP_INFO.project_stack.configuration_bucket = MOCK_S3_CODE_BUCKET
    RESOURCE_GROUP_INFO.stack_name = ResourceHandlerTestCase.STACK_NAME

    RESOURCE_GROUP_INFO.ancestry = [PARENT_STACK]
    TEST_REGION = 'region'
    MOCK_ROLE_ARN = 'arn:aws:cloudformation:{}:{}:role'.format(ResourceHandlerTestCase.REGION, ResourceHandlerTestCase.ACCOUNT)

    DEFAULT_LAMBDA_TIMEOUT = 600
    DEFAULT_LAMBDA_MEMORY = 128

    event = {}
    lambda_client = None  # Need to make a new client each time

    mock_s3_client = MagicMock()
    mock_iam_client = MagicMock()

    def setUp(self):
        self.mock_s3_client.reset_mock()

    def _generate_new_lambda_client(self):
        self.lambda_client = boto3.client('lambda')
        return self.lambda_client

    def _get_lambda_client(self, stack_arn):
        return self.lambda_client

    def config_mock_clients(self, mock_s3_client, mock_iam_client):
        # Simulate no prior configuration to load from S3
        mock_s3_client.get_object.side_effect = ClientError({'Error': {'Code': 'NoSuchKey', 'Message': 'NotFound'}}, "get_object")
        # Ensure that create_role returns expected information
        mock_iam_client.create_role.return_value = {'Role': {'Arn': self.MOCK_ROLE_ARN}}

    def __assert_handler_succeeds(self, definitions, request_type, expected_output):
        """Helper class to ensure that calling the handler function behaves as expected"""
        event = {
            'RequestType': request_type,
            'ResourceProperties': {
                'Definitions': definitions,
                'LambdaConfiguration': {
                    'Code': {'S3Bucket': self.MOCK_S3_CODE_BUCKET},
                    'Runtime': 'python3.7'
                },
                'LambdaTimeout': self.DEFAULT_LAMBDA_TIMEOUT
            },
            'LogicalResourceId': self.LOGICAL_RESOURCE_ID,
            'StackId': self.STACK_ARN,
            'ResourceType': 'Custom::ResourceTypes',
            'RequestId': 'bf1143a7-484b-4d3c-8e23-4de2437c4301'
        }

        mock_response_succeed = MagicMock()
        with mock.patch.object(ResourceTypesResourceHandler, '_create_lambda_client', new=self._get_lambda_client):
            with mock.patch.object(custom_resource_response, 'succeed', new=mock_response_succeed):
                ResourceTypesResourceHandler.handler(event, self.CONTEXT)

        expected_physical_resource_id = 'Parent-{}.json'.format(self.LOGICAL_RESOURCE_ID)

        expected_data = {
            'ConfigBucket': self.MOCK_S3_CODE_BUCKET,
            'ConfigKey': 'resource-definitions/Parent/{}.json'.format(self.LOGICAL_RESOURCE_ID)
        }
        mock_response_succeed.assert_called_with(expected_output, self.CONTEXT, expected_data, expected_physical_resource_id)

    def __generate_create_function_lambda_expected_params(self, expected_handler):
        """Helper class to generate the expected calling params for a create_function call"""
        return {
            'FunctionName': ANY,
            'Description': ANY,
            'Runtime': ANY,
            'Role': ANY,
            'Publish': True,
            'Code': {'S3Bucket': self.MOCK_S3_CODE_BUCKET},
            'Handler': 'resource_types.{}'.format(expected_handler)
        }

    def __generate_update_function_expected_parameters(self, function_name):
        """Helper class to generate the expected calling params for an update_function call"""
        return {
            'FunctionName': function_name,
            'Publish': True,
            'S3Bucket': self.MOCK_S3_CODE_BUCKET
        }

    def __generate_lambda_stubber_for_create_function(self,
                                                      lambda_client_to_stub,
                                                      expected_handler='Custom_AccessControl.handler',
                                                      expected_memory_value=None,
                                                      expected_timeout_value=None,
                                                      expected_tags=None):
        """Rather than use a MagicMock for the client, use a boto3 stubber to wrap a real lambda client for the expected calls
        when we create a custom resource lambda.

        Note: Can't share stubber or client across test blocks as stubber modifies client. Boto3 stubber requires all
        add_response calls are added before calling activate.

        Stubber's main advantage is that it verifies params are valid for calls and provides mechanism to inspect results.
        Stubber's drawback is speed, as it's slow to use

        :param expected_handler: The expected name of the handler function to invoke
        :param expected_memory_value: The memory size expected for the lambda. May be None
        :param expected_timeout_value: The expected timeout value specified for the Lambda. May be None
        :param expected_tags: A dictionary of string, string pairs expected to be specified for the lambda. May be None
        """
        lambda_stubber = Stubber(lambda_client_to_stub)
        expected_params = self.__generate_create_function_lambda_expected_params(expected_handler=expected_handler)

        if expected_memory_value is not None:
            expected_params.update({'MemorySize': expected_memory_value})

        if expected_timeout_value is not None:
            expected_params.update({'Timeout': expected_timeout_value})

        if expected_tags is not None:
            expected_params.update({'Tags': expected_tags})

        create_function_response = {
            'FunctionArn': 'arn:aws:lambda:region:account:function:lambda',
            'Version': '1'
        }
        lambda_stubber.add_response('create_function', create_function_response, expected_params)
        return lambda_stubber

    def __generate_lambda_stubber_for_update_function(self,
                                                      lambda_client_to_stub,
                                                      expected_function_name,
                                                      expected_tags=None):
        """Rather than use a MagicMock for the client, use a boto3 stubber to wrap a real lambda client for the expected calls
        when we update a custom resource function.

        Note: Can't share stubber or client across test blocks as stubber modifies client. Stubber requires all
        add_response calls are added before calling activate.

        Stubber's main advantage is that it verifies params are valid for calls and provides mechanism to inspect results.
        Stubber's drawback is speed, as it's slow to use

        :param expected_function_name: The name of the lambda function expected to be referenced.
        :param expected_tags: The expected tags found on the lambda function
        """
        _mock_function_arn = 'arn:aws:lambda:region:account:function:lambda'

        lambda_stubber = Stubber(lambda_client_to_stub)
        expected_params = self.__generate_update_function_expected_parameters(function_name=expected_function_name)

        # 1. get function
        get_function_expected_params = {'FunctionName': expected_function_name}
        get_function_response = {
            'Configuration': {
                'FunctionArn': _mock_function_arn
            }
        }
        lambda_stubber.add_response('get_function', get_function_response, get_function_expected_params)

        # 2. tag function
        tag_resource_expected_params = {
            'Resource': 'arn:aws:lambda:region:account:function:lambda'
        }
        if expected_tags is not None:
            tag_resource_expected_params.update({'Tags': expected_tags})

        lambda_stubber.add_response('tag_resource', {}, tag_resource_expected_params)

        # 3. update function
        update_function_response = {
            'FunctionArn': 'arn:aws:lambda:region:account:function:lambda',
            'Version': '1'
        }
        lambda_stubber.add_response('update_function_code', update_function_response, expected_params)

        return lambda_stubber

    def __build_expected_output(self, _definitions, _request_type='Create'):
        """Format the expected output from the handler function"""
        return {
            'StackId': self.STACK_ARN,
            'RequestType': _request_type,
            'ResourceProperties': {
                'Definitions': _definitions,
                'LambdaConfiguration': {
                    'Code': {'S3Bucket': self.MOCK_S3_CODE_BUCKET},
                    'Runtime': 'python3.7'
                },
                'LambdaTimeout': self.DEFAULT_LAMBDA_TIMEOUT
            },
            'RequestId': 'bf1143a7-484b-4d3c-8e23-4de2437c4301',
            'ResourceType': 'Custom::ResourceTypes',
            'LogicalResourceId': self.LOGICAL_RESOURCE_ID
        }

    @mock.patch('ResourceTypesResourceHandler.s3_client', mock_s3_client)
    @mock.patch('ResourceTypesResourceHandler.iam_client', mock_iam_client)
    @mock.patch.object(stack_info.StackInfoManager, 'get_stack_info', return_value=RESOURCE_GROUP_INFO)
    def test_custom_resource_lambda_config_with_both_overrides(self, *args):
        # Given
        _timeout_value = 12
        _memory_value = 256
        _expected_tag_value = {
            constant.PROJECT_NAME_TAG: ResourceHandlerTestCase.STACK_NAME,
            constant.STACK_ID_TAG: ResourceHandlerTestCase.STACK_ARN
        }

        self.config_mock_clients(self.mock_s3_client, self.mock_iam_client)
        lambda_client = self._generate_new_lambda_client()
        lambda_stubber = self.__generate_lambda_stubber_for_create_function(lambda_client_to_stub=lambda_client,
                                                                            expected_memory_value=_memory_value,
                                                                            expected_timeout_value=_timeout_value,
                                                                            expected_tags=_expected_tag_value)
        lambda_stubber.activate()

        _definitions = {
            "Custom::AccessControl": {
                "ArnFormat": "*",
                "HandlerFunction": {
                    "Function": "Custom_AccessControl.handler",
                    "HandlerFunctionConfiguration": {
                        "MemorySize": _memory_value,
                        "Timeout": _timeout_value
                    },
                    "PolicyStatement": [
                        {
                            "Sid": "AllowAll",
                            "Effect": "Allow",
                            "Action": "*",
                            "Resource": "*"
                        }
                    ]
                }
            }
        }

        _expected_output = self.__build_expected_output(_definitions)
        self.__assert_handler_succeeds(_definitions, 'Create', _expected_output)

    @mock.patch('ResourceTypesResourceHandler.s3_client', mock_s3_client)
    @mock.patch('ResourceTypesResourceHandler.iam_client', mock_iam_client)
    @mock.patch.object(stack_info.StackInfoManager, 'get_stack_info', return_value=RESOURCE_GROUP_INFO)
    def test_custom_resource_lambda_config_with_memory_override(self, *args):
        # Given
        _memory_value = 256
        _expected_tag_value = {
            constant.PROJECT_NAME_TAG: ResourceHandlerTestCase.STACK_NAME,
            constant.STACK_ID_TAG: ResourceHandlerTestCase.STACK_ARN
        }

        self.config_mock_clients(self.mock_s3_client, self.mock_iam_client)
        lambda_client = self._generate_new_lambda_client()
        lambda_stubber = self.__generate_lambda_stubber_for_create_function(lambda_client_to_stub=lambda_client,
                                                                            expected_memory_value=_memory_value,
                                                                            expected_timeout_value=self.DEFAULT_LAMBDA_TIMEOUT,
                                                                            expected_tags=_expected_tag_value)
        lambda_stubber.activate()

        _definitions = {
            "Custom::AccessControl": {
                "ArnFormat": "*",
                "HandlerFunction": {
                    "Function": "Custom_AccessControl.handler",
                    "HandlerFunctionConfiguration": {
                        "MemorySize": _memory_value
                    }
                }
            }
        }

        _expected_output = self.__build_expected_output(_definitions)
        self.__assert_handler_succeeds(_definitions, 'Create', _expected_output)

    @mock.patch('ResourceTypesResourceHandler.s3_client', mock_s3_client)
    @mock.patch('ResourceTypesResourceHandler.iam_client', mock_iam_client)
    @mock.patch.object(stack_info.StackInfoManager, 'get_stack_info', return_value=RESOURCE_GROUP_INFO)
    def test_custom_resource_lambda_config_with_timeout_override(self, *args):
        # Given
        _timeout_value = 20
        _expected_tag_value = {
            constant.PROJECT_NAME_TAG: ResourceHandlerTestCase.STACK_NAME,
            constant.STACK_ID_TAG: ResourceHandlerTestCase.STACK_ARN
        }

        self.config_mock_clients(self.mock_s3_client, self.mock_iam_client)
        lambda_client = self._generate_new_lambda_client()
        lambda_stubber = self.__generate_lambda_stubber_for_create_function(lambda_client_to_stub=lambda_client,
                                                                            expected_timeout_value=_timeout_value,
                                                                            expected_tags=_expected_tag_value)
        lambda_stubber.activate()

        _definitions = {
            "Custom::AccessControl": {
                "ArnFormat": "*",
                "HandlerFunction": {
                    "Function": "Custom_AccessControl.handler",
                    "HandlerFunctionConfiguration": {
                        "Timeout": _timeout_value
                    }
                }
            }
        }

        _expected_output = self.__build_expected_output(_definitions)
        self.__assert_handler_succeeds(_definitions, 'Create', _expected_output)

    @mock.patch('ResourceTypesResourceHandler.s3_client', mock_s3_client)
    @mock.patch('ResourceTypesResourceHandler.iam_client', mock_iam_client)
    @mock.patch.object(stack_info.StackInfoManager, 'get_stack_info', return_value=RESOURCE_GROUP_INFO)
    def test_new_custom_resources_are_tagged_as_expected(self, *args):
        # Given
        _expected_lambda_tags = {constant.PROJECT_NAME_TAG: self.STACK_NAME, constant.STACK_ID_TAG: self.STACK_ARN}
        _expected_iam_tags = [
            {'Key': constant.PROJECT_NAME_TAG, 'Value': self.STACK_NAME},
            {'Key': constant.STACK_ID_TAG, 'Value': self.STACK_ARN},
        ]

        self.config_mock_clients(self.mock_s3_client, self.mock_iam_client)
        lambda_client = self._generate_new_lambda_client()
        lambda_stubber = self.__generate_lambda_stubber_for_create_function(lambda_client_to_stub=lambda_client,
                                                                            expected_handler='Custom_UserPool.handler',
                                                                            expected_tags=_expected_lambda_tags,
                                                                            expected_timeout_value=self.DEFAULT_LAMBDA_TIMEOUT)
        lambda_stubber.activate()

        _definitions = {
            "Custom::UserPool": {
                "ArnFormat": "*",
                "HandlerFunction": {
                    "Function": "Custom_UserPool.handler"
                }
            }
        }

        _expected_output = self.__build_expected_output(_definitions)
        self.__assert_handler_succeeds(_definitions, 'Create', _expected_output)

        # test IAM roles are tagged as expected
        self.mock_iam_client.create_role.called_with(ANY, ANY, ANY, Tags=_expected_iam_tags)

    @mock.patch('ResourceTypesResourceHandler.s3_client', mock_s3_client)
    @mock.patch('ResourceTypesResourceHandler.iam_client', mock_iam_client)
    @mock.patch.object(stack_info.StackInfoManager, 'get_stack_info', return_value=RESOURCE_GROUP_INFO)
    def test_update_custom_resources_are_tagged_as_expected(self, *args):
        _test_lambda_name = 'Custom_UserPool'
        _expected_lambda_tags = {"stack-name": self.STACK_NAME}
        _expected_iam_tags = [{'Key': 'stack-name', 'Value': self.STACK_NAME}]
        _function_name = '{}-CRH-TestLogicalId-{}'.format(self.STACK_NAME, _test_lambda_name)

        _expected_lambda_tags = {constant.PROJECT_NAME_TAG: self.STACK_NAME, constant.STACK_ID_TAG: self.STACK_ARN}

        self.mock_iam_client.create_role.return_value = {'Role': {'Arn': self.MOCK_ROLE_ARN}}

        # Set up a mock definition so code sees lambda as an existing lambda
        mock_s3_definition = json.dumps({
            'Definitions': {},
            'Lambdas': {_function_name: None}
        })
        mock_s3_response = MagicMock()
        mock_s3_response.read.return_value = mock_s3_definition
        self.mock_s3_client.get_object.side_effect = [{'Body': mock_s3_response}]

        lambda_client = self._generate_new_lambda_client()
        lambda_stubber = self.__generate_lambda_stubber_for_update_function(lambda_client_to_stub=lambda_client,
                                                                            expected_function_name=_function_name,
                                                                            expected_tags=_expected_lambda_tags)
        lambda_stubber.activate()

        _definitions = {
            "Custom::UserPool": {
                "ArnFormat": "*",
                "HandlerFunction": {
                    "Function": "{}.handler".format(_test_lambda_name)
                }
            }
        }

        _expected_output = self.__build_expected_output(_definitions, _request_type='Update')

        self.__assert_handler_succeeds(_definitions, 'Update', _expected_output)

        # test IAM roles are tagged as expected
        self.mock_iam_client.create_role.called_with(ANY, ANY, ANY, Tags=_expected_iam_tags)

    @mock.patch('ResourceTypesResourceHandler.s3_client', mock_s3_client)
    @mock.patch('ResourceTypesResourceHandler.iam_client', mock_iam_client)
    @mock.patch.object(stack_info.StackInfoManager, 'get_stack_info', return_value=RESOURCE_GROUP_INFO)
    def test_custom_resource_lambda_config_with_condition(self, *args):
        # Given
        _expected_tag_value = {
            constant.PROJECT_NAME_TAG: ResourceHandlerTestCase.STACK_NAME,
            constant.STACK_ID_TAG: ResourceHandlerTestCase.STACK_ARN
        }

        self.config_mock_clients(self.mock_s3_client, self.mock_iam_client)
        lambda_client = self._generate_new_lambda_client()
        lambda_stubber = self.__generate_lambda_stubber_for_create_function(lambda_client_to_stub=lambda_client,
                                                                            expected_timeout_value=self.DEFAULT_LAMBDA_TIMEOUT,
                                                                            expected_tags=_expected_tag_value)
        lambda_stubber.activate()

        _definitions = {
            "Custom::AccessControl": {
                "ArnFormat": "*",
                "HandlerFunction": {
                    "Function": "Custom_AccessControl.handler",
                    "PolicyStatement": [
                        {
                            "Sid": "AllowAll",
                            "Effect": "Allow",
                            "Action": "*",
                            "Resource": "*",
                            "Condition": {"Bool": {"aws:SecureTransport": "true"}}
                        }
                    ]
                }
            }
        }

        _expected_output = self.__build_expected_output(_definitions)
        self.__assert_handler_succeeds(_definitions, 'Create', _expected_output)
