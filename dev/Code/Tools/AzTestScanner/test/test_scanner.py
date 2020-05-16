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
import os
try:
    import mock
except ImportError:
    import unittest.mock as mock
import unittest
import aztest.scanner as scanner
from aztest.errors import InvalidUseError

WHITELIST = "lmbr_test_whitelist.txt"
BLACKLIST = "lmbr_test_blacklist.txt"


class CreateXMLOutputFilenameTests(unittest.TestCase):

    def test_CreateXMLOutputFilename_PathToFileIsNone_ThrowsInvalidUseError(self):
        with self.assertRaises(InvalidUseError) as ex:
            scanner.create_xml_output_filename(None, "OutputDir")

    def test_CreateXMLOutputFilename_OutputDirIsNone_ThrowsTypeError(self):
        with self.assertRaises(TypeError) as ex:
            scanner.create_xml_output_filename("PathToFile", None)

    @mock.patch('os.path.abspath')
    def test_CreateXMLOutputFilename_ValidPathToFileAndOutputDir_XMLPathCreated(self, mock_abspath):
        test_path_to_file = os.path.join('path', 'to', 'module.dll')
        test_output_dir = os.path.join('TestResults', 'Timestamp')

        scanner.create_xml_output_filename(test_path_to_file, test_output_dir)

        expected_path = os.path.join(test_output_dir, "test_results_module.xml")
        mock_abspath.assert_called_with(expected_path)


class AddDirsToPathTests(unittest.TestCase):

    def test_AddDirsToPath_ListIsNone_NoChanges(self):
        cur_path = os.environ['PATH']

        scanner.add_dirs_to_path(None)

        self.assertEqual(cur_path, os.environ['PATH'])

    @mock.patch('os.path.abspath')
    def test_AddDirsToPath_ListHasDirs_PathContainsDirs(self, mock_abspath):
        cur_path = os.environ['PATH']
        test_dir1 = "path/1"
        test_dir2 = "path/2"
        test_dir3 = "path/3"
        mock_abspath.side_effect = [test_dir1, test_dir2, test_dir3]

        scanner.add_dirs_to_path([test_dir1, test_dir2, test_dir3])

        expected_path = cur_path + os.pathsep + test_dir1 + os.pathsep + test_dir2 + os.pathsep + test_dir3
        self.assertEqual(expected_path, os.environ['PATH'])


class GetDefaultWhitelistTests(unittest.TestCase):

    def setUp(self):
        self.test_path = "path/to/whitelist"

    @mock.patch('os.path.abspath')
    @mock.patch('os.path.exists')
    def test_GetDefaultWhitelist_FileDoesNotExist_ReturnsBlankString(self, mock_exists, mock_abspath):
        mock_abspath.return_value = self.test_path
        mock_exists.return_value = False

        whitelist = scanner.get_default_whitelist()

        mock_abspath.assert_called_with(WHITELIST)
        mock_exists.assert_called_with(self.test_path)
        self.assertEqual("", whitelist)

    @mock.patch('os.path.abspath')
    @mock.patch('os.path.exists')
    def test_GetDefaultWhitelist_FileDoesExist_ReturnsFilePath(self, mock_exists, mock_abspath):
        mock_abspath.return_value = self.test_path
        mock_exists.return_value = True

        whitelist = scanner.get_default_whitelist()

        mock_abspath.assert_called_with(WHITELIST)
        mock_exists.assert_called_with(self.test_path)
        self.assertEqual(self.test_path, whitelist)


class GetDefaultBlacklistTests(unittest.TestCase):
    def setUp(self):
        self.test_path = "path/to/blacklist"

    @mock.patch('os.path.abspath')
    @mock.patch('os.path.exists')
    def test_GetDefaultBlacklist_FileDoesNotExist_ReturnsBlankString(self, mock_exists, mock_abspath):
        mock_abspath.return_value = self.test_path
        mock_exists.return_value = False

        blacklist = scanner.get_default_blacklist()

        mock_abspath.assert_called_with(BLACKLIST)
        mock_exists.assert_called_with(self.test_path)
        self.assertEqual("", blacklist)

    @mock.patch('os.path.abspath')
    @mock.patch('os.path.exists')
    def test_GetDefaultBlacklist_FileDoesExist_ReturnsFilePath(self, mock_exists, mock_abspath):
        mock_abspath.return_value = self.test_path
        mock_exists.return_value = True

        blacklist = scanner.get_default_blacklist()

        mock_abspath.assert_called_with(BLACKLIST)
        mock_exists.assert_called_with(self.test_path)
        self.assertEqual(self.test_path, blacklist)
