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
import boto3
import StringIO
import zipfile
import json
import types
import os
import role_utils

from time import time, sleep
from botocore.exceptions import ClientError

from resource_manager_common import stack_info
import custom_resource_response

import LambdaConfigurationResourceHandler

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
                }
            },
            'StackId': 'arn:aws:cloudformation:TestRegion:TestAccount:stack/TestStack/TestUUID',
            'LogicalResourceId': 'TestLogicalResourceId'
        }


    def test_handler_create(self):

        self.event['RequestType'] = 'Create'

        expected_data = {
          'ConfigurationBucket': 'TestBucket',
          'ConfigurationKey': 'TestOutputKey',
          'Runtime': 'TestRuntime',
          'Role': 'TestRole',
          'RoleName': 'TestRoleName'
        }

        expected_physical_id = 'TestStack-TestLogicalResourceId::{}'

        with mock.patch.object(custom_resource_response, 'succeed') as mock_custom_resource_handler_response_succeed:
            with mock.patch.object(role_utils, 'create_access_control_role') as mock_create_role:
                mock_create_role.return_value = expected_data['Role']
                with mock.patch.object(role_utils, 'get_access_control_role_name') as mock_get_access_control_role_name:
                    mock_get_access_control_role_name.return_value = expected_data['RoleName']
                    with mock.patch.object(LambdaConfigurationResourceHandler, '_inject_settings') as mock_inject_settings:
                        with mock.patch.object(LambdaConfigurationResourceHandler, '_add_built_in_settings') as mock_add_built_in_settings:
                            with mock.patch.object(LambdaConfigurationResourceHandler, '_get_project_service_lambda_arn') as mock_get_project_service_lambda_arn:
                                with mock.patch.object(LambdaConfigurationResourceHandler, '_get_input_key') as mock_get_input_key:
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
                                        self.event['StackId'])
                                    mock_get_project_service_lambda_arn.assert_called_once_with(
                                        self.event['StackId'])


    def test_handler_update(self):

        self.event['RequestType'] = 'Update'
        self.event['PhysicalResourceId'] = 'TestStack-TestLogicalResourceId'

        expected_data = {
          'ConfigurationBucket': 'TestBucket',
          'ConfigurationKey': 'TestKey/lambda-function-code.zip',
          'Runtime': 'TestRuntime',
          'Role': 'TestRole',
          'RoleName': 'TestRoleName'
        }

        expected_physical_id = self.event['PhysicalResourceId'] + "::{}"
                
        with mock.patch.object(custom_resource_response, 'succeed') as mock_custom_resource_handler_response_succeed:
            with mock.patch.object(role_utils, 'get_access_control_role_arn') as mock_get_access_control_role_arn:
                mock_get_access_control_role_arn.return_value = expected_data['Role']
                with mock.patch.object(role_utils, 'get_access_control_role_name') as mock_get_access_control_role_name:
                    mock_get_access_control_role_name.return_value = expected_data['RoleName']
                    with mock.patch.object(LambdaConfigurationResourceHandler, '_inject_settings') as mock_inject_settings:
                        with mock.patch.object(LambdaConfigurationResourceHandler, '_add_built_in_settings') as mock_add_built_in_settings:
                            with mock.patch.object(LambdaConfigurationResourceHandler, '_get_input_key') as mock_get_input_key:
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
                                    self.event['StackId'])


    def test_handler_delete(self):

        self.event['RequestType'] = 'Delete'
        self.event['PhysicalResourceId'] = 'TestStack-TestLogicalResourceId'

        expected_data = {}

        expected_physical_id = self.event['PhysicalResourceId'] + "::{}"
                
        with mock.patch.object(custom_resource_response, 'succeed') as mock_custom_resource_handler_response_succeed:
            with mock.patch.object(role_utils, 'delete_access_control_role') as mock_delete_role:
                LambdaConfigurationResourceHandler.handler(self.event, self.context)
                mock_custom_resource_handler_response_succeed.assert_called_once_with(self.event, self.context, expected_data, expected_physical_id)
                mock_delete_role.assert_called_once_with(
                    {}, 
                    self.event['ResourceProperties']['FunctionName'])


    def test_inject_settings(self):

        with mock.patch.object(boto3, 'client') as mock_boto3_client:

            zip_content = StringIO.StringIO()
            zip_file = zipfile.ZipFile(zip_content, 'w')
            zip_file.close()

            mock_body = mock.MagicMock()
            mock_body.read = mock.MagicMock(return_value=zip_content.getvalue())

            mock_s3_client = mock_boto3_client.return_value
            mock_s3_client.get_object = mock.MagicMock(return_value={'Body': mock_body})
            mock_s3_client.put_object = mock.MagicMock()

            reload(LambdaConfigurationResourceHandler) # so it uses mocked methods

            settings = self.event['ResourceProperties']['Settings']
            runtime = self.event['ResourceProperties']['Runtime']
            bucket = self.event['ResourceProperties']['ConfigurationBucket']
            input_key = self.event['ResourceProperties']['ConfigurationKey']
            function_name = self.event['ResourceProperties']['FunctionName']

            mock_injector = mock.MagicMock()
            LambdaConfigurationResourceHandler._SETTINGS_INJECTORS[runtime] = mock_injector

            output_key = LambdaConfigurationResourceHandler._inject_settings(settings, runtime, bucket, input_key, function_name)

            mock_boto3_client.assert_called_with('s3')
            mock_s3_client.get_object.assert_called_once_with(Bucket=bucket, Key=input_key)
            mock_injector.assert_called_once_with(AnyZipFileObject(), settings)
            mock_s3_client.put_object.assert_called_once_with(Bucket='TestBucket', Key=output_key, Body=AnyValidZipFileContent())


    def test_inject_settings_python(self):
        
        expected_settings = self.event['ResourceProperties']['Settings']
        expected_zip_name = 'CloudCanvas/settings.py'

        zip_file = zipfile.ZipFile(StringIO.StringIO(), 'w')

        LambdaConfigurationResourceHandler._inject_settings_python(zip_file, expected_settings)                        

        with zip_file.open(expected_zip_name, 'r') as zip_content_file:
            globals = {}
            exec(zip_content_file.read(), globals)
            actual_settings = globals['settings']
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



