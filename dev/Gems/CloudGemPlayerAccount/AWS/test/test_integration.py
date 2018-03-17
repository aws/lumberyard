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

import resource_manager_common.constant as constant
import boto3
from botocore.exceptions import ClientError
from copy import deepcopy
import datetime
import json
from resource_manager.test import lmbr_aws_test_support
import os
from random import randint
from requests_aws4auth import AWS4Auth
import requests
import subprocess
import time
import uuid

REGION='us-east-1'

class IntegrationTest_CloudGemPlayerAccount_EndToEnd(lmbr_aws_test_support.lmbr_aws_TestCase):

    # Fails in cleanup to keep the deployment stack intact for the next test rerun.
    FAST_TEST_RERUN = False
    # Upload lambda code if the deployment stack is re-used.
    UPLOAD_LAMBDA_CODE_FOR_EXISTING_STACK = True

    GEM_NAME = 'CloudGemPlayerAccount'
    TEST_USERNAME_PREFIX = 'TestUser'
    TEST_PASSWORD = '$Password1'
    TEST_EMAIL = 'nobody-{}@example.com'.format(datetime.datetime.now().strftime('%Y-%m-%d-%H-%M-%S'))
    IDP_COGNITO = 'Cognito'

    COGNITO_ATTRIBUTES = set(["address", "birthdate", "email", "family_name", "gender", "given_name", "locale", "middle_name", "nickname", "phone_number", "picture", "profile", "website", "zoneinfo"])

    TEST_ATTRIBUTES = {
        'address': 'test_address2',
        'birthdate': 'birthdate2',
        'email': 'test_email@example.com',
        'family_name': 'test_family_name',
        'gender': 'test_gender',
        'given_name': 'test_given_name',
        'locale': 'test_locale',
        'middle_name': 'test_middle_name',
        'nickname': 'test_nickname',
        'phone_number': '+14325551212',
        'picture': 'test_picture',
        'website': 'test_website',
        'zoneinfo': 'test_zoneinfo'
    }

    maxDiff = None

    def __init__(self, *args, **kwargs):
        super(IntegrationTest_CloudGemPlayerAccount_EndToEnd, self).__init__(*args, **kwargs)

    def setUp(self):
        self.prepare_test_envionment("cloud_gem_player_account_test")

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
                self.lmbr_aws('function', 'upload-code', '--resource-group', 'CloudGemPlayerAccount')

        self.context['user_pool_id'] = self.get_stack_resource_physical_id(self.get_resource_group_stack_arn(self.TEST_DEPLOYMENT_NAME, self.GEM_NAME), 'PlayerUserPool')
        clients_response = self.aws_cognito_idp.list_user_pool_clients(UserPoolId=self.context['user_pool_id'], MaxResults=1)
        self.context['user_pool_client_id'] = clients_response['UserPoolClients'][0]['ClientId']
        self.context['identity_pool_id'] = self.get_stack_resource_physical_id(self.get_deployment_access_stack_arn(self.TEST_DEPLOYMENT_NAME), 'PlayerAccessIdentityPool')
        self.context['user_pool_provider_id'] = 'cognito-idp.' + self.TEST_REGION + '.amazonaws.com/' + self.context['user_pool_id']
        self.context['accounts_table'] = self.get_stack_resource_physical_id(self.get_resource_group_stack_arn(self.TEST_DEPLOYMENT_NAME, self.GEM_NAME), 'PlayerAccounts')
        self.context['test_run_id'] = datetime.datetime.now().strftime('%Y-%m-%d-%H-%M-%S')
        self.context['generator_sequence'] = 0

    # ------------------------------------------------ Create player account, get auth tokens, custom auth flow ------------------------------------------------

    def __010_create_test_user_and_sign_in(self):
        self.context['test_username'] = self.TEST_USERNAME_PREFIX + '-' + self.__generate_id()
        print 'Creating user {} in pool {}'.format(self.context['test_username'], self.context['user_pool_id'])

        self.__signup(self.context['test_username'], self.TEST_PASSWORD, self.TEST_EMAIL)
        authResult = self.__signin(self.context['test_username'], self.TEST_PASSWORD)
        self.context['auth_tokens'] = authResult
        self.context['account_id'] = self.__get_account_id(self.context['test_username'])
        print 'Account ID: {}'.format(self.context['account_id'])

        self.context['identity_id'] = self.__get_identity_id(authResult)
        self.context['aws_credentials'] = self.__get_aws_credentials(self.context['identity_id'], authResult)

    def __020_create_blacklist_user(self):
        self.context['test_username_blacklisted_true'] = self.TEST_USERNAME_PREFIX + '-' + self.__generate_id()
        print 'Creating user {} in pool {}'.format(self.context['test_username_blacklisted_true'], self.context['user_pool_id'])

        self.__signup(self.context['test_username_blacklisted_true'], self.TEST_PASSWORD, self.TEST_EMAIL)
        authResult = self.__signin(self.context['test_username_blacklisted_true'], self.TEST_PASSWORD)
        self.context['account_id_blacklisted_true'] = self.__get_account_id(self.context['test_username_blacklisted_true'])
        print 'Account ID: {}'.format(self.context['account_id_blacklisted_true'])

        self.context['identity_id_blacklisted_true'] = self.__get_identity_id(authResult)
        self.context['aws_credentials_blacklisted_true'] = self.__get_aws_credentials(self.context['identity_id_blacklisted_true'], authResult)

        # Blacklist the user
        self.__service_put('/admin/accounts/{}'.format(self.context['account_id_blacklisted_true']), {'AccountBlacklisted': True}, admin_auth=True)

        # Existing access tokens should now be invalid.
        with self.assertRaisesRegexp(ClientError, 'NotAuthorizedException') as context_manager:
            self.aws_cognito_idp.get_user(AccessToken=authResult['AccessToken'])

    def __030_create_no_longer_blacklisted_user(self):
        self.context['test_username_blacklisted_false'] = self.TEST_USERNAME_PREFIX + '-' + self.__generate_id()
        print 'Creating user {} in pool {}'.format(self.context['test_username_blacklisted_false'], self.context['user_pool_id'])

        self.__signup(self.context['test_username_blacklisted_false'], self.TEST_PASSWORD, self.TEST_EMAIL)
        authResult = self.__signin(self.context['test_username_blacklisted_false'], self.TEST_PASSWORD)
        self.context['account_id_blacklisted_false'] = self.__get_account_id(self.context['test_username_blacklisted_false'])
        print 'Account ID: {}'.format(self.context['account_id_blacklisted_false'])

        # Blacklist/unblacklist the user
        self.__service_put('/admin/accounts/{}'.format(self.context['account_id_blacklisted_false']), {'AccountBlacklisted': True}, admin_auth=True)
        self.__service_put('/admin/accounts/{}'.format(self.context['account_id_blacklisted_false']), {'AccountBlacklisted': False}, admin_auth=True)

        # Sign in after unblacklist
        authResult = self.__signin(self.context['test_username_blacklisted_false'], self.TEST_PASSWORD)
        self.context['identity_id_blacklisted_false'] = self.__get_identity_id(authResult)
        self.context['aws_credentials_blacklisted_false'] = self.__get_aws_credentials(self.context['identity_id_blacklisted_false'], authResult)

    def __040_custom_auth_flow_fails_when_wrong_password(self):
        with self.assertRaisesRegexp(ClientError, 'Authentication failed') as context_manager:
            self.__signin(self.context['test_username'], 'InvalidPassword')

    def __050_get_anonymous_aws_credentials(self):
        identity_response = self.aws_cognito_identity.get_id(IdentityPoolId=self.context['identity_pool_id'], Logins={})
        response = self.aws_cognito_identity.get_credentials_for_identity(IdentityId=identity_response['IdentityId'], Logins={})
        self.context['anonymous_aws_credentials'] = {
            'AccessKeyId': response['Credentials']['AccessKeyId'],
            'SecretKey': response['Credentials']['SecretKey'],
            'SessionToken': response['Credentials']['SessionToken']
        }

    def __060_custom_auth_flow_rejects_blacklisted_accounts(self):
        with self.assertRaisesRegexp(ClientError, 'blacklist') as context_manager:
            self.__signin(self.context['test_username_blacklisted_true'], self.TEST_PASSWORD)

    def __070_custom_auth_flow_allows_no_longer_blacklisted(self):
        self.__signin(self.context['test_username_blacklisted_false'], self.TEST_PASSWORD)

    def __080_custom_auth_flow_with_force_change_password(self):
        username = self.TEST_USERNAME_PREFIX + '-' + self.__generate_id()

        # admin_create_user always creates users with the force change password status.
        self.aws_cognito_idp.admin_create_user(
            UserPoolId=self.context['user_pool_id'],
            Username=username,
            UserAttributes=[{'Name': 'email', 'Value': self.TEST_ATTRIBUTES['email']}],
            MessageAction='SUPPRESS',
            TemporaryPassword=self.TEST_PASSWORD
        )

        challenge_response = self.aws_cognito_idp.initiate_auth(
            AuthFlow='CUSTOM_AUTH',
            AuthParameters={'USERNAME': username},
            ClientId=self.context['user_pool_client_id']
        )
        self.assertEqual('ForceChangePassword', challenge_response.get('ChallengeParameters').get('type'))

        new_password = self.TEST_PASSWORD + '2'
        auth_response = self.aws_cognito_idp.respond_to_auth_challenge(
            ClientId=self.context['user_pool_client_id'],
            ChallengeName='CUSTOM_CHALLENGE',
            Session=challenge_response['Session'],
            ChallengeResponses={'USERNAME': username, 'ANSWER': json.dumps({'password': self.TEST_PASSWORD, 'newPassword': new_password})}
        )
        self.assertIsNotNone(auth_response.get('AuthenticationResult').get('AccessToken'))

        # Check that the new password is valid.
        self.__signin(username, new_password)

    # ------------------------------------------------ Service status ------------------------------------------------

    def __100_call_status_with_player_credentials(self):
        result = self.__service_get('/service/status', player_auth=True)
        status = result.get('status', None)
        self.assertIsNotNone(status, "Missing 'status' in result: {}".format(result))
        self.assertEqual(status, 'online')

    def __105_call_status_with_admin_credentials(self):
        result = self.__service_get('/service/status', admin_auth=True)
        status = result.get('status', None)
        self.assertIsNotNone(status, "Missing 'status' in result: {}".format(result))
        self.assertEqual(status, 'online')

    def __110_call_status_with_no_credentials(self):
        self.__service_get('/service/status', expected_status_code=403)

    # ------------------------------------------------ Player APIs ------------------------------------------------

    def __200_call_get_account(self):
        result = self.__service_get('/account', player_auth=True)
        self.assertIn('AccountId', result)
        self.assertNotIn('CognitoIdentityId', result)
        self.assertNotIn('CognitoUsername', result)
        self.assertNotIn('IndexedPlayerName', result)
        self.assertNotIn('PlayerNameSortKey', result)

    def __205_call_get_account_with_admin_credentials(self):
        self.__service_get('/account', admin_auth=True, expected_status_code=403)

    def __210_call_get_account_with_no_credentials(self):
        self.__service_get('/account', expected_status_code=403)

    def __215_call_get_account_for_blacklisted_user(self):
        self.__service_get('/account', player_credentials=self.context['aws_credentials_blacklisted_true'], expected_status_code=403)

    def __220_call_get_account_for_no_longer_blacklisted_user(self):
        self.__service_get('/account', player_credentials=self.context['aws_credentials_blacklisted_false'])

    def __225_call_put_account(self):
        name = 'TestName' + str(randint(0,1000))

        result = self.__service_put('/account', {'PlayerName': name}, player_auth=True)
        self.assertIn('AccountId', result)
        self.assertNotIn('CognitoIdentityId', result)
        self.assertNotIn('CognitoUsername', result)
        self.assertNotIn('IndexedPlayerName', result)
        self.assertNotIn('PlayerNameSortKey', result)
        self.assertEquals(result.get('PlayerName', None), name)

        result = self.__service_get('/account', player_auth=True)
        self.assertIn('AccountId', result)
        self.assertNotIn('CognitoIdentityId', result)
        self.assertNotIn('CognitoUsername', result)
        self.assertNotIn('IndexedPlayerName', result)
        self.assertNotIn('PlayerNameSortKey', result)
        self.assertEquals(result.get('PlayerName', None), name)

    def __230_call_put_account_to_create_anonymous_account(self):
        name = 'TestName' + str(randint(0,1000))

        result = self.__service_put('/account', {'PlayerName': name}, anonymous_auth=True)
        self.assertIn('AccountId', result)
        self.assertNotIn('CognitoIdentityId', result)
        self.assertNotIn('CognitoUsername', result)
        self.assertNotIn('IndexedPlayerName', result)
        self.assertNotIn('PlayerNameSortKey', result)
        self.assertEquals(result.get('PlayerName', None), name)

        result = self.__service_get('/account', anonymous_auth=True)
        self.assertIn('AccountId', result)
        self.assertNotIn('CognitoIdentityId', result)
        self.assertNotIn('CognitoUsername', result)
        self.assertNotIn('IndexedPlayerName', result)
        self.assertNotIn('PlayerNameSortKey', result)
        self.assertEquals(result.get('PlayerName', None), name)

    def __235_call_put_account_with_admin_credentials(self):
        self.__service_put('/account', {'PlayerName': 'TestName'}, admin_auth=True, expected_status_code=403)

    def __240_call_put_account_with_no_credentials(self):
        self.__service_put('/account', {'PlayerName': 'TestName'}, expected_status_code=403)

    def __245_call_put_account_with_long_player_name(self):
        self.__service_put('/account', {'PlayerName': 'Test' * 100}, player_auth=True, expected_status_code=400)

    def __250_call_put_account_for_blacklisted_user(self):
        self.__service_put('/account', {'PlayerName': 'TestName2'}, player_credentials=self.context['aws_credentials_blacklisted_true'], expected_status_code=403)

    def __255_call_put_account_for_no_longer_blacklisted_user(self):
        self.__service_put('/account', {'PlayerName': 'TestName3'}, player_credentials=self.context['aws_credentials_blacklisted_false'], player_auth=True)

    # ------------------------------------------------ Admin Account APIs ------------------------------------------------

    def __300_call_admin_get_account(self):
        result = self.__service_get('/admin/accounts/{}'.format(self.context['account_id']), admin_auth=True)
        self.__assert_username_is_correct(result)
        self.__assert_cognito_user_equals(result.get('IdentityProviders', {}).get(self.IDP_COGNITO, {}), {'email': self.TEST_EMAIL}, self.context['test_username'])
        self.assertEquals(result.get('AccountId'), self.context['account_id'])
        self.assertFalse(result.get('AccountBlacklisted'))
        self.assertEquals(result.get('CognitoIdentityId'), self.context['identity_id'])

    def __305_call_admin_get_account_for_non_existent_account(self):
        self.__service_get('/admin/accounts/non-existent', admin_auth=True, expected_status_code=400)

    def __310_call_admin_get_account_with_player_credentials(self):
        self.__service_get('/admin/accounts/{}'.format(self.context['account_id']), player_auth=True, expected_status_code=403)

    def __315_call_admin_get_account_with_no_credentials(self):
        self.__service_get('/admin/accounts/{}'.format(self.context['account_id']), expected_status_code=403)

    def __320_call_admin_get_account_without_cognito(self):
        accountId = self.__service_post('/admin/accounts', {'PlayerName': "test"}, admin_auth=True).get('AccountId')
        result = self.__service_get('/admin/accounts/{}'.format(accountId), admin_auth=True)
        self.assertNotIn('CognitoUsername', result)
        self.assertNotIn('IdentityProviders', result)
        self.assertEquals(result.get('AccountId'), accountId)
        self.assertFalse(result.get('AccountBlacklisted'))
        self.assertNotIn('CognitoIdentityId', result)


    def __325_call_admin_put_account(self):
        name = 'TestName' + str(randint(0,1000))

        result = self.__service_put('/admin/accounts/{}'.format(self.context['account_id']), {'AccountBlacklisted': True, 'PlayerName': name}, admin_auth=True)
        self.__assert_username_is_correct(result)
        self.__assert_cognito_user_equals(result.get('IdentityProviders', {}).get(self.IDP_COGNITO, {}), {'email': self.TEST_EMAIL}, self.context['test_username'])
        self.assertEquals(result.get('AccountId'), self.context['account_id'])
        self.assertTrue(result.get('AccountBlacklisted'))
        self.assertEquals(result.get('CognitoIdentityId'), self.context['identity_id'])
        self.assertEquals(result.get('PlayerName'), name)

        result = self.__service_get('/admin/accounts/{}'.format(self.context['account_id']), admin_auth=True)
        self.__assert_username_is_correct(result)
        self.__assert_cognito_user_equals(result.get('IdentityProviders', {}).get(self.IDP_COGNITO, {}), {'email': self.TEST_EMAIL}, self.context['test_username'])
        self.assertEquals(result.get('AccountId'), self.context['account_id'])
        self.assertTrue(result.get('AccountBlacklisted'))
        self.assertEquals(result.get('CognitoIdentityId'), self.context['identity_id'])
        self.assertEquals(result.get('PlayerName'), name)

    def __330_call_admin_put_account_blacklist(self):
        name = 'TestName' + str(randint(0,1000))

        # Blacklist stays true
        result = self.__service_put('/admin/accounts/{}'.format(self.context['account_id']), {'PlayerName': name}, admin_auth=True)
        self.assertTrue(result.get('AccountBlacklisted'))
        self.assertEquals(result.get('PlayerName'), name)

        result = self.__service_get('/admin/accounts/{}'.format(self.context['account_id']), admin_auth=True)
        self.assertTrue(result.get('AccountBlacklisted'))
        self.assertEquals(result.get('PlayerName'), name)

        # Change blacklist to false, no player name given.
        result = self.__service_put('/admin/accounts/{}'.format(self.context['account_id']), {'AccountBlacklisted': False}, admin_auth=True)
        self.assertFalse(result.get('AccountBlacklisted'))
        self.assertEquals(result.get('PlayerName'), name)

        result = self.__service_get('/admin/accounts/{}'.format(self.context['account_id']), admin_auth=True)
        self.assertFalse(result.get('AccountBlacklisted'))
        self.assertEquals(result.get('PlayerName'), name)

    def __335_call_admin_put_account_with_player_credentials(self):
        self.__service_put('/admin/accounts/{}'.format(self.context['account_id']), {'PlayerName': 'TestName'}, player_auth=True, expected_status_code=403)

    def __340_call_admin_put_account_with_no_credentials(self):
        self.__service_put('/admin/accounts/{}'.format(self.context['account_id']), {'PlayerName': 'TestName'}, expected_status_code=403)

    def __345_call_admin_put_with_empty_cognito_request(self):
        request = {
            'IdentityProviders': {
                self.IDP_COGNITO: {
                }
            }
        }
        result = self.__service_put('/admin/accounts/{}'.format(self.context['account_id']), request, admin_auth=True)
        self.__assert_username_is_correct(result)
        self.__assert_cognito_user_equals(result.get('IdentityProviders', {}).get(self.IDP_COGNITO, {}), {'email': self.TEST_EMAIL}, self.context['test_username'])
        self.assertEquals(result.get('AccountId'), self.context['account_id'])
        self.assertEquals(result.get('CognitoIdentityId'), self.context['identity_id'])

    def __350_call_admin_put_identity_provider_partial_update(self):
        request = {
            'IdentityProviders': {
                self.IDP_COGNITO: {
                    'address': 'test_address'
                }
            }
        }
        result = self.__service_put('/admin/accounts/{}'.format(self.context['account_id']), request, admin_auth=True)
        self.__assert_username_is_correct(result)
        expectedAttributes = {
            'email': self.TEST_EMAIL,
            'address': request['IdentityProviders'][self.IDP_COGNITO]['address']
        }
        self.__assert_cognito_user_equals(result.get('IdentityProviders', {}).get(self.IDP_COGNITO, {}), expectedAttributes, self.context['test_username'])
        self.assertEquals(result.get('AccountId'), self.context['account_id'])
        self.assertEquals(result.get('CognitoIdentityId'), self.context['identity_id'])

    def __355_call_admin_put_identity_provider_full_update(self):
        request = {
            'IdentityProviders': {
                self.IDP_COGNITO: self.TEST_ATTRIBUTES
            }
        }
        result = self.__service_put('/admin/accounts/{}'.format(self.context['account_id']), request, admin_auth=True)
        self.__assert_username_is_correct(result)
        self.__assert_cognito_user_equals(result.get('IdentityProviders', {}).get(self.IDP_COGNITO, {}), self.TEST_ATTRIBUTES, self.context['test_username'])
        self.assertEquals(result.get('AccountId'), self.context['account_id'])
        self.assertEquals(result.get('CognitoIdentityId'), self.context['identity_id'])

        result = self.__service_get('/admin/accounts/{}'.format(self.context['account_id']), admin_auth=True)
        self.__assert_username_is_correct(result)
        self.__assert_cognito_user_equals(result.get('IdentityProviders', {}).get(self.IDP_COGNITO, {}), self.TEST_ATTRIBUTES, self.context['test_username'])
        self.assertEquals(result.get('AccountId'), self.context['account_id'])
        self.assertEquals(result.get('CognitoIdentityId'), self.context['identity_id'])

    def __360_call_admin_put_identity_provider_fail_and_undo_playername(self):
        account = self.__create_account({'PlayerName': 'OriginalPlayerName', 'CognitoUsername': 'invalid'}, None)
        request = {
            'PlayerName': 'PlayerNameInBadRequest',
            'IdentityProviders': {
                self.IDP_COGNITO: {'address': 'test'}
            }
        }
        self.__service_put('/admin/accounts/{}'.format(account['AccountId']), request, admin_auth=True, expected_status_code=400)

        result = self.__service_get('/admin/accounts/{}'.format(account['AccountId']), admin_auth=True)
        self.assertEquals(result.get('PlayerName'), 'OriginalPlayerName')

    def __365_call_admin_put_identity_provider_fail_and_delete_playername(self):
        account = self.__create_account({'CognitoUsername': 'invalid'}, None)
        request = {
            'IdentityProviders': {
                self.IDP_COGNITO: {'address': 'test'}
            }
        }
        self.__service_put('/admin/accounts/{}'.format(account['AccountId']), request, admin_auth=True, expected_status_code=400)

        result = self.__service_get('/admin/accounts/{}'.format(account['AccountId']), admin_auth=True)
        self.assertIsNone(result.get('PlayerName'))

    def __370_call_admin_put_account_with_long_player_name(self):
        self.__service_put('/admin/accounts/{}'.format(self.context['account_id']), {'PlayerName': 'Test' * 100}, admin_auth=True, expected_status_code=400)

    def __375_call_admin_put_account_with_long_attributes(self):
        long_value = 'x' * 3000
        for key,value in self.TEST_ATTRIBUTES.iteritems():
            request = {
                'IdentityProviders': {
                    self.IDP_COGNITO: {key: long_value}
                }
            }
            result = self.__service_put('/admin/accounts/{}'.format(self.context['account_id']), request, admin_auth=True, expected_status_code=400)

    def __380_call_admin_create_account_without_user(self):
        name = 'TestName' + str(randint(0,1000))

        result = self.__service_post('/admin/accounts', {'PlayerName': name}, admin_auth=True)
        self.assertNotIn('CognitoUsername', result)
        self.assertNotIn('IdentityProviders', result)
        self.assertIsNotNone(result.get('AccountId'))
        self.assertFalse(result.get('AccountBlacklisted'))
        self.assertNotIn('CognitoIdentityId', result)
        self.assertEquals(result.get('PlayerName'), name)
        account_id = result.get('AccountId')

        result = self.__service_get('/admin/accounts/{}'.format(account_id), admin_auth=True)
        self.assertNotIn('CognitoUsername', result)
        self.assertNotIn('IdentityProviders', result)
        self.assertEquals(result.get('AccountId'), account_id)
        self.assertFalse(result.get('AccountBlacklisted'))
        self.assertNotIn('CognitoIdentityId', result)
        self.assertEquals(result.get('PlayerName'), name)

    def __385_call_admin_create_account_with_user(self):
        name = 'TestName' + str(randint(0,1000))
        username = self.TEST_USERNAME_PREFIX + '-' + self.__generate_id()
        request = {
            'CognitoUsername': username,
            'PlayerName': name,
            'IdentityProviders': {
                self.IDP_COGNITO: self.TEST_ATTRIBUTES
            }
        }

        result = self.__service_post('/admin/accounts', request, admin_auth=True)
        self.assertEquals(result.get('CognitoUsername'), username)
        self.__assert_cognito_user_equals(result.get('IdentityProviders', {}).get(self.IDP_COGNITO, {}), self.TEST_ATTRIBUTES, username)
        self.assertIsNotNone(result.get('AccountId'))
        self.assertFalse(result.get('AccountBlacklisted'))
        self.assertNotIn('CognitoIdentityId', result)
        self.assertEquals(result.get('PlayerName'), name)
        account_id = result.get('AccountId')

        result = self.__service_get('/admin/accounts/{}'.format(account_id), admin_auth=True)
        self.assertEquals(result.get('CognitoUsername'), username)
        self.__assert_cognito_user_equals(result.get('IdentityProviders', {}).get(self.IDP_COGNITO, {}), self.TEST_ATTRIBUTES, username)
        self.assertEquals(result.get('AccountId'), account_id)
        self.assertFalse(result.get('AccountBlacklisted'))
        self.assertNotIn('CognitoIdentityId', result)
        self.assertEquals(result.get('PlayerName'), name)

    def __390_call_admin_create_account_with_existing_user(self):
        name = 'TestName' + str(randint(0,1000))
        username = self.TEST_USERNAME_PREFIX + '-' + self.__generate_id()
        request = {
            'CognitoUsername': username,
            'PlayerName': name,
            'IdentityProviders': {
                self.IDP_COGNITO: self.TEST_ATTRIBUTES
            }
        }

        self.aws_cognito_idp.admin_create_user(
            UserPoolId=self.context['user_pool_id'],
            Username=username,
            UserAttributes=[{'Name': 'email', 'Value': self.TEST_ATTRIBUTES['email']}],
            MessageAction='SUPPRESS'
        )

        response = self.__service_post('/admin/accounts', request, admin_auth=True, expected_status_code=400)
        self.assertRegexpMatches(response, 'username already exists')

    def __395_cleanup_user_attributes(self):
        self.aws_cognito_idp.admin_delete_user_attributes(
            UserPoolId = self.context['user_pool_id'],
            Username = self.context['test_username'],
            UserAttributeNames = ["address", "birthdate", "family_name", "gender", "given_name", "locale", "middle_name", "nickname", "phone_number", "picture", "profile", "website", "zoneinfo"]
        )
        self.aws_cognito_idp.admin_update_user_attributes(
            UserPoolId = self.context['user_pool_id'],
            Username = self.context['test_username'],
            UserAttributes=[
                {
                    'Name': 'email',
                    'Value': self.TEST_EMAIL
                }
            ]
        )

    # ------------------------------------------------ Admin Identity Provider User APIs ------------------------------------------------

    def __400_call_admin_get_identity_provider_user(self):
        result = self.__service_get('/admin/identityProviders/{}/users/{}'.format(self.IDP_COGNITO, self.context['test_username']), admin_auth=True)
        self.__assert_cognito_user_equals(result, {'email': self.TEST_EMAIL}, self.context['test_username'])

    def __405_call_admin_get_identity_provider_user_for_non_exitent_account(self):
        self.__service_get('/admin/identityProviders/{}/users/{}'.format(self.IDP_COGNITO, 'non-existent'), admin_auth=True, expected_status_code=400)

    def __410_call_admin_get_identity_provider_user_for_invalid_provider(self):
        self.__service_get('/admin/identityProviders/{}/users/{}'.format('invalid', self.context['test_username']), admin_auth=True, expected_status_code=404)

    def __415_call_admin_get_identity_provider_user_without_identity(self):
        username = self.TEST_USERNAME_PREFIX + '-' + self.__generate_id()
        print 'Creating user {} in pool {}'.format(username, self.context['user_pool_id'])
        self.aws_cognito_idp.admin_create_user(
            UserPoolId=self.context['user_pool_id'],
            Username=username,
            UserAttributes=[{'Name': 'email', 'Value': self.TEST_EMAIL}],
            MessageAction='SUPPRESS'
        )
        result = self.__service_get('/admin/identityProviders/{}/users/{}'.format(self.IDP_COGNITO, username), admin_auth=True)
        self.__assert_cognito_user_equals(result, {'email': self.TEST_EMAIL}, username)

    def __420_call_admin_get_identity_provider_user_with_player_credentials(self):
        self.__service_get('/admin/identityProviders/{}/users/{}'.format(self.IDP_COGNITO, self.context['test_username']), player_auth=True, expected_status_code=403)

    def __425_call_admin_get_identity_provider_user_with_no_credentials(self):
        self.__service_get('/admin/identityProviders/{}/users/{}'.format(self.IDP_COGNITO, self.context['test_username']), expected_status_code=403)


    def __430_call_admin_put_identity_provider_user_empty_request(self):
        request = {
        }
        result = self.__service_put('/admin/identityProviders/{}/users/{}'.format(self.IDP_COGNITO, self.context['test_username']), request, admin_auth=True)
        self.__assert_cognito_user_equals(result, {'email': self.TEST_EMAIL}, self.context['test_username'])
        self.assertFalse('address' in result)

    def __435_call_admin_put_identity_provider_user_partial_update(self):
        request = {
            'address': 'test_address'
        }
        result = self.__service_put('/admin/identityProviders/{}/users/{}'.format(self.IDP_COGNITO, self.context['test_username']), request, admin_auth=True)
        self.__assert_cognito_user_equals(result, {'email': self.TEST_EMAIL, 'address': request['address']}, self.context['test_username'])

    def __440_call_admin_put_identity_provider_user_full_update(self):
        request = self.TEST_ATTRIBUTES
        result = self.__service_put('/admin/identityProviders/{}/users/{}'.format(self.IDP_COGNITO, self.context['test_username']), request, admin_auth=True)
        self.__assert_cognito_user_equals(result, request, self.context['test_username'])

        result = self.__service_get('/admin/identityProviders/{}/users/{}'.format(self.IDP_COGNITO, self.context['test_username']), admin_auth=True)
        self.__assert_cognito_user_equals(result, request, self.context['test_username'])

    def __445_call_admin_put_identity_provider_user_with_player_credentials(self):
        self.__service_put('/admin/identityProviders/{}/users/{}'.format(self.IDP_COGNITO, self.context['test_username']), {}, player_auth=True, expected_status_code=403)

    def __450_call_admin_put_identity_provider_user_with_no_credentials(self):
        self.__service_put('/admin/identityProviders/{}/users/{}'.format(self.IDP_COGNITO, self.context['test_username']), {}, expected_status_code=403)

    def __455_call_admin_confirm_signup(self):
        username = self.TEST_USERNAME_PREFIX + '-' + self.__generate_id()
        print 'Creating user {} in pool {}'.format(username, self.context['user_pool_id'])
        self.aws_cognito_idp.sign_up(
            ClientId=self.context['user_pool_client_id'],
            Username=username,
            Password=self.TEST_PASSWORD,
            UserAttributes=[ {'Name': 'email', 'Value': self.TEST_EMAIL} ]
        )

        result = self.__service_get('/admin/identityProviders/{}/users/{}'.format(self.IDP_COGNITO, username), admin_auth=True)
        self.assertEqual('UNCONFIRMED', result.get('status'))

        self.__service_post('/admin/identityProviders/{}/users/{}/confirmSignUp'.format(self.IDP_COGNITO, username), {}, admin_auth=True)

        result = self.__service_get('/admin/identityProviders/{}/users/{}'.format(self.IDP_COGNITO, username), admin_auth=True)
        self.assertEqual('CONFIRMED', result.get('status'))

    def __460_call_admin_confirm_signup_with_player_credentials(self):
        self.__service_put('/admin/identityProviders/{}/users/{}/confirmSignUp'.format(self.IDP_COGNITO, self.context['test_username']), {}, player_auth=True, expected_status_code=403)

    def __465_call_admin_confirm_signup_with_no_credentials(self):
        self.__service_put('/admin/identityProviders/{}/users/{}/confirmSignUp'.format(self.IDP_COGNITO, self.context['test_username']), {}, expected_status_code=403)

    def __470_call_admin_reset_user_password(self):
        # Reset user password will only work on a user with a verified email address.
        # This test does not support reading email, so instead this test checks that it's not a 500 response.
        self.__service_post('/admin/identityProviders/{}/users/{}/resetUserPassword'.format(self.IDP_COGNITO, self.context['test_username']), {}, admin_auth=True, expected_status_code=400)

    def __475_call_admin_reset_user_password_with_player_credentials(self):
        self.__service_put('/admin/identityProviders/{}/users/{}/resetUserPassword'.format(self.IDP_COGNITO, self.context['test_username']), {}, player_auth=True, expected_status_code=403)

    def __480_call_admin_reset_user_password_with_no_credentials(self):
        self.__service_put('/admin/identityProviders/{}/users/{}/resetUserPassword'.format(self.IDP_COGNITO, self.context['test_username']), {}, expected_status_code=403)

    # ------------------------------------------------ Admin Identity Provider APIs ------------------------------------------------

    def __500_call_admin_get_identity_provider(self):
        result = self.__service_get('/admin/identityProviders/{}'.format(self.IDP_COGNITO), admin_auth=True)
        self.assertIn('EstimatedNumberOfUsers', result)
        self.assertTrue(result['EstimatedNumberOfUsers'] > 0)

    def __505_call_admin_get_identity_provider_for_invalid_provider(self):
        self.__service_get('/admin/identityProviders/{}'.format('invalid'), admin_auth=True, expected_status_code=400)

    def __510_call_admin_get_identity_providerwith_player_credentials(self):
        self.__service_get('/admin/identityProviders/{}'.format(self.IDP_COGNITO), player_auth=True, expected_status_code=403)

    def __515_call_admin_get_identity_provider_with_no_credentials(self):
        self.__service_get('/admin/identityProviders/{}'.format(self.IDP_COGNITO), expected_status_code=403)

    # ------------------------------------------------ Admin Search API ------------------------------------------------

    def __600_setup_search_test_data(self):
        self.__delete_all_users()
        self.__delete_all_accounts()

        # Add test accounts
        testAccounts = {}

        testAccounts['accountWithIdentity1'] = self.__create_account({'AccountId': 'accountWithIdentity1', 'CognitoIdentityId': 'identity1', 'AccountBlacklisted': True}, None)
        testAccounts['accountWithIdentity2'] = self.__create_account({'AccountId': 'accountWithIdentity2', 'CognitoIdentityId': 'identity2'}, None)
        testAccounts['accountWithInvalidUser1'] = self.__create_account({'AccountId': 'accountWithInvalidUser1', 'CognitoUsername': 'invalid1'}, None)
        testAccounts['accountWithInvalidUser2'] = self.__create_account({'AccountId': 'accountWithInvalidUser2', 'CognitoUsername': 'invalid2'}, None)
        testAccounts['accountWithPlayerName1'] = self.__create_account({'AccountId': 'accountWithPlayerName1', 'PlayerName': 'TestName1'}, None)
        testAccounts['accountWithPlayerName2'] = self.__create_account({'AccountId': 'accountWithPlayerName2', 'PlayerName': 'TestName2'}, None)
        testAccounts['accountWithUser1'] = self.__create_account({'AccountId': 'accountWithUser1'}, {'email': self.TEST_EMAIL})
        testAccounts['accountWithUser2'] = self.__create_account({'AccountId': 'accountWithUser2'}, {'email': self.TEST_EMAIL})
        testAccounts['duplicateUsername1'] = self.__create_account({'AccountId': 'duplicateUsername1', 'CognitoUsername': 'duplicate'}, {'email': self.TEST_EMAIL})
        testAccounts['duplicateUsername2'] = self.__create_account({'AccountId': 'duplicateUsername2', 'CognitoUsername': 'duplicate'}, None)
        testAccounts['duplicateUsername2']['IdentityProviders'] = deepcopy(testAccounts['duplicateUsername1']['IdentityProviders'])
        testAccounts['emptyAccount'] = self.__create_account({'AccountId': 'emptyAccount'}, None)
        testAccounts['user1'] = self.__create_account(None, {'email': self.TEST_EMAIL})
        testAccounts['user2'] = self.__create_account(None, {'email': self.TEST_EMAIL})

        self.context['testAccounts'] = testAccounts

        time.sleep(10) # wait for cloud state to stabalize

    def __605_call_admin_account_search_no_parameters(self):
        result = self.__service_get('/admin/accountSearch', admin_auth=True)
        self.__assert_search_results_equal(result, [account for name, account in self.context['testAccounts'].iteritems()])

    def __610_call_admin_account_search_with_start_name(self):
        result = self.__service_get('/admin/accountSearch', {'StartPlayerName': 'a'}, admin_auth=True)
        self.__assert_search_results_equal(result, [account for name, account in self.context['testAccounts'].iteritems() if 'PlayerName' in account])

    def __615_call_admin_account_search_with_username(self):
        username = self.context['testAccounts']['accountWithUser1']['CognitoUsername']
        result = self.__service_get('/admin/accountSearch', {'CognitoUsername': username}, admin_auth=True)
        self.__assert_search_results_equal(result, [account for name, account in self.context['testAccounts'].iteritems() if account.get('CognitoUsername') == username])

    def __616_call_admin_account_search_with_username_prefix(self):
        username = self.context['testAccounts']['accountWithUser1']['CognitoUsername'][:4]
        result = self.__service_get('/admin/accountSearch', {'CognitoUsername': username}, admin_auth=True)
        self.__assert_search_results_equal(result, [account for name, account in self.context['testAccounts'].iteritems() if account.get('IdentityProviders', {}).get(self.IDP_COGNITO, {}).get('username', '').startswith(username)])

    def __620_call_admin_account_search_with_invalid_username(self):
        username = self.context['testAccounts']['accountWithInvalidUser1']['CognitoUsername']
        result = self.__service_get('/admin/accountSearch', {'CognitoUsername': username}, admin_auth=True)
        self.__assert_search_results_equal(result, [])

    def __625_call_admin_account_search_with_duplicate_username(self):
        username = self.context['testAccounts']['duplicateUsername1']['CognitoUsername']
        result = self.__service_get('/admin/accountSearch', {'CognitoUsername': username}, admin_auth=True)
        self.__assert_search_results_equal(result, [self.context['testAccounts']['duplicateUsername1']])

    def __630_call_admin_account_search_with_email(self):
        # The double quote should be stripped by the lambda.
        result = self.__service_get('/admin/accountSearch', {'Email': self.TEST_EMAIL + '"'}, admin_auth=True)
        self.__assert_search_results_equal(result, [account for name, account in self.context['testAccounts'].iteritems()
            if account.get('IdentityProviders', {}).get(self.IDP_COGNITO, {}).get('email') == self.TEST_EMAIL and name != 'duplicateUsername2'])

    def __631_call_admin_account_search_with_email_prefix(self):
        result = self.__service_get('/admin/accountSearch', {'Email': 'nobody'}, admin_auth=True)
        self.__assert_search_results_equal(result, [account for name, account in self.context['testAccounts'].iteritems()
            if account.get('IdentityProviders', {}).get(self.IDP_COGNITO, {}).get('email') == self.TEST_EMAIL and name != 'duplicateUsername2'])

    def __635_call_admin_account_search_with_identity_id(self):
        result = self.__service_get('/admin/accountSearch', {'CognitoIdentityId': 'identity1'}, admin_auth=True)
        self.__assert_search_results_equal(result, [account for name, account in self.context['testAccounts'].iteritems() if account.get('CognitoIdentityId') == 'identity1'])

    def __640_call_admin_account_search_with_player_credentials(self):
        self.__service_get('/admin/accountSearch', player_auth=True, expected_status_code=403)

    def __645_call_admin_account_search_with_no_credentials(self):
        self.__service_get('/admin/accountSearch', expected_status_code=403)

    def __650_create_pagination_test_accounts(self):
        self.__delete_all_accounts()

        for index in range(50):
            self.__create_account({'AccountId': 'testAccount{}'.format(index), 'PlayerName': 'Test{}'.format(str(index).rjust(2,'0'))}, None)

    def __655_player_name_search_pagination(self):
        def check_page_results(results, page_size, page_offset, has_next, has_previous):
            print results
            self.assertEqual('next' in results, has_next)
            self.assertEqual('previous' in results, has_previous)
            self.assertEqual(len(results['Accounts']), page_size)
            for index in range(page_size):
                self.assertEqual(results['Accounts'][index]['PlayerName'], 'Test{}'.format(str(index + page_offset).rjust(2,'0')))

        # Get the first page.
        result1 = self.__service_get('/admin/accountSearch', {'StartPlayerName': ' '}, admin_auth=True)
        check_page_results(result1, 20, 0, has_next=True, has_previous=True)

        # Get the next two pages using next tokens.
        result2 = self.__service_get('/admin/accountSearch', {'PageToken': result1['next']}, admin_auth=True)
        check_page_results(result2, 20, 20, has_next=True, has_previous=True)

        result3 = self.__service_get('/admin/accountSearch', {'PageToken': result2['next']}, admin_auth=True)
        check_page_results(result3, 10, 40, has_next=False, has_previous=True)

        # Check the previous tokens.
        result2_previous = self.__service_get('/admin/accountSearch', {'PageToken': result2['previous']}, admin_auth=True)
        check_page_results(result2_previous, 20, 0, has_next=True, has_previous=False)

        result3_previous = self.__service_get('/admin/accountSearch', {'PageToken': result3['previous']}, admin_auth=True)
        check_page_results(result3_previous, 20, 20, has_next=True, has_previous=True)

    # ------------------------------------------------ C++ ------------------------------------------------

    def __900_run_cpp_tests(self):

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
                '--integ',
                '--only', 'Gem.CloudGemPlayerAccount.fd4ea4ff80a64bb9a90e55b46e9539ef.v0.1.0.dll',
                '--dir', os.environ.get("TEST_BUILD_DIR", "Bin64vc140.Debug.Test")
            ]
            print 'EXECUTING', ' '.join(args)

            result = subprocess.call(args, shell=True)

            # Currently lmbr_test blows up when exiting. The tests all pass.
            # lbmr_test fails even if our tests do absolutely nothing, so it
            # seems to be an issue with the test infrastructure itself.

            # self.assertEquals(result, 0)

    # ------------------------------------------------ Logging ------------------------------------------------

    def __950_check_logs_for_prohibited_content(self):
        prohibited_strings = [
            self.TEST_EMAIL,
            self.TEST_ATTRIBUTES['email'],
            self.context['auth_tokens']['IdToken'],
            self.TEST_PASSWORD,
            self.context['anonymous_aws_credentials']['AccessKeyId'],
            self.context['anonymous_aws_credentials']['SecretKey'],
            self.context['anonymous_aws_credentials']['SessionToken']
        ]
        print 'Checking logs'
        found = False
        for log_group in self.__get_log_groups():
            log_group_name = log_group['logGroupName']
            print '    Log group: {}'.format(log_group_name)
            for log_stream in self.__get_log_streams(log_group_name):
                log_stream_name = log_stream['logStreamName']
                print '        Log stream: {}'.format(log_stream_name)
                for event in GetLogEvents(self.aws_logs, log_group_name, log_stream_name):
                    for value in prohibited_strings:
                        find_result = event.get('message', '').find(value)
                        if find_result >= 0:
                            found = True
                            print 'Found "{}" that should not be logged in log group {} stream {} at {} position {}: {}'.format(
                                value, log_group_name, log_stream_name, event.get('timestamp'), find_result, event.get('message')
                            )
        self.assertFalse(found, "Found strings in the logs that should not have been logged, see above.")

    # ------------------------------------------------ Cleanup ------------------------------------------------

    def __999_cleanup(self):
        if self.FAST_TEST_RERUN:
            print 'Tests passed enough to reach cleanup, failing in cleanup to prevent stack deletion since FAST_TEST_RERUN is true.'
            self.assertFalse(self.FAST_TEST_RERUN)
        self.lmbr_aws('deployment', 'delete', '--deployment', self.TEST_DEPLOYMENT_NAME, '--confirm-resource-deletion')
        self.lmbr_aws('project', 'delete', '--confirm-resource-deletion')

    # ------------------------------------------------ Helpers ------------------------------------------------

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

    def __signup(self, username, password, email):
        self.aws_cognito_idp.sign_up(
            ClientId=self.context['user_pool_client_id'],
            Username=username,
            Password=password,
            UserAttributes=[ {'Name': 'email', 'Value': email} ]
        )

        self.aws_cognito_idp.admin_confirm_sign_up(UserPoolId=self.context['user_pool_id'], Username=username)

    def __signin(self, username, password):
        challenge_response = self.aws_cognito_idp.initiate_auth(
            AuthFlow='CUSTOM_AUTH',
            AuthParameters={'USERNAME': username},
            ClientId=self.context['user_pool_client_id']
        )
        self.assertEqual('BasicAuth', challenge_response.get('ChallengeParameters').get('type'))

        auth_response = self.aws_cognito_idp.respond_to_auth_challenge(
            ClientId=self.context['user_pool_client_id'],
            ChallengeName='CUSTOM_CHALLENGE',
            Session=challenge_response['Session'],
            ChallengeResponses={'USERNAME': username, 'ANSWER': password}
        )
        self.assertIsNotNone(auth_response.get('AuthenticationResult').get('AccessToken'))

        return auth_response['AuthenticationResult']

    def __create_account(self, account, user):
        result = {}

        username = None
        if user != None:
            if account and 'CognitoUsername' in account:
                username = account['CognitoUsername']
            elif 'username' in user:
                username = user['username']
            else:
                username = self.TEST_USERNAME_PREFIX + '-' + self.__generate_id()
            print 'Creating user {} in pool {}'.format(username, self.context['user_pool_id'])

            resultUser = user.copy()
            resultUser['username'] = username
            result['IdentityProviders'] = {
                self.IDP_COGNITO: resultUser
            }
            result['CognitoUsername'] = username

            updates = []
            for key, value in user.iteritems():
                if key in self.COGNITO_ATTRIBUTES:
                    updates.append({'Name': key, 'Value': value})

            self.aws_cognito_idp.admin_create_user(
                UserPoolId=self.context['user_pool_id'],
                Username=username,
                UserAttributes=updates,
                MessageAction='SUPPRESS'
            )

        accountId = None
        if account != None:
            accountId = account.get('AccountId', str(uuid.uuid4()))
            dynamoItem = {
                'AccountId': {'S': accountId},
            }
            result['AccountId'] = accountId
            if 'CognitoUsername' in account:
                dynamoItem['CognitoUsername'] = {'S': account['CognitoUsername']}
                result['CognitoUsername'] = account['CognitoUsername']
            if 'CognitoIdentityId' in account:
                dynamoItem['CognitoIdentityId'] = {'S': account['CognitoIdentityId']}
                result['CognitoIdentityId'] = account['CognitoIdentityId']
            if 'AccountBlacklisted' in account:
                dynamoItem['AccountBlacklisted'] = {'BOOL': account['AccountBlacklisted']}
                result['AccountBlacklisted'] = account['AccountBlacklisted']
            elif username:
                dynamoItem['CognitoUsername'] = {'S': username}
            if 'PlayerName' in account:
                dynamoItem['PlayerName'] = {'S': account['PlayerName']}
                dynamoItem['IndexedPlayerName'] = {'S': account['PlayerName'].lower()}
                dynamoItem['PlayerNameSortKey'] = {'N': str(randint(1, 3))}
                result['PlayerName'] = account['PlayerName']
            print 'Creating account {}'.format(accountId)
            self.aws_dynamo.put_item(TableName=self.context['accounts_table'], Item=dynamoItem)

        return result

    def __delete_all_users(self):
        listUsersResponse = self.aws_cognito_idp.list_users(UserPoolId=self.context['user_pool_id'])
        for user in listUsersResponse['Users']:
            self.aws_cognito_idp.admin_delete_user(UserPoolId=self.context['user_pool_id'], Username=user['Username'])
            print 'Deleted user {}'.format(user['Username'])

    def __delete_all_accounts(self):
        scanResponse = self.aws_dynamo.scan(TableName=self.context['accounts_table'])
        for account in scanResponse['Items']:
            self.aws_dynamo.delete_item(TableName=self.context['accounts_table'], Key={'AccountId': account['AccountId']})
            print 'Deleted account {}'.format(account['AccountId'])

    def __get_identity_id(self, auth_tokens):
        logins = {self.context['user_pool_provider_id']: auth_tokens['IdToken']}
        identity_response = self.aws_cognito_identity.get_id(IdentityPoolId=self.context['identity_pool_id'], Logins=logins)
        return identity_response['IdentityId']

    def __get_aws_credentials(self, identity_id, auth_tokens):
        logins = {self.context['user_pool_provider_id']: auth_tokens['IdToken']}
        response = self.aws_cognito_identity.get_credentials_for_identity(IdentityId=identity_id, Logins=logins)
        return {
            'AccessKeyId': response['Credentials']['AccessKeyId'],
            'SecretKey': response['Credentials']['SecretKey'],
            'SessionToken': response['Credentials']['SessionToken']
        }

    def __get_account_id(self, username):
        accounts = self.__service_get('/admin/accountSearch', {'CognitoUsername': username}, admin_auth=True).get('Accounts', [])
        self.assertEqual(len(accounts), 1)
        return accounts[0]['AccountId']

    def __assert_username_is_correct(self, account):
        username = account.get('CognitoUsername')
        self.assertIsNotNone(username, "Missing 'CognitoUsername' in account, it should have been populated by the custom auth flow: {}".format(account))
        self.assertEqual(username, self.context['test_username'],
            'Expected username {} but got {}, it should have been populated by the custom auth flow'.format(self.context['test_username'], username))

    def __assert_search_results_equal(self, actual, expectedResults):
        expected = deepcopy(expectedResults)
        expectedAccounts = {account['AccountId']:account for account in expected if 'AccountId' in account}
        expectedUsers = {account['CognitoUsername']:account for account in expected if 'AccountId' not in account and 'CognitoUsername' in account}

        for account in actual.get('Accounts', []):
            print 'Checking account: ', account
            accountId = account.get('AccountId')
            if accountId:
                expectedAccount = expectedAccounts.get(accountId)
                self.assertFalse(expectedAccount.get('matched'))
                print 'Expected: ', expectedAccount

                self.assertEqual(account.get('CognitoIdentityId'), expectedAccount.get('CognitoIdentityId'))
                self.assertEqual(account.get('PlayerName'), expectedAccount.get('PlayerName'))

                if 'CognitoUsername' in account:
                    actualAttributes = account.get('IdentityProviders', {}).get(self.IDP_COGNITO, {})
                    expectedAttributes = expectedAccount.get('IdentityProviders', {}).get(self.IDP_COGNITO, {})
                    self.__assert_cognito_user_equals(actualAttributes, expectedAttributes, expectedAttributes.get('username'))

                expectedAccount['matched'] = True
            else:
                username = account.get('CognitoUsername')
                expectedUser = expectedUsers.get(username)
                self.assertIsNotNone(expectedUser, 'User {} should not have been in the search results.'.format(username))
                self.assertFalse(expectedUser.get('matched'))
                print 'Expected: ', expectedUser

                actualAttributes = account.get('IdentityProviders', {}).get(self.IDP_COGNITO, {})
                expectedAttributes = expectedUser.get('IdentityProviders', {}).get(self.IDP_COGNITO, {})
                self.__assert_cognito_user_equals(actualAttributes, expectedAttributes, expectedAttributes.get('username'))

                expectedUser['matched'] = True

        for accountId, account in expectedAccounts.iteritems():
            self.assertTrue(account.get('matched'), 'Account {} should have been in the search results'.format(accountId))
        for username, user in expectedUsers.iteritems():
            self.assertTrue(user.get('matched'), 'User {} should have been in the search results'.format(username))

    def __assert_cognito_user_equals(self, actual, expectedAttributes, username):
        time.sleep(3) # wait to allow cognito to refresh
        if not username:
            self.assertFalse(actual)
        else:
            self.assertEqual(actual['username'], username)
            self.assertTrue('status' in actual)
            self.assertTrue(actual['create_date'] > 0)
            self.assertTrue(actual['last_modified_date'] > 0)
            self.assertTrue(actual['enabled'])
            self.assertEqual(actual['IdentityProviderId'], self.IDP_COGNITO)
            for key, value in expectedAttributes.iteritems():
                self.assertEqual(actual[key], value)
            # Check that the standard cognito attributes were not present in the request if they were not expected.
            for attribute in self.COGNITO_ATTRIBUTES:
                if attribute not in expectedAttributes:
                    self.assertNotIn(attribute, actual)

    def __generate_id(self):
        self.context['generator_sequence'] = self.context['generator_sequence'] + 1
        return self.context['test_run_id'] + '-' + str(self.context['generator_sequence'])

    def __get_log_groups(self):
        log_prefix = '/aws/lambda/' + self.context['ProjectStackName']
        response = self.aws_logs.describe_log_groups(logGroupNamePrefix=log_prefix)
        log_groups = response.get('logGroups', [])
        while 'nextToken' in response:
            response = self.aws_logs.describe_log_groups(logGroupNamePrefix=log_prefix, nextToken=response['nextToken'])
            log_groups.extend(response.get('logGroups', []))
        return log_groups

    def __get_log_streams(self, log_group):
        response = self.aws_logs.describe_log_streams(logGroupName=log_group)
        log_streams = response.get('logStreams', [])
        while 'nextToken' in response:
            response = self.aws_logs.describe_log_streams(logGroupName=log_group, nextToken=response['nextToken'])
            log_streams.extend(response.get('logStreams', []))
        return log_streams

    @classmethod
    def isrunnable(self, methodname, dict):
        if self.FAST_TEST_RERUN:
            return True
        return lmbr_aws_test_support.lmbr_aws_TestCase.isrunnable(methodname, dict)

class GetLogEvents:
    def __init__(self, logs_client, log_group, log_stream):
        self.__logs_client = logs_client
        self.__log_group = log_group
        self.__log_stream = log_stream
        self.__initialized = False
    def __iter__(self):
        return self
    def next(self):
        if not self.__initialized:
            response = self.__logs_client.get_log_events(logGroupName=self.__log_group, logStreamName=self.__log_stream)
            self.__next_token = response.get('nextBackwardToken')
            self.__events = response.get('events', [])
            self.__initialized = True
        if not self.__events:
            response = self.__logs_client.get_log_events(logGroupName=self.__log_group, logStreamName=self.__log_stream, nextToken=self.__next_token)
            self.__next_token = response.get('nextBackwardToken')
            self.__events = response.get('events', [])
        if self.__events:
            return self.__events.pop()
        raise StopIteration