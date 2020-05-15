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

from botocore.exceptions import ClientError

from .custom_resource_test_case import CustomResourceTestCase

from resource_manager_common import stack_info
from resource_manager_common import constant

from cgf_utils import custom_resource_response
from cgf_utils import custom_resource_utils

from resource_types.cognito import identity_pool
from resource_types import Custom_CognitoIdentityPool


# Specific unit tests to test setting up Cognito identity pools
class UnitTest_CloudGemFramework_ResourceTypeResourcesHandler_CognitoIdentityPool(CustomResourceTestCase):
    _mock_cognito_client = MagicMock()
    _mock_s3_client = MagicMock()

    MOCK_IDENTITY_POOL_NAME = 'ProjectUserPool'
    MOCK_IDENTITY_POOL_ID = "{}.123456789".format(CustomResourceTestCase.REGION)

    def __assert_handler_succeeds(self, request_type, expected_data, pool_id=MOCK_IDENTITY_POOL_ID):
        """Helper class to ensure that calling the handler function behaves as expected"""
        event = {
            'RequestType': request_type,
            'LogicalResourceId': self.LOGICAL_RESOURCE_ID,
            'ResourceProperties': {
                'IdentityPoolName': self.MOCK_IDENTITY_POOL_NAME,
                'ConfigurationKey': 'Key',
                'ConfigurationBucket': self.MOCK_S3_CONFIG_BUCKET,
                'AllowUnauthenticatedIdentities': 'true',
                'UseAuthSettingsObject': 'false'
            },
            'StackId': self.STACK_ARN,
            'ResourceType': 'Custom::ResourceTypes',
            'RequestId': 'bf1143a7-484b-4d3c-8e23-4de2437c4301'
        }

        mock_response_succeed = MagicMock()
        self._mock_cognito_client().create_identity_pool.return_value = {'IdentityPoolId': self.MOCK_IDENTITY_POOL_ID,
                                                                         'IdentityPoolName': self.MOCK_IDENTITY_POOL_NAME}

        # Set up a bunch o' mocks so we can call the handler as we expect
        with mock.patch.object(Custom_CognitoIdentityPool, '_get_s3_client', new=self._mock_s3_client()):
            with mock.patch.object(identity_pool, 'get_identity_client', return_value=self._mock_cognito_client()):
                with mock.patch.object(custom_resource_response, 'success_response', new=mock_response_succeed):
                    with mock.patch('cgf_utils.aws_utils.get_cognito_pool_from_file', return_value={}):
                        Custom_CognitoIdentityPool.handler(event, self.CONTEXT)

        # Ensure response is what we expected
        mock_response_succeed.assert_called_with(expected_data, pool_id)

    @mock.patch.object(stack_info.StackInfoManager, 'get_stack_info', return_value=CustomResourceTestCase.RESOURCE_GROUP_INFO)
    def test_custom_cognito_identity_pool_creation(self, *args):
        """Test that new resources are created as expected"""
        # Given
        _timeout_value = 12
        _memory_value = 256
        _expected_tag_value = {"stack-name": CustomResourceTestCase.STACK_NAME}
        _expected_data = {
            'IdentityPoolName': "{}{}".format(self.STACK_NAME, self.MOCK_IDENTITY_POOL_NAME),
            'IdentityPoolId': self.MOCK_IDENTITY_POOL_ID
        }

        self._mock_s3_client.get_object.side_effect = ClientError({'Error': {'Code': 'NoSuchKey', 'Message': 'NotFound'}}, "get_object")
        self.__assert_handler_succeeds('Create', expected_data=_expected_data)

        # Ensure call to create include tags
        expected_pool_name = "{}{}".format(self.STACK_NAME, self.MOCK_IDENTITY_POOL_NAME)
        self._mock_cognito_client().create_identity_pool.assert_called_with(
            AllowUnauthenticatedIdentities=True,
            CognitoIdentityProviders=[],
            IdentityPoolName=expected_pool_name,
            IdentityPoolTags={constant.STACK_ID_TAG: self.STACK_ARN, constant.PROJECT_NAME_TAG: self.STACK_NAME},
            SupportedLoginProviders={})

    @mock.patch.object(stack_info.StackInfoManager, 'get_stack_info', return_value=CustomResourceTestCase.RESOURCE_GROUP_INFO)
    @mock.patch.object(identity_pool, 'get_identity_pool', return_value=MagicMock())
    @mock.patch.object(custom_resource_utils, 'get_embedded_physical_id', return_value=MOCK_IDENTITY_POOL_ID)
    def test_custom_cognito_identity_pool_update(self, *args):
        """Test that updated resources are created as expected"""
        # Given
        _timeout_value = 12
        _memory_value = 256
        _expected_tag_value = {"stack-name": CustomResourceTestCase.STACK_NAME}
        _expected_data = {
            'IdentityPoolName': "{}{}".format(self.STACK_NAME, self.MOCK_IDENTITY_POOL_NAME),
            'IdentityPoolId': self.MOCK_IDENTITY_POOL_ID
        }

        self._mock_s3_client.get_object.side_effect = ClientError({'Error': {'Code': 'NoSuchKey', 'Message': 'NotFound'}}, "get_object")
        self.__assert_handler_succeeds('Update', _expected_data)

        # Ensure call to create include tags
        expected_pool_name = "{}{}".format(self.STACK_NAME, self.MOCK_IDENTITY_POOL_NAME)
        self._mock_cognito_client().update_identity_pool.assert_called_with(
            IdentityPoolId=self.MOCK_IDENTITY_POOL_ID,
            AllowUnauthenticatedIdentities=True,
            CognitoIdentityProviders=[],
            IdentityPoolName=expected_pool_name,
            IdentityPoolTags={constant.STACK_ID_TAG: self.STACK_ARN, constant.PROJECT_NAME_TAG: self.STACK_NAME},
            SupportedLoginProviders={})

    @mock.patch.object(stack_info.StackInfoManager, 'get_stack_info', return_value=CustomResourceTestCase.RESOURCE_GROUP_INFO)
    @mock.patch.object(identity_pool, 'get_identity_pool', return_value=MagicMock())
    @mock.patch.object(custom_resource_utils, 'get_embedded_physical_id', return_value=MOCK_IDENTITY_POOL_ID)
    def test_custom_cognito_identity_pool_delete(self, *args):
        """Test that updated resources are created as expected"""
        # Given
        _timeout_value = 12
        _memory_value = 256
        _expected_tag_value = {"stack-name": CustomResourceTestCase.STACK_NAME}
        _expected_data = {}

        self._mock_s3_client.get_object.side_effect = ClientError({'Error': {'Code': 'NoSuchKey', 'Message': 'NotFound'}}, "get_object")
        self.__assert_handler_succeeds('Delete', {})

        # Ensure call to delete resource
        self._mock_cognito_client().delete_identity_pool.assert_called_with(IdentityPoolId=self.MOCK_IDENTITY_POOL_ID)
