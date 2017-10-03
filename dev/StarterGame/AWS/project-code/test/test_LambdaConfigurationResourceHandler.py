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

from time import time, sleep
from botocore.exceptions import ClientError

import discovery_utils
import custom_resource_response

TEST_REGION = 'us-east-1'
TEST_PROFILE = 'default'

os.environ['AWS_DEFAULT_REGION'] = TEST_REGION
os.environ['AWS_PROFILE'] = TEST_PROFILE

# import after setting AWS configuration in envionment 
import LambdaConfigurationResourceHandler
import role_utils

class TestLambdaConfigurationResourceHandler(unittest.TestCase):

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
          'Role': 'TestRole'
        }

        expected_physical_id = 'TestStack-TestLogicalResourceId'

        with mock.patch.object(custom_resource_response, 'succeed') as mock_custom_resource_response_succeed:
            with mock.patch.object(role_utils, 'create_role') as mock_create_role:
                mock_create_role.return_value = expected_data['Role']
                with mock.patch.object(LambdaConfigurationResourceHandler, '_inject_settings') as mock_inject_settings:
                    mock_inject_settings.return_value = expected_data['ConfigurationKey']
                    LambdaConfigurationResourceHandler.handler(self.event, self.context)
                    mock_custom_resource_response_succeed.assert_called_once_with(
                        self.event, 
                        self.context, 
                        expected_data, 
                        expected_physical_id)
                    mock_create_role.assert_called_once_with(
                        self.event['StackId'], 
                        self.event['LogicalResourceId'],
                        LambdaConfigurationResourceHandler.POLICY_NAME, 
                        'lambda.amazonaws.com',
                        LambdaConfigurationResourceHandler.DEFAULT_POLICY_STATEMENTS,
                        AnyFunction())
                    mock_inject_settings.assert_called_once_with(
                        self.event['ResourceProperties']['Settings'], 
                        self.event['ResourceProperties']['Runtime'], 
                        self.event['ResourceProperties']['ConfigurationBucket'],
                        '{}/lambda-function-code.zip'.format(self.event['ResourceProperties']['ConfigurationKey']),
                        'TestFunction')


    def test_handler_update(self):

        self.event['RequestType'] = 'Update'
        self.event['PhysicalResourceId'] = 'TestStack-TestLogicalResourceId'

        expected_data = {
          'ConfigurationBucket': 'TestBucket',
          'ConfigurationKey': 'TestKey/lambda-function-code.zip',
          'Runtime': 'TestRuntime',
          'Role': 'TestRole'
        }

        expected_physical_id = self.event['PhysicalResourceId']
                
        with mock.patch.object(custom_resource_response, 'succeed') as mock_custom_resource_response_succeed:
            with mock.patch.object(role_utils, 'update_role') as mock_update_role:
                mock_update_role.return_value = expected_data['Role']
                with mock.patch.object(LambdaConfigurationResourceHandler, '_inject_settings') as mock_inject_settings:
                    mock_inject_settings.return_value = expected_data['ConfigurationKey']
                    LambdaConfigurationResourceHandler.handler(self.event, self.context)
                    mock_custom_resource_response_succeed.assert_called_once_with(
                        self.event, 
                        self.context, 
                        expected_data, 
                        expected_physical_id)
                    mock_update_role.assert_called_once_with(
                        self.event['StackId'], 
                        self.event['LogicalResourceId'], 
                        LambdaConfigurationResourceHandler.POLICY_NAME,
                        LambdaConfigurationResourceHandler.DEFAULT_POLICY_STATEMENTS,
                        AnyFunction())
                    mock_inject_settings.assert_called_once_with(
                        self.event['ResourceProperties']['Settings'], 
                        self.event['ResourceProperties']['Runtime'], 
                        self.event['ResourceProperties']['ConfigurationBucket'], 
                        '{}/lambda-function-code.zip'.format(self.event['ResourceProperties']['ConfigurationKey']),
                        'TestFunction')


    def test_handler_delete(self):

        self.event['RequestType'] = 'Delete'
        self.event['PhysicalResourceId'] = 'TestStack-TestLogicalResourceId'

        expected_data = {}

        expected_physical_id = self.event['PhysicalResourceId']
                
        with mock.patch.object(custom_resource_response, 'succeed') as mock_custom_resource_response_succeed:
            with mock.patch.object(role_utils, 'delete_role') as mock_delete_role:
                LambdaConfigurationResourceHandler.handler(self.event, self.context)
                mock_custom_resource_response_succeed.assert_called_once_with(self.event, self.context, expected_data, expected_physical_id)
                mock_delete_role.assert_called_once_with(
                    self.event['StackId'], 
                    self.event['LogicalResourceId'], 
                    LambdaConfigurationResourceHandler.POLICY_NAME)


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


    def test_inject_settings_nodejs(self):
        
        expected_settings = self.event['ResourceProperties']['Settings']
        expected_zip_name = 'CloudCanvas/settings.js'

        zip_file = zipfile.ZipFile(StringIO.StringIO(), 'w')

        LambdaConfigurationResourceHandler._inject_settings_nodejs(zip_file, expected_settings)                        

        with zip_file.open(expected_zip_name, 'r') as zip_content_file:
            content = zip_content_file.read()
            print content
            self.assertTrue('TestSettingKey1' in content)
            self.assertTrue('TestSettingValue1' in content)
            self.assertTrue('TestSettingKey2' in content)
            self.assertTrue('TestSettingValue2' in content)


    # @unittest.skip("integration test disabled")
    def test_integration_inject_settings_python(self):
        
        # we need both the s3 client below and the one created by the 
        # custom resource handler to use this region
        boto3.setup_default_session(region_name=TEST_REGION)
        reload(LambdaConfigurationResourceHandler) # reset global s3 client object

        s3 = boto3.client('s3')

        bucket = 'lmbr_aws_settings_test_' + str(int(time() * 1000))
        input_key = 'TestKey/lambda-function-code.zip'
        output_key = None

        s3.create_bucket(Bucket=bucket)

        try:

            zip_content = StringIO.StringIO()

            with zipfile.ZipFile(zip_content, 'w') as zip_file:
                zip_file.writestr('InitialName', 'InitialContent')

            body = zip_content.getvalue()
            s3.put_object(Bucket=bucket, Key=input_key, Body=body)

            zip_content.close()

            sleep(10) # seconds

            expected_settings = self.event['ResourceProperties']['Settings']
            runtime = 'python2.7'
            function_name = 'TestFunction'
            output_key = LambdaConfigurationResourceHandler._inject_settings(expected_settings, runtime, bucket, input_key, function_name)

            expected_zip_name = 'CloudCanvas/settings.py'

            sleep(10) # seconds

            print 'output_key', output_key
            print 'bucket', bucket
            res = s3.get_object(Bucket=bucket, Key=output_key)
            body = res['Body'].read()

            zip_content = StringIO.StringIO(body)
            with zipfile.ZipFile(zip_content, 'r') as zip_file:
                with zip_file.open('InitialName', 'r') as zip_content_file:
                    actual_zip_content = zip_content_file.read()
                    self.assertEquals('InitialContent', actual_zip_content)
                with zip_file.open(expected_zip_name, 'r') as zip_content_file:
                    globals = {}
                    exec(zip_content_file.read(), globals)
                    actual_settings = globals['settings']
                    self.assertEquals(expected_settings, actual_settings)
                    zip_content.close()

        finally:

            try:
                s3.delete_object(Bucket=bucket, Key=input_key)
            except Exception as e:
                print 'Error when deleting object {} from bucket {}: {}'.format(input_key, bucket, e)

            if output_key is not None:
                try:
                    s3.delete_object(Bucket=bucket, Key=output_key)
                except Exception as e:
                    print 'Error when deleting object {} from bucket {}: {}'.format(output_key, bucket, e)

            try:
                s3.delete_bucket(Bucket=bucket)
            except Exception as e:
                print 'Error when deleting bucket {}: {}'.format(bucket, e)

    # @unittest.skip("integration test disabled")
    def test_integration_create_update_delete_role(self):
        
        with mock.patch.object(discovery_utils,'ResourceGroupInfo') as mock_ResourceGroupInfo:

            mock_ResourceGroupInfo.return_value.resource_group_name = 'TestGroup'
            mock_ResourceGroupInfo.return_value.deployment = mock.MagicMock()
            mock_ResourceGroupInfo.return_value.deployment.deployment_name = 'TestDeployment'
            mock_ResourceGroupInfo.return_value.deployment.project = mock.MagicMock()
            mock_ResourceGroupInfo.return_value.deployment.project.project_name = 'TestProject'

            with mock.patch.object(custom_resource_response, 'succeed') as mock_custom_resource_response_succeed:

                with mock.patch.object(LambdaConfigurationResourceHandler, '_inject_settings') as mock_inject_settings:

                    mock_inject_settings.return_value = 'TestOutputConfigurationKey'

                    stack_arn = self._create_role_test_stack()

                    self.event['StackId'] = stack_arn

                    try:

                        capture_data = CaptureValue()
                        capture_physical_resource_id = CaptureValue()

                        # test create

                        self.event['RequestType'] = 'Create'
                        LambdaConfigurationResourceHandler.handler(self.event, self.context)

                        mock_custom_resource_response_succeed.assert_called_once_with(
                            self.event, 
                            self.context, 
                            capture_data, 
                            capture_physical_resource_id)

                        created_role_arn = capture_data.value['Role']

                        self._validate_role(created_role_arn, stack_arn)

                        # test update

                        mock_custom_resource_response_succeed.reset_mock()

                        self.event['RequestType'] = 'Update'
                        self.event['PhysicalResourceId'] = capture_physical_resource_id.value
                        LambdaConfigurationResourceHandler.handler(self.event, self.context)

                        mock_custom_resource_response_succeed.assert_called_once_with(
                            self.event, 
                            self.context, 
                            capture_data, 
                            capture_physical_resource_id)

                        updated_role_arn = capture_data.value['Role']

                        self.assertEquals(created_role_arn, updated_role_arn)
                        self._validate_role(updated_role_arn, stack_arn)

                        # rest delete

                        mock_custom_resource_response_succeed.reset_mock()

                        self.event['RequestType'] = 'Delete'
                        self.event['PhysicalResourceId'] = capture_physical_resource_id.value
                        LambdaConfigurationResourceHandler.handler(self.event, self.context)

                        mock_custom_resource_response_succeed.assert_called_once_with(
                            self.event, 
                            self.context, 
                            capture_data, 
                            capture_physical_resource_id)

                        self._validate_role_deleted(created_role_arn)

                    finally:

                        # self._delete_role_test_stack(stack_arn)
                        pass


    def _create_role_test_stack(self):

        cf = boto3.client('cloudformation', region_name=TEST_REGION)

        stack_name = 'lmbr-aws-update-role-test-' + str(int(time() * 1000))

        print 'creating stack', stack_name

        res = cf.create_stack(
            StackName = stack_name,
            TemplateBody = self.ROLE_TEST_STACK_TEMPLATE,
            Capabilities = [ 'CAPABILITY_IAM' ])

        stack_arn = res['StackId']

        print 'CreateStack', res

        while True:
            sleep(5)
            res = cf.describe_stacks(StackName=stack_arn)
            print 'Checking', res
            if res['Stacks'][0]['StackStatus'] != 'CREATE_IN_PROGRESS':
                break

        self.assertEquals(res['Stacks'][0]['StackStatus'], 'CREATE_COMPLETE')

        return stack_arn


    def _delete_role_test_stack(self, stack_arn):
        print 'deleting stack', stack_arn
        cf = boto3.client('cloudformation', region_name=TEST_REGION)
        cf.delete_stack(StackName=stack_arn)


    def _validate_role(self, role_arn, stack_arn):

        iam = boto3.client('iam')
        print 'role_arn', role_arn
        res = iam.get_role(RoleName=self._get_role_name_from_role_arn(role_arn))
        print 'res', res
        role = res['Role']
        self.assertEquals(role['Path'], '/TestProject/TestDeployment/TestGroup/TestLogicalResourceId/')

        cf = boto3.client('cloudformation', region_name=TEST_REGION)
        res = cf.describe_stack_resources(StackName=stack_arn)
        print res
        resources = res['StackResources']

        expected_statement = {
            'TestTableAccess': {
                'Sid': 'TestTableAccess',
                'Effect': 'Allow',
                'Action': [ 'dynamodb:PutItem' ],
                'Resource': self._get_resource_arn(stack_arn, resources, 'TestTable')
            },
            'TestFunctionAccess': {
                'Sid': 'TestFunctionAccess',
                'Effect': 'Allow',
                'Action': [ 'lambda:InvokeFunction' ],
                'Resource': self._get_resource_arn(stack_arn, resources, 'TestFunction')
            },
            'TestQueueAccess': {
                'Sid': 'TestQueueAccess',
                'Effect': 'Allow',
                'Action':  [ 'sqs:SendMessage' ],
                'Resource': self._get_resource_arn(stack_arn, resources, 'TestQueue')
            },
            'TestTopicAccess': {
                'Sid': 'TestTopicAccess',
                'Effect': 'Allow',
                'Action': [ 'sns:Subscribe' ],
                'Resource': self._get_resource_arn(stack_arn, resources, 'TestTopic')
            },
            'TestBucketAccess': {
                'Sid': 'TestBucketAccess',
                'Effect': 'Allow',
                'Action': [ 's3:GetObject', 's3:PutObject' ],
                'Resource': self._get_resource_arn(stack_arn, resources, 'TestBucket') + "TestSuffix"
            },
            'WriteLogs': {
                'Sid': 'WriteLogs',
                'Action': ['logs:CreateLogGroup', 'logs:CreateLogStream', 'logs:PutLogEvents'], 
                'Resource': 'arn:aws:logs:*:*:*', 
                'Effect': 'Allow'
            }
        }

        res = iam.get_role_policy(RoleName=self._get_role_name_from_role_arn(role_arn), PolicyName='FunctionAccess')
        print res
        actual_policy = res['PolicyDocument']

        count = 0
        for actual_statement in actual_policy['Statement']:
            self.assertEquals(actual_statement, expected_statement.get(actual_statement['Sid'], None))
            count += 1

        self.assertEquals(count, 6)

    
    def _get_resource_arn(self, stack_arn, resources, name):

        arn = None

        for resource in resources:
            if resource['LogicalResourceId'] == name:
                arn = self._make_resource_arn(stack_arn, resource['ResourceType'], resource['PhysicalResourceId'])

        self.assertIsNotNone(arn)

        return arn


    RESOURCE_ARN_PATTERNS = {
        'AWS::DynamoDB::Table': 'arn:aws:dynamodb:{region}:{account_id}:table/{resource_name}',
        'AWS::Lambda::Function': 'arn:aws:lambda:{region}:{account_id}:function:{resource_name}',
        'AWS::SQS::Queue': 'arn:aws:sqs:{region}:{account_id}:{resource_name}',
        'AWS::SNS::Topic': 'arn:aws:sns:{region}:{account_id}:{resource_name}',
        'AWS::S3::Bucket': 'arn:aws:s3:::{resource_name}'
    }


    def _make_resource_arn(self, stack_arn, resource_type, resource_name):
    
        pattern = self.RESOURCE_ARN_PATTERNS.get(resource_type, None)
        self.assertIsNotNone(pattern)

        return pattern.format(
            region=TEST_REGION,
            account_id=self._get_account_id_from_stack_arn(stack_arn),
            resource_name=resource_name)


    def _get_account_id_from_stack_arn(self, stack_arn):
        # arn:aws:cloudformation:REGION:ACCOUNT:stack/STACK/UUID
        return stack_arn.split(':')[4]


    def _get_role_name_from_role_arn(self, role_arn):
        # arn:aws:cloudformation:REGION:ACCOUNT:stack/STACK/UUID
        return role_arn.split('/')[-1]


    def _validate_role_deleted(self, role_arn):
        iam = boto3.client('iam')
        try:
            iam.get_role(RoleName=self._get_role_name_from_role_arn(role_arn))
            self.assertTrue(False)
        except ClientError as e:
            self.assertEquals(e.response["Error"]["Code"], "NoSuchEntity")


    ROLE_TEST_STACK_TEMPLATE = '''{
            "AWSTemplateFormatVersion": "2010-09-09",

            "Resources": {

                "TestTable": {
                    "Type": "AWS::DynamoDB::Table",
                    "Properties": {
                        "AttributeDefinitions": [
                            {
                                "AttributeName": "PlayerId",
                                "AttributeType": "S"
                            }
                        ],
                        "KeySchema": [
                            {
                                "AttributeName": "PlayerId",
                                "KeyType": "HASH"
                            }
                        ],
                        "ProvisionedThroughput": {
                            "ReadCapacityUnits": "1",
                            "WriteCapacityUnits": "1"
                        }
                    },
                    "Metadata": {
                        "CloudCanvas": {
                            "FunctionAccess": [
                                {
                                    "FunctionName": "TestFunction",
                                    "Action": "dynamodb:PutItem"
                                }
                            ]
                        }
                    }
                },

                "TestFunctionRole": {
                    "Type": "AWS::IAM::Role",
                    "Properties": {
                        "AssumeRolePolicyDocument": {
                            "Version": "2012-10-17",
                            "Statement": [
                                {
                                    "Effect": "Allow",
                                    "Action": "sts:AssumeRole",
                                    "Principal": {
                                        "Service": "lambda.amazonaws.com"
                                    }
                                }
                            ]
                        },
                        "Policies": [
                            {
                                "PolicyName": "Execution",
                                "PolicyDocument": {
                                    "Version": "2012-10-17",
                                    "Statement": [
                                        {
                                            "Action": [
                                                "logs:CreateLogGroup",
                                                "logs:CreateLogStream",
                                                "logs:PutLogEvents"
                                            ],
                                            "Effect": "Allow",
                                            "Resource": "arn:aws:logs:*:*:*"
                                        }
                                    ]
                                }
                            }
                        ]
                    }
                },

                "TestFunction": {
                    "Type": "AWS::Lambda::Function",
                    "Properties": {
                        "Description": "Implements the custom resources used in this project's templates.",
                        "Handler": "index.handler",
                        "Role": { "Fn::GetAtt": [ "TestFunctionRole", "Arn" ] },
                        "Runtime": "nodejs",
                        "Code": {
                            "ZipFile": "exports.handler = function(event, context) { return 'Test'; }"
                        }
                    },
                    "Metadata": {
                        "CloudCanvas": {
                            "FunctionAccess": {
                                "FunctionName": "TestFunction",
                                "Action": "lambda:InvokeFunction"
                            }
                        }
                    }
                },

                "TestQueue": {
                    "Type": "AWS::SQS::Queue",
                    "Properties": {
                    },
                    "Metadata": {
                        "CloudCanvas": {
                            "FunctionAccess": [
                                {
                                    "FunctionName": "TestFunction",
                                    "Action": [ "sqs:SendMessage" ]
                                }
                            ]
                        }
                    }
                },
            
                "TestTopic": {
                    "Type": "AWS::SNS::Topic",
                    "Properties": {
                    },
                    "Metadata": {
                        "CloudCanvas": {
                            "FunctionAccess": [
                                {
                                    "FunctionName": "TestFunction",
                                    "Action": "sns:Subscribe"
                                }
                            ]
                        }
                    }
                },

                "TestBucket": {
                    "Type": "AWS::S3::Bucket",
                    "Properties": {
                    },
                    "Metadata": {
                        "CloudCanvas": {
                            "FunctionAccess": [
                                {
                                    "FunctionName": "TestFunction",
                                    "Action": [ "s3:GetObject", "s3:PutObject" ],
                                    "ResourceSuffix": "TestSuffix"
                                }
                            ]
                        }
                    }
                }

            }

        }'''

 
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


class AnyFunction(object):
     
    def __init__(self):
        pass

    def __eq__(self, other):
        return isinstance(other, types.FunctionType)

class CaptureValue(object):

    def __init__(self):
        self.value = None

    def __eq__(self, other):
        self.value = other
        return True



