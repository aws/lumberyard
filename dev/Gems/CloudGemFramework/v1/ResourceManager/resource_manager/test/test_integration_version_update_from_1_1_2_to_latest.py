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
# $Revision$

import contextlib
import mock
import os

from cgf_utils.version_utils import Version
import resource_manager.hook

import lmbr_aws_test_support
import project_snapshot
import test_constant

class Foo(object):

    def bar(self, x):
        print 'callled bar with', x



class IntegrationTest_CloudGemFramework_ResourceManager_version_update1_1_2(lmbr_aws_test_support.lmbr_aws_TestCase):

    def __init__(self, *args, **kwargs):
        super(IntegrationTest_CloudGemFramework_ResourceManager_version_update1_1_2, self).__init__(*args, **kwargs)

    def setUp(self):
        self.prepare_test_envionment("project_update_1_1_2")

    def test_framework_version_update_end_to_end(self):  
        self.run_all_tests()    

    def snapshot_path(self, snapshot_name):
        return os.path.abspath(os.path.join(__file__, '..', 'snapshots', snapshot_name))

    ############################################
    ## Unitialized Project Tests
    ##

    def __010_make_unitialized_framework_version_1_1_2_project(self):
        project_snapshot.restore_snapshot(
            region = self.TEST_REGION, 
            profile = self.TEST_PROFILE, 
            stack_name = self.TEST_PROJECT_STACK_NAME, 
            project_directory_path = self.GAME_DIR, 
            snapshot_file_path = self.snapshot_path('CGF_1_1_2_Minimal_Uninitialized'),
            root_directory_path = self.REAL_ROOT_DIR)

    def __020_commands_fail_before_updating_unitialized_project(self):
        self.lmbr_aws('resource-group', 'list', expect_failure = True)
        self.lmbr_aws('project', 'create', '--stack-name', self.TEST_PROJECT_STACK_NAME, '--region', self.TEST_REGION, '--confirm-aws-usage', '--confirm-security-change', expect_failure = True)
        self.lmbr_aws('deployment', 'list', expect_failure = True)
        self.lmbr_aws('deployment', 'create', '--deployment', 'TestDeployment1', expect_failure = True)

    def __030_updating_unitialized_project_is_successful(self):

        with self.spy_decorator(resource_manager.hook.HookContext.call_module_handlers) as mock_call_module_handlers:
        
            self.lmbr_aws('project', 'update-framework-version')

            mock_call_module_handlers.assert_any_call(
                'resource-manager-code/update.py', 
                'before_framework_version_updated', 
                kwargs={
                    'from_version': Version('1.1.2'),
                    'to_version': self.CURRENT_FRAMEWORK_VERSION
                })
        
            mock_call_module_handlers.assert_any_call(
                'resource-manager-code/update.py', 
                'after_framework_version_updated', 
                kwargs={
                    'from_version': Version('1.1.2'),
                    'to_version': self.CURRENT_FRAMEWORK_VERSION
                })

    def __040_commands_succeed_after_updating_unitialized_project(self):
        self.lmbr_aws('resource-group', 'list')

    def __041_commands_succeed_after_updating_unitialized_project(self):
        self.lmbr_aws('project', 'create', '--stack-name', self.TEST_PROJECT_STACK_NAME, '--region', self.TEST_REGION, '--confirm-aws-usage', '--confirm-security-change')
    
    def __042_commands_succeed_after_updating_unitialized_project(self):
        self.lmbr_aws('deployment', 'list')
    
    def __043_commands_succeed_after_updating_unitialized_project(self):
        self.lmbr_aws('deployment', 'create', '--deployment', 'TestDeployment1', '--confirm-aws-usage', '--confirm-security-change')

    def __099_cleanup_uninitialized_project(self):
        self.lmbr_aws('deployment', 'delete', '-d', 'TestDeployment1', '--confirm-resource-deletion')
        self.lmbr_aws('project', 'delete', '--confirm-resource-deletion')

    ##
    ## Unitialized Project Tests
    ############################################
    ## Initialized Project Tests
    ##

    def __110_make_initialized_framework_version_1_1_2_project(self):
        project_snapshot.restore_snapshot(
            region = self.TEST_REGION, 
            profile = self.TEST_PROFILE, 
            stack_name = self.TEST_PROJECT_STACK_NAME, 
            project_directory_path = self.GAME_DIR, 
            snapshot_file_path = self.snapshot_path('CGF_1_1_2_Minimal_Initialized'),
            root_directory_path = self.REAL_ROOT_DIR)

    def __120_commands_fail_before_updating_initialized_project(self):
        self.lmbr_aws('resource-group', 'list', expect_failure = True)
        self.lmbr_aws('deployment', 'list', expect_failure = True)
        self.lmbr_aws('deployment', 'create', '--deployment', 'TestDeployment2', expect_failure = True)

    def __130_updating_initialized_project_is_successful(self):

        with self.spy_decorator(resource_manager.hook.HookContext.call_module_handlers) as mock_call_module_handlers:

            self.lmbr_aws('project', 'update-framework-version', '--confirm-aws-usage', '--confirm-security-change', '--confirm-resource-deletion')

            mock_call_module_handlers.assert_any_call(
                'resource-manager-code/update.py', 
                'before_framework_version_updated', 
                kwargs={
                    'from_version': Version('1.1.2'),
                    'to_version': self.CURRENT_FRAMEWORK_VERSION
                })
        
            mock_call_module_handlers.assert_any_call(
                'resource-manager-code/update.py', 
                'after_framework_version_updated', 
                kwargs={
                    'from_version': Version('1.1.2'),
                    'to_version': self.CURRENT_FRAMEWORK_VERSION
                })

    def __140_commands_succeed_after_updating_initialized_project(self):
        self.lmbr_aws('resource-group', 'list')
        self.assertIn('CGF100ResourceGroup', self.lmbr_aws_stdout) # verify project local resource group present
        self.assertIn('CGF100GemResourceGroup', self.lmbr_aws_stdout) # verify gem resource group present

    def __141_commands_succeed_after_updating_initialized_project(self):
        self.lmbr_aws('deployment', 'list')

    def __142_commands_succeed_after_updating_initialized_project(self):
        self.lmbr_aws('deployment', 'create', '--deployment', 'TestDeployment2', '--confirm-aws-usage', '--confirm-security-change')

    def __199_cleanup_initialized_project(self):
        self.lmbr_aws('deployment', 'delete', '-d', 'TestDeployment1', '--confirm-resource-deletion')
        self.lmbr_aws('deployment', 'delete', '-d', 'TestDeployment2', '--confirm-resource-deletion')
        self.lmbr_aws('project', 'delete', '--confirm-resource-deletion')


if __name__ == '__main__':
    unittest.main()
