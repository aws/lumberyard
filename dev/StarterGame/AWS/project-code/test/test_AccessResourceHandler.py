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
import json
import boto3

from botocore.exceptions import ClientError

import discovery_utils
import errors
import custom_resource_response

import AccessResourceHandler

class TestAccess(unittest.TestCase):

    event = {}

    context = {}

    def setUp(self):
        reload(AccessResourceHandler)
        self.event = {
            'ResourceProperties': {
                'ConfigurationBucket': 'TestBucket',
                'ConfigurationKey': 'TestKey',
                'RoleLogicalId': 'AccessRole',
                'MetadataKey' : 'ResourceAccess',
                'UsePropagationDelay' : 'false',
                'RequireRoleExists' : 'false',
                'PhysicalResourceId' : 'CloudCanvas:TestAccess:TestStackName'
            },
            'StackId': 'TestStackId',
            'LogicalResourceId': 'TestLogicalResourceId'
        }
        self.properties = self.event['ResourceProperties']

    def test_handler_with_no_resource_group_or_deployment_stack(self):
        print errors.ValidationError
        with self.assertRaises(errors.ValidationError):
            AccessResourceHandler.handler(self.event, self.context)

    def test_handler_with_both_resource_group_and_deployment_stack(self):
        self.properties['ResourceGroupStack'] = 'TestResourceGroupStack'
        self.properties['DeploymentStack'] = 'TestDeploymentStack'
        with self.assertRaises(errors.ValidationError):
            AccessResourceHandler.handler(self.event, self.context)

    def test_handler_with_resource_group_stack_create_without_role(self):
        self._do_handler_with_resource_group_stack_test('Create', with_access_stack=False)

    def test_handler_with_resource_group_stack_create_with_role(self):
        self._do_handler_with_resource_group_stack_test('Create')

    def test_handler_with_resource_group_stack_update(self):
        self._do_handler_with_resource_group_stack_test('Update')

    def _do_handler_with_resource_group_stack_test(self, request_type, with_access_stack = True):

        self.properties['ResourceGroupStack'] = 'TestResourceGroupStackId'
        self.event['RequestType'] = request_type

        mock_aim_client = mock.MagicMock()
        mock_put_role_policy = mock.MagicMock(return_value={})
        mock_aim_client.put_role_policy = mock_put_role_policy

        def mock_describe_stack_resources_side_effect(PhysicalResourceId=None, StackName=None):
           
            if PhysicalResourceId is None and StackName == 'TestProjectStackId':
                return {
                    'StackResources': [
                        {
                            'PhysicalResourceId': 'TestDeploymentStackId',
                            'LogicalResourceId': 'TestDeploymentStackName',
                            'ResourceType': 'AWS::CloudFormation::Stack',
                            'StackId': 'TestProjectStackId'
                        }
                    ]
                }
           
            if PhysicalResourceId == 'TestResourceGroupStackId' or StackName == 'TestDeploymentStackId':
                return {
                    'StackResources': [
                        {
                            'PhysicalResourceId': 'TestResourceGroupStackId',
                            'LogicalResourceId': 'TestResourceGroupStackName',
                            'ResourceType': 'AWS::CloudFormation::Stack',
                            'StackId': 'TestDeploymentStackId'
                        }
                    ]
                }

            if with_access_stack and StackName == 'TestDeploymentStackId-Access':
                return {
                    'StackResources': [
                        {                    
                            'PhysicalResourceId': 'AccessRoleResourceId',
                            'LogicalResourceId': 'AccessRole',
                            'ResourceType': 'AWS::IAM::Role',
                            'StackId': 'TestDeploymentStackId-Access'
                        }
                    ]
                }

            if StackName == 'TestResourceGroupStackId':
                return {
                    'StackResources': [
                        {
                            'PhysicalResourceId': 'AccessibleResourceId1',
                            'LogicalResourceId': 'AccessibleResourceName1',
                            'ResourceType': 'TestResourceType1',
                            'StackId': 'TestResourceGroupStackId'
                        },                
                        {
                            'PhysicalResourceId': 'AccessibleResourceId2',
                            'LogicalResourceId': 'AccessibleResourceName2',
                            'ResourceType': 'TestResourceType2',
                            'StackId': 'TestResourceGroupStackId'
                        },                
                        {
                            'PhysicalResourceId': 'NonAccessibleResourceId',
                            'LogicalResourceId': 'NonAccessibleResourceName',
                            'ResourceType': 'TestResourceType',
                            'StackId': 'TestResourceGroupStackId'
                        }
                    ]
                }

            print '**** mock_describe_stack_resources_side_effect ***', PhysicalResourceId, StackName

            return None

        def mock_describe_stack_resource_side_effect(StackName=None, LogicalResourceId=None):
            
            if StackName != 'TestResourceGroupStackId':
                return None

            if LogicalResourceId == 'AccessibleResourceName1':
                return {
                    'StackResourceDetail': {
                        'PhysicalResourceId': 'AccessibleResourceId1',
                        'LogicalResourceId': 'AccessibleResourceName1',
                        'ResourceType': 'TestResourceType1',
                        'StackId': 'TestResourceGroupStackId',
                        'Metadata': '''{
                            "CloudCanvas": {
                                "ResourceAccess": {
                                    "Action": "TestAction1"
                                }
                            }
                        }'''
                    }
                }
            
            if LogicalResourceId == 'AccessibleResourceName2':
                return {
                    'StackResourceDetail': {
                        'PhysicalResourceId': 'AccessibleResourceId2',
                        'LogicalResourceId': 'AccessibleResourceName2',
                        'ResourceType': 'TestResourceType2',
                        'StackId': 'TestResourceGroupStackId',
                        'Metadata': '''{
                            "CloudCanvas": {
                                "ResourceAccess": {
                                    "Action": [ "TestAction2A", "TestAction2B" ],
                                    "ResourceSuffix": "TestSuffix"
                                }
                            }
                        }'''
                    }
                }
            
            if LogicalResourceId == 'NonAccessibleResourceName':
                return {
                    'StackResourceDetail': {
                        'PhysicalResourceId': 'NonAccessibleResourceId',
                        'LogicalResourceId': 'NonAccessibleResourceName',
                        'ResourceType': 'TestResourceType',
                        'StackId': 'TestResourceGroupStackId'
                    }
                }

            return None

        def mock_describe_stacks_side_effect(StackName=None):
            if StackName == 'TestDeploymentStackName-Access':
                if with_access_stack:
                    return {
                        'Stacks': [
                            {
                                'StackId': 'TestDeploymentStackId-Access',
                                'StackStatus': 'CREATE_COMPLETE'
                            }
                        ]
                    }
                else:
                    return {
                        'Stacks': [
                        ]
                    }
            if StackName == 'TestDeploymentStackId':
                return {
                    'Stacks': [
                        {
                            'Parameters': [
                                {
                                    'ParameterKey': 'DeploymentName',
                                    'ParameterValue': 'TestDeployment'
                                },
                                {
                                    'ParameterKey': 'ProjectStackId',
                                    'ParameterValue': 'TestProjectStackId'
                                }
                            ]
                        }
                    ]
                }
            return None

        def mock_get_stack_name_from_stack_arn_side_effect(stack_arn):
            return stack_arn.replace('Id', 'Name')

        def mock_get_resource_arn_side_effect(stack_arn, resource_type, resource_name):
            return resource_name + 'Arn'

        mock_cf_client = mock.MagicMock()
        mock_cf_client.describe_stack_resources = mock.MagicMock(side_effect=mock_describe_stack_resources_side_effect)
        mock_cf_client.describe_stack_resource = mock.MagicMock(side_effect=mock_describe_stack_resource_side_effect)
        mock_cf_client.describe_stacks = mock.MagicMock(side_effect=mock_describe_stacks_side_effect)

        def boto3_client_side_effect(client_type, region_name=None):
            if client_type == 'iam':
                return mock_aim_client
            elif client_type == 'cloudformation':
                return mock_cf_client
            else:
                return None

        expected_data = {}
        expected_physical_id = 'CloudCanvas:TestAccess:TestStackName'

        with mock.patch.object(custom_resource_response, 'succeed') as mock_custom_resource_response_succeed:
            with mock.patch.object(boto3, 'client') as mock_boto3_client:
                with mock.patch.object(discovery_utils, 'get_stack_name_from_stack_arn') as mock_get_stack_name_from_stack_arn:
                    with mock.patch.object(discovery_utils, 'get_region_from_stack_arn') as mock_get_region_from_stack_arn:
                        with mock.patch.object(discovery_utils, 'get_account_id_from_stack_arn') as mock_get_account_id_from_stack_arn:
                            with mock.patch.object(discovery_utils, 'get_resource_arn') as mock_get_resource_arn:
                                mock_get_stack_name_from_stack_arn.side_effect = mock_get_stack_name_from_stack_arn_side_effect
                                mock_get_region_from_stack_arn.return_value = 'TestRegion'
                                mock_get_account_id_from_stack_arn.return_value = 'TestAccount'
                                mock_get_resource_arn.side_effect = mock_get_resource_arn_side_effect

                                mock_boto3_client.side_effect = boto3_client_side_effect
            
                                reload(AccessResourceHandler)
                                AccessResourceHandler.handler(self.event, self.context)
            
                                if not with_access_stack:
                                    self.assertEquals(mock_put_role_policy.call_count, 0)
                                else:
                                    self.assertEquals(mock_put_role_policy.call_count, 1)
                                    mock_put_role_policy_kwargs = mock_put_role_policy.call_args[1]

                                    self.assertEquals(mock_put_role_policy_kwargs['RoleName'], 'AccessRoleResourceId')
                                    self.assertEquals(mock_put_role_policy_kwargs['PolicyName'], 'TestResourceGroupStackName')

                                    policy_document_string = mock_put_role_policy_kwargs['PolicyDocument']
                                    self.assertTrue(isinstance(policy_document_string, basestring))
                                    policy_document = json.loads(policy_document_string)   
            
                                    expected_policy_document = {
                                        u'Version': u'2012-10-17',
                                        u'Statement': [
                                            {
                                                u'Sid': u'AccessibleResourceName1Access',
                                                u'Effect': u'Allow',
                                                u'Action': [ u'TestAction1' ],
                                                u'Resource': u'AccessibleResourceId1Arn'
                                            },
                                            {
                                                u'Sid': u'AccessibleResourceName2Access',
                                                u'Effect': u'Allow',
                                                u'Action': [ u'TestAction2A', u'TestAction2B' ],
                                                u'Resource': u'AccessibleResourceId2ArnTestSuffix'
                                            }
                                        ]
                                    }
             
                                    self.maxDiff = None
                                    self.assertEquals(expected_policy_document, policy_document)        

                                mock_custom_resource_response_succeed.assert_called_once_with(
                                    self.event, 
                                    self.context, 
                                    expected_data, 
                                    expected_physical_id)



    def test_handler_with_resource_group_stack_delete(self):

        self.properties['ResourceGroupStack'] = 'TestResourceGroupStackId'
        self.event['RequestType'] = 'Delete'

        mock_aim_client = mock.MagicMock()
        mock_delete_role_policy = mock.MagicMock(return_value={})
        mock_aim_client.delete_role_policy = mock_delete_role_policy

        def mock_describe_stack_resources_side_effect(PhysicalResourceId=None, StackName=None):
           
            if PhysicalResourceId == 'TestResourceGroupStackId' or StackName == 'TestDeploymentStackId':
                return {
                    'StackResources': [
                        {
                            'PhysicalResourceId': 'TestResourceGroupStackId',
                            'LogicalResourceId': 'TestResourceGroupStackName',
                            'ResourceType': 'AWS::CloudFormation::Stack',
                            'StackId': 'TestDeploymentStackId'
                        }
                    ]
                }

            if StackName == 'TestDeploymentStackId-Access':
                return {
                    'StackResources': [
                        {                    
                            'PhysicalResourceId': 'AccessRoleResourceId',
                            'LogicalResourceId': 'AccessRole',
                            'ResourceType': 'AWS::IAM::Role',
                            'StackId': 'TestDeploymentStackId-Access'
                        }
                    ]
                }

            if StackName == 'TestResourceGroupStackId':
                return {
                    'StackResources': [
                        {
                            'PhysicalResourceId': 'AccessibleResourceId1',
                            'LogicalResourceId': 'AccessibleResourceName1',
                            'ResourceType': 'TestResourceType1',
                            'StackId': 'TestResourceGroupStackId'
                        },                
                        {
                            'PhysicalResourceId': 'AccessibleResourceId2',
                            'LogicalResourceId': 'AccessibleResourceName2',
                            'ResourceType': 'TestResourceType2',
                            'StackId': 'TestResourceGroupStackId'
                        },                
                        {
                            'PhysicalResourceId': 'NonAccessibleResourceId',
                            'LogicalResourceId': 'NonAccessibleResourceName',
                            'ResourceType': 'TestResourceType',
                            'StackId': 'TestResourceGroupStackId'
                        }
                    ]
                }
                
            print '**** mock_describe_stack_resources_side_effect ***', PhysicalResourceId, StackName

            return None

        def mock_describe_stacks_side_effect(StackName=None):
            if StackName == 'TestDeploymentStackName-Access':
                return {
                    'Stacks': [
                        {
                            'StackId': 'TestDeploymentStackId-Access',
                            'StackStatus': 'CREATE_COMPLETE'
                        }
                    ]
                }
            if StackName == 'TestDeploymentStackId':
                return {
                    'Stacks': [
                        {
                            'Parameters': [
                                {
                                    'ParameterKey': 'DeploymentName',
                                    'ParameterValue': 'TestDeployment'
                                },
                                {
                                    'ParameterKey': 'ProjectStackId',
                                    'ParameterValue': 'TestProjectStackId'
                                }
                            ]
                        }
                    ]
                }
            return None

        def mock_get_stack_name_from_stack_arn_side_effect(stack_arn):
            return stack_arn.replace('Id', 'Name')

        mock_cf_client = mock.MagicMock()
        mock_cf_client.describe_stack_resources = mock.MagicMock(side_effect=mock_describe_stack_resources_side_effect)
        mock_cf_client.describe_stacks = mock.MagicMock(side_effect=mock_describe_stacks_side_effect)

        def boto3_client_side_effect(client_type, region_name=None):
            if client_type == 'iam':
                return mock_aim_client
            elif client_type == 'cloudformation':
                return mock_cf_client
            else:
                return None

        expected_data = {}
        expected_physical_id = 'CloudCanvas:TestAccess:TestStackName'

        with mock.patch.object(custom_resource_response, 'succeed') as mock_custom_resource_response_succeed:
            with mock.patch.object(boto3, 'client') as mock_boto3_client:
                with mock.patch.object(discovery_utils, 'get_stack_name_from_stack_arn') as mock_get_stack_name_from_stack_arn:
                    with mock.patch.object(discovery_utils, 'get_region_from_stack_arn') as mock_get_region_from_stack_arn:
                        with mock.patch.object(discovery_utils, 'get_account_id_from_stack_arn') as mock_get_account_id_from_stack_arn:
                            mock_get_stack_name_from_stack_arn.side_effect = mock_get_stack_name_from_stack_arn_side_effect
                            mock_get_region_from_stack_arn.return_value = 'TestRegion'
                            mock_get_account_id_from_stack_arn.return_value = 'TestAccount'

                            mock_boto3_client.side_effect = boto3_client_side_effect
            
                            reload(AccessResourceHandler)
                            AccessResourceHandler.handler(self.event, self.context)

                            mock_delete_role_policy.assert_called_once_with(RoleName='AccessRoleResourceId', PolicyName='TestResourceGroupStackName')

                            mock_custom_resource_response_succeed.assert_called_once_with(
                                self.event, 
                                self.context, 
                                expected_data, 
                                expected_physical_id)


    def test_handler_with_resource_group_stack_delete_without_role(self):

        self.properties['ResourceGroupStack'] = 'TestResourceGroupStackId'
        self.event['RequestType'] = 'Delete'

        mock_aim_client = mock.MagicMock()
        mock_delete_role_policy = mock.MagicMock(return_value={})
        mock_aim_client.delete_role_policy = mock_delete_role_policy

        def mock_describe_stack_resources_side_effect(PhysicalResourceId=None, StackName=None):
           
            if PhysicalResourceId is None and StackName == 'TestProjectStackId':
                return {
                    'StackResources': [
                        {
                            'PhysicalResourceId': 'TestDeploymentStackId',
                            'LogicalResourceId': 'TestDeploymentStackName',
                            'ResourceType': 'AWS::CloudFormation::Stack',
                            'StackId': 'TestProjectStackId'
                        }
                    ]
                }
           
            if PhysicalResourceId == 'TestResourceGroupStackId' or StackName == 'TestDeploymentStackId':
                return {
                    'StackResources': [
                        {
                            'PhysicalResourceId': 'TestResourceGroupStackId',
                            'LogicalResourceId': 'TestResourceGroupStackName',
                            'ResourceType': 'AWS::CloudFormation::Stack',
                            'StackId': 'TestDeploymentStackId'
                        }
                    ]
                }
                
            if StackName == 'TestResourceGroupStackId':
                return {
                    'StackResources': [
                        {
                            'PhysicalResourceId': 'AccessibleResourceId1',
                            'LogicalResourceId': 'AccessibleResourceName1',
                            'ResourceType': 'TestResourceType1',
                            'StackId': 'TestResourceGroupStackId'
                        },                
                        {
                            'PhysicalResourceId': 'AccessibleResourceId2',
                            'LogicalResourceId': 'AccessibleResourceName2',
                            'ResourceType': 'TestResourceType2',
                            'StackId': 'TestResourceGroupStackId'
                        },                
                        {
                            'PhysicalResourceId': 'NonAccessibleResourceId',
                            'LogicalResourceId': 'NonAccessibleResourceName',
                            'ResourceType': 'TestResourceType',
                            'StackId': 'TestResourceGroupStackId'
                        }
                    ]
                }
                
            print '**** mock_describe_stack_resources_side_effect ***', PhysicalResourceId, StackName

            return None

        def mock_describe_stacks_side_effect(StackName=None):
            if StackName == 'TestDeploymentStackName-Access':
                return {
                    'Stacks': [
                    ]
                }
            if StackName == 'TestDeploymentStackId':
                return {
                    'Stacks': [
                        {
                            'Parameters': [
                                {
                                    'ParameterKey': 'DeploymentName',
                                    'ParameterValue': 'TestDeployment'
                                },
                                {
                                    'ParameterKey': 'ProjectStackId',
                                    'ParameterValue': 'TestProjectStackId'
                                }
                            ]
                        }
                    ]
                }
            return None

        def mock_get_stack_name_from_stack_arn_side_effect(stack_arn):
            return stack_arn.replace('Id', 'Name')

        mock_cf_client = mock.MagicMock()
        mock_cf_client.describe_stack_resources = mock.MagicMock(side_effect=mock_describe_stack_resources_side_effect)
        mock_cf_client.describe_stacks = mock.MagicMock(side_effect=mock_describe_stacks_side_effect)

        def boto3_client_side_effect(client_type, region_name=None):
            if client_type == 'iam':
                return mock_aim_client
            elif client_type == 'cloudformation':
                return mock_cf_client
            else:
                return None

        expected_data = {}
        expected_physical_id = 'CloudCanvas:TestAccess:TestStackName'

        with mock.patch.object(custom_resource_response, 'succeed') as mock_custom_resource_response_succeed:
            with mock.patch.object(boto3, 'client') as mock_boto3_client:
                with mock.patch.object(discovery_utils, 'get_stack_name_from_stack_arn') as mock_get_stack_name_from_stack_arn:
                    with mock.patch.object(discovery_utils, 'get_region_from_stack_arn') as mock_get_region_from_stack_arn:
                        with mock.patch.object(discovery_utils, 'get_account_id_from_stack_arn') as mock_get_account_id_from_stack_arn:
                            mock_get_stack_name_from_stack_arn.side_effect = mock_get_stack_name_from_stack_arn_side_effect
                            mock_get_region_from_stack_arn.return_value = 'TestRegion'
                            mock_get_account_id_from_stack_arn.return_value = 'TestAccount'

                            mock_boto3_client.side_effect = boto3_client_side_effect
            
                            reload(AccessResourceHandler)
                            AccessResourceHandler.handler(self.event, self.context)

                            self.assertEquals(0, mock_delete_role_policy.called)

                            mock_custom_resource_response_succeed.assert_called_once_with(
                                self.event, 
                                self.context, 
                                expected_data, 
                                expected_physical_id)


    def test_handler_with_resource_group_stack_delete_with_role_deleted(self):

        self.properties['ResourceGroupStack'] = 'TestResourceGroupStackId'
        self.event['RequestType'] = 'Delete'

        def mock_delete_role_policy_side_effect(RoleName=None, PolicyName=None):
            raise ClientError({ "Error": { "Code": "AccessDenied" } }, 'DeleteRolePolicy')

        mock_aim_client = mock.MagicMock()
        mock_delete_role_policy = mock.MagicMock(return_value={})
        mock_delete_role_policy.side_effect = mock_delete_role_policy_side_effect
        mock_aim_client.delete_role_policy = mock_delete_role_policy

        def mock_describe_stack_resources_side_effect(PhysicalResourceId=None, StackName=None):
           
            if PhysicalResourceId == 'TestResourceGroupStackId' or StackName == 'TestDeploymentStackId':
                return {
                    'StackResources': [
                        {
                            'PhysicalResourceId': 'TestResourceGroupStackId',
                            'LogicalResourceId': 'TestResourceGroupStackName',
                            'ResourceType': 'AWS::CloudFormation::Stack',
                            'StackId': 'TestDeploymentStackId'
                        }
                    ]
                }

            if StackName == 'TestDeploymentStackId-Access':
                return {
                    'StackResources': [
                        {                    
                            'PhysicalResourceId': 'AccessRoleResourceId',
                            'LogicalResourceId': 'AccessRole',
                            'ResourceType': 'AWS::IAM::Role',
                            'StackId': 'TestDeploymentStackId-Access'
                        }
                    ]
                }
				
            if StackName == 'TestResourceGroupStackId':
                return {
                    'StackResources': [
                        {
                            'PhysicalResourceId': 'AccessibleResourceId1',
                            'LogicalResourceId': 'AccessibleResourceName1',
                            'ResourceType': 'TestResourceType1',
                            'StackId': 'TestResourceGroupStackId'
                        },                
                        {
                            'PhysicalResourceId': 'AccessibleResourceId2',
                            'LogicalResourceId': 'AccessibleResourceName2',
                            'ResourceType': 'TestResourceType2',
                            'StackId': 'TestResourceGroupStackId'
                        },                
                        {
                            'PhysicalResourceId': 'NonAccessibleResourceId',
                            'LogicalResourceId': 'NonAccessibleResourceName',
                            'ResourceType': 'TestResourceType',
                            'StackId': 'TestResourceGroupStackId'
                        }
                    ]
                }
				
            print '**** mock_describe_stack_resources_side_effect ***', PhysicalResourceId, StackName

            return None

        def mock_describe_stacks_side_effect(StackName=None):
            if StackName == 'TestDeploymentStackName-Access':
                return {
                    'Stacks': [
                        {
                            'StackId': 'TestDeploymentStackId-Access',
                            'StackStatus': 'CREATE_COMPLETE'
                        }
                    ]
                }
            if StackName == 'TestDeploymentStackId':
                return {
                    'Stacks': [
                        {
                            'Parameters': [
                                {
                                    'ParameterKey': 'DeploymentName',
                                    'ParameterValue': 'TestDeployment'
                                },
                                {
                                    'ParameterKey': 'ProjectStackId',
                                    'ParameterValue': 'TestProjectStackId'
                                }
                            ]
                        }
                    ]
                }
            return None

        def mock_get_stack_name_from_stack_arn_side_effect(stack_arn):
            return stack_arn.replace('Id', 'Name')

        mock_cf_client = mock.MagicMock()
        mock_cf_client.describe_stack_resources = mock.MagicMock(side_effect=mock_describe_stack_resources_side_effect)
        mock_cf_client.describe_stacks = mock.MagicMock(side_effect=mock_describe_stacks_side_effect)

        def boto3_client_side_effect(client_type, region_name=None):
            if client_type == 'iam':
                return mock_aim_client
            elif client_type == 'cloudformation':
                return mock_cf_client
            else:
                return None

        expected_data = {}
        expected_physical_id = 'CloudCanvas:TestAccess:TestStackName'

        with mock.patch.object(custom_resource_response, 'succeed') as mock_custom_resource_response_succeed:
            with mock.patch.object(boto3, 'client') as mock_boto3_client:
                with mock.patch.object(discovery_utils, 'get_stack_name_from_stack_arn') as mock_get_stack_name_from_stack_arn:
                    with mock.patch.object(discovery_utils, 'get_region_from_stack_arn') as mock_get_region_from_stack_arn:
                        with mock.patch.object(discovery_utils, 'get_account_id_from_stack_arn') as mock_get_account_id_from_stack_arn:
                            mock_get_stack_name_from_stack_arn.side_effect = mock_get_stack_name_from_stack_arn_side_effect
                            mock_get_region_from_stack_arn.return_value = 'TestRegion'
                            mock_get_account_id_from_stack_arn.return_value = 'TestAccount'

                            mock_boto3_client.side_effect = boto3_client_side_effect
            
                            reload(AccessResourceHandler)
                            AccessResourceHandler.handler(self.event, self.context)

                            mock_delete_role_policy.assert_called_once_with(RoleName='AccessRoleResourceId', PolicyName='TestResourceGroupStackName')

                            mock_custom_resource_response_succeed.assert_called_once_with(
                                self.event, 
                                self.context, 
                                expected_data, 
                                expected_physical_id)

    def test_handler_with_deployment_stack(self):

        self.properties['DeploymentStack'] = 'TestDeploymentStackId'
        self.event['RequestType'] = 'TestRequestType'

        def mock_describe_stack_resources_side_effect(PhysicalResourceId=None, StackName=None):
           
            if StackName == 'TestDeploymentStackId':
                return {
                    'StackResources': [
                        {
                            'PhysicalResourceId': 'TestResourceGroupStackId1',
                            'LogicalResourceId': 'TestResourceGroupStackName1',
                            'ResourceType': 'AWS::CloudFormation::Stack',
                            'StackId': 'TestDeploymentStackId'
                        },
                        {
                            'PhysicalResourceId': 'TestResourceGroupStackId2',
                            'LogicalResourceId': 'TestResourceGroupStackName2',
                            'ResourceType': 'AWS::CloudFormation::Stack',
                            'StackId': 'TestDeploymentStackId'
                        },
                        {
                            'PhysicalResourceId': 'OtherId',
                            'LogicalResourceId': 'OtherName',
                            'ResourceType': 'OtherType',
                            'StackId': 'TestDeploymentStackId'
                        }
                    ]
                }

            if StackName == 'TestDeploymentStackId-Access':
                return {
                    'StackResources': [
                        {                    
                            'PhysicalResourceId': 'AccessRoleResourceId',
                            'LogicalResourceId': 'AccessRole',
                            'ResourceType': 'AWS::IAM::Role',
                            'StackId': 'TestDeploymentStackId-Access'
                        }
                    ]
                }

            print '**** mock_describe_stack_resources_side_effect ***', PhysicalResourceId, StackName

            return None

        def mock_get_stack_name_from_stack_arn_side_effect(stack_arn):
            return stack_arn.replace('Id', 'Name')

        def mock_describe_stacks_side_effect(StackName = None):
            if StackName == 'TestDeploymentStackName-Access':
                return {
                    'Stacks': [
                        {
                            'StackId': 'TestDeploymentStackId-Access',
                            'StackStatus': 'CREATE_COMPLETE'
                        }
                    ]
                }
            if StackName == 'TestDeploymentStackId':
                return {
                    'Stacks': [
                        {
                            'Parameters': [
                                {
                                    'ParameterKey': 'DeploymentName',
                                    'ParameterValue': 'TestDeployment'
                                },
                                {
                                    'ParameterKey': 'ProjectStackId',
                                    'ParameterValue': 'TestProjectStackId'
                                }
                            ]
                        }
                    ]
                }
            return None

        mock_cf_client = mock.MagicMock()
        mock_cf_client.describe_stack_resources = mock.MagicMock(side_effect=mock_describe_stack_resources_side_effect)
        mock_cf_client.describe_stacks = mock.MagicMock(side_effect=mock_describe_stacks_side_effect)

        def boto3_client_side_effect(client_type, region_name=None):
            if client_type == 'cloudformation':
                return mock_cf_client
            else:
                return None

        expected_data = {}
        expected_physical_id = 'CloudCanvas:TestAccess:TestStackName'

        with mock.patch.object(custom_resource_response, 'succeed') as mock_custom_resource_response_succeed:
            with mock.patch.object(boto3, 'client') as mock_boto3_client:
                with mock.patch.object(discovery_utils, 'get_stack_name_from_stack_arn') as mock_get_stack_name_from_stack_arn:
                    with mock.patch.object(discovery_utils, 'get_region_from_stack_arn') as mock_get_region_from_stack_arn:
                        with mock.patch.object(discovery_utils, 'get_account_id_from_stack_arn') as mock_get_account_id_from_stack_arn:

                            mock_get_stack_name_from_stack_arn.side_effect = mock_get_stack_name_from_stack_arn_side_effect
                            mock_get_region_from_stack_arn.return_value = 'TestRegion'
                            mock_get_account_id_from_stack_arn.return_value = 'TestAccount'

                            mock_boto3_client.side_effect = boto3_client_side_effect
            
                            reload(AccessResourceHandler)
                            
                            with mock.patch.object(AccessResourceHandler, '_process_resource_group_stack') as mock_process_resource_group_stack:

                                AccessResourceHandler.handler(self.event, self.context)
                                
                                self.assertEquals(mock_process_resource_group_stack.call_count, 2)
                                mock_process_resource_group_stack.assert_any_call('TestRequestType', ResourceGroupInfoForResourceGroup('TestResourceGroupStackName1'), 'AccessRoleResourceId', 'ResourceAccess', False)
                                mock_process_resource_group_stack.assert_any_call('TestRequestType', ResourceGroupInfoForResourceGroup('TestResourceGroupStackName2'), 'AccessRoleResourceId', 'ResourceAccess', False)

                                mock_custom_resource_response_succeed.assert_called_once_with(
                                    self.event, 
                                    self.context, 
                                    expected_data, 
                                    expected_physical_id)

class ResourceGroupInfoForResourceGroup(object):
     
    def __init__(self, resource_group_name):
        self.resource_group_name = resource_group_name

    def __repr__(self):
        return 'ResourceGroupInfoForResourceGroup(resource_group_name="{}")'.format(self.resource_group_name)

    def __eq__(self, other):
        return other.resource_group_name == self.resource_group_name
