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

import boto3
from botocore.exceptions import ClientError
import datetime
import json
from resource_manager.test import lmbr_aws_test_support
from resource_manager.test import base_stack_test
from requests_aws4auth import AWS4Auth
import requests
import time

REGION='us-east-1'

class IntegrationTest_CloudGemComputeFarm_BasicFunctionality(base_stack_test.BaseStackTestCase):

    # Fails in cleanup to keep the deployment stack intact for the next test rerun.
    FAST_TEST_RERUN = False
    # Upload lambda code if the deployment stack is re-used.
    UPLOAD_LAMBDA_CODE_FOR_EXISTING_STACK = True

    GEM_NAME = 'CloudGemComputeFarm'

    def __init__(self, *args, **kwargs):
        super(IntegrationTest_CloudGemComputeFarm_BasicFunctionality, self).__init__(*args, **kwargs)

    def setUp(self):
        self.prepare_test_environment("cloud_gem_compute_farm_test")
        self.register_for_shared_resources()
        self.enable_shared_gem(self.GEM_NAME, 'v1')

    def test_end_to_end(self):
        self.run_all_tests()

    def __000_create_stacks(self):
        new_proj_created, new_deploy_created = self.setup_base_stack()
            
        if not new_deploy_created and self.UPLOAD_LAMBDA_CODE_FOR_EXISTING_STACK:
            self.lmbr_aws('function', 'upload-code', '--resource-group', self.GEM_NAME, '-d', self.TEST_DEPLOYMENT_NAME)

    def __005_test_events(self):
        response = self.__service_get('/events?time=0&run_id=-', admin_auth=True)
        self.context['events'] = response
        self.assertEquals(len(response['events']), 0)

    def __010_test_builds(self):
        response = self.__service_get('/builds', admin_auth=True)
        self.context['builds'] = response
        self.assertTrue(isinstance(response, list))
        self.assertEquals(len(response), 0)

    def __015_test_fleet_config(self):
        response = self.__service_get('/fleetconfig', admin_auth=True)
        self.context['fleetconfig'] = response
        self.assertTrue(isinstance(response, dict))
        self.assertEquals(len(response), 0)

    def __999_cleanup(self):
        if self.FAST_TEST_RERUN:
            print 'Tests passed enough to reach cleanup, failing in cleanup to prevent stack deletion since FAST_TEST_RERUN is true.'
            self.assertFalse(self.FAST_TEST_RERUN)
        self.teardown_base_stack()

    def __get_service_url(self, path):
        base_url = self.get_stack_output(self.get_resource_group_stack_arn(self.TEST_DEPLOYMENT_NAME, self.GEM_NAME), 'ServiceUrl')
        self.assertIsNotNone(base_url, "Missing ServiceUrl stack output.")
        return base_url + path

    def __service_get(self, path, params=None, anonymous_auth=False, player_auth=False, player_credentials=None, admin_auth=False, expected_status_code=200):
        return self.__service_call('GET', path, params=params, anonymous_auth=anonymous_auth, player_auth=player_auth,
                player_credentials=player_credentials, admin_auth=admin_auth, expected_status_code=expected_status_code)

    def __service_put(self, path, body=None, anonymous_auth=False, player_auth=False, player_credentials=None, admin_auth=False, expected_status_code=200):
        return self.__service_call('PUT', path, body=body, anonymous_auth=anonymous_auth, player_auth=player_auth,
                player_credentials=player_credentials, admin_auth=admin_auth, expected_status_code=expected_status_code)

    def __service_post(self, path, body=None, anonymous_auth=False, player_auth=False, player_credentials=None, admin_auth=False, expected_status_code=200):
        return self.__service_call('POST', path, body=body, anonymous_auth=anonymous_auth, player_auth=player_auth,
                player_credentials=player_credentials, admin_auth=admin_auth, expected_status_code=expected_status_code)

    def __service_call(self, method, path, body=None, params=None, anonymous_auth=False, player_auth=False, player_credentials=None, admin_auth=False, assumed_role=None, expected_status_code=200):
        url = self.__get_service_url(path)
        if admin_auth:
            session_credentials = self.session.get_credentials().get_frozen_credentials()
            auth = AWS4Auth(session_credentials.access_key, session_credentials.secret_key, self.TEST_REGION, 'execute-api')
        elif anonymous_auth:
            creds = self.context['anonymous_aws_credentials']
            auth = AWS4Auth(creds['AccessKeyId'], creds['SecretKey'], self.TEST_REGION, 'execute-api', session_token=creds['SessionToken'])
        elif player_credentials:
            auth = AWS4Auth(player_credentials['AccessKeyId'], player_credentials['SecretKey'], self.TEST_REGION, 'execute-api', session_token=player_credentials['SessionToken'])
        elif player_auth:
            creds = self.context['aws_credentials']
            auth = AWS4Auth(creds['AccessKeyId'], creds['SecretKey'], self.TEST_REGION, 'execute-api', session_token=creds['SessionToken'])
        elif assumed_role:
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
        else:
            auth = None

        response = requests.request(method, url, auth=auth, json=body, params=params)

        self.assertEquals(response.status_code, expected_status_code,
            'Expected status code {} but got {}, response: {}'.format(expected_status_code, response.status_code, response.text))

        if response.status_code == 200:
            result = response.json().get('result', None)
            self.assertIsNotNone(result, "Missing 'result' in response: {}".format(response))
            return result
        else:
            return response.text

    def __get_aws_credentials(self, identity_id, auth_tokens):
        logins = {self.context['user_pool_provider_id']: auth_tokens['IdToken']}
        response = self.aws_cognito_identity.get_credentials_for_identity(IdentityId=identity_id, Logins=logins)
        return {
            'AccessKeyId': response['Credentials']['AccessKeyId'],
            'SecretKey': response['Credentials']['SecretKey'],
            'SessionToken': response['Credentials']['SessionToken']
        }

    @classmethod
    def isrunnable(self, methodname, dict):
        if self.FAST_TEST_RERUN:
            return True
        return lmbr_aws_test_support.lmbr_aws_TestCase.isrunnable(methodname, dict)
