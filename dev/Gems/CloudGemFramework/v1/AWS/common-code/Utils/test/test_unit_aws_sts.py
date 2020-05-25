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
from unittest.mock import MagicMock
from cgf_utils.aws_sts import AWSSTSUtils


class UnitTest_AWSSTSUtils(unittest.TestCase):
    TEST_REGION = "test-region"
    TOKEN_FROM_REGIONAL = "010230dfs0fsdf"  # random string
    TOKEN_FROM_GLOBAL = "F" + TOKEN_FROM_REGIONAL

    MOCK_SESSION = MagicMock()
    MOCK_SESSION.client.return_value = MagicMock()

    def test_endpoint_construction(self):
        aws_sts = AWSSTSUtils(self.TEST_REGION)
        self.assertTrue(self.TEST_REGION in aws_sts.endpoint_url)

    def test_client_construction_with_session(self):
        aws_sts = AWSSTSUtils(self.TEST_REGION)
        client = aws_sts.client(self.MOCK_SESSION)

        self.assertIsNotNone(client)
        self.MOCK_SESSION.client.assert_called_once_with('sts', endpoint_url=aws_sts.endpoint_url)

    @mock.patch("boto3.client")
    def test_client_construction(self, mock_boto_sts_client):
        aws_sts = AWSSTSUtils(self.TEST_REGION)
        client = aws_sts.client()

        self.assertIsNotNone(client)
        mock_boto_sts_client.assert_called_once_with('sts', endpoint_url=aws_sts.endpoint_url, region_name=self.TEST_REGION)

    @mock.patch("boto3.Session")
    def test_client_construction_with_credentials(self, mock_get_session):
        mock_session = mock.Mock()
        mock_session.client.return_value = MagicMock()
        mock_get_session.return_value = mock_session

        aws_sts = AWSSTSUtils(self.TEST_REGION)
        client = aws_sts.client_with_credentials(aws_access_key_id="ACCESS_KEY_ID",
                                                 aws_secret_access_key="SECRET_ACCESS_KEY",
                                                 aws_session_token="SESSION_TOKEN")

        self.assertIsNotNone(client)
        mock_get_session.assert_called_once_with(aws_access_key_id="ACCESS_KEY_ID",
                                                 aws_secret_access_key="SECRET_ACCESS_KEY",
                                                 aws_session_token="SESSION_TOKEN",
                                                 region_name=self.TEST_REGION)
        mock_session.client.assert_called_once_with('sts', endpoint_url=aws_sts.endpoint_url)

    def test_session_token_validation(self):
        # No exception when calling
        AWSSTSUtils.validate_session_token(self.TOKEN_FROM_REGIONAL)

        # Expect exception when calling
        with self.assertRaises(RuntimeError):
            AWSSTSUtils.validate_session_token(self.TOKEN_FROM_GLOBAL)
