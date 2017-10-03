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

class IntegrationTest_CloudGemFramework_ResourceManager_ProjectResourceHooks(lmbr_aws_test_support.lmbr_aws_TestCase):

    TEST_GEM_NAME = 'TestGem'
    TEST_GEM_PROJECT_RESOURCE_NAME = 'TestGemProjectResource'
    TEST_GEM_PROJECT_RESOURCE_TYPE = 'AWS::S3::Bucket'

    def setUp(self):        
        self.prepare_test_envionment(temp_file_suffix = type(self).__name__)   
        
    def test_project_resource_hooks_end_to_end(self):
        self.run_all_tests()

    def __000_create_test_gem(self):

        self.lmbr_aws(
            'cloud-gem', 'create',
            '--gem', self.TEST_GEM_NAME,
            '--initial-content', 'resource-manager-plugin',
            '--enable')

        gem_project_template_file_path = self.get_gem_aws_path(self.TEST_GEM_NAME, 'project-template.json')

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
        self.lmbr_aws('create-project-stack', '--stack-name', self.TEST_PROJECT_STACK_NAME, '--confirm-aws-usage', '--confirm-security-change', '--region', lmbr_aws_test_support.REGION)

    def __010_verify_initial_stack(self):
        spec = mock_specification.ok_project_stack()
        spec['StackResources'][self.TEST_GEM_PROJECT_RESOURCE_NAME] = {
            'ResourceType': self.TEST_GEM_PROJECT_RESOURCE_TYPE
        }
        self.verify_stack("project stack",  self.get_project_stack_arn(), spec)

    def __020_remove_gem(self):
        self.lmbr_aws(
            'cloud-gem', 'disable',
            '--gem', self.TEST_GEM_NAME)

    def __030_update_project_stack(self):        
        self.lmbr_aws('project', 'update', '--confirm-resource-deletion', '--confirm-aws-usage')
        
    def __040_verify_updated_stack(self):
        spec = mock_specification.ok_project_stack()
        self.verify_stack("project stack",  self.get_project_stack_arn(), spec)

    def __050_delete_project_stack(self):
        self.lmbr_aws('project', 'delete', '--confirm-resource-deletion')