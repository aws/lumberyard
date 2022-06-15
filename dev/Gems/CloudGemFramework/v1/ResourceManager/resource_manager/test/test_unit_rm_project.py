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

from resource_manager import project
from resource_manager.errors import HandledError
from resource_manager import util
from cgf_utils.version_utils import Version

import unit_common_utils


class UnitTest_Project(unittest.TestCase):

    @mock.patch("botocore.session.Session")
    @mock.patch("boto3.session.Session")
    def test_project_create_stack_in_invalid_region_fails(self, mock_session_call, mock_botocore_session_call):
        # GIVEN
        context = self.__set_up_context()
        args = mock.MagicMock()
        args.region = 'INVALID_REGION'
        mock_botocore_session = mock_botocore_session_call.return_value
        mock_botocore_session.get_available_regions.return_value = ['VALID_REGION']

        # WHEN - throws exception
        with self.assertRaises(HandledError):
            project.create_stack(context, args)

        # THEN
        # Check project creation didn't proceed
        self.assertEqual(0, context.config.initialize_aws_directory.call_count)

    @mock.patch("botocore.session.Session")
    @mock.patch("boto3.session.Session")
    def test_project_create_stack_files_only(self, mock_session_call, mock_botocore_session_call):
        # GIVEN
        context = self.__set_up_context()
        args = mock.MagicMock()
        args.region = 'us-east-1'
        args.files_only = True

        mock_botocore_session = mock_botocore_session_call.return_value
        mock_botocore_session.get_available_regions.return_value = [args.region]

        # WHEN
        project.create_stack(context, args)

        # THEN
        # Check we set-up initial aws project
        self.assertEqual(1, context.config.initialize_aws_directory.call_count)
        self.assertEqual(0, context.config.local_project_settings.project_stack_exists.call_count)

    @mock.patch("botocore.session.Session")
    @mock.patch("boto3.session.Session")
    def test_project_create_stack_project_already_created(self, mock_session_call, mock_botocore_session_call):
        # GIVEN
        context = self.__set_up_context()
        context.config.local_project_settings.project_stack_exists.return_value = True

        args = mock.MagicMock()
        args.region = 'us-east-1'
        args.files_only = False
        mock_botocore_session = mock_botocore_session_call.return_value
        mock_botocore_session.get_available_regions.return_value = [args.region]

        # WHEN
        # Throws exception because project stack exists
        with self.assertRaises(HandledError):
            project.create_stack(context, args)

        # THEN
        # Check we set-up initial aws project
        self.assertEqual(1, context.config.local_project_settings.project_stack_exists.call_count)

    @mock.patch("botocore.session.Session")
    @mock.patch("boto3.session.Session")
    def test_project_create_stack_with_existing_name(self, mock_session_call, mock_botocore_session_call):
        # GIVEN
        context = self.__set_up_context()
        context.config.local_project_settings.project_stack_exists.return_value = False
        context.config.framework_aws_directory_path = "TEST_AWS_DIRECTORY_PATH"
        context.config.get_pending_project_stack_id.return_value = None  # Creating a new stack
        context.stack.name_exists.return_value = True

        args = mock.MagicMock()
        args.region = 'us-east-1'
        args.files_only = False
        args.stack_name = "TESTSTACKNAME"
        args.create_admin_roles = True
        mock_botocore_session = mock_botocore_session_call.return_value
        mock_botocore_session.get_available_regions.return_value = [args.region]
        
        # WHEN
        with self.assertRaises(HandledError):
            project.create_stack(context, args)

        # THEN
        self.assertEqual(1, context.stack.name_exists.call_count)
        context.stack.name_exists.assert_called_with(args.stack_name, args.region)

    def __ensure_cgp_created(self, version: str, cgp_created: bool, mock_session_call: mock.MagicMock):
        context = self.__set_up_context(framework_version=version)
        args = mock.MagicMock()
        args.region = 'us-east-1'
        args.files_only = True

        mock_botocore_session = mock_session_call.return_value
        mock_botocore_session.get_available_regions.return_value = [args.region]

        # WHEN
        project.create_stack(context, args)

        # THEN
        self.assertEqual(1, context.config.initialize_aws_directory.call_count)
        self.assertEqual(0, context.config.local_project_settings.project_stack_exists.call_count)
        context.stack.set_deploy_cgp.set_deploy_cloud_gem_portal(cgp_created)

    @mock.patch("botocore.session.Session")
    @mock.patch("boto3.session.Session")
    def test_project_create_stack_ensure_no_cgp(self, mock_session_call, mock_botocore_session_call):
        self.__ensure_cgp_created(version='1.1.5', cgp_created=False, mock_session_call=mock_botocore_session_call)

    @mock.patch("botocore.session.Session")
    @mock.patch("boto3.session.Session")
    def test_project_create_legacy_stack_ensure_cgp(self, mock_session_call, mock_botocore_session_call):
        self.__ensure_cgp_created(version='1.1.4', cgp_created=True, mock_session_call=mock_botocore_session_call)

    def __project_create_sets_admin_roles_and_custom_domain_name_creates_stack(self, create_admin_roles, custom_domain_name, mock_session_call,
                                                                               mock_botocore_session_call):
        # GIVEN
        project_url = "TEST_URL"
        mock_botocore_session = mock_botocore_session_call.return_value
        mock_botocore_session.get_available_regions.return_value = ['VALID_REGION']

        context = self.__set_up_context()
        context.config.local_project_settings.project_stack_exists.return_value = False
        context.config.framework_aws_directory_path = "TEST_AWS_DIRECTORY_PATH"
        context.config.get_pending_project_stack_id.return_value = None  # Creating a new stack
        context.stack.name_exists.return_value = False  # New stack in region
        context.config.aws_directory_path = "TEST_AWS_DIRECTORY_PATH"

        # Small Hack to avoid having to simulate loading/saving of of local_project_settings
        type(context.config).create_admin_roles = mock.PropertyMock(return_value=True if create_admin_roles else False)
        type(context.config).custom_domain_name = mock.PropertyMock(return_value=custom_domain_name if custom_domain_name else '')

        args = mock.MagicMock()
        args.region = 'VALID_REGION'
        args.files_only = False
        args.stack_name = "TESTSTACKNAME"
        args.create_admin_roles = create_admin_roles
        args.record_cognito_pools = False
        args.custom_domain_name = custom_domain_name

        # WHEN
        unit_common_utils.make_os_path_patcher([True])
        with mock.patch("resource_manager.project.ProjectUploader") as mock_uploader_call:
            mock_uploader = mock_uploader_call.return_value
            mock_uploader.upload_content.return_value = project_url
            project.create_stack(context, args)

        self.assertEqual(1, context.stack.name_exists.call_count)
        context.stack.name_exists.assert_called_with(args.stack_name, args.region)

        self.assertEqual(1, context.stack.create_using_template.call_count)
        context.stack.create_using_template.assert_called_with(
            args.stack_name,
            project.bootstrap_template,
            'VALID_REGION',
            created_callback=unit_common_utils.AnyArg(),
            capabilities=context.stack.confirm_stack_operation(),
            timeout_in_minutes=30,
            parameters={'CreateAdminRoles': 'true' if args.create_admin_roles else 'false', 
                        'CustomDomainName': args.custom_domain_name if args.custom_domain_name else ''},
            tags=[{'Value': args.stack_name, 'Key': 'cloudcanvas:project-name'}]
        )

        self.assertEqual(1, context.stack.update.call_count)
        context.stack.update.assert_called_with(
            context.config.project_stack_id,
            project_url,
            parameters={
                'ProjectName': util.get_stack_name_from_arn(context.config.project_stack_id),
                'ConfigurationKey': mock_uploader.key,
                'CreateAdminRoles': 'true' if args.create_admin_roles else 'false',
                'CustomDomainName': args.custom_domain_name if args.custom_domain_name else ''
            },
            pending_resource_status=unit_common_utils.AnyArg(),
            capabilities=context.stack.confirm_stack_operation(),
            tags=unit_common_utils.AnyArg()
        )

    @mock.patch("botocore.session.Session")
    @mock.patch("boto3.session.Session")
    def test_project_create_with_admin_roles_creates_stack(self, mock_session_call, mock_botocore_session_call):
        self.__project_create_sets_admin_roles_and_custom_domain_name_creates_stack(create_admin_roles=True, custom_domain_name=None, mock_session_call=mock_session_call,
                                                             mock_botocore_session_call=mock_botocore_session_call)

    @mock.patch("botocore.session.Session")
    @mock.patch("boto3.session.Session")
    def test_project_create_with_no_admin_roles_creates_stack(self, mock_session_call, mock_botocore_session_call):
        self.__project_create_sets_admin_roles_and_custom_domain_name_creates_stack(create_admin_roles=False, custom_domain_name=None, mock_session_call=mock_session_call,
                                                             mock_botocore_session_call=mock_botocore_session_call)

    @mock.patch("botocore.session.Session")
    @mock.patch("boto3.session.Session")
    def test_project_create_with_default_admin_roles_creates_stack(self, mock_session_call, mock_botocore_session_call):
        self.__project_create_sets_admin_roles_and_custom_domain_name_creates_stack(create_admin_roles=None, custom_domain_name=None, mock_session_call=mock_session_call,
                                                             mock_botocore_session_call=mock_botocore_session_call)


    @mock.patch("botocore.session.Session")
    @mock.patch("boto3.session.Session")
    def test_project_create_with_custom_domain_name_creates_stack(self, mock_session_call, mock_botocore_session_call):
        self.__project_create_sets_admin_roles_and_custom_domain_name_creates_stack(create_admin_roles=None, custom_domain_name='TestCustomDomainName', mock_session_call=mock_session_call,
                                                             mock_botocore_session_call=mock_botocore_session_call)

    @mock.patch("botocore.session.Session")
    @mock.patch("boto3.session.Session")
    def test_project_create_with_empty_custom_domain_name_creates_stack(self, mock_session_call, mock_botocore_session_call):
        self.__project_create_sets_admin_roles_and_custom_domain_name_creates_stack(create_admin_roles=None, custom_domain_name='', mock_session_call=mock_session_call,
                                                             mock_botocore_session_call=mock_botocore_session_call)

    @mock.patch("botocore.session.Session")
    @mock.patch("boto3.session.Session")
    def test_project_create_with_default_custom_domain_name_creates_stack(self, mock_session_call, mock_botocore_session_call):
        self.__project_create_sets_admin_roles_and_custom_domain_name_creates_stack(create_admin_roles=None, custom_domain_name=None, mock_session_call=mock_session_call,
                                                             mock_botocore_session_call=mock_botocore_session_call)

    @staticmethod
    def __set_up_context(capabilities=None, framework_version: str = '1.1.5'):
        """
        Set up the Context object that drives everything from a CloudCanvas POV

        :param capabilities: A list of CloudFormation capabilities to mock
        :param framework_version: The framework version to set
        :return: A context object that can be configured in the test
        """
        if capabilities is None:
            capabilities = []

        context = mock.MagicMock()
        context.config = mock.MagicMock()
        context.stack = mock.MagicMock()
        context.stack.confirm_stack_operation.return_value = capabilities

        context.config.local_project_settings = mock.MagicMock()

        context.config.framework_version = Version(framework_version)
        context.config.project_template_aggregator = mock.MagicMock()
        context.config.project_template_aggregator.effective_template = {}
        return context


