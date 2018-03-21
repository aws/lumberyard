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

import unittest
import os
import uuid

import resource_manager.uploader

import lmbr_aws_test_support

uploader_call_file_paths = []
update_call_file_paths = []

class IntegrationTest_CloudGemFramework_ResourceManager_UploaderHooks(lmbr_aws_test_support.lmbr_aws_TestCase):

    PROJECT_HOOK_NAME = 'Project'

    TEST_RESOURCE_GROUP_NAME_1 = 'TestResourceGroup1'
    TEST_RESOURCE_GROUP_NAME_2 = 'TestResourceGroup2'

    def __init__(self, *args, **kwargs):
        super(IntegrationTest_CloudGemFramework_ResourceManager_UploaderHooks, self).__init__(*args, **kwargs)

    def setUp(self):
        self.prepare_test_envionment("uploader_hooks_test")

    # Uploader hooks were deprecated in 1.9. TODO: Remove tests when support is removed.

    # Begin deprecated support

    UPLOADER_CODE = '''

import os

def upload_resource_group_content_pre(hook_module, resource_group_uploader):
    file_path = os.path.join(os.path.dirname(__file__), 'upload_resource_group_content_pre_calls.txt')
    print '>>>>>>>>>>> upload_resource_group_pre_content', file_path
    with open(file_path, 'a') as f:
        f.write(resource_group_uploader.key + ',' + resource_group_uploader.resource_group_name + ',' + resource_group_uploader.deployment_uploader.deployment_name + '\\n')

def upload_resource_group_content_post(hook_module, resource_group_uploader):
    file_path = os.path.join(os.path.dirname(__file__), 'upload_resource_group_content_post_calls.txt')
    print '>>>>>>>>>>> upload_resource_group_post_content', file_path
    with open(file_path, 'a') as f:
        f.write(resource_group_uploader.key + ',' + resource_group_uploader.resource_group_name + ',' + resource_group_uploader.deployment_uploader.deployment_name + '\\n')
    
def upload_deployment_content_pre(hook_module, deployment_uploader):
    file_path = os.path.join(os.path.dirname(__file__), 'upload_deployment_content_pre_calls.txt')
    print '>>>>>>>>>>> upload_deployment_pre_content', file_path
    with open(file_path, 'a') as f:
        f.write(deployment_uploader.key + ',' + deployment_uploader.deployment_name + '\\n')

def upload_deployment_content_post(hook_module, deployment_uploader):
    file_path = os.path.join(os.path.dirname(__file__), 'upload_deployment_content_post_calls.txt')
    print '>>>>>>>>>>> upload_deployment_post_content', file_path
    with open(file_path, 'a') as f:
        f.write(deployment_uploader.key + ',' + deployment_uploader.deployment_name + '\\n')

def upload_project_content_pre(hook_module, project_uploader):
    file_path = os.path.join(os.path.dirname(__file__), 'upload_project_content_pre_calls.txt')
    print '>>>>>>>>>>> upload_project_pre_content', file_path
    with open(file_path, 'a') as f:
        f.write(project_uploader.key + ',' + '\\n')
    
def upload_project_content_post(hook_module, project_uploader):
    file_path = os.path.join(os.path.dirname(__file__), 'upload_project_content_post_calls.txt')
    print '>>>>>>>>>>> upload_project_post_content', file_path
    with open(file_path, 'a') as f:
        f.write(project_uploader.key + ',' + '\\n')

'''

    def __create_uploader_hook(self, directory_path):

        global uploader_call_file_paths

        plugin_path = os.path.join(directory_path, 'cli-plugin-code')
        if not os.path.exists(plugin_path):
            os.makedirs(plugin_path)

        file_path = os.path.join(plugin_path, 'upload.py')
        with open(file_path, 'w') as f:
            f.write(self.UPLOADER_CODE)

        print '>>>>>>>>>>> created uploader', file_path

        uploader_call_file_paths.append(os.path.join(plugin_path, 'upload_resource_group_content_pre_calls.txt'))
        uploader_call_file_paths.append(os.path.join(plugin_path, 'upload_resource_group_content_post_calls.txt'))
        uploader_call_file_paths.append(os.path.join(plugin_path, 'upload_deployment_content_post_calls.txt'))
        uploader_call_file_paths.append(os.path.join(plugin_path, 'upload_deployment_content_pre_calls.txt'))
        uploader_call_file_paths.append(os.path.join(plugin_path, 'upload_project_content_pre_calls.txt'))
        uploader_call_file_paths.append(os.path.join(plugin_path, 'upload_project_content_post_calls.txt'))

    def __delete_uploader_call_files(self):
        global uploader_call_file_paths
        for file_path in uploader_call_file_paths:
            if os.path.isfile(file_path):
                os.remove(file_path)

    def __assert_uploader_call_files(self, expected_list):
        global uploader_call_file_paths
        for file_path in uploader_call_file_paths:
            was_expected = False
            for expected in expected_list:
                if expected in file_path:
                    was_expected = True
            if was_expected:
                self.assertTrue(os.path.isfile(file_path), msg='Expected ' + file_path)
            else:
                self.assertFalse(os.path.isfile(file_path), msg='Did not expect ' + file_path)

    # End deprecated support

    # The hooks here do not have an **kwargs parameter so that we can test that all the expected
    # args, and only the expected args, are provided.

    UPDATE_CODE = '''

import os

def log_resource_group_hook_call(base_name, hook, deployment_name, resource_group_name, resource_group_uploader, **kwargs):
    file_path = os.path.join(os.path.dirname(__file__), base_name + '_{HOOK_NAME}_' + resource_group_name + '.txt')
    print '>>>>>>>>>>> ' + base_name + '_{HOOK_NAME}_' + resource_group_name, file_path
    with open(file_path, 'a') as f:
        f.write(hook.context.config.root_directory_path + ',' + resource_group_uploader.key + ',' + resource_group_name + ',' + deployment_name + '\\n')

def before_this_resource_group_updated(hook, deployment_name, resource_group_name, resource_group_uploader, **kwargs):
    log_resource_group_hook_call('before_this_resource_group_updated', hook, deployment_name, resource_group_name, resource_group_uploader)

def after_this_resource_group_updated(hook,  deployment_name, resource_group_name, resource_group_uploader, **kwargs):
    log_resource_group_hook_call('after_this_resource_group_updated', hook, deployment_name, resource_group_name, resource_group_uploader)
    
def before_resource_group_updated(hook, deployment_name, resource_group_name, resource_group_uploader, **kwargs):
    log_resource_group_hook_call('before_resource_group_updated', hook, deployment_name, resource_group_name, resource_group_uploader)

def after_resource_group_updated(hook,  deployment_name, resource_group_name, resource_group_uploader, **kwargs):
    log_resource_group_hook_call('after_resource_group_updated', hook, deployment_name, resource_group_name, resource_group_uploader)

def log_project_hook_call(base_name, hook, project_uploader, **kwargs):
    file_path = os.path.join(os.path.dirname(__file__), base_name + '_{HOOK_NAME}.txt')
    print '>>>>>>>>>>> ' + base_name + '_{HOOK_NAME}', file_path
    with open(file_path, 'a') as f:
        f.write(hook.context.config.root_directory_path + ',' + project_uploader.key + '\\n')
    
def before_project_updated(hook, project_uploader, **kwargs):
    log_project_hook_call('before_project_updated', hook, project_uploader)
    
def after_project_updated(hook, project_uploader, **kwargs):
    log_project_hook_call('after_project_updated', hook, project_uploader)

'''
    def __create_update_hooks(self, directory_path, hook_name):

        global update_call_file_paths

        plugin_path = os.path.join(directory_path, 'resource-manager-code')
        if not os.path.exists(plugin_path):
            os.makedirs(plugin_path)

        file_path = os.path.join(plugin_path, 'update.py')
        with open(file_path, 'w') as f:
            f.write(self.UPDATE_CODE.format(HOOK_NAME=hook_name))

        print '>>>>>>>>>>> created update hook', hook_name, file_path

        update_call_file_paths.append(os.path.join(plugin_path, 'before_this_resource_group_updated_{HOOK_NAME}_{RESOURCE_GROUP_NAME}.txt'.format(HOOK_NAME=hook_name, RESOURCE_GROUP_NAME=self.TEST_RESOURCE_GROUP_NAME_1)))
        update_call_file_paths.append(os.path.join(plugin_path, 'after_this_resource_group_updated_{HOOK_NAME}_{RESOURCE_GROUP_NAME}.txt'.format(HOOK_NAME=hook_name, RESOURCE_GROUP_NAME=self.TEST_RESOURCE_GROUP_NAME_1)))
        update_call_file_paths.append(os.path.join(plugin_path, 'before_resource_group_updated_{HOOK_NAME}_{RESOURCE_GROUP_NAME}.txt'.format(HOOK_NAME=hook_name, RESOURCE_GROUP_NAME=self.TEST_RESOURCE_GROUP_NAME_1)))
        update_call_file_paths.append(os.path.join(plugin_path, 'after_resource_group_updated_{HOOK_NAME}_{RESOURCE_GROUP_NAME}.txt'.format(HOOK_NAME=hook_name, RESOURCE_GROUP_NAME=self.TEST_RESOURCE_GROUP_NAME_1)))

        update_call_file_paths.append(os.path.join(plugin_path, 'before_project_updated_{HOOK_NAME}.txt'.format(HOOK_NAME=hook_name)))
        update_call_file_paths.append(os.path.join(plugin_path, 'after_project_updated_{HOOK_NAME}.txt'.format(HOOK_NAME=hook_name)))

    def __delete_update_call_files(self):
        global update_call_file_paths
        for file_path in update_call_file_paths:
            if os.path.isfile(file_path):
                os.remove(file_path)

    def __assert_update_call_files(self, *expected_lists):

        expected_list = []
        for list in expected_lists:
            expected_list.extend(list)

        print '>>> checking for', expected_list

        global update_call_file_paths
        for file_path in update_call_file_paths:
            was_expected = False
            for expected in expected_list:
                if expected in file_path:
                    was_expected = True
            if was_expected:
                self.assertTrue(os.path.isfile(file_path), msg='Expected ' + file_path)
            else:
                self.assertFalse(os.path.isfile(file_path), msg='Did not expect ' + file_path)

    def __get_update_call_file_list(self, base_name, hook_names, resource_group_name = None):
        if resource_group_name is None:
            return [ base_name + '_' + hook_name for hook_name in hook_names ]
        else:
            return [ base_name + '_' + hook_name + '_' + resource_group_name for hook_name in hook_names ]

    def test_update_hooks_end_to_end(self):  
        self.run_all_tests()      

    def __010_initialize_project_files(self):
        self.lmbr_aws('project', 'create', '--files-only', '--region', lmbr_aws_test_support.REGION)

    def __020_create_resource_groups(self):

        self.lmbr_aws(
            'cloud-gem', 'create',
            '--gem', self.TEST_RESOURCE_GROUP_NAME_1,
            '--initial-content', 'no-resources',
            '--enable')

        self.lmbr_aws(
            'cloud-gem', 'create',
            '--gem', self.TEST_RESOURCE_GROUP_NAME_2,
            '--initial-content', 'no-resources',
            '--enable')

    def __030_create_update_hooks(self):

        # Uploader hooks were deprecated in 1.9. TODO: Remove tests when support is removed.
        self.__create_uploader_hook(self.AWS_DIR)
        self.__create_uploader_hook(self.get_gem_aws_path(self.TEST_RESOURCE_GROUP_NAME_1))
        resource_manager.uploader._uploader_hook_modules = None

        self.__create_update_hooks(self.AWS_DIR, self.PROJECT_HOOK_NAME)
        self.__create_update_hooks(self.get_gem_aws_path(self.TEST_RESOURCE_GROUP_NAME_1), self.TEST_RESOURCE_GROUP_NAME_1)
        self.__create_update_hooks(self.get_gem_aws_path(self.TEST_RESOURCE_GROUP_NAME_2), self.TEST_RESOURCE_GROUP_NAME_2)

    def __040_create_project_stack(self):
        self.__delete_uploader_call_files() # deprecated
        self.__delete_update_call_files()
        self.lmbr_aws('project', 'create', '--stack-name', self.TEST_PROJECT_STACK_NAME, '--confirm-aws-usage', '--confirm-security-change', '--region', lmbr_aws_test_support.REGION)
        self.__assert_uploader_call_files(['project_content_pre','project_content_post'])        
        self.__assert_update_call_files(
            self.__get_update_call_file_list('before_project_updated', [ self.PROJECT_HOOK_NAME, self.TEST_RESOURCE_GROUP_NAME_1, self.TEST_RESOURCE_GROUP_NAME_2 ]),
            self.__get_update_call_file_list('after_project_updated', [ self.PROJECT_HOOK_NAME, self.TEST_RESOURCE_GROUP_NAME_1, self.TEST_RESOURCE_GROUP_NAME_2 ])
        )

    def __050_create_deployment_stack(self):
        self.__delete_uploader_call_files()
        self.__delete_update_call_files()
        self.lmbr_aws('deployment', 'create', '--deployment', self.TEST_DEPLOYMENT_NAME, '--confirm-aws-usage', '--confirm-security-change')
        self.__assert_uploader_call_files(['deployment_content_pre', 'deployment_content_post', 'resource_group_content_pre','resource_group_content_post'])        
        self.__assert_update_call_files(

            self.__get_update_call_file_list('before_this_resource_group_updated', hook_names = [ self.TEST_RESOURCE_GROUP_NAME_1 ], resource_group_name = self.TEST_RESOURCE_GROUP_NAME_1),
            self.__get_update_call_file_list('after_this_resource_group_updated', hook_names = [ self.TEST_RESOURCE_GROUP_NAME_1 ], resource_group_name = self.TEST_RESOURCE_GROUP_NAME_1),

            self.__get_update_call_file_list('before_this_resource_group_updated', hook_names = [ self.TEST_RESOURCE_GROUP_NAME_2 ], resource_group_name = self.TEST_RESOURCE_GROUP_NAME_2),
            self.__get_update_call_file_list('after_this_resource_group_updated', hook_names = [ self.TEST_RESOURCE_GROUP_NAME_2 ], resource_group_name = self.TEST_RESOURCE_GROUP_NAME_2),

            self.__get_update_call_file_list('before_resource_group_updated', hook_names = [ self.PROJECT_HOOK_NAME, self.TEST_RESOURCE_GROUP_NAME_1, self.TEST_RESOURCE_GROUP_NAME_2 ], resource_group_name = self.TEST_RESOURCE_GROUP_NAME_1),
            self.__get_update_call_file_list('after_resource_group_updated', hook_names = [ self.PROJECT_HOOK_NAME, self.TEST_RESOURCE_GROUP_NAME_1, self.TEST_RESOURCE_GROUP_NAME_2 ], resource_group_name = self.TEST_RESOURCE_GROUP_NAME_1),

            self.__get_update_call_file_list('before_resource_group_updated', hook_names = [ self.PROJECT_HOOK_NAME, self.TEST_RESOURCE_GROUP_NAME_1, self.TEST_RESOURCE_GROUP_NAME_2 ], resource_group_name = self.TEST_RESOURCE_GROUP_NAME_2),
            self.__get_update_call_file_list('after_resource_group_updated', hook_names = [ self.PROJECT_HOOK_NAME, self.TEST_RESOURCE_GROUP_NAME_1, self.TEST_RESOURCE_GROUP_NAME_2 ], resource_group_name = self.TEST_RESOURCE_GROUP_NAME_2)

        )        

    def __060_disable_resource_group(self):
        self.lmbr_aws('resource-group', 'disable', '--resource-group', self.TEST_RESOURCE_GROUP_NAME_1)

    def __070_delete_resource_group_stack(self):
        self.__delete_update_call_files()
        self.lmbr_aws('resource-group', 'update', '--deployment', self.TEST_DEPLOYMENT_NAME, '--resource-group', self.TEST_RESOURCE_GROUP_NAME_1, '--confirm-resource-deletion')
        self.__assert_update_call_files(
            # no hooks should have been called when deleting
        ) 

    def __080_enable_resource_group(self):
        self.lmbr_aws('resource-group', 'enable', '--resource-group', self.TEST_RESOURCE_GROUP_NAME_1)

    def __090_recreate_resource_group_stack(self):
        self.__delete_uploader_call_files()
        self.__delete_update_call_files()
        self.lmbr_aws('resource-group', 'update', '--deployment', self.TEST_DEPLOYMENT_NAME, '--resource-group', self.TEST_RESOURCE_GROUP_NAME_1, '--confirm-aws-usage', '--confirm-security-change', '--verbose')
        self.__assert_uploader_call_files(['resource_group_content_pre','resource_group_content_post'])        
        self.__assert_update_call_files(

            # only hooks for the resource group should have been called

            self.__get_update_call_file_list('before_this_resource_group_updated', hook_names = [ self.TEST_RESOURCE_GROUP_NAME_1 ], resource_group_name = self.TEST_RESOURCE_GROUP_NAME_1),
            self.__get_update_call_file_list('after_this_resource_group_updated', hook_names = [ self.TEST_RESOURCE_GROUP_NAME_1 ], resource_group_name = self.TEST_RESOURCE_GROUP_NAME_1),

            self.__get_update_call_file_list('before_resource_group_updated', hook_names = [ self.PROJECT_HOOK_NAME, self.TEST_RESOURCE_GROUP_NAME_1, self.TEST_RESOURCE_GROUP_NAME_2 ], resource_group_name = self.TEST_RESOURCE_GROUP_NAME_1),
            self.__get_update_call_file_list('after_resource_group_updated', hook_names = [ self.PROJECT_HOOK_NAME, self.TEST_RESOURCE_GROUP_NAME_1, self.TEST_RESOURCE_GROUP_NAME_2 ], resource_group_name = self.TEST_RESOURCE_GROUP_NAME_1),

        )

    def __100_project_updates(self):
        self.__delete_uploader_call_files()
        self.__delete_update_call_files()
        self.lmbr_aws('project', 'update', '--confirm-aws-usage', '--confirm-security-change')
        self.__assert_uploader_call_files(['project_content_pre','project_content_post'])        
        self.__assert_update_call_files(
            self.__get_update_call_file_list('before_project_updated', [ self.PROJECT_HOOK_NAME, self.TEST_RESOURCE_GROUP_NAME_1, self.TEST_RESOURCE_GROUP_NAME_2 ]),
            self.__get_update_call_file_list('after_project_updated', [ self.PROJECT_HOOK_NAME, self.TEST_RESOURCE_GROUP_NAME_1, self.TEST_RESOURCE_GROUP_NAME_2 ])
        )

    def __110_deployment_updates(self):
        self.__delete_uploader_call_files()
        self.__delete_update_call_files()
        self.lmbr_aws('deployment', 'update', '--deployment', self.TEST_DEPLOYMENT_NAME, '--confirm-aws-usage')
        self.__assert_uploader_call_files(['deployment_content_pre', 'deployment_content_post', 'resource_group_content_pre','resource_group_content_post'])
        self.__assert_update_call_files(

            self.__get_update_call_file_list('before_this_resource_group_updated', hook_names = [ self.TEST_RESOURCE_GROUP_NAME_1 ], resource_group_name = self.TEST_RESOURCE_GROUP_NAME_1),
            self.__get_update_call_file_list('after_this_resource_group_updated', hook_names = [ self.TEST_RESOURCE_GROUP_NAME_1 ], resource_group_name = self.TEST_RESOURCE_GROUP_NAME_1),

            self.__get_update_call_file_list('before_this_resource_group_updated', hook_names = [ self.TEST_RESOURCE_GROUP_NAME_2 ], resource_group_name = self.TEST_RESOURCE_GROUP_NAME_2),
            self.__get_update_call_file_list('after_this_resource_group_updated', hook_names = [ self.TEST_RESOURCE_GROUP_NAME_2 ], resource_group_name = self.TEST_RESOURCE_GROUP_NAME_2),
            
            self.__get_update_call_file_list('before_resource_group_updated', hook_names = [ self.PROJECT_HOOK_NAME, self.TEST_RESOURCE_GROUP_NAME_1, self.TEST_RESOURCE_GROUP_NAME_2 ], resource_group_name = self.TEST_RESOURCE_GROUP_NAME_1),
            self.__get_update_call_file_list('after_resource_group_updated', hook_names = [ self.PROJECT_HOOK_NAME, self.TEST_RESOURCE_GROUP_NAME_1, self.TEST_RESOURCE_GROUP_NAME_2 ], resource_group_name = self.TEST_RESOURCE_GROUP_NAME_1),

            self.__get_update_call_file_list('before_resource_group_updated', hook_names = [ self.PROJECT_HOOK_NAME, self.TEST_RESOURCE_GROUP_NAME_1, self.TEST_RESOURCE_GROUP_NAME_2 ], resource_group_name = self.TEST_RESOURCE_GROUP_NAME_2),
            self.__get_update_call_file_list('after_resource_group_updated', hook_names = [ self.PROJECT_HOOK_NAME, self.TEST_RESOURCE_GROUP_NAME_1, self.TEST_RESOURCE_GROUP_NAME_2 ], resource_group_name = self.TEST_RESOURCE_GROUP_NAME_2)

        )        

    def __120_resource_group_updates(self):
        self.__delete_uploader_call_files()
        self.__delete_update_call_files()
        self.lmbr_aws('resource-group', 'update', '--deployment', self.TEST_DEPLOYMENT_NAME, '--resource-group', self.TEST_RESOURCE_GROUP_NAME_1, '--confirm-aws-usage')
        self.__assert_uploader_call_files(['resource_group_content_pre','resource_group_content_post'])
        self.__assert_update_call_files(

            # only hooks for the resource group should have been called

            self.__get_update_call_file_list('before_this_resource_group_updated', hook_names = [ self.TEST_RESOURCE_GROUP_NAME_1 ], resource_group_name = self.TEST_RESOURCE_GROUP_NAME_1),
            self.__get_update_call_file_list('after_this_resource_group_updated', hook_names = [ self.TEST_RESOURCE_GROUP_NAME_1 ], resource_group_name = self.TEST_RESOURCE_GROUP_NAME_1),

            self.__get_update_call_file_list('before_resource_group_updated', hook_names = [ self.PROJECT_HOOK_NAME, self.TEST_RESOURCE_GROUP_NAME_1, self.TEST_RESOURCE_GROUP_NAME_2 ], resource_group_name = self.TEST_RESOURCE_GROUP_NAME_1),
            self.__get_update_call_file_list('after_resource_group_updated', hook_names = [ self.PROJECT_HOOK_NAME, self.TEST_RESOURCE_GROUP_NAME_1, self.TEST_RESOURCE_GROUP_NAME_2 ], resource_group_name = self.TEST_RESOURCE_GROUP_NAME_1),

        )

    def __900_delete_deployment(self):
        self.__delete_update_call_files()
        self.lmbr_aws('deployment', 'delete', '--deployment', self.TEST_DEPLOYMENT_NAME, '--confirm-resource-deletion')
        self.__assert_update_call_files(
            # no hooks should have been called when deleting
        ) 

    def __999_delete_project(self):
        self.__delete_update_call_files()
        self.lmbr_aws('delete-project-stack', '--confirm-resource-deletion')
        self.__assert_update_call_files(
            # no hooks should have been called when deleting
        ) 



