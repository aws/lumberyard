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
# $Revision: #2 $

import unittest
import mock

from cgf_utils import custom_resource_response
from resource_types import Custom_Helper

from cgf_utils.properties import ValidationError

class UnitTest_CloudGemFramework_ProjectResourceHandler_HelperResourceHandler(unittest.TestCase):


    @mock.patch.object(custom_resource_response, 'success_response')
    def __assert_succeeds(self, input, expected_output, mock_response_succeed):

        event = {
            'ResourceProperties': {
                'Input': input
            },
            'LogicalResourceId': 'TestLogicalId',
            'StackId': 'arn:aws:cloudformation:{region}:{account}:stack/TestStackName/{uuid}'
        }

        context = {}

        Custom_Helper.handler(event, context)

        expected_physical_resource_id = 'TestStackName-TestLogicalId'

        mock_response_succeed.assert_called_with(expected_output, expected_physical_resource_id)


    def __assert_fails(self, input):

        event = {
            'ResourceProperties': {
                'Input': input
            },
            'LogicalResourceId': 'TestLogicalId',
            'StackId': 'arn:aws:cloudformation:{region}:{account}:stack/TestStackName/{uuid}'
        }

        context = {}

        with self.assertRaises(ValidationError):
            Custom_Helper.handler(event, context)


    def test_non_functions(self):

        input = {
            'Dict': {
                'A': 1,
                'B': 2
            },
            'List': [
                1, 2, { 'C': 3 }
            ]
        }

        expected_output = input

        self.__assert_succeeds(input, expected_output)


    def test_lower_case_with_string(self):

        input = {
            'Test': { 'HelperFn::LowerCase': 'UPPER' }
        }

        expected_otput = {
            'Test': 'upper'
        }

        self.__assert_succeeds(input, expected_otput)


    def test_lower_case_with_non_string(self):
        
        input = {
            'Test': { 'HelperFn::LowerCase': {} }
        }

        self.__assert_fails(input)            