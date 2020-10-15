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

from unittest import mock
from unittest.mock import MagicMock
import os

from .custom_resource_test_case import CustomResourceTestCase

from resource_manager_common.test.mock_stack_info import MockResourceGroupInfo

# Need to patch the environment before loading InterfaceDependencyResolver
TEST_REGION = 'test-region'
os.environ['AWS_DEFAULT_REGION'] = TEST_REGION
from resource_types import InterfaceDependencyResolver

REST_API_ID = 'TestRestApiId'
PATH = 'path'
STAGE_NAME = 'api'
DEFAULT_URL = 'https://{}.execute-api.{}.amazonaws.com/{}/{}'.format(REST_API_ID, MockResourceGroupInfo.MOCK_REGION, STAGE_NAME, PATH)
CUSTOM_DOMAIN_NAME = 'TestCustomDomainName'
ALTERNATIVE_URL = 'https://{}/{}.{}.{}/{}'.format(CUSTOM_DOMAIN_NAME, MockResourceGroupInfo.MOCK_REGION, STAGE_NAME, REST_API_ID, PATH)


# Specific unit tests to test setting up Cognito User pools
class UnitTest_CloudGemFramework_ResourceTypeResourcesHandler_InterfaceDependencyResolver(CustomResourceTestCase):

    @mock.patch.object(InterfaceDependencyResolver, 'InterfaceUrlParts')
    @mock.patch.object(InterfaceDependencyResolver, 'InterfaceUrlParts')
    def test_parse_default_interface_url(self, *args):
        os.environ['CustomDomainName'] = ''
        InterfaceDependencyResolver._parse_interface_url(DEFAULT_URL)
        
        response = {
            'api_id': REST_API_ID,
            'region': MockResourceGroupInfo.MOCK_REGION,
            'stage_name': STAGE_NAME,
            'path': PATH
        }

        InterfaceDependencyResolver.InterfaceUrlParts.assert_called_once_with(
            api_id=REST_API_ID, path=PATH, region=MockResourceGroupInfo.MOCK_REGION, stage_name=STAGE_NAME)
    
    @mock.patch.object(InterfaceDependencyResolver, 'InterfaceUrlParts')
    def test_parse_interface_url_with_custom_domain_name(self, *args):
        os.environ['CustomDomainName'] = CUSTOM_DOMAIN_NAME
        InterfaceDependencyResolver._parse_interface_url(ALTERNATIVE_URL)

        response = {
            'api_id': REST_API_ID,
            'region': MockResourceGroupInfo.MOCK_REGION,
            'stage_name': STAGE_NAME,
            'path': PATH
        }

        InterfaceDependencyResolver.InterfaceUrlParts.assert_called_once_with(
            api_id=REST_API_ID, path=PATH, region=MockResourceGroupInfo.MOCK_REGION, stage_name=STAGE_NAME)
