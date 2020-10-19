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
import unittest
from unittest import mock

# Class/file under test
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


class UnitTest_AWSCredentials(unittest.TestCase):

    def test_AwsCredentials_creation(self):
        credentials = AwsCredentials()

        # THEN
        self.assertIsNotNone(credentials)


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
