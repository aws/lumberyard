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
from unittest.mock import MagicMock
from cgf_utils.aws_iam_role import IAMRole


class UnitTest_IAMRole(unittest.TestCase):
    TEST_ROLE_NAME = 'TEST_ROLE_NAME'
    TEST_ACCOUNT = '1234456789'
    TEST_ROLE_ARN = 'arn:aws:iam::{}:role/my-role/{}'.format(TEST_ACCOUNT, TEST_ROLE_NAME)

    def test_role_construction(self):
        # GIVEN

        # WHEN
        role = IAMRole(role_name=self.TEST_ROLE_NAME, role_arn=self.TEST_ROLE_ARN, assume_role_policy=None)

        # THEN
        self.assertEqual(self.TEST_ACCOUNT, role.account)
        self.assertEqual(self.TEST_ROLE_ARN, role.arn)
        self.assertEqual(self.TEST_ROLE_NAME, role.role_name)
        self.assertEqual({}, role.assume_role_policy)

    def test_role_factory(self):
        expected_policy = {'Test': False}
        mock_iam_client = MagicMock()
        mock_iam_client.get_role.return_value = {
            'Role': {
                'Arn': self.TEST_ROLE_ARN,
                'AssumeRolePolicyDocument': expected_policy
            }
        }

        role = IAMRole.factory(role_name=self.TEST_ROLE_NAME, iam_client=mock_iam_client)

        mock_iam_client.get_role.assert_called_once_with(RoleName=self.TEST_ROLE_NAME)

        self.assertEqual(self.TEST_ACCOUNT, role.account)
        self.assertEqual(self.TEST_ROLE_ARN, role.arn)

        self.assertEqual(self.TEST_ROLE_NAME, role.role_name)
        self.assertEqual(expected_policy, role.assume_role_policy)

    def test_update_assume_role_policy(self):
        expected_policy = {'Test': False}
        mock_iam_client = MagicMock()

        role = IAMRole(role_name=self.TEST_ROLE_NAME, role_arn=self.TEST_ROLE_ARN, assume_role_policy=None)

        role.update_trust_policy(new_policy=expected_policy, iam_client=mock_iam_client)

        mock_iam_client.update_assume_role_policy.assert_called_once_with(RoleName=self.TEST_ROLE_NAME,
                                                                          PolicyDocument=expected_policy)
        self.assertEqual(expected_policy, role.assume_role_policy)

    def test_role_name_from_arn_without_path(self):
        role_name = IAMRole.role_name_from_arn_without_path(role_arn=self.TEST_ROLE_ARN)
        self.assertEqual(self.TEST_ROLE_NAME, role_name)
