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

# Python 2.7/3.7 Compatibility
from six import string_types

import unittest


class EnhancedTestCase(unittest.TestCase):

    def __init__(self, *args, **kwargs):
        super(EnhancedTestCase, self).__init__(*args, **kwargs)

    class AnyInstance(object):

        def __init__(self, expected_type):
            self.__expected_type = expected_type

        def __eq__(self, other):
            return isinstance(other, self.__expected_type)

    ANY_LIST = AnyInstance(list)
    ANY_STRING = AnyInstance(string_types)


class ResourceHandlerTestCase(EnhancedTestCase):

    def __init__(self, *args, **kwargs):
        super(ResourceHandlerTestCase, self).__init__(*args, **kwargs)

    PROPERTIES = {
        'ConfigurationBucket': 'test-bucket',
        'ConfigurationKey': 'test-key'
    }

    STACK_NAME = 'TestStack'
    LOGICAL_RESOURCE_ID = 'TestLogicalId'
    PHYSICAL_RESOURCE_ID = STACK_NAME + '-' + 'TestLogicalId'
    DEPLOYMENT_NAME = 'TestDeployment'
    REGION = 'TestRegion'
    ACCOUNT = 'TestAccount'
    UUID = 'TestUUID'
    STACK_ARN = 'arn:aws:cloudformation:{region}:{account}:stack/{name}/{uuid}'.format(
        region=REGION,
        account=ACCOUNT,
        name=STACK_NAME,
        uuid=UUID)

    CONTEXT = {}

    @classmethod
    def make_event(cls, request_type, properties=PROPERTIES, stack_arn=STACK_ARN, logical_resource_id=LOGICAL_RESOURCE_ID,
                   physical_resource_id=PHYSICAL_RESOURCE_ID):
        event = {
            'RequestType': request_type,
            'ResourceProperties': properties,
            'StackId': stack_arn,
            'LogicalResourceId': logical_resource_id
        }
        if request_type != 'Create':
            event['PhysicalResourceId'] = physical_resource_id
        return event
