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
from requests_aws4auth import AWS4Auth
import requests
import time

REGION='us-east-1'

class IntegrationTest_CloudGemInGameSurvey_EndToEnd(lmbr_aws_test_support.lmbr_aws_TestCase):

    # Fails in cleanup to keep the deployment stack intact for the next test rerun.
    FAST_TEST_RERUN = False
    # Upload lambda code if the deployment stack is re-used.
    UPLOAD_LAMBDA_CODE_FOR_EXISTING_STACK = True

    GEM_NAME = 'CloudGemInGameSurvey'

    maxDiff = None

    SURVEY1 = {
        'survey_name': 'survey1',
        'questions': [
            {
                'title': 'survey1_q1',
                'type': 'text',
                'max_chars': 140
            },
            {
                'title': 'survey1_q2',
                'type': 'scale',
                'min': 1,
                'max': 10,
                'min_label': 'm',
                'max_label': 'M'
            }
        ]
    }

    SURVEY2 = {
        'survey_name': 'survey2',
        'questions': [
            {
                'title': 'survey2_q1',
                'type': 'predefined',
                'predefines': ['a1','a2','a3'],
                'multiple_select': True
            },
            {
                'title': 'survey2_q2',
                'type': 'predefined',
                'predefines': ['a1','a2'],
                'multiple_select': False
            },
        ]
    }

    def __init__(self, *args, **kwargs):
        super(IntegrationTest_CloudGemInGameSurvey_EndToEnd, self).__init__(*args, **kwargs)

    def setUp(self):
        self.prepare_test_envionment("cloud_gem_in_game_survey_test")

    def test_end_to_end(self):
        self.run_all_tests()

    def __000_create_stacks(self):
        self.enable_real_gem(self.GEM_NAME)

        if not self.__has_project_stack():
            self.lmbr_aws('project', 'create', '--stack-name', self.TEST_PROJECT_STACK_NAME, '--confirm-aws-usage', '--confirm-security-change', '--region', lmbr_aws_test_support.REGION)
        else:
            print 'Reusing existing project stack {}'.format(self.get_project_stack_arn())

        if not self.__has_deployment_stack():
            self.lmbr_aws('deployment', 'create', '--deployment', self.TEST_DEPLOYMENT_NAME, '--confirm-aws-usage', '--confirm-security-change')
        else:
            print 'Reusing existing deployment stack.'
            if self.UPLOAD_LAMBDA_CODE_FOR_EXISTING_STACK:
                self.lmbr_aws('function', 'upload-code', '--resource-group', 'CloudGemInGameSurvey')

    def __create_survey(self, survey_data):
        survey_info = {}
        response = self.__service_post('/surveys', { 'survey_name': survey_data['survey_name'] }, admin_auth=True)
        survey_info['survey_id'] = response['survey_id']

        survey_info['question_ids'] = []
        for question in survey_data['questions']:
            response = self.__service_post('/surveys/{}/questions'.format(survey_info['survey_id']), question, admin_auth=True)
            survey_info['question_ids'].append(response['question_id'])

        return survey_info

    def __005_create_surveys(self):
        survey_info = self.__create_survey(self.SURVEY1)
        self.context['survey1'] = survey_info

        # sleep for 1 second for the creation time of survey 1 & 2 to be different
        # to test sorting by creation time
        time.sleep(1)

        survey_info = self.__create_survey(self.SURVEY2)
        self.context['survey2'] = survey_info

    def __010_test_get_survey_metadata_sort_desc(self):
        response = self.__service_get('/survey_metadata', admin_auth=True)
        survey_metadata_list = response['metadata_list']
        self.assertEquals(len(survey_metadata_list), 2)
        self.assertEquals(survey_metadata_list[0]['survey_name'], self.SURVEY2['survey_name'])
        self.assertEquals(survey_metadata_list[0]['num_active_questions'], len(self.SURVEY2['questions']))
        self.assertEquals(survey_metadata_list[0]['published'], False)
        self.assertEquals(survey_metadata_list[0]['num_responses'], 0)

        self.assertEquals(survey_metadata_list[1]['survey_name'], self.SURVEY1['survey_name'])
        self.assertEquals(survey_metadata_list[0]['num_active_questions'], len(self.SURVEY1['questions']))
        self.assertEquals(survey_metadata_list[1]['published'], False)
        self.assertEquals(survey_metadata_list[1]['num_responses'], 0)

    def __015_test_get_survey_metadata_sort_asc(self):
        response = self.__service_get('/survey_metadata?sort=ASC', admin_auth=True)
        survey_metadata_list = response['metadata_list']
        self.assertEquals(len(survey_metadata_list), 2)
        self.assertEquals(survey_metadata_list[0]['survey_name'], self.SURVEY1['survey_name'])
        self.assertEquals(survey_metadata_list[0]['num_active_questions'], len(self.SURVEY1['questions']))
        self.assertEquals(survey_metadata_list[0]['published'], False)
        self.assertEquals(survey_metadata_list[0]['num_responses'], 0)

        self.assertEquals(survey_metadata_list[1]['survey_name'], self.SURVEY2['survey_name'])
        self.assertEquals(survey_metadata_list[0]['num_active_questions'], len(self.SURVEY2['questions']))
        self.assertEquals(survey_metadata_list[1]['published'], False)
        self.assertEquals(survey_metadata_list[1]['num_responses'], 0)

    def __020_test_admin_get_survey1(self):
        response = self.__service_get('/surveys/{}'.format(self.context['survey1']['survey_id']), admin_auth=True)
        self.assertEquals(response['survey_name'], self.SURVEY1['survey_name'])
        self.assertEquals(len(response['questions']), len(self.SURVEY1['questions']))
        for i, question in enumerate(response['questions']):
            self.assertEquals(question['enabled'], True)
            del question['enabled']
            self.assertEquals(question['question_id'], self.context['survey1']['question_ids'][i])
            del question['question_id']
        self.assertEquals(response['questions'], self.SURVEY1['questions'])

    def __025_test_admin_get_survey2(self):
        response = self.__service_get('/surveys/{}'.format(self.context['survey2']['survey_id']), admin_auth=True)
        self.assertEquals(response['survey_name'], self.SURVEY2['survey_name'])
        self.assertEquals(len(response['questions']), len(self.SURVEY2['questions']))
        for i, question in enumerate(response['questions']):
            self.assertEquals(question['enabled'], True)
            del question['enabled']
            self.assertEquals(question['question_id'], self.context['survey2']['question_ids'][i])
            del question['question_id']
        self.assertEquals(response['questions'], self.SURVEY2['questions'])

    def __030_test_get_active_survey_metadata_return_nothing(self):
        response = self.__service_get('/active/survey_metadata', admin_auth=True)
        survey_metadata_list = response['metadata_list']
        self.assertEquals(len(survey_metadata_list), 0)

    def __035_test_get_active_survey1_forbidden(self):
        self.__service_get('/active/surveys/{}'.format(self.context['survey1']['survey_id']), admin_auth=True, expected_status_code=403)

    def __040_test_get_active_survey1_forbidden(self):
        self.__service_get('/active/surveys/{}'.format(self.context['survey2']['survey_id']), admin_auth=True, expected_status_code=403)

    def __045_publish_surveys(self):
        self.__service_put('/surveys/{}/published'.format(self.context['survey1']['survey_id']), {"published": True}, admin_auth=True)
        self.__service_put('/surveys/{}/published'.format(self.context['survey2']['survey_id']), {"published": True}, admin_auth=True)

    def __050_test_get_active_survey_metadata_sort_desc(self):
        response = self.__service_get('/active/survey_metadata', admin_auth=True)
        survey_metadata_list = response['metadata_list']
        self.assertEquals(len(survey_metadata_list), 2)
        self.assertEquals(survey_metadata_list[0]['survey_name'], self.SURVEY2['survey_name'])
        self.assertEquals(survey_metadata_list[0]['num_active_questions'], len(self.SURVEY2['questions']))
        self.assertEquals(survey_metadata_list[0]['num_responses'], 0)

        self.assertEquals(survey_metadata_list[1]['survey_name'], self.SURVEY1['survey_name'])
        self.assertEquals(survey_metadata_list[0]['num_active_questions'], len(self.SURVEY1['questions']))
        self.assertEquals(survey_metadata_list[1]['num_responses'], 0)

    def __055_test_get_active_survey_metadata_sort_asc(self):
        response = self.__service_get('/active/survey_metadata?sort=ASC', admin_auth=True)
        survey_metadata_list = response['metadata_list']
        self.assertEquals(len(survey_metadata_list), 2)
        self.assertEquals(survey_metadata_list[0]['survey_name'], self.SURVEY1['survey_name'])
        self.assertEquals(survey_metadata_list[0]['num_active_questions'], len(self.SURVEY1['questions']))
        self.assertEquals(survey_metadata_list[0]['num_responses'], 0)

        self.assertEquals(survey_metadata_list[1]['survey_name'], self.SURVEY2['survey_name'])
        self.assertEquals(survey_metadata_list[0]['num_active_questions'], len(self.SURVEY2['questions']))
        self.assertEquals(survey_metadata_list[1]['num_responses'], 0)

    def __060_test_get_active_survey1(self):
        response = self.__service_get('/active/surveys/{}'.format(self.context['survey1']['survey_id']), admin_auth=True)
        self.assertEquals(response['survey_name'], self.SURVEY1['survey_name'])
        self.assertEquals(len(response['questions']), len(self.SURVEY1['questions']))
        for i, question in enumerate(response['questions']):
            self.assertEquals(question['question_id'], self.context['survey1']['question_ids'][i])
            del question['question_id']
        self.assertEquals(response['questions'], self.SURVEY1['questions'])

    def __065_test_get_active_survey2(self):
        response = self.__service_get('/active/surveys/{}'.format(self.context['survey2']['survey_id']), admin_auth=True)
        self.assertEquals(response['survey_name'], self.SURVEY2['survey_name'])
        self.assertEquals(len(response['questions']), len(self.SURVEY2['questions']))
        for i, question in enumerate(response['questions']):
            self.assertEquals(question['question_id'], self.context['survey2']['question_ids'][i])
            del question['question_id']
        self.assertEquals(response['questions'], self.SURVEY2['questions'])

    def __070_disable_survey1_question1(self):
        self.__service_put('/surveys/{}/questions/{}/status'.format(self.context['survey1']['survey_id'], self.context['survey1']['question_ids'][0]), {"enabled": False}, admin_auth=True)

    def __075_test_get_active_survey1_metadata_1_question_left(self):
        response = self.__service_get('/active/survey_metadata?sort=ASC', admin_auth=True)
        survey_metadata_list = response['metadata_list']
        self.assertEquals(len(survey_metadata_list), 2)
        self.assertEquals(survey_metadata_list[0]['survey_name'], self.SURVEY1['survey_name'])
        self.assertEquals(survey_metadata_list[0]['num_active_questions'], 1)

    def __080_test_get_active_survey1_1_question_left(self):
        response = self.__service_get('/active/surveys/{}'.format(self.context['survey1']['survey_id']), admin_auth=True)
        self.assertEquals(response['survey_name'], self.SURVEY1['survey_name'])
        self.assertEquals(len(response['questions']), 1)
        del response['questions'][0]['question_id']
        self.assertEquals(response['questions'][0], self.SURVEY1['questions'][1])

    def __999_cleanup(self):
        if self.FAST_TEST_RERUN:
            print 'Tests passed enough to reach cleanup, failing in cleanup to prevent stack deletion since FAST_TEST_RERUN is true.'
            self.assertFalse(self.FAST_TEST_RERUN)
        self.lmbr_aws('deployment', 'delete', '--deployment', self.TEST_DEPLOYMENT_NAME, '--confirm-resource-deletion')
        self.lmbr_aws('project', 'delete', '--confirm-resource-deletion')

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

    def __has_project_stack(self):
        return bool(self.get_project_stack_arn())

    def __has_deployment_stack(self):
        return bool(self.get_deployment_stack_arn(self.TEST_DEPLOYMENT_NAME))

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
