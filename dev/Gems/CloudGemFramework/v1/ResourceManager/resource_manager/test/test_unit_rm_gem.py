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
import uuid
from unittest import mock

import unit_common_utils
from resource_manager.gem import Gem
from resource_manager.gem import GemContext


def _set_up_context(gem_json: dict = None):
    """
    Set up the Context object that drives everything from a CloudCanvas POV

    :param gem_json: A mock gem description to load
    :return: A context object that can be configured in the test
    """
    context = mock.MagicMock()
    context.config = mock.MagicMock()
    context.stack = mock.MagicMock()
    context.config = mock.MagicMock()
    context.config.game_directory_path = 'TestGamePath'
    context.config.root_directory_path = "TestRootPath"
    context.config.load_json.return_value = gem_json if gem_json is not None else None
    return context


class UnitTest_GemContext(unittest.TestCase):

    # A test time stamp as returned by os.path.getmtime (last modification time)
    TEST_FILE_STAMP_TIME = 1558447897.0442736

    def test_bootstrap(self):
        # GIVEN
        context = _set_up_context()
        args = mock.MagicMock()
        args.only_cloud_gems.return_value = True

        # WHEN
        context = GemContext(context=context)
        context.bootstrap(args=args)

        # THEN
        self.assertIsNotNone(context)

    @mock.patch('subprocess.Popen', unit_common_utils.MockedPopen)
    def test_gem_disable_gem(self):
        # GIVEN
        context = _set_up_context()
        args = mock.MagicMock()
        args.only_cloud_gems.return_value = True
        context = GemContext(context=context)

        with mock.patch('os.path.getmtime') as getmtime_patch:
            getmtime_patch.return_value = self.TEST_FILE_STAMP_TIME
            with mock.patch('os.path.isfile') as isfile_patch:
                isfile_patch.return_value = True
                disabled = context.disable_gem(gem_name='TEST_GEM')

        # THEN
        self.assertTrue(disabled)

    @mock.patch('subprocess.Popen', unit_common_utils.MockedPopen)
    def test_gem_disable_gem_twice(self):
        # GIVEN
        context = _set_up_context()
        args = mock.MagicMock()
        args.only_cloud_gems.return_value = True
        context = GemContext(context=context)

        # WHEN
        with mock.patch('os.path.getmtime') as getmtime_patch:
            getmtime_patch.return_value = self.TEST_FILE_STAMP_TIME
            with mock.patch('os.path.isfile') as isfile_patch:
                isfile_patch.return_value = True
                disabled = context.disable_gem(gem_name='TEST_GEM')
                second_disabled = context.disable_gem(gem_name='TEST_GEM')

        # THEN
        self.assertTrue(disabled)
        self.assertTrue(second_disabled)


class UnitTest_Gem(unittest.TestCase):

    def _test_create(self, is_enabled):
        # GIVEN
        gem_json = self.__fake_gem_json('TESTGEM')
        context = _set_up_context(gem_json=gem_json)
        expected_root_path = 'ROOT_DIRECTORY_PATH'

        # WHEN
        gem = Gem(context=context, root_directory_path=expected_root_path, is_enabled=is_enabled)

        # THEN
        self.assertIsNotNone(gem)
        self.assertEqual(gem.is_enabled, is_enabled)
        self.assertEqual(gem.root_directory_path, expected_root_path)

        def side_effect(filename):
            return True

        with mock.patch('os.path.isfile') as exists_patch:
            exists_patch.side_effect = side_effect
            self.assertIsNotNone(gem.file_object)

            self.assertEqual(gem.name, 'TESTGEM')
            self.assertTrue(gem.is_defined)

    def test_create_enabled(self):
        self._test_create(is_enabled=True)

    def test_create_disabled(self):
        self._test_create(is_enabled=False)

    @staticmethod
    def __fake_gem_json(gem_name: str, gem_version: str = "1.0.0"):
        """Generate a gem.json file for testing"""
        return {
            "GemFormatVersion": 4,
            "Uuid": uuid.uuid1(),
            "Name": gem_name,
            "DisplayName": gem_name,
            "Version": gem_version,
            "Summary": f"A short description of the {gem_name} gem.",
            "Tags": ["Untagged"],
            "IconPath": "preview.png",
            "Modules": [{"Type": "GameModule"}],
            "Dependencies": [
                {
                    "Uuid": uuid.uuid1(),
                    "VersionConstraints": ["~>1.1.4"],
                    "_comment": "CloudGemFramework"
                }
            ]
        }
