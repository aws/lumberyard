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
import mock
import json
import os
import tempfile

import swagger_processor

from mock_swagger_json_navigator import SwaggerNavigatorMatcher
from resource_manager.errors import HandledError

class UnitTest_CloudGemFramework_ResourceManagerCode_swagger_processor_file_handling(unittest.TestCase):


    def setUp(self):
        self.__temp_files = []
        return super(UnitTest_CloudGemFramework_ResourceManagerCode_swagger_processor_file_handling, self).setUp()


    def tearDown(self):
        for temp_file in self.__temp_files:
            os.remove(temp_file)            
        return super(UnitTest_CloudGemFramework_ResourceManagerCode_swagger_processor_file_handling, self).tearDown()


    def test_invalid_file(self):
        mock_context = mock.MagicMock()
        self.assertRaises(HandledError, swagger_processor.process_swagger_path, mock_context, 'C:\\bad_swagger_file_path')


    def test_invalid_json(self):
        mock_context = mock.MagicMock()
        swagger_path = self.__temp_file('{ this is invalid json }')
        self.assertRaises(HandledError, swagger_processor.process_swagger_path, mock_context, swagger_path)


    @mock.patch.object(swagger_processor, 'process_swagger')
    def test_valid_json(self, mock_process_swagger):
        input_json = '{ "this is": "valid json" }'
        swagger_path = self.__temp_file(input_json)
        mock_context = mock.MagicMock()
        swagger_processor.process_swagger_path(mock_context, swagger_path)
        expected_json = json.loads(input_json)
        mock_process_swagger.assert_called_once_with(mock_context, expected_json)


    def test_invalid_swagger(self):
        swagger = { "this is" : "not valid swagger" }
        mock_context = mock.MagicMock()
        self.assertRaises(ValueError, swagger_processor.process_swagger, mock_context, swagger)


    @mock.patch('swagger_processor.interface.process_interface_implementation_objects')
    @mock.patch('swagger_processor.lambda_dispatch.process_lambda_dispatch_objects')
    def test_valid_swagger(self, mock_process_lambda_dispatch_objects, mock_process_interface_implementation_objects):

        swagger = {
            "swagger": "2.0",
            "info": {
                "version": "1.0.0",
                "title": ""
           },
           "paths": {
            }
        }

        mock_context = mock.MagicMock()
        swagger_navigator_matcher = SwaggerNavigatorMatcher(swagger)

        swagger_processor.process_swagger(mock_context, swagger)

        mock_process_lambda_dispatch_objects.assert_called_with(mock_context, swagger_navigator_matcher)
        mock_process_interface_implementation_objects.assert_called_with(mock_context, swagger_navigator_matcher)


    def __temp_file(self, content):
        (fd, path) = tempfile.mkstemp()
        self.__temp_files.append(path)
        os.write(fd, content) 
        os.close(fd)
        return path


