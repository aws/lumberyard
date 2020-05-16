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
import os
import platform

from unittest import mock
from unittest.mock import MagicMock

import unittest
from resource_manager.hook import HookContext
from resource_manager.hook import HookModule

TEST_MODULE_ROOT = 'C:' if platform.system() == 'Windows' else '/User'

TEST_MODULE_PATH = os.path.join(TEST_MODULE_ROOT, "ModulePath")
TEST_MODULE_PATH2 = os.path.join(TEST_MODULE_ROOT, "AnotherModulePath")
TEST_MODULE_PATH3 = os.path.join(TEST_MODULE_ROOT, "YetAnotherModulePath")

TEST_MODULE_NAME = "TestModule"
TEST_DEFAULT_HOOK_NAME = "PROJECT"  # Hooks defaults to this if gem or resource group are not provided


class UnitTest_HookContext(unittest.TestCase):

    def test_creation_succeeds(self):
        # Given
        context = MagicMock()

        # When
        hook_context = HookContext(context=context)

        # Then
        self.assertEqual(context, hook_context.context)

    def test_load_modules_with_empty_data_has_record(self):
        # Given
        context = MagicMock()
        context.config = MagicMock()
        context.config.aws_directory_path = ""  # Invalid path, so skip loading

        # When
        hook_context = HookContext(context=context)
        hook_context.load_modules(TEST_MODULE_NAME)

        # Then
        # Should create module -> {} mapping
        self.assertEqual(1, len(hook_context))
        self.assertEqual(0, hook_context.count(TEST_MODULE_NAME))

    def test_load_modules_with_project_module_loads_expected_records(self):
        # Given
        context = MagicMock()
        context.config = MagicMock()
        context.config.aws_directory_path = TEST_MODULE_PATH

        # When
        hook_context = HookContext(context=context)

        # Control if file path is valid
        def file_side_effect(filename):
            return True

        with mock.patch("sys.modules", new_callable=mock.PropertyMock) as mock_hooked_module:
            with mock.patch('os.path.isfile') as mock_isfile:
                mock_isfile.side_effect = file_side_effect
                hook_context.load_modules(TEST_MODULE_NAME)

        # Then
        # Module should load self (+1)
        self.assertEqual(1, len(hook_context))
        self.assertEqual(1, hook_context.count(TEST_MODULE_NAME))

    def test_load_modules_with_duplicate_modules_expected_records(self):
        # GIVEN
        context = MagicMock()
        context.config = MagicMock()
        context.config.aws_directory_path = TEST_MODULE_PATH

        mock_resource_group1 = self.__make_mock_resource_group('MODULE1', TEST_MODULE_PATH)
        mock_resource_group2 = self.__make_mock_resource_group('MODULE2', TEST_MODULE_PATH2)

        context.resource_groups = {
            'MODULE1': mock_resource_group1,
            'MODULE2': mock_resource_group2
        }

        # Make one gem's path duplicate a resource_group
        mock_gem1 = self.__make_mock_gem('GEM1', TEST_MODULE_PATH)
        mock_gem2 = self.__make_mock_gem('GEM2', TEST_MODULE_PATH3)

        context.gems = MagicMock()
        context.gem.enabled_gems = [
            mock_gem1, mock_gem2
        ]

        # Control if file path is valid
        def file_side_effect(filename):
            return True

        # WHEN
        hook_context = HookContext(context=context)

        with mock.patch("sys.modules", new_callable=mock.PropertyMock):
            with mock.patch('os.path.isfile') as mock_isfile:
                mock_isfile.side_effect = file_side_effect
                hook_context.load_modules(TEST_MODULE_NAME)

        # THEN
        # Should have single mapping
        self.assertEqual(1, len(hook_context))
        # Module should load self (+1), plus two resource groups (+2) and one gem (+1) as the other is a duplicate of a resource_group entry
        self.assertEqual(4, hook_context.count(TEST_MODULE_NAME))

    @staticmethod
    def __make_mock_resource_group(name, path):
        mock_resource_group = MagicMock()
        mock_resource_group.name = name
        mock_resource_group.directory_path = path
        return mock_resource_group

    @staticmethod
    def __make_mock_gem(name, path):
        mock_gem = MagicMock()
        mock_gem.name = name
        mock_gem.aws_directory_path = path
        return mock_gem


class UnitTest_HookModule(unittest.TestCase):

    def test_simple_creation_succeeds(self):
        # Given
        context = MagicMock()
        module_location = os.path.join(TEST_MODULE_PATH, TEST_MODULE_NAME)

        # When
        with mock.patch("sys.modules", new_callable=mock.PropertyMock):
            hook_module = HookModule(context, module_location, resource_group=None, gem=None)

        # Then
        self.assertEqual(context, hook_module.context)
        self.assertEqual(TEST_MODULE_PATH, hook_module.hook_path)
        self.assertEqual(TEST_DEFAULT_HOOK_NAME, hook_module.hook_name)
