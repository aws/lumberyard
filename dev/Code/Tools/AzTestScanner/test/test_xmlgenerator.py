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
try:
    import mock
except ImportError:
    import unittest.mock as mock
import unittest
import aztest.scanner as scanner
from aztest.errors import InvalidUseError


@mock.patch('datetime.datetime', mock.Mock)
@mock.patch('xml.etree.ElementTree')
class CreateXMLOutputFileTests(unittest.TestCase):
    def setUp(self):
        self.error_code = 102
        self.error_message = "error message"
        self.xml_test_path = "path/to/test_results.xml"
        self.mock_open = mock.mock_open()
        try:
            self.patcher = mock.patch('__builtin__.open', self.mock_open)
            self.patcher.start()
        except ImportError:
            self.patcher = mock.patch('builtins.open', self.mock_open)
            self.patcher.start()

    def tearDown(self):
        self.patcher.stop()

    def test_XMLGenerator_PathToFileIsNone_ThrowsInvalidUseError(self, mock_elemTree):
        with self.assertRaises(InvalidUseError):
            scanner.XMLGenerator.create_xml_output_file(None, self.error_code, self.error_message)

    def test_XMLGenerator_ErrorCodeIsNone_ThrowsInvalidUseError(self, mock_elemTree):
        with self.assertRaises(InvalidUseError):
            scanner.XMLGenerator.create_xml_output_file(self.xml_test_path, None, self.xml_test_path)

    def test_XMLGenerator_ErrorMessageIsNone_ThrowsInvalidUseError(self, mock_elemTree):
        with self.assertRaises(InvalidUseError):
            scanner.XMLGenerator.create_xml_output_file(self.xml_test_path, self.error_code, None)

    def test_XMLGenerator_ValidPathToFile_XMLFileGenerated(self, mock_elemTree):
        scanner.XMLGenerator.create_xml_output_file(self.xml_test_path, self.error_code, self.error_message)

        self.mock_open.assert_called_with(self.xml_test_path, 'w')
        mock_elemTree.return_value.write(self.xml_test_path, encoding="UTF-8", xml_declaration=True)
