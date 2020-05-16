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
from unittest import mock
import unit_common_utils

# Class/file under test
from resource_manager.config import ConfigContext


class Constants:
    def __init__(self):
        pass

    TEST_REGION = "TEST_REGION"
    AWS_DIRECTORY = "AWS_DIRECTORY"
    GAME_DIRECTORY = "GAME_DIRECTORY"
    USER_DIRECTORY = "USER_DIRECTORY"


class UnitTest_ConfigContext(unittest.TestCase):
    """Unit tests for ConfigContext"""

    def test_config_context_creation(self):
        context = mock.MagicMock()
        # WHEN
        config = ConfigContext(context)

        # THEN
        self.assertIsNotNone(config)
        self.assertEqual(context, config.context)

    def test_config_context_creation_bootstrap(self):
        # GIVEN
        context = mock.MagicMock()
        config = ConfigContext(context)
        args = self.__make_base_test_args()

        # WHEN
        unit_common_utils.make_os_path_patcher([True])
        with mock.patch("resource_manager.config.LocalProjectSettings") as mock_local_project_settings_call:
            config.bootstrap(args)

        # THEN
        self.assertEqual(Constants.GAME_DIRECTORY, config.game_directory_path)
        self.assertEqual(Constants.TEST_REGION, config.region)

    def test_config_context_creation_admin_roles_from_metadata(self):
        # GIVEN
        admin_roles_expected_value = False
        context = mock.MagicMock()
        config = ConfigContext(context)

        args = self.__make_base_test_args()

        # WHEN
        unit_common_utils.make_os_path_patcher([False, True])   # Skip loading aws_directory content
        with mock.patch("resource_manager.config.LocalProjectSettings") as mock_local_project_settings_call:
            config.bootstrap(args)
            config.initialize(args)

            mock_local_project_settings = mock_local_project_settings_call.return_value
            mock_local_project_settings.get.return_value = {'AdminRoles': admin_roles_expected_value}

            admin_roles_actual_value = config.create_admin_roles

        # THEN
        self.assertIsNotNone(config)
        self.assertEqual(Constants.TEST_REGION, config.region)
        self.assertEqual(context, config.context)
        self.assertEqual(admin_roles_expected_value, admin_roles_actual_value)

    def test_config_context_creation_admin_roles_from_legacy(self):
        # GIVEN
        S3_BUCKET = 'Bucket'
        admin_roles_expected_value = True

        context = mock.MagicMock()
        context.stack = mock.MagicMock()
        context.stack.describe_resources.return_value = {
            'Configuration': {'PhysicalResourceId': S3_BUCKET},          # Pretend we have a configuration bucket
            'ProjectOwner': {'PhysicalResourceId': 'ProjectOwner'}      # Pretend we have some resources
        }

        config = ConfigContext(context)

        args = self.__make_base_test_args()

        def project_settings_side_effect(key, default_value=None):
            """Controls tests values when using mocked local project settings"""
            if key == 'ProjectStackId':
                return 'arn:aws:{}:{}'.format(Constants.TEST_REGION, key)   # Fake out an ARN like thing
            else:
                return {}  # no existing mapping found

        # WHEN
        with mock.patch("resource_manager.config.CloudProjectSettings") as mock_cloud_project_settings_call:
            with mock.patch("resource_manager.config.LocalProjectSettings") as mock_local_project_settings_call:
                config.bootstrap(args)
                config.initialize(args)

                mock_local_project_settings = mock_local_project_settings_call.return_value
                mock_local_project_settings.get.side_effect = project_settings_side_effect

                admin_roles_actual_value = config.create_admin_roles

        # THEN
        self.assertIsNotNone(config)
        self.assertEqual(Constants.TEST_REGION, config.region)
        self.assertEqual(context, config.context)
        self.assertEqual(admin_roles_expected_value, admin_roles_actual_value)

        mock_cloud_project_settings_call.factory.assert_called_with(context, S3_BUCKET, verbose=args.verbose)
        mock_local_project_settings_call.assert_called_once_with(context, unit_common_utils.AnyArg(), Constants.TEST_REGION)

    @staticmethod
    def __make_base_test_args():
        args = mock.MagicMock()
        args.verbose = False
        args.game_directory = Constants.GAME_DIRECTORY
        args.aws_directory = Constants.AWS_DIRECTORY
        args.user_directory = Constants.USER_DIRECTORY
        args.region_override = None
        args.region = Constants.TEST_REGION
        args.root_directory = None  # Use CWD
        return args

