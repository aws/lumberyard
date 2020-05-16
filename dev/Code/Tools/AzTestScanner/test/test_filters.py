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
from aztest.filters import FileApprover
from aztest.errors import AzTestError


class FileApproverTests(unittest.TestCase):

    def get_sample_set(self):
        return {'pattern1', 'pattern2', 'pattern3'}

    @mock.patch('aztest.filters.FileApprover.make_whitelist')
    @mock.patch('aztest.filters.FileApprover.make_blacklist')
    def test_Constructor_NoParams_MakeListsCalledWithNone(self, mock_blacklist, mock_whitelist):
        FileApprover()

        mock_blacklist.assert_called_with(None)
        mock_whitelist.assert_called_with(None)

    @mock.patch('aztest.filters.FileApprover.make_whitelist')
    @mock.patch('aztest.filters.FileApprover.make_blacklist')
    def test_Constructor_PathsGiven_MakeListsCalledWithPaths(self, mock_blacklist, mock_whitelist):
        test_whitelist = "path/to/whitelist"
        test_blacklist = "path/to/blacklist"

        FileApprover(test_whitelist, test_blacklist)

        mock_whitelist.assert_called_with(test_whitelist)
        mock_blacklist.assert_called_with(test_blacklist)

    def test_MakeWhitelist_WhitelistFilesIsNone_WhitelistIsNone(self):
        file_approver = FileApprover()

        file_approver.make_whitelist(None)

        self.assertIsNone(file_approver.whitelist)

    @mock.patch('aztest.filters.FileApprover.add_whitelist')
    def test_MakeWhitelist_WhitelistFilesIsSinglePath_AddWhitelistCalledWithPath(self, mock_add_whitelist):
        test_whitelist = "path/to/whitelist"
        file_approver = FileApprover()

        file_approver.make_whitelist(test_whitelist)

        mock_add_whitelist.assert_called_with(test_whitelist)

    def test_MakeWhitelist_WhitelistFilesIsEmptyList_WhitelistIsNone(self):
        file_approver = FileApprover()

        file_approver.make_whitelist([])

        self.assertIsNone(file_approver.whitelist)

    @mock.patch('aztest.filters.FileApprover.add_whitelist')
    def test_MakeWhitelist_WhitelistFilesIsList_AddWhitelistCalledWithPaths(self, mock_add_whitelist):
        test_whitelist1 = "path/to/whitelist"
        test_whitelist2 = "path/to/whitelist/2"
        test_whitelist3 = "path/to/whitelist/3"
        test_whitelist_list = [test_whitelist1, test_whitelist2, test_whitelist3]
        file_approver = FileApprover()

        file_approver.make_whitelist(test_whitelist_list)

        expected_whitelists = [mock.call(test_whitelist1), mock.call(test_whitelist2), mock.call(test_whitelist3)]
        mock_add_whitelist.assert_has_calls(expected_whitelists)

    def test_MakeBlacklist_BlacklistFilesIsNone_BlacklistIsNone(self):
        file_approver = FileApprover()

        file_approver.make_blacklist(None)

        self.assertIsNone(file_approver.blacklist)

    @mock.patch('aztest.filters.FileApprover.add_blacklist')
    def test_MakeBlacklist_BlacklistFilesIsSinglePath_AddBlacklistCalledWithPath(self, mock_add_blacklist):
        test_blacklist = "path/to/blacklist"
        file_approver = FileApprover()

        file_approver.make_blacklist(test_blacklist)

        mock_add_blacklist.assert_called_with(test_blacklist)

    def test_MakeBlacklist_BlacklistFilesIsEmptyList_BlacklistIsNone(self):
        file_approver = FileApprover()

        file_approver.make_blacklist([])

        self.assertIsNone(file_approver.blacklist)

    @mock.patch('aztest.filters.FileApprover.add_blacklist')
    def test_MakeBlacklist_BlacklistFilesIsList_AddBlacklistCalledWithPaths(self, mock_add_blacklist):
        test_blacklist1 = "path/to/blacklist"
        test_blacklist2 = "path/to/blacklist/2"
        test_blacklist3 = "path/to/blacklist/3"
        test_blacklist_list = [test_blacklist1, test_blacklist2, test_blacklist3]
        file_approver = FileApprover()

        file_approver.make_blacklist(test_blacklist_list)

        expected_blacklists = [mock.call(test_blacklist1), mock.call(test_blacklist2), mock.call(test_blacklist3)]
        mock_add_blacklist.assert_has_calls(expected_blacklists)

    def test_AddWhitelist_WhitelistFileIsNone_WhitelistIsUnchanged(self):
        file_approver = FileApprover()
        file_approver.whitelist = self.get_sample_set()

        file_approver.add_whitelist(None)

        self.assertEqual(self.get_sample_set(), file_approver.whitelist)

    @mock.patch('aztest.filters.FileApprover.get_patterns_from_file')
    def test_AddWhitelist_GetPatternsFromFileReturnsNone_WhitelistIsUnchanged(self, mock_get_patterns):
        file_approver = FileApprover()
        file_approver.whitelist = self.get_sample_set()
        mock_get_patterns.return_value = None

        file_approver.add_whitelist("path/to/whitelist")

        self.assertEqual(self.get_sample_set(), file_approver.whitelist)

    @mock.patch('aztest.filters.FileApprover.get_patterns_from_file')
    def test_AddWhitelist_ValidWhitelistFileAndWhitelistIsNone_WhitelistHasPatterns(self, mock_get_patterns):
        file_approver = FileApprover()
        test_patterns = self.get_sample_set()
        test_filename = "path/to/whitelist"
        mock_get_patterns.return_value = test_patterns

        file_approver.add_whitelist(test_filename)

        mock_get_patterns.assert_called_with(test_filename)
        self.assertEqual(test_patterns, file_approver.whitelist)

    @mock.patch('aztest.filters.FileApprover.get_patterns_from_file')
    def test_AddWhitelist_ValidWhitelistFileAndWhitelistHasValues_WhitelistHasNewPatterns(self, mock_get_patterns):
        file_approver = FileApprover()
        file_approver.whitelist = self.get_sample_set()
        test_patterns = {'pattern1', 'pattern4'}
        test_filename = "path/to/whitelist"
        mock_get_patterns.return_value = test_patterns

        file_approver.add_whitelist(test_filename)

        mock_get_patterns.assert_called_with(test_filename)
        expected_patterns = test_patterns | self.get_sample_set()
        self.assertEqual(expected_patterns, file_approver.whitelist)

    def test_AddBlacklist_BlacklistFileIsNone_BlacklistIsUnchanged(self):
        file_approver = FileApprover()
        file_approver.blacklist = self.get_sample_set()

        file_approver.add_blacklist(None)

        self.assertEqual(self.get_sample_set(), file_approver.blacklist)

    @mock.patch('aztest.filters.FileApprover.get_patterns_from_file')
    def test_AddBlacklist_GetPatternsFromFileReturnsNone_BlacklistIsUnchanged(self, mock_get_patterns):
        file_approver = FileApprover()
        file_approver.blacklist = self.get_sample_set()
        mock_get_patterns.return_value = None

        file_approver.add_blacklist("path/to/blacklist")

        self.assertEqual(self.get_sample_set(), file_approver.blacklist)

    @mock.patch('aztest.filters.FileApprover.get_patterns_from_file')
    def test_AddBlacklist_ValidBlacklistFileAndBlacklistIsNone_BlacklistHasPatterns(self, mock_get_patterns):
        file_approver = FileApprover()
        test_patterns = self.get_sample_set()
        test_filename = "path/to/blacklist"
        mock_get_patterns.return_value = test_patterns

        file_approver.add_blacklist(test_filename)

        mock_get_patterns.assert_called_with(test_filename)
        self.assertEqual(test_patterns, file_approver.blacklist)

    @mock.patch('aztest.filters.FileApprover.get_patterns_from_file')
    def test_AddBlacklist_ValidBlacklistFileAndBlacklistHasValues_BlacklistHasNewPatterns(self, mock_get_patterns):
        file_approver = FileApprover()
        file_approver.blacklist = self.get_sample_set()
        test_patterns = {'pattern1', 'pattern4'}
        test_filename = "path/to/blacklist"
        mock_get_patterns.return_value = test_patterns

        file_approver.add_blacklist(test_filename)

        mock_get_patterns.assert_called_with(test_filename)
        expected_patterns = test_patterns | self.get_sample_set()
        self.assertEqual(expected_patterns, file_approver.blacklist)

    def test_AddWhitelistPatterns_PatternIsNone_WhitelistIsUnchanged(self):
        file_approver = FileApprover()
        file_approver.whitelist = self.get_sample_set()

        file_approver.add_whitelist_patterns(None)

        self.assertEqual(self.get_sample_set(), file_approver.whitelist)

    def test_AddWhitelistPatterns_ValidPatternWhitelistIsNone_WhitelistHasPattern(self):
        file_approver = FileApprover()
        test_pattern = "pattern1"

        file_approver.add_whitelist_patterns(test_pattern)

        self.assertSetEqual({test_pattern}, file_approver.whitelist)

    def test_AddWhitelistPatterns_ValidPatternWhitelistHasValues_WhitelastHasNewPattern(self):
        file_approver = FileApprover()
        file_approver.whitelist = self.get_sample_set()
        test_pattern = "pattern4"

        file_approver.add_whitelist_patterns(test_pattern)

        expected_patterns = self.get_sample_set() | {test_pattern}
        self.assertEqual(expected_patterns, file_approver.whitelist)

    def test_AddWhitelistPatterns_ValidListOfPatternsWhitelistIsNone_WhitelistHasPatterns(self):
        file_approver = FileApprover()
        test_patterns = ["pattern1", "pattern2", "pattern3"]

        file_approver.add_whitelist_patterns(test_patterns)

        self.assertSetEqual(set(test_patterns), file_approver.whitelist)

    def test_AddWhitelistPatterns_ValidListOfPatternsWhitelistHasValues_WhitelistHasNewPatterns(self):
        file_approver = FileApprover()
        file_approver.whitelist = self.get_sample_set()
        test_patterns = ["pattern1", "pattern4", "pattern5"]

        file_approver.add_whitelist_patterns(test_patterns)

        expected_patterns = self.get_sample_set() | set(test_patterns)
        self.assertSetEqual(expected_patterns, file_approver.whitelist)


    def test_AddBlacklistPatterns_PatternIsNone_BlacklistIsUnchanged(self):
        file_approver = FileApprover()
        file_approver.blacklist = self.get_sample_set()

        file_approver.add_blacklist_patterns(None)

        self.assertEqual(self.get_sample_set(), file_approver.blacklist)

    def test_AddBlacklistPatterns_ValidPatternBlacklistIsNone_BlacklistHasPattern(self):
        file_approver = FileApprover()
        test_pattern = "pattern1"

        file_approver.add_blacklist_patterns(test_pattern)

        self.assertSetEqual({test_pattern}, file_approver.blacklist)

    def test_AddBlacklistPatterns_ValidPatternBlacklistHasValues_BlacklastHasNewPattern(self):
        file_approver = FileApprover()
        file_approver.blacklist = self.get_sample_set()
        test_pattern = "pattern4"

        file_approver.add_blacklist_patterns(test_pattern)

        expected_patterns = self.get_sample_set() | {test_pattern}
        self.assertEqual(expected_patterns, file_approver.blacklist)

    def test_AddBlacklistPatterns_ValidListOfPatternsBlacklistIsNone_BlacklistHasPatterns(self):
        file_approver = FileApprover()
        test_patterns = ["pattern1", "pattern2", "pattern3"]

        file_approver.add_blacklist_patterns(test_patterns)

        self.assertSetEqual(set(test_patterns), file_approver.blacklist)

    def test_AddBlacklistPatterns_ValidListOfPatternsBlacklistHasValues_BlacklistHasNewPatterns(self):
        file_approver = FileApprover()
        file_approver.blacklist = self.get_sample_set()
        test_patterns = ["pattern1", "pattern4", "pattern5"]

        file_approver.add_blacklist_patterns(test_patterns)

        expected_patterns = self.get_sample_set() | set(test_patterns)
        self.assertSetEqual(expected_patterns, file_approver.blacklist)

    @mock.patch('aztest.filters.FileApprover.is_whitelisted')
    @mock.patch('aztest.filters.FileApprover.is_blacklisted')
    def test_IsApproved_FilenameGiven_IsListedCalledWithFilename(self, mock_blacklisted, mock_whitelisted):
        file_approver = FileApprover()
        test_filename = "testModule.dll"

        file_approver.is_approved(test_filename)

        mock_whitelisted.assert_called_with(test_filename)
        mock_blacklisted.assert_called_with(test_filename)

    @mock.patch('aztest.filters.FileApprover.is_whitelisted')
    @mock.patch('aztest.filters.FileApprover.is_blacklisted')
    def test_IsApproved_NotWhitelistedNotBlacklisted_ReturnFalse(self, mock_blacklisted, mock_whitelisted):
        mock_whitelisted.return_value = False
        mock_blacklisted.return_value = False
        file_approver = FileApprover()

        approved = file_approver.is_approved(None)

        self.assertFalse(approved)

    @mock.patch('aztest.filters.FileApprover.is_whitelisted')
    @mock.patch('aztest.filters.FileApprover.is_blacklisted')
    def test_IsApproved_WhitelistedNotBlacklisted_ReturnTrue(self, mock_blacklisted, mock_whitelisted):
        mock_whitelisted.return_value = True
        mock_blacklisted.return_value = False
        file_approver = FileApprover()

        approved = file_approver.is_approved(None)

        self.assertTrue(approved)

    @mock.patch('aztest.filters.FileApprover.is_whitelisted')
    @mock.patch('aztest.filters.FileApprover.is_blacklisted')
    def test_IsApproved_NotWhitelistedBlacklisted_ReturnFalse(self, mock_blacklisted, mock_whitelisted):
        mock_whitelisted.return_value = False
        mock_blacklisted.return_value = True
        file_approver = FileApprover()

        approved = file_approver.is_approved(None)

        self.assertFalse(approved)

    @mock.patch('aztest.filters.FileApprover.is_whitelisted')
    @mock.patch('aztest.filters.FileApprover.is_blacklisted')
    def test_IsApproved_WhitelistedAndBlacklisted_ReturnFalse(self, mock_blacklisted, mock_whitelisted):
        mock_whitelisted.return_value = True
        mock_blacklisted.return_value = True
        file_approver = FileApprover()

        approved = file_approver.is_approved(None)

        self.assertFalse(approved)

    def test_IsWhitelisted_WhitelistIsNone_ReturnTrue(self):
        file_approver = FileApprover()

        whitelisted = file_approver.is_whitelisted(None)

        self.assertTrue(whitelisted)

    @mock.patch('aztest.filters.FileApprover.is_in_list')
    def test_IsWhitelisted_FilenameGiven_IsInListCalledWithFilenameAndReturns(self, mock_in_list):
        file_approver = FileApprover()
        test_whitelist = ['testPattern']
        file_approver.whitelist = test_whitelist
        test_filename = "testModule.dll"
        mock_in_list.return_value = False

        whitelisted = file_approver.is_whitelisted(test_filename)

        mock_in_list.assert_called_with(test_filename, test_whitelist, "whitelist")
        self.assertFalse(whitelisted)

    def test_IsBlacklisted_BlacklistIsNone_ReturnTrue(self):
        file_approver = FileApprover()

        blacklisted = file_approver.is_blacklisted(None)

        self.assertFalse(blacklisted)

    @mock.patch('aztest.filters.FileApprover.is_in_list')
    def test_IsBlacklisted_FilenameGiven_IsInListCalledWithFilenameAndReturns(self, mock_in_list):
        file_approver = FileApprover()
        test_blacklist = ['testPattern']
        file_approver.blacklist = test_blacklist
        test_filename = "testModule.dll"
        mock_in_list.return_value = True

        blacklisted = file_approver.is_blacklisted(test_filename)

        mock_in_list.assert_called_with(test_filename, test_blacklist, "blacklist")
        self.assertTrue(blacklisted)

    def test_GetPatternsFromFile_FilenameIsNone_ThrowsError(self):
        with self.assertRaises(AzTestError) as ex:
            patterns = FileApprover.get_patterns_from_file(None)

    @mock.patch('os.path.exists')
    def test_GetPatternsFromFile_FileDoesNotExist_ThrowsError(self, mock_path_exists):
        mock_path_exists.return_value = False

        with self.assertRaises(AzTestError) as ex:
            patterns = FileApprover.get_patterns_from_file("path/to/list")

    @mock.patch('os.path.exists')
    def test_GetPatternsFromFile_FilenameGiven_OpenCalledWithFilenameAndReturnsList(self, mock_path_exists):
        mock_path_exists.return_value = True
        test_filename = "path/to/list"
        test_data = "line1\nline2\nline3\n"
        mock_open = mock.mock_open(read_data=test_data)
        
        try:
            patcher = mock.patch('__builtin__.open', mock_open)
            patcher.start()
            mock_open.return_value.__iter__.return_value = test_data.splitlines()
        except ImportError:  # py3
            patcher = mock.patch('builtins.open', mock_open)
            patcher.start()
            
        expected_patterns = set(["line1", "line2", "line3"])

        patterns = FileApprover.get_patterns_from_file(test_filename)

        mock_open.assert_called_with(test_filename, 'r')
        self.assertSetEqual(expected_patterns, patterns)
        patcher.stop()
    
    def test_IsInList_FilenameIsNone_ReturnsFalse(self):
        in_list = FileApprover.is_in_list(None, ['pattern1'])

        self.assertFalse(in_list)

    def test_IsInList_PatternsIsNone_ReturnsFalse(self):
        in_list = FileApprover.is_in_list("testModule.dll", None)

        self.assertFalse(in_list)

    def test_IsInList_PatternsIsEmpty_ReturnsFalse(self):
        in_list = FileApprover.is_in_list("testModule.dll", [])

        self.assertFalse(in_list)

    @mock.patch('re.match')
    def test_IsInList_PatternIsInList_ReturnsTrue(self, mock_match):
        import re

        mock_match.return_value = True
        test_filename = "testModule.dll"
        test_patterns = ['testModule']
        expected_pattern = r"^.*[/\\]testModule(\.dll|\.exe|\.dylib)?$"

        in_list = FileApprover.is_in_list(test_filename, test_patterns)

        mock_match.assert_called_with(expected_pattern, test_filename, re.IGNORECASE)
        self.assertTrue(in_list)
