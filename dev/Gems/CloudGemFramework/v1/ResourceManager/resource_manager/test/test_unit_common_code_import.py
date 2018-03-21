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
# $Revision: #16 $

import platform
import os
import subprocess

from time import sleep

import lmbr_aws_test_support

class UnitTest_CloudGemFramework_ResourceManager_CommonCodeImport(lmbr_aws_test_support.lmbr_aws_TestCase):

    TEST_GEM_NAME_1 = 'TestGem1'
    TEST_GEM_NAME_2 = 'TestGem2'
    TEST_PLUGIN_NAME = 'TestPlugin'
    TEST_EMPTY_PLUGIN_NAME = 'TestEmptyPlugin'

    CLI_PLUGIN_CODE = '''
import resource_manager.cli
import TestPlugin
import TestEmptyPlugin

def add_cli_commands(hook, subparsers, add_common_args, **kwargs):
    subparser = subparsers.add_parser("unit-test-common-code-import")
    subparser.register('action', 'parsers', resource_manager.cli.AliasedSubParsersAction)
    cgf_subparsers = subparser.add_subparsers(dest = 'subparser_name')

    subparser = cgf_subparsers.add_parser('run-test')
    add_common_args(subparser)
    subparser.set_defaults(func=__run_test)

def __run_test(context, args):

    # NOTE: These asserts should be similar to the ones in test_integration_service_api.py
    # Importing should work the same locally in a workspace and in a Lambda.

    __assert(TestPlugin.__all__ == ['TestGem1', 'TestGem2'], 'Both test modules should be listed in __all__ in sorted order')

    __assert(TestPlugin.TestGem1.get_name() == 'TestGem1', 'First gem should be callable as a submodule')
    __assert(TestPlugin.TestGem2.get_name() == 'TestGem2', 'Second gem should be callable as a submodule')

    __assert(TestPlugin.imported_modules, 'imported_modules should be defined')
    __assert(set(TestPlugin.imported_modules.keys()) == {'TestGem1', 'TestGem2'}, 'Both test modules should be in imported_modules')
    __assert(TestPlugin.imported_modules['TestGem1'].get_name() == 'TestGem1', 'First gem should be callable from imported_modules')
    __assert(TestPlugin.imported_modules['TestGem2'].get_name() == 'TestGem2', 'Second gem should be callable from imported_modules')

    __assert(TestEmptyPlugin.__all__ == [], 'The empty plugin should have no imported modules in __all__')
    __assert(len(TestEmptyPlugin.imported_modules.keys()) == 0, 'The empty plugin imported_modules should be empty')

def __assert(result, msg):
    if not result:
        raise AssertionError(msg)
'''

    def setUp(self):        
        self.prepare_test_envionment("test_unit_common_code_import")
        
    def test_unit_lmbr_aws_end_to_end(self):
        self.run_all_tests()

    def __100_setup_test_gems(self):
        self.__create_test_gem(self.TEST_GEM_NAME_1)
        self.__create_test_gem(self.TEST_GEM_NAME_2)

        self.make_gem_aws_file(self.CLI_PLUGIN_CODE, self.TEST_GEM_NAME_1, 'resource-manager-code', 'command.py')

        self.make_gem_aws_file(
            '*.{}\n*.{}'.format(self.TEST_PLUGIN_NAME, self.TEST_EMPTY_PLUGIN_NAME),
            self.TEST_GEM_NAME_1, 'resource-manager-code', '.import'
        )

    def __200_test_multi_import(self):
        self.lmbr_aws('unit-test-common-code-import', 'run-test')

    def __create_test_gem(self, gem_name):
        self.lmbr_aws(
            'cloud-gem', 'create',
            '--gem', gem_name,
            '--initial-content', 'no-resources',
            '--enable'
        )

        self.make_gem_aws_file(
            'def get_name():\n    return("{}")'.format(gem_name),
            gem_name, 'common-code', self.TEST_PLUGIN_NAME,
            '{}__{}.py'.format(self.TEST_PLUGIN_NAME, gem_name)
        )
