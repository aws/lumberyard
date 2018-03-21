#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the 'License'). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an 'AS IS' BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#
# $Revision$

import os
import json
import unittest

import boto3

import resource_manager_common.constant

import lmbr_aws_test_support
import test_constant


class IntegrationTest_CloudGemFramework_ResourceManager_CognitoResourceHandlers(lmbr_aws_test_support.lmbr_aws_TestCase):

    USER_POOL_1_NAME = 'TestUserPool1'
    USER_POOL_1 = test_constant.RESOURCE_GROUP_NAME + '.' + USER_POOL_1_NAME

    USER_POOL_2_NAME = 'TestUserPool2'
    USER_POOL_2 = 'access.' + USER_POOL_2_NAME

    USER_POOL_3_NAME = 'TestUserPool3'
    USER_POOL_3 = test_constant.RESOURCE_GROUP_NAME + '.' + USER_POOL_3_NAME

    USER_POOL_4_NAME = 'TestUserPool4'
    USER_POOL_4 = test_constant.RESOURCE_GROUP_NAME + '.' + USER_POOL_4_NAME

    def setUp(self):        
        self.prepare_test_envionment(type(self).__name__)
        
    def test_cognito_resource_handlers_end_to_end(self):
        self.run_all_tests()

    def __100_create_project_stack(self):
        self.lmbr_aws(
            'project', 'create', 
            '--stack-name', self.TEST_PROJECT_STACK_NAME, 
            '--confirm-aws-usage',
            '--confirm-security-change', 
            '--region', lmbr_aws_test_support.REGION
        )

    def __110_add_resource_groups(self):
        self.lmbr_aws(
            'cloud-gem', 'create',
            '--gem', self.TEST_RESOURCE_GROUP_NAME,
            '--initial-content', 'no-resources',
            '--enable'
        )

    def __119_create_deployment_access_template_extensions_file(self):
        self.lmbr_aws('project', 'create-extension-template', '--deployment-access')

    def __120_configure_stack(self):
        # This pool also tests that it stays linked when updated.
        self.put_identity_pool_in_resource_group_template(self.TEST_RESOURCE_GROUP_NAME, 'TestIdentityPool1')

        # Tests creating identity pool after user pool.
        self.put_user_pool_in_resource_group_template(
            self.TEST_RESOURCE_GROUP_NAME,
            self.USER_POOL_1_NAME,
            identities = {'PlayerAccessIdentityPool': 'Client1'},
            client_apps = ['Client1'])

        # Tests creating user pool after identity pool.
        self.put_user_pool_in_deployment_access_template(
            self.USER_POOL_2_NAME,
            identities = {'TestIdentityPool1': 'Client1'},
            client_apps = ['Client1'])

        # This pool is for testing that it stays linked when updated.
        # The non-existant pool name is ignored.
        self.put_user_pool_in_resource_group_template(
            self.TEST_RESOURCE_GROUP_NAME,
            self.USER_POOL_3_NAME,
            identities = {'PlayerAccessIdentityPool': 'Client1', 'NonExistantPoolName': 'Client1'},
            client_apps = ['Client1'])

        self.put_user_pool_in_resource_group_template(
            self.TEST_RESOURCE_GROUP_NAME,
            self.USER_POOL_4_NAME,
            identities = {'PlayerAccessIdentityPool': 'Client1'},
            client_apps = ['Client1'])

    def __130_create_deployment_stack(self):
        self.lmbr_aws(
            'deployment', 'create',
            '--deployment', self.TEST_DEPLOYMENT_NAME,
            '--confirm-aws-usage',
            '--confirm-security-change'
        )

    def __140_verify_stack_after_creation(self):
        self.assert_linked('access.PlayerAccessIdentityPool', {self.USER_POOL_1: ['Client1'], self.USER_POOL_3: ['Client1'], self.USER_POOL_4: ['Client1']})
        self.assert_linked('access.PlayerLoginIdentityPool', {})
        self.assert_linked(self.TEST_RESOURCE_GROUP_NAME + '.TestIdentityPool1', {self.USER_POOL_2: ['Client1']})

    def __200_configure_stack_for_update(self):
        # Test changing the client for PlayerAccess.
        self.put_user_pool_in_resource_group_template(
            self.TEST_RESOURCE_GROUP_NAME,
            self.USER_POOL_1_NAME,
            identities = {'PlayerAccessIdentityPool': 'Client2'},
            client_apps = ['Client2'])

        # Test that a non-existant pool name is ignored on a user pool during an update.
        # Remove PlayerAccessIdentityPool to test unlinking during a user pool update.
        # Add PlayerLoginIdentityPool to test linking during a user pool update.
        self.put_user_pool_in_resource_group_template(
            self.TEST_RESOURCE_GROUP_NAME,
            self.USER_POOL_4_NAME,
            identities = {'PlayerLoginIdentityPool': 'Client2', 'NonExistentPoolName': 'Client1'},
            client_apps = ['Client1', 'Client2'])

    def __210_update_deployment_stack(self):
        self.lmbr_aws(
            'deployment', 'upload-resources',
            '--deployment', self.TEST_DEPLOYMENT_NAME,
            '--confirm-aws-usage',
            '--confirm-security-change'
        )

    def __220_verify_stack_after_update(self):
        self.assert_linked('access.PlayerAccessIdentityPool', {self.USER_POOL_1: ['Client2'], self.USER_POOL_3: ['Client1']})
        self.assert_linked('access.PlayerLoginIdentityPool', {self.USER_POOL_4: ['Client2']})
        self.assert_linked(self.TEST_RESOURCE_GROUP_NAME + '.TestIdentityPool1', {self.USER_POOL_2: ['Client1']})

    def __900_delete_deployment_stack(self):
        self.lmbr_aws(
            'deployment', 'delete',
            '--deployment', self.TEST_DEPLOYMENT_NAME, 
            '--confirm-resource-deletion'
        )

    def __910_delete_project_stack(self):
        self.lmbr_aws(
            'project', 'delete', 
            '--confirm-resource-deletion'
        )

    def put_identity_pool_in_resource_group_template(self, gem_name, pool_name):
        with self.edit_gem_aws_json(gem_name, resource_manager_common.constant.RESOURCE_GROUP_TEMPLATE_FILENAME) as template:
            resources = template['Resources']
            resources[pool_name] = {
                "Type": "Custom::CognitoIdentityPool",
                "Properties": {
                    "ServiceToken": { "Ref": "ProjectResourceHandler" },
                    "AllowUnauthenticatedIdentities": "true",
                    "UseAuthSettingsObject": "false",
                    "ConfigurationBucket": { "Ref": "ConfigurationBucket" },
                    "ConfigurationKey": { "Ref": "ConfigurationKey" },
                    "IdentityPoolName": "PlayerAccess", # This name is recognized by Tools/lmbr_aws/test/cleanup.py
                    "Roles": {
                    }
                }
            }

    def put_user_pool_in_resource_group_template(self, gem_name, pool_name, identities, client_apps):
        with self.edit_gem_aws_json(gem_name, resource_manager_common.constant.RESOURCE_GROUP_TEMPLATE_FILENAME) as template:
            self.put_user_pool_in_template(template, pool_name, identities, client_apps)

    def put_user_pool_in_deployment_access_template(self, pool_name, identities, client_apps):
        with self.edit_project_aws_json(resource_manager_common.constant.DEPLOYMENT_ACCESS_TEMPLATE_EXTENSIONS_FILENAME) as template:
            self.put_user_pool_in_template(template, pool_name, identities, client_apps)

    def put_user_pool_in_template(self, template, pool_name, identities, client_apps):
        resources = template['Resources']

        resources[pool_name] = {
            "Properties": {
                "ClientApps": client_apps,
                "ConfigurationKey": {
                    "Ref": "ConfigurationKey"
                },
                "PoolName": "PlayerAccess", # This name is recognized by Tools/lmbr_aws/test/cleanup.py
                "ServiceToken": {
                    "Ref": "ProjectResourceHandler"
                }
            },
            "Type": "Custom::CognitoUserPool"
        }

        if identities:
            resources[pool_name]['Metadata'] = {
                'CloudCanvas': {
                    'Identities': [{'IdentityPoolLogicalName': k, 'ClientApp': v} for k,v in identities.iteritems()]
                }
            }

    def get_identity_pool(self, identity_pool_id):
        return self.aws_cognito_identity.describe_identity_pool(IdentityPoolId=identity_pool_id)

    def assert_linked(self, identity_pool_logical_id, expected_user_pools):
        actual_providers = {}
        identity_pool_id = self.get_physical_id(identity_pool_logical_id)
        identity_pool = self.get_identity_pool(identity_pool_id)
        for provider in identity_pool.get('CognitoIdentityProviders', []):
            providers = actual_providers.get(provider['ProviderName'], set())
            providers.add(provider['ClientId'])
            actual_providers[provider['ProviderName']] = providers

        expected_providers = {}
        debug_messages = ['Identity pool {} has physical id {}'.format(identity_pool_logical_id, identity_pool_id)]
        for user_pool_logical_id, client_names in expected_user_pools.iteritems():
            user_pool_id = self.get_physical_id(user_pool_logical_id)
            debug_messages.append('User pool {} has physical id {}'.format(user_pool_logical_id, user_pool_id))
            client_apps = self.aws_cognito_idp.list_user_pool_clients(UserPoolId=user_pool_id, MaxResults=60).get('UserPoolClients', [])
            for client_app in client_apps:
                debug_messages.append('    {}: {}'.format(client_app['ClientName'], client_app['ClientId']))
            client_id_by_name = {client_app['ClientName']: client_app['ClientId'] for client_app in client_apps}
            providers = set()
            for client_name in client_names:
                self.assertIn(client_name, client_id_by_name, 'User pool {} should have a client named {}'.format(user_pool_logical_id, client_name))
                providers.add(client_id_by_name[client_name])
            expected_providers[unicode(self.get_provider_name(user_pool_id))] = providers

        if expected_providers != actual_providers:
            print "\n".join(debug_messages)
        self.assertEqual(expected_providers, actual_providers)

    def get_physical_id(self, logical_id):
        group_and_logical = logical_id.split('.')
        self.assertEqual(len(group_and_logical), 2)
        if group_and_logical[0] == 'access':
            stack_arn = self.get_deployment_access_stack_arn(self.TEST_DEPLOYMENT_NAME)
        elif group_and_logical[0] == 'project':
            stack_arn = self.get_project_stack_arn()
        else:
            stack_arn = self.get_resource_group_stack_arn(self.TEST_DEPLOYMENT_NAME, group_and_logical[0])

        return self.get_stack_resource_physical_id(stack_arn, group_and_logical[1])

    def get_client_id(self, user_pool_id, client_name):
        client_apps = self.aws_cognito_idp.list_user_pool_clients(UserPoolId=user_pool_id, MaxResults=60)
        for client_app in client_apps.get('UserPoolClients', []):
            if client_app['ClientName'] == client_name:
                return client_app['ClientId']
        self.fail('Client {} not found in user pool {}'.format(client_name, user_pool_id))

    def get_provider_name(self, user_pool_id):
        # User pool IDs are of the form: us-east-1_123456789
        # Provider names are of the form: cognito-idp.us-east-1.amazonaws.com/us-east-1_123456789
        beginning = "cognito-idp."
        middle = ".amazonaws.com/"
        region_size = user_pool_id.find("_")  # Get the region from the first part of the Pool ID
        region = ""
        if region_size >= 0:
            region = user_pool_id[0 : region_size]
    
        return beginning + region + middle + user_pool_id
