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


class MockDeploymentInfo(object):

    MOCK_DEPLOYMENT_NAME = 'TestDeployment'
    MOCK_STACK_NAME = 'MockDeploymentStack'
    MOCK_REGION = 'MockRegion'
    MOCK_ACCOUNT = 'MockAccount'
    MOCK_UUID = 'MockUUID'
    MOCK_STACK_ARN = 'arn:aws:cloudformation:{region}:{account}:stack/{name}/{uuid}'.format(
        region = MOCK_REGION,
        account = MOCK_ACCOUNT,
        name = MOCK_STACK_NAME,
        uuid = MOCK_UUID)

    def __init__(self):
        self.deployment_name = self.MOCK_DEPLOYMENT_NAME
        self.stack_arn = self.MOCK_STACK_ARN
        self.stack_name = self.MOCK_STACK_NAME
        self.region = self.MOCK_REGION

class MockResourceGroupInfo(object):

    MOCK_RESOURCE_GROUP_NAME = 'TestResourceGroup'
    MOCK_STACK_NAME = 'MockResourceGroupStack'
    MOCK_REGION = 'MockRegion'
    MOCK_ACCOUNT = 'MockAccount'
    MOCK_UUID = 'MockUUID'
    MOCK_STACK_ARN = 'arn:aws:cloudformation:{region}:{account}:stack/{name}/{uuid}'.format(
        region = MOCK_REGION,
        account = MOCK_ACCOUNT,
        name = MOCK_STACK_NAME,
        uuid = MOCK_UUID)

    def __init__(self):
        self.resource_group_name = self.MOCK_RESOURCE_GROUP_NAME
        self.stack_arn = self.MOCK_STACK_ARN
        self.stack_name = self.MOCK_STACK_NAME
        self.region = self.MOCK_REGION
        self.deployment = MockDeploymentInfo()

