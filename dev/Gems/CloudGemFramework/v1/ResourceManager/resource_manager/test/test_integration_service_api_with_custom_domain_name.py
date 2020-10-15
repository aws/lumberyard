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

import json
import subprocess
import os
import time
import warnings

from resource_manager.test import lmbr_aws_test_support
from resource_manager.test import service_api_common_support
import resource_management
import cgf_service_client
from resource_manager.test import base_stack_test


class IntegrationTest_CloudGemFramework_ServiceApiWithCustomDomainName(
    service_api_common_support.BaseServiceApiTestCase):

    """ Integration tests for service API created with custom domain name.
        To run these tests:
        (1) Contact ly-infra to create a sub-domain name under your AWS own account
        (2) Follow all the steps mentioned in the following document to set up a custom domain name for API Gateway:
            https://aws.amazon.com/premiumsupport/knowledge-center/custom-domain-name-amazon-api-gateway/
    """

    # Replace the value of CUSTOM_DOMAIN_NAME with your own custom domain name.
    CUSTOM_DOMAIN_NAME = 'example.com'
    STAGE = 'api'

    def __init__(self, *args, **kwargs):
        super(IntegrationTest_CloudGemFramework_ServiceApiWithCustomDomainName, self).__init__(*args, **kwargs)

    def setUp(self):
        self.set_deployment_name(lmbr_aws_test_support.unique_name())
        self.set_resource_group_name(lmbr_aws_test_support.unique_name('customdomain'))
        self.prepare_test_environment("cloud_gem_framework_service_api_with_custom_domain_name_test")
        self.register_for_shared_resources()

        # Ignore warnings based on https://github.com/boto/boto3/issues/454 for now
        # Needs to be set per tests as its reset
        warnings.filterwarnings(action="ignore", message="unclosed", category=ResourceWarning)

    def __000_create_stacks(self):
        if not self.has_project_stack():
            self.lmbr_aws(
                'project', 'create',
                '--stack-name', self.TEST_PROJECT_STACK_NAME,
                '--confirm-aws-usage',
                '--confirm-security-change',
                '--admin-roles',
                '--region', lmbr_aws_test_support.REGION,
                '--custom-domain-name', self.CUSTOM_DOMAIN_NAME
            )
        self.setup_deployment_stacks()

    def __010_add_service_api_resources(self):
       self.add_service_api_resources()

    def __020_create_resources(self):
        self.create_resources()

    def __030_verify_service_api_resources(self):
        self.verify_service_api_resources()

    def __040_verify_service_url(self):
        self.__verify_service_url(self.CUSTOM_DOMAIN_NAME)

    def __050_call_simple_api(self):
        self.call_simple_api()

    def __060_call_simple_api_with_no_credentials(self):
        self.call_simple_api_with_no_credentials()

    def __100_add_complex_apis(self):
        self.add_complex_apis()

    def __110_add_apis_with_optional_parameters(self):
        self.add_apis_with_optional_parameters()

    def __120_add_interfaces(self):
        self.add_interfaces()

    def __130_add_legacy_plugin(self):
        self.add_legacy_plugin()

    def __200_update_deployment(self):
        self.update_deployment()

    def __210_verify_service_api_mapping(self):
        self.verify_service_api_mapping()

    def __300_call_complex_apis(self):
        self.call_complex_apis()

    def __301_call_complex_api_using_player_credentials(self):
        self.call_complex_api_using_player_credentials()

    def __302_call_complex_api_using_project_admin_credentials(self):
        self.call_complex_api_using_project_admin_credentials()

    def __303_call_complex_api_using_deployment_admin_credentials(self):
        self.call_complex_api_using_deployment_admin_credentials()

    def __304_call_complex_api_with_missing_string_pathparam(self):
        self.call_complex_api_with_missing_string_pathparam()

    def __305_call_complex_api_with_missing_string_queryparam(self):
        self.call_complex_api_with_missing_string_queryparam()

    def __306_call_complex_api_with_missing_string_bodyparam(self):
        self.call_complex_api_with_missing_string_bodyparam()

    def __307_call_api_with_both_parameters(self):
        self.call_api_with_both_parameters()

    def __308_call_api_without_bodyparam(self):
        self.call_api_without_bodyparam()

    def __309_call_api_without_queryparam(self):
        self.call_api_without_queryparam()

    def __400_call_test_plugin(self):
        self.call_test_plugin()

    def __500_call_interface_directly(self):
        self.call_interface_directly()

    def __501_call_interface_directly_with_player_credential(self):
        self.call_interface_directly_with_player_credential()

    def __502_call_interface_directly_without_credential(self):
        self.call_interface_directly_without_credential()

    def __503_invoke_interface_caller(self):
        self.invoke_interface_caller()

    def __700_run_cpp_tests(self):
        self.run_cpp_tests()

    def __800_set_gem_service_api_custom_resource_version(self):
        self.__set_gem_service_api_custom_resource_version()

    def __810_remove_custom_domain_name(self):
        self.lmbr_aws(
                'project', 'update',
                '--confirm-aws-usage',
                '--confirm-security-change'
            )
        self.lmbr_aws('resource-group', 'upload-resources', '--resource-group', self.TEST_RESOURCE_GROUP_NAME, '--deployment', self.TEST_DEPLOYMENT_NAME,
            '--confirm-aws-usage', '--confirm-security-change', '--confirm-resource-deletion')

        self.refresh_stack_description(self.get_resource_group_stack_arn(self.TEST_DEPLOYMENT_NAME, self.TEST_RESOURCE_GROUP_NAME))
        self.lmbr_aws('mappings', 'update', '-d', self.TEST_DEPLOYMENT_NAME, '--ignore-cache')

    def __820_verify_default_service_url(self):
        self.__verify_service_url()

    def __830_call_simple_api_using_default_service_url(self):
        self.call_simple_api()

    def __840_verify_service_api_mapping_with_default_service_url(self):
        self.verify_service_api_mapping()

    def __850_update_custom_domain_name(self):
        self.lmbr_aws(
                'project', 'update',
                '--custom-domain-name', self.CUSTOM_DOMAIN_NAME,
                '--confirm-aws-usage',
                '--confirm-security-change'
            )
        self.lmbr_aws('resource-group', 'upload-resources', '--resource-group', self.TEST_RESOURCE_GROUP_NAME, '--deployment', self.TEST_DEPLOYMENT_NAME,
            '--confirm-aws-usage', '--confirm-security-change', '--confirm-resource-deletion')

        self.refresh_stack_description(self.get_resource_group_stack_arn(self.TEST_DEPLOYMENT_NAME, self.TEST_RESOURCE_GROUP_NAME))
        self.lmbr_aws('mappings', 'update', '-d', self.TEST_DEPLOYMENT_NAME, '--ignore-cache')

    def __860_verify_alternative_service_url(self):
        self.__verify_service_url(self.CUSTOM_DOMAIN_NAME)

    def __870_call_simple_api_using_alternative_service_url(self):
        self.call_simple_api()

    def __880_verify_service_api_mapping_with_alternative_service_url(self):
        self.verify_service_api_mapping()

    def __900_remove_service_api_resources(self):
        self.remove_service_api_resources()

    def __910_delete_resources(self):
        self.delete_resources()

    def __999_cleanup(self):
        self.cleanup()

    def __verify_service_url(self, custom_domain_name=''):
        url = self.get_service_url(stack_id=self.get_resource_group_stack_arn(self.TEST_DEPLOYMENT_NAME, self.TEST_RESOURCE_GROUP_NAME))
        slash_parts = url.split('/')
        self.assertEqual(len(slash_parts), 4)

        if custom_domain_name:
            # Alternative service Url has the format of https://{custom_domain_name}/{region}.{stage_name}.{rest_api_id}
            self.assertEqual(slash_parts[2], custom_domain_name)

            dot_parts = slash_parts[3].split('.')
            self.assertEqual(len(dot_parts), 3)

            self.assertEqual(dot_parts[0], lmbr_aws_test_support.REGION)
            self.assertEqual(dot_parts[1], self.STAGE)
        else:
            # Default service Url has the format of https://{rest_api_id}.execute-api.{region}.amazonaws.com/{stage_name}
            self.assertEqual(slash_parts[3], self.STAGE)

            dot_parts = slash_parts[2].split('.')
            self.assertEqual(len(dot_parts), 5)
            self.assertEqual(dot_parts[1], 'execute-api')
            self.assertEqual(dot_parts[2], lmbr_aws_test_support.REGION)
            self.assertEqual(dot_parts[3], 'amazonaws')
            self.assertEqual(dot_parts[4], 'com')

    def __set_gem_service_api_custom_resource_version(self):
        # Set CustomResourceVersion to $LATEST so that we can use the latest version of the handler lambda 
        file_path = self.get_gem_aws_path(self.TEST_RESOURCE_GROUP_NAME, 'resource-template.json')
        with open(file_path, 'r') as file:
            template = json.load(file)

        if not template['Resources']['ServiceApi']['Metadata']:
            template['Resources']['ServiceApi']['Metadata'] = {}

        if not template['Resources']['ServiceApi']['Metadata']['CloudCanvas']:
            template['Resources']['ServiceApi']['Metadata']['CloudCanvas'] = {}

        template['Resources']['ServiceApi']['Metadata']['CloudCanvas'].update(
            {'CustomResourceVersion': '$LATEST'})

        with open(file_path, 'w') as file:
            json.dump(template, file, indent=4, sort_keys=True)
