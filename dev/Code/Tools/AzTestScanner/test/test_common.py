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
from aztest.common import clean_timestamp, to_list
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

        self.assertEquals([test_string], gen_list)

    def test_ToList_ListSingleString_ReturnsString(self):
        test_list = ["test"]

        gen_list = [x for x in to_list(test_list)]

        self.assertEquals(test_list, gen_list)

    def test_ToList_ListMultipleStrings_ReturnsStringsInOrder(self):
        test_list = ["test1", "test2", "test3"]

        gen_list = [x for x in to_list(test_list)]

        self.assertEquals(test_list, gen_list)

    def test_ToList_SetSingleString_ReturnsString(self):
        test_set = {"test"}

        gen_list = [x for x in to_list(test_set)]

        self.assertEquals(list(test_set), gen_list)

    def test_ToList_SetMultipleStrings_ReturnsStringsInOrder(self):
        test_set = {"test1", "test2", "test3"}

        gen_list = [x for x in to_list(test_set)]

        self.assertEquals(list(test_set), gen_list)

    def test_ToList_TupleMultipleStrings_ReturnsStringsInOrder(self):
        test_tuple = ("test1", "test2", "test3")

        gen_list = [x for x in to_list(test_tuple)]

        self.assertEquals(list(test_tuple), gen_list)

    def test_ToList_Integer_ReturnsIntegerInList(self):
        test_int = 123

        gen_list = [x for x in to_list(test_int)]

        self.assertEquals([123], gen_list)

    def test_ToList_Dictionary_ReturnsObjectInList(self):
        test_dict = {'test_key': 'test_value'}

        gen_list = [x for x in to_list(test_dict)]

        self.assertEquals([test_dict], gen_list)

    def test_ToList_Object_ReturnsObjectInList(self):
        class TestObject(object):
            pass

        test_obj = TestObject()

        gen_list = [x for x in to_list(test_obj)]

        self.assertEquals([test_obj], gen_list)
