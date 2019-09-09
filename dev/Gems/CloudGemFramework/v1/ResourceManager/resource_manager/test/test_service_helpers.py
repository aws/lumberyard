from resource_manager.test import lmbr_aws_test_support
from requests_aws4auth import AWS4Auth
import requests
import unittest


class Test_Service_Helpers(lmbr_aws_test_support.lmbr_aws_TestCase):

    def __init__(self, gem_name, deployment_name, region):
        self.gem_name = gem_name
        self.deployment_name = deployment_name
        self.region = region


    def get_service_url(self, path):
        base_url = self.get_stack_output(self.get_resource_group_stack_arn(self.deployment_name, self.gem_name), 'ServiceUrl')
        self.assertIsNotNone(base_url, "Missing ServiceUrl stack output.")
        return base_url + path


    def service_get(self, path, params=None, anonymous_auth=False, player_auth=False, player_credentials=None, admin_auth=False, expected_status_code=200):
        return self.__service_call(
            'GET', 
            path, 
            params=params, 
            anonymous_auth=anonymous_auth, 
            player_auth=player_auth,
            player_credentials=player_credentials, 
            admin_auth=admin_auth, 
            expected_status_code=expected_status_code
            )


    def service_put(self, path, body=None, anonymous_auth=False, player_auth=False, player_credentials=None, admin_auth=False, expected_status_code=200):
        return self.__service_call(
            'PUT', 
            path, 
            body=body, 
            anonymous_auth=anonymous_auth, 
            player_auth=player_auth,
            player_credentials=player_credentials, 
            admin_auth=admin_auth, 
            expected_status_code=expected_status_code
            )


    def service_post(self, path, body=None, anonymous_auth=False, player_auth=False, player_credentials=None, admin_auth=False, expected_status_code=200):
        return self.__service_call(
            'POST', 
            path, 
            body=body, 
            anonymous_auth=anonymous_auth, 
            player_auth=player_auth,
            player_credentials=player_credentials, 
            admin_auth=admin_auth, 
            expected_status_code=expected_status_code
            )


    def __service_call(self, method, path, body=None, params=None, anonymous_auth=False, player_auth=False, player_credentials=None, admin_auth=False, assumed_role=None, expected_status_code=200):
        url = self.get_service_url(path)
        if admin_auth:
            session_credentials = self.session.get_credentials().get_frozen_credentials()
            auth = AWS4Auth(session_credentials.access_key, session_credentials.secret_key, self.region, 'execute-api')
        elif anonymous_auth:
            creds = self.context['anonymous_aws_credentials']
            auth = AWS4Auth(creds['AccessKeyId'], creds['SecretKey'], self.region, 'execute-api', session_token=creds['SessionToken'])
        elif player_credentials:
            auth = AWS4Auth(player_credentials['AccessKeyId'], player_credentials['SecretKey'], self.region, 'execute-api', session_token=player_credentials['SessionToken'])
        elif player_auth:
            creds = self.context['aws_credentials']
            auth = AWS4Auth(creds['AccessKeyId'], creds['SecretKey'], self.region, 'execute-api', session_token=creds['SessionToken'])
        elif assumed_role:
            if assumed_role.startswith('Project'):
                role_arn = self.get_stack_resource_arn(self.get_project_stack_arn(), assumed_role)
            else:
                role_arn = self.get_stack_resource_arn(self.get_deployment_access_stack_arn(self.deployment_name), assumed_role)

            sts = self.session.client('sts')

            res = sts.assume_role(RoleArn=role_arn, RoleSessionName='CloudGemFrameworkTest')

            access_key = res['Credentials']['AccessKeyId']
            secret_key = res['Credentials']['SecretAccessKey']
            session_token = res['Credentials']['SessionToken']

            auth = AWS4Auth(access_key, secret_key, self.region, 'execute-api', session_token=session_token)
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