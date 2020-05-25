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
import warnings

import resource_manager_common.constant
from . import lmbr_aws_test_support
import mock_specification

from cgf_utils import aws_utils
from cgf_utils import aws_sts
from cgf_utils import aws_iam_role
from resource_manager.security import IdentityPoolUtils


class IntegrationTest_CloudGemFramework_ResourceManager_Security(lmbr_aws_test_support.lmbr_aws_TestCase):
    OTHER_CONTEXT = 'OtherContext'

    def setUp(self):
        self.prepare_test_environment(type(self).__name__, alternate_context_names=[self.OTHER_CONTEXT])
        # Ignore warnings based on https://github.com/boto/boto3/issues/454 for now
        # Needs to be set per tests as its reset between integration tests
        warnings.filterwarnings(action="ignore", message="unclosed", category=ResourceWarning)

    def test_security_end_to_end(self):
        self.run_all_tests()

    def __100_create_other_project(self):
        # We verify that access granted to a target project does not allow access 
        # to the "other" project.

        with self.alternate_context(self.OTHER_CONTEXT):
            if not self.has_project_stack():
                self.lmbr_aws(
                    'project', 'create',
                    '--stack-name', self.TEST_PROJECT_STACK_NAME,
                    '--confirm-aws-usage',
                    '--confirm-security-change',
                    '--no-admin-roles',  # NB: No ProjectOwner, ProjectAdmin role in account
                    '--region', lmbr_aws_test_support.REGION
                )

            self.lmbr_aws(
                'cloud-gem', 'create',
                '--gem', self.TEST_RESOURCE_GROUP_NAME,
                '--initial-content', 'api-lambda-dynamodb',
                '--enable', '--no-sln-change',
                ignore_failure=True
            )
            if not self.has_deployment_stack():
                self.lmbr_aws(
                    'deployment', 'create',
                    '--deployment', self.TEST_DEPLOYMENT_NAME,
                    '--confirm-aws-usage',
                    '--confirm-security-change'
                )

    def __120_create_project_stack(self):
        if not self.has_project_stack():
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

        # We will verify that what is granted for the target stack isn't granted for the "other" stack.
        with self.alternate_context(self.OTHER_CONTEXT):
            other_project_stack_arn = self.get_project_stack_arn()
            other_configuration_bucket_arn = self.get_stack_resource_arn(other_project_stack_arn, 'Configuration')
            other_logs_bucket_arn = self.get_stack_resource_arn(other_project_stack_arn, 'Logs')
            other_project_execute_api_arn = self.get_api_execution_arn(other_project_stack_arn, self.get_service_api_id(stack_arn=other_project_stack_arn))

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
                                                     # who knows what the owner/admin needs to do to fix a problem... don't be restrictive here
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
                                                 'Description': 'SID CreateAndManageProjectAndDeploymentRoles -- Owner/Admin denied full access for target project.',
                                                 'Resources': [
                                                     project_level_role_arn
                                                 ],
                                                 'Deny': [
                                                     # never grant all IAM operations, be selective
                                                     'iam:*'
                                                 ]
                                             },
                                             {
                                                 'Description': 'SID CreateAndManageProjectAndDeploymentRoles -- Owner/Admin allowed specific access for target project.',
                                                 'Resources': [
                                                     project_level_role_arn
                                                 ],
                                                 'Allow': [
                                                     # This is not intended to be a complete list of what is actually granted.
                                                     # Other tests that use the role verify that it has what it needs to do its job
                                                     # Here, along with the test using the "other" resources below, we are verifying
                                                     # that a permission granted in this project do not extend to other projects.
                                                     'iam:GetRole'
                                                 ]
                                             },
                                             # SID ExecuteServiceAPI
                                             {
                                                 'Description': 'SID ExecuteServiceAPI -- Owner/Admin denied permissions to execute apis for other project',
                                                 "Resources": [
                                                     other_project_execute_api_arn
                                                 ],
                                                 'Deny': [
                                                     # verify a permission they have on their project is denied on the other project
                                                     'execute-api:Invoke'
                                                 ]
                                             }
                                         ])

        # now call the helper function for each role
        verify_permissions_for_role('ProjectOwner')
        verify_permissions_for_role('ProjectAdmin')

    def __131_verify_project_has_project_roles(self):
        """Explict check for valid ARNs for ProjectOwner/ProjectAdmin roles and policies"""
        project_stack_arn = self.get_project_stack_arn()

        project_owner_role_arn = self.get_stack_resource_arn(project_stack_arn, 'ProjectOwner')
        project_owner_policy_arn = self.get_stack_resource_arn(project_stack_arn, 'ProjectOwnerAccess')
        project_admin_role_arn = self.get_stack_resource_arn(project_stack_arn, 'ProjectAdmin')
        project_admin_policy_arn = self.get_stack_resource_arn(project_stack_arn, 'ProjectAccess')

        print(aws_utils.ArnParser.is_valid_arn(project_owner_role_arn))
        self.assertTrue(aws_utils.ArnParser.is_valid_arn(project_owner_role_arn))
        self.assertTrue(aws_utils.ArnParser.is_valid_arn(project_owner_policy_arn))
        self.assertTrue(aws_utils.ArnParser.is_valid_arn(project_admin_role_arn))
        self.assertTrue(aws_utils.ArnParser.is_valid_arn(project_admin_policy_arn))

    def __ensure_resource_is_missing(self, stack_arn, resource):
        """Ensure that no valid resource ARN exists for project"""
        found = True
        try:
            resource_arn = self.get_stack_resource_arn(stack_arn, resource)
            if resource_arn is None:
                found = False
        except AssertionError:
            found = False

        self.assertTrue(not found, "Unexpectedly found resource {} in stack".format(resource))

    def __132_verify_other_project_does_not_have_project_roles(self):
        """These should all be none in the other stack as --no-admin-roles is defined"""
        with self.alternate_context(self.OTHER_CONTEXT):
            other_project_stack_arn = self.get_project_stack_arn()

            self.__ensure_resource_is_missing(other_project_stack_arn, 'ProjectOwner')
            self.__ensure_resource_is_missing(other_project_stack_arn, 'ProjectOwnerAccess')
            self.__ensure_resource_is_missing(other_project_stack_arn, 'ProjectAdmin')
            self.__ensure_resource_is_missing(other_project_stack_arn, 'ProjectAccess')

    def __133_verify_project_roles_are_assumed_via_regional_sts(self):
        project_stack_arn = self.get_project_stack_arn()

        project_owner_role_arn = self.get_stack_resource_arn(project_stack_arn, 'ProjectOwner')
        region = self.get_region_from_arn(project_stack_arn)

        # Validate assuming role through Util works as expected
        print("region: {}".format(region))
        client = aws_sts.AWSSTSUtils(region).client()
        res = client.assume_role(RoleArn=project_owner_role_arn, RoleSessionName='verify_sts_session')

        self.assertIsNotNone(res)
        aws_sts.AWSSTSUtils.validate_session_token(res['Credentials']['SessionToken'])

    def __140_create_project_user(self):
        self.create_project_user()

    def __160_update_project_stack_using_project_owner_role(self):
        settings = self.load_local_project_settings()
        project_stack_arn = settings['ProjectStackId']

        res = self.aws_cloudformation.describe_stacks(StackName=project_stack_arn)
        timestamp_before_update = res['Stacks'][0]['LastUpdatedTime']

        self.lmbr_aws(
            'project',
            'update',
            '--confirm-aws-usage',
            '--confirm-security-change',
            project_role='ProjectOwner'
        )

        self.verify_stack("project stack", project_stack_arn, mock_specification.ok_project_stack(admin_roles=True))

        res = self.aws_cloudformation.describe_stacks(StackName=project_stack_arn)
        self.assertNotEqual(timestamp_before_update, res['Stacks'][0]['LastUpdatedTime'], 'update-project-stack did not update the stack')

    def __170_update_project_stack_using_project_admin_role(self):

        settings = self.load_local_project_settings()
        project_stack_arn = settings['ProjectStackId']

        res = self.aws_cloudformation.describe_stacks(StackName=project_stack_arn)
        timestamp_before_update = res['Stacks'][0]['LastUpdatedTime']

        self.lmbr_aws(
            'project', 'update',
            '--confirm-aws-usage',
            project_role='ProjectAdmin'
        )

        self.verify_stack("project stack", project_stack_arn, mock_specification.ok_project_stack(admin_roles=True))

        res = self.aws_cloudformation.describe_stacks(StackName=project_stack_arn)
        self.assertNotEqual(timestamp_before_update, res['Stacks'][0]['LastUpdatedTime'], 'update-project-stack did not update the stack')

    def __171_update_other_project_stack_without_security_roles_succeeds(self):
        """ Ensure stacks without project admin roles can be updated """
        with self.alternate_context(self.OTHER_CONTEXT):
            other_project_stack_arn = self.get_project_stack_arn()
            res = self.aws_cloudformation.describe_stacks(StackName=other_project_stack_arn)
            timestamp_before_update = res['Stacks'][0]['LastUpdatedTime']

            # Should not require security flag as no update should be required
            self.lmbr_aws(
                'project', 'update',
                '--confirm-aws-usage'
            )

            self.verify_stack("project stack", other_project_stack_arn, mock_specification.ok_project_stack(admin_roles=False))

            res = self.aws_cloudformation.describe_stacks(StackName=other_project_stack_arn)
            self.assertNotEqual(timestamp_before_update, res['Stacks'][0]['LastUpdatedTime'], 'update-project-stack did not update the stack')

    def __validate_project_identity_roles_are_configured(self, project_stack_arn):
        identity_pool_id = json.loads(self.get_stack_resource_physical_id(project_stack_arn, "ProjectIdentityPool"))["id"]

        roles = IdentityPoolUtils.get_identity_pool_roles(identity_pool_id, self.aws_cognito_identity)
        roles.append(self.get_stack_resource_physical_id(project_stack_arn, 'CloudGemPortalUserRole'))
        roles.append(self.get_stack_resource_physical_id(project_stack_arn, 'CloudGemPortalAdministratorRole'))

        policy_documents = []
        for role in roles:
            iam_role = aws_iam_role.IAMRole.factory(role_name=role, iam_client=self.aws_iam)

            # Skip complexity of simulate policy here as its doesn't handle sign-in and federation well
            pool_ids, condition_statement, aud_key = IdentityPoolUtils.find_existing_pool_ids_references_in_assume_role_policy(iam_role.assume_role_policy)
            print(condition_statement)
            self.assertIsNotNone(condition_statement, "Did not find Cognito Federation Statement with expected Condition")
            self.assertIsNotNone(aud_key, "Expected IAM condition key for Cognito statements is missing")
            self.assertIn(identity_pool_id, pool_ids, "Did not find pool id in existing pool ids")

    def __174_validate_project_identity_roles_are_configured(self):
        project_stack_arn = self.get_project_stack_arn()
        self.__validate_project_identity_roles_are_configured(project_stack_arn)

    def __175_validate_other_project_identity_roles_are_configured(self):
        # iterate over project stacks, get identity pools
        with self.alternate_context(self.OTHER_CONTEXT):
            other_project_stack_arn = self.get_project_stack_arn()
            self.__validate_project_identity_roles_are_configured(other_project_stack_arn)

    def __185_add_resource_group(self):
        self.lmbr_aws(
            'cloud-gem', 'create',
            '--gem', self.TEST_RESOURCE_GROUP_NAME,
            '--initial-content', 'api-lambda-dynamodb',
            '--enable', '--no-sln-change',
            ignore_failure=False
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

    def verify_access_control(self, expected_mappings_for_logical_ids=None):
        # We are disabling this test as it currently fails in the call the simulate_custom_policy
        if expected_mappings_for_logical_ids is None:
            expected_mappings_for_logical_ids = []
        return

        resource_group_stack_arn = self.get_stack_resource_arn(self.get_deployment_stack_arn(self.TEST_DEPLOYMENT_NAME), self.TEST_RESOURCE_GROUP_NAME)

        self.refresh_stack_resources(resource_group_stack_arn)

        player_role_arn = self.get_stack_resource_arn(self.get_deployment_access_stack_arn(self.TEST_DEPLOYMENT_NAME), 'Player')
        resource_group_stack_arn = self.get_stack_resource_arn(self.get_deployment_stack_arn(self.TEST_DEPLOYMENT_NAME), self.TEST_RESOURCE_GROUP_NAME)
        service_api_arn = self.get_stack_resource_arn(resource_group_stack_arn, 'ServiceApi')
        service_lambda_arn = self.get_stack_resource_arn(resource_group_stack_arn, 'ServiceLambda')
        table_arn = self.get_stack_resource_arn(resource_group_stack_arn, 'Table')

        # player should be able to invoke the base api and not able to call the authenticated api, call the lambda or get an item from the table
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
                                                 service_api_arn + '/api/GET/example/data/authenticated'
                                             ],
                                             'Deny': [
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

        # An authenticated player should be able to invoke the base api and the authenticated api, but not call the lambda or get an item from the table
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
                                                 service_api_arn + '/api/GET/example/data/authenticated'
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
        res = self.aws_lambda.get_function(FunctionName=service_lambda_arn)
        service_lambda_role_name = self.get_role_name_from_arn(res['Configuration']['Role'])
        deployment_access_arn = self.get_deployment_access_stack_arn(self.TEST_DEPLOYMENT_NAME)
        self.verify_role_permissions('deployment',
                                     deployment_access_arn,
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
                                     deployment_access_arn,
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
                                         },
                                         {
                                             'Resources': [
                                                 deployment_access_arn
                                             ],
                                             'Allow': [
                                                 'cloudformation:DescribeStacks'
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

        expected_logical_ids = [self.TEST_RESOURCE_GROUP_NAME + '.ServiceApi']
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
        self.assertIn('_COMPLETE', self.lmbr_aws_stdout)

    def __300_list_resource_groups_for_deployment_using_project_admin_role(self):
        self.lmbr_aws(
            'resource-group', 'list',
            '--deployment', self.TEST_DEPLOYMENT_NAME,
            deployment_role='DeploymentAdmin'
        )
        self.assertIn(self.TEST_RESOURCE_GROUP_NAME, self.lmbr_aws_stdout)
        self.assertIn('_COMPLETE', self.lmbr_aws_stdout)

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
                                      'ResourceType': 'Custom::DynamoDBTable'
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

        self.verify_access_control(expected_mappings_for_logical_ids=['TestFunction'])

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
                                      'ResourceType': 'Custom::DynamoDBTable'
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

    def __420_verify_cloud_gem_portal_user_access(self):
        project_stack_arn = self.get_project_stack_arn()
        resource_group_stack_arn = self.get_stack_resource_arn(self.get_deployment_stack_arn(self.TEST_DEPLOYMENT_NAME), self.TEST_RESOURCE_GROUP_NAME)
        service_api_arn = self.get_stack_resource_arn(resource_group_stack_arn, 'ServiceApi')

        # Verify that user can access api gateway for deployed gem
        self.verify_role_permissions('project',
                                     project_stack_arn,
                                     self.get_stack_resource_physical_id(project_stack_arn, 'CloudGemPortalUserRole'),
                                     [
                                         {
                                             'Resources': [
                                                 service_api_arn + '/api/GET/example/data'
                                             ],
                                             'Allow': [
                                                 'execute-api:Invoke'
                                             ]
                                         }
                                     ])

        with self.alternate_context(self.OTHER_CONTEXT):
            other_project_stack_arn = self.get_project_stack_arn()
            other_project_execute_api_arn = self.get_api_execution_arn(other_project_stack_arn, self.get_service_api_id(stack_arn=other_project_stack_arn))

        # Verify that user cannot access service api in a different stack
        self.verify_role_permissions('project',
                                     project_stack_arn,
                                     self.get_stack_resource_physical_id(project_stack_arn, 'CloudGemPortalUserRole'),
                                     [
                                         {
                                             'Resources': [
                                                 other_project_execute_api_arn + '/api/GET/example/data'
                                             ],
                                             'Deny': [
                                                 'execute-api:Invoke'
                                             ]
                                         }
                                     ])

    def __check_roles_names_have_expected_permissions(self, items_to_verify, project_stack_arn, permission='Allow', context_entries=None):
        """
        Verify that for a list of role logical ids in the project_stack that actions and resources are allowed/denied
        :param items_to_verify: List of items, where each item is {'role':, 'actions':, 'resources':}
        :param project_stack_arn: The stack to pull the the role from
        :param permission: Must be 'Allow' or 'Deny'
        """
        for item in items_to_verify:
            role = item['role_name'] if 'role_name' in item else self.get_stack_resource_physical_id(project_stack_arn, item['role'])
            self.verify_role_permissions('project',
                                         project_stack_arn,
                                         role,
                                         [
                                             {
                                                 'Resources': item['resources'],
                                                 permission: item['actions']
                                             }
                                         ],
                                         context_entries)

    def __check_roles_have_expected_permissions(self, items_to_verify, project_stack_arn, permission='Allow'):
        """
        Verify that for a list of role logical ids in the project_stack that actions and resources are allowed/denied
        :param items_to_verify: List of items, where each item is {'role':, 'actions':, 'resources':}
        :param project_stack_arn: The stack to pull the the role from
        :param permission: Must be 'Allow' or 'Deny'
        """
        for item in items_to_verify:
            self.verify_role_permissions('project',
                                         project_stack_arn,
                                         self.get_stack_resource_physical_id(project_stack_arn, item['role']),
                                         [
                                             {
                                                 'Resources': item['resources'],
                                                 permission: item['actions']
                                             }
                                         ])

    def __check_policies_have_expected_permissions(self, items_to_verify, project_stack_arn, permission='Allow'):
        """
        Verify a list of policies, actions and resources are allowed/denied
        :param items_to_verify: List of items, where each item is {'role':, 'actions':, 'resources':}
        :param project_stack_arn: The stack to pull the the policy from
        :param permission: Must be 'Allow' or 'Deny'
        """
        for item in items_to_verify:
            policy_documents = []

            # Extract policy document
            policy_arn = self.get_stack_resource_physical_id(project_stack_arn, item['policy'])
            default_version = self.aws_iam.get_policy(PolicyArn=policy_arn)['Policy']['DefaultVersionId']

            get_policy_version_res = self.aws_iam.get_policy_version(PolicyArn=policy_arn, VersionId=default_version)
            # print '*** get_policy_version_res', policy_arn, policy_version['VersionId'], get_policy_version_res
            policy_documents.append(json.dumps(get_policy_version_res['PolicyVersion']['Document'], indent=4))
            self.verify_permissions('project', policy_documents,
                                    [
                                        {
                                            'Resources': item['resources'],
                                            permission: item['actions']
                                        }
                                    ])

    def __440_verify_roles_and_policies_can_only_describe_own_project_stacks(self):
        project_stack_arn = self.get_project_stack_arn()
        deployment_stack_arn = self.get_deployment_stack_arn(self.TEST_DEPLOYMENT_NAME)

        # Roles that are expected to list stacks in project stack
        roles_to_verify = [
            {'role': 'CloudGemPortalUserRole', 'actions': ['cloudformation:DescribeStacks'],
             'resources': [deployment_stack_arn]},
            {'role': 'ProjectResourceHandlerExecution', 'actions': ['cloudformation:DescribeStacks'],
             'resources': [project_stack_arn, deployment_stack_arn]}
        ]

        print("Checking project roles can describe stacks")
        self.__check_roles_have_expected_permissions(roles_to_verify, project_stack_arn, permission='Allow')

        # Direct managed policies to check
        policies_to_verify = [
            {'policy': 'ServiceLambdaExecution', 'actions': ['cloudformation:DescribeStacks'],
             'resources': [project_stack_arn, deployment_stack_arn]},
            {'policy': 'ProjectOwnerAccess', 'actions': ['cloudformation:DescribeStacks'],
             'resources': [deployment_stack_arn]}
        ]

        print("Checking managed policies can describe stacks")
        self.__check_policies_have_expected_permissions(policies_to_verify, project_stack_arn, permission='Allow')

        # Nothing should have access to the other deployment stack
        with self.alternate_context(self.OTHER_CONTEXT):
            other_project_stack_arn = self.get_project_stack_arn()

            other_roles_to_verify = [
                {'role': 'CloudGemPortalUserRole', 'actions': ['cloudformation:DescribeStacks'],
                 'resources': [other_project_stack_arn]},
                {'role': 'ProjectResourceHandlerExecution', 'actions': ['cloudformation:DescribeStacks'],
                 'resources': [other_project_stack_arn]}
            ]

            print("Checking project roles cannot describe stacks in other projects")
            self.__check_roles_have_expected_permissions(other_roles_to_verify, project_stack_arn, permission='Deny')

            other_policies_to_verify = [
                {'policy': 'ServiceLambdaExecution', 'actions': ['cloudformation:DescribeStacks'],
                 'resources': [other_project_stack_arn]},
                {'policy': 'ProjectOwnerAccess', 'actions': ['cloudformation:DescribeStacks'],
                 'resources': [other_project_stack_arn]}
            ]

            print("Checking managed policies cannot describe stacks in other projects")
            self.__check_policies_have_expected_permissions(other_policies_to_verify, project_stack_arn, permission='Deny')

    def __450_ensure_lambda_log_events_are_for_project_only(self):
        project_stack_arn = self.get_project_stack_arn()
        project_resource_handler_arn = self.get_stack_resource_arn(project_stack_arn, 'ProjectResourceHandler')
        project_resource_handler_name = aws_utils.ArnParser(project_resource_handler_arn).resource_id
        region = self.get_region_from_arn(project_stack_arn)
        account_id = self.get_account_id_from_arn(project_stack_arn)

        # Check roles defined in the project stack
        log_events_arn = 'arn:aws:logs:{}:{}:log-group:/aws/lambda/{}:log-stream:*'.format(region, account_id, project_resource_handler_name)
        log_group_arn = 'arn:aws:logs:{}:{}:log-group:/aws/lambda/{}'.format(region, account_id, project_resource_handler_name)

        roles_to_verify = [
            {'role': 'ProjectResourceHandlerExecution', 'actions': ['logs:PutLogEvents'], 'resources': [log_events_arn, log_group_arn]},
            {'role': 'ProjectResourceHandlerExecution', 'actions': ['logs:CreateLogStream'], 'resources': [log_group_arn]}
        ]

        print("Checking project roles can log events")
        self.__check_roles_have_expected_permissions(roles_to_verify, project_stack_arn, permission='Allow')

        lambda_roles_to_verify = []

        # Find all deployed lambdas from the project stack
        lambdas = self.get_all_deployed_lambdas_and_roles(project_stack_arn)
        for lambda_arn in lambdas:
            if lambdas[lambda_arn] is None:
                print("[WARNING] Missing lambada execution role for {}. Skipping in test".format(lambda_arn))
                continue

            lambda_name = aws_utils.ArnParser(lambda_arn).resource_id
            role_arn = lambdas[lambda_arn]
            role_name = self.__get_lambda_role_name_without_path_from_arn(role_arn)

            log_events_arn = \
                aws_utils.ArnParser.format_arn_with_resource_type(service="logs", region=region, account_id=account_id, resource_type="log-group",
                                                                  resource_id="/aws/lambda/{}".format(lambda_name), separator=":")

            log_stream_arn = \
                aws_utils.ArnParser.format_arn_with_resource_type(service="logs", region=region, account_id=account_id, resource_type="log-group",
                                                                  resource_id="/aws/lambda/{}:log-stream:*".format(lambda_name), separator=":")

            lambda_roles_to_verify.append({'role_name': role_name, 'actions': ['logs:PutLogEvents'], 'resources': [log_events_arn, log_stream_arn]})
            lambda_roles_to_verify.append({'role_name': role_name, 'actions': ['logs:CreateLogStream'], 'resources': [log_events_arn]})

        print("Checking deployed project lambdas can log events")
        self.__check_roles_names_have_expected_permissions(lambda_roles_to_verify, project_stack_arn, permission='Allow')

    def __500_ensure_cognito_pool_roles_are_separated(self):
        """Roles should not support access to user and identity pools in other project"""

        # Note: Project defines two roles but neither of them are testable via code
        # * ProjectIdentityPoolUnauthenticatedRole - which has no permissions for anything
        # * ProjectIdentityPoolAuthenticatedRole - which only sets up a trust relationship but no permissions
        project_stack_arn = self.get_project_stack_arn()
        project_name = project_stack_arn.split('/')[1]
        region = self.get_region_from_arn(project_stack_arn)
        account_id = self.get_account_id_from_arn(project_stack_arn)

        identity_pool_id = json.loads(self.get_stack_resource_physical_id(self.get_project_stack_arn(), "ProjectIdentityPool"))["id"]
        identity_pool_arn = aws_utils.ArnParser.format_arn_with_resource_type(service="cognito-identity", region=region, account_id=account_id,
                                                                              resource_type="identitypool", resource_id=identity_pool_id)

        user_pool_id = json.loads(self.get_stack_resource_physical_id(self.get_project_stack_arn(), "ProjectUserPool"))["id"]
        user_pool_arn = aws_utils.ArnParser.format_arn_with_resource_type(service="cognito-idp", region=region, account_id=account_id, resource_type="userpool",
                                                                          resource_id=user_pool_id)

        # 2. Find pool and identity lambdas
        crh_lambdas = self.get_all_deployed_crh_lambdas_for_stack(stack_arn=project_stack_arn)

        # Scan for CRH execution rules for Cognito lambdas
        user_pool_lambda_role_name = None
        identity_pool_lambda_role_name = None

        for crh in crh_lambdas:
            if 'Custom_CognitoUserPool' in crh:
                user_pool_lambda_role = crh_lambdas[crh]
                user_pool_lambda_role_name = self.__get_lambda_role_name_without_path_from_arn(user_pool_lambda_role)
            elif 'Custom_CognitoIdentityPool' in crh:
                identity_pool_lambda_role = crh_lambdas[crh]
                identity_pool_lambda_role_name = self.__get_lambda_role_name_without_path_from_arn(identity_pool_lambda_role)

        # Roles that are expected to list stacks in project stack
        lambda_roles_to_verify = [
            {'role_name': user_pool_lambda_role_name, 'actions': ['cognito-idp:DescribeUserPool'],
             'resources': [user_pool_arn]},
            {'role_name': identity_pool_lambda_role_name, 'actions': ['cognito-identity:DescribeIdentityPool'],
             'resources': [identity_pool_arn]}
        ]

        print("Checking Cognito custom lambdas have permission on identity and user pools in project")
        context = self.__convert_expected_tags_into_context_entries({'aws:ResourceTag/cloudcanvas:project-name': project_name})
        self.__check_roles_names_have_expected_permissions(lambda_roles_to_verify, project_stack_arn, permission='Allow', context_entries=context)

        with self.alternate_context(self.OTHER_CONTEXT):
            other_project_stack_arn = self.get_project_stack_arn()
            other_project_name = other_project_stack_arn.split('/')[1]
            other_identity_pool_id = json.loads(self.get_stack_resource_physical_id(other_project_stack_arn, "ProjectIdentityPool"))["id"]
            other_identity_pool_arn = aws_utils.ArnParser.format_arn_with_resource_type(service="cognito-identity", region=region, account_id=account_id,
                                                                                        resource_type="identitypool", resource_id=other_identity_pool_id)

            other_user_pool_id = json.loads(self.get_stack_resource_physical_id(other_project_stack_arn, "ProjectUserPool"))["id"]
            other_user_pool_arn = aws_utils.ArnParser.format_arn_with_resource_type(service="cognito-idp", region=region, account_id=account_id,
                                                                                    resource_type="userpool",
                                                                                    resource_id=other_user_pool_id)

            other_policies_to_verify = [
                {'role_name': user_pool_lambda_role_name, 'actions': ['cognito-idp:DescribeUserPool'],
                 'resources': [other_user_pool_arn]},
                {'role_name': identity_pool_lambda_role_name, 'actions': ['cognito-identity:DescribeIdentityPool'],
                 'resources': [other_identity_pool_arn]}
            ]

            print("Checking Cognito custom lambdas are denied permission on identity and user pools in other projects")
            context = self.__convert_expected_tags_into_context_entries({'aws:ResourceTag/cloudcanvas:project-name': other_project_name})
            self.__check_roles_names_have_expected_permissions(other_policies_to_verify, project_stack_arn, permission='Deny',
                                                               context_entries=context)

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
                "ServiceToken": {"Ref": "ProjectResourceHandler"},
                "ConfigurationBucket": {"Ref": "ConfigurationBucket"},
                "ConfigurationKey": {"Ref": "ConfigurationKey"},
                "FunctionName": function_name,
                "Runtime": "python3.7"
            }
        }

        resource_group_resources[function_name] = {
            "Type": "AWS::Lambda::Function",
            "Properties": {
                "Description": "Example of a function called by the game to write data into a DynamoDB table.",
                "Handler": "main.test",
                "Role": {"Fn::GetAtt": [function_name + "Configuration", "Role"]},
                "Runtime": {"Fn::GetAtt": [function_name + "Configuration", "Runtime"]},
                "Code": {
                    "S3Bucket": {"Fn::GetAtt": [function_name + "Configuration", "ConfigurationBucket"]},
                    "S3Key": {"Fn::GetAtt": [function_name + "Configuration", "ConfigurationKey"]}
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

    def get_service_api_id(self, stack_arn):
        """Extracts the Service api Id for a given stack"""
        resource_description = self.get_stack_resource(stack_arn, 'ServiceApi')
        physical_resource_id = resource_description['PhysicalResourceId']

        # physical resource id is a mini json document, so need to pull out required data from id field
        id_data = aws_utils.get_data_from_custom_physical_resource_id(physical_resource_id)
        rest_api_id = id_data.get('RestApiId', '')
        return rest_api_id

    def get_lambda_function_execution_role_arn_from_arn(self, function_arn):
        res = self.aws_lambda.get_function(FunctionName=function_arn)
        role_arn = res['Configuration']['Role']
        return role_arn

    def get_lambda_function_execution_role_arn(self, stack_arn, function_name):
        function_arn = self.get_stack_resource_arn(stack_arn, function_name)
        return self.get_lambda_function_execution_role_arn_from_arn(function_arn)

    def get_api_execution_arn(self, stack_arn, api_id, path='api'):
        """Return a API execution arn for a given api gateway rest api id"""
        region = self.get_region_from_arn(stack_arn)
        account_id = self.get_account_id_from_arn(stack_arn)
        return "arn:aws:execute-api:{}:{}:{}:{}/*".format(region, account_id, api_id, path)

    def get_all_deployed_crh_lambdas_for_stack(self, stack_arn):
        """
        Finds all the deployed lambdas in a stack that are Custom Resource Handlers (-CRH-).

        :param arn stack_arn:   The arn of the stack to query
        :return a map of lambda arns to their execution roles {FunctionArn:, FunctionArn}
        """
        lambdas = {}

        # find all lambdas in region and filter to those matching stack
        # and are custom resource handlers
        # Note: Lambdas list function provides very little filtering, must post filter
        stack_name = self.get_stack_name_from_arn(stack_arn)

        for response in self.aws_lambda.get_paginator('list_functions').paginate():
            for entry in response['Functions']:
                function_name = entry['FunctionName']
                if function_name.startswith(stack_name):
                    if function_name.find('-CRH-') > -1:  # Contains on unicode value
                        lambdas[entry['FunctionArn']] = entry['Role']
        return lambdas

    def get_all_deployed_lambdas_and_roles(self, stack_arn):
        """
        Finds all the deployed lambdas in a stack. Deployed being either deployed directly by the project stack or
        through  a Custom resource (-CRH-).

        :param arn stack_arn:   The arn of the stack to query
        :return a map of lambda arns to their execution roles
        """
        lambdas = {}
        res = self.aws_cloudformation.describe_stack_resources(StackName=stack_arn)

        # find all lambdas directly deployed by stack
        for resource in res['StackResources']:
            if resource['ResourceType'] == 'AWS::Lambda::Function':
                function_arn = self.get_stack_resource_arn(stack_arn, resource['LogicalResourceId'])
                # Need to grab execution role elsewhere
                lambdas[function_arn] = self.get_lambda_function_execution_role_arn_from_arn(function_arn)

        # Now find all the CRH handlers in the stack
        crh_lambdas = self.get_all_deployed_crh_lambdas_for_stack(stack_arn=stack_arn)

        # Merge all lambda information together
        lambdas.update(crh_lambdas)
        return lambdas

    @staticmethod
    def __get_lambda_role_name_without_path_from_arn(role_arn):
        """
        Gets the lambda role name, discarding any to pathing information in ARN
        # ie cctestPV2ZAJA/NONE/NONE/ServiceLambda/AccessControl/cctestPV2ZAJA-ServiceLambda will return just the
        cctestPV2ZAJA-ServiceLambda
        """
        role_name = aws_utils.ArnParser(role_arn).resource_id.rsplit('/', 1)[1]
        return role_name

    @staticmethod
    def __convert_expected_tags_into_context_entries(tags):
        """
        Converts a list of Tag key value pairs into a ContextEntry list suitable for use with simulate_custom_policy
        https://boto3.amazonaws.com/v1/documentation/api/latest/reference/services/iam.html#IAM.Client.simulate_custom_policy
        """
        context_entries = []

        for tag in tags:
            context_entries.append({
                'ContextKeyName': tag,
                'ContextKeyValues': [tags[tag]],
                'ContextKeyType': 'string'
            })
        return context_entries
