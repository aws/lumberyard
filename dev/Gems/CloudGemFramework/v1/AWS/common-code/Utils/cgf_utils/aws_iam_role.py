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
from .aws_utils import ArnParser


class IAMRole:
    """Utils for working with IAM Roles and TrustPolicies"""
    def __init__(self, role_name, role_arn, assume_role_policy=None):
        if assume_role_policy is None:
            assume_role_policy = {}

        self.__role_name = role_name
        self.__arn = role_arn

        _ap = ArnParser(self.__arn)
        self.__partition = _ap.partition
        self.__account = _ap.account_id
        self.__assume_role_policy = assume_role_policy

    @staticmethod
    def factory(role_name, iam_client):
        _response = iam_client.get_role(RoleName=role_name)
        role = _response.get("Role", {})
        return IAMRole(role_name=role_name,
                       role_arn=role.get("Arn", {}),
                       assume_role_policy=role.get("AssumeRolePolicyDocument", {}))

    @staticmethod
    def role_name_from_arn_without_path(role_arn):
        role_name = ArnParser(role_arn).resource_id
        if "/" in role_name:
            role_name = role_name.rsplit("/", 1)[1]
        return role_name

    @property
    def assume_role_policy(self):
        return self.__assume_role_policy

    @property
    def arn(self):
        return self.__arn

    @property
    def role_name(self):
        return self.__role_name

    @property
    def account(self):
        return self.__account

    def update_trust_policy(self, new_policy, iam_client):
        iam_client.update_assume_role_policy(
            RoleName=self.__role_name,
            PolicyDocument=new_policy)

        self.__assume_role_policy = new_policy


