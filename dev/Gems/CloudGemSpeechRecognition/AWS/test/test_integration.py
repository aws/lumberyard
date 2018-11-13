import resource_manager_common.constant as constant
import boto3
from botocore.exceptions import ClientError
from resource_manager.test import lmbr_aws_test_support
from resource_manager.test import base_stack_test
from requests_aws4auth import AWS4Auth
from cgf_utils import custom_resource_utils
import requests
import time
import json

class IntegrationTest_CloudGemSpeechRecognition_APISecurity(base_stack_test.BaseStackTestCase):

    TEST_TEXT_PAYLOAD = {
        "user_id": "test_dummy",
        "name": "lytestbot_qpjUfgcERGqZvGNc_Export",
        "bot_alias": "$LATEST",
        "text": "test"
    }

    TEST_BOT_SPEC = {
        "bot" : {
            "name" : "lytestbot_qpjUfgcERGqZvGNc_Export",
            "locale" : "en-US",
            "voiceId" : "Joanna",
            "childDirected" : False,
            "idleSessionTTLInSeconds" : 300,
            "abortStatement": {
                "messages": [
                    {
                        "content": "Sorry, I could not understand. Goodbye.",
                        "contentType": "PlainText"
                    }
                ]
            },
            "clarificationPrompt": {
                "maxAttempts": 5,
                "messages": [
                    {
                        "content": "Sorry, can you please repeat that?",
                        "contentType": "PlainText"
                    }
                ]
            },
            "intents" : [
                {
                    "intentVersion": "$LATEST",
                    "intentName": "lytestintent_YenNEQnbbGSpMVFF"
                }
            ]
        },
        "intents" : [
            {
                "name": "lytestintent_YenNEQnbbGSpMVFF",
                "sampleUtterances": [
                    "test"
                ],
                "slots": [],
                "fulfillmentActivity": {
                    "type": "ReturnIntent"
                }
            }
        ],
        "slot_types" : [
        ]
    }

    GEM_NAME = 'CloudGemSpeechRecognition'

    def __init__(self, *args, **kwargs):
        super(IntegrationTest_CloudGemSpeechRecognition_APISecurity, self).__init__(*args, **kwargs)

    def setUp(self):
        self.prepare_test_environment("cloud_gem_speech_recognition_test")
        self.register_for_shared_resources()
        self.enable_shared_gem(self.GEM_NAME)

    def test_end_to_end(self):
        self.run_all_tests()

    ''' TESTS '''
    def __000_create_stacks(self):
        self.setup_base_stack()        

    def __010_get_anonymous_aws_credentials(self):
        self.context['identity_pool_id'] = custom_resource_utils.get_embedded_physical_id(self.get_stack_resource_physical_id(self.get_deployment_access_stack_arn(self.TEST_DEPLOYMENT_NAME), 'PlayerAccessIdentityPool'))
        identity_response = self.aws_cognito_identity.get_id(IdentityPoolId=self.context['identity_pool_id'], Logins={})
        response = self.aws_cognito_identity.get_credentials_for_identity(IdentityId=identity_response['IdentityId'], Logins={})
        self.context['anonymous_aws_credentials'] = {
            'AccessKeyId': response['Credentials']['AccessKeyId'],
            'SecretKey': response['Credentials']['SecretKey'],
            'SessionToken': response['Credentials']['SessionToken']
        }

    def __015_put_bot_desc(self):
        spec = {
            'desc_file' : self.TEST_BOT_SPEC
        }
        print "Sending bot desc to create test bot"
        response = self.__service_put('/admin/botdesc', spec, admin_auth=True)
        print "Response to put bot desc: " + str(response)
        while True:
            response = self.__service_get('/admin/botstatus/' + self.TEST_BOT_SPEC['bot']['name'], admin_auth=True)
            if 'status' not in response:
                break
            else:
                self.assertFalse("ERROR" in response['status'])
                if response['status'] == 'READY':
                    break;
                if "ERROR" in response['status']:
                    break;    
                else:
                    print "Waiting for bot build: " + response['status']
            time.sleep(5)

    def __020_test_game_client_check_posttext(self):
        # will throw error in __service_post if not 200
        response = self.__service_post('/service/posttext', self.TEST_TEXT_PAYLOAD, anonymous_auth=True)

    def __30_test_game_client_use_admin_api(self):
        response = self.__service_get('/admin/numbots', anonymous_auth=True, expected_status_code=403)

    def __40_test_admin_use_admin_api(self):
        # will throw error in __service_get if not 200
        response = self.__service_get('/admin/numbots', admin_auth=True)

    def __999_cleanup(self):
        self.__service_delete('/admin/intent/' + self.TEST_BOT_SPEC['intents'][0]['name'], admin_auth=True)
        self.__service_delete('/admin/bot/' + self.TEST_BOT_SPEC['bot']['name'], admin_auth=True)
        self.teardown_base_stack()

    ''' Service URL helpers '''
    def __get_service_url(self, path):
        base_url = self.get_stack_output(self.get_resource_group_stack_arn(self.TEST_DEPLOYMENT_NAME, self.GEM_NAME), 'ServiceUrl')
        self.assertIsNotNone(base_url, "Missing ServiceUrl stack output.")
        return base_url + path

    def __service_get(self, path, params=None, anonymous_auth=False, admin_auth=False, expected_status_code=200):
        return self.__service_call('GET', path, params=params, anonymous_auth=anonymous_auth, admin_auth=admin_auth, expected_status_code=expected_status_code)

    def __service_post(self, path, body=None, anonymous_auth=False, admin_auth=False, expected_status_code=200):
        return self.__service_call('POST', path, body=body, anonymous_auth=anonymous_auth, admin_auth=admin_auth, expected_status_code=expected_status_code)

    def __service_delete(self, path, params=None, anonymous_auth=False, admin_auth=False, expected_status_code=200):
        return self.__service_call('DELETE', path, params=params, anonymous_auth=anonymous_auth, admin_auth=admin_auth, expected_status_code=expected_status_code)

    def __service_put(self, path, body=None, anonymous_auth=False, admin_auth=False, expected_status_code=200):
        return self.__service_call('PUT', path, body=body, anonymous_auth=anonymous_auth, admin_auth=admin_auth, expected_status_code=expected_status_code)

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

    @classmethod
    def isrunnable(self, methodname, dict):
        return lmbr_aws_test_support.lmbr_aws_TestCase.isrunnable(methodname, dict)