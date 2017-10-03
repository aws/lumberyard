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
import subprocess
import os
import time

from resource_manager.test import lmbr_aws_test_support
import resource_management

import resource_manager.constant as constant
    
import requests
from requests_aws4auth import AWS4Auth

import boto3
from botocore.exceptions import ClientError

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
        self.verify_s3_object_exists(cgp_bucket_name, 'www/config.js')
        self.verify_s3_object_exists(cgp_bucket_name, 'www/bundles/app.bundle.js')
        self.verify_s3_object_exists(cgp_bucket_name, 'www/bundles/dependencies.bundle.js')

    def __110_add_service_api_resources(self):
        self.lmbr_aws('cloud-gem-framework', 'add-service-api-resources', '--resource-group', self.TEST_RESOURCE_GROUP_NAME)

    def __120_add_cgp_resources(self):
        cgp_file_path = self.__get_cgp_content_file_path()
        with open(cgp_file_path, 'w') as f:
            f.write('test file content')

    def __200_create_resources(self):
        self.lmbr_aws('resource-group', 'upload-resources', '--resource-group', self.TEST_RESOURCE_GROUP_NAME, '--deployment', self.TEST_DEPLOYMENT_NAME, '--confirm-aws-usage', '--confirm-security-change')

    def __210_verify_service_api_resources(self):

        self.verify_stack("resource group stack", self.get_resource_group_stack_arn(self.TEST_DEPLOYMENT_NAME, self.TEST_RESOURCE_GROUP_NAME),
            {
                'StackStatus': 'UPDATE_COMPLETE',
                'StackResources': {
                    resource_management.LAMBDA_CONFIGURATION_RESOURCE_NAME: {
                        'ResourceType': 'Custom::LambdaConfiguration'
                    },
                    resource_management.LAMBDA_FUNCTION_RESOURCE_NAME: {
                        'ResourceType': 'AWS::Lambda::Function'
                    },
                    resource_management.API_RESOURCE_NAME: {
                        'ResourceType': 'Custom::ServiceApi'
                    },
                    'AccessControl': {
                        'ResourceType': 'Custom::AccessControl'
                    }
                }
            })

        # ServiceApi not player accessible (yet), so should be no mapping
        expected_logical_ids = []
        self.verify_user_mappings(self.TEST_DEPLOYMENT_NAME, expected_logical_ids)

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

    def __300_call_simple_api(self):
        result = self.__service_get('/service/status')
        status = result.get('status', None)
        self.assertIsNotNone(status, "Missing 'status' in result: {}".format(result))
        self.assertEqual(status, 'online')

    def __310_call_simple_api_with_no_credentials(self):
        self.__service_get('/service/status', use_aws_auth=False, expected_status_code=403)
        pass

    def __400_add_complex_apis(self):
        self.__add_complex_api('string')
        self.__add_complex_api('number')
        self.__add_complex_api('boolean')
        self.__enable_complex_api_player_access('string')

    def __405_upload_complex_apis(self):
        self.lmbr_aws('resource-group', 'upload-resources', '--resource-group', self.TEST_RESOURCE_GROUP_NAME, '--deployment', self.TEST_DEPLOYMENT_NAME, '--confirm-aws-usage', '--confirm-security-change')
        time.sleep(30) # seems as if API Gateway can take a few seconds to update

    def __410_verify_service_api_mapping(self):
        expected_logical_id = self.TEST_RESOURCE_GROUP_NAME + '.ServiceApi'
        expected_logical_ids = [ expected_logical_id ]
        expected_service_url = self.__get_service_url('')
        expected_physical_resource_ids = { expected_logical_id: expected_service_url }
        self.verify_user_mappings(self.TEST_DEPLOYMENT_NAME, expected_logical_ids, expected_physical_resource_ids = expected_physical_resource_ids)

    def __420_call_complex_api_with_simple_strings(self):
        self.__call_complex_api('string', 'a', 'b', 'c')

    def __430_call_complex_api_with_complex_strings(self):
        self.__call_complex_api('string', '"pathparam" \'string\'', '"queryparam" \'string\'', '"bodyparam" \'string\'')

    def __440_call_complex_api_with_numbers(self):
        self.__call_complex_api('number', 42, 43, 44)

    def __450_call_complex_api_with_booleans(self):
        self.__call_complex_api('boolean', True, False, True)

    def __460_call_complex_api_using_player_credentials(self):
        self.__call_complex_api('string', 'c', 'b', 'a', assumed_role='Player')
        self.__call_complex_api('number', 44, 43, 42, assumed_role='Player', expected_status_code=403)

    def __470_call_complex_api_using_project_admin_credentials(self):
        self.__call_complex_api('string', 'x', 'y', 'z', assumed_role='ProjectAdmin')
        self.__call_complex_api('number', 24, 34, 44, assumed_role='ProjectAdmin')

    def __480_call_complex_api_using_deployment_admin_credentials(self):
        self.__call_complex_api('string', 'z', 'y', 'z', assumed_role='DeploymentAdmin')
        self.__call_complex_api('number', 24, 23, 22, assumed_role='DeploymentAdmin')

    def __490_call_complex_api_with_missing_string_pathparam(self):
        self.__call_complex_api('string', None, 'b', 'c', expected_status_code=404)

    def __491_call_complex_api_with_missing_string_queryparam(self):
        self.__call_complex_api('string', 'a', None, 'c', expected_status_code=400)

    def __492_call_complex_api_with_missing_string_bodyparam(self):
        self.__call_complex_api('string', 'a', 'b', None, expected_status_code=400)

    def __500_add_apis_with_optional_parameters(self):
        self.__add_apis_with_optional_parameters()

    def __505_upload_apis_with_optional_parameters(self):
        self.lmbr_aws('resource-group', 'upload-resources', '--resource-group', self.TEST_RESOURCE_GROUP_NAME, '--deployment', self.TEST_DEPLOYMENT_NAME, '--confirm-aws-usage', '--confirm-security-change')
        time.sleep(30) # seems as if API Gateway can take a few seconds to update

    def __510_call_api_with_both_parameters(self):
        self.__call_apis_with_optional_parameters(body_param = self.ACTUAL_BODY_PARAMETER_VALUE, query_param = self.ACTUAL_QUERY_PARAMETER_VALUE)

    def __520_call_api_without_bodyparam(self):
        self.__call_apis_with_optional_parameters(body_param = None, query_param = self.ACTUAL_QUERY_PARAMETER_VALUE)

    def __530_call_api_without_queryparam(self):
        self.__call_apis_with_optional_parameters(body_param = self.ACTUAL_BODY_PARAMETER_VALUE, query_param = None)

    def __600_verify_cgp_user_pool(self):
        self.__verify_only_administrator_can_signup_user_for_cgp()
        self.__verify_cgp_user_pool_groups()
        self.__verify_cgp_user_pool_permissions()

    def __800_run_cpp_tests(self):

        if not os.environ.get("ENABLE_CLOUD_CANVAS_CPP_INTEGRATION_TESTS", None):
            print '\n*** SKIPPING cpp tests because the ENABLE_CLOUD_CANVAS_CPP_INTEGRATION_TESTS envionment variable is not set.\n'
        else:

            self.lmbr_aws('update-mappings', '--release')
        
            mappings_file = os.path.join(self.GAME_DIR, 'Config', self.TEST_DEPLOYMENT_NAME + '.player.awsLogicalMappings.json')
            print 'Using mappings from', mappings_file
            os.environ["cc_override_resource_map"] = mappings_file

            lmbr_test_cmd = os.path.join(self.REAL_ROOT_DIR, 'lmbr_test.cmd')
            args =[
                lmbr_test_cmd, 
                'scan', 
                # '--wait-for-debugger', # uncomment if the tests are failing and you want to debug them.
                '--include-gems', 
                '--integ', 
                '--only', 'Gem.CloudGemFramework.6fc787a982184217a5a553ca24676cfa.v0.1.0.dll', 
                '--dir', os.environ.get("TEST_BUILD_DIR", "Bin64vc140.Debug.Test")
            ]
            print 'EXECUTING', ' '.join(args)

            result = subprocess.call(args, shell=True)

            # Currently lmbr_test blows up when exiting. The tests all pass.
            # lbmr_test fails even if our tests do absolutely nothing, so it
            # seems to be an issue with the test infrastructure itself.

            self.assertEquals(result, 0)

    def __860_verify_cgp_user_pool_after_project_update(self):
        self.lmbr_aws('project', 'update', '--confirm-aws-usage', '--confirm-security-change')
        self.lmbr_aws('cloud-gem-framework', 'cloud-gem-portal', '--show-url-only', '--silent-create-admin')
        time.sleep(30) # Give the cloud gem framework a few seconds to populate cgp_bootstrap 
        self.__verify_only_administrator_can_signup_user_for_cgp()
        self.__verify_cgp_user_pool_groups()

    def __900_remove_service_api_resources(self):
        self.lmbr_aws('cloud-gem-framework', 'remove-service-api-resources', '--resource-group', self.TEST_RESOURCE_GROUP_NAME)

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

    def __get_service_url(self, path):
        base_url = self.get_stack_output(self.get_resource_group_stack_arn(self.TEST_DEPLOYMENT_NAME, self.TEST_RESOURCE_GROUP_NAME), 'ServiceUrl')
        self.assertIsNotNone(base_url, "Missing ServiceUrl stack output.")
        return base_url + path

    def __service_get(self, path, use_aws_auth=True, expected_status_code=200):
        url = self.__get_service_url(path)
        if use_aws_auth:
            session_credentials = self.session.get_credentials().get_frozen_credentials()
            auth = AWS4Auth(session_credentials.access_key, session_credentials.secret_key, self.TEST_REGION, 'execute-api')
        else:
            auth = None
        response = requests.get(url, auth=auth)
        self.assertEquals(response.status_code, expected_status_code)
        if response.status_code == 200:
            result = response.json().get('result', None)
            self.assertIsNotNone(result, "Missing 'result' in response: {}".format(response))
            return result
        else:
            return None

    def __service_post(self, path, body, params, assumed_role=None, expected_status_code=200):
        url = self.__get_service_url(path)
        if assumed_role is None:
            session_credentials = self.session.get_credentials().get_frozen_credentials()
            access_key = session_credentials.access_key
            secret_key = session_credentials.secret_key
            session_token = None
        else:

            if assumed_role.startswith('Project'):
                role_arn = self.get_stack_resource_arn(self.get_project_stack_arn(), assumed_role)
            else:
                role_arn = self.get_stack_resource_arn(self.get_deployment_access_stack_arn(self.TEST_DEPLOYMENT_NAME), assumed_role)

            sts = self.session.client('sts')

            res = sts.assume_role(RoleArn=role_arn, RoleSessionName='CloudGemFrameworkTest')

            access_key = res['Credentials']['AccessKeyId']
            secret_key = res['Credentials']['SecretAccessKey']
            session_token = res['Credentials']['SessionToken']

        auth = AWS4Auth(access_key, secret_key, self.TEST_REGION, 'execute-api', session_token=session_token)
        
        response = requests.post(url, auth=auth, json=body, params=params)

        self.assertEquals(response.status_code, expected_status_code)

        if response.status_code == 200:
            result = response.json().get('result', None)
            self.assertIsNotNone(result, "Missing 'result' in response: {}".format(response))
            return result
        else:
            return None

    def __add_complex_api(self, param_type):
        self.__add_complex_api_to_swagger(param_type)
        self.__add_complex_api_to_code(param_type)

    def __add_complex_api_to_swagger(self, param_type):

        swagger_file_path = self.get_gem_aws_path(self.TEST_RESOURCE_GROUP_NAME, 'swagger.json')
        with open(swagger_file_path, 'r') as file:
            swagger = json.load(file)

        paths = swagger['paths']
        paths['/test/complex/' + param_type + '/{pathparam}'] = {
            'post': {
                "responses": {
                    "200": {
                        "description": "A successful service status response.",
                        "schema": {
                            "$ref": "#/definitions/ComplexResult" + param_type.title()
                        }
                    }
                },
                'parameters': [
                    {
                        'in': 'path',
                        'name': 'pathparam',
                        'type': param_type,
                        'required': True
                    },
                    {
                        'in': 'body',
                        'name': 'bodyparam',
                        'schema': {
                            "$ref": "#/definitions/ComplexBody" + param_type.title()
                        },
                        'required': True
                    },
                    {
                        'in': 'query',
                        'name': 'queryparam',
                        'type': param_type,
                        'required': True
                    }
                ]
            }
        }

        definitions = swagger['definitions']
        definitions['ComplexBody' + param_type.title()] = {
            "type": "object",
            "properties": {
                "data": {
                    "type": param_type
                }
            }, 
            "required": [
                "data"
            ]
        }
        definitions['ComplexResult' + param_type.title()] = {
            "properties": {
                'pathparam': {
                    "type": param_type
                },
                'queryparam': {
                    "type": param_type
                },
                'bodyparam': {
                    "$ref": "#/definitions/ComplexBody" + param_type.title()
                }
            },
            "required": [
                "pathparam",
                "queryparam",
                "bodyparam"
            ],
            "type": "object"
        }

        with open(swagger_file_path, 'w') as file:
            json.dump(swagger, file, indent=4)


    def __add_complex_api_to_code(self, param_type):
        file_path = self.get_gem_aws_path(self.TEST_RESOURCE_GROUP_NAME, 'lambda-code', 'ServiceLambda', 'api', 'test_complex_{}.py'.format(param_type))
        with open(file_path, 'w') as file:
            file.write(self.COMPLEX_API_CODE)

    def __enable_complex_api_player_access(self, param_type):
        file_path = self.get_gem_aws_path(self.TEST_RESOURCE_GROUP_NAME, 'resource-template.json')
        with open(file_path, 'r') as file:
            template = json.load(file)

        service_api_metadata = template['Resources']['ServiceApi']['Metadata'] = {
            'CloudCanvas': {
                'Permissions': {
                    'AbstractRole': 'Player',
                    'Action': 'execute-api:*',
                    'ResourceSuffix': '/*/POST/test/complex/' + param_type + '/*'
                }
            }
        }

        with open(file_path, 'w') as file:
            json.dump(template, file, indent=4, sort_keys=True)

    def __call_complex_api(self, param_type, path_param, query_param, body_param, assumed_role=None, expected_status_code=200):
        
        expected = {}
        
        body = { 'data': body_param } if body_param is not None else None
        expected['bodyparam'] = body

        if path_param is not None:
            expected['pathparam'] = path_param
        else:
            path_param = ''

        if query_param is not None:
            expected['queryparam'] = query_param

        # Make strings unicode so they match, this is a python json thing
        # not something API Gateway or the Lambda Function is doing.
        expected = json.loads(json.dumps(expected)) 

        # 'encode' url parametrs in JSON format
        if param_type == 'boolean': 
            path_param = 'true' if path_param else 'false'
            query_param = 'true' if query_param else 'false'

        url = '/test/complex/{}/{}'.format(
            param_type,
            str(path_param), 
        )

        params = {}
        if query_param is not None:
            params['queryparam'] = str(query_param)

        result = self.__service_post(url, body, params=params, assumed_role = assumed_role, expected_status_code = expected_status_code)
        if expected_status_code == 200:
            self.assertEquals(expected, result)


    COMPLEX_API_CODE = '''
import service
import errors

class TestErrorType(errors.ClientError):
    def __init__(self):
        super(TestErrorType, self).__init__('Test error message.')

@service.api
def post(request, pathparam, queryparam, bodyparam):
    if queryparam == 'test-query-param-failure':
        raise TestErrorType()
    else:
        return {
            'pathparam': pathparam,
            'queryparam': queryparam,
            'bodyparam': bodyparam
        }
'''

    DEFAULT_QUERY_PARAMETER_VALUE = "default_query_parameter_value"


    ACTUAL_BODY_PARAMETER_VALUE = {
        "data": "actual_body_paramter_value"
    }


    ACTUAL_QUERY_PARAMETER_VALUE = "actual_query_parameter_value"


    def __add_apis_with_optional_parameters(self):
        
        with_defaults = True
        self.__add_api_with_optional_parameters_to_swagger(with_defaults)
        self.__add_api_with_optional_parameters_to_code(with_defaults)

        with_defaults = False
        self.__add_api_with_optional_parameters_to_swagger(with_defaults)
        self.__add_api_with_optional_parameters_to_code(with_defaults)


    def __add_api_with_optional_parameters(self, with_defaults):
        self.__add_api_with_optional_parameters_to_swagger(with_defaults)
        self.__add_api_with_optional_parameters_to_code(with_defaults)


    def __add_api_with_optional_parameters_to_swagger(self, with_defaults):

        swagger_file_path = self.get_gem_aws_path(self.TEST_RESOURCE_GROUP_NAME, 'swagger.json')
        with open(swagger_file_path, 'r') as file:
            swagger = json.load(file)

        bodyparam = {
            'in': 'body',
            'name': 'bodyparam',
            'schema': {
                "$ref": "#/definitions/OptionalRequest"
            },
            'required': False
        }

        queryparam = {
            'in': 'query',
            'name': 'queryparam',
            'type': 'string',
            'required': False
        }

        if with_defaults:
            queryparam['default'] = self.DEFAULT_QUERY_PARAMETER_VALUE

        paths = swagger['paths']
        paths['/test/optional/{}'.format(self.__with_defaults_postfix(with_defaults))] = {
            'post': {
                "responses": {
                    "200": {
                        "description": "A successful service status response.",
                        "schema": {
                            "$ref": "#/definitions/OptionalResult"
                        }
                    }
                },
                'parameters': [
                    bodyparam,
                    queryparam
                ]
            }
        }

        definitions = swagger['definitions']
        definitions['OptionalRequest'] = {
            "type": "object",
            "properties": {
                "data": {
                    "type": "string"
                }
            }, 
            "required": [
                "data"
            ]
        }
        definitions['OptionalResult'] = {
            "properties": {
                'queryparam': {
                    "type": "string"
                },
                'bodyparam': {
                    "$ref": "#/definitions/OptionalRequest"
                }
            },
            "type": "object"
        }

        with open(swagger_file_path, 'w') as file:
            json.dump(swagger, file, indent=4)


    def __add_api_with_optional_parameters_to_code(self, with_defaults):
        file_path = self.get_gem_aws_path(self.TEST_RESOURCE_GROUP_NAME, 'lambda-code', 'ServiceLambda', 'api', 'test_optional_{}.py'.format(self.__with_defaults_postfix(with_defaults)))
        with open(file_path, 'w') as file:
            file.write(self.OPTIONAL_API_CODE)


    def __with_defaults_postfix(self, with_defaults):
        return 'withdefaults' if with_defaults else 'withoutdefaults'


    def __call_apis_with_optional_parameters(self, body_param = None, query_param = None, expected_status_code = 200):
        self.__call_api_with_optional_parameters(with_defaults = True, body_param = body_param, query_param = query_param, expected_status_code = expected_status_code)
        self.__call_api_with_optional_parameters(with_defaults = False, body_param = body_param, query_param = query_param, expected_status_code = expected_status_code)


    def __call_api_with_optional_parameters(self, with_defaults = False, query_param = None, body_param = None, assumed_role=None, expected_status_code=200):
        
        expected = {}
        
        if body_param is not None:
            body = { 'data': body_param } 
            expected['bodyparam'] = body
        else:
            body = None

        if query_param is not None:
            expected['queryparam'] = query_param
        else:
            if with_defaults:
                expected['queryparam'] = self.DEFAULT_QUERY_PARAMETER_VALUE

        # Make strings unicode so they match, this is a python json thing
        # not something API Gateway or the Lambda Function is doing.
        expected = json.loads(json.dumps(expected)) 

        url = '/test/optional/{}'.format(
            self.__with_defaults_postfix(with_defaults)
        )

        params = {}
        if query_param is not None:
            params['queryparam'] = str(query_param)

        result = self.__service_post(url, body, params = params, expected_status_code = expected_status_code)
        if expected_status_code == 200:
            self.assertEquals(expected, result)

    OPTIONAL_API_CODE = '''
import service
import errors

@service.api
def post(request, queryparam=None, bodyparam=None):
    result = {}
    if queryparam is not None:
        result['queryparam'] = queryparam
    if bodyparam is not None:
        result['bodyparam'] = bodyparam
    return result
'''

