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

import custom_resource_response
import role_utils
from resource_manager_common import stack_info
import properties
from resource_manager_common.test import mock_aws
import ServiceApiResourceHandler 
from resource_manager_common.test.mock_stack_info import MockResourceGroupInfo, MockDeploymentInfo
from mock_properties import PropertiesMatcher


ROLE_NAME = 'TestRoleName'
ROLE_ARN = 'TestRoleArn'
LOGICAL_RESOURCE_ID = 'TestLogicalResoureId'
REST_API_RESOURCE_NAME = MockResourceGroupInfo.MOCK_STACK_NAME + '-' + LOGICAL_RESOURCE_ID
CONFIGURATION_BUCKET = 'TestBucket'
CONFIGURATION_KEY = 'test-key'
INPUT_SWAGGER_KEY = CONFIGURATION_KEY + '/swagger.json'
REST_API_ID = 'TestRestApiId'
EXPECTED_URL = 'https://{}.execute-api.{}.amazonaws.com/{}'.format(REST_API_ID, MockResourceGroupInfo.MOCK_REGION, ServiceApiResourceHandler.STAGE_NAME)
SWAGGER_CONTENT = 'TestSwaggerContent'
RESOURCE_GROUP_INFO = MockResourceGroupInfo()
SWAGGER_DIGEST = 'TestSwaggerDigest'
SWAGGER_DIGEST_A = 'TestSwaggerDigestA'
SWAGGER_DIGEST_B = 'TestSwaggerDigestB'
MOCK_PATCH_OPERATIONS = [ 'Mock Patch Operation' ]
EMPTY_PATCH_OPERATIONS = []
REST_API_DEPLOYMENT_ID = 'TEstRestApiDeploymentId'

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

FULL_PROPS = properties._Properties(FULL_RESOURCE_PROPERTIES, ServiceApiResourceHandler.PROPERTY_SCHEMA)
MINIMAL_PROPS = properties._Properties(MINIMAL_RESOURCE_PROPERTIES, ServiceApiResourceHandler.PROPERTY_SCHEMA)

PROPS_MATCHER = PropertiesMatcher()

class UnitTest_CloudGemFramework_ProjectCode_ServiceApiResourceHandler_RequestTypes(unittest.TestCase):

    @mock.patch.object(custom_resource_response, 'succeed')
    @mock.patch.object(role_utils, 'create_access_control_role', return_value = ROLE_ARN)
    @mock.patch.object(stack_info, 'get_stack_info', return_value = RESOURCE_GROUP_INFO)
    @mock.patch.object(ServiceApiResourceHandler, 'get_configured_swagger_content', return_value = SWAGGER_CONTENT)
    @mock.patch.object(ServiceApiResourceHandler, 'create_api_gateway', return_value = REST_API_ID)
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

        ServiceApiResourceHandler.handler(event, context)

        # Validate

        role_utils.create_access_control_role.assert_called_once_with(
            {'RestApiId': REST_API_ID},
            MockResourceGroupInfo.MOCK_STACK_ARN, 
            LOGICAL_RESOURCE_ID, 
            ServiceApiResourceHandler.API_GATEWAY_SERVICE_NAME)

        ServiceApiResourceHandler.get_configured_swagger_content.assert_called_once_with(
            RESOURCE_GROUP_INFO,
            PROPS_MATCHER,
            ROLE_ARN,
            REST_API_RESOURCE_NAME)

        ServiceApiResourceHandler.create_api_gateway.assert_called_once_with(
            PROPS_MATCHER,
            SWAGGER_CONTENT)

        expected_physical_resource_id = '{}-{}::{}'.format(MockResourceGroupInfo.MOCK_STACK_NAME, LOGICAL_RESOURCE_ID, json.dumps({"RestApiId": REST_API_ID}))

        custom_resource_response.succeed.assert_called_once_with(
            event, 
            context, 
            expected_data, 
            expected_physical_resource_id)


    @mock.patch.object(custom_resource_response, 'succeed')
    @mock.patch.object(role_utils, 'create_access_control_role', return_value = ROLE_ARN)
    @mock.patch.object(stack_info, 'get_stack_info', return_value=RESOURCE_GROUP_INFO)
    @mock.patch.object(ServiceApiResourceHandler, 'get_configured_swagger_content', return_value = SWAGGER_CONTENT)
    @mock.patch.object(ServiceApiResourceHandler, 'create_api_gateway', return_value = REST_API_ID)
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

        ServiceApiResourceHandler.handler(event, context)

        # Validate

        role_utils.create_access_control_role.assert_called_once_with(
            {'RestApiId': REST_API_ID},
            MockResourceGroupInfo.MOCK_STACK_ARN, 
            LOGICAL_RESOURCE_ID, 
            ServiceApiResourceHandler.API_GATEWAY_SERVICE_NAME)

        ServiceApiResourceHandler.get_configured_swagger_content.assert_called_once_with(
            RESOURCE_GROUP_INFO,
            PROPS_MATCHER,
            ROLE_ARN,
            REST_API_RESOURCE_NAME)

        ServiceApiResourceHandler.create_api_gateway.assert_called_once_with(
            PROPS_MATCHER,
            SWAGGER_CONTENT)

        expected_physical_resource_id = MockResourceGroupInfo.MOCK_STACK_NAME + '-' + LOGICAL_RESOURCE_ID + '::{"RestApiId": "TestRestApiId"}'

        custom_resource_response.succeed.assert_called_once_with(
            event, 
            context, 
            expected_data, 
            expected_physical_resource_id)


    @mock.patch.object(custom_resource_response, 'succeed')
    @mock.patch.object(role_utils, 'get_access_control_role_arn', return_value = ROLE_ARN)
    @mock.patch.object(stack_info, 'get_stack_info', return_value=RESOURCE_GROUP_INFO)
    @mock.patch.object(ServiceApiResourceHandler, 'get_configured_swagger_content', return_value = SWAGGER_CONTENT)
    @mock.patch.object(ServiceApiResourceHandler, 'update_api_gateway')
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

        ServiceApiResourceHandler.handler(event, context)

        # Validate

        role_utils.get_access_control_role_arn.assert_called_once_with(
            id_data, 
            LOGICAL_RESOURCE_ID)

        ServiceApiResourceHandler.get_configured_swagger_content.assert_called_once_with(
            RESOURCE_GROUP_INFO,
            PROPS_MATCHER,
            ROLE_ARN,
            REST_API_RESOURCE_NAME)

        ServiceApiResourceHandler.update_api_gateway.assert_called_once_with(
            REST_API_ID,
            PROPS_MATCHER,
            SWAGGER_CONTENT)

        custom_resource_response.succeed.assert_called_once_with(
            event, 
            context, 
            expected_data, 
            physical_resource_id)


    @mock.patch.object(custom_resource_response, 'succeed')
    @mock.patch.object(role_utils, 'get_access_control_role_arn', return_value = ROLE_ARN)
    @mock.patch.object(stack_info, 'get_stack_info', return_value=RESOURCE_GROUP_INFO)
    @mock.patch.object(ServiceApiResourceHandler, 'get_configured_swagger_content', return_value = SWAGGER_CONTENT)
    @mock.patch.object(ServiceApiResourceHandler, 'update_api_gateway')
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

        ServiceApiResourceHandler.handler(event, context)

        # Validate

        role_utils.get_access_control_role_arn.assert_called_once_with(
            id_data, 
            LOGICAL_RESOURCE_ID)

        ServiceApiResourceHandler.get_configured_swagger_content.assert_called_once_with(
            RESOURCE_GROUP_INFO,
            PROPS_MATCHER,
            ROLE_ARN,
            REST_API_RESOURCE_NAME)

        ServiceApiResourceHandler.update_api_gateway.assert_called_once_with(
            REST_API_ID,
            PROPS_MATCHER,
            SWAGGER_CONTENT)

        custom_resource_response.succeed.assert_called_once_with(
            event, 
            context, 
            expected_data, 
            physical_resource_id)


    @mock.patch.object(custom_resource_response, 'succeed')
    @mock.patch.object(role_utils, 'delete_access_control_role')
    @mock.patch.object(stack_info, 'get_stack_info', return_value=RESOURCE_GROUP_INFO)
    @mock.patch.object(ServiceApiResourceHandler, 'delete_api_gateway')
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

        ServiceApiResourceHandler.handler(event, context)

        # Validate

        role_utils.delete_access_control_role.assert_called_once_with(
            { 'AbstractRoleMappings': { LOGICAL_RESOURCE_ID : ROLE_NAME } },
            LOGICAL_RESOURCE_ID)

        ServiceApiResourceHandler.delete_api_gateway(
            REST_API_ID)

        expected_physical_resource_id = '{}-{}::{}'.format(
            MockResourceGroupInfo.MOCK_STACK_NAME, 
            LOGICAL_RESOURCE_ID, 
            json.dumps({ 'AbstractRoleMappings': { LOGICAL_RESOURCE_ID : ROLE_NAME } }, sort_keys=True)
        )

        custom_resource_response.succeed.assert_called_once_with(
            event, 
            context, 
            expected_data, 
            expected_physical_resource_id)


class UnitTest_CloudGemFramework_ProjectCode_ServiceApiResourceHandler_SwaggerConfiguration(unittest.TestCase):

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

    PROPS = properties._Properties(RESOURCE_PROPERTIES, ServiceApiResourceHandler.PROPERTY_SCHEMA)

    @mock.patch.object(ServiceApiResourceHandler, 's3')
    def test_get_input_swagger_content(self, *args):

        # Setup 

        ServiceApiResourceHandler.s3.get_object.return_value = mock_aws.s3_get_object_response(self.INPUT_SWAGGER_CONTENT)

        # Execute

        result = ServiceApiResourceHandler.get_input_swagger_content(self.PROPS)

        # Verify

        ServiceApiResourceHandler.s3.get_object.assert_called_once_with(
            Bucket = CONFIGURATION_BUCKET,
            Key = INPUT_SWAGGER_KEY)

        self.assertEquals(self.INPUT_SWAGGER_CONTENT, result)


    def test_configure_swagger_content(self, *args):

        # Execute

        result = ServiceApiResourceHandler.configure_swagger_content(
            RESOURCE_GROUP_INFO, 
            self.PROPS, 
            ROLE_ARN, 
            REST_API_RESOURCE_NAME, 
            self.INPUT_SWAGGER_CONTENT)

        # Verify

        self.assertEquals(self.OUTPUT_SWAGGER_CONTENT, result)



class UnitTest_CloudGemFramework_ProjectCode_ServiceApiResourceHandler_ApiGateway(unittest.TestCase):

    @mock.patch.object(ServiceApiResourceHandler, 'import_rest_api', return_value = REST_API_ID)
    @mock.patch.object(ServiceApiResourceHandler, 'compute_swagger_digest', return_value = SWAGGER_DIGEST)
    @mock.patch.object(ServiceApiResourceHandler, 'create_rest_api_deployment')
    @mock.patch.object(ServiceApiResourceHandler, 'update_rest_api_stage')
    def test_create_api_gateway(self, *args):

        # Execute

        result = ServiceApiResourceHandler.create_api_gateway(
            FULL_PROPS, 
            SWAGGER_CONTENT)

        # Verify

        ServiceApiResourceHandler.import_rest_api.assert_called_once_with(
            SWAGGER_CONTENT)

        ServiceApiResourceHandler.create_rest_api_deployment.assert_called_once_with(
            REST_API_ID, 
            SWAGGER_DIGEST)

        ServiceApiResourceHandler.update_rest_api_stage(
            REST_API_ID,
            FULL_PROPS)

        self.assertEquals(REST_API_ID, result)


    @mock.patch.object(ServiceApiResourceHandler, 'put_rest_api')
    @mock.patch.object(ServiceApiResourceHandler, 'get_rest_api_deployment_id', return_value = REST_API_DEPLOYMENT_ID)
    @mock.patch.object(ServiceApiResourceHandler, 'detect_swagger_changes', return_value = SWAGGER_DIGEST)
    @mock.patch.object(ServiceApiResourceHandler, 'create_rest_api_deployment')
    @mock.patch.object(ServiceApiResourceHandler, 'update_rest_api_stage')
    def test_update_api_gateway_with_swagger_changes(self, *args):

        # Execute

        ServiceApiResourceHandler.update_api_gateway(
            REST_API_ID, 
            FULL_PROPS,
            SWAGGER_CONTENT)

        # Verify

        ServiceApiResourceHandler.get_rest_api_deployment_id.assert_called_once_with(
            REST_API_ID)

        ServiceApiResourceHandler.detect_swagger_changes.assert_called_once_with(
            REST_API_ID, 
            REST_API_DEPLOYMENT_ID, 
            SWAGGER_CONTENT)

        ServiceApiResourceHandler.put_rest_api.assert_called_once_with(
            REST_API_ID,
            SWAGGER_CONTENT)

        ServiceApiResourceHandler.create_rest_api_deployment.assert_called_once_with(
            REST_API_ID, 
            SWAGGER_DIGEST)

        ServiceApiResourceHandler.update_rest_api_stage.assert_called_once_with(
            REST_API_ID,
            FULL_PROPS)


    @mock.patch.object(ServiceApiResourceHandler, 'put_rest_api')
    @mock.patch.object(ServiceApiResourceHandler, 'get_rest_api_deployment_id', return_value = REST_API_DEPLOYMENT_ID)
    @mock.patch.object(ServiceApiResourceHandler, 'detect_swagger_changes', return_value = None)
    @mock.patch.object(ServiceApiResourceHandler, 'update_rest_api_stage')
    @mock.patch.object(ServiceApiResourceHandler, 'api_gateway')
    def test_update_api_gateway_without_swagger_changes(self, *args):

        # Execute

        ServiceApiResourceHandler.update_api_gateway(
            REST_API_ID, 
            FULL_PROPS, 
            SWAGGER_CONTENT)

        # Verify

        ServiceApiResourceHandler.get_rest_api_deployment_id.assert_called_once_with(
            REST_API_ID)

        ServiceApiResourceHandler.detect_swagger_changes.assert_called_once_with(
            REST_API_ID, 
            REST_API_DEPLOYMENT_ID, 
            SWAGGER_CONTENT)

        ServiceApiResourceHandler.update_rest_api_stage.assert_called_once_with(
            REST_API_ID,
            FULL_PROPS)

        self.assertFalse(ServiceApiResourceHandler.api_gateway.put_rest_api.called)


    @mock.patch.object(ServiceApiResourceHandler, 'delete_rest_api')
    def test_delete_api_gateway(self, *args):

        # Execute

        ServiceApiResourceHandler.delete_api_gateway(
            REST_API_ID)

        # Verify

        ServiceApiResourceHandler.delete_rest_api.assert_called_once_with(
            REST_API_ID)


    @mock.patch.object(ServiceApiResourceHandler, 'api_gateway')
    def test_delete_rest_api(self, *args):

        # Execute

        ServiceApiResourceHandler.delete_rest_api(
            REST_API_ID)

        # Verify

        kwargs = {
            'restApiId': REST_API_ID
        }

        ServiceApiResourceHandler.api_gateway.delete_rest_api(**kwargs)


    @mock.patch.object(ServiceApiResourceHandler, 'api_gateway')
    @mock.patch.object(ServiceApiResourceHandler, 'compute_swagger_digest', return_value = SWAGGER_DIGEST_A)
    def test_detect_swagger_changes_with_changes(self, *args):

        # Setup

        ServiceApiResourceHandler.api_gateway.get_deployment.return_value = { 
            'description': SWAGGER_DIGEST_B 
        }

        # Execute

        result = ServiceApiResourceHandler.detect_swagger_changes(
            REST_API_ID,
            REST_API_DEPLOYMENT_ID,
            SWAGGER_CONTENT)

        # Verify

        self.assertEquals(result, SWAGGER_DIGEST_A)


    @mock.patch.object(ServiceApiResourceHandler, 'api_gateway')
    @mock.patch.object(ServiceApiResourceHandler, 'compute_swagger_digest', return_value = SWAGGER_DIGEST_A)
    def test_detect_swagger_changes_without_changes(self, *args):

        # Setup

        ServiceApiResourceHandler.api_gateway.get_deployment.return_value = { 
            'description': SWAGGER_DIGEST_A 
        }

        # Execute

        result = ServiceApiResourceHandler.detect_swagger_changes(
            REST_API_ID,
            REST_API_DEPLOYMENT_ID,
            SWAGGER_CONTENT)

        # Verify

        self.assertIsNone(result)


    @mock.patch.object(ServiceApiResourceHandler, 'api_gateway')
    def test_import_rest_api(self, *args):

        # Execute

        ServiceApiResourceHandler.import_rest_api(
            SWAGGER_CONTENT)

        # Verify

        kwargs = {
            "failOnWarnings": True,
            "body": SWAGGER_CONTENT
        }

        ServiceApiResourceHandler.api_gateway.import_rest_api.assert_called_once_with(**kwargs)


    @mock.patch.object(ServiceApiResourceHandler, 'api_gateway')
    def test_put_rest_api(self, *args):

        # Execute

        ServiceApiResourceHandler.put_rest_api(
            REST_API_ID,
            SWAGGER_CONTENT)

        # Verify

        kwargs = {
            "failOnWarnings": True,
            "body": SWAGGER_CONTENT,
            "mode": "overwrite",
            "restApiId": REST_API_ID
        }

        ServiceApiResourceHandler.api_gateway.put_rest_api.assert_called_once_with(**kwargs)


    @mock.patch.object(ServiceApiResourceHandler, 'api_gateway')
    def test_create_rest_api_deployment(self, *args):

        # Execute

        ServiceApiResourceHandler.create_rest_api_deployment(
            REST_API_ID, 
            SWAGGER_DIGEST)

        # Verify

        kwargs = {
            'restApiId': REST_API_ID,
            'description': SWAGGER_DIGEST,
            'stageName': ServiceApiResourceHandler.STAGE_NAME
        }

        ServiceApiResourceHandler.api_gateway.create_deployment.assert_called_once_with(**kwargs)


    @mock.patch.object(ServiceApiResourceHandler, 'api_gateway')
    @mock.patch.object(ServiceApiResourceHandler, 'get_rest_api_stage_update_patch_operations', return_value = MOCK_PATCH_OPERATIONS)
    def test_update_rest_api_stage_with_patch_operations(self, *args):

        # Setup

        mock_stage = { 'MockStage': '' }

        ServiceApiResourceHandler.api_gateway.get_stage.return_value = mock_stage

        # Execute
        
        ServiceApiResourceHandler.update_rest_api_stage(
            REST_API_ID,
            FULL_PROPS)

        # Verify

        kwargs = {
            'restApiId': REST_API_ID,
            'stageName': ServiceApiResourceHandler.STAGE_NAME,
        }

        ServiceApiResourceHandler.api_gateway.get_stage.assert_called_once_with(**kwargs)


        ServiceApiResourceHandler.get_rest_api_stage_update_patch_operations.assert_called_once_with(
            mock_stage,
            FULL_PROPS)

        kwargs = {
            'restApiId': REST_API_ID,
            'patchOperations': MOCK_PATCH_OPERATIONS,
            'stageName': ServiceApiResourceHandler.STAGE_NAME
        }

        ServiceApiResourceHandler.api_gateway.update_stage.assert_called_once_with(**kwargs)


    @mock.patch.object(ServiceApiResourceHandler, 'api_gateway')
    @mock.patch.object(ServiceApiResourceHandler, 'get_rest_api_stage_update_patch_operations', return_value = EMPTY_PATCH_OPERATIONS)
    def test_update_rest_api_stage_without_patch_operations(self, *args):

        # Setup

        mock_stage = { 'MockStage': '' }

        ServiceApiResourceHandler.api_gateway.get_stage.return_value = mock_stage

        # Execute
        
        ServiceApiResourceHandler.update_rest_api_stage(
            REST_API_ID,
            FULL_PROPS)

        # Verify

        kwargs = {
            'restApiId': REST_API_ID,
            'stageName': ServiceApiResourceHandler.STAGE_NAME,
        }

        ServiceApiResourceHandler.api_gateway.get_stage.assert_called_once_with(**kwargs)

        ServiceApiResourceHandler.get_rest_api_stage_update_patch_operations.assert_called_once_with(
            mock_stage,
            FULL_PROPS)

        self.assertFalse(ServiceApiResourceHandler.api_gateway.update_stage.called)


    @mock.patch.object(ServiceApiResourceHandler, 'api_gateway')
    def test_get_rest_api_deployment_id(self, *args):

        # Setup

        ServiceApiResourceHandler.api_gateway.get_stage.return_value = {
            'deploymentId': REST_API_DEPLOYMENT_ID
        }

        # Execute

        ServiceApiResourceHandler.get_rest_api_deployment_id(REST_API_ID)

        # Verify

        kwargs = {
            'restApiId': REST_API_ID,
            'stageName': ServiceApiResourceHandler.STAGE_NAME
        }

        ServiceApiResourceHandler.api_gateway.get_stage.assert_called_once_with(**kwargs)


if __name__ == '__main__':
    unittest.main()
