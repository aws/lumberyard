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

import json
import os
import time
import warnings
import random
from typing import Tuple

from botocore.exceptions import ClientError

import resource_manager_common.constant as constant
from resource_manager.test import lmbr_aws_test_support
from . import base_stack_test
from . import test_constant


class IntegrationTest_CloudGemFramework_CloudGemPortal(base_stack_test.BaseStackTestCase):
    """
    Note: This tests now confirm the absence of CloudGemPortal resources as CGP
    has been deprecated.
    """
    CGP_CONTENT_FILE_NAME = "Foo.js"

    def __init__(self, *args, **kwargs):
        super(IntegrationTest_CloudGemFramework_CloudGemPortal, self).__init__(*args, **kwargs)
        self.base = super(IntegrationTest_CloudGemFramework_CloudGemPortal, self)
        self.base.__init__(*args, **kwargs)

    def setUp(self):
        # Ignore warnings based on https://github.com/boto/boto3/issues/454 for now
        # Needs to be set per tests as its reset between integration tests
        warnings.filterwarnings(action="ignore", message="unclosed", category=ResourceWarning)

        self.set_deployment_name(lmbr_aws_test_support.unique_name())
        self.set_resource_group_name(lmbr_aws_test_support.unique_name('rg'))
        self.prepare_test_environment("cloud_gem_framework_test")
        self.register_for_shared_resources()

    def test_end_to_end(self):
        self.run_all_tests()

    def __000_create_stacks(self):
        self.lmbr_aws('cloud-gem', 'create', '--gem', self.TEST_RESOURCE_GROUP_NAME, '--initial-content', 'no-resources', '--enable', '--no-cpp-code',
                      ignore_failure=True)
        self.enable_shared_gem(self.TEST_RESOURCE_GROUP_NAME, 'v1', path=os.path.join(self.context[test_constant.ATTR_ROOT_DIR],
                                                                                      os.path.join(test_constant.DIR_GEMS,  self.TEST_RESOURCE_GROUP_NAME)))
        self.base_create_project_stack()
        self.base_create_deployment_stack()

    def __100_verify_cgp_content_not_deployed(self):
        cgp_bucket_name = self.get_stack_resource_physical_id(self.get_project_stack_arn(), 'CloudGemPortal')
        self.verify_s3_object_missing(cgp_bucket_name, 'www/index.html')
        self.verify_s3_object_missing(cgp_bucket_name, 'www/bundles/app.bundle.js')
        self.verify_s3_object_missing(cgp_bucket_name, 'www/bundles/dependencies.bundle.js')

    def __120_create_cgp_resources(self):
        cgp_file_path = self.__get_cgp_content_file_path()
        with open(cgp_file_path, 'w') as f:
            f.write('test file content')

    def __200_create_resources(self):
        self.lmbr_aws('resource-group', 'upload-resources', '--resource-group', self.TEST_RESOURCE_GROUP_NAME, '--deployment', self.TEST_DEPLOYMENT_NAME,
                      '--confirm-aws-usage', '--confirm-security-change', '--only-cloud-gems', self.TEST_RESOURCE_GROUP_NAME)

    def __219_verify_cgp_admin_account_not_created(self):
        self.lmbr_aws('cloud-gem-framework', 'create-admin', ignore_failure=True)

        user_pool_id = json.loads(self.get_stack_resource_physical_id(self.get_project_stack_arn(), "ProjectUserPool"))["id"]
        user_not_created = False
        try:
            self.aws_cognito_idp.admin_get_user(
                UserPoolId=user_pool_id,
                Username='administrator'
            )
        except ClientError as ex:
            if ex.response['Error']['Code'] == 'UserNotFoundException':
                user_not_created = True

        self.assertTrue(user_not_created, 'The administrator account exists.')

    def __220_verify_cgp_resources(self):
        (bucket, key) = self.__get_cgp_content_location()
        self.verify_s3_object_missing(bucket, key)

    def __230_verify_cgp_default_url_not_available(self):
        self.lmbr_aws('cloud-gem-framework', 'cloud-gem-portal', '--show-url-only', expect_failure=True)

    def __600_verify_cgp_user_pool(self):
        self.__verify_only_administrator_can_signup_user_for_cgp()
        self.__verify_cgp_user_pool_groups()
        self.__verify_cgp_user_pool_permissions()

    def __860_verify_cgp_user_pool_after_project_update(self):
        self.base_update_project_stack()
        self.lmbr_aws('cloud-gem-framework', 'cloud-gem-portal', '--show-url-only', expect_failure=True)
        time.sleep(20)  # Give the cloud gem framework a few seconds to populate cgp_bootstrap
        self.__verify_only_administrator_can_signup_user_for_cgp()
        self.__verify_cgp_user_pool_groups()

    def __910_remove_cgp_resources(self):
        cgp_file_path = self.__get_cgp_content_file_path()
        os.remove(cgp_file_path)

    def __999_cleanup(self):
        self.teardown_base_stack()

    def __verify_only_administrator_can_signup_user_for_cgp(self):
        time.sleep(20)  # allow time for IAM to reach a consistent state

        client_id = None
        user_pool_id = json.loads(self.get_stack_resource_physical_id(self.get_project_stack_arn(), "ProjectUserPool"))["id"]
        client_apps = self.aws_cognito_idp.list_user_pool_clients(UserPoolId=user_pool_id, MaxResults=60)
        for client_app in client_apps.get('UserPoolClients', []):
            if client_app['ClientName'] == 'CloudGemPortalApp':
                client_id = client_app['ClientId']

        user_name = "test_user1"
        signup_succeeded = True
        try:
            self.aws_cognito_idp.sign_up(
                ClientId=client_id,
                Username=user_name,
                Password='T3mp1@3456'
            )
        except ClientError as e:
            if e.response['Error']['Code'] == 'NotAuthorizedException':
                signup_succeeded = False

        self.assertFalse(signup_succeeded, 'A regular user was able to sign up to the Cognito User Pool.  This should be restricted to administrators only.')

    def __verify_cgp_user_pool_groups(self):
        user_pool_id = json.loads(self.get_stack_resource_physical_id(self.get_project_stack_arn(), "ProjectUserPool"))["id"]
        user_name = f'test_user_{random.randrange(100)}'

        response = self.aws_cognito_idp.admin_create_user(
            UserPoolId=user_pool_id,
            Username=user_name,
            TemporaryPassword="T3mp1@34"
        )
        self.assertTrue(response['User']['UserStatus'] == 'FORCE_CHANGE_PASSWORD', 'The user has been added but is not in the correct user state.')

        response = self.aws_cognito_idp.admin_add_user_to_group(
            UserPoolId=user_pool_id,
            Username=user_name,
            GroupName='user'
        )
        self.assertTrue(response['ResponseMetadata']['HTTPStatusCode'] == 200, 'Unable to add a user to the user group.')

        response = self.aws_cognito_idp.admin_add_user_to_group(
            UserPoolId=user_pool_id,
            Username=user_name,
            GroupName='administrator'
        )
        self.assertTrue(response['ResponseMetadata']['HTTPStatusCode'] == 200, 'Unable to add a user to the administrator group.')

        response = self.aws_cognito_idp.admin_delete_user(
            UserPoolId=user_pool_id,
            Username=user_name
        )
        self.assertTrue(response['ResponseMetadata']['HTTPStatusCode'] == 200, 'Unable to delete the user.')

    def __verify_cgp_user_pool_permissions(self):
        user_pool_id = json.loads(self.get_stack_resource_physical_id(self.get_project_stack_arn(), "ProjectUserPool"))["id"]
        user_name = f'test_user_{random.randrange(100)}'

        response = self.aws_cognito_idp.admin_create_user(
            UserPoolId=user_pool_id,
            Username=user_name,
            TemporaryPassword="T3mp1@34"
        )
        self.assertTrue(response['User']['UserStatus'] == 'FORCE_CHANGE_PASSWORD', 'The user has been added but is not in the correct user state.')

        response = self.aws_cognito_idp.admin_add_user_to_group(
            UserPoolId=user_pool_id,
            Username=user_name,
            GroupName='user'
        )
        self.assertTrue(response['ResponseMetadata']['HTTPStatusCode'] == 200, 'Unable to add a user to the user group.')

        response = self.aws_cognito_idp.admin_get_user(
            UserPoolId=user_pool_id,
            Username=user_name
        )
        self.assertTrue(response['ResponseMetadata']['HTTPStatusCode'] == 200, 'Unable to get user.')
        self.assertTrue(response['Username'] == user_name, 'Unable to get user.')

        response = self.aws_cognito_idp.admin_list_groups_for_user(
            UserPoolId=user_pool_id,
            Username=user_name
        )
        self.assertTrue(response['ResponseMetadata']['HTTPStatusCode'] == 200, 'Unable to list the users groups.')
        self.assertTrue(len(response['Groups']) > 0, 'Unable to list the users groups.')

        response = self.aws_cognito_idp.list_users(
            UserPoolId=user_pool_id
        )
        self.assertTrue(response['ResponseMetadata']['HTTPStatusCode'] == 200, 'Unable to list the users.')
        self.assertTrue(len(response['Users']) > 0, 'Unable to list the users.')

        response = self.aws_cognito_idp.list_users_in_group(
            UserPoolId=user_pool_id,
            GroupName='administrator'
        )
        self.assertTrue(response['ResponseMetadata']['HTTPStatusCode'] == 200, 'Unable to list the users in the group administrator.')
        self.assertTrue(len(response['Users']) == 0, 'Able to list the users in the group administrator.')

        response = self.aws_cognito_idp.admin_delete_user(
            UserPoolId=user_pool_id,
            Username=user_name
        )
        self.assertTrue(response['ResponseMetadata']['HTTPStatusCode'] == 200, 'Unable to delete the user.')

    def __get_cgp_content_file_path(self) -> str:
        cgp_dir_path = self.get_gem_aws_path(self.TEST_RESOURCE_GROUP_NAME, constant.GEM_CGP_DIRECTORY_NAME)
        if not os.path.exists(cgp_dir_path):
            os.mkdir(cgp_dir_path)

        cgp_dir_path = os.path.join(cgp_dir_path, 'dist')
        if not os.path.exists(cgp_dir_path):
            os.mkdir(cgp_dir_path)

        cgp_file_path = os.path.join(cgp_dir_path, self.CGP_CONTENT_FILE_NAME)

        return cgp_file_path

    def __get_cgp_content_location(self) -> Tuple[str, str]:
        configuration_bucket = self.get_stack_resource_physical_id(self.get_project_stack_arn(), 'Configuration')
        object_key = f'{constant.GEM_CGP_DIRECTORY_NAME}/deployment/{self.TEST_DEPLOYMENT_NAME}/resource-group/' \
                     f'{self.TEST_RESOURCE_GROUP_NAME}/dist/{self.CGP_CONTENT_FILE_NAME}'
        return configuration_bucket, object_key
