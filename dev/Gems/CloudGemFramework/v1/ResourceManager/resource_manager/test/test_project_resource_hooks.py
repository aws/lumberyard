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
# $Revision: #1 $

import os

import resource_manager.util

import lmbr_aws_test_support
import mock_specification
from resource_manager.test import base_stack_test
import resource_manager_common.constant as  c
import test_constant

class IntegrationTest_CloudGemFramework_ResourceManager_ProjectResourceHooks(base_stack_test.BaseStackTestCase):
    
    TEST_GEM_PROJECT_RESOURCE_NAME = 'TestGemProjectResource'
    TEST_GEM_PROJECT_RESOURCE_TYPE = 'AWS::S3::Bucket'
    TEST_PROJECT_NAME = lmbr_aws_test_support.unique_name()

    def setUp(self):        
        self.set_resource_group_name(lmbr_aws_test_support.unique_name('prh'))
        self.prepare_test_environment(temp_file_suffix = type(self).__name__)
        self.register_for_shared_resources()

    def test_project_resource_hooks_end_to_end(self):
        self.run_all_tests()

    def __000_create_test_gem(self):
        self.lmbr_aws(
            'cloud-gem', 'create',
            '--gem', self.TEST_RESOURCE_GROUP_NAME,
            '--initial-content', 'resource-manager-plugin',
            '--enable','--no-sln-change', ignore_failure=True)
        self.enable_shared_gem(self.TEST_RESOURCE_GROUP_NAME, 'v1', path=os.path.join(self.context[test_constant.ATTR_ROOT_DIR], os.path.join(test_constant.DIR_GEMS, self.TEST_RESOURCE_GROUP_NAME)))

        gem_project_template_file_path = self.get_gem_aws_path(self.TEST_RESOURCE_GROUP_NAME, c.PROJECT_TEMPLATE_FILENAME)

        resource_manager.util.save_json(gem_project_template_file_path, 
            {
                "Resources": {
                    self.TEST_GEM_PROJECT_RESOURCE_NAME: {
                        "Type": self.TEST_GEM_PROJECT_RESOURCE_TYPE,
                        "Properties": {}
                    }      
                }
            }
        )

    def __005_create_project_stack(self):
        self.base_create_project_stack()

    def __010_verify_initial_stack(self):
        spec = mock_specification.ok_project_stack()
        spec['StackResources'][self.TEST_GEM_PROJECT_RESOURCE_NAME] = {
            'ResourceType': self.TEST_GEM_PROJECT_RESOURCE_TYPE
        }
        self.verify_stack("project stack",  self.get_project_stack_arn(), spec, exact=False)

    def __020_remove_gem(self):
        self.lmbr_aws(
            'cloud-gem', 'enable',
            '--gem', self.TEST_RESOURCE_GROUP_NAME, ignore_failure=True)
        self.lmbr_aws(
            'cloud-gem', 'disable',
            '--gem', self.TEST_RESOURCE_GROUP_NAME, ignore_failure=True)

    def __030_update_project_stack(self):        
        self.base_update_project_stack()
        
    def __040_verify_updated_stack(self):
        spec = mock_specification.ok_project_stack()
        self.verify_stack("project stack",  self.get_project_stack_arn(), spec, exact=False)

    def __050_delete_project_stack(self):
        self.unregister_for_shared_resources()
        self.teardown_base_stack()