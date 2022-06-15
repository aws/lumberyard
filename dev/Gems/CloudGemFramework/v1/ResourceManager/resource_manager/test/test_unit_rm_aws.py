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
import os
import random, string
import unittest
from unittest import mock
from resource_manager_common import constant

# Class/files under test
from resource_manager.aws import AWSContext
from resource_manager.aws import AwsCredentials
from resource_manager.aws import ClientWrapper
from resource_manager.errors import HandledError


class Constants:
    def __init__(self):
        pass

    TEST_REGION = "TEST_REGION"
    TEST_SERVICE = "TEST_SERVICE"
    TEST_PROFILE = "TEST_PROFILE"
    TEST_ACCESS_KEY = "AK1234567"
    TEST_SECRET_KEY = "AS1234567"
    TEST_PATH = "/my/awesome/file"


def _generate_random_alphanumeric(length=10) -> str:
    return ''.join(random.choices(string.ascii_letters + string.digits, k=length))


class UnitTest_AWSCredentials(unittest.TestCase):

    TEST_ACCESS_KEY = _generate_random_alphanumeric(10).upper()
    TEST_SECRET_KEY = _generate_random_alphanumeric(16)

    TEST_CREDS = '[Test1]'\
                 f'\naws_access_key_id = {TEST_ACCESS_KEY}' \
                 f'\naws_secret_access_key = {TEST_SECRET_KEY}' \
                 '\n' \
                 '\n[Test2]' \
                 f'\naws_secret_access_key = {_generate_random_alphanumeric()}' \
                 f'\naws_access_key_id = {TEST_ACCESS_KEY}' \
                 '\ntoolkit_artifact_guid = 418aec5b-049f-497a-a9ef-7fa70e67d01d'

    TEST_CREDS_WITH_ROLE = '[Test2]'\
                            '\ncredential_process = ls'\
                            '\noutput = json'\
                            f'\nregion = {Constants.TEST_REGION}'\
                            '\ntoolkit_artifact_guid = d88d7bcf-1f8e-46fb-b465-8c75e545090e'

    def test_creation(self):
        credentials = AwsCredentials()

        # THEN
        self.assertIsNotNone(credentials)

    def test_credentials_read(self):
        with mock.patch('builtins.open', mock.mock_open(read_data=UnitTest_AWSCredentials.TEST_CREDS)):
            # GIVEN
            credentials = AwsCredentials()

            # WHEN
            credentials.read(path=Constants.TEST_PATH)

            # THEN
            self.assertTrue('Test1' in credentials.sections())

            print(credentials.options('Test1'))
            key = credentials.get('Test1', constant.ACCESS_KEY_OPTION)
            secret = credentials.get('Test1', constant.SECRET_KEY_OPTION)
            self.assertEqual(UnitTest_AWSCredentials.TEST_SECRET_KEY, secret)
            self.assertEqual(UnitTest_AWSCredentials.TEST_ACCESS_KEY, key)

    def test_credentials_with_role_creds_read(self):
        with mock.patch('builtins.open', mock.mock_open(read_data=UnitTest_AWSCredentials.TEST_CREDS_WITH_ROLE)):
            # GIVEN
            credentials = AwsCredentials()

            # WHEN
            credentials.read(path=Constants.TEST_PATH)

            # THEN
            self.assertTrue('Test2' in credentials.sections())

            print(credentials.options('Test2'))
            key = credentials.get_with_default('Test2', constant.ACCESS_KEY_OPTION, optional=None)
            secret = credentials.get_with_default('Test2', constant.SECRET_KEY_OPTION, optional=None)
            self.assertIsNone(secret)
            self.assertIsNone(key)


class UnitTest_ClientWrapper(unittest.TestCase):

    def test_AWSContext_creation(self):
        mockClient = mock.MagicMock()
        wrapper = ClientWrapper(wrapped_client=mockClient, verbose=False)

        # THEN
        self.assertIsNotNone(wrapper)


class UnitTest_AWSContext(unittest.TestCase):
    """
    Unit tests for AWSContent class

    Note: use_role has to be true when calling client()
    """

    def test_AWSContext_creation(self):
        mock_context = mock.MagicMock()
        mock_context.config.project_region = Constants.TEST_REGION

        # WHEN
        context = AWSContext(context=mock_context)

        # THEN
        self.assertIsNotNone(context)
        self.assertEqual(Constants.TEST_REGION, context.region)

    def test_AWSContext_client_not_initialized(self):
        mock_context = mock.MagicMock()
        mock_context.region = Constants.TEST_REGION

        # WHEN
        context = AWSContext(context=mock_context)

        # Will fail because context has not been initialized
        with self.assertRaises(HandledError):
            client = context.client(Constants.TEST_SERVICE, region=Constants.TEST_REGION, use_role=False)

    @mock.patch("botocore.session.Session")
    def test_AWSContext_client_with_credentials(self, mock_botocore_session_call):
        mock_context = mock.MagicMock()
        mock_context.region = Constants.TEST_REGION
        mock_args = self.__generate_mock_args()
        mock_botocore_session = self.__generate_mock_botocore_session(mock_botocore_session_call)

        # WHEN
        context = AWSContext(context=mock_context)
        context.initialize(mock_args)
        client = context.client(Constants.TEST_SERVICE, region=Constants.TEST_REGION, use_role=True)

        # THEN
        self.assertIsNotNone(client)
        self.assertEqual(Constants.TEST_ACCESS_KEY, os.environ["AWS_ACCESS_KEY_ID"])
        self.assertEqual(Constants.TEST_SECRET_KEY, os.environ["AWS_SECRET_ACCESS_KEY"])

        # Note: credentials are not passed to this call,
        mock_botocore_session.create_client.assert_called_once_with(
            Constants.TEST_SERVICE,
            api_version=None,
            aws_access_key_id=None,
            aws_secret_access_key=None,
            aws_session_token=None,
            endpoint_url=None,
            region_name=Constants.TEST_REGION,
            config=mock.ANY,
            use_ssl=True,
            verify=None
        )
        # But expect credentials are grabbed from the botocore session and were set there
        mock_botocore_session.set_credentials.assert_called_once_with(
            Constants.TEST_ACCESS_KEY,
            Constants.TEST_SECRET_KEY,
            None
        )

    @mock.patch("botocore.session.Session")
    @mock.patch("boto3.session.Session")
    def test_AWSContext_client_with_profile(self, mock_session_call, mock_botocore_session_call):
        mock_context = mock.MagicMock()
        mock_context.region = Constants.TEST_REGION
        mock_args = self.__generate_mock_args(profile=True)
        mock_botocore_session = self.__generate_mock_botocore_session(mock_botocore_session_call)

        mock_credentials = mock.MagicMock()
        mock_frozen_credentials = mock.MagicMock()
        mock_credentials.get_frozen_credentials.return_value = mock_frozen_credentials
        mock_frozen_credentials.access_key = Constants.TEST_ACCESS_KEY
        mock_frozen_credentials.secret_key = Constants.TEST_SECRET_KEY
        mock_botocore_session.get_credentials.return_value = mock_credentials

        # WHEN
        context = AWSContext(context=mock_context)
        context.initialize(mock_args)
        client = context.client(Constants.TEST_SERVICE, region=Constants.TEST_REGION, use_role=True)

        # THEN
        self.assertIsNotNone(client)
        self.assertEqual(Constants.TEST_ACCESS_KEY, os.environ["AWS_ACCESS_KEY_ID"])
        self.assertEqual(Constants.TEST_SECRET_KEY, os.environ["AWS_SECRET_ACCESS_KEY"])

        # Note: credentials are not passed to this call,
        mock_botocore_session.create_client.assert_called_once_with(
            Constants.TEST_SERVICE,
            api_version=None,
            aws_access_key_id=None,
            aws_secret_access_key=None,
            aws_session_token=None,
            endpoint_url=None,
            region_name=Constants.TEST_REGION,
            config=mock.ANY,
            use_ssl=True,
            verify=None
        )
        # But expect credentials are grabbed from the botocore session
        mock_botocore_session.get_credentials.assert_called_once()
        mock_credentials.get_frozen_credentials.assert_called_once()

    def test_AWSContext_set_default_profile(self):
        mock_context = mock.MagicMock()
        mock_context.region = Constants.TEST_REGION
        mock_args = self.__generate_mock_args()

        # WHEN
        context = AWSContext(context=mock_context)
        context.initialize(mock_args)
        context.set_default_profile(Constants.TEST_PROFILE)

        # THEN
        self.assertEqual(Constants.TEST_PROFILE, context.get_default_profile())

    def __generate_mock_args(self, profile=False):
        mock_args = mock.MagicMock()
        if profile:
            mock_args.profile = Constants.TEST_PROFILE
            mock_args.aws_access_key = None
            mock_args.aws_secret_key = None
        else:
            mock_args.aws_access_key = Constants.TEST_ACCESS_KEY
            mock_args.aws_secret_key = Constants.TEST_SECRET_KEY
            mock_args.profile = None
        mock_args.verbose = True
        mock_args.session = None
        return mock_args

    def __generate_mock_botocore_session(self, mock_botocore_session_call):
        mock_botocore_session = mock_botocore_session_call.return_value
        mock_botocore_session._session.user_agent_extra = None
        mock_botocore_session.get_config_variable.return_value = Constants.TEST_REGION
        mock_botocore_session.get_available_regions.return_value = [Constants.TEST_REGION]
        return mock_botocore_session
