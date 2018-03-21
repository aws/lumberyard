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

from resource_manager.test import lmbr_aws_test_support

import resource_manager_common.constant as constant
    
import boto3
from botocore.exceptions import ClientError

import requests

# TODO: split the service api tests out into a seperate class
class IntegrationTest_CloudGemFramework_CloudGemPortal(lmbr_aws_test_support.lmbr_aws_TestCase):

    CGP_CONTENT_FILE_NAME = "Foo.js"

    def __init__(self, *args, **kwargs):
        super(IntegrationTest_CloudGemFramework_CloudGemPortal, self).__init__(*args, **kwargs)

    def setUp(self):        
        self.prepare_test_envionment("cloud_gem_framework_test")

    def test_end_to_end(self):
        self.run_all_tests()

    def __000_create_stacks(self): 
        self.lmbr_aws('project', 'create', '--stack-name', self.TEST_PROJECT_STACK_NAME, '--confirm-aws-usage', '--confirm-security-change', '--region', lmbr_aws_test_support.REGION)
        self.lmbr_aws('cloud-gem', 'create', '--gem', self.TEST_RESOURCE_GROUP_NAME, '--initial-content', 'no-resources', '--enable')
        self.lmbr_aws('deployment', 'create', '--deployment', self.TEST_DEPLOYMENT_NAME, '--confirm-aws-usage', '--confirm-security-change')

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
        self.lmbr_aws('resource-group', 'upload-resources', '--resource-group', self.TEST_RESOURCE_GROUP_NAME, '--deployment', self.TEST_DEPLOYMENT_NAME, '--confirm-aws-usage', '--confirm-security-change')

    def __219_verify_cgp_admin_account(self):
        self.lmbr_aws('cloud-gem-framework', 'create-admin', ignore_failure=True )

        userpoolid = self.get_stack_resource_physical_id(self.get_project_stack_arn(), "ProjectUserPool")
        response = self.aws_cognito_idp.admin_get_user(
            UserPoolId=userpoolid,
            Username='administrator'
        )
        self.assertTrue(response, 'The administrator account was not created.')
        self.assertTrue(response['Username'], 'The administrator account was not created.')

    def __220_verify_cgp_resources(self):
        (bucket, key) = self.__get_cgp_content_location()
        self.verify_s3_object_exists(bucket, key)

    def __230_verify_cgp_default_url(self):
        self.lmbr_aws('cloud-gem-framework', 'cloud-gem-portal', '--show-url-only', '--silent-create-admin')

        url = self.lmbr_aws_stdout
        self.assertTrue(url, "The Cloud Gem Portal URL was not generated.")

        awsaccesskeyid, expiration, signature = self.__validate_presigned_url_parts(url)

        self.assertAlmostEqual(int(expiration), constant.PROJECT_CGP_DEFAULT_EXPIRATION_SECONDS, delta=5) # +/- 5 seconds

        host, querystring = self.__parse_url_into_parts(url)

        self.assertTrue(querystring, "The URL querystring was not found.")

        self.lmbr_aws('cloud-gem-framework', 'cloud-gem-portal', '--show-current-configuration', '--silent-create-admin')
        s_object = str(self.lmbr_aws_stdout)
        
        bootstrap_config = json.loads(s_object)

        self.__validate_bootstrap_config(bootstrap_config)

        self.lmbr_aws('cloud-gem-framework', 'cloud-gem-portal', '--show-url-only', '--silent-create-admin')

        url2 = self.lmbr_aws_stdout
        self.assertTrue(url2, "The Cloud Gem Portal URL was not generated.")

        request = requests.get(str(url2).strip())
        self.assertEqual(request.status_code, 200, "The pre-signed url response code was not 200 but instead it was " + str(request.status_code))

    def __240_verify_cgp_url_with_expiration(self):
        expiration_duration_in_seconds = 1200
        self.lmbr_aws('cloud-gem-framework', 'cloud-gem-portal', '--show-url-only', '--silent-create-admin', '--duration-seconds', str(expiration_duration_in_seconds))

        url = self.lmbr_aws_stdout
        self.assertTrue(url, "The Cloud Gem Portal URL was not generated.")

        awsaccesskeyid, expiration, signature = self.__validate_presigned_url_parts(url)

        self.assertAlmostEqual(int(expiration), expiration_duration_in_seconds, delta=5) # +/- 5 seconds

    def __600_verify_cgp_user_pool(self):
        self.__verify_only_administrator_can_signup_user_for_cgp()
        self.__verify_cgp_user_pool_groups()
        self.__verify_cgp_user_pool_permissions()

    def __860_verify_cgp_user_pool_after_project_update(self):
        self.lmbr_aws('project', 'update', '--confirm-aws-usage', '--confirm-security-change')
        self.lmbr_aws('cloud-gem-framework', 'cloud-gem-portal', '--show-url-only', '--silent-create-admin')
        time.sleep(30) # Give the cloud gem framework a few seconds to populate cgp_bootstrap 
        self.__verify_only_administrator_can_signup_user_for_cgp()
        self.__verify_cgp_user_pool_groups()

    def __910_remove_cgp_resources(self):
        cgp_file_path = self.__get_cgp_content_file_path()
        os.remove(cgp_file_path)

    def __920_delete_resources(self):
        self.lmbr_aws('resource-group', 'upload-resources', '--resource-group', self.TEST_RESOURCE_GROUP_NAME, '--deployment', self.TEST_DEPLOYMENT_NAME, '--confirm-aws-usage', '--confirm-security-change', '--confirm-resource-deletion')
        self.verify_stack("resource group stack", self.get_resource_group_stack_arn(self.TEST_DEPLOYMENT_NAME, self.TEST_RESOURCE_GROUP_NAME),
            {
                'StackStatus': 'UPDATE_COMPLETE',
                'StackResources': {
                    'AccessControl': {
                        'ResourceType': 'Custom::AccessControl'
                    }
                }
            })
        expected_logical_ids = []
        self.verify_user_mappings(self.TEST_DEPLOYMENT_NAME, expected_logical_ids)

    def __999_cleanup(self):
        self.lmbr_aws('deployment', 'delete', '--deployment', self.TEST_DEPLOYMENT_NAME, '--confirm-resource-deletion')
        self.lmbr_aws('project', 'delete', '--confirm-resource-deletion')

    def __parse_url_into_parts(self, url):
        parts = url.split('?')

        self.assertEqual(len(parts), 2, 'The Cloud Gem Portal URL is malformed.')

        return parts[0], parts[1]

    def __verify_only_administrator_can_signup_user_for_cgp(self):
        time.sleep(20) # allow time for IAM to reach a consistant state
        
        userpoolid = self.get_stack_resource_physical_id(self.get_project_stack_arn(), "ProjectUserPool")
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
        user_pool_id = self.get_stack_resource_physical_id(self.get_project_stack_arn(), "ProjectUserPool")
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
        user_pool_id = self.get_stack_resource_physical_id(self.get_project_stack_arn(), "ProjectUserPool")
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

    def __validate_presigned_url_parts(self, url):
        parts = url.split('?')

        self.assertEqual(len(parts), 2, 'The pre-signed URL is malformed.')
        querystring_fragment = parts[1]
        qf_parts = querystring_fragment.split('#')

        querystring = qf_parts[0]

        self.assertTrue(querystring, "The pre-signed URL querystring is missing.")

        q_parts = querystring.split("&")
        self.assertEqual(len(q_parts), 6, 'The pre-signed URL querystring is malformed.')
        kv_algorithm= q_parts[0]
        kv_expires = q_parts[1]
        kv_creds = q_parts[2]
        kv_header = q_parts[3]
        kv_date= q_parts[4]
        kv_signature = q_parts[5]

        self.assertTrue(kv_creds, "The AWS access key id is missing in the pre-signed URL.")
        self.assertTrue(kv_expires, "The expiration is missing in the pre-signed URL.")
        self.assertTrue(kv_signature, "The signature is missing in the pre-signed URL.")
        self.assertTrue(kv_algorithm, "The AWS algorithm id is missing in the pre-signed URL.")
        self.assertTrue(kv_header, "The header is missing in the pre-signed URL.")
        self.assertTrue(kv_date, "The date is missing in the pre-signed URL.")

        aws_access_key_parts = kv_creds.split('=')
        expires_parts = kv_expires.split('=')
        signature_parts = kv_signature.split('=')
        header_parts = kv_header.split('=')
        date_parts = kv_date.split('=')
        algorithm_parts = kv_algorithm.split('=')

        self.assertEqual(aws_access_key_parts[0], 'X-Amz-Credential', "The AWS access key id is missing in the pre-signed URL.")
        self.assertEqual(expires_parts[0], 'X-Amz-Expires', "The expiration is missing in the pre-signed URL.")
        self.assertEqual(signature_parts[0], 'X-Amz-Signature', "The signature is missing in the pre-signed URL.")
        self.assertEqual(header_parts[0], 'X-Amz-SignedHeaders', "The AWS header id is missing in the pre-signed URL.")
        self.assertEqual(date_parts[0], 'X-Amz-Date', "The date is missing in the pre-signed URL.")
        self.assertEqual(algorithm_parts[0], 'X-Amz-Algorithm', "The algorithm id is missing in the pre-signed URL.")

        self.assertTrue(aws_access_key_parts[1], "The AWS access key id value is missing in the pre-signed URL.")
        self.assertTrue(expires_parts[1], "The expiration value is missing in the pre-signed URL.")
        self.assertTrue(signature_parts[1], "The signature value is missing in the pre-signed URL.")
        self.assertTrue(header_parts[1], "The AWS header value is missing in the pre-signed URL.")
        self.assertTrue(date_parts[1], "The data value is missing in the pre-signed URL.")
        self.assertTrue(algorithm_parts[1], "The algorithm value is missing in the pre-signed URL.")

        return aws_access_key_parts[1], expires_parts[1], signature_parts[1]

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

