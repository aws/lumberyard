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

from unittest import mock
from unittest.mock import MagicMock
from unittest.mock import ANY

from botocore.exceptions import ClientError

from .custom_resource_test_case import CustomResourceTestCase

from resource_manager_common import stack_info
from resource_manager_common import constant

from cgf_utils import custom_resource_response
from cgf_utils import custom_resource_utils

from resource_types.cognito import user_pool
from resource_types.cognito import identity_pool
from resource_types import Custom_CognitoUserPool


# Specific unit tests to test setting up Cognito User pools
class UnitTest_CloudGemFramework_ResourceTypeResourcesHandler_CognitoUserPool(CustomResourceTestCase):
    _mock_cognito_idp_client = MagicMock()
    _mock_cognito_identity_client = MagicMock()
    _mock_s3_client = MagicMock()

    MOCK_USER_POOL_NAME = 'ProjectUserPool'
    MOCK_USER_POOL_ID = "{}.123456789".format(CustomResourceTestCase.REGION)

    def __assert_handler_succeeds(self, request_type, expected_data, pool_id=MOCK_USER_POOL_ID):
        """Helper class to ensure that calling the handler function behaves as expected"""
        event = {
            'RequestType': request_type,
            'LogicalResourceId': self.LOGICAL_RESOURCE_ID,
            'ResourceProperties': {
                'PoolName': self.MOCK_USER_POOL_NAME,
                'ConfigurationKey': 'Key',
                'ClientApps': [],
                'ExplicitAuthFlows': [],
                'RefreshTokenValidity': "30",
                "LambdaConfig": {},
                "Groups": [
                    {
                        'Role': 'arn:aws:iam::{}:role/TestRole'.format(self.ACCOUNT),
                        'Description': 'A test user.',
                        'Precedence': '10',
                        'Name': 'user'
                    }
                ],
                "AllowAdminCreateUserOnly": "true"
            },
            'StackId': self.STACK_ARN,
            'ResourceType': 'Custom::ResourceTypes',
            'RequestId': 'bf1143a7-484b-4d3c-8e23-4de2437c4301'
        }

        mock_response_succeed = MagicMock()
        self._mock_cognito_idp_client().create_user_pool.return_value = {
            'UserPool': {
                'Id': self.MOCK_USER_POOL_ID,
                'Name': self.MOCK_USER_POOL_NAME
            }
        }

        # Set up a bunch o' mocks so we can call the handler as we expect
        with mock.patch.object(user_pool, 'get_idp_client', return_value=self._mock_cognito_idp_client()):
            with mock.patch.object(custom_resource_response, 'success_response', new=mock_response_succeed):
                with mock.patch('cgf_utils.aws_utils.get_cognito_pool_from_file', return_value={}):
                    Custom_CognitoUserPool.handler(event, self.CONTEXT)

        #
        # Ensure response is what we expected
        mock_response_succeed.assert_called_with(expected_data, pool_id)

    @mock.patch.object(stack_info.StackInfoManager, 'get_stack_info', return_value=CustomResourceTestCase.RESOURCE_GROUP_INFO)
    @mock.patch('resource_types.cognito.identity_pool.update_cognito_identity_providers')
    def test_custom_cognito_user_pool_creation(self, *args):
        """Test that new resources are created as expected"""
        # Given
        _timeout_value = 12
        _memory_value = 256
        _expected_tag_value = {"stack-name": CustomResourceTestCase.STACK_NAME}
        _expected_pool_name = "{}{}".format(self.STACK_NAME, self.MOCK_USER_POOL_NAME)
        _expected_data = {
            'UserPoolName': _expected_pool_name,
            'ClientApps': {
                'Deleted': [],
                'Updated': [],
                'Created': []
            },
            'UserPoolId': self.MOCK_USER_POOL_ID
        }

        self._mock_s3_client.get_object.side_effect = ClientError({'Error': {'Code': 'NoSuchKey', 'Message': 'NotFound'}}, "get_object")
        self.__assert_handler_succeeds('Create', expected_data=_expected_data)

        # Ensure call to create include tags
        self._mock_cognito_idp_client().create_user_pool.assert_called_with(
            AdminCreateUserConfig={'AllowAdminCreateUserOnly': True},
            AutoVerifiedAttributes=['email'],
            LambdaConfig={},
            MfaConfiguration='OFF',
            PoolName=_expected_pool_name,
            UserPoolTags={
                constant.STACK_ID_TAG: self.STACK_ARN,
                constant.PROJECT_NAME_TAG: self.STACK_NAME
            }
        )

        # Ensure call to cogito identity with expected parameters
        # Note: 1st param is the stack manager so we don't care about that
        identity_pool.update_cognito_identity_providers.assert_called_with(
            ANY, self.STACK_ARN, self.MOCK_USER_POOL_ID, {self.STACK_ARN: {'TestLogicalId': {'physical_id': self.MOCK_USER_POOL_ID, 'client_apps': {}}}}
        )

    @mock.patch.object(stack_info.StackInfoManager, 'get_stack_info', return_value=CustomResourceTestCase.RESOURCE_GROUP_INFO)
    @mock.patch.object(user_pool, 'get_user_pool', return_value=MagicMock())
    @mock.patch.object(custom_resource_utils, 'get_embedded_physical_id', return_value=MOCK_USER_POOL_ID)
    def test_custom_cognito_user_pool_update(self, *args):
        """Test that updated resources are created as expected"""
        # Given
        _timeout_value = 12
        _memory_value = 256
        _expected_tag_value = {"stack-name": CustomResourceTestCase.STACK_NAME}
        _expected_pool_name = "{}{}".format(self.STACK_NAME, self.MOCK_USER_POOL_NAME)
        _expected_data = {
            'UserPoolName': _expected_pool_name,
            'ClientApps': {
                'Deleted': [],
                'Updated': [],
                'Created': []
            },
            'UserPoolId': self.MOCK_USER_POOL_ID
        }

        self._mock_s3_client.get_object.side_effect = ClientError({'Error': {'Code': 'NoSuchKey', 'Message': 'NotFound'}}, "get_object")
        self.__assert_handler_succeeds('Update', _expected_data)

        # Ensure call to create include tags
        self._mock_cognito_idp_client().update_user_pool.assert_called_with(
            AdminCreateUserConfig={'AllowAdminCreateUserOnly': True},
            AutoVerifiedAttributes=['email'],
            LambdaConfig={},
            MfaConfiguration='OFF',
            UserPoolId=self.MOCK_USER_POOL_ID,
            UserPoolTags={
                constant.STACK_ID_TAG: self.STACK_ARN,
                constant.PROJECT_NAME_TAG: self.STACK_NAME
            }
        )

    @mock.patch.object(stack_info.StackInfoManager, 'get_stack_info', return_value=CustomResourceTestCase.RESOURCE_GROUP_INFO)
    @mock.patch.object(user_pool, 'get_user_pool', return_value=MagicMock())
    @mock.patch.object(custom_resource_utils, 'get_embedded_physical_id', return_value=MOCK_USER_POOL_ID)
    def test_custom_cognito_user_pool_delete(self, *args):
        """Test that updated resources are created as expected"""
        # Given
        _timeout_value = 12
        _memory_value = 256
        _expected_tag_value = {"stack-name": CustomResourceTestCase.STACK_NAME}
        _expected_data = {}

        self._mock_s3_client.get_object.side_effect = ClientError({'Error': {'Code': 'NoSuchKey', 'Message': 'NotFound'}}, "get_object")
        self.__assert_handler_succeeds('Delete', {})

        # Ensure call to delete resource
        self._mock_cognito_idp_client().delete_user_pool.assert_called_with(UserPoolId=self.MOCK_USER_POOL_ID)
