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

import unittest
import mock
import os
import json

# Need to patch the envionment before loading ServicApiResourceHandler
TEST_REGION = 'test-region'
os.environ['AWS_DEFAULT_REGION'] = TEST_REGION

from cgf_utils import custom_resource_response
from cgf_utils import role_utils
from resource_manager_common import stack_info
from cgf_utils import properties
from cgf_utils.test import mock_aws
from resource_types import Custom_ServiceApi 
from resource_manager_common.test.mock_stack_info import MockResourceGroupInfo, MockDeploymentInfo
from cgf_utils.test.mock_properties import PropertiesMatcher
from resource_manager_common import service_interface


ROLE_NAME = 'TestRoleName'
ROLE_ARN = 'TestRoleArn'
LOGICAL_RESOURCE_ID = 'TestLogicalResoureId'
REST_API_RESOURCE_NAME = MockResourceGroupInfo.MOCK_STACK_NAME + '-' + LOGICAL_RESOURCE_ID
CONFIGURATION_BUCKET = 'TestBucket'
CONFIGURATION_KEY = 'test-key'
INPUT_SWAGGER_KEY = CONFIGURATION_KEY + '/swagger.json'
REST_API_ID = 'TestRestApiId'
EXPECTED_URL = 'https://{}.execute-api.{}.amazonaws.com/{}'.format(REST_API_ID, MockResourceGroupInfo.MOCK_REGION, Custom_ServiceApi.STAGE_NAME)
SWAGGER_CONTENT = 'TestSwaggerContent'
RESOURCE_GROUP_INFO = MockResourceGroupInfo()
SWAGGER_DIGEST = 'TestSwaggerDigest'
SWAGGER_DIGEST_A = 'TestSwaggerDigestA'
SWAGGER_DIGEST_B = 'TestSwaggerDigestB'
MOCK_PATCH_OPERATIONS = [ 'Mock Patch Operation' ]
EMPTY_PATCH_OPERATIONS = []
REST_API_DEPLOYMENT_ID = 'TEstRestApiDeploymentId'
STACK_MANAGER = stack_info.StackInfoManager()

FULL_RESOURCE_PROPERTIES = {
    'ConfigurationBucket': CONFIGURATION_BUCKET,
    'ConfigurationKey': CONFIGURATION_KEY,
    'SwaggerSettings': { 'SwaggerSettings': '' },
    'MethodSettings': { 
        'MethodSettingPath': { 
            'MethodSettingMethod': {  
                'CacheDataEncrypted': True,
                'CacheTtlInSeconds': 10,
                'CachingEnable': True,
                'DataTraceEnabled': True,
                'LoggingLevel': 'OFF',
                'MetricsEnabled': True,
                'ThrottlingBurstLimit': 20,
                'ThrottlingRateLimit': 30,
            }
        }
    },
    'CacheClusterEnabled': True,
    'CacheClusterSize': '0.5',
    'StageVariables': { 'StageVariables': '' }
}

MINIMAL_RESOURCE_PROPERTIES = {
    'ConfigurationBucket': CONFIGURATION_BUCKET,
    'ConfigurationKey': CONFIGURATION_KEY
}

FULL_PROPS = properties._Properties(FULL_RESOURCE_PROPERTIES, Custom_ServiceApi.PROPERTY_SCHEMA)
MINIMAL_PROPS = properties._Properties(MINIMAL_RESOURCE_PROPERTIES, Custom_ServiceApi.PROPERTY_SCHEMA)

PROPS_MATCHER = PropertiesMatcher()

class UnitTest_CloudGemFramework_ProjectResourceHandler_Custom_ServiceApi_RequestTypes(unittest.TestCase):

    @mock.patch.object(custom_resource_response, 'success_response')
    @mock.patch.object(role_utils, 'create_access_control_role', return_value = ROLE_ARN)
    @mock.patch.object(stack_info.StackInfoManager, 'get_stack_info', return_value = RESOURCE_GROUP_INFO)
    @mock.patch.object(Custom_ServiceApi, 'get_configured_swagger_content', return_value = SWAGGER_CONTENT)
    @mock.patch.object(Custom_ServiceApi, 'create_api_gateway', return_value = REST_API_ID)
    @mock.patch.object(Custom_ServiceApi, 'register_service_interfaces')
    def test_Create_with_full_properties(self, *args):

        # Setup

        event = {
            'RequestType': 'Create',
            'StackId': MockResourceGroupInfo.MOCK_STACK_ARN,
            'LogicalResourceId': LOGICAL_RESOURCE_ID,
            'ResourceProperties': FULL_RESOURCE_PROPERTIES
        }

        context = {}

        expected_data = {
            'Url': EXPECTED_URL
        }

        # Execute

        with mock.patch('resource_manager_common.stack_info.StackInfoManager', return_value=STACK_MANAGER) as mock_stack_info_manager:
            Custom_ServiceApi.handler(event, context)

            # Validate

            role_utils.create_access_control_role.assert_called_once_with(
                mock_stack_info_manager.return_value,
                {'RestApiId': REST_API_ID},
                MockResourceGroupInfo.MOCK_STACK_ARN,
                LOGICAL_RESOURCE_ID,
                Custom_ServiceApi.API_GATEWAY_SERVICE_NAME)

            Custom_ServiceApi.get_configured_swagger_content.assert_called_once_with(
                RESOURCE_GROUP_INFO,
                PROPS_MATCHER,
                ROLE_ARN,
                REST_API_RESOURCE_NAME)

            Custom_ServiceApi.create_api_gateway.assert_called_once_with(
                PROPS_MATCHER,
                SWAGGER_CONTENT)

            expected_physical_resource_id = '{}-{}::{}'.format(MockResourceGroupInfo.MOCK_STACK_NAME, LOGICAL_RESOURCE_ID, json.dumps({"RestApiId": REST_API_ID}))

            custom_resource_response.success_response.assert_called_once_with(
                expected_data,
                expected_physical_resource_id)

            Custom_ServiceApi.register_service_interfaces.assert_called_once_with(
                RESOURCE_GROUP_INFO, 
                'https://{rest_api_id}.execute-api.{region}.amazonaws.com/{stage_name}'.format(
                    rest_api_id = REST_API_ID,
                    region = RESOURCE_GROUP_INFO.region,
                    stage_name = Custom_ServiceApi.STAGE_NAME
                ), 
                SWAGGER_CONTENT)


    @mock.patch.object(custom_resource_response, 'success_response')
    @mock.patch.object(role_utils, 'create_access_control_role', return_value = ROLE_ARN)
    @mock.patch.object(stack_info.StackInfoManager, 'get_stack_info', return_value=RESOURCE_GROUP_INFO)
    @mock.patch.object(Custom_ServiceApi, 'get_configured_swagger_content', return_value = SWAGGER_CONTENT)
    @mock.patch.object(Custom_ServiceApi, 'create_api_gateway', return_value = REST_API_ID)
    @mock.patch.object(Custom_ServiceApi, 'register_service_interfaces')
    def test_Create_with_mimimal_properties(self, *args):

        # Setup

        event = {
            'RequestType': 'Create',
            'StackId': MockResourceGroupInfo.MOCK_STACK_ARN,
            'LogicalResourceId': LOGICAL_RESOURCE_ID,
            'ResourceProperties': MINIMAL_RESOURCE_PROPERTIES
        }

        context = {}

        expected_data = {
            'Url': EXPECTED_URL
        }

        # Execute

        with mock.patch('resource_manager_common.stack_info.StackInfoManager', return_value=STACK_MANAGER) as mock_stack_info_manager:
            Custom_ServiceApi.handler(event, context)

            # Validate

            role_utils.create_access_control_role.assert_called_once_with(
                mock_stack_info_manager.return_value,
                {'RestApiId': REST_API_ID},
                MockResourceGroupInfo.MOCK_STACK_ARN,
                LOGICAL_RESOURCE_ID,
                Custom_ServiceApi.API_GATEWAY_SERVICE_NAME)

            Custom_ServiceApi.get_configured_swagger_content.assert_called_once_with(
                RESOURCE_GROUP_INFO,
                PROPS_MATCHER,
                ROLE_ARN,
                REST_API_RESOURCE_NAME)

            Custom_ServiceApi.create_api_gateway.assert_called_once_with(
                PROPS_MATCHER,
                SWAGGER_CONTENT)

            expected_physical_resource_id = MockResourceGroupInfo.MOCK_STACK_NAME + '-' + LOGICAL_RESOURCE_ID + '::{"RestApiId": "TestRestApiId"}'

            custom_resource_response.success_response.assert_called_once_with(
                expected_data,
                expected_physical_resource_id)

            Custom_ServiceApi.register_service_interfaces.assert_called_once_with(
                RESOURCE_GROUP_INFO, 
                'https://{rest_api_id}.execute-api.{region}.amazonaws.com/{stage_name}'.format(
                    rest_api_id = REST_API_ID,
                    region = RESOURCE_GROUP_INFO.region,
                    stage_name = Custom_ServiceApi.STAGE_NAME
                ), 
                SWAGGER_CONTENT)


    @mock.patch.object(custom_resource_response, 'success_response')
    @mock.patch.object(role_utils, 'get_access_control_role_arn', return_value = ROLE_ARN)
    @mock.patch.object(stack_info.StackInfoManager, 'get_stack_info', return_value=RESOURCE_GROUP_INFO)
    @mock.patch.object(Custom_ServiceApi, 'get_configured_swagger_content', return_value = SWAGGER_CONTENT)
    @mock.patch.object(Custom_ServiceApi, 'update_api_gateway')
    @mock.patch.object(Custom_ServiceApi, 'create_documentation_version')
    @mock.patch.object(Custom_ServiceApi, 'register_service_interfaces')
    def test_Update_with_full_properties(self, *args):

        # Setup

        id_data = { 'AbstractRoleMappings': { LOGICAL_RESOURCE_ID : ROLE_NAME }, 'RestApiId': REST_API_ID }

        physical_resource_id = '{}-{}::{}'.format(
            MockResourceGroupInfo.MOCK_STACK_NAME, 
            LOGICAL_RESOURCE_ID, 
            json.dumps(id_data, sort_keys=True)
        )

        event = {
            'RequestType': 'Update',
            'StackId': MockResourceGroupInfo.MOCK_STACK_ARN,
            'LogicalResourceId': LOGICAL_RESOURCE_ID,
            'PhysicalResourceId': physical_resource_id,
            'ResourceProperties': FULL_RESOURCE_PROPERTIES
        }

        context = {}

        expected_data = {
            'Url': EXPECTED_URL
        }

        # Execute

        Custom_ServiceApi.handler(event, context)

        # Validate

        role_utils.get_access_control_role_arn.assert_called_once_with(
            id_data,
            LOGICAL_RESOURCE_ID)

        Custom_ServiceApi.get_configured_swagger_content.assert_called_once_with(
            RESOURCE_GROUP_INFO,
            PROPS_MATCHER,
            ROLE_ARN,
            REST_API_RESOURCE_NAME)

        Custom_ServiceApi.update_api_gateway.assert_called_once_with(
            REST_API_ID,
            PROPS_MATCHER,
            SWAGGER_CONTENT)

        custom_resource_response.success_response.assert_called_once_with(
            expected_data,
            physical_resource_id)

        Custom_ServiceApi.register_service_interfaces.assert_called_once_with(
            RESOURCE_GROUP_INFO, 
            'https://{rest_api_id}.execute-api.{region}.amazonaws.com/{stage_name}'.format(
                rest_api_id = REST_API_ID,
                region = RESOURCE_GROUP_INFO.region,
                stage_name = Custom_ServiceApi.STAGE_NAME
            ), 
            SWAGGER_CONTENT)

    @mock.patch.object(custom_resource_response, 'success_response')
    @mock.patch.object(role_utils, 'get_access_control_role_arn', return_value = ROLE_ARN)
    @mock.patch.object(stack_info.StackInfoManager, 'get_stack_info', return_value=RESOURCE_GROUP_INFO)
    @mock.patch.object(Custom_ServiceApi, 'get_configured_swagger_content', return_value = SWAGGER_CONTENT)
    @mock.patch.object(Custom_ServiceApi, 'update_api_gateway')
    @mock.patch.object(Custom_ServiceApi, 'create_documentation_version')
    @mock.patch.object(Custom_ServiceApi, 'register_service_interfaces')
    def test_Update_with_minimal_properties(self, *args):

        # Setup

        id_data = { 'AbstractRoleMappings': { LOGICAL_RESOURCE_ID : ROLE_NAME }, 'RestApiId': REST_API_ID }

        physical_resource_id = '{}-{}::{}'.format(
            MockResourceGroupInfo.MOCK_STACK_NAME, 
            LOGICAL_RESOURCE_ID, 
            json.dumps(id_data, sort_keys=True)
        )

        event = {
            'RequestType': 'Update',
            'StackId': MockResourceGroupInfo.MOCK_STACK_ARN,
            'LogicalResourceId': LOGICAL_RESOURCE_ID,
            'PhysicalResourceId': physical_resource_id,
            'ResourceProperties': MINIMAL_RESOURCE_PROPERTIES
        }

        context = {}

        expected_data = {
            'Url': EXPECTED_URL
        }

        # Execute

        Custom_ServiceApi.handler(event, context)

        # Validate

        role_utils.get_access_control_role_arn.assert_called_once_with(
            id_data, 
            LOGICAL_RESOURCE_ID)

        Custom_ServiceApi.get_configured_swagger_content.assert_called_once_with(
            RESOURCE_GROUP_INFO,
            PROPS_MATCHER,
            ROLE_ARN,
            REST_API_RESOURCE_NAME)

        Custom_ServiceApi.update_api_gateway.assert_called_once_with(
            REST_API_ID,
            PROPS_MATCHER,
            SWAGGER_CONTENT)

        custom_resource_response.success_response.assert_called_once_with(
            expected_data,
            physical_resource_id)

        Custom_ServiceApi.register_service_interfaces.assert_called_once_with(
            RESOURCE_GROUP_INFO, 
            'https://{rest_api_id}.execute-api.{region}.amazonaws.com/{stage_name}'.format(
                rest_api_id = REST_API_ID,
                region = RESOURCE_GROUP_INFO.region,
                stage_name = Custom_ServiceApi.STAGE_NAME
            ), 
            SWAGGER_CONTENT)


    @mock.patch.object(custom_resource_response, 'success_response')
    @mock.patch.object(role_utils, 'delete_access_control_role')
    @mock.patch.object(stack_info.StackInfoManager, 'get_stack_info', return_value=RESOURCE_GROUP_INFO)
    @mock.patch.object(Custom_ServiceApi, 'delete_api_gateway')
    @mock.patch.object(Custom_ServiceApi, 'unregister_service_interfaces')
    def test_Delete(self, *args):

        # Setup

        id_data = { 'AbstractRoleMappings': { LOGICAL_RESOURCE_ID : ROLE_NAME }, 'RestApiId': REST_API_ID }

        physical_resource_id = '{}-{}::{}'.format(
            MockResourceGroupInfo.MOCK_STACK_NAME, 
            LOGICAL_RESOURCE_ID, 
            json.dumps(id_data, sort_keys=True)
        )

        event = {
            'RequestType': 'Delete',
            'StackId': MockResourceGroupInfo.MOCK_STACK_ARN,
            'LogicalResourceId': LOGICAL_RESOURCE_ID,
            'PhysicalResourceId': physical_resource_id
        }

        context = {}

        expected_data = {}

        # Execute

        Custom_ServiceApi.handler(event, context)

        # Validate

        role_utils.delete_access_control_role.assert_called_once_with(
            { 'AbstractRoleMappings': { LOGICAL_RESOURCE_ID : ROLE_NAME } },
            LOGICAL_RESOURCE_ID)

        Custom_ServiceApi.delete_api_gateway(
            REST_API_ID)

        expected_physical_resource_id = '{}-{}::{}'.format(
            MockResourceGroupInfo.MOCK_STACK_NAME, 
            LOGICAL_RESOURCE_ID, 
            json.dumps({ 'AbstractRoleMappings': { LOGICAL_RESOURCE_ID : ROLE_NAME } }, sort_keys=True)
        )

        custom_resource_response.success_response.assert_called_once_with(
            expected_data,
            expected_physical_resource_id)

        Custom_ServiceApi.unregister_service_interfaces.assert_called_once_with(
            RESOURCE_GROUP_INFO, 
            'https://{rest_api_id}.execute-api.{region}.amazonaws.com/{stage_name}'.format(
                rest_api_id = REST_API_ID,
                region = RESOURCE_GROUP_INFO.region,
                stage_name = Custom_ServiceApi.STAGE_NAME
            ))

class UnitTest_CloudGemFramework_ProjectResourceHandler_Custom_ServiceApi_SwaggerConfiguration(unittest.TestCase):

    INPUT_SWAGGER_CONTENT = 'Test Swagger $TestSetting1$ $TestSetting1$ $TestSetting2$ $ResourceGroupName$ $DeploymentName$ $RoleArn$ $Region$ $RestApiResourceName$'

    SWAGGER_SETTINGS = { 'TestSetting1': 'A', 'TestSetting2': 'B' }

    OUTPUT_SWAGGER_CONTENT = 'Test Swagger A A B {} {} {} {} {}'.format(
        MockResourceGroupInfo.MOCK_RESOURCE_GROUP_NAME, 
        MockDeploymentInfo.MOCK_DEPLOYMENT_NAME,
        ROLE_ARN,
        MockResourceGroupInfo.MOCK_REGION,
        REST_API_RESOURCE_NAME)

    RESOURCE_PROPERTIES = {
        'ConfigurationBucket': CONFIGURATION_BUCKET,
        'ConfigurationKey': CONFIGURATION_KEY,
        'SwaggerSettings': SWAGGER_SETTINGS
    }

    PROPS = properties._Properties(RESOURCE_PROPERTIES, Custom_ServiceApi.PROPERTY_SCHEMA)

    @mock.patch.object(Custom_ServiceApi, 's3')
    def test_get_input_swagger_content(self, *args):

        # Setup 

        Custom_ServiceApi.s3.get_object.return_value = mock_aws.s3_get_object_response(self.INPUT_SWAGGER_CONTENT)

        # Execute

        result = Custom_ServiceApi.get_input_swagger_content(self.PROPS)

        # Verify

        Custom_ServiceApi.s3.get_object.assert_called_once_with(
            Bucket = CONFIGURATION_BUCKET,
            Key = INPUT_SWAGGER_KEY)

        self.assertEquals(self.INPUT_SWAGGER_CONTENT, result)


    def test_configure_swagger_content(self, *args):

        # Execute

        result = Custom_ServiceApi.configure_swagger_content(
            RESOURCE_GROUP_INFO, 
            self.PROPS, 
            ROLE_ARN, 
            REST_API_RESOURCE_NAME, 
            self.INPUT_SWAGGER_CONTENT)

        # Verify

        self.assertEquals(self.OUTPUT_SWAGGER_CONTENT, result)

class UnitTest_CloudGemFramework_ProjectResourceHandler_Custom_ServiceApi_ApiGateway(unittest.TestCase):

    @mock.patch.object(Custom_ServiceApi, 'import_rest_api', return_value = REST_API_ID)
    @mock.patch.object(Custom_ServiceApi, 'compute_swagger_digest', return_value = SWAGGER_DIGEST)
    @mock.patch.object(Custom_ServiceApi, 'create_rest_api_deployment')
    @mock.patch.object(Custom_ServiceApi, 'update_rest_api_stage')
    @mock.patch.object(Custom_ServiceApi, 'create_documentation_version')
    def test_create_api_gateway(self, *args):

        # Execute

        result = Custom_ServiceApi.create_api_gateway(
            FULL_PROPS, 
            SWAGGER_CONTENT)

        # Verify

        Custom_ServiceApi.import_rest_api.assert_called_once_with(
            SWAGGER_CONTENT)

        Custom_ServiceApi.create_rest_api_deployment.assert_called_once_with(
            REST_API_ID, 
            SWAGGER_DIGEST)

        Custom_ServiceApi.update_rest_api_stage(
            REST_API_ID,
            FULL_PROPS)

        self.assertEquals(REST_API_ID, result)


    @mock.patch.object(Custom_ServiceApi, 'put_rest_api')
    @mock.patch.object(Custom_ServiceApi, 'get_rest_api_deployment_id', return_value = REST_API_DEPLOYMENT_ID)
    @mock.patch.object(Custom_ServiceApi, 'detect_swagger_changes', return_value = SWAGGER_DIGEST)
    @mock.patch.object(Custom_ServiceApi, 'create_rest_api_deployment')
    @mock.patch.object(Custom_ServiceApi, 'update_rest_api_stage')
    @mock.patch.object(Custom_ServiceApi, 'create_documentation_version')
    def test_update_api_gateway_with_swagger_changes(self, *args):

        # Execute

        Custom_ServiceApi.update_api_gateway(
            REST_API_ID, 
            FULL_PROPS,
            SWAGGER_CONTENT)

        # Verify

        Custom_ServiceApi.get_rest_api_deployment_id.assert_called_once_with(
            REST_API_ID)

        Custom_ServiceApi.detect_swagger_changes.assert_called_once_with(
            REST_API_ID, 
            REST_API_DEPLOYMENT_ID, 
            SWAGGER_CONTENT)

        Custom_ServiceApi.put_rest_api.assert_called_once_with(
            REST_API_ID,
            SWAGGER_CONTENT)

        Custom_ServiceApi.create_rest_api_deployment.assert_called_once_with(
            REST_API_ID, 
            SWAGGER_DIGEST)

        Custom_ServiceApi.update_rest_api_stage.assert_called_once_with(
            REST_API_ID,
            FULL_PROPS)


    @mock.patch.object(Custom_ServiceApi, 'put_rest_api')
    @mock.patch.object(Custom_ServiceApi, 'get_rest_api_deployment_id', return_value = REST_API_DEPLOYMENT_ID)
    @mock.patch.object(Custom_ServiceApi, 'detect_swagger_changes', return_value = None)
    @mock.patch.object(Custom_ServiceApi, 'update_rest_api_stage')
    @mock.patch.object(Custom_ServiceApi, 'api_gateway')
    def test_update_api_gateway_without_swagger_changes(self, *args):

        # Execute

        Custom_ServiceApi.update_api_gateway(
            REST_API_ID, 
            FULL_PROPS, 
            SWAGGER_CONTENT)

        # Verify

        Custom_ServiceApi.get_rest_api_deployment_id.assert_called_once_with(
            REST_API_ID)

        Custom_ServiceApi.detect_swagger_changes.assert_called_once_with(
            REST_API_ID, 
            REST_API_DEPLOYMENT_ID, 
            SWAGGER_CONTENT)

        Custom_ServiceApi.update_rest_api_stage.assert_called_once_with(
            REST_API_ID,
            FULL_PROPS)

        self.assertFalse(Custom_ServiceApi.api_gateway.put_rest_api.called)


    @mock.patch.object(Custom_ServiceApi, 'delete_rest_api')
    def test_delete_api_gateway(self, *args):

        # Execute

        Custom_ServiceApi.delete_api_gateway(
            REST_API_ID)

        # Verify

        Custom_ServiceApi.delete_rest_api.assert_called_once_with(
            REST_API_ID)


    @mock.patch.object(Custom_ServiceApi, 'api_gateway')
    def test_delete_rest_api(self, *args):

        # Execute

        Custom_ServiceApi.delete_rest_api(
            REST_API_ID)

        # Verify

        kwargs = {
            'restApiId': REST_API_ID
        }

        Custom_ServiceApi.api_gateway.delete_rest_api(**kwargs)


    @mock.patch.object(Custom_ServiceApi, 'api_gateway')
    @mock.patch.object(Custom_ServiceApi, 'compute_swagger_digest', return_value = SWAGGER_DIGEST_A)
    def test_detect_swagger_changes_with_changes(self, *args):

        # Setup

        Custom_ServiceApi.api_gateway.get_deployment.return_value = { 
            'description': SWAGGER_DIGEST_B 
        }

        # Execute

        result = Custom_ServiceApi.detect_swagger_changes(
            REST_API_ID,
            REST_API_DEPLOYMENT_ID,
            SWAGGER_CONTENT)

        # Verify

        self.assertEquals(result, SWAGGER_DIGEST_A)


    @mock.patch.object(Custom_ServiceApi, 'api_gateway')
    @mock.patch.object(Custom_ServiceApi, 'compute_swagger_digest', return_value = SWAGGER_DIGEST_A)
    def test_detect_swagger_changes_without_changes(self, *args):

        # Setup

        Custom_ServiceApi.api_gateway.get_deployment.return_value = { 
            'description': SWAGGER_DIGEST_A 
        }

        # Execute

        result = Custom_ServiceApi.detect_swagger_changes(
            REST_API_ID,
            REST_API_DEPLOYMENT_ID,
            SWAGGER_CONTENT)

        # Verify

        self.assertIsNone(result)


    @mock.patch.object(Custom_ServiceApi, 'api_gateway')
    def test_import_rest_api(self, *args):

        # Execute

        Custom_ServiceApi.import_rest_api(
            SWAGGER_CONTENT)

        # Verify

        kwargs = {
            "failOnWarnings": True,
            "body": SWAGGER_CONTENT,
            'baseBackoff': 1.5,
            'maxBackoff': 90.0
        }

        Custom_ServiceApi.api_gateway.import_rest_api.assert_called_once_with(**kwargs)


    @mock.patch.object(Custom_ServiceApi, 'api_gateway')
    def test_put_rest_api(self, *args):

        # Execute

        Custom_ServiceApi.put_rest_api(
            REST_API_ID,
            SWAGGER_CONTENT)

        # Verify

        kwargs = {
            "failOnWarnings": True,
            "body": SWAGGER_CONTENT,
            "mode": "overwrite",
            "restApiId": REST_API_ID
        }

        Custom_ServiceApi.api_gateway.put_rest_api.assert_called_once_with(**kwargs)


    @mock.patch.object(Custom_ServiceApi, 'api_gateway')
    def test_create_rest_api_deployment(self, *args):

        # Execute

        Custom_ServiceApi.create_rest_api_deployment(
            REST_API_ID, 
            SWAGGER_DIGEST)

        # Verify

        kwargs = {
            'restApiId': REST_API_ID,
            'description': SWAGGER_DIGEST,
            'stageName': Custom_ServiceApi.STAGE_NAME
        }

        Custom_ServiceApi.api_gateway.create_deployment.assert_called_once_with(**kwargs)


    @mock.patch.object(Custom_ServiceApi, 'api_gateway')
    @mock.patch.object(Custom_ServiceApi, 'get_rest_api_stage_update_patch_operations', return_value = MOCK_PATCH_OPERATIONS)
    def test_update_rest_api_stage_with_patch_operations(self, *args):

        # Setup

        mock_stage = { 'MockStage': '' }

        Custom_ServiceApi.api_gateway.get_stage.return_value = mock_stage

        # Execute
        
        Custom_ServiceApi.update_rest_api_stage(
            REST_API_ID,
            FULL_PROPS)

        # Verify

        kwargs = {
            'restApiId': REST_API_ID,
            'stageName': Custom_ServiceApi.STAGE_NAME,
        }

        Custom_ServiceApi.api_gateway.get_stage.assert_called_once_with(**kwargs)


        Custom_ServiceApi.get_rest_api_stage_update_patch_operations.assert_called_once_with(
            mock_stage,
            FULL_PROPS)

        kwargs = {
            'restApiId': REST_API_ID,
            'patchOperations': MOCK_PATCH_OPERATIONS,
            'stageName': Custom_ServiceApi.STAGE_NAME
        }

        Custom_ServiceApi.api_gateway.update_stage.assert_called_once_with(**kwargs)


    @mock.patch.object(Custom_ServiceApi, 'api_gateway')
    @mock.patch.object(Custom_ServiceApi, 'get_rest_api_stage_update_patch_operations', return_value = EMPTY_PATCH_OPERATIONS)
    def test_update_rest_api_stage_without_patch_operations(self, *args):

        # Setup

        mock_stage = { 'MockStage': '' }

        Custom_ServiceApi.api_gateway.get_stage.return_value = mock_stage

        # Execute
        
        Custom_ServiceApi.update_rest_api_stage(
            REST_API_ID,
            FULL_PROPS)

        # Verify

        kwargs = {
            'restApiId': REST_API_ID,
            'stageName': Custom_ServiceApi.STAGE_NAME,
        }

        Custom_ServiceApi.api_gateway.get_stage.assert_called_once_with(**kwargs)

        Custom_ServiceApi.get_rest_api_stage_update_patch_operations.assert_called_once_with(
            mock_stage,
            FULL_PROPS)

        self.assertFalse(Custom_ServiceApi.api_gateway.update_stage.called)


    @mock.patch.object(Custom_ServiceApi, 'api_gateway')
    def test_get_rest_api_deployment_id(self, *args):

        # Setup

        Custom_ServiceApi.api_gateway.get_stage.return_value = {
            'deploymentId': REST_API_DEPLOYMENT_ID
        }

        # Execute

        Custom_ServiceApi.get_rest_api_deployment_id(REST_API_ID)

        # Verify

        kwargs = {
            'restApiId': REST_API_ID,
            'stageName': Custom_ServiceApi.STAGE_NAME
        }

        Custom_ServiceApi.api_gateway.get_stage.assert_called_once_with(**kwargs)


class UnitTest_CloudGemFramework_ProjectResourceHandler_Custom_ServiceApi_Interfaces(unittest.TestCase):

    @mock.patch.object(Custom_ServiceApi, 'get_interface_swagger')
    @mock.patch.object(Custom_ServiceApi, 'get_service_directory')
    def test_register_service_interfaces(self, *args):

        interface_id = 'test-interface-id'
        interface_metadata = {
            'basePath': '/base-path',
            'paths': ['path'],
            'definitions': ['definition']
        }
        stack_info = mock.MagicMock()
        stack_info.is_project_stack = False
        service_url = 'https://host:port/path'
        swagger = {
            service_interface.INTERFACE_METADATA_OBJECT_NAME: {
                interface_id: interface_metadata
            }
        }
        swagger_content = json.dumps(swagger)

        Custom_ServiceApi.register_service_interfaces(
            stack_info, 
            service_url, 
            swagger_content)

        Custom_ServiceApi.get_interface_swagger.assert_called_once_with(
            swagger, 
            interface_id, 
            interface_metadata['basePath'],
            interface_metadata['paths'],
            interface_metadata['definitions'])

        Custom_ServiceApi.get_service_directory.assert_called_once_with(
            stack_info)

        Custom_ServiceApi.get_service_directory.return_value.put_service_interfaces.assert_called_once_with(
            stack_info.deployment.deployment_name,
            service_url,
            [ 
                { 
                    'InterfaceId': interface_id, 
                    'InterfaceUrl': service_url + interface_metadata['basePath'],
                    'InterfaceSwagger': Custom_ServiceApi.get_interface_swagger.return_value
                }
            ]
        )


if __name__ == '__main__':
    unittest.main()
