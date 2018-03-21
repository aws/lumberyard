#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the 'License'). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an 'AS IS' BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#
# $Revision: #17 $

import os
import json
import shutil
import unittest
from copy import deepcopy
from time import sleep

from botocore.exceptions import ClientError

import resource_manager_common.constant

import lmbr_aws_test_support
import mock_specification

class IntegrationTest_CloudGemFramework_ResourceManager_Security(lmbr_aws_test_support.lmbr_aws_TestCase):

    OTHER_CONTEXT = 'OtherContext'

    def setUp(self):    
        self.prepare_test_envionment(type(self).__name__, alternate_context_names=[self.OTHER_CONTEXT])

        
    def test_security_end_to_end(self):
        self.run_all_tests()


    def __100_create_other_project(self):

        # We verify that access granted to a target project does not allow access 
        # to the "other" project.

        with self.alternate_context(self.OTHER_CONTEXT):

            self.lmbr_aws(
                'project', 'create', 
                '--stack-name', self.TEST_PROJECT_STACK_NAME, 
                '--confirm-aws-usage',
                '--confirm-security-change', 
                '--region', lmbr_aws_test_support.REGION
            )

            self.lmbr_aws(
                'cloud-gem', 'create',
                '--gem', self.TEST_RESOURCE_GROUP_NAME, 
                '--initial-content', 'api-lambda-dynamodb',
                '--enable'
            )

            self.lmbr_aws(
                'deployment', 'create', 
                '--deployment', self.TEST_DEPLOYMENT_NAME, 
                '--confirm-aws-usage', 
                '--confirm-security-change'
            )
        

    def __120_create_project_stack(self):
        self.lmbr_aws(
            'project', 'create', 
            '--stack-name', self.TEST_PROJECT_STACK_NAME, 
            '--confirm-aws-usage',
            '--confirm-security-change', 
            '--region', lmbr_aws_test_support.REGION
        )


    def __130_verify_project_stack_permission(self):

        project_stack_arn = self.get_project_stack_arn()
        configuration_bucket_arn = self.get_stack_resource_arn(project_stack_arn, 'Configuration')
        logs_bucket_arn = self.get_stack_resource_arn(project_stack_arn, 'Logs')
        project_resource_handler_role_arn = self.get_lambda_function_execution_role_arn(project_stack_arn, 'ProjectResourceHandler')
        project_token_exchange_handler_role_arn = self.get_lambda_function_execution_role_arn(project_stack_arn, 'PlayerAccessTokenExchange')
        project_service_lambda_role_arn = self.get_lambda_function_execution_role_arn(project_stack_arn, 'ServiceLambda')

        project_level_role_arn = self.get_stack_resource_arn(project_stack_arn, 'ProjectOwner')
        project_level_policy_arn = self.get_stack_resource_arn(project_stack_arn, 'ProjectAccess')

        # We will verify that what is granted for the target stack isn't ganted for the "other" stack.
        with self.alternate_context(self.OTHER_CONTEXT):
            other_project_stack_arn = self.get_project_stack_arn()
            other_configuration_bucket_arn = self.get_stack_resource_arn(other_project_stack_arn, 'Configuration')
            other_logs_bucket_arn = self.get_stack_resource_arn(other_project_stack_arn, 'Logs')
            other_project_level_role_arn = self.get_stack_resource_arn(other_project_stack_arn, 'ProjectOwner')
            other_project_level_policy_arn = self.get_stack_resource_arn(other_project_stack_arn, 'ProjectAccess')

        # Helper function for testing permissions granted to both ProjectOwner and ProjectAdmin. We test
        # the roles instead of the ProjectOwnerAccess policy directly (it's an integration test after all).
        def verify_permissions_for_role(role_name):

            self.verify_role_permissions('project',
                project_stack_arn,
                self.get_stack_resource_physical_id(project_stack_arn, role_name),
                [

                    # SID FullAccessToProjectConfigurationAndLogs
                    {
                        'Description': 'SID FullAccessToProjectConfigurationAndLogs - Owner/Admin gets full access for target project.',
                        'Resources': [
                            configuration_bucket_arn,
                            logs_bucket_arn
                        ],
                        'Allow': [
                            # who knows what the owner/admin needs to do to fix a problem... don't be restrive here
                            's3:*' 
                        ]
                    },
                    {
                        'Description': 'SID FullAccessToProjectConfigurationAndLogs - Owner/Admin denied access for other project.',
                        'Resources': [
                            other_configuration_bucket_arn,
                            other_logs_bucket_arn
                        ],
                        'Deny': [
                            # verify a permission they have on their project is denied on the other project
                            's3:ListBucket' 
                        ]
                    },

                    # SID CreateAndManageProjectAndDeploymentRoles
                    {
                        'Description': 'SID CreateAndManageProjectAndDeploymentRoles -- Owner/Admin denied full accees for target project.',
                        'Resources': [
                            project_level_role_arn
                        ],
                        'Deny': [
                            # never grant all IAM operations, be selective
                            'iam:*' 
                        ]
                    },
                    {
                        'Description': 'SID CreateAndManageProjectAndDeploymentRoles -- Owner/Admin allowed specific accees for target project.',
                        'Resources': [
                            project_level_role_arn
                        ],
                        'Allow': [
                            # This is not intended to be a complete list of what is actually granted.
                            # Other tests that use the role verify that it has what it needs to do its job
                            # Here, along with the test using the "other" resources below, we are verifing
                            # that a permission ganted in this project do not extend to other projects.
                            'iam:GetRole'
                        ]
                    },
                    {
                        'Description': 'SID CreateAndManageProjectAndDeploymentRoles -- Owner/Admin denied specific accees for other project.',
                        'Resources': [
                            other_project_level_role_arn
                        ],
                        'Deny': [
                            # verify a permission they have on their project is denied on the other project
                            'iam:GetRole'
                        ]
                    }
                ])

        # now call the helper function for each role
        verify_permissions_for_role('ProjectOwner')
        verify_permissions_for_role('ProjectAdmin')


    def __140_create_project_user(self):
        self.create_project_user()


    def __160_update_project_stack_using_project_owner_role(self):

        settings = self.load_local_project_settings()
        project_stack_arn = settings['ProjectStackId']

        res = self.aws_cloudformation.describe_stacks(StackName = project_stack_arn)
        timestamp_before_update = res['Stacks'][0]['LastUpdatedTime']

        self.lmbr_aws(
            'project', 'update', 
            '--confirm-aws-usage',
            '--confirm-security-change',
            project_role='ProjectOwner'
        )

        self.verify_stack("project stack", project_stack_arn, mock_specification.ok_project_stack())

        res = self.aws_cloudformation.describe_stacks(StackName = project_stack_arn)
        self.assertNotEqual(timestamp_before_update, res['Stacks'][0]['LastUpdatedTime'], 'update-project-stack did not update the stack')


    def __170_update_project_stack_using_project_admin_role(self):

        settings = self.load_local_project_settings()
        project_stack_arn = settings['ProjectStackId']

        res = self.aws_cloudformation.describe_stacks(StackName = project_stack_arn)
        timestamp_before_update = res['Stacks'][0]['LastUpdatedTime']

        self.lmbr_aws(
            'project', 'update',
            '--confirm-aws-usage', 
            project_role='ProjectAdmin'
        )

        self.verify_stack("project stack", project_stack_arn, mock_specification.ok_project_stack())

        res = self.aws_cloudformation.describe_stacks(StackName = project_stack_arn)
        self.assertNotEqual(timestamp_before_update, res['Stacks'][0]['LastUpdatedTime'], 'update-project-stack did not update the stack')


    def __180_project_admin_cant_create_Release_stack(self):
        self.lmbr_aws(
            'deployment', 'create',
            '--deployment', 'Release1', 
            '--confirm-aws-usage', 
            '--confirm-security-change', 
            project_role='ProjectAdmin', 
            expect_failure=True
        )


    def __185_add_resource_group(self):     
        self.lmbr_aws(
            'cloud-gem', 'create',
            '--gem', self.TEST_RESOURCE_GROUP_NAME, 
            '--initial-content', 'api-lambda-dynamodb',
            '--enable'
        )


    def __190_create_deployment_using_project_admin_role(self):
        self.lmbr_aws(
            'deployment', 'create', 
            '--deployment', self.TEST_DEPLOYMENT_NAME, 
            '--confirm-aws-usage', 
            '--confirm-security-change', 
            project_role='ProjectAdmin'
        )            


    def __200_verify_access_control_in_created_deployment_stack(self):
        self.verify_access_control()


    def verify_access_control(self, expected_mappings_for_logical_ids = []):

        resource_group_stack_arn = self.get_stack_resource_arn(self.get_deployment_stack_arn(self.TEST_DEPLOYMENT_NAME), self.TEST_RESOURCE_GROUP_NAME)

        self.refresh_stack_resources(resource_group_stack_arn)

        player_role_arn = self.get_stack_resource_arn(self.get_deployment_access_stack_arn(self.TEST_DEPLOYMENT_NAME), 'Player')
        resource_group_stack_arn = self.get_stack_resource_arn(self.get_deployment_stack_arn(self.TEST_DEPLOYMENT_NAME), self.TEST_RESOURCE_GROUP_NAME)
        service_api_arn = self.get_stack_resource_arn(resource_group_stack_arn, 'ServiceApi')
        service_lambda_arn = self.get_stack_resource_arn(resource_group_stack_arn, 'ServiceLambda')
        table_arn = self.get_stack_resource_arn(resource_group_stack_arn, 'Table')

        # player should be able to invoke the api, but not call the lambda or get an item from the table
        self.verify_role_permissions('deployment',
            self.get_deployment_access_stack_arn(self.TEST_DEPLOYMENT_NAME),
            self.get_stack_resource_physical_id(self.get_deployment_access_stack_arn(self.TEST_DEPLOYMENT_NAME), 'Player'),
            [
                {
                    'Resources': [
                        service_api_arn + '/api/GET/example/data'
                    ],
                    'Allow': [
                        'execute-api:Invoke'
                    ]
                },
                {
                    'Resources': [
                        service_lambda_arn
                    ],
                    'Deny': [
                        'lambda:InvokeFunction'
                    ]
                },
                {
                    'Resources': [
                        table_arn
                    ],
                    'Deny': [
                        'dynamodb:GetItem'
                    ]
                }
            ])

        # the service lambda should be able to get an item from the table
        res = self.aws_lambda.get_function(FunctionName = service_lambda_arn)
        service_lambda_role_name = self.get_role_name_from_arn(res['Configuration']['Role'])
        self.verify_role_permissions('deployment',
            self.get_deployment_access_stack_arn(self.TEST_DEPLOYMENT_NAME),
            service_lambda_role_name,
            [
                {
                    'Resources': [
                        table_arn
                    ],
                    'Allow': [
                        'dynamodb:GetItem'
                    ]
                }
            ])

        # deployment admin should be able to call the api, invoke the lambda, and get an item from the table
        self.verify_role_permissions('deployment',
            self.get_deployment_access_stack_arn(self.TEST_DEPLOYMENT_NAME),
            self.get_stack_resource_physical_id(self.get_deployment_access_stack_arn(self.TEST_DEPLOYMENT_NAME), 'DeploymentAdmin'),
            [
                {
                    'Resources': [
                        service_api_arn + '/api/GET/example/data'
                    ],
                    'Allow': [
                        'execute-api:Invoke'
                    ]
                },
                {
                    'Resources': [
                        service_lambda_arn
                    ],
                    'Allow': [
                        'lambda:InvokeFunction'
                    ]
                },
                {
                    'Resources': [
                        table_arn
                    ],
                    'Allow': [
                        'dynamodb:GetItem'
                    ]
                }
            ]
        )

        # project admin should be able to call the api, invoke the lambda, and get an item from the table
        self.verify_role_permissions('project',
            self.get_project_stack_arn(),
            self.get_stack_resource_physical_id(self.get_project_stack_arn(), 'ProjectAdmin'),
            [
                {
                    'Resources': [
                        service_api_arn + '/api/GET/example/data'
                    ],
                    'Allow': [
                        'execute-api:Invoke'
                    ]
                },
                {
                    'Resources': [
                        service_lambda_arn
                    ],
                    'Allow': [
                        'lambda:InvokeFunction'
                    ]
                },
                {
                    'Resources': [
                        table_arn
                    ],
                    'Allow': [
                        'dynamodb:GetItem'
                    ]
                }
            ]
        )


        expected_logical_ids = [ self.TEST_RESOURCE_GROUP_NAME + '.ServiceApi' ]
        for logical_id in expected_mappings_for_logical_ids:
            expected_logical_ids.append(self.TEST_RESOURCE_GROUP_NAME + '.' + logical_id)
        self.verify_user_mappings(self.TEST_DEPLOYMENT_NAME, expected_logical_ids)
        self.verify_release_mappings(self.TEST_DEPLOYMENT_NAME, expected_logical_ids, 'Player')
        self.verify_release_mappings(self.TEST_DEPLOYMENT_NAME, [], 'Server')


    def __210_create_deployment_user(self):
        self.create_deployment_user(self.TEST_DEPLOYMENT_NAME)


    def __220_list_deployments_using_project_admin(self):
        self.lmbr_aws(
            'deployment', 'list',
            project_role='ProjectAdmin'
        )
        self.assertIn(self.TEST_DEPLOYMENT_NAME, self.lmbr_aws_stdout)
        self.assertIn('CREATE_COMPLETE', self.lmbr_aws_stdout)


    def __230_list_deployments_using_deployment_admin(self):
        self.lmbr_aws(
            'deployment', 'list',
            deployment_role='DeploymentAdmin'
        )
        self.assertIn(self.TEST_DEPLOYMENT_NAME, self.lmbr_aws_stdout)
        self.assertIn('CREATE_COMPLETE', self.lmbr_aws_stdout)


    def __250_list_resource_groups_using_project_admin(self):
        self.lmbr_aws(
            'resource-group', 'list', 
            project_role='ProjectAdmin'
        )
        self.assertIn(self.TEST_RESOURCE_GROUP_NAME, self.lmbr_aws_stdout)


    def __260_list_resource_groups_using_deployment_admin(self):
        self.lmbr_aws(
            'resource-group', 'list',
            deployment_role='DeploymentAdmin'
        )
        self.assertIn(self.TEST_RESOURCE_GROUP_NAME, self.lmbr_aws_stdout)


    def __270_update_deployment_stack_using_project_admin(self):
        self.lmbr_aws(
            'deployment', 'upload', 
            '--deployment', self.TEST_DEPLOYMENT_NAME, 
            '--confirm-aws-usage', 
            '--confirm-security-change', 
            project_role='ProjectAdmin'
        )


    def __275_update_deployment_stack_using_deployment_admin(self):
        self.lmbr_aws(
            'deployment', 'upload', 
            '--deployment', self.TEST_DEPLOYMENT_NAME, 
            '--confirm-aws-usage', 
            '--confirm-security-change', 
            deployment_role='DeploymentAdmin'
        )


    def __280_verify_access_control_unchanged_in_updated_deployment_stack(self):
        self.verify_access_control()


    def __290_list_resource_groups_for_deployment_using_deployment_admin_role(self):
        self.lmbr_aws(
            'resource-group', 'list',
            '--deployment', self.TEST_DEPLOYMENT_NAME, 
            deployment_role='DeploymentAdmin'
        )
        self.assertIn(self.TEST_RESOURCE_GROUP_NAME, self.lmbr_aws_stdout)
        self.assertIn('UPDATE_COMPLETE', self.lmbr_aws_stdout)


    def __300_list_resource_groups_for_deployment_using_project_admin_role(self):
        self.lmbr_aws(
            'resource-group', 'list',
            '--deployment', self.TEST_DEPLOYMENT_NAME, 
            deployment_role='DeploymentAdmin'
        )
        self.assertIn(self.TEST_RESOURCE_GROUP_NAME, self.lmbr_aws_stdout)
        self.assertIn('UPDATE_COMPLETE', self.lmbr_aws_stdout)


    def __310_list_resources_using_deployment_admin_role(self):
        self.lmbr_aws(
            'deployment', 'list-resources', 
            '--deployment', self.TEST_DEPLOYMENT_NAME, 
            deployment_role='DeploymentAdmin'
        )
        self.assertIn('ServiceLambda', self.lmbr_aws_stdout)


    def __320_list_resources_using_project_admin_role(self):
        self.lmbr_aws(
            'deployment', 'list-resources', 
            '--deployment', self.TEST_DEPLOYMENT_NAME, 
            project_role='ProjectAdmin'
        )
        self.assertIn('ServiceLambda', self.lmbr_aws_stdout)


    def __350_add_test_function_to_resource_group(self):              
        self.add_lambda_function_to_resource_group(self.TEST_RESOURCE_GROUP_NAME, 'TestFunction')


    def __360_update_resource_group_stack_with_to_create_test_function(self):              
        self.lmbr_aws(
            'resource-group', 'upload-resources', 
            '--deployment', self.TEST_DEPLOYMENT_NAME, 
            '--resource-group', self.TEST_RESOURCE_GROUP_NAME, 
            '--confirm-aws-usage', 
            '--confirm-security-change', 
            deployment_role='DeploymentAdmin'
        )


    def __370_verify_updated_resource_group_stack_with_with_test_function(self):
        self.verify_stack("resource group stack", self.get_resource_group_stack_arn(self.TEST_DEPLOYMENT_NAME, self.TEST_RESOURCE_GROUP_NAME),
            {
                'StackStatus': 'UPDATE_COMPLETE',
                'StackResources': {
                    'Table': {
                        'ResourceType': 'AWS::DynamoDB::Table'
                    },
                    'ServiceLambdaConfiguration': {
                        'ResourceType': 'Custom::LambdaConfiguration'
                    },
                    'ServiceLambda': {
                        'ResourceType': 'AWS::Lambda::Function',
                        'Permissions': [
                            {
                                'Resources': [
                                    '$Table$'
                                ],
                                'Allow': [
                                    'dynamodb:PutItem'
                                ]
                            }
                        ]
                    },
                    'ServiceApi': {
                        'ResourceType': 'Custom::ServiceApi'
                    },
                    'TestFunctionConfiguration': {
                        'ResourceType': 'Custom::LambdaConfiguration'
                    },
                    'TestFunction': {
                        'ResourceType': 'AWS::Lambda::Function',
                        'Permissions': [
                            {
                                'Resources': [
                                    '$Table$'
                                ],
                                'Allow': [
                                    'dynamodb:PutItem'
                                ]
                            }
                        ]
                    },
                    'AccessControl': {
                        'ResourceType': 'Custom::AccessControl'
                    }
                }
            })

        self.verify_access_control(expected_mappings_for_logical_ids = [ 'TestFunction' ])
                        
        resource_group_stack_arn = self.get_stack_resource_arn(self.get_deployment_stack_arn(self.TEST_DEPLOYMENT_NAME), self.TEST_RESOURCE_GROUP_NAME)
        test_function_arn = self.get_stack_resource_arn(resource_group_stack_arn, 'TestFunction')

        self.verify_role_permissions('deployment',
            self.get_deployment_access_stack_arn(self.TEST_DEPLOYMENT_NAME),
            self.get_stack_resource_physical_id(self.get_deployment_access_stack_arn(self.TEST_DEPLOYMENT_NAME), 'Player'),
            [
                {
                    'Resources': [
                        test_function_arn
                    ],
                    'Allow': [
                        'lambda:InvokeFunction'
                    ]
                }
            ])

        self.verify_role_permissions('deployment',
            self.get_deployment_access_stack_arn(self.TEST_DEPLOYMENT_NAME),
            self.get_stack_resource_physical_id(self.get_deployment_access_stack_arn(self.TEST_DEPLOYMENT_NAME), 'DeploymentAdmin'),
            [
                {
                    'Resources': [
                        test_function_arn
                    ],
                    'Allow': [
                        'lambda:InvokeFunction'
                    ]
                }
            ])

        self.verify_role_permissions('project',
            self.get_project_stack_arn(),
            self.get_stack_resource_physical_id(self.get_project_stack_arn(), 'ProjectAdmin'),
            [
                {
                    'Resources': [
                        test_function_arn
                    ],
                    'Allow': [
                        'lambda:InvokeFunction'
                    ]
                }
            ])


    def __380_remove_player_access_from_test_function(self):
        self.lmbr_aws(
            'permission', 'remove',
            '--resource-group', self.TEST_RESOURCE_GROUP_NAME,
            '--resource', 'TestFunction',
            '--role', 'Player'
        )


    def __390_update_resource_group_stack_with_removed_player_access(self):
        self.lmbr_aws(
            'resource-group', 'upload-resources', 
            '--deployment', self.TEST_DEPLOYMENT_NAME, 
            '--resource-group', self.TEST_RESOURCE_GROUP_NAME, 
            '--confirm-aws-usage', 
            '--confirm-security-change', 
            deployment_role='DeploymentAdmin'
        )


    def __400_verify_resource_group_stack_with_removed_player_access(self):
        self.verify_stack("resource group stack", self.get_resource_group_stack_arn(self.TEST_DEPLOYMENT_NAME, self.TEST_RESOURCE_GROUP_NAME),
            {
                'StackStatus': 'UPDATE_COMPLETE',
                'StackResources': {
                    'Table': {
                        'ResourceType': 'AWS::DynamoDB::Table'
                    },
                    'ServiceLambdaConfiguration': {
                        'ResourceType': 'Custom::LambdaConfiguration'
                    },
                    'ServiceLambda': {
                        'ResourceType': 'AWS::Lambda::Function',
                        'Permissions': [
                            {
                                'Resources': [
                                    '$Table$'
                                ],
                                'Allow': [
                                    'dynamodb:PutItem'
                                ]
                            }
                        ]
                    },
                    'ServiceApi': {
                        'ResourceType': 'Custom::ServiceApi'
                    },
                    'TestFunctionConfiguration': {
                        'ResourceType': 'Custom::LambdaConfiguration'
                    },
                    'TestFunction': {
                        'ResourceType': 'AWS::Lambda::Function',
                        'Permissions': [
                            {
                                'Resources': [
                                    '$Table$'
                                ],
                                'Allow': [
                                    'dynamodb:PutItem'
                                ]
                            }
                        ]
                    },
                    'AccessControl': {
                        'ResourceType': 'Custom::AccessControl'
                    }
                }
            })

        self.verify_access_control()

        resource_group_stack_arn = self.get_stack_resource_arn(self.get_deployment_stack_arn(self.TEST_DEPLOYMENT_NAME), self.TEST_RESOURCE_GROUP_NAME)
        test_function_arn = self.get_stack_resource_arn(resource_group_stack_arn, 'TestFunction')

        self.verify_role_permissions('deployment',
            self.get_deployment_access_stack_arn(self.TEST_DEPLOYMENT_NAME),
            self.get_stack_resource_physical_id(self.get_deployment_access_stack_arn(self.TEST_DEPLOYMENT_NAME), 'Player'),
            [
                {
                    'Resources': [
                        test_function_arn
                    ],
                    'Deny': [
                        'lambda:InvokeFunction'
                    ]
                }
            ])

        self.verify_role_permissions('deployment',
            self.get_deployment_access_stack_arn(self.TEST_DEPLOYMENT_NAME),
            self.get_stack_resource_physical_id(self.get_deployment_access_stack_arn(self.TEST_DEPLOYMENT_NAME), 'DeploymentAdmin'),
            [
                {
                    'Resources': [
                        test_function_arn
                    ],
                    'Allow': [
                        'lambda:InvokeFunction'
                    ]
                }
            ])

        self.verify_role_permissions('project',
            self.get_project_stack_arn(),
            self.get_stack_resource_physical_id(self.get_project_stack_arn(), 'ProjectAdmin'),
            [
                {
                    'Resources': [
                        test_function_arn
                    ],
                    'Allow': [
                        'lambda:InvokeFunction'
                    ]
                }
            ])


    def __910_delete_deployment_user(self):
        self.delete_deployment_user(self.TEST_DEPLOYMENT_NAME)


    def __920_delete_deployment_stack_using_project_admin(self):
        self.lmbr_aws(
            'deployment', 'delete',
            '--deployment', self.TEST_DEPLOYMENT_NAME, 
            '--confirm-resource-deletion',
            project_role='ProjectAdmin'
        )


    def __940_delete_project_user(self):
        self.delete_project_user()


    def __950_delete_project_stack(self):
        self.lmbr_aws(
            'project', 'delete', 
            '--confirm-resource-deletion'
        )

    def __960_delete_other_project(self):

        with self.alternate_context(self.OTHER_CONTEXT):

            self.lmbr_aws(
                'deployment', 'delete',
                '--deployment', self.TEST_DEPLOYMENT_NAME, 
                '--confirm-resource-deletion'
            )

            self.lmbr_aws(
                'project', 'delete', 
                '--confirm-resource-deletion'
            )


    def add_lambda_function_to_resource_group(self, resource_group_name, function_name):

        resource_group_path = self.get_gem_aws_path(resource_group_name)
        resource_group_template_path = os.path.join(resource_group_path, resource_manager_common.constant.RESOURCE_GROUP_TEMPLATE_FILENAME)

        with open(resource_group_template_path, 'r') as f:
            resource_group_template = json.load(f)

        resource_group_resources = resource_group_template['Resources']

        resource_group_resources['Table']['Metadata']['CloudCanvas']['Permissions'].append(
            {
                'AbstractRole': function_name,
                'Action': 'dynamodb:PutItem'
            })

        resource_group_resources[function_name + 'Configuration'] = {
            "Type": "Custom::LambdaConfiguration",
            "Properties": {
                "ServiceToken": { "Ref": "ProjectResourceHandler" },
                "ConfigurationBucket": { "Ref": "ConfigurationBucket" },
                "ConfigurationKey": { "Ref": "ConfigurationKey" },
                "FunctionName": function_name,
                "Runtime": "python2.7"
            }
        }

        resource_group_resources[function_name] = {
            "Type": "AWS::Lambda::Function",
            "Properties": {
                "Description": "Example of a function called by the game to write data into a DynamoDB table.",
                "Handler": "main.test",
                "Role": { "Fn::GetAtt": [ function_name + "Configuration", "Role" ] },
                "Runtime": { "Fn::GetAtt": [ function_name + "Configuration", "Runtime" ] },
                "Code": {
                    "S3Bucket": { "Fn::GetAtt": [ function_name + "Configuration", "ConfigurationBucket" ] },
                    "S3Key": { "Fn::GetAtt": [ function_name + "Configuration", "ConfigurationKey" ] }
                }
            },
            "Metadata": {
                "CloudCanvas": {
                    "Permissions": [
                        {
                            "Action": "lambda:InvokeFunction",
                            "AbstractRole": "Player"
                        }
                    ]
                }
            }
        }

        resource_group_resources['AccessControl']['DependsOn'].append(function_name)

        with open(resource_group_template_path, 'w') as f:
            json.dump(resource_group_template, f)

        lambda_code_directory_path = os.path.join(resource_group_path, 'lambda-code', function_name)
        if not os.path.isdir(lambda_code_directory_path):
            os.makedirs(lambda_code_directory_path)

        lambda_code_file = os.path.join(lambda_code_directory_path, 'main.py')
        with open(lambda_code_file, 'w') as f:
            f.write('print "testing"')

    def get_lambda_function_execution_role_arn(self, stack_arn, function_name):
        function_arn = self.get_stack_resource_arn(stack_arn, function_name)
        res = self.aws_lambda.get_function(FunctionName = function_arn)
        role_arn = res['Configuration']['Role']
        return role_arn

