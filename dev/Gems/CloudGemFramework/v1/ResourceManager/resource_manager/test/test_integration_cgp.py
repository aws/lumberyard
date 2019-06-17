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
import resource_manager_common.constant as constant
import requests    
import boto3
from botocore.exceptions import ClientError
from resource_manager.test import base_stack_test
from resource_manager.test import lmbr_aws_test_support
import test_constant

# TODO: split the service api tests out into a seperate class
class IntegrationTest_CloudGemFramework_CloudGemPortal(base_stack_test.BaseStackTestCase):

    CGP_CONTENT_FILE_NAME = "Foo.js"

    def __init__(self, *args, **kwargs):
        self.base = super(IntegrationTest_CloudGemFramework_CloudGemPortal, self)
        self.base.__init__(*args, **kwargs)


    def setUp(self):        
        self.prepare_test_environment("cloud_gem_framework_test")
        self.register_for_shared_resources()                

    def test_end_to_end(self):
        self.run_all_tests()

    def __000_create_stacks(self):       
        self.lmbr_aws('cloud-gem', 'create', '--gem', self.TEST_RESOURCE_GROUP_NAME, '--initial-content', 'no-resources', '--enable', '--no-cpp-code', '--no-sln-change', ignore_failure=True)
        self.enable_shared_gem(self.TEST_RESOURCE_GROUP_NAME, 'v1', path=os.path.join(self.context[test_constant.ATTR_ROOT_DIR], os.path.join(test_constant.DIR_GEMS,  self.TEST_RESOURCE_GROUP_NAME)))
        self.setup_base_stack()        

    def __100_verify_cgp_content(self):
        cgp_bucket_name = self.get_stack_resource_physical_id(self.get_project_stack_arn(), 'CloudGemPortal')
        self.verify_s3_object_exists(cgp_bucket_name, 'www/index.html')        
        self.verify_s3_object_exists(cgp_bucket_name, 'www/bundles/app.bundle.js')
        self.verify_s3_object_exists(cgp_bucket_name, 'www/bundles/dependencies.bundle.js')

    def __120_add_cgp_resources(self):
        cgp_file_path = self.__get_cgp_content_file_path()
        with open(cgp_file_path, 'w') as f:
            f.write('test file content')

    def __200_create_resources(self):
        self.lmbr_aws('resource-group', 'upload-resources', '--resource-group', self.TEST_RESOURCE_GROUP_NAME, '--deployment', self.TEST_DEPLOYMENT_NAME, '--confirm-aws-usage', '--confirm-security-change', '--only-cloud-gems', self.TEST_RESOURCE_GROUP_NAME)

    def __219_verify_cgp_admin_account(self):
        self.lmbr_aws('cloud-gem-framework', 'create-admin', '-d', self.TEST_DEPLOYMENT_NAME, ignore_failure=True )

        userpoolid = json.loads(self.get_stack_resource_physical_id(self.get_project_stack_arn(), "ProjectUserPool"))["id"]
        response = self.aws_cognito_idp.admin_get_user(
            UserPoolId=userpoolid,
            Username='administrator'
        )
        self.assertTrue(response, 'The administrator account already exists.')        

    def __220_verify_cgp_resources(self):
        (bucket, key) = self.__get_cgp_content_location()
        self.verify_s3_object_exists(bucket, key)

    def __230_verify_cgp_default_url(self):
        self.lmbr_aws('cloud-gem-framework', 'cloud-gem-portal', '--show-url-only')

        url = self.lmbr_aws_stdout
        self.assertTrue(url, "The Cloud Gem Portal URL was not generated.")

        host, querystring = self.__parse_url_into_parts(url)

        self.assertIsNone(querystring, "The URL querystring was not empty.")

        self.lmbr_aws('cloud-gem-framework', 'cloud-gem-portal', '--show-bootstrap-configuration')
        s_object = str(self.lmbr_aws_stdout)
        
        bootstrap_config = json.loads(s_object)

        self.__validate_bootstrap_config(bootstrap_config)

        self.lmbr_aws('cloud-gem-framework', 'cloud-gem-portal', '--show-url-only')

        url2 = self.lmbr_aws_stdout
        self.assertTrue(url2, "The Cloud Gem Portal URL was not generated.")

        request = requests.get(str(url2).strip())
        self.assertEqual(request.status_code, 200, "The pre-signed url response code was not 200 but instead it was " + str(request.status_code))

    def __600_verify_cgp_user_pool(self):
        self.__verify_only_administrator_can_signup_user_for_cgp()
        self.__verify_cgp_user_pool_groups()
        self.__verify_cgp_user_pool_permissions()

    def __860_verify_cgp_user_pool_after_project_update(self):
        self.base_update_project_stack()
        self.lmbr_aws('cloud-gem-framework', 'cloud-gem-portal', '--show-url-only')
        time.sleep(20) # Give the cloud gem framework a few seconds to populate cgp_bootstrap 
        self.__verify_only_administrator_can_signup_user_for_cgp()
        self.__verify_cgp_user_pool_groups()

    def __910_remove_cgp_resources(self):
        cgp_file_path = self.__get_cgp_content_file_path()
        os.remove(cgp_file_path)

    def __999_cleanup(self):
        self.teardown_base_stack()        

    def __parse_url_into_parts(self, url):
        if "?" not in url:
            return url, None

        parts = url.split('?')

        self.assertEqual(len(parts), 2, 'The Cloud Gem Portal URL is malformed.')

        return parts[0], parts[1]

    def __verify_only_administrator_can_signup_user_for_cgp(self):
        time.sleep(20) # allow time for IAM to reach a consistant state
        
        userpoolid = json.loads(self.get_stack_resource_physical_id(self.get_project_stack_arn(), "ProjectUserPool"))["id"]
        client_apps = self.aws_cognito_idp.list_user_pool_clients(UserPoolId=userpoolid, MaxResults=60)
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
        user_name = "test_user2"

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
        user_name = "test_user3"

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
        self.assertTrue(len(response['Users']) > 0, 'Unable to list the users in the group adminitrator.')

        response = self.aws_cognito_idp.admin_delete_user(
            UserPoolId=user_pool_id,
            Username=user_name
        )
        self.assertTrue(response['ResponseMetadata']['HTTPStatusCode'] == 200, 'Unable to delete the user.')

    def __validate_bootstrap_config(self, config):

        self.assertTrue(config, "The Cloud Gem Portal configuration was not found.")

        client_id = config["clientId"]
        user_pool_id = config["userPoolId"]
        identity_pool_id = config["identityPoolId"]
        project_config_bucket = config["projectConfigBucketId"]
        region = config["region"]

        self.assertTrue(client_id, "The Cognito User Pool Client Id is missing in the AES payload.")
        self.assertTrue(user_pool_id, "The Cognito User Pool Id is missing in the AES payload.")
        self.assertTrue(identity_pool_id, "The Identity Pool Id is missing in the AES payload.")
        self.assertTrue(project_config_bucket, "The project configuration bucket is missing in the AES payload.")
        self.assertTrue(region, "The region is missing in the AES payload.")

        return project_config_bucket, region

    def __get_custom_sts_session(self, access_key_id, secret_access_key, session_token):
        session = boto3.Session(
            aws_access_key_id=access_key_id,
            aws_secret_access_key=secret_access_key,
            aws_session_token=session_token,
        )
        sts = session.client('sts')
        return sts

    def __get_cgp_content_file_path(self):

        cgp_dir_path = self.get_gem_aws_path(self.TEST_RESOURCE_GROUP_NAME, constant.GEM_CGP_DIRECTORY_NAME) 
        if not os.path.exists(cgp_dir_path):
            os.mkdir(cgp_dir_path)

        cgp_dir_path = os.path.join(cgp_dir_path, 'dist')
        if not os.path.exists(cgp_dir_path):
            os.mkdir(cgp_dir_path)

        cgp_file_path = os.path.join(cgp_dir_path, self.CGP_CONTENT_FILE_NAME)

        return cgp_file_path

    def __get_cgp_content_location(self):
        configuration_bucket = self.get_stack_resource_physical_id(self.get_project_stack_arn(), 'Configuration')
        object_key = constant.GEM_CGP_DIRECTORY_NAME + '/deployment/' + self.TEST_DEPLOYMENT_NAME + '/resource-group/' + self.TEST_RESOURCE_GROUP_NAME + '/dist/' + self.CGP_CONTENT_FILE_NAME
        return (configuration_bucket, object_key)

