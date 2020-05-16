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

from resource_manager import security


class UnitTest_Security_IdentityPoolUtils(unittest.TestCase):
    TEST_POOL_ID = "region:test-pool-id-12345678"

    # Aud condition generation tests

    def test_generate_aud_condition_statement(self):
        expected_statement = {'cognito-identity.amazonaws.com:aud': [self.TEST_POOL_ID]}
        statement = security.IdentityPoolUtils.generate_cognito_identity_condition_statement(identity_pool_id=self.TEST_POOL_ID)
        self.assertEqual(expected_statement, statement)

    # Role extraction tests
    def test_get_identity_pool_roles(self):
        authenticated_role = 'Cognito_MyIdentityPoolAuth_Role'
        unauthenticated_role = 'Cognito_MyIdentityPoolUnauth_Role'
        expected_roles = [authenticated_role, unauthenticated_role]
        mock_cognito_client = mock.MagicMock()

        mock_cognito_client.get_identity_pool_roles.return_value = {
            "IdentityPoolId": self.TEST_POOL_ID,
            "Roles": {
                "authenticated": "arn:aws:iam::111111111111:role/" + authenticated_role,
                "unauthenticated": "arn:aws:iam::111111111111:role/" + unauthenticated_role
            }
        }

        roles = security.IdentityPoolUtils.get_identity_pool_roles(identity_pool_id=self.TEST_POOL_ID, cognito_client=mock_cognito_client)

        mock_cognito_client.get_identity_pool_roles.assert_called_once_with(IdentityPoolId=self.TEST_POOL_ID)
        self.assertCountEqual(expected_roles, roles)

    # Update Assume Role Policy tests

    def test_add_identity_pool_id_to_assumed_role_policy_when_no_condition_block(self):
        expected_pool_ids = [self.TEST_POOL_ID]
        starting_policy = self._generate_assume_role_policy(condition=None)

        policy, updated = security.IdentityPoolUtils.add_pool_to_assume_role_policy(identity_pool_id=self.TEST_POOL_ID, policy_document=starting_policy)

        # policy should be updated
        self.assertEqual(True, updated)
        self._validate_assume_role_policy(policy, expected_pool_ids)

    def test_add_identity_pool_id_to_assumed_role_policy_with_empty_condition_block(self):
        expected_pool_ids = [self.TEST_POOL_ID]
        starting_policy = self._generate_assume_role_policy(condition={})

        policy, updated = security.IdentityPoolUtils.add_pool_to_assume_role_policy(identity_pool_id=self.TEST_POOL_ID, policy_document=starting_policy)

        # policy should be updated
        self.assertEqual(True, updated)
        self._validate_assume_role_policy(policy, expected_pool_ids)

    def test_add_identity_pool_id_to_assumed_role_policy_with_duplicate_pool_id_no_update(self):
        existing_pool_ids = [self.TEST_POOL_ID]
        starting_policy = self._generate_assume_role_policy(condition=self._generate_condition_block(existing_pool_ids))

        policy, updated = security.IdentityPoolUtils.add_pool_to_assume_role_policy(identity_pool_id=self.TEST_POOL_ID, policy_document=starting_policy)

        # policy should not be updated, as pool id is already in policy
        self.assertEqual(False, updated)

    def _run_update_test_for_pool_ids(self, existing_pool_ids, updated):
        expected_pool_ids = [self.TEST_POOL_ID]
        expected_pool_ids.extend(existing_pool_ids)

        starting_policy = self._generate_assume_role_policy(condition=self._generate_condition_block(existing_pool_ids))

        policy, updated = security.IdentityPoolUtils.add_pool_to_assume_role_policy(identity_pool_id=self.TEST_POOL_ID, policy_document=starting_policy)

        # policy should be updated as we're adding a poolid
        self.assertEqual(updated, updated)
        self._validate_assume_role_policy(policy, expected_pool_ids)

    def test_add_identity_pool_id_to_assumed_role_policy_with_existing_pool_ids_updates(self):
        existing_pool_ids = ["pool-id-122356789"]
        self._run_update_test_for_pool_ids(existing_pool_ids, True)

    def test_add_identity_pool_id_to_assumed_role_policy_with_multiple_existing_pool_ids_updates(self):
        existing_pool_ids = ["pool-id-122356789", "pool-id-987654321"]
        self._run_update_test_for_pool_ids(existing_pool_ids, True)

    def test_assume_role_policy_is_active(self):
        expected_pool_ids = [self.TEST_POOL_ID]
        starting_policy = self._generate_assume_role_policy(condition={})

        policy, updated = security.IdentityPoolUtils.add_pool_to_assume_role_policy(identity_pool_id=self.TEST_POOL_ID, policy_document=starting_policy)

        # policy should be updated
        self.assertEqual(True, updated)
        self._validate_assume_role_policy(policy, expected_pool_ids, 'Allow')

    def _validate_assume_role_policy(self, policy, expected_pool_ids, effect=None):
        for statement in policy['Statement']:
            action = statement.get("Action")
            if action == "sts:AssumeRoleWithWebIdentity":
                condition = statement.get('Condition')
                self.assertIsNotNone(condition, "Condition should not be None")
                string_equals = condition.get('StringEquals')
                self.assertIsNotNone(condition, "stringequals should not be None")
                aud_list = string_equals.get('cognito-identity.amazonaws.com:aud')
                self.assertIsNotNone(condition, "aud_list should not be None")
                self.assertEqual(expected_pool_ids, aud_list)

                if effect is not None:
                    self.assertEqual(effect, statement.get('Effect'))

    def _generate_condition_block(self, pool_ids, authenticated=True, ):
        authenticated_str = "authenticated" if authenticated else "unauthenticated"
        return {
            "StringEquals": {
                "cognito-identity.amazonaws.com:aud": pool_ids
            },
            "ForAnyValue:StringLike": {
                "cognito-identity.amazonaws.com:amr": authenticated_str
            }
        }

    def _generate_assume_role_policy(self, condition=None, effect=None):
        """Make a test assume role policy"""
        if effect is None:
            effect = 'Allow'

        policy = {
            "Version": "2012-10-17",
            "Statement": [
                {
                    "Effect": effect,
                    "Principal": {
                        "Federated": "cognito-identity.amazonaws.com"
                    },
                    "Action": "sts:AssumeRoleWithWebIdentity"
                }
            ]
        }

        if condition:
            for statement in policy['Statement']:
                statement['Condition'] = condition

        return policy
