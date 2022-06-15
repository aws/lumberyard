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

from resource_manager import profile
from resource_manager.aws import AwsCredentials

TEST_REGION = "TEST_REGION"
TEST_PROFILE_NAME = "test-profile"
TEST_SECRET_KEY = 'test-secret-key'
TEST_ACCESS_KEY = 'test-access-key'
TEST_ACCOUNT_ID = '01234567890'
TEST_ROLE_NAME = 'my-role-session-name'


class UnitTest_Profile(unittest.TestCase):
    TEST_CREDS_WITH_ROLE = '\n[Test1]'
    '\naws_secret_access_key = dss' \
    '\naws_access_key_id = dsds' \
    '\ntoolkit_artifact_guid = 57729693-a310-4597-a695-016fba7f912e' \
    '\n' \
    '\n[Test2]' \
    '\ncredential_process = ls' \
    '\noutput = json' \
    f'\nregion = {TEST_REGION}' \
    '\ntoolkit_artifact_guid = d88d7bcf-1f8e-46fb-b465-8c75e545090e'


@mock.patch("cgf_utils.aws_sts.AWSSTSUtils.client_with_credentials")
def test_load_credentials_with_named_profile_succeeds(self, mock_client_with_credentials):
    # GIVEN
    mock_sts_client = mock.MagicMock()
    mock_client_with_credentials.return_value = mock_sts_client
    mock_credentials = mock.MagicMock()
    mock_credentials.sections.return_value = [TEST_PROFILE_NAME]
    # Allow for two reads of each credential part
    mock_credentials.get.side_effect = [TEST_ACCESS_KEY, TEST_SECRET_KEY, TEST_ACCESS_KEY, TEST_SECRET_KEY]

    mock_context = mock.MagicMock()
    mock_context.aws.load_credentials.return_value = mock_credentials
    mock_context.aws.get_credentials_file_path = mock.MagicMock(return_value='test-path')
    # Select the test profile via passed args
    mock_args = mock.MagicMock()
    type(mock_args).profile = mock.PropertyMock(return_value=TEST_PROFILE_NAME)

    mock_sts_results = self.mock_sts_caller_identity_results()
    mock_sts_client.get_caller_identity.return_value = mock_sts_results

    # WHEN
    profile.list(mock_context, mock_args)

    # THEN
    # Validate STS calls
    mock_client_with_credentials.assert_called_once_with(aws_access_key_id=TEST_ACCESS_KEY,
                                                         aws_secret_access_key=TEST_SECRET_KEY)
    mock_sts_client.get_caller_identity.assert_called_once_with()

    # Validate results
    mock_context.view.profile_list.assert_called_once_with(self.expected_view_results(),
                                                           mock_context.aws.get_credentials_file_path())


@mock.patch("cgf_utils.aws_sts.AWSSTSUtils.client_with_credentials")
def test_load_credentials_with_named_profile_and_role_succeeds(self, mock_client_with_credentials):
    # GIVEN
    with mock.patch('builtins.open', mock.mock_open(read_data=UnitTest_Profile.TEST_CREDS_WITH_ROLE)):
        # GIVEN
        credentials = AwsCredentials()

        # WHEN
        credentials.read(path='test-path')

    mock_sts_client = mock.MagicMock()
    mock_client_with_credentials.return_value = mock_sts_client

    mock_context = mock.MagicMock()
    mock_context.aws.load_credentials.return_value = credentials
    mock_context.aws.get_credentials_file_path = mock.MagicMock(return_value='test-path')
    # Select the test profile via passed args
    mock_args = mock.MagicMock()
    type(mock_args).profile = mock.PropertyMock(return_value=TEST_PROFILE_NAME)

    mock_sts_results = self.mock_sts_caller_identity_results()
    mock_sts_client.get_caller_identity.return_value = mock_sts_results

    # WHEN
    profile.list(mock_context, mock_args)

    # THEN
    # Validate STS calls
    mock_client_with_credentials.assert_called_once_with(aws_access_key_id=TEST_ACCESS_KEY,
                                                         aws_secret_access_key=TEST_SECRET_KEY)
    mock_sts_client.get_caller_identity.assert_called_once_with()

    # Validate results
    mock_context.view.profile_list.assert_called_once_with(self.expected_view_results(),
                                                           mock_context.aws.get_credentials_file_path())


@staticmethod
def mock_sts_caller_identity_results():
    # See
    return {
        'UserId': 'Test-user/test',
        'Account': TEST_ACCOUNT_ID,
        'Arn': 'arn:aws:sts::0123456789:assumed-role/my-role-name/' + TEST_ROLE_NAME
    }


@staticmethod
def expected_view_results():
    return [
        {'UserName': TEST_ROLE_NAME,
         'Account': TEST_ACCOUNT_ID,
         'Name': TEST_PROFILE_NAME,
         'Default': False,
         'AccessKey': TEST_ACCESS_KEY,
         'SecretKey': TEST_SECRET_KEY}
    ]
