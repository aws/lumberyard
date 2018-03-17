import resource_manager_common.constant as constant
import boto3
from botocore.exceptions import ClientError
from resource_manager.test import lmbr_aws_test_support
from requests_aws4auth import AWS4Auth
import requests

class IntegrationTest_CloudGemTextToSpeech_APISecurity(lmbr_aws_test_support.lmbr_aws_TestCase):

    TEST_VOICE_PAYLOAD = {
        "voice": "Joanna",
        "message": "Hello world, this is an integration test"
    }

    GEM_NAME = 'CloudGemTextToSpeech'

    def __init__(self, *args, **kwargs):
        super(IntegrationTest_CloudGemTextToSpeech_APISecurity, self).__init__(*args, **kwargs)

    def setUp(self):
        self.prepare_test_envionment("cloud_gem_text_to_speech_test")

    def test_end_to_end(self):
        self.run_all_tests()

    ''' TESTS '''
    def __000_create_stacks(self):
        self.enable_real_gem(self.GEM_NAME)

        if not self.__has_project_stack():
            self.lmbr_aws('project', 'create', '--stack-name', self.TEST_PROJECT_STACK_NAME, '--confirm-aws-usage', '--confirm-security-change', '--region', lmbr_aws_test_support.REGION)
        else:
            print 'Reusing existing project stack {}'.format(self.get_project_stack_arn())

        if not self.__has_deployment_stack():
            self.lmbr_aws('deployment', 'create', '--deployment', self.TEST_DEPLOYMENT_NAME, '--confirm-aws-usage', '--confirm-security-change')

        self.context['identity_pool_id'] = self.get_stack_resource_physical_id(self.get_deployment_access_stack_arn(self.TEST_DEPLOYMENT_NAME), 'PlayerAccessIdentityPool')

    def __010_get_anonymous_aws_credentials(self):
        identity_response = self.aws_cognito_identity.get_id(IdentityPoolId=self.context['identity_pool_id'], Logins={})
        response = self.aws_cognito_identity.get_credentials_for_identity(IdentityId=identity_response['IdentityId'], Logins={})
        self.context['anonymous_aws_credentials'] = {
            'AccessKeyId': response['Credentials']['AccessKeyId'],
            'SecretKey': response['Credentials']['SecretKey'],
            'SessionToken': response['Credentials']['SessionToken']
        }

    def __020_test_game_client_post_speech(self):
        # will throw error in __service_post if not 200
        response = self.__service_post('/tts/voiceline', self.TEST_VOICE_PAYLOAD, anonymous_auth=True)

    def __30_test_game_client_use_admin_api(self):
        response = self.__service_get('/characterlib', anonymous_auth=True, expected_status_code=403)

    def __40_test_admin_use_admin_api(self):
        # will throw error in __service_get if not 200
        response = self.__service_get('/characterlib', admin_auth=True)


    def __999_cleanup(self):
        self.lmbr_aws('deployment', 'delete', '--deployment', self.TEST_DEPLOYMENT_NAME, '--confirm-resource-deletion')
        self.lmbr_aws('project', 'delete', '--confirm-resource-deletion')


    ''' Service URL helpers '''
    def __get_service_url(self, path):
        base_url = self.get_stack_output(self.get_resource_group_stack_arn(self.TEST_DEPLOYMENT_NAME, self.GEM_NAME), 'ServiceUrl')
        self.assertIsNotNone(base_url, "Missing ServiceUrl stack output.")
        return base_url + path

    def __service_get(self, path, params=None, anonymous_auth=False, admin_auth=False, expected_status_code=200):
        return self.__service_call('GET', path, params=params, anonymous_auth=anonymous_auth, admin_auth=admin_auth, expected_status_code=expected_status_code)

    def __service_post(self, path, body=None, anonymous_auth=False, admin_auth=False, expected_status_code=200):
        return self.__service_call('POST', path, body=body, anonymous_auth=anonymous_auth, admin_auth=admin_auth, expected_status_code=expected_status_code)

    def __service_call(self, method, path, body=None, params=None, anonymous_auth=False, admin_auth=False, expected_status_code=200):
        url = self.__get_service_url(path)
        if admin_auth:
            session_credentials = self.session.get_credentials().get_frozen_credentials()
            auth = AWS4Auth(session_credentials.access_key, session_credentials.secret_key, self.TEST_REGION, 'execute-api')
        elif anonymous_auth:
            creds = self.context['anonymous_aws_credentials']
            auth = AWS4Auth(creds['AccessKeyId'], creds['SecretKey'], self.TEST_REGION, 'execute-api', session_token=creds['SessionToken'])
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


    ''' Test stack helpers '''
    def __has_project_stack(self):
        return bool(self.get_project_stack_arn())

    def __has_deployment_stack(self):
        return bool(self.get_deployment_stack_arn(self.TEST_DEPLOYMENT_NAME))

    @classmethod
    def isrunnable(self, methodname, dict):
        return lmbr_aws_test_support.lmbr_aws_TestCase.isrunnable(methodname, dict)