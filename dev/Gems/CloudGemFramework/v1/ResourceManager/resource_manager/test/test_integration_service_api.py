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


class IntegrationTest_CloudGemFramework_ServiceApi(
    service_api_common_support.BaseServiceApiTestCase):

    def __init__(self, *args, **kwargs):
        super(IntegrationTest_CloudGemFramework_ServiceApi, self).__init__(*args, **kwargs)

    def setUp(self):
        self.set_deployment_name(lmbr_aws_test_support.unique_name())
        self.set_resource_group_name(lmbr_aws_test_support.unique_name('sapi'))
        self.prepare_test_environment("cloud_gem_framework_service_api_test")
        self.register_for_shared_resources()

        # Ignore warnings based on https://github.com/boto/boto3/issues/454 for now
        # Needs to be set per tests as its reset
        warnings.filterwarnings(action="ignore", message="unclosed", category=ResourceWarning)

    def test_end_to_end(self):
        self.run_all_tests()

    def __000_create_stacks(self):
        self.base_create_project_stack()
        self.setup_deployment_stacks()

    def __010_add_service_api_resources(self):
       self.add_service_api_resources()

    def __020_create_resources(self):
        self.create_resources()

    def __030_verify_service_api_resources(self):
        self.verify_service_api_resources()

    def __040_call_simple_api(self):
        self.call_simple_api()

    def __050_call_simple_api_with_no_credentials(self):
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

    def __800_run_cpp_tests(self):
        self.run_cpp_tests()

    def __900_remove_service_api_resources(self):
        self.remove_service_api_resources()

    def __910_delete_resources(self):
        self.delete_resources()

    def __999_cleanup(self):
        self.cleanup()
