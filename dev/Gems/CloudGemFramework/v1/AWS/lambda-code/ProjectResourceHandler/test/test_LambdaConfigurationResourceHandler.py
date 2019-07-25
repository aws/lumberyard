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
# $Revision: #2 $

# Python
import json
import mock
import os
import StringIO
import types
import unittest
import zipfile
from time import time, sleep

# Boto3
import boto3
from botocore.exceptions import ClientError

# ProjectResourceHandler
from cgf_utils import properties
import LambdaConfigurationResourceHandler
from cgf_utils.test.mock_properties import PropertiesMatcher

# ServiceClient
import cgf_service_client.mock

# ResourceManagerCommon
from resource_manager_common import stack_info


class UnitTest_CloudGemFramework_ProjectResourceHandler_LambdaConfigurationResourceHandler(unittest.TestCase):

    event = {}

    context = {}

    def setUp(self):
        reload(LambdaConfigurationResourceHandler) # reset any accumulated state
        self.event = {
            'ResourceProperties': {
                'ConfigurationBucket': 'TestBucket',
                'ConfigurationKey': 'TestInputKey',
                'FunctionName': 'TestFunction',
                'Runtime': 'TestRuntime',
                'Settings': {
                    'TestSettingKey1': 'TestSettingValue1',
                    'TestSettingKey2': 'TestSettingValue2'
                },
                'Services': [
                    {
                        "InterfaceId": "Gem_TestInterface1_1_0_0",
                        "Optional": True
                    },
                    {
                        "InterfaceId": "Gem_TestInterface2_1_0_0",
                        "Optional": True
                    }
                ]
            },
            'StackId': 'arn:aws:cloudformation:TestRegion:TestAccount:stack/TestStack/TestUUID',
            'LogicalResourceId': 'TestLogicalResourceId'
        }


    @mock.patch('cgf_utils.custom_resource_response.succeed')
    @mock.patch('cgf_utils.role_utils.create_access_control_role')
    @mock.patch('cgf_utils.role_utils.get_access_control_role_name')
    @mock.patch('LambdaConfigurationResourceHandler._inject_settings')
    @mock.patch('LambdaConfigurationResourceHandler._add_built_in_settings')
    @mock.patch('LambdaConfigurationResourceHandler._get_project_service_lambda_arn')
    @mock.patch('LambdaConfigurationResourceHandler._get_input_key')
    @mock.patch('resource_manager_common.stack_info.StackInfoManager')
    def test_handler_create(
        self,
        mock_StackInfoManager,
        mock_get_input_key,
        mock_get_project_service_lambda_arn,
        mock_add_built_in_settings,
        mock_inject_settings,
        mock_get_access_control_role_name,
        mock_create_role,
        mock_custom_resource_handler_response_succeed
    ):

        mock_get_stack_info = mock_StackInfoManager.return_value.get_stack_info

        self.event['RequestType'] = 'Create'

        expected_data = {
          'CCSettings': {'TestSettingKey1': 'TestSettingValue1', 'TestSettingKey2': 'TestSettingValue2'},
          'ConfigurationBucket': 'TestBucket',
          'ConfigurationKey': 'TestOutputKey',
          'Runtime': 'TestRuntime',
          'Role': 'TestRole',
          'RoleName': 'TestRoleName',
          'ComposedLambdaConfiguration': {
              'Environment': {
                  'Variables': {'TestSettingKey1': 'TestSettingValue1', 'TestSettingKey2': 'TestSettingValue2'}
              },
              'Code': {
                  'S3Bucket': 'TestBucket',
                  'S3Key': 'TestOutputKey'
              },
              'Role': 'TestRole',
              'Runtime': 'TestRuntime'
          }
        }

        expected_physical_id = 'TestStack-TestLogicalResourceId::{}'

        mock_create_role.return_value = expected_data['Role']
        mock_get_access_control_role_name.return_value = expected_data['RoleName']
        mock_get_input_key.return_value = '{}/lambda-function-code.zip'.format(self.event['ResourceProperties']['ConfigurationKey'])
        mock_inject_settings.return_value = expected_data['ConfigurationKey']
        mock_get_project_service_lambda_arn.return_value = None

        LambdaConfigurationResourceHandler.handler(self.event, self.context)

        mock_custom_resource_handler_response_succeed.assert_called_once_with(
            self.event,
            self.context,
            expected_data,
            expected_physical_id)

        mock_create_role.assert_called_once_with(
            mock_StackInfoManager.return_value,
            {},
            self.event['StackId'],
            self.event['ResourceProperties']['FunctionName'],
            'lambda.amazonaws.com',
            default_policy = LambdaConfigurationResourceHandler.get_default_policy(None))

        mock_inject_settings.assert_called_once_with(
            self.event['ResourceProperties']['Settings'],
            self.event['ResourceProperties']['Runtime'],
            self.event['ResourceProperties']['ConfigurationBucket'],
            '{}/lambda-function-code.zip'.format(self.event['ResourceProperties']['ConfigurationKey']),
            'TestFunction')

        mock_add_built_in_settings.assert_called_once_with(
            self.event['ResourceProperties']['Settings'],
            mock_get_stack_info.return_value)

        mock_get_project_service_lambda_arn.assert_called_once_with(
            mock_get_stack_info.return_value)

        mock_get_stack_info.assert_called_once_with(
            self.event['StackId'])


    @mock.patch('cgf_utils.custom_resource_response.succeed')
    @mock.patch('cgf_utils.role_utils.get_access_control_role_arn')
    @mock.patch('cgf_utils.role_utils.get_access_control_role_name')
    @mock.patch('LambdaConfigurationResourceHandler._inject_settings')
    @mock.patch('LambdaConfigurationResourceHandler._add_built_in_settings')
    @mock.patch('LambdaConfigurationResourceHandler._get_input_key')
    @mock.patch('resource_manager_common.stack_info.StackInfoManager')
    def test_handler_update(
        self,
        mock_StackInfoManager,
        mock_get_input_key,
        mock_add_built_in_settings,
        mock_inject_settings,
        mock_get_access_control_role_name,
        mock_get_access_control_role_arn,
        mock_custom_resource_handler_response_succeed
    ):

        mock_get_stack_info = mock_StackInfoManager.return_value.get_stack_info

        self.event['RequestType'] = 'Update'
        self.event['PhysicalResourceId'] = 'TestStack-TestLogicalResourceId'

        expected_data = {
          'CCSettings': {'TestSettingKey1': 'TestSettingValue1', 'TestSettingKey2': 'TestSettingValue2'},
          'ConfigurationBucket': 'TestBucket',
          'ConfigurationKey': 'TestKey/lambda-function-code.zip',
          'Runtime': 'TestRuntime',
          'Role': 'TestRole',
          'RoleName': 'TestRoleName',
          'ComposedLambdaConfiguration': {
              'Environment': {
                  'Variables': {'TestSettingKey1': 'TestSettingValue1', 'TestSettingKey2': 'TestSettingValue2'}
              },
              'Code': {
                  'S3Bucket': 'TestBucket',
                  'S3Key': 'TestKey/lambda-function-code.zip'
              },
              'Role': 'TestRole',
              'Runtime': 'TestRuntime'
          }
        }

        expected_physical_id = self.event['PhysicalResourceId'] + "::{}"

        mock_get_access_control_role_arn.return_value = expected_data['Role']
        mock_get_access_control_role_name.return_value = expected_data['RoleName']
        mock_get_input_key.return_value = '{}/lambda-function-code.zip'.format(self.event['ResourceProperties']['ConfigurationKey'])
        mock_inject_settings.return_value = expected_data['ConfigurationKey']

        LambdaConfigurationResourceHandler.handler(self.event, self.context)

        mock_custom_resource_handler_response_succeed.assert_called_once_with(
            self.event,
            self.context,
            expected_data,
            expected_physical_id)

        mock_get_access_control_role_arn.assert_called_once_with(
            {},
            self.event['ResourceProperties']['FunctionName'])

        mock_inject_settings.assert_called_once_with(
            self.event['ResourceProperties']['Settings'],
            self.event['ResourceProperties']['Runtime'],
            self.event['ResourceProperties']['ConfigurationBucket'],
            '{}/lambda-function-code.zip'.format(self.event['ResourceProperties']['ConfigurationKey']),
            'TestFunction')

        mock_add_built_in_settings.assert_called_once_with(
            self.event['ResourceProperties']['Settings'],
            mock_get_stack_info.return_value)

        mock_get_stack_info.assert_called_once_with(
            self.event['StackId'])

    @mock.patch('cgf_utils.custom_resource_response.succeed')
    @mock.patch('cgf_utils.role_utils.delete_access_control_role')
    def test_handler_delete(
        self,
        mock_delete_role,
        mock_custom_resource_handler_response_succeed
    ):

        self.event['RequestType'] = 'Delete'
        self.event['PhysicalResourceId'] = 'TestStack-TestLogicalResourceId'

        expected_data = {}

        expected_physical_id = self.event['PhysicalResourceId'] + "::{}"

        LambdaConfigurationResourceHandler.handler(self.event, self.context)

        mock_custom_resource_handler_response_succeed.assert_called_once_with(self.event, self.context, expected_data, expected_physical_id)
        mock_delete_role.assert_called_once_with(
            {},
            self.event['ResourceProperties']['FunctionName'])

    def test_inject_settings_python(self):

        expected_settings = self.event['ResourceProperties']['Settings']
        expected_zip_name = 'cgf_lambda_settings/settings.json'

        zip_file = zipfile.ZipFile(StringIO.StringIO(), 'w')

        LambdaConfigurationResourceHandler._inject_settings_python(zip_file, expected_settings)

        with zip_file.open(expected_zip_name, 'r') as zip_content_file:
            actual_settings = json.load(zip_content_file)
            self.assertEquals(expected_settings, actual_settings)

    def test_get_settings_injector(self):

        python_injector = LambdaConfigurationResourceHandler._SETTINGS_INJECTORS.get('python')
        self.assertIsNotNone(python_injector)

        nodejs_injector = LambdaConfigurationResourceHandler._SETTINGS_INJECTORS.get('nodejs')
        self.assertIsNotNone(nodejs_injector)

        self.assertIs(python_injector, LambdaConfigurationResourceHandler._get_settings_injector('python2.7'))
        self.assertIs(nodejs_injector, LambdaConfigurationResourceHandler._get_settings_injector('nodejs4.3'))
        self.assertIs(nodejs_injector, LambdaConfigurationResourceHandler._get_settings_injector('nodejs')) # legacy support
        self.assertIs(python_injector, LambdaConfigurationResourceHandler._get_settings_injector('python999'))
        self.assertIs(nodejs_injector, LambdaConfigurationResourceHandler._get_settings_injector('nodejs999'))

        with self.assertRaises(RuntimeError):
            LambdaConfigurationResourceHandler._get_settings_injector('something')



class AnyZipFileObject(object):

    def __init__(self):
        pass

    def __eq__(self, other):
        return isinstance(other, zipfile.ZipFile)


class AnyValidZipFileContent(object):

    def __init__(self):
        pass

    def __eq__(self, other):
        zip_file = zipfile.ZipFile(StringIO.StringIO(other))
        return zip_file.testzip() is None


class AnyOrderListMatcher(object):

    def __init__(self, expected_value):
        self.__expected_value = expected_value

    def __eq__(self, other):
        return sorted(other) == sorted(self.__expected_value)

    def __repr__(self):
        return str(self.__expected_value) + ' (in any order)'
