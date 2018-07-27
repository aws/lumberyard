/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 */


#include "StdAfx.h"
#include "StringHelpers.h"
#include <AzTest/AzTest.h>

namespace StringHelpersTest
{
    TEST(CryCommonToolsStringHelpersTest, Compare_TwoMatchingStrings_ReturnsZero)
    {
        const char* string1 = "foo";
        const char* string2 = "foo";
        int result = StringHelpers::Compare(string1, string2);
        EXPECT_EQ(0, result);
    }

    TEST(CryCommonToolsStringHelpersTest, Compare_SecondStringLonger_ReturnsGreaterThanZero)
    {
        const char* string1 = "foo";
        const char* string2 = "foobar";
        int result = StringHelpers::Compare(string1, string2);
        EXPECT_GT(0, result);
    }

    TEST(CryCommonToolsStringHelpersTest, Compare_FirstStringLonger_ReturnsLessThanZero)
    {
        const char* string1 = "foobar";
        const char* string2 = "foo";
        int result = StringHelpers::Compare(string1, string2);
        EXPECT_LT(0, result);
    }

    TEST(CryCommonToolsStringHelpersTest, Compare_FirstStringCapitalized_ReturnsGreaterThanZero)
    {
        const char* string1 = "FOO";
        const char* string2 = "foo";
        int result = StringHelpers::Compare(string1, string2);
        EXPECT_GT(0, result);
    }

    TEST(CryCommonToolsStringHelpersTest, Compare_SecondStringCapitalized_ReturnsLessThanZero)
    {
        const char* string1 = "foo";
        const char* string2 = "FOO";
        int result = StringHelpers::Compare(string1, string2);
        EXPECT_LT(0, result);
    }

    TEST(CryCommonToolsStringHelpersTest, Compare_TwoMatchingWStrings_ReturnsZero)
    {
        const wchar_t string1[] = L"foo";
        const wchar_t string2[] = L"foo";
        int result = StringHelpers::Compare(string1, string2);
        EXPECT_EQ(0, result);
    }

    TEST(CryCommonToolsStringHelpersTest, Compare_SecondWStringLonger_ReturnsGreaterThanZero)
    {
        const wchar_t string1[] = L"foo";
        const wchar_t string2[] = L"foobar";
        int result = StringHelpers::Compare(string1, string2);
        EXPECT_GT(0, result);
    }

    TEST(CryCommonToolsStringHelpersTest, Compare_FirstWStringLonger_ReturnsLessThanZero)
    {
        const wchar_t string1[] = L"foobar";
        const wchar_t string2[] = L"foo";
        int result = StringHelpers::Compare(string1, string2);
        EXPECT_LT(0, result);
    }

    TEST(CryCommonToolsStringHelpersTest, Compare_FirstWStringCapitalized_ReturnsGreaterThanZero)
    {
        const wchar_t string1[] = L"FOO";
        const wchar_t string2[] = L"foo";
        int result = StringHelpers::Compare(string1, string2);
        EXPECT_GT(0, result);
    }

    TEST(CryCommonToolsStringHelpersTest, Compare_SecondWStringCapitalized_ReturnsLessThanZero)
    {
        const wchar_t string1[] = L"foo";
        const wchar_t string2[] = L"FOO";
        int result = StringHelpers::Compare(string1, string2);
        EXPECT_LT(0, result);
    }

    TEST(CryCommonToolsStringHelpersTest, CompareIgnoreCase_TwoMatchingStrings_ReturnsZero)
    {
        const char* string1 = "foo";
        const char* string2 = "foo";
        int result = StringHelpers::CompareIgnoreCase(string1, string2);
        EXPECT_EQ(0, result);
    }

    TEST(CryCommonToolsStringHelpersTest, CompareIgnoreCase_FirstStringCapitalized_ReturnsZero)
    {
        const char* string1 = "FOO";
        const char* string2 = "foo";
        int result = StringHelpers::CompareIgnoreCase(string1, string2);
        EXPECT_EQ(0, result);
    }

    TEST(CryCommonToolsStringHelpersTest, CompareIgnoreCase_SecondStringCapitalized_ReturnsZero)
    {
        const char* string1 = "foo";
        const char* string2 = "FOO";
        int result = StringHelpers::CompareIgnoreCase(string1, string2);
        EXPECT_EQ(0, result);
    }

    TEST(CryCommonToolsStringHelpersTest, CompareIgnoreCase_SecondStringLonger_ReturnsGreaterThanZero)
    {
        const char* string1 = "foo";
        const char* string2 = "foobar";
        int result = StringHelpers::CompareIgnoreCase(string1, string2);
        EXPECT_GT(0, result);
    }

    TEST(CryCommonToolsStringHelpersTest, CompareIgnoreCase_FirstStringLonger_ReturnsLessThanZero)
    {
        const char* string1 = "foobar";
        const char* string2 = "foo";
        int result = StringHelpers::CompareIgnoreCase(string1, string2);
        EXPECT_LT(0, result);
    }

    TEST(CryCommonToolsStringHelpersTest, CompareIgnoreCase_TwoMatchingWStrings_ReturnsZero)
    {
        const wchar_t string1[] = L"foo";
        const wchar_t string2[] = L"foo";
        int result = StringHelpers::CompareIgnoreCase(string1, string2);
        EXPECT_EQ(0, result);
    }

    TEST(CryCommonToolsStringHelpersTest, CompareIgnoreCase_FirstWStringCapitalized_ReturnsZero)
    {
        const wchar_t string1[] = L"FOO";
        const wchar_t string2[] = L"foo";
        int result = StringHelpers::CompareIgnoreCase(string1, string2);
        EXPECT_EQ(0, result);
    }

    TEST(CryCommonToolsStringHelpersTest, CompareIgnoreCase_SecondWStringCapitalized_ReturnsZero)
    {
        const wchar_t string1[] = L"foo";
        const wchar_t string2[] = L"FOO";
        int result = StringHelpers::CompareIgnoreCase(string1, string2);
        EXPECT_EQ(0, result);
    }

    TEST(CryCommonToolsStringHelpersTest, CompareIgnoreCase_SecondWStringLonger_ReturnsGreaterThanZero)
    {
        const wchar_t string1[] = L"foo";
        const wchar_t string2[] = L"foobar";
        int result = StringHelpers::CompareIgnoreCase(string1, string2);
        EXPECT_GT(0, result);
    }

    TEST(CryCommonToolsStringHelpersTest, CompareIgnoreCase_FirstWStringLonger_ReturnsLessThanZero)
    {
        const wchar_t string1[] = L"foobar";
        const wchar_t string2[] = L"foo";
        int result = StringHelpers::CompareIgnoreCase(string1, string2);
        EXPECT_LT(0, result);
    }

    TEST(CryCommonToolsStringHelpersTest, Equals_SameTwoStrings_ReturnsTrue)
    {
        const char* string1 = "foo";
        const char* string2 = "foo";
        bool result = StringHelpers::Equals(string1, string2);
        EXPECT_TRUE(result);
    }

    TEST(CryCommonToolsStringHelpersTest, Equals_SecondStringUpperCase_ReturnsFalse)
    {
        const char* string1 = "foo";
        const char* string2 = "FOO";
        bool result = StringHelpers::Equals(string1, string2);
        EXPECT_FALSE(result);
    }

    TEST(CryCommonToolsStringHelpersTest, Equals_FirstStringUpperCase_ReturnsFalse)
    {
        const char* string1 = "FOO";
        const char* string2 = "foo";
        bool result = StringHelpers::Equals(string1, string2);
        EXPECT_FALSE(result);
    }

    TEST(CryCommonToolsStringHelpersTest, Equals_DifferentStrings_ReturnsFalse)
    {
        const char* string1 = "foo";
        const char* string2 = "foobar";
        bool result = StringHelpers::Equals(string1, string2);
        EXPECT_FALSE(result);
    }

    TEST(CryCommonToolsStringHelpersTest, Equals_SameTwoWStrings_ReturnsTrue)
    {
        const wchar_t string1[] = L"foo";
        const wchar_t string2[] = L"foo";
        bool result = StringHelpers::Equals(string1, string2);
        EXPECT_TRUE(result);
    }

    TEST(CryCommonToolsStringHelpersTest, Equals_SecondWStringUpperCase_ReturnsFalse)
    {
        const wchar_t string1[] = L"foo";
        const wchar_t string2[] = L"FOO";
        bool result = StringHelpers::Equals(string1, string2);
        EXPECT_FALSE(result);
    }

    TEST(CryCommonToolsStringHelpersTest, Equals_FirstWStringUpperCase_ReturnsFalse)
    {
        const wchar_t string1[] = L"FOO";
        const wchar_t string2[] = L"foo";
        bool result = StringHelpers::Equals(string1, string2);
        EXPECT_FALSE(result);
    }

    TEST(CryCommonToolsStringHelpersTest, Equals_DifferentWStrings_ReturnsFalse)
    {
        const wchar_t string1[] = L"foo";
        const wchar_t string2[] = L"foobar";
        bool result = StringHelpers::Equals(string1, string2);
        EXPECT_FALSE(result);
    }

    TEST(CryCommonToolsStringHelpersTest, EqualsIgnoreCase_SameTwoStrings_ReturnsTrue)
    {
        const char* string1 = "foo";
        const char* string2 = "foo";
        bool result = StringHelpers::EqualsIgnoreCase(string1, string2);
        EXPECT_TRUE(result);
    }

    TEST(CryCommonToolsStringHelpersTest, EqualsIgnoreCase_SecondStringUpperCase_ReturnsFalse)
    {
        const char* string1 = "foo";
        const char* string2 = "FOO";
        bool result = StringHelpers::EqualsIgnoreCase(string1, string2);
        EXPECT_TRUE(result);
    }

    TEST(CryCommonToolsStringHelpersTest, EqualsIgnoreCase_FirstStringUpperCase_ReturnsFalse)
    {
        const char* string1 = "FOO";
        const char* string2 = "foo";
        bool result = StringHelpers::EqualsIgnoreCase(string1, string2);
        EXPECT_TRUE(result);
    }

    TEST(CryCommonToolsStringHelpersTest, EqualsIgnoreCase_DifferentStrings_ReturnsFalse)
    {
        const char* string1 = "foo";
        const char* string2 = "foobar";
        bool result = StringHelpers::EqualsIgnoreCase(string1, string2);
        EXPECT_FALSE(result);
    }

    TEST(CryCommonToolsStringHelpersTest, EqualsIgnoreCase_SameTwoWStrings_ReturnsTrue)
    {
        const wchar_t string1[] = L"foo";
        const wchar_t string2[] = L"foo";
        bool result = StringHelpers::EqualsIgnoreCase(string1, string2);
        EXPECT_TRUE(result);
    }

    TEST(CryCommonToolsStringHelpersTest, EqualsIgnoreCase_SecondWStringUpperCase_ReturnsFalse)
    {
        const wchar_t string1[] = L"foo";
        const wchar_t string2[] = L"FOO";
        bool result = StringHelpers::EqualsIgnoreCase(string1, string2);
        EXPECT_TRUE(result);
    }

    TEST(CryCommonToolsStringHelpersTest, EqualsIgnoreCase_FirstWStringUpperCase_ReturnsFalse)
    {
        const wchar_t string1[] = L"FOO";
        const wchar_t string2[] = L"foo";
        bool result = StringHelpers::EqualsIgnoreCase(string1, string2);
        EXPECT_TRUE(result);
    }

    TEST(CryCommonToolsStringHelpersTest, EqualsIgnoreCase_DifferentWStrings_ReturnsFalse)
    {
        const wchar_t string1[] = L"foo";
        const wchar_t string2[] = L"foobar";
        bool result = StringHelpers::EqualsIgnoreCase(string1, string2);
        EXPECT_FALSE(result);
    }

    TEST(CryCommonToolsStringHelpersTest, StartsWith_StringPathsWithLongerPattern_ReturnsFalse)
    {
        const char* string = "foo";
        const char* pattern = "foobar";
        bool result = StringHelpers::StartsWith(string, pattern);
        EXPECT_FALSE(result);
    }

    TEST(CryCommonToolsStringHelpersTest, StartsWith_StringPathAndPattern_ReturnsTrue)
    {
        const char* string = "foobar";
        const char* pattern = "foo";
        bool result = StringHelpers::StartsWith(string, pattern);
        EXPECT_TRUE(result);
    }

    TEST(CryCommonToolsStringHelpersTest, StartsWith_StringPathCapitalized_ReturnsFalse)
    {
        const char* string = "FOOBAR";
        const char* pattern = "foo";
        bool result = StringHelpers::StartsWith(string, pattern);
        EXPECT_FALSE(result);
    }

    TEST(CryCommonToolsStringHelpersTest, StartsWith_PatternStringCapitalized_ReturnsFalse)
    {
        const char* string = "foobar";
        const char* pattern = "FOO";
        bool result = StringHelpers::StartsWith(string, pattern);
        EXPECT_FALSE(result);
    }

    TEST(CryCommonToolsStringHelpersTest, StartsWith_StringPathAndNoMatchingPattern_ReturnsFalse)
    {
        const char* string = "foo";
        const char* pattern = "bar";
        bool result = StringHelpers::StartsWith(string, pattern);
        EXPECT_FALSE(result);
    }

    TEST(CryCommonToolsStringHelpersTest, StartsWith_WStringPathsWithLongerPattern_ReturnsFalse)
    {
        const wchar_t string[] = L"foo";
        const wchar_t pattern[] = L"foobar";
        bool result = StringHelpers::StartsWith(string, pattern);
        EXPECT_FALSE(result);
    }

    TEST(CryCommonToolsStringHelpersTest, StartsWith_WStringPathAndPattern_ReturnsTrue)
    {
        const wchar_t string[] = L"foobar";
        const wchar_t pattern[] = L"foo";
        bool result = StringHelpers::StartsWith(string, pattern);
        EXPECT_TRUE(result);
    }

    TEST(CryCommonToolsStringHelpersTest, StartsWith_WStringPathCapitalized_ReturnsFalse)
    {
        const wchar_t string[] = L"FOOBAR";
        const wchar_t pattern[] = L"foo";
        bool result = StringHelpers::StartsWith(string, pattern);
        EXPECT_FALSE(result);
    }

    TEST(CryCommonToolsStringHelpersTest, StartsWith_PatternWstringCapitalized_ReturnsFalse)
    {
        const wchar_t string[] = L"foobar";
        const wchar_t pattern[] = L"FOO";
        bool result = StringHelpers::StartsWith(string, pattern);
        EXPECT_FALSE(result);
    }

    TEST(CryCommonToolsStringHelpersTest, StartsWith_WStringPathAndNoMatchingPattern_ReturnsFalse)
    {
        const wchar_t string[] = L"foo";
        const wchar_t pattern[] = L"bar";
        bool result = StringHelpers::StartsWith(string, pattern);
        EXPECT_FALSE(result);
    }

    TEST(CryCommonToolsStringHelpersTest, StartsWithIgnoreCase_StringPathsWithLongerPattern_ReturnsFalse)
    {
        const char* string = "foo";
        const char* pattern = "foobar";
        bool result = StringHelpers::StartsWithIgnoreCase(string, pattern);
        EXPECT_FALSE(result);
    }

    TEST(CryCommonToolsStringHelpersTest, StartsWithIgnoreCase_StringPathAndPattern_ReturnsTrue)
    {
        const char* string = "foobar";
        const char* pattern = "foo";
        bool result = StringHelpers::StartsWithIgnoreCase(string, pattern);
        EXPECT_TRUE(result);
    }

    TEST(CryCommonToolsStringHelpersTest, StartsWithIgnoreCase_StringPathCapitalized_ReturnsFalse)
    {
        const char* string = "FOOBAR";
        const char* pattern = "foo";
        bool result = StringHelpers::StartsWithIgnoreCase(string, pattern);
        EXPECT_TRUE(result);
    }

    TEST(CryCommonToolsStringHelpersTest, StartsWithIgnoreCase_PatternStringCapitalized_ReturnsFalse)
    {
        const char* string = "foobar";
        const char* pattern = "FOO";
        bool result = StringHelpers::StartsWithIgnoreCase(string, pattern);
        EXPECT_TRUE(result);
    }

    TEST(CryCommonToolsStringHelpersTest, StartsWithIgnoreCase_StringPathAndNoMatchingPattern_ReturnsFalse)
    {
        const char* string = "foo";
        const char* pattern = "bar";
        bool result = StringHelpers::StartsWithIgnoreCase(string, pattern);
        EXPECT_FALSE(result);
    }

    TEST(CryCommonToolsStringHelpersTest, StartsWithIgnoreCase_WStringPathsWithLongerPattern_ReturnsFalse)
    {
        const wchar_t string[] = L"foo";
        const wchar_t pattern[] = L"foobar";
        bool result = StringHelpers::StartsWithIgnoreCase(string, pattern);
        EXPECT_FALSE(result);
    }

    TEST(CryCommonToolsStringHelpersTest, StartsWithIgnoreCase_WStringPathAndPattern_ReturnsTrue)
    {
        const wchar_t string[] = L"foobar";
        const wchar_t pattern[] = L"foo";
        bool result = StringHelpers::StartsWithIgnoreCase(string, pattern);
        EXPECT_TRUE(result);
    }

    TEST(CryCommonToolsStringHelpersTest, StartsWithIgnoreCase_WStringPathCapitalized_ReturnsFalse)
    {
        const wchar_t string[] = L"FOOBAR";
        const wchar_t pattern[] = L"foo";
        bool result = StringHelpers::StartsWithIgnoreCase(string, pattern);
        EXPECT_TRUE(result);
    }

    TEST(CryCommonToolsStringHelpersTest, StartsWithIgnoreCase_PatternWStringCapitalized_ReturnsFalse)
    {
        const wchar_t string[] = L"foobar";
        const wchar_t pattern[] = L"FOO";
        bool result = StringHelpers::StartsWithIgnoreCase(string, pattern);
        EXPECT_TRUE(result);
    }

    TEST(CryCommonToolsStringHelpersTest, StartsWithIgnoreCase_WStringPathAndNoMatchingPattern_ReturnsFalse)
    {
        const wchar_t string[] = L"foo";
        const wchar_t pattern[] = L"bar";
        bool result = StringHelpers::StartsWithIgnoreCase(string, pattern);
        EXPECT_FALSE(result);
    }

    TEST(CryCommonToolsStringHelpersTest, EndsWith_StringPathWithLongerPattern_ReturnsFalse)
    {
        const char* string = "foo";
        const char* pattern = "foobar";
        bool result = StringHelpers::EndsWith(string, pattern);
        EXPECT_FALSE(result);
    }

    TEST(CryCommonToolsStringHelpersTest, EndsWith_StringPathAndPattern_ReturnsTrue)
    {
        const char* string = "foobar";
        const char* pattern = "bar";
        bool result = StringHelpers::EndsWith(string, pattern);
        EXPECT_TRUE(result);
    }

    TEST(CryCommonToolsStringHelpersTest, EndsWith_StringPathCapitalized_ReturnsFalse)
    {
        const char* string = "FOOBAR";
        const char* pattern = "bar";
        bool result = StringHelpers::EndsWith(string, pattern);
        EXPECT_FALSE(result);
    }

    TEST(CryCommonToolsStringHelpersTest, EndsWith_PatternStringCapitalized_ReturnsFalse)
    {
        const char* string = "foobar";
        const char* pattern = "BAR";
        bool result = StringHelpers::EndsWith(string, pattern);
        EXPECT_FALSE(result);
    }

    TEST(CryCommonToolsStringHelpersTest, EndsWith_StringPathAndNoMatchingPattern_ReturnsFalse)
    {
        const char* string = "foo";
        const char* pattern = "bar";
        bool result = StringHelpers::EndsWithIgnoreCase(string, pattern);
        EXPECT_FALSE(result);
    }

    TEST(CryCommonToolsStringHelpersTest, EndsWith_WStringPathWithLongerPattern_ReturnsFalse)
    {
        const wchar_t string[] = L"foo";
        const wchar_t pattern[] = L"foobar";
        bool result = StringHelpers::EndsWith(string, pattern);
        EXPECT_FALSE(result);
    }

    TEST(CryCommonToolsStringHelpersTest, EndsWith_WStringPathAndPattern_ReturnsTrue)
    {
        const wchar_t string[] = L"foobar";
        const wchar_t pattern[] = L"bar";
        bool result = StringHelpers::EndsWith(string, pattern);
        EXPECT_TRUE(result);
    }

    TEST(CryCommonToolsStringHelpersTest, EndsWith_WStringPathCapitalized_ReturnsFalse)
    {
        const wchar_t string[] = L"FOOBAR";
        const wchar_t pattern[] = L"bar";
        bool result = StringHelpers::EndsWith(string, pattern);
        EXPECT_FALSE(result);
    }

    TEST(CryCommonToolsStringHelpersTest, EndsWith_PatternWStringCapitalized_ReturnsFalse)
    {
        const wchar_t string[] = L"foobar";
        const wchar_t pattern[] = L"BAR";
        bool result = StringHelpers::EndsWith(string, pattern);
        EXPECT_FALSE(result);
    }

    TEST(CryCommonToolsStringHelpersTest, EndsWith_WStringPathAndNoMatchingPattern_ReturnsFalse)
    {
        const wchar_t string[] = L"foo";
        const wchar_t pattern[] = L"bar";
        bool result = StringHelpers::EndsWithIgnoreCase(string, pattern);
        EXPECT_FALSE(result);
    }

    TEST(CryCommonToolsStringHelpersTest, EndsWithIgnoreCase_StringPathWithLongerPattern_ReturnsFalse)
    {
        const char* string = "foo";
        const char* pattern = "foobar";
        bool result = StringHelpers::EndsWithIgnoreCase(string, pattern);
        EXPECT_FALSE(result);
    }

    TEST(CryCommonToolsStringHelpersTest, EndsWithIgnoreCase_StringPathAndPattern_ReturnsTrue)
    {
        const char* string = "foobar";
        const char* pattern = "bar";
        bool result = StringHelpers::EndsWithIgnoreCase(string, pattern);
        EXPECT_TRUE(result);
    }

    TEST(CryCommonToolsStringHelpersTest, EndsWithIgnoreCase_StringPathCapitalized_ReturnsTrue)
    {
        const char* string = "FOOBAR";
        const char* pattern = "bar";
        bool result = StringHelpers::EndsWithIgnoreCase(string, pattern);
        EXPECT_TRUE(result);
    }

    TEST(CryCommonToolsStringHelpersTest, EndsWithIgnoreCase_PatternStringCapitalized_ReturnsTrue)
    {
        const char* string = "foobar";
        const char* pattern = "BAR";
        bool result = StringHelpers::EndsWithIgnoreCase(string, pattern);
        EXPECT_TRUE(result);
    }

    TEST(CryCommonToolsStringHelpersTest, EndsWithIgnoreCase_StringPathAndNoMatchingPattern_ReturnsFalse)
    {
        const char* string = "foo";
        const char* pattern = "bar";
        bool result = StringHelpers::EndsWithIgnoreCase(string, pattern);
        EXPECT_FALSE(result);
    }

    TEST(CryCommonToolsStringHelpersTest, EndsWithIgnoreCase_WStringPathWithLongerPattern_ReturnsFalse)
    {
        const wchar_t string[] = L"foo";
        const wchar_t pattern[] = L"foobar";
        bool result = StringHelpers::EndsWithIgnoreCase(string, pattern);
        EXPECT_FALSE(result);
    }

    TEST(CryCommonToolsStringHelpersTest, EndsWithIgnoreCase_WStringPathAndPattern_ReturnsTrue)
    {
        const wchar_t string[] = L"foobar";
        const wchar_t pattern[] = L"bar";
        bool result = StringHelpers::EndsWithIgnoreCase(string, pattern);
        EXPECT_TRUE(result);
    }

    TEST(CryCommonToolsStringHelpersTest, EndsWithIgnoreCase_WStringPathCapitalized_ReturnsTrue)
    {
        const wchar_t string[] = L"FOOBAR";
        const wchar_t pattern[] = L"bar";
        bool result = StringHelpers::EndsWithIgnoreCase(string, pattern);
        EXPECT_TRUE(result);
    }

    TEST(CryCommonToolsStringHelpersTest, EndsWithIgnoreCase_PatternWStringCapitalized_ReturnsTrue)
    {
        const wchar_t string[] = L"foobar";
        const wchar_t pattern[] = L"BAR";
        bool result = StringHelpers::EndsWithIgnoreCase(string, pattern);
        EXPECT_TRUE(result);
    }

    TEST(CryCommonToolsStringHelpersTest, EndsWithIgnoreCase_WStringPathAndNoMatchingPattern_ReturnsFalse)
    {
        const wchar_t string[] = L"foo";
        const wchar_t pattern[] = L"bar";
        bool result = StringHelpers::EndsWithIgnoreCase(string, pattern);
        EXPECT_FALSE(result);
    }

    TEST(CryCommonToolsStringHelpersTest, Contains_StringPathWithLongerPattern_ReturnsFalse)
    {
        const char* string = "foo";
        const char* pattern = "barbar";
        bool result = StringHelpers::Contains(string, pattern);
        EXPECT_FALSE(result);
    }

    TEST(CryCommonToolsStringHelpersTest, Contains_StringPathAndPattern_ReturnsTrue)
    {
        const char* string = "foobarfoo";
        const char* pattern = "bar";
        bool result = StringHelpers::Contains(string, pattern);
        EXPECT_TRUE(result);
    }

    TEST(CryCommonToolsStringHelpersTest, Contains_PatternStringCapitalized_ReturnsFalse)
    {
        const char* string = "foobarfoo";
        const char* pattern = "BAR";
        bool result = StringHelpers::Contains(string, pattern);
        EXPECT_FALSE(result);
    }

    TEST(CryCommonToolsStringHelpersTest, Contains_StringPathCapitalized_ReturnsFalse)
    {
        const char* string = "FOOBARFOO";
        const char* pattern = "bar";
        bool result = StringHelpers::Contains(string, pattern);
        EXPECT_FALSE(result);
    }

    TEST(CryCommonToolsStringHelpersTest, Contains_StringPathNoMatchingPattern_ReturnsFalse)
    {
        const char* string = "foofoofoo";
        const char* pattern = "bar";
        bool result = StringHelpers::Contains(string, pattern);
        EXPECT_FALSE(result);
    }

    TEST(CryCommonToolsStringHelpersTest, Contains_WStringPathWithLongerPattern_ReturnsFalse)
    {
        const wchar_t string[] = L"foo";
        const wchar_t pattern[] = L"barbar";
        bool result = StringHelpers::Contains(string, pattern);
        EXPECT_FALSE(result);
    }

    TEST(CryCommonToolsStringHelpersTest, Contains_WStringPathAndPattern_ReturnsTrue)
    {
        const wchar_t string[] = L"foobarfoo";
        const wchar_t pattern[] = L"bar";
        bool result = StringHelpers::Contains(string, pattern);
        EXPECT_TRUE(result);
    }

    TEST(CryCommonToolsStringHelpersTest, Contains_PatternWStringCapitalized_ReturnsFalse)
    {
        const wchar_t string[] = L"foobarfoo";
        const wchar_t pattern[] = L"BAR";
        bool result = StringHelpers::Contains(string, pattern);
        EXPECT_FALSE(result);
    }

    TEST(CryCommonToolsStringHelpersTest, Contains_WStringPathCapitalized_ReturnsFalse)
    {
        const wchar_t string[] = L"FOOBARFOO";
        const wchar_t pattern[] = L"bar";
        bool result = StringHelpers::Contains(string, pattern);
        EXPECT_FALSE(result);
    }

    TEST(CryCommonToolsStringHelpersTest, Contains_WStringPathNoMatchingPattern_ReturnsFalse)
    {
        const wchar_t string[] = L"foofoofoo";
        const wchar_t pattern[] = L"bar";
        bool result = StringHelpers::Contains(string, pattern);
        EXPECT_FALSE(result);
    }

    TEST(CryCommonToolsStringHelpersTest, ContainsIgnoreCase_StringPathWithLongerPattern_ReturnsFalse)
    {
        const char* string = "foo";
        const char* pattern = "barbar";
        bool result = StringHelpers::ContainsIgnoreCase(string, pattern);
        EXPECT_FALSE(result);
    }

    TEST(CryCommonToolsStringHelpersTest, ContainsIgnoreCase_StringPathAndPattern_ReturnsTrue)
    {
        const char* string = "foobarfoo";
        const char* pattern = "bar";
        bool result = StringHelpers::ContainsIgnoreCase(string, pattern);
        EXPECT_TRUE(result);
    }

    TEST(CryCommonToolsStringHelpersTest, ContainsIgnoreCase_PatternStringCapitalized_ReturnsTrue)
    {
        const char* string = "foobarfoo";
        const char* pattern = "BAR";
        bool result = StringHelpers::ContainsIgnoreCase(string, pattern);
        EXPECT_TRUE(result);
    }

    TEST(CryCommonToolsStringHelpersTest, ContainsIgnoreCase_StringPathCapitalized_ReturnsTrue)
    {
        const char* string = "FOOBARFOO";
        const char* pattern = "bar";
        bool result = StringHelpers::ContainsIgnoreCase(string, pattern);
        EXPECT_TRUE(result);
    }

    TEST(CryCommonToolsStringHelpersTest, ContainsIgnoreCase_StringPathNoMatchingPattern_ReturnsFalse)
    {
        const char* string = "foofoofoo";
        const char* pattern = "bar";
        bool result = StringHelpers::ContainsIgnoreCase(string, pattern);
        EXPECT_FALSE(result);
    }

    TEST(CryCommonToolsStringHelpersTest, ContainsIgnoreCase_WStringPathWithLongerPattern_ReturnsFalse)
    {
        const wchar_t string[] = L"foo";
        const wchar_t pattern[] = L"barbar";
        bool result = StringHelpers::ContainsIgnoreCase(string, pattern);
        EXPECT_FALSE(result);
    }

    TEST(CryCommonToolsStringHelpersTest, ContainsIgnoreCase_WStringPathAndPattern_ReturnsTrue)
    {
        const wchar_t string[] = L"foobarfoo";
        const wchar_t pattern[] = L"bar";
        bool result = StringHelpers::ContainsIgnoreCase(string, pattern);
        EXPECT_TRUE(result);
    }

    TEST(CryCommonToolsStringHelpersTest, ContainsIgnoreCase_PatternWStringCapitalized_ReturnsTrue)
    {
        const wchar_t string[] = L"foobarfoo";
        const wchar_t pattern[] = L"BAR";
        bool result = StringHelpers::ContainsIgnoreCase(string, pattern);
        EXPECT_TRUE(result);
    }

    TEST(CryCommonToolsStringHelpersTest, ContainsIgnoreCase_WStringPathCapitalized_ReturnsTrue)
    {
        const wchar_t string[] = L"FOOBARFOO";
        const wchar_t pattern[] = L"bar";
        bool result = StringHelpers::ContainsIgnoreCase(string, pattern);
        EXPECT_TRUE(result);
    }

    TEST(CryCommonToolsStringHelpersTest, ContainsIgnoreCase_WStringPathNoMatchingPattern_ReturnsFalse)
    {
        const wchar_t string[] = L"foofoofoo";
        const wchar_t pattern[] = L"bar";
        bool result = StringHelpers::ContainsIgnoreCase(string, pattern);
        EXPECT_FALSE(result);
    }

    TEST(CryCommonToolsStringHelpersTest, MatchesWildcards_StringMatchingPattern_ReturnsTrue)
    {
        const char* string = "foo";
        const char* wildcard = "f*o";
        bool result = StringHelpers::MatchesWildcards(string, wildcard);
        EXPECT_TRUE(result);
    }

    TEST(CryCommonToolsStringHelpersTest, MatchesWildcards_PatternStringCapitalized_ReturnsFalse)
    {
        const char* string = "foo";
        const char* wildcard = "F*O";
        bool result = StringHelpers::MatchesWildcards(string, wildcard);
        EXPECT_FALSE(result);
    }

    TEST(CryCommonToolsStringHelpersTest, MatchesWildcards_StringCapitalized_ReturnsFalse)
    {
        const char* string = "FOO";
        const char* wildcard = "f*o";
        bool result = StringHelpers::MatchesWildcards(string, wildcard);
        EXPECT_FALSE(result);
    }

    TEST(CryCommonToolsStringHelpersTest, MatchesWildcards_StringNoMatchingPatternWithAsterisk_ReturnsFalse)
    {
        const char* string = "foo";
        const char* wildcard = "f*r";
        bool result = StringHelpers::MatchesWildcards(string, wildcard);
        EXPECT_FALSE(result);
    }

    TEST(CryCommonToolsStringHelpersTest, MatchesWildcards_StringNoMatchingPattern_ReturnsFalse)
    {
        const char* string = "foobar";
        const char* wildcard = "foo";
        bool result = StringHelpers::MatchesWildcards(string, wildcard);
        EXPECT_FALSE(result);
    }

    TEST(CryCommonToolsStringHelpersTest, MatchesWildcards_WStringMatchingPattern_ReturnsTrue)
    {
        const wchar_t string[] = L"foo";
        const wchar_t wildcard[] = L"f*o";
        bool result = StringHelpers::MatchesWildcards(string, wildcard);
        EXPECT_TRUE(result);
    }

    TEST(CryCommonToolsStringHelpersTest, MatchesWildcards_WPatternStringCapitalized_ReturnsFalse)
    {
        const wchar_t string[] = L"foo";
        const wchar_t wildcard[] = L"F*O";
        bool result = StringHelpers::MatchesWildcards(string, wildcard);
        EXPECT_FALSE(result);
    }

    TEST(CryCommonToolsStringHelpersTest, MatchesWildcards_WStringCapitalized_ReturnsFalse)
    {
        const wchar_t string[] = L"FOO";
        const wchar_t wildcard[] = L"f*o";
        bool result = StringHelpers::MatchesWildcards(string, wildcard);
        EXPECT_FALSE(result);
    }

    TEST(CryCommonToolsStringHelpersTest, MatchesWildcards_WStringNoMatchingPatternWithAsterisk_ReturnsFalse)
    {
        const wchar_t string[] = L"foo";
        const wchar_t wildcard[] = L"f*r";
        bool result = StringHelpers::MatchesWildcards(string, wildcard);
        EXPECT_FALSE(result);
    }

    TEST(CryCommonToolsStringHelpersTest, MatchesWildcards_WStringNoMatchingPattern_ReturnsFalse)
    {
        const wchar_t string[] = L"foobar";
        const wchar_t wildcard[] = L"foo";
        bool result = StringHelpers::MatchesWildcards(string, wildcard);
        EXPECT_FALSE(result);
    }

    TEST(CryCommonToolsStringHelpersTest, MatchesWildcardsIgnoreCase_StringMatchingPattern_ReturnsTrue)
    {
        const char* string = "foo";
        const char* wildcard = "f*o";
        bool result = StringHelpers::MatchesWildcardsIgnoreCase(string, wildcard);
        EXPECT_TRUE(result);
    }

    TEST(CryCommonToolsStringHelpersTest, MatchesWildcardsIgnoreCase_PatternStringCapitalized_ReturnsTrue)
    {
        const char* string = "foo";
        const char* wildcard = "F*O";
        bool result = StringHelpers::MatchesWildcardsIgnoreCase(string, wildcard);
        EXPECT_TRUE(result);
    }

    TEST(CryCommonToolsStringHelpersTest, MatchesWildcardsIgnoreCase_StringCapitalized_ReturnsTrue)
    {
        const char* string = "FOO";
        const char* wildcard = "f*o";
        bool result = StringHelpers::MatchesWildcardsIgnoreCase(string, wildcard);
        EXPECT_TRUE(result);
    }

    TEST(CryCommonToolsStringHelpersTest, MatchesWildcardsIgnoreCase_StringNoMatchingPatternWithAsterisk_ReturnsFalse)
    {
        const char* string = "foo";
        const char* wildcard = "f*r";
        bool result = StringHelpers::MatchesWildcardsIgnoreCase(string, wildcard);
        EXPECT_FALSE(result);
    }

    TEST(CryCommonToolsStringHelpersTest, MatchesWildcardsIgnoreCase_StringNoMatchingPattern_ReturnsFalse)
    {
        const char* string = "foobar";
        const char* wildcard = "foo";
        bool result = StringHelpers::MatchesWildcardsIgnoreCase(string, wildcard);
        EXPECT_FALSE(result);
    }

    TEST(CryCommonToolsStringHelpersTest, MatchesWildcardsIgnoreCase_WStringMatchingPattern_ReturnsTrue)
    {
        const wchar_t string[] = L"foo";
        const wchar_t wildcard[] = L"f*o";
        bool result = StringHelpers::MatchesWildcardsIgnoreCase(string, wildcard);
        EXPECT_TRUE(result);
    }

    TEST(CryCommonToolsStringHelpersTest, MatchesWildcardsIgnoreCase_PatternWStringCapitalized_ReturnsTrue)
    {
        const wchar_t string[] = L"foo";
        const wchar_t wildcard[] = L"F*O";
        bool result = StringHelpers::MatchesWildcardsIgnoreCase(string, wildcard);
        EXPECT_TRUE(result);
    }

    TEST(CryCommonToolsStringHelpersTest, MatchesWildcardsIgnoreCase_WStringCapitalized_ReturnsTrue)
    {
        const wchar_t string[] = L"FOO";
        const wchar_t wildcard[] = L"f*o";
        bool result = StringHelpers::MatchesWildcardsIgnoreCase(string, wildcard);
        EXPECT_TRUE(result);
    }

    TEST(CryCommonToolsStringHelpersTest, MatchesWildcardsIgnoreCase_WStringNoMatchingPatternWithAsterisk_ReturnsFalse)
    {
        const wchar_t string[] = L"foo";
        const wchar_t wildcard[] = L"f*r";
        bool result = StringHelpers::MatchesWildcardsIgnoreCase(string, wildcard);
        EXPECT_FALSE(result);
    }

    TEST(CryCommonToolsStringHelpersTest, MatchesWildcardsIgnoreCase_WStringNoMatchingPattern_ReturnsFalse)
    {
        const wchar_t string[] = L"foobar";
        const wchar_t wildcard[] = L"foo";
        bool result = StringHelpers::MatchesWildcardsIgnoreCase(string, wildcard);
        EXPECT_FALSE(result);
    }

    TEST(CryCommonToolsStringHelpersTest, TrimLeft_StringWithoutReturnOrTab_ReturnsString)
    {
        const char* stringInput = "foo";
        string result = StringHelpers::TrimLeft(stringInput);
        EXPECT_STREQ(stringInput, result);
    }

    TEST(CryCommonToolsStringHelpersTest, TrimRight_StringWithoutReturnOrTab_ReturnsString)
    {
        const char* stringInput = "foo";
        string result = StringHelpers::TrimRight(stringInput);
        EXPECT_STREQ(stringInput, result);
    }

    TEST(CryCommonToolsStringHelpersTest, MakeLowerCase_UpperCaseString_ReturnsLowerCaseString)
    {
        const char* stringInput = "FOO";
        string result = StringHelpers::MakeLowerCase(stringInput);
        EXPECT_STREQ("foo", result);
    }

    TEST(CryCommonToolsStringHelpersTest, MakeLowerCase_UpperCaseWString_ReturnsLowerCaseString)
    {
        const wchar_t stringInput[] = L"FOO";
        const wchar_t expectedString[] = L"foo";
        wstring result = StringHelpers::MakeLowerCase(stringInput);
        EXPECT_TRUE(result == expectedString);
    }

    TEST(CryCommonToolsStringHelpersTest, MakeUpperCase_LowerCaseString_ReturnsUpperCaseString)
    {
        const char* stringInput = "foo";
        string result = StringHelpers::MakeUpperCase(stringInput);
        EXPECT_STREQ("FOO", result);
    }

    TEST(CryCommonToolsStringHelpersTest, MakeUpperCase_LowerCaseWString_ReturnsUpperCaseString)
    {
        const wchar_t stringInput[] = L"foo";
        const wchar_t expectedString[] = L"FOO";
        wstring result = StringHelpers::MakeUpperCase(stringInput);
        EXPECT_TRUE(result == expectedString);
    }

    TEST(CryCommonToolsStringHelpersTest, Replace_CharacterToReplace_ReturnsStringWithReplacedCharacters)
    {
        const char* stringInput = "foo";
        char oldChar = 'o';
        char newChar = 'i';
        string result = StringHelpers::Replace(stringInput, oldChar, newChar);
        EXPECT_STREQ("fii", result);
    }

    TEST(CryCommonToolsStringHelpersTest, Replace_CharacterToReplace_ReturnsWStringWithReplacedCharacters)
    {
        const wchar_t stringInput[] = L"foo";
        wchar_t oldChar = 'o';
        wchar_t newChar = 'i';
        const wchar_t expectedString[] = L"fii";
        wstring result = StringHelpers::Replace(stringInput, oldChar, newChar);
        EXPECT_TRUE(result == expectedString);
    }

    TEST(CryCommonToolsStringHelpersTest, Replace_CharacterToReplaceNotInString_ReturnsOriginalString)
    {
        const char* stringInput = "foo";
        char oldChar = 'a';
        char newChar = 'i';
        string result = StringHelpers::Replace(stringInput, oldChar, newChar);
        EXPECT_STREQ(stringInput, result);
    }

    TEST(CryCommonToolsStringHelpersTest, Replace_CharacterToReplaceNotInWString_ReturnsOriginalWString)
    {
        const wchar_t stringInput[] = L"foo";
        wchar_t oldChar = 'a';
        wchar_t newChar = 'i';
        wstring result = StringHelpers::Replace(stringInput, oldChar, newChar);
        EXPECT_TRUE(result == stringInput);
    }
}
