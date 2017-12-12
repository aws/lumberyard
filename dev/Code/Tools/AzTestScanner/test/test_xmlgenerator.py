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
import mock
import unittest
import aztest.scanner as scanner
from aztest.errors import InvalidUseError


@mock.patch('datetime.datetime')
@mock.patch('xml.etree.ElementTree')
@mock.patch('__builtin__.open')
class CreateXMLOutputFileTests(unittest.TestCase):
    def setUp(self):
        self.error_code = 102
        self.error_message = "error message"
        self.xml_test_path = "path/to/test_results.xml"

    def test_XMLGenerator_PathToFileIsNone_ThrowsInvalidUseError(self, mock_open, mock_elemTree, mock_datetime):
        with self.assertRaises(InvalidUseError):
            scanner.XMLGenerator.create_xml_output_file(None, self.error_code, self.error_message)

    def test_XMLGenerator_ErrorCodeIsNone_ThrowsInvalidUseError(self, mock_open, mock_elemTree, mock_datetime):
        with self.assertRaises(InvalidUseError):
            scanner.XMLGenerator.create_xml_output_file(self.xml_test_path, None, self.xml_test_path)

    def test_XMLGenerator_ErrorMessageIsNone_ThrowsInvalidUseError(self, mock_open, mock_elemTree, mock_datetime):
        with self.assertRaises(InvalidUseError):
            scanner.XMLGenerator.create_xml_output_file(self.xml_test_path, self.error_code, None)

    def test_XMLGenerator_ValidPathToFile_XMLFileGenerated(self, mock_open, mock_elemTree, mock_datetime):
        scanner.XMLGenerator.create_xml_output_file(self.xml_test_path, self.error_code, self.error_message)

        mock_open.assert_called_with(self.xml_test_path, 'w')
        mock_elemTree.return_value.write(self.xml_test_path, encoding="UTF-8", xml_declaration=True)
