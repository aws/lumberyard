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
import datetime
try:
    import mock
except ImportError:
    import unittest.mock as mock
import unittest
from aztest.common import clean_timestamp, get_output_dir, to_list
from aztest.errors import InvalidUseError


class CleanTimestampTests(unittest.TestCase):

    def test_CleanTimestamp_TimestampIsNone_ReturnsEmptyString(self):
        cleaned_timestamp = clean_timestamp(None)

        self.assertEqual("", cleaned_timestamp)

    def test_CleanTimestamp_TimestampIsEmptyString_ReturnsEmptyString(self):
        cleaned_timestamp = clean_timestamp("")

        self.assertEqual("", cleaned_timestamp)

    def test_CleanTimestamp_TimestampIsNotString_RaisesInvalidUseError(self):
        with self.assertRaises(InvalidUseError) as ex:
            clean_timestamp(123)

    def test_CleanTimestamp_NoColonsNoPeriods_StringIsUnchanged(self):
        test_timestamp = "20160101T120000"

        cleaned_timestamp = clean_timestamp(test_timestamp)

        self.assertEqual(test_timestamp, cleaned_timestamp)

    def test_CleanTimestamp_OnlyColons_ColonsBecomeUnderscores(self):
        test_timestamp = "12:00:00"
        expected_timestamp = "12_00_00"

        cleaned_timestamp = clean_timestamp(test_timestamp)

        self.assertEqual(expected_timestamp, cleaned_timestamp)

    def test_CleanTimestamp_OnlyPeriods_PeriodsBecomeUnderscores(self):
        test_timestamp = "12.00.00"
        expected_timestamp = "12_00_00"

        cleaned_timestamp = clean_timestamp(test_timestamp)

        self.assertEqual(expected_timestamp, cleaned_timestamp)

    def test_CleanTimestamp_ColonsAndPeriods_ColonsAndPeriodsBecomeUnderscores(self):
        test_timestamp = "2016.01.01 12:00:00"
        expected_timestamp = "2016_01_01_12_00_00"

        cleaned_timestamp = clean_timestamp(test_timestamp)

        self.assertEqual(expected_timestamp, cleaned_timestamp)


class ToListTests(unittest.TestCase):

    def test_ToList_ParamIsNone_ReturnsNothing(self):
        gen_list = [x for x in to_list(None)]

        self.assertEqual([], gen_list)

    def test_ToList_EmptyList_ReturnsNothing(self):
        gen_list = [x for x in to_list([])]

        self.assertEqual([], gen_list)

    def test_ToList_EmptyString_ReturnsEmptyString(self):
        gen_list = [x for x in to_list("")]

        self.assertEqual([""], gen_list)

    def test_ToList_SingleString_ReturnsString(self):
        test_string = "test"

        gen_list = [x for x in to_list(test_string)]

        self.assertEqual([test_string], gen_list)

    def test_ToList_ListSingleString_ReturnsString(self):
        test_list = ["test"]

        gen_list = [x for x in to_list(test_list)]

        self.assertEqual(test_list, gen_list)

    def test_ToList_ListMultipleStrings_ReturnsStringsInOrder(self):
        test_list = ["test1", "test2", "test3"]

        gen_list = [x for x in to_list(test_list)]

        self.assertEqual(test_list, gen_list)

    def test_ToList_SetSingleString_ReturnsString(self):
        test_set = {"test"}

        gen_list = [x for x in to_list(test_set)]

        self.assertEqual(list(test_set), gen_list)

    def test_ToList_SetMultipleStrings_ReturnsStringsInOrder(self):
        test_set = {"test1", "test2", "test3"}

        gen_list = [x for x in to_list(test_set)]

        self.assertEqual(list(test_set), gen_list)

    def test_ToList_TupleMultipleStrings_ReturnsStringsInOrder(self):
        test_tuple = ("test1", "test2", "test3")

        gen_list = [x for x in to_list(test_tuple)]

        self.assertEqual(list(test_tuple), gen_list)

    def test_ToList_Integer_ReturnsIntegerInList(self):
        test_int = 123

        gen_list = [x for x in to_list(test_int)]

        self.assertEqual([123], gen_list)

    def test_ToList_Dictionary_ReturnsObjectInList(self):
        test_dict = {'test_key': 'test_value'}

        gen_list = [x for x in to_list(test_dict)]

        self.assertEqual([test_dict], gen_list)

    def test_ToList_Object_ReturnsObjectInList(self):
        class TestObject(object):
            pass

        test_obj = TestObject()

        gen_list = [x for x in to_list(test_obj)]

        self.assertEqual([test_obj], gen_list)


class GetOutputDirTests(unittest.TestCase):

    @mock.patch('os.path.join')
    def test_GetOutputDir_TimestampIsNoneDefaultPrefix_ReturnsNoTimestampDefaultPrefix(self, mock_join):
        prefix_dir = "TestResults"
        get_output_dir(None, prefix_dir)

        mock_join.assert_called_with(prefix_dir, "NO_TIMESTAMP")

    @mock.patch('os.path.join')
    def test_GetOutputDir_TimestampIsNoneCustomPrefix_ReturnsNoTimestampCustomPrefix(self, mock_join):
        test_prefix = "CustomPrefix"

        get_output_dir(None, test_prefix)

        mock_join.assert_called_with(test_prefix, "NO_TIMESTAMP")

    @mock.patch('os.path.join')
    def test_GetOutputDir_TimestampGivenDefaultPrefix_ReturnsTimestampedDirDefaultPrefix(self, mock_join):
        prefix_dir = "TestResults"
        timestamp = datetime.datetime(2016, 1, 1, 12, 0, 0, 123456)

        get_output_dir(timestamp, prefix_dir)

        expected_timestamp = "2016-01-01T12_00_00_123456"
        mock_join.assert_called_with("TestResults", expected_timestamp)

    @mock.patch('os.path.join')
    def test_GetOutputDir_TimestampGivenCustomPrefix_ReturnsTimestampedDirCustomPrefix(self, mock_join):
        timestamp = datetime.datetime(2016, 1, 1, 12, 0, 0, 123456)
        test_prefix = "CustomPrefix"

        get_output_dir(timestamp, test_prefix)

        expected_timestamp = "2016-01-01T12_00_00_123456"
        mock_join.assert_called_with(test_prefix, expected_timestamp)