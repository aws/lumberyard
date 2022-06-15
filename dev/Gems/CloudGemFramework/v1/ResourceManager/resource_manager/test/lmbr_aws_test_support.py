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

# python standard lib imports
import contextlib
import datetime
import json
import os
import platform
import random
import re
import shutil
import stat
import string
import sys
import threading
import time
import unittest
import unittest.mock
from contextlib import contextmanager
from tempfile import mkdtemp, gettempdir
from time import sleep
from unittest import mock

# 3rd party library imports
import boto3
import cgf_service_client
from botocore.exceptions import ClientError

# resource manager imports
import resource_manager.cli
import resource_manager.stack
import resource_manager.util
import resource_manager_common.constant
from cgf_utils import aws_utils, custom_resource_utils
# Cloud Gem Framework imports
from cgf_utils.version_utils import Version
from resource_manager.gem import GemContext
# resource manager test imports
from resource_manager.test import test_constant
from resource_manager.test.test_integration_shared_resource import SharedResourceManager
from resource_manager_common import stack_info

REGION = 'us-east-1'

UNIQUE_NAME_CHARACTERS = string.ascii_uppercase + string.digits
UNIQUE_NAME_CHARACTER_COUNT = 7
TEST_PREFIX = test_constant.TEST_NAME_PREFIX


def create(**enums):
    return type('Enum', (), enums)


def unique_name(prefix=TEST_PREFIX, suffix=""):
    return prefix + ''.join(random.SystemRandom().choice(UNIQUE_NAME_CHARACTERS) for _ in range(UNIQUE_NAME_CHARACTER_COUNT)) + suffix


def _del_rw(action, name, exc):
    try:
        os.chmod(name, stat.S_IWRITE)
        os.remove(name)
    except:
        pass


class Credentials(object):

    def __init__(self, access_key, secret_key):
        self.__access_key = access_key
        self.__secret_key = secret_key

    @property
    def access_key(self):
        return self.__access_key

    @property
    def secret_key(self):
        return self.__secret_key


class ResourceTransition:
    def __init__(self, id, transition, bypass=None, condition=None, post_alwaysrun=None):
        self.__id = id
        self.__transition = transition
        self.__bypass = bypass
        self.__condition = condition
        self.__always_run = post_alwaysrun

    @property
    def id(self):
        return self.__id

    def execute(self, context):
        if self.__condition is None:
            self.__transition(context)
        else:
            if self.__condition(context):
                self.__transition(context)
            elif self.__bypass:
                self.__bypass(context)

        if self.__always_run:
            self.__always_run(context)


RESOURCE_TYPE = create(PROJECT_STACK="PROJECT_STK", DEPLOYMENT_STACK="DEPLOYMENT_STK")


class lmbr_aws_TestCase(unittest.TestCase):
    context = {}

    @classmethod
    def setUpClass(self):
        self.verify_handlers = {
            'AWS::CloudFormation::Stack': self.verify_child_stack,
            'AWS::Lambda::Function': self.verify_lambda_function,
            'AWS::IAM::Role': self.verify_iam_role,
            'AWS::IAM::ManagedPolicy': self.verify_iam_managed_policy
        }

        if os.environ.get('NO_TEST_PROFILE', None):
            self.session = boto3.Session(region_name=REGION)
        else:
            self.session = boto3.Session(region_name=REGION, profile_name=test_constant.TEST_PROFILE)
        self.aws_cloudformation = self.session.client('cloudformation')
        self.aws_cognito_identity = self.session.client('cognito-identity')
        self.aws_cognito_idp = self.session.client('cognito-idp')
        self.aws_dynamo = self.session.client('dynamodb')
        self.aws_s3 = self.session.client('s3')
        self.aws_lambda = self.session.client('lambda')
        self.aws_logs = self.session.client('logs')
        self.aws_iam = self.session.client('iam')
        self.stack_info_manager = stack_info.StackInfoManager(self.session)

    def __init__(self, *args, **kwargs):
        super(lmbr_aws_TestCase, self).__init__(*args, **kwargs)
        self.stack_resource_descriptions = {}
        self.stack_descriptions = {}
        self.deployment_name_override = None
        self.resource_group_name_override = None
        self.deployment_tags = None
        resource_manager.stack.STACK_UPDATE_DELAY_TIME = 20

        # assumes this file is ...\dev\Gems\CloudGemFramework\v?\ResourceManager\resource_manager\test\lmbr_aws_test_support.py
        # we want ...\dev
        self.__real_root_dir = os.path.abspath(os.path.join(__file__, '..', '..', '..', '..', '..', '..', '..'))
        self.shared_resource_manager = SharedResourceManager()
        self.__project_resource_transitions = create(
            create=ResourceTransition(1, lambda context: (
                # allow a 20s window for all sub processes to register for shared resources
                time.sleep(20),
                self.shared_resource_manager.sync_registered_gems(self.context["GameDir"], self.enable_real_gem),
                self.lmbr_aws('project', 'create', '--stack-name', unique_name(suffix="-hyptest"),
                              '--confirm-aws-usage',
                              '--confirm-security-change',
                              '--region', self.TEST_REGION),
                self.shared_resource_manager.update_context_attribute(context,
                                                                      test_constant.ATTR_LOCAL_PROJECT_SETTINGS_FILE_PATH,
                                                                      self.context[test_constant.ATTR_LOCAL_PROJECT_SETTINGS_FILE_PATH]),
                self.shared_resource_manager.update_context_attribute(context,
                                                                      SharedResourceManager.PROJECT_CONTEXT_STATE_NAME,
                                                                      1),
                self.shared_resource_manager.update_context_attribute(context,
                                                                      SharedResourceManager.PROJECT_CREATOR,
                                                                      self.context[test_constant.ATTR_TEMP_DIR]
                                                                      ),
                self.shared_resource_manager.update_context_attribute(context,
                                                                      SharedResourceManager.PROJECT_CREATOR_PATH,
                                                                      self.context[test_constant.ATTR_CONTEXT_FILE_PATH]
                                                                      )
            ),
                                      post_alwaysrun=lambda context: (
                                          self.shared_resource_manager.sync_project_settings_file(context,
                                                                                                  context[test_constant.ATTR_LOCAL_PROJECT_SETTINGS_FILE_PATH],
                                                                                                  self.context[
                                                                                                      test_constant.ATTR_LOCAL_PROJECT_SETTINGS_FILE_PATH]),
                                          self.lmbr_aws('mappings', 'update', ignore_failure=True)
                                      ),
                                      condition=lambda context: (
                                          self.is_stack_missing(context, self.has_project_stack)
                                      )
                                      ),
            update=ResourceTransition(2, lambda context: (
                self.shared_resource_manager.sync_registered_gems(self.context["GameDir"], self.enable_real_gem),
                self.lmbr_aws('project', 'update', '--confirm-aws-usage', '--confirm-security-change', '--confirm-resource-deletion'),
                self.shared_resource_manager.update_context_attribute(context,
                                                                      SharedResourceManager.PROJECT_CONTEXT_STATE_NAME,
                                                                      2),
                self.shared_resource_manager.update_context_attribute(context,
                                                                      SharedResourceManager.LAST_MODIFIED_BY,
                                                                      self.context
                                                                      )
            ),
                                      post_alwaysrun=lambda context: (
                                          time.sleep(30),
                                          self.lmbr_aws('mappings', 'update', ignore_failure=True)
                                      ),
                                      ),
            delete=ResourceTransition(3, lambda context: (
                self.delete_all_deployments(),
                self.lmbr_aws('project', 'delete', '--confirm-resource-deletion'),
                self.shared_resource_manager.remove_json_file_attribute_with_lock(
                    SharedResourceManager.FILE_SHARED_RESOURCES_CONTEXT,
                    SharedResourceManager.CONTEXT_REGISTERED_GEM),
                self.shared_resource_manager.remove_json_file_attribute_with_lock(
                    SharedResourceManager.FILE_SHARED_RESOURCES_CONTEXT,
                    SharedResourceManager.CONTEXT_REGISTERED_ATTR),
                self.shared_resource_manager.remove_json_file_attribute(
                    context,
                    SharedResourceManager.PROJECT_LOCK_FILE_NAME,
                    test_constant.ATTR_LOCAL_PROJECT_SETTINGS_FILE_PATH),
                self.shared_resource_manager.remove_json_file_attribute(
                    context,
                    SharedResourceManager.PROJECT_LOCK_FILE_NAME,
                    SharedResourceManager.PROJECT_CONTEXT_STATE_NAME),
                self.shared_resource_manager.update_context_attribute(context,
                                                                      SharedResourceManager.LAST_MODIFIED_BY,
                                                                      self.context
                                                                      )
            ),
                                      post_alwaysrun=lambda context: (
                                          self.shared_resource_manager.remove_project_settings_file(
                                              self.LOCAL_PROJECT_SETTINGS_FILE_PATH
                                          )
                                      ),
                                      condition=lambda context: (
                                          self.run_func_only_if_last_test()
                                      )
                                      )
        )
        self.__deployment_resource_transitions = create(
            create=ResourceTransition(1, lambda context: (
                self.shared_resource_manager.sync_registered_gems(self.context["GameDir"], self.enable_real_gem),
                # creating a new deployment stack requires we lock the project stack since the configuration will be updated  
                self.lmbr_aws('deployment', 'create', '--deployment', self.TEST_DEPLOYMENT_NAME, '--tags', self.TEST_DEPLOYMENT_TAGS,
                              '--confirm-aws-usage',
                              '--confirm-security-change') if self.TEST_DEPLOYMENT_TAGS else
                self.lmbr_aws('deployment', 'create', '--deployment', self.TEST_DEPLOYMENT_NAME,
                              '--confirm-aws-usage',
                              '--confirm-security-change'),
                self.shared_resource_manager.update_context_attribute(context, SharedResourceManager.DEPLOYMENT_CONTEXT_STATE_NAME, 1),
                self.shared_resource_manager.update_context_attribute(context,
                                                                      SharedResourceManager.LAST_MODIFIED_BY,
                                                                      self.context
                                                                      ),
            ),
                                      post_alwaysrun=lambda context: (
                                          time.sleep(30),
                                          self.lmbr_aws('mappings', 'update', '-d', self.TEST_DEPLOYMENT_NAME, ignore_failure=True)
                                      ),
                                      condition=lambda context: (
                                          self.is_stack_missing(context, self.has_deployment_stack)
                                      ),
                                      bypass=lambda context: (
                                          self.lmbr_aws('deployment', 'default', '--set', self.TEST_DEPLOYMENT_NAME, ignore_failure=True)
                                      )
                                      ),
            update=ResourceTransition(2, lambda context: (
                self.shared_resource_manager.sync_registered_gems(self.context["GameDir"], self.enable_real_gem),
                self.lmbr_aws('deployment', 'update', '--confirm-aws-usage', '--confirm-security-change', '--confirm-resource-deletion', '-d',
                              self.TEST_DEPLOYMENT_NAME),
                self.shared_resource_manager.update_context_attribute(context,
                                                                      SharedResourceManager.DEPLOYMENT_CONTEXT_STATE_NAME,
                                                                      2),
                self.shared_resource_manager.update_context_attribute(context,
                                                                      SharedResourceManager.LAST_MODIFIED_BY,
                                                                      self.context)
            ),
                                      post_alwaysrun=lambda context: (
                                          time.sleep(30),
                                          self.lmbr_aws('mappings', 'update', '-d', self.TEST_DEPLOYMENT_NAME, ignore_failure=True)
                                      ),
                                      bypass=lambda context: (
                                          self.lmbr_aws('deployment', 'default', '--set', self.TEST_DEPLOYMENT_NAME, ignore_failure=True)
                                      )
                                      ),
            delete=ResourceTransition(3, lambda context: (
                # deleting the deployment stack requires we lock the project stack since the configuration will be updated
                self.lmbr_aws('deployment', 'delete', '--deployment', self.TEST_DEPLOYMENT_NAME,
                              '--confirm-resource-deletion'),
                self.shared_resource_manager.update_context_attribute(context,
                                                                      SharedResourceManager.LAST_MODIFIED_BY,
                                                                      self.context
                                                                      )),
                                      condition=lambda context: (self.run_func_only_if_last_test() and self.has_deployment_stack()),
                                      post_alwaysrun=lambda context: (
                                          self.shared_resource_manager.update_context_attribute(context,
                                                                                                SharedResourceManager.DEPLOYMENT_CONTEXT_STATE_NAME, 3),
                                          self.shared_resource_manager.remove_json_file_attribute(context,
                                                                                                  SharedResourceManager.DEPLOYMENT_LOCK_FILE_NAME,
                                                                                                  SharedResourceManager.DEPLOYMENT_CONTEXT_STATE_NAME)
                                      )
                                      ))

    def delete_all_deployments(self):
        project_settings = self.load_cloud_project_settings()
        if "deployment" in project_settings:
            threads = []
            deployments = project_settings["deployment"]
            print("Deployments to delete: {}".format(deployments.keys()))
            for deployment_name in deployments.keys():
                if deployment_name != "*":
                    t = threading.Thread(target=self.lmbr_aws, args=('deployment', 'delete', '--deployment', deployment_name,
                                                                     '--confirm-resource-deletion'), kwargs={"ignore_failure": True})
                    t.start()
                    threads.append(t)
            # wait for all of the deployments to delete
            for t in threads:
                t.join(timeout=3600)
                time.sleep(10)
                project_settings = self.load_cloud_project_settings()

            print("Deployments remaining after the deletion: {}".format(deployments.keys()))
            if len(deployments.keys()) > 1:
                print("Deployments remaining: {}".format(deployments.keys()))
                # seems a deployment was added to the project when the other deployments were being deleted.
                self.delete_all_deployments()

    def has_project_stack(self):
        return bool(self.get_project_stack_arn())

    def has_deployment_stack(self):
        return bool(self.get_deployment_stack_arn(self.TEST_DEPLOYMENT_NAME))

    def run_func_only_if_last_test(self):
        registered_gems = self.shared_resource_manager.get_attribute(
            SharedResourceManager.FILE_SHARED_RESOURCES_CONTEXT,
            SharedResourceManager.CONTEXT_REGISTERED_ATTR,
            None)

        return registered_gems is not None and len(registered_gems) < 1

    def is_stack_missing(self, context, func):
        if test_constant.ATTR_LOCAL_PROJECT_SETTINGS_FILE_PATH in context:
            self.shared_resource_manager.sync_project_settings_file(context,
                                                                    context[
                                                                        test_constant.ATTR_LOCAL_PROJECT_SETTINGS_FILE_PATH],
                                                                    self.context[
                                                                        test_constant.ATTR_LOCAL_PROJECT_SETTINGS_FILE_PATH])
        response = not func()
        return response

    @property
    def project_transitions(self):
        return self.__project_resource_transitions

    @property
    def deployment_transitions(self):
        return self.__deployment_resource_transitions

    @staticmethod
    def windows_mklink(source_path, link_path):
        """
        Make a symlink via a junction or copied file on Windows
        :param source_path:  The file path to link from
        :param link_path:    The destination of the link to create
        """
        if os.path.isdir(source_path):
            # There isn't a simple "CreateJunction" Win32 API we can call using ctypes,
            # so instead we execute mklink /J in using the shell.
            import subprocess
            result = subprocess.call(['mklink', '/J', link_path, source_path], shell=True)
            if result != 0:
                raise RuntimeError('[ERROR] Could not create junction point between {} and {}.'.format(source_path, link_path))
        else:
            # Can't use symlinks on windows, so just copy the file instead due to admin privilege issues
            shutil.copy(source_path, link_path)

    @staticmethod
    def mklink(source_path, link_path):
        if not os.path.exists(source_path):
            print("Skipping symlink of {}. File does not exist".format(source_path))
            return

        # remove any pre-existing symlinks
        if os.path.islink(link_path):
            os.remove(link_path)

        # Use junctions instead of symlinks on the Windows platform.
        if sys.platform.startswith('win'):
            lmbr_aws_TestCase.windows_mklink(source_path, link_path)
        else:
            os_symlink = getattr(os, "symlink", None)
            if callable(os_symlink):
                try:
                    os.symlink(source_path, link_path)
                except Exception as e:
                    print("[ERROR] Failed to create symlink between {} and {}".format(source_path, link_path))
                    raise e
            else:
                raise RuntimeError("No os.symlink defined for {}".format(sys.platform))

    def lock_and_run_lmbr_aws_shared_project_command(self, *args, **kwargs):
        self.shared_resource_manager.lock_file_and_execute(SharedResourceManager.PROJECT_LOCK_FILE_NAME, lambda c: self.lmbr_aws(*args, **kwargs))

    def lock_and_run_lmbr_aws_shared_deployment_command(self, *args, **kwargs):
        self.shared_resource_manager.lock_file_and_execute(SharedResourceManager.DEPLOYMENT_LOCK_FILE_NAME, lambda c: self.lmbr_aws(*args, **kwargs))

    def register_for_shared_resources(self):
        self.shared_resource_manager.register_for_shared_resource(self.context[test_constant.ATTR_TEMP_DIR])

    def unregister_for_shared_resources(self):
        self.shared_resource_manager.unregister_for_shared_resource(self.context[test_constant.ATTR_TEMP_DIR])

    def wait_for_project(self, transition):
        self.shared_resource_manager.lock_file_and_execute(SharedResourceManager.PROJECT_LOCK_FILE_NAME, transition.execute)

    def wait_for_deployment(self, transition):
        self.shared_resource_manager.lock_file_and_execute(SharedResourceManager.DEPLOYMENT_LOCK_FILE_NAME, transition.execute)

    def prepare_test_environment(self, temp_file_suffix, game_name=test_constant.GAME_NAME, alternate_context_names=None):
        if alternate_context_names is None:
            alternate_context_names = []

        root_temp_dir = gettempdir()
        last_run_root_dir = self.REAL_ROOT_DIR.replace(':', '_').replace('\\', '_')
        last_run_temp_file_path = os.path.join(root_temp_dir, test_constant.TEST_CONTEXT_FILE.format(temp_file_suffix, last_run_root_dir))
        print('\nAttempting to load the test state from file \'{}\'\n'.format(last_run_temp_file_path))
        context = resource_manager.util.load_json(last_run_temp_file_path, {})

        if test_constant.ATTR_TEMP_DIR not in context:
            context[test_constant.ATTR_CONTEXT_FILE_PATH] = last_run_temp_file_path

            context.update(self.__prepare_test_directory(game_name))

            # Allow more than one project to be created for test suite... this is used by the security tests to 
            # make sure that permissions granted to one project don't bleed over into another project. But it
            # could also be used for other similar situations should they come up. See the alternate_context function
            # below.

            context[test_constant.ATTR_ALTERNATE_CONTEXTS] = {}
            for alternate_context_name in alternate_context_names:
                context[test_constant.ATTR_ALTERNATE_CONTEXTS][alternate_context_name] = self.__prepare_test_directory(alternate_context_name)

            resource_manager.util.save_json(last_run_temp_file_path, context)
            print('\n\nNo test state file found. Created temporary directory: {}\n'.format(context[test_constant.ATTR_TEMP_DIR]))
        else:
            print('\n\nTest state file found. Reused temporary directory: {}\n'.format(context[test_constant.ATTR_TEMP_DIR]))
            if test_constant.ATTR_DEPLOYMENT_NAME in context:
                self.deployment_name_override = context[test_constant.ATTR_DEPLOYMENT_NAME]
            if test_constant.ATTR_RESOURCE_GROUP_NAME in context:
                self.resource_group_name_override = context[test_constant.ATTR_RESOURCE_GROUP_NAME]

        print('Using context\n{}'.format(json.dumps(context, indent=4, sort_keys=True)))
        self.context = context

        self.context_stack = []  # see push_context and pop_context
        self.root_context = self.context  # so the root context can get saved after push_context

    def __prepare_test_directory(self, game_name):
        temp_dir = mkdtemp()
        game_dir = os.path.join(temp_dir, game_name)
        aws_dir = os.path.join(game_dir, resource_manager_common.constant.PROJECT_AWS_DIRECTORY_NAME)
        pc_cache_dir = os.path.join(temp_dir, 'Cache', game_name, 'pc', game_name)  # yes, two game_name directories!?!
        local_project_settings_file_path = os.path.join(aws_dir, resource_manager_common.constant.PROJECT_LOCAL_SETTINGS_FILENAME)

        bootstrap_cfg_path = os.path.join(temp_dir, 'bootstrap.cfg')
        if not os.path.exists(bootstrap_cfg_path):
            with open(bootstrap_cfg_path, 'w') as file:
                file.write('sys_game_folder={}\n'.format(game_name))

        if not os.path.exists(game_dir):
            os.makedirs(game_dir)

        game_config_file_path = os.path.join(game_dir, 'game.cfg')
        if not os.path.exists(game_config_file_path):
            with open(game_config_file_path, 'w') as f:
                f.write('Test Place Holder\n')

        self.__enable_real_gem(temp_dir, game_name, 'CloudCanvasCommon')
        self.__enable_real_gem(temp_dir, game_name, 'CloudGemFramework', 'v1')

        # The following are needed to run lmbr.exe, which is used to create gems with resource groups.
        self.__copy_root_dir_file(temp_dir, 'engine.json')
        self.__copy_root_dir_file(temp_dir, 'engineroot.txt')
        self.__copy_root_dir_file(temp_dir, 'LyzardConfig.xml')
        # Ensure project templates are available for Lyzard execution during gem creation
        self.__make_root_dir_link(temp_dir, 'ProjectTemplates')
        sys_platform = platform.system()
        if sys_platform == 'Windows':
            # Link both VS 2017 and 2019 potential outputs
            self.__make_root_dir_link(temp_dir, 'Bin64vc141.Debug.Test')
            self.__make_root_dir_link(temp_dir, 'Bin64vc141.Debug')
            self.__make_root_dir_link(temp_dir, 'Bin64vc142.Debug.Test')
            self.__make_root_dir_link(temp_dir, 'Bin64vc142.Debug')
        elif sys_platform == 'Darwin':
            self.__make_root_dir_link(temp_dir, 'BinMac64.Debug.Test')
            self.__make_root_dir_link(temp_dir, 'BinMac64.Debug')
        self.__make_root_dir_link(temp_dir, '_WAF_')
        self.__make_root_dir_link(temp_dir, 'Tools')
        self.__make_file(game_dir, 'project.json', PROJECT_JSON_CONTENT)
        self.__make_game_config_file(game_dir, 'Editor.xml', EDITOR_XML_CONTENT)
        self.__make_game_config_file(game_dir, 'Game.xml', GAME_XML_CONTENT)
        lmbr_path = GemContext.get_lmbr_exe_path_from_root(temp_dir)

        if not lmbr_path:
            raise RuntimeError('Failed to find {}'.format(GemContext.lmbr_exe_name()))

        # make it so lmbr_aws can call itself (used to generate client code when creating cloud gems)
        lmbr_aws_cmd = 'lmbr_aws.cmd' if sys_platform == 'Windows' else 'lmbr_aws.sh'
        self.__make_file(temp_dir, lmbr_aws_cmd, '@CALL {} %*'.format(os.path.join(self.REAL_ROOT_DIR, lmbr_aws_cmd)))

        return {
            test_constant.ATTR_GAME_NAME: game_name,
            test_constant.ATTR_ROOT_DIR: temp_dir,
            test_constant.ATTR_TEMP_DIR: temp_dir,
            test_constant.ATTR_PROJECT_STACK_NAME: unique_name(test_constant.TEST_NAME_PREFIX),
            test_constant.ATTR_GAME_DIR: game_dir,
            test_constant.ATTR_AWS_DIR: aws_dir,
            test_constant.ATTR_LOCAL_PROJECT_SETTINGS_FILE_PATH: local_project_settings_file_path,
            test_constant.ATTR_PC_CACHE_DIR: pc_cache_dir
        }

    def __copy_root_dir_file(self, temp_dir, file_name):
        src_path = os.path.join(self.REAL_ROOT_DIR, file_name)
        dst_path = os.path.join(temp_dir, file_name)
        print('Copying {} to {}.'.format(src_path, dst_path))
        shutil.copyfile(src_path, dst_path)

    def __make_root_dir_link(self, temp_dir, target_name):
        target_path = os.path.join(self.REAL_ROOT_DIR, target_name)
        link_path = os.path.join(temp_dir, target_name)
        print('Linking {} to {}.'.format(target_path, link_path))
        self.mklink(target_path, link_path)

    def __make_file(self, root_dir, file_name, file_content):
        path = os.path.join(root_dir, file_name)
        with open(path, 'w') as file:
            file.write(file_content)

    def __make_game_config_file(self, game_dir, file_name, file_content):
        path = os.path.join(game_dir, 'Config')
        if not os.path.exists(path):
            os.makedirs(path)
        path = os.path.join(path, file_name)
        with open(path, 'w') as file:
            file.write(file_content)

    def __make_gem_link(self, temp_dir, gem_name, ignore_existing=False, path=None):
        temp_gems_dir_path = os.path.join(temp_dir, test_constant.DIR_GEMS)
        if not os.path.exists(temp_gems_dir_path):
            os.makedirs(temp_gems_dir_path)
        target_path = path if path else os.path.join(self.REAL_ROOT_DIR, test_constant.DIR_GEMS, gem_name)
        link_path = os.path.join(temp_dir, test_constant.DIR_GEMS, gem_name)
        print('Linking {} to {}.'.format(target_path, link_path))
        try:
            self.mklink(target_path, link_path)
        except RuntimeError as e:
            if not ignore_existing:
                raise e

    @contextlib.contextmanager
    def alternate_context(self, alternate_context_name):
        self.context_stack.append(self.context)
        self.context = self.root_context[test_constant.ATTR_ALTERNATE_CONTEXTS][alternate_context_name]
        yield
        self.context = self.context_stack.pop()

    def tearDown(self):
        ok = self.currentResult.wasSuccessful()
        if ok:

            self.assertEqual(len(self.context_stack), 0)
            self.assertIs(self.context, self.root_context)

            def check_context():
                local_project_settings = self.load_local_project_settings()
                self.assertNotIn('PendingProjectStackId', local_project_settings)
                self.assertNotIn('ProjectStackId', local_project_settings)

            check_context()
            for alternate_context_name in self.root_context.get(test_constant.ATTR_ALTERNATE_CONTEXTS, {}).keys():
                with self.alternate_context(alternate_context_name):
                    check_context()

        self.clean_test_environment(self.currentResult)

    def clean_test_environment(self, results):
        ok = results.wasSuccessful()
        errors = results.errors
        failures = results.failures
        print('All tests passed!' if ok else ' {} errors and {} failures so far'.format(len(errors), len(failures)))

        context_file_path = self.root_context[test_constant.ATTR_CONTEXT_FILE_PATH]

        return
        if ok:
            if os.path.isfile(context_file_path):
                print('Deleting temporary file: {}'.format(context_file_path))
                os.remove(context_file_path)

            all_contexts = [self.root_context]
            all_contexts.extend(self.root_context[test_constant.ATTR_ALTERNATE_CONTEXTS].values())
            for context in all_contexts:
                temp_dir = self.context[test_constant.ATTR_TEMP_DIR]
                if os.path.isdir(temp_dir):
                    print('Deleting temporary directory: {}'.format(temp_dir))
                    junction_safe.rmtree(temp_dir, onerror=_del_rw)
        else:
            resource_manager.util.save_json(context_file_path, self.root_context)
            print('\nTest state saved in {}'.format(context_file_path))

    @property
    def TEST_REGION(self):
        return REGION

    @property
    def ROOT_DIR(self):
        return self.context[test_constant.ATTR_ROOT_DIR]

    @property
    def TEMP_DIR(self):
        return self.context[test_constant.ATTR_TEMP_DIR]

    @property
    def TEST_PROJECT_STACK_NAME(self):
        arn = self.get_project_stack_arn()
        if arn:
            project_id = self.get_stack_name_from_arn(arn)
            return project_id
        return self.context[test_constant.ATTR_PROJECT_STACK_NAME]

    @property
    def TEST_DEPLOYMENT_NAME(self):
        return self.deployment_name_override if self.deployment_name_override else test_constant.DEPLOYMENT_NAME

    @property
    def RELEASE_DEPLOYMENT_NAME(self):
        return test_constant.RELEASE_DEPLOYMENT_NAME

    def set_deployment_name(self, value):
        self.deployment_name_override = value

    @property
    def TEST_DEPLOYMENT_TAGS(self):
        return self.deployment_tags if self.deployment_tags else ''

    def set_deployment_tags(self, deployment_tags):
        self.deployment_tags = deployment_tags

    @property
    def TEST_RESOURCE_GROUP_NAME(self):
        return self.resource_group_name_override if self.resource_group_name_override else test_constant.RESOURCE_GROUP_NAME

    def set_resource_group_name(self, value):
        self.resource_group_name_override = value

    @property
    def GAME_DIR(self):
        return self.context[test_constant.ATTR_GAME_DIR]

    @property
    def GAME_NAME(self):
        return self.context[test_constant.ATTR_GAME_NAME]

    @property
    def AWS_DIR(self):
        return self.context[test_constant.ATTR_AWS_DIR]

    @property
    def LOCAL_PROJECT_SETTINGS_FILE_PATH(self):
        return self.context[test_constant.ATTR_LOCAL_PROJECT_SETTINGS_FILE_PATH]

    @property
    def PC_CACHE_DIR(self):
        return self.context[test_constant.ATTR_PC_CACHE_DIR]

    @property
    def REAL_ROOT_DIR(self):
        return self.__real_root_dir

    @property
    def TEST_PROFILE(self):
        return test_constant.TEST_PROFILE

    @property
    def CURRENT_FRAMEWORK_VERSION(self):
        framework_gem_file_path = os.path.abspath(os.path.join(__file__, '..', '..', '..', '..', 'gem.json'))
        with open(framework_gem_file_path, 'r') as file:
            framework_gem = json.load(file)
        return Version(framework_gem['Version'])

    def save_context_to_disk(self):
        self.context[test_constant.ATTR_DEPLOYMENT_NAME] = self.TEST_DEPLOYMENT_NAME
        self.context[test_constant.ATTR_RESOURCE_GROUP_NAME] = self.TEST_RESOURCE_GROUP_NAME
        resource_manager.util.save_json(self.context[test_constant.ATTR_CONTEXT_FILE_PATH], self.context)

    def run(self, result=None):
        self.currentResult = result  # remember result for use in tearDown
        unittest.TestCase.run(self, result)  # call superclass run method

    def run_all_tests(self):
        aws_access_key = os.environ.get('AWS_ACCESS_KEY_ID')
        aws_secret_key = os.environ.get('AWS_SECRET_ACCESS_KEY')

        self.lmbr_aws_credentials_args = []

        if aws_access_key and aws_secret_key:
            print('Using environment credentials.')
            self.lmbr_aws_credentials_args = [
                '--aws-access-key', aws_access_key,
                '--aws-secret-key', aws_secret_key
            ]
        elif not os.environ.get('NO_TEST_PROFILE', None):
            self.lmbr_aws_credentials_args = [
                '--profile', self.TEST_PROFILE
            ]

        prog = re.compile('_{}__\d*_'.format(type(self).__name__))
        for name in sorted(dir(self)):
            value = getattr(self, name)
            if prog.search(name) and callable(value):
                self.runtest(value, name)

    def runtest(self, func, name):
        if not self.context:
            raise RuntimeError('prepare_test_environment has not been called.')
        if name is None:
            raise RuntimeError('runtest must be supplied with valid name.')

        func_name = name
        if self.is_runnable(func_name, self.context):
            print('\n\n**** Running test \'{}\' ***********************************************************************\n'.format(func_name))
            func()
            print('\n\n**** Finished test \'{}\' ***********************************************************************\n'.format(func_name))
            self.context[func_name] = test_constant.STATE_RAN
            resource_manager.util.save_json(self.context[test_constant.ATTR_CONTEXT_FILE_PATH], self.context)
            print('\nTest state saved in {}'.format(self.context[test_constant.ATTR_CONTEXT_FILE_PATH]))
        else:
            print('\n\n**** Skipping test \'{}\' ***********************************************************************\n\n'.format(func_name))

    def __add_to_gems_file(self, temp_dir, game_name, relative_gem_path):

        gems_json_path = os.path.join(temp_dir, game_name, 'gems.json')

        if os.path.exists(gems_json_path):
            with open(gems_json_path, 'r') as f:
                gems_file_object = json.load(f)
        else:
            gems_file_object = {
                "GemListFormatVersion": 2
            }

        gems_list = gems_file_object.setdefault(test_constant.DIR_GEMS, [])

        found = False
        for gem_object in gems_list:
            if gem_object.get('Path') == relative_gem_path:
                found = True
                break

        if not found:
            dependency_object = self.__get_gem_dependency_object(temp_dir, relative_gem_path)
            gems_list.append(dependency_object)

        print('Saving gem file at {}'.format(gems_json_path))

        with open(gems_json_path, 'w+') as f:
            json.dump(gems_file_object, f)

    def __get_gem_dependency_object(self, temp_dir, relative_gem_path):

        path = os.path.join(temp_dir, relative_gem_path.replace('/', os.sep), 'gem.json')

        with open(path) as file:
            content = json.load(file)

        return {
            "Path": relative_gem_path,
            "Uuid": content['Uuid'],
            "Version": content['Version'],
            "_comment": content['Name']
        }

    def __remove_from_gems_file(self, gem_name, gem_path=None, gem_version_directory_name=None):

        gems_json_path = os.path.join(self.GAME_DIR, 'gems.json')

        if os.path.exists(gems_json_path):
            with open(gems_json_path, 'r') as f:
                gems_file_object = json.load(f)
        else:
            gems_file_object = {}

        gems_list = gems_file_object.setdefault(test_constant.DIR_GEMS, [])

        if not gem_path:
            if gem_version_directory_name:
                gem_path = os.path.join(self.REAL_ROOT_DIR, test_constant.DIR_GEMS, gem_name, gem_version_directory_name)
            else:
                gem_path = os.path.join(self.REAL_ROOT_DIR, test_constant.DIR_GEMS, gem_name)

        for gem_object in gems_list:
            if gem_object.get('Path') == gem_path:
                gems_list.remove(gem_object)

        print('Saving gem file at {}'.format(gems_json_path))

        with open(gems_json_path, 'w+') as f:
            json.dump(gems_file_object, f)

    def copy_support_file(self, file_path):
        src_file_path = os.path.join(self.REAL_ROOT_DIR, file_path)
        if not os.path.exists(src_file_path):
            raise RuntimeError('Source file does not exist: {}'.format(src_file_path))
        dst_file_path = os.path.join(self.ROOT_DIR, file_path)
        print('Copying {} to {}'.format(src_file_path, dst_file_path))
        if os.path.exists(dst_file_path):
            print('File already exists: {} (Attempting overwrite)'.format(dst_file_path))
        dest_path, dest_file = os.path.split(dst_file_path)
        if not os.path.exists(dest_path):
            os.mkdir(dest_path)
        shutil.copyfile(src_file_path, dst_file_path)

    def enable_shared_gem(self, gem_name, version_subdirectory=None, ignore_existing_symlinks=True, path=None):
        if path and version_subdirectory:
            path = os.path.join(path, version_subdirectory)

        self.__add_gem_to_shared_resources(gem_name, version_subdirectory, path)
        self.enable_real_gem(gem_name, version_subdirectory, ignore_existing_symlinks, path)

    def enable_real_gem(self, gem_name, version_subdirectory=None, ignore_existing_symlinks=True, path=None):
        self.__enable_real_gem(self.ROOT_DIR, self.GAME_NAME, gem_name, version_subdirectory, ignore_existing_symlinks, path)

    def __enable_real_gem(self, temp_dir, game_name, gem_name, version_subdirectory=None, ignore_existing_symlinks=False, path=None):
        if path:
            rel_path = path
        else:
            rel_path = self.__get_relative_gem_path(gem_name, version_subdirectory)
        self.__make_gem_link(temp_dir, gem_name, ignore_existing_symlinks, rel_path if path else None)
        self.__add_to_gems_file(temp_dir, game_name, rel_path)

        # Ensure the gem resources are available by checking the symlink exists.  We verify this by checking the existence of the gem.json file.
        end = datetime.datetime.utcnow() + datetime.timedelta(minutes=1)
        while datetime.datetime.utcnow() <= end:
            print('{} {}'.format(datetime.datetime.utcnow(), end))
            if os.path.isfile(os.path.join(rel_path, "gem.json")):
                break
            time.sleep(2)

    def __add_gem_to_shared_resources(self, gem_name, version_subdirectory, path=None):
        self.shared_resource_manager.update_shared_resource_context_attr(
            SharedResourceManager.CONTEXT_REGISTERED_GEM,
            lambda value: (
                self.shared_resource_manager.append_shared_gem(gem_name, version_subdirectory, value, path)
            )
        )

    def disable_real_gem(self, gem_name, version_subdirectory=None):
        self.__remove_from_gems_file(self.__get_relative_gem_path(gem_name, version_subdirectory))

    def __get_relative_gem_path(self, gem_name, version_subdirectory):
        path_prefix = 'Gems/'
        relative_gem_path = gem_name
        if path_prefix not in relative_gem_path:
            relative_gem_path = 'Gems/' + gem_name
        if version_subdirectory:
            relative_gem_path += '/' + version_subdirectory
        return relative_gem_path

    def find_arg(self, args, arg_name, default):
        next = False
        for arg in args:
            if next:
                return arg
            if arg == arg_name:
                next = True
        return default

    def get_project_user_credentials(self):
        user = self.context.get(test_constant.ATTR_USERS, {}).get(self.get_project_user_name())
        if not user:
            self.create_project_user()
            user = self.context[test_constant.ATTR_USERS].get(self.get_project_user_name())
        return Credentials(user['AccessKeyId'], user['SecretAccessKey'])

    def get_deployment_user_credentials(self, deployment_name):
        user = self.context.get(test_constant.ATTR_USERS, {}).get(self.get_deployment_user_name(deployment_name))
        if not user:
            self.create_deployment_user(deployment_name)
            user = self.context[test_constant.ATTR_USERS].get(self.get_deployment_user_name(deployment_name))
        return Credentials(user['AccessKeyId'], user['SecretAccessKey'])

    def lmbr_aws(self, *args, **kwargs):

        expect_failure = kwargs.get('expect_failure', False)
        ignore_failure = kwargs.get('ignore_failure', False)
        credentials = {}

        assumed_role = kwargs.get('project_role', None)
        if assumed_role:
            credentials = self.get_project_user_credentials()
        else:
            assumed_role = kwargs.get('deployment_role', None)
            if assumed_role:
                deployment_name = self.find_arg(args, '--deployment', self.TEST_DEPLOYMENT_NAME)
                credentials = self.get_deployment_user_credentials(deployment_name)

        sys.argv = ['']
        sys.argv.extend(args)

        sys.argv.extend(['--root-directory', self.ROOT_DIR])
        sys.argv.extend(['--no-prompt'])

        if assumed_role:
            sys.argv.extend(['--assume-role', assumed_role, '--aws-access-key', credentials.access_key, '--aws-secret-key', credentials.secret_key])
        else:
            sys.argv.extend(self.lmbr_aws_credentials_args)

        display = 'lmbr_aws'

        for arg in sys.argv[1:]:
            display = display + ' ' + arg
        print('\n\n{}\n'.format(display))

        with self.captured_output() as (out, err):
            res = resource_manager.cli.main()

        self.lmbr_aws_stdout = out.getvalue()
        self.lmbr_aws_stderr = err.getvalue()

        if ignore_failure:
            return res

        if expect_failure:
            self.assertNotEqual(res, 0)
        else:
            self.assertEqual(res, 0)
        return res

    def load_local_project_settings(self):
        if os.path.exists(self.LOCAL_PROJECT_SETTINGS_FILE_PATH):
            with open(self.LOCAL_PROJECT_SETTINGS_FILE_PATH, 'r') as f:
                setting = json.load(f)
                if test_constant.DEFAULT not in setting:
                    return {}
                attr = setting[test_constant.DEFAULT][test_constant.SET]
                if attr not in setting:
                    return {}
                return setting[attr]
        else:
            return {}

    def load_cloud_project_settings(self):
        project_stack_arn = self.get_project_stack_arn()
        if not project_stack_arn:
            return {}
        config_bucket_id = self.get_stack_resource_physical_id(project_stack_arn, 'Configuration')
        res = self.aws_s3.list_objects_v2(Bucket=config_bucket_id, Prefix="dstack")

        cloud_deployments = res.get('Contents', {})
        deployments = {
            "deployment": {}
        }
        entries = deployments["deployment"]
        for deployment in cloud_deployments:
            name = deployment['Key']
            prefix, group, deployment_name, ext = name.split('.')
            if group == "ReleaseDeployment" or group == "DefaultDeployment":
                deployments[group] = self.aws_s3.get_object(Bucket=config_bucket_id, Key=name)["Body"].read()
            else:
                definition = json.load(self.aws_s3.get_object(Bucket=config_bucket_id, Key=name)["Body"])
                entries[deployment_name] = definition

        return deployments

    def refresh_stack_description(self, arn):
        self.stack_descriptions[arn] = self.aws_cloudformation.describe_stacks(StackName=arn)['Stacks'][0]

    def get_stack_description(self, arn):
        description = self.stack_descriptions.get(arn)
        if not description:
            self.refresh_stack_description(arn)
            description = self.stack_descriptions.get(arn)
        return description

    def get_stack_output(self, arn, output_name):
        description = self.get_stack_description(arn)
        outputs = description.get('Outputs', [])
        for output in outputs:
            if output.get('OutputKey') == output_name:
                return output.get('OutputValue')
        return None

    def refresh_stack_resources(self, arn):
        self.stack_resource_descriptions[arn] = self.aws_cloudformation.describe_stack_resources(StackName=arn)

    def get_stack_resource(self, stack_arn, logical_resource_id):
        describe_stack_resources_result = self.stack_resource_descriptions.get(stack_arn, None)
        if describe_stack_resources_result is None:
            describe_stack_resources_result = self.aws_cloudformation.describe_stack_resources(StackName=stack_arn)
            self.stack_resource_descriptions[stack_arn] = describe_stack_resources_result

        for resource in describe_stack_resources_result['StackResources']:
            if resource['LogicalResourceId'] == logical_resource_id:
                return resource
        self.fail('Resource {} not found in stack {}'.format(logical_resource_id, stack_arn))

    def get_stack_resource_physical_id(self, stack_arn, logical_resource_id):
        resource = self.get_stack_resource(stack_arn, logical_resource_id)
        return resource['PhysicalResourceId'] if resource else {}

    def get_stack_resource_arn(self, stack_arn, logical_resource_id):
        resource_description = self.get_stack_resource(stack_arn, logical_resource_id)
        return self.get_stack_resource_description_arn(resource_description)

    def get_stack_resource_description_arn(self, resource_description):
        resource_type = resource_description['ResourceType']
        if resource_type in ['AWS::CloudFormation::Stack', 'AWS::IAM::ManagedPolicy']:
            return resource_description['PhysicalResourceId']
        elif resource_type == 'AWS::IAM::Role':
            res = self.aws_iam.get_role(RoleName=resource_description['PhysicalResourceId'])
            return res['Role']['Arn']
        else:
            stack = self.stack_info_manager.get_stack_info(resource_description['StackId'], session=self.session)
            return aws_utils.get_resource_arn(
                stack.resource_definitions,
                resource_description['StackId'],
                resource_description['ResourceType'],
                resource_description['PhysicalResourceId'],
                lambda_client=self.aws_lambda)

    def get_stack_name_from_arn(self, arn):
        # Stack ARN format: arn:aws:cloudformation:{region}:{account}:stack/{name}/{guid}
        if arn is None:
            return None
        return arn.split('/')[1]

    def get_region_from_arn(self, arn):
        # Stack ARN format: arn:aws:cloudformation:{region}:{account}:stack/{name}/{guid}
        if arn is None:
            return None
        return arn.split(':')[3]

    def get_account_id_from_arn(self, arn):
        # Stack ARN format: arn:aws:cloudformation:{region}:{account}:stack/{name}/{guid}
        if arn is None:
            return None
        return arn.split(':')[4]

    def get_role_name_from_arn(self, arn):
        # Role ARN format: arn:aws:iam::{account_id}:role/{resource_name}
        if arn is None:
            return None
        return arn[arn.rfind('/') + 1:]

    def get_project_stack_arn(self):
        settings = self.load_local_project_settings()
        return settings.get('ProjectStackId') if settings else None

    def get_resource_group_stack_arn(self, deployment_name, resource_group_name):
        deployment_stack_arn = self.get_deployment_stack_arn(deployment_name)
        return self.get_stack_resource_arn(deployment_stack_arn, resource_group_name)

    def get_deployment_stack_arn(self, deployment_name):
        deployment = self.get_deployment_settings(deployment_name)
        return deployment['DeploymentStackId'] if 'DeploymentStackId' in deployment else None

    def get_deployment_access_stack_arn(self, deployment_name):
        deployment = self.get_deployment_settings(deployment_name)
        return deployment['DeploymentAccessStackId'] if 'DeploymentAccessStackId' in deployment else None

    def get_deployment_settings(self, deployment_name):
        settings = self.load_cloud_project_settings()

        if not settings:
            return {}

        deployments = settings['deployment']
        if not deployments or deployment_name not in deployments:
            return {}
        return deployments[deployment_name]

    def verify_s3_object_does_not_exist(self, bucket, key):
        try:
            res = self.aws_s3.head_object(Bucket=bucket, Key=key)
            self.fail("s3 bucket {} object {} was not deleted. head_object returned {}".format(bucket, key, res))
        except ClientError as e:
            self.assertEquals(e.response['Error']['Code'], '404')

    def verify_s3_object_exists(self, bucket, key):
        try:
            res = self.aws_s3.head_object(Bucket=bucket, Key=key)
        except NotFoundException as e:
            self.fail("head_object(Bucket='{}', Key='{}') failed: {}".format(bucket, key, e))

        if res.get('DeleteMarker', False):
            self.fail("head_object(Bucket='{}', Key='{}') -> DeleteMarker is True".format(bucket, key))

        if res.get('ContentLength', 0) == 0:
            self.fail("head_object(Bucket='{}', Key='{}') -> ContentLength is 0".format(bucket, key))

    def verify_s3_object_missing(self, bucket, key) -> bool:
        try:
            self.aws_s3.head_object(Bucket=bucket, Key=key)
        except ClientError as e:
            if e.response['Error']['Code'] == "404":
                return True
            else:
                raise e

        self.fail("head_object(Bucket='{}', Key='{}') -> Was located".format(bucket, key))

    def verify_stack(self, context, stack_arn, spec, exact=True):
        verified = False
        if 'StackStatus' in spec:
            res = self.aws_cloudformation.describe_stacks(StackName=stack_arn)
            self.assertEqual(res['Stacks'][0]['StackStatus'], spec['StackStatus'],
                             'Stack Status {} when expected {} for stack with context {}.'.format(res['Stacks'][0]['StackStatus'], spec['StackStatus'],
                                                                                                  stack_arn, context))
            verified = True

        if 'StackResources' in spec:
            self.verify_stack_resources(context, stack_arn, spec['StackResources'], exact)
            verified = True

        self.assertTrue(verified)

    def verify_stack_resources(self, context, stack_arn, expected_resources, exact=True):

        res = self.aws_cloudformation.describe_stack_resources(StackName=stack_arn)
        stack_resources = res['StackResources']

        resources_seen = []

        for stack_resource in stack_resources:

            expected_resource = expected_resources.get(stack_resource['LogicalResourceId'], None)
            if exact:
                self.assertIsNotNone(expected_resource, 'Unexpected Resource {} in Stack {}'.format(stack_resource, stack_arn))
            else:
                continue

            resources_seen.append(stack_resource['LogicalResourceId'])

            self.assertEquals(expected_resource['ResourceType'], stack_resource['ResourceType'],
                              'Expected type {} on resource {} in stack {}, Found {}'.format(
                                  expected_resource['ResourceType'],
                                  stack_resource['LogicalResourceId'],
                                  stack_arn, stack_resource['ResourceType']))

            if 'Permissions' in expected_resource:
                self.expand_resource_references(expected_resource['Permissions'], stack_resources)

            handler = self.verify_handlers.get(expected_resource['ResourceType'], None)
            if handler is not None:
                handler(self, context + ' resource ' + stack_resource['LogicalResourceId'], stack_resource, expected_resource)

        if exact:
            self.assertEquals(sorted(resources_seen), sorted(expected_resources.keys()),
                              'The examined stack \'{}\' has additional resources outside the expected ones. \nSeen: {}\nExpected: {}'.format(stack_arn, sorted(
                                  resources_seen), sorted(expected_resources.keys())))

    def expand_resource_references(self, permissions, stack_resources):
        for permission in permissions:
            permission_resources = permission['Resources']
            for permission_resource_index, permission_resource in enumerate(permission_resources):
                if permission_resource.startswith('$'):
                    permission_resource_name = permission_resource[1:-1]
                    for stack_resource in stack_resources:
                        if stack_resource['LogicalResourceId'] == permission_resource_name:
                            permission_resources[permission_resource_index] = self.get_stack_resource_description_arn(stack_resource)

    def verify_child_stack(self, context, stack_resource, expected_resource):
        context += ' child stack ' + stack_resource['LogicalResourceId']
        self.verify_stack(context, stack_resource['PhysicalResourceId'], expected_resource)

    def verify_lambda_function(self, context, stack_resource, expected_resource):
        if 'Permissions' in expected_resource:
            get_function_configuration_res = self.aws_lambda.get_function_configuration(FunctionName=stack_resource['PhysicalResourceId'])
            role_name = self.get_role_name_from_arn(get_function_configuration_res['Role'])
            self.verify_role_permissions(context + ' role ' + role_name, stack_resource['StackId'], role_name, expected_resource['Permissions'])

    def verify_iam_role(self, context, stack_resource, expected_resource):
        if 'Permissions' in expected_resource:
            self.verify_role_permissions(context, stack_resource['StackId'], stack_resource['PhysicalResourceId'], expected_resource['Permissions'])

    def verify_iam_managed_policy(self, context, stack_resource, expected_resource):
        if 'Permissions' in expected_resource:
            self.verify_managed_policy_permissions(context, stack_resource['PhysicalResourceId'], expected_resource['Permissions'])

    def verify_role_permissions(self, context, stack_arn, role_name, permissions, context_entries=None):
        policy_documents = []

        # get inline policy documents

        list_role_policies_res = self.aws_iam.list_role_policies(RoleName=role_name)
        # print '*** list_role_policies_res', role_name, list_role_policies_res
        for policy_name in list_role_policies_res['PolicyNames']:
            get_role_policy_res = self.aws_iam.get_role_policy(RoleName=role_name, PolicyName=policy_name)
            # print '*** get_role_policy_res', role_name, policy_name, get_role_policy_res
            policy_documents.append(json.dumps(get_role_policy_res['PolicyDocument']))

        # get attached policy documents

        list_attached_role_policies_res = self.aws_iam.list_attached_role_policies(RoleName=role_name)
        # print '*** list_attached_role_policies_res', role_name, list_attached_role_policies_res
        for attached_policy in list_attached_role_policies_res['AttachedPolicies']:
            policy_arn = attached_policy['PolicyArn']
            list_policy_versions_res = self.aws_iam.list_policy_versions(PolicyArn=policy_arn)
            # print '*** list_policy_versions_res', policy_arn, list_policy_versions_res
            for policy_version in list_policy_versions_res['Versions']:
                if policy_version['IsDefaultVersion']:
                    get_policy_version_res = self.aws_iam.get_policy_version(PolicyArn=policy_arn, VersionId=policy_version['VersionId'])
                    # print '*** get_policy_version_res', policy_arn, policy_version['VersionId'], get_policy_version_res
                    policy_documents.append(json.dumps(get_policy_version_res['PolicyVersion']['Document'], indent=4))

        # verify using the accumulated policy documents

        context += ' role ' + role_name

        self.verify_permissions(context, policy_documents, permissions, context_entries)

    def verify_managed_policy_permissions(self, context, policy_arn, permissions):
        policy_documents = []

        # get policy document

        list_policy_versions_res = self.aws_iam.list_policy_versions(PolicyArn=policy_arn)
        # print '*** list_policy_versions_res', policy_arn, list_policy_versions_res
        for policy_version in list_policy_versions_res['Versions']:
            if policy_version['IsDefaultVersion']:
                get_policy_version_res = self.aws_iam.get_policy_version(PolicyArn=policy_arn, VersionId=policy_version['VersionId'])
                # print '*** get_policy_version_res', policy_arn, policy_version['VersionId'], get_policy_version_res
                policy_documents.append(json.dumps(get_policy_version_res['PolicyVersion']['Document'], indent=4))

        # verify using the accumulated policy documents

        context += ' managed policy ' + policy_arn

        self.verify_permissions(context, policy_documents, permissions)

    def verify_permissions(self, context, policy_documents, permissions, context_entries=None):
        if context_entries is None:
            context_entries = []

        for permission in permissions:
            description = permission.get('Description', None)

            if 'Allow' in permission:
                self.assertNotIn('Deny', permission)
                action_names = permission['Allow']
                expect_allowed = True
            elif 'Deny' in permission:
                self.assertNotIn('Allow', permission)
                action_names = permission['Deny']
                expect_allowed = False
            else:
                self.fail('For "{}" neither Allow or Deny was specified for permission {} in context {}'.format(description, permission, context))

            resource_arns = permission.get('Resources', [])

            simulate_custom_policy_res = self.aws_iam.simulate_custom_policy(
                PolicyInputList=policy_documents,
                ActionNames=action_names,
                ResourceArns=resource_arns,
                ContextEntries=context_entries)

            # print '**** simulate_custom_policy_res', json.dumps(simulate_custom_policy_res, indent=4)

            def find_evaluation_result(action, resource_arn):
                for evaluation_result in simulate_custom_policy_res['EvaluationResults']:
                    # print '*** evaluation_result', json.dumps(evaluation_result, indent=4)
                    if evaluation_result['EvalActionName'] == action:
                        if 'ResourceSpecificResults' in evaluation_result:
                            for resource_specific_result in evaluation_result['ResourceSpecificResults']:
                                if resource_specific_result['EvalResourceName'] == resource_arn:
                                    return evaluation_result
                        else:
                            if evaluation_result['EvalResourceName'] == resource_arn:
                                return evaluation_result
                self.fail('No evaluation result found for action {} and resource {} for context {}'.format(action, resource_arn, context))

            def format_error_message(expected_permission):

                dumped_policy_documents = ""
                for policy_document in policy_documents:
                    dumped_policy_documents = dumped_policy_documents + '\n' + json.dumps(json.loads(policy_document), indent=4) + '\n'

                return 'For "{}" expected permission {} for action {} and resource {} in context {}.\n\nEvaluation Result:\n\n{}\n\nPolicy Documents:\n{}'.format(
                    description,
                    expected_permission,
                    action,
                    resource_arn,
                    context,
                    json.dumps(evaluation_result, indent=4),
                    dumped_policy_documents
                )

            for action in action_names:
                for resource_arn in resource_arns:
                    evaluation_result = find_evaluation_result(action, resource_arn)
                    if expect_allowed:
                        self.assertEqual(evaluation_result['EvalDecision'], 'allowed', format_error_message('allowed'))
                    else:
                        self.assertNotEqual(evaluation_result['EvalDecision'], 'allowed', format_error_message('denied'))

    def get_mapping(self, resource_group_name, resource_name):
        user_settings_file_path = os.path.join(self.ROOT_DIR, resource_manager_common.constant.PROJECT_CACHE_DIRECTORY_NAME, test_constant.GAME_NAME, "pc",
                                               resource_manager_common.constant.PROJECT_USER_DIRECTORY_NAME,
                                               resource_manager_common.constant.PROJECT_AWS_DIRECTORY_NAME,
                                               resource_manager_common.constant.USER_SETTINGS_FILENAME)
        with open(user_settings_file_path, 'r') as user_settings_file:
            user_settings = json.load(user_settings_file)
        mappings = user_settings.get('Mappings', {})
        mapping_name = resource_group_name + '.' + resource_name
        mapping = mappings.get(mapping_name, None)
        self.assertIsNotNone(mapping, 'Missing mapping for {}'.format(mapping_name))
        return mapping

    def verify_user_mappings(self, deployment_name, logical_ids, expected_physical_resource_ids=None):
        if expected_physical_resource_ids is None:
            expected_physical_resource_ids = {}

        user_settings_file_path = os.path.join(self.ROOT_DIR, resource_manager_common.constant.PROJECT_CACHE_DIRECTORY_NAME, test_constant.GAME_NAME, "pc",
                                               resource_manager_common.constant.PROJECT_USER_DIRECTORY_NAME,
                                               resource_manager_common.constant.PROJECT_AWS_DIRECTORY_NAME,
                                               resource_manager_common.constant.USER_SETTINGS_FILENAME)
        print('Verifying mappings in {}'.format(user_settings_file_path))
        with open(user_settings_file_path, 'r') as user_settings_file:
            user_settings = json.load(user_settings_file)
        mappings = user_settings.get('Mappings', {})
        self.__verify_mappings(mappings, deployment_name, logical_ids, expected_physical_resource_ids=expected_physical_resource_ids)

    def verify_release_mappings(self, deployment_name, logical_ids, role, expected_physical_resource_ids=None):
        if expected_physical_resource_ids is None:
            expected_physical_resource_ids = {}

        release_mappings_file_name = deployment_name + '.' + role.lower() + '.awsLogicalMappings.json'
        release_mappings_file_path = os.path.join(self.GAME_DIR, 'Config', release_mappings_file_name)
        print('Verifying mappings in {}'.format(release_mappings_file_path))
        self.assertTrue(os.path.exists(release_mappings_file_path))
        with open(release_mappings_file_path, 'r') as release_mappings_file:
            release_mappings_contents = json.load(release_mappings_file)
        mappings = release_mappings_contents.get('LogicalMappings', {})
        self.__verify_mappings(mappings, deployment_name, logical_ids, expected_physical_resource_ids=expected_physical_resource_ids)

        if role == 'Server':
            self.assertIn('server_role_arn', mappings)
            self.assertEquals(mappings['server_role_arn']['ResourceType'], 'Configuration')
            self.assertIn('PhysicalResourceId', mappings['server_role_arn'])

    def __verify_mappings(self, mappings, deployment_name, logical_ids, expected_physical_resource_ids):
        print('mappings {}'.format(mappings))
        print('logical_ids {}'.format(logical_ids))
        print('expected_physical_resource_ids {}'.format(expected_physical_resource_ids))

        for logical_id in logical_ids:
            mapping = mappings.get(logical_id, None)
            self.assertIsNotNone(mapping, "Missing mapping " + logical_id)
            logical_id_parts = logical_id.split('.')
            resource_group_name = logical_id_parts[0]
            logical_resource_id = logical_id_parts[1]

            resource_group_stack_arn = self.get_resource_group_stack_arn(deployment_name, resource_group_name)
            resource = self.get_stack_resource(resource_group_stack_arn, logical_resource_id)

            # Expected_physical_resource_ids provides overrides to the default, which is 
            # the physical resource id as provided by cloud formation. This might get
            # cleaned up a little when we add a mapping hook for cloud gems. 

            if resource['ResourceType'] == 'Custom::ServiceApi':
                expected_physical_resource_id = expected_physical_resource_ids.get(logical_id, None)
                if not expected_physical_resource_id:
                    expected_physical_resource_id = self.get_stack_output(resource['StackId'], 'ServiceUrl')
            else:
                expected_physical_resource_id = expected_physical_resource_ids.get(logical_id, resource.get('PhysicalResourceId'))

            self.assertEquals(mapping['PhysicalResourceId'], expected_physical_resource_id,
                              mapping['PhysicalResourceId'] + " != " + expected_physical_resource_id + " for mapping " + logical_id)
            self.assertEquals(mapping['ResourceType'], resource['ResourceType'],
                              mapping['ResourceType'] + " != " + resource['ResourceType'] + "for mapping " + logical_id)

        self.assertIn('account_id', mappings)
        self.assertEquals(mappings['account_id']['ResourceType'], 'Configuration', 'for mapping account_id')
        self.assertIn('PhysicalResourceId', mappings['account_id'], 'for mapping account_id')

        self.assertIn('region', mappings)
        self.assertEquals(mappings['region']['ResourceType'], 'Configuration', 'for mapping region')
        self.assertEquals(mappings['region']['PhysicalResourceId'], self.TEST_REGION, 'for mapping region')

        self.assertIn('PlayerAccessIdentityPool', mappings)
        self.assertEquals(mappings['PlayerAccessIdentityPool']['ResourceType'], 'Custom::CognitoIdentityPool', 'for mapping PlayerAccessIdentityPool')
        physical_resource_id = custom_resource_utils.get_embedded_physical_id(
            self.get_stack_resource_physical_id(self.get_deployment_access_stack_arn(deployment_name), 'PlayerAccessIdentityPool'))
        self.assertEquals(mappings['PlayerAccessIdentityPool']['PhysicalResourceId'], physical_resource_id, 'for mapping PlayerAccessIdentityPool')

        self.assertIn('PlayerLoginIdentityPool', mappings)
        self.assertEquals(mappings['PlayerLoginIdentityPool']['ResourceType'], 'Custom::CognitoIdentityPool', 'for mapping PlayerLoginIdentityPool')
        physical_resource_id = custom_resource_utils.get_embedded_physical_id(
            self.get_stack_resource_physical_id(self.get_deployment_access_stack_arn(deployment_name), 'PlayerLoginIdentityPool'))
        self.assertEquals(mappings['PlayerLoginIdentityPool']['PhysicalResourceId'], physical_resource_id, 'for mapping PlayerLoginIdentityPool')

        self.assertIn('PlayerAccessTokenExchange', mappings)
        self.assertEquals(mappings['PlayerAccessTokenExchange']['ResourceType'], 'AWS::Lambda::Function', 'for mapping PlayerAccessTokenExchange')
        physical_resource_id = custom_resource_utils.get_embedded_physical_id(
            self.get_stack_resource_physical_id(self.get_project_stack_arn(), 'PlayerAccessTokenExchange'))
        self.assertEquals(mappings['PlayerAccessTokenExchange']['PhysicalResourceId'], physical_resource_id, 'for mapping PlayerAccessTokenExchange')

        for logical_id in mappings.keys():
            if logical_id not in ['account_id', 'region', 'PlayerAccessIdentityPool', 'PlayerLoginIdentityPool', 'PlayerAccessTokenExchange',
                                  'server_role_arn']:
                self.assertIn(logical_id, logical_ids, 'unexpected mapping: ' + logical_id)

    def get_gem_path(self, gem_name, *args):
        # a more robust solution would open the gem.json and compare against name in that....
        for gem in self.read_project_json('gems.json').get('Gems', []):
            if gem.get('_comment') == gem_name:
                return os.path.join(self.ROOT_DIR, gem.get('Path', '').replace('/', os.sep), *args)
        raise RuntimeError('Could not get path for gem {} because it is not enabled for the project.'.format(gem_name))

    def get_gem_aws_path(self, gem_name, *args):
        return self.get_gem_path(gem_name, 'AWS', *args)

    def get_project_path(self, *args):
        return os.path.join(self.GAME_DIR, *args)

    def get_project_aws_path(self, *args):
        return self.get_project_path('AWS', *args)

    @classmethod
    def is_runnable(cls, methodname, dict):
        if methodname in dict and dict[methodname] == 1:
            return False
        return True

    class OutputCapture:

        def __init__(self, target):
            self.__everything = ''
            self.__target = target

        def write(self, txt):
            self.__target.write(txt)
            self.__target.flush()
            self.__everything += txt

        def getvalue(self):
            return self.__everything

        def flush(self):
            self.__everything = ''

    @contextmanager
    def captured_output(self):
        new_out, new_err = self.OutputCapture(sys.stdout), self.OutputCapture(sys.stderr)
        old_out, old_err = sys.stdout, sys.stderr
        try:
            sys.stdout, sys.stderr = new_out, new_err
            yield sys.stdout, sys.stderr
        finally:
            sys.stdout, sys.stderr = old_out, old_err

    def create_project_user(self):

        user_name = self.get_project_user_name()

        project_stack_arn = self.get_project_stack_arn()
        project_access_policy_arn = self.get_stack_resource_arn(project_stack_arn, 'ProjectAccess')

        assume_role_path = self.TEST_PROJECT_STACK_NAME

        self.create_user(user_name, project_access_policy_arn, assume_role_path)

    def delete_project_user(self):
        user_name = self.get_project_user_name()
        self.delete_user(user_name)

    def get_project_user_name(self):
        return self.TEST_PROJECT_STACK_NAME + 'Project'

    def get_deployment_user_name(self, deployment_name):
        return self.TEST_PROJECT_STACK_NAME + 'Deployment' + deployment_name

    def create_deployment_user(self, deployment_name):

        user_name = self.get_deployment_user_name(deployment_name)

        deployment_access_stack_arn = self.get_deployment_access_stack_arn(deployment_name)
        deployment_access_policy_arn = self.get_stack_resource_arn(deployment_access_stack_arn, 'DeploymentAccess')

        assume_role_path = self.TEST_PROJECT_STACK_NAME + '/' + deployment_name

        self.create_user(user_name, deployment_access_policy_arn, assume_role_path)

    def delete_deployment_user(self, deployment_name):
        user_name = self.get_deployment_user_name(deployment_name)
        self.delete_user(user_name)

    def create_user(self, user_name, attached_policy_arn, assume_role_path):

        res = self.aws_iam.create_user(UserName=user_name)

        res = self.aws_iam.create_access_key(UserName=user_name)
        self.context.setdefault(test_constant.ATTR_USERS, {})[user_name] = {
            'AccessKeyId': res['AccessKey']['AccessKeyId'],
            'SecretAccessKey': res['AccessKey']['SecretAccessKey'],
            'AttachedPolicyArn': attached_policy_arn
        }

        res = self.aws_iam.attach_user_policy(
            UserName=user_name,
            PolicyArn=attached_policy_arn
        )

        # arn:aws:iam::<account-id>:policy/...
        attached_policy_arn_parts = attached_policy_arn.split(':')
        account_id = attached_policy_arn_parts[4]

        res = self.aws_iam.put_user_policy(
            UserName=user_name,
            PolicyName='AssumeRoles',
            PolicyDocument='''{
                "Version": "2012-10-17",
                "Statement": {
                    "Effect": "Allow",
                    "Action": "sts:AssumeRole",
                    "Resource": "arn:aws:iam::''' + account_id + ''':role/''' + assume_role_path + '''/*"
                }
            }'''
        )

        print('waiting 15 seconds after creating user...')
        sleep(15)

    def delete_user(self, user_name):

        user = self.context[test_constant.ATTR_USERS].get(user_name)
        if not user:
            return

        try:
            res = self.aws_iam.delete_access_key(
                UserName=user_name,
                AccessKeyId=user['AccessKeyId']
            )
        except ClientError as e:
            if e.response['Error']['Code'] != 'NoSuchEntity':
                raise e

        try:
            res = self.aws_iam.detach_user_policy(
                UserName=user_name,
                PolicyArn=user['AttachedPolicyArn']
            )
        except ClientError as e:
            if e.response['Error']['Code'] != 'NoSuchEntity':
                raise e

        try:
            res = self.aws_iam.delete_user_policy(
                UserName=user_name,
                PolicyName='AssumeRoles'
            )
        except ClientError as e:
            if e.response['Error']['Code'] != 'NoSuchEntity':
                raise e

        try:
            res = self.aws_iam.delete_user(UserName=user_name)
        except ClientError as e:
            if e.response['Error']['Code'] != 'NoSuchEntity':
                raise e

        del self.context[test_constant.ATTR_USERS][user_name]

    def make_project_json(self, content, *path_parts):
        file_path = self.get_project_path(*path_parts)
        dir_path = os.path.dirname(file_path)
        if not os.path.exists(dir_path):
            os.makedirs(dir_path)
        with open(file_path, 'w') as file:
            json.dump(content, file, indent=4, sort_keys=True)

    def make_project_aws_json(self, content, *path_parts):
        return self.make_project_json('AWS', *path_parts)

    def make_gem_json(self, content, gem_name, *path_parts):
        return self.make_gem_file(json.dumps(content, indent=4, sort_keys=True), gem_name, *path_parts)

    def make_gem_aws_json(self, content, gem_name, *path_parts):
        return self.make_gem_aws_file(json.dumps(content, indent=4, sort_keys=True), gem_name, *path_parts)

    def make_gem_file(self, content, gem_name, *path_parts):
        file_path = self.get_gem_path(gem_name, *path_parts)
        dir_path = os.path.dirname(file_path)
        if not os.path.exists(dir_path):
            os.makedirs(dir_path)
        with open(file_path, 'w') as file:
            file.write(content)

    def make_gem_aws_file(self, content, gem_name, *path_parts):
        return self.make_gem_file(content, gem_name, 'AWS', *path_parts)

    def read_project_json(self, *path_parts):
        with open(self.get_project_path(*path_parts), 'r') as file:
            return json.load(file)

    def read_project_aws_json(self, *path_parts):
        return self.read_project_json('AWS', *path_parts)

    def read_gem_json(self, gem_name, *path_parts):
        with open(self.get_gem_path(gem_name, *path_parts), 'r') as file:
            return json.load(file)

    def read_gem_aws_json(self, gem_name, *path_parts):
        return self.read_gem_json(gem_name, 'AWS', *path_parts)

    def edit_project_aws_json(self, *path_parts):
        return self.EditableJsonDocument(self.get_project_aws_path(*path_parts))

    def edit_gem_aws_json(self, gem_name, *path_parts):
        return self.EditableJsonDocument(self.get_gem_aws_path(gem_name, *path_parts))

    def edit_project_json(self, *path_parts):
        return self.EditableJsonDocument(self.get_project_path(*path_parts))

    def edit_gem_json(self, gem_name, *path_parts):
        return self.EditableJsonDocument(self.get_gem_path(gem_name, *path_parts))

    class EditableJsonDocument:
        def __init__(self, document_path):
            self.__document_path = document_path

        def __enter__(self):
            with open(self.__document_path, 'r') as f:
                self.__document = json.load(f)
                return self.__document

        def __exit__(self, type, value, traceback):
            if not type and not value and not traceback:
                with open(self.__document_path, 'w') as f:
                    json.dump(self.__document, f, indent=2, sort_keys=True)
            return False

    @contextlib.contextmanager
    def spy_decorator(self, method_to_decorate):
        """Provides a MagicMock that will record calls to a specified method while still executing the original method."""

        def get_class_that_defined_method(method):
            method_name = method.__name__
            if method.__self__:
                classes = [method.__self__.__class__]
            else:
                classes = [method.im_class]
            while classes:
                c = classes.pop()
                if method_name in c.__dict__:
                    return c
                else:
                    classes = list(c.__bases__) + classes
            return None

        _mock = mock.MagicMock()

        def wrapper(self, *args, **kwargs):
            _mock(*args, **kwargs)
            return method_to_decorate(self, *args, **kwargs)

        wrapper.mock = _mock
        with mock.patch.object(get_class_that_defined_method(method_to_decorate), method_to_decorate.__name__, new=wrapper):
            yield _mock

    def get_service_client(self, assumed_role=None, use_aws_auth=True, stack_id=None, access_stack_id=None):
        """Gets a service client for a stack's service api.

        Arguments:

           assumed_role: the name of a role to assume. A role with this name is looked for in
           the project and TEST_DEPLOYMENT_NAME deployment access stacks. Parameter use_aws_auth is ignored (AWS auth is always used), if this parameter is set.

           use_aws_auth: if True, then the local AWS credentials will be used to make the request. Otherwise the request will not be signed.

           stack_id: identifies the stack that provides the service api. If not specified, the stack identified using TEST_DEPLOYMENT_NAME and
           TEST_RESOURCE_GROUP_NAME will be used.

           access_stack_id: identifies the access stack. If assumed_role is not a project role, provide the access stack id to examine it for the assumed_role,
           otherwise defaults to the deployment stack. Defaults to None.

        Returns a cgf_service_client.Path object that can be used to make requests.

        """

        url = self.get_service_url(stack_id=stack_id)

        if assumed_role:
            if assumed_role.startswith('Project'):
                role_arn = self.get_stack_resource_arn(self.get_project_stack_arn(), assumed_role)
            else:
                if access_stack_id:
                    role_arn = self.get_stack_resource_arn(access_stack_id, assumed_role)
                else:
                    role_arn = self.get_stack_resource_arn(self.get_deployment_access_stack_arn(self.TEST_DEPLOYMENT_NAME), assumed_role)
        else:
            role_arn = None

        if use_aws_auth or role_arn:
            session = self.session
        else:
            session = None

        return cgf_service_client.for_url(url, session=session, role_arn=role_arn, verbose=True)

    def get_service_url(self, stack_id=None):
        """Gets the service API url for a stack's service api. If no stack is specified, the stack
        identified using TEST_DEPLOYMENT_NAME and TEST_RESOURCE_GROUP_NAME will be used.
        """

        if not stack_id:
            stack_id = self.get_resource_group_stack_arn(self.TEST_DEPLOYMENT_NAME, self.TEST_RESOURCE_GROUP_NAME)

        url = self.get_stack_output(stack_id, 'ServiceUrl')
        self.assertIsNotNone(url, "Missing ServiceUrl stack output.")

        return url


PROJECT_JSON_CONTENT = '''{
    "project_name": "TestGame",
    "product_name": "Empty Template",
    "executable_name": "TestGameLauncher",
    "modules" : [],
    "project_id": "{8C48136E-A807-42E1-828E-FC15EB6A4D4D}",

    "android_settings" : {
        "package_name" : "com.lumberyard.empty",
        "version_number" : 1,
        "version_name" : "1.0.0.0",
        "orientation" : "landscape"
    }
}
'''

EDITOR_XML_CONTENT = '''<ObjectStream version="1">
	<Class name="ComponentApplication::Descriptor" type="{70277A3E-2AF5-4309-9BBF-6161AFBDE792}" version="2" specializationTypeId="{70277A3E-2AF5-4309-9BBF-6161AFBDE792}">
		<Class name="AZStd::vector" field="modules" type="{2BADE35A-6F1B-4698-B2BC-3373D010020C}" specializationTypeId="{8E779F80-AEAA-565B-ABB1-DE10B18CF995}">
			<Class name="DynamicModuleDescriptor" field="element" type="{D2932FA3-9942-4FD2-A703-2E750F57C003}" specializationTypeId="{D2932FA3-9942-4FD2-A703-2E750F57C003}">
				<Class name="AZStd::string" field="dynamicLibraryPath" type="{EF8FF807-DDEE-4EB0-B678-4CA3A2C490A4}" value="Gem.FooTest.c039306c475d4ed6b0cefc67b23eb9c7.v0.1.0" specializationTypeId="{189CC2ED-FDDE-5680-91D4-9F630A79187F}"/>
			</Class>
		</Class>
	</Class>
</ObjectStream>
'''

GAME_XML_CONTENT = '''<ObjectStream version="1">
	<Class name="ComponentApplication::Descriptor" type="{70277A3E-2AF5-4309-9BBF-6161AFBDE792}" version="2" specializationTypeId="{70277A3E-2AF5-4309-9BBF-6161AFBDE792}">
		<Class name="AZStd::vector" field="modules" type="{2BADE35A-6F1B-4698-B2BC-3373D010020C}" specializationTypeId="{8E779F80-AEAA-565B-ABB1-DE10B18CF995}">
			<Class name="DynamicModuleDescriptor" field="element" type="{D2932FA3-9942-4FD2-A703-2E750F57C003}" specializationTypeId="{D2932FA3-9942-4FD2-A703-2E750F57C003}">
				<Class name="AZStd::string" field="dynamicLibraryPath" type="{EF8FF807-DDEE-4EB0-B678-4CA3A2C490A4}" value="Gem.FooTest.c039306c475d4ed6b0cefc67b23eb9c7.v0.1.0" specializationTypeId="{189CC2ED-FDDE-5680-91D4-9F630A79187F}"/>
			</Class>
		</Class>
	</Class>
</ObjectStream>
'''
