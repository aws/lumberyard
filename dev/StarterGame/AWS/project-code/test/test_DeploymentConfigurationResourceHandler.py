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
# $Revision: #1 $

import unittest
import mock

import DeploymentConfigurationResourceHandler
import custom_resource_response

class TestDeploymentConfiguration(unittest.TestCase):

    event = {}

    context = {}

    def setUp(self):
        self.event = {
            'ResourceProperties': {
                'ConfigurationBucket': 'TestBucket',
                'ConfigurationKey': 'TestKey',
                'DeploymentName': 'TestDeployment'
            },
            'StackId': 'arn:aws:cloudformation:TestRegion:TestAccount:stack/TestStack/9574f270-787e-11e5-95cb-500160d4da44'
        }


    def test_handler(self):

        expected_data = {
            'ConfigurationBucket': 'TestBucket',
            'ConfigurationKey': 'TestKey/deployment/TestDeployment',
            'DeploymentTemplateURL': 'https://s3.amazonaws.com/TestBucket/TestKey/deployment/TestDeployment/deployment-template.json',
            'AccessTemplateURL': 'https://s3.amazonaws.com/TestBucket/TestKey/deployment-access-template-TestDeployment.json',
            'DeploymentName': 'TestDeployment'
        }

        expected_physical_id = 'CloudCanvas:DeploymentConfiguration:TestStack:TestDeployment'
                
        with mock.patch.object(custom_resource_response, 'succeed') as mock_custom_resoruce_response_succeed:
            DeploymentConfigurationResourceHandler.handler(self.event, self.context)
            mock_custom_resoruce_response_succeed.assert_called_with(self.event, self.context, expected_data, expected_physical_id)

