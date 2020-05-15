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
# $Revision: #3 $

from mock import MagicMock
import unittest


class CustomResourceTestCase(unittest.TestCase):

    def __init__(self, *args, **kwargs):
        super(CustomResourceTestCase, self).__init__(*args, **kwargs)

    PROPERTIES = {
        'ConfigurationBucket': 'test-bucket',
        'ConfigurationKey': 'test-key'
    }

    STACK_NAME = 'TestStack'
    LOGICAL_RESOURCE_ID = 'TestLogicalId'
    PHYSICAL_RESOURCE_ID = STACK_NAME + '-' + 'TestLogicalId'
    DEPLOYMENT_NAME = 'TestDeployment'
    REGION = 'TestRegion'
    ACCOUNT = '123456789'
    UUID = 'TestUUID'
    STACK_ARN = 'arn:aws:cloudformation:{region}:{account}:stack/{name}/{uuid}'.format(
        region=REGION,
        account=ACCOUNT,
        name=STACK_NAME,
        uuid=UUID)

    MOCK_S3_CONFIG_BUCKET = 'arn:aws:s3:{}:{}:bucket'.format(REGION, ACCOUNT)

    PROJECT_STACK = MagicMock()
    PROJECT_STACK.project_name = STACK_NAME

    RESOURCE_GROUP_INFO = MagicMock()
    RESOURCE_GROUP_INFO.is_project_stack = True
    RESOURCE_GROUP_INFO.project_stack = MagicMock()
    RESOURCE_GROUP_INFO.project_stack.configuration_bucket = MOCK_S3_CONFIG_BUCKET
    RESOURCE_GROUP_INFO.stack_name = STACK_NAME
    RESOURCE_GROUP_INFO.project_stack = PROJECT_STACK

    CONTEXT = {}

