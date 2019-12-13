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

#include <gtest/gtest-param-test.h>

#include <AzCore/UnitTest/TestTypes.h>
#include <AzFramework/StringFunc/StringFunc.h>

namespace AzFramework
{
    using namespace UnitTest;

    using StringFuncTest = AllocatorsFixture;

    // Strip out any trailing path separators
    

    TEST_F(StringFuncTest, Strip_ValidInputExtraEndingPathSeparators_Success)
    {
        AZStd::string       samplePath = "F:\\w s 1\\dev\\/";
        AZStd::string       expectedResult = "F:\\w s 1\\dev";
        const char*         stripCharacters = AZ_CORRECT_FILESYSTEM_SEPARATOR_STRING  AZ_WRONG_FILESYSTEM_SEPARATOR_STRING;

        AzFramework::StringFunc::Strip(samplePath, stripCharacters, false, false, true);

        ASSERT_TRUE(samplePath == expectedResult);
    }
    
    TEST_F(StringFuncTest, Strip_AllEmptyResult1_Success)
    {
        AZStd::string input = "aa";
        const char stripToken = 'a';

        AzFramework::StringFunc::Strip(input, stripToken);

        ASSERT_TRUE(input.empty());
    }

    TEST_F(StringFuncTest, Strip_AllEmptyResult2_Success)
    {
        AZStd::string input = "aaaa";
        const char stripToken = 'a';

        AzFramework::StringFunc::Strip(input, stripToken);

        ASSERT_TRUE(input.empty());
    }

    TEST_F(StringFuncTest, Strip_BeginEndCaseSensitiveEmptyResult1_Success)
    {
        AZStd::string input = "aa";
        const char stripToken = 'a';

        AzFramework::StringFunc::Strip(input, stripToken, true, true, true);

        ASSERT_TRUE(input.empty());
    }

    TEST_F(StringFuncTest, Strip_BeginEndCaseSensitiveEmptyResult2_Success)
    {
        AZStd::string input = "aaaa";
        AZStd::string expectedResult = "aa";
        const char stripToken = 'a';

        AzFramework::StringFunc::Strip(input, stripToken, true, true, true);

        ASSERT_EQ(input, expectedResult);
    }

    TEST_F(StringFuncTest, Strip_BeginEndCaseInsensitiveEmptyResult1_Success)
    {
        AZStd::string input = "aa";
        const char stripToken = 'a';

        AzFramework::StringFunc::Strip(input, stripToken, false, true, true);

        ASSERT_TRUE(input.empty());
    }

    TEST_F(StringFuncTest, Strip_BeginEndCaseInsensitiveEmptyResult2_Success)
    {
        AZStd::string input = "aaaa";
        AZStd::string expectedResult = "aa";
        const char stripToken = 'a';

        AzFramework::StringFunc::Strip(input, stripToken, false, true, true);

        ASSERT_EQ(input, expectedResult);
    }

    TEST_F(StringFuncTest, GroupDigits_BasicFunctionality)
    {
        // Test a bunch of numbers and other inputs
        const AZStd::pair<AZStd::string, AZStd::string> inputsAndExpectedOutputs[] = 
        {
            { "0", "0" },
            { "10", "10" },
            { "100", "100" },
            { "1000", "1,000" },
            { "10000", "10,000" },
            { "100000", "100,000" },
            { "1000000", "1,000,000" },
            { "0.0", "0.0"},
            { "10.0", "10.0"},
            { "100.0", "100.0" },
            { "1000.0", "1,000.0" },
            { "10000.0", "10,000.0" },
            { "100000.0", "100,000.0" },
            { "1000000.0", "1,000,000.0" },
            { "-0.0", "-0.0" },
            { "-10.0", "-10.0" },
            { "-100.0", "-100.0" },
            { "-1000.0", "-1,000.0" },
            { "-10000.0", "-10,000.0" },
            { "-100000.0", "-100,000.0" },
            { "-1000000.0", "-1,000,000.0" },
            { "foo", "foo" },
            { "foo123.0", "foo123.0" },
            { "foo1234.0", "foo1,234.0" }
        };

        static const size_t BUFFER_SIZE = 32;
        char buffer[BUFFER_SIZE];

        for (const auto& inputAndExpectedOutput : inputsAndExpectedOutputs)
        {
            azstrncpy(buffer, BUFFER_SIZE, inputAndExpectedOutput.first.c_str(), inputAndExpectedOutput.first.length());
            auto endPos = StringFunc::NumberFormatting::GroupDigits(buffer, BUFFER_SIZE);
            EXPECT_STREQ(buffer, inputAndExpectedOutput.second.c_str());
            EXPECT_EQ(inputAndExpectedOutput.second.length(), endPos);
        }

        // Test valid inputs
        AZ_TEST_START_ASSERTTEST;
        StringFunc::NumberFormatting::GroupDigits(nullptr, 0);  // Should assert twice, for null pointer and 0 being <= decimalPosHint
        StringFunc::NumberFormatting::GroupDigits(buffer, BUFFER_SIZE, 0, ',', '.', 0, 0);  // Should assert for having a non-positive grouping size
        AZ_TEST_STOP_ASSERTTEST(3);

        // Test buffer overruns
        static const size_t SMALL_BUFFER_SIZE = 8;
        char smallBuffer[SMALL_BUFFER_SIZE];
        char prevSmallBuffer[SMALL_BUFFER_SIZE];

        for (const auto& inputAndExpectedOutput : inputsAndExpectedOutputs)
        {
            azstrncpy(smallBuffer, SMALL_BUFFER_SIZE, inputAndExpectedOutput.first.c_str(), AZStd::min(inputAndExpectedOutput.first.length(), SMALL_BUFFER_SIZE - 1));
            smallBuffer[SMALL_BUFFER_SIZE - 1] = 0;  // Force null-termination
            memcpy(prevSmallBuffer, smallBuffer, SMALL_BUFFER_SIZE);
            auto endPos = StringFunc::NumberFormatting::GroupDigits(smallBuffer, SMALL_BUFFER_SIZE);

            if (inputAndExpectedOutput.second.length() >= SMALL_BUFFER_SIZE)
            {
                EXPECT_STREQ(smallBuffer, prevSmallBuffer);  // No change if buffer overruns
            }
            else
            {
                EXPECT_STREQ(smallBuffer, inputAndExpectedOutput.second.c_str());
                EXPECT_EQ(inputAndExpectedOutput.second.length(), endPos);
            }
        }
    }

    TEST_F(StringFuncTest, GroupDigits_DifferentSettings)
    {
        struct TestSettings
        {
            AZStd::string m_input;
            AZStd::string m_output;
            size_t m_decimalPosHint;
            char m_digitSeparator;
            char m_decimalSeparator;
            int m_groupingSize;
            int m_firstGroupingSize;
        };

        const TestSettings testSettings[] =
        {
            { "123456789.0123", "123,456,789.0123", 9, ',', '.', 3, 0 },  // Correct decimal position hint
            { "123456789.0123", "123,456,789.0123", 8, ',', '.', 3, 0 },  // Incorrect decimal position hint
            { "123456789,0123", "123.456.789,0123", 0, '.', ',', 3, 0 },  // Swap glyphs used for decimal and grouping
            { "123456789.0123", "1,23,45,67,89.0123", 8, ',', '.', 2, 0 },  // Different grouping size
            { "123456789.0123", "12,34,56,789.0123", 8, ',', '.', 2, 3 },  // Customized grouping size for first group
        };

        static const size_t BUFFER_SIZE = 32;
        char buffer[BUFFER_SIZE];

        for (const auto& settings : testSettings)
        {
            azstrncpy(buffer, BUFFER_SIZE, settings.m_input.c_str(), settings.m_input.length());
            auto endPos = StringFunc::NumberFormatting::GroupDigits(buffer, BUFFER_SIZE, settings.m_decimalPosHint, settings.m_digitSeparator, 
                settings.m_decimalSeparator, settings.m_groupingSize, settings.m_firstGroupingSize);
            EXPECT_STREQ(buffer, settings.m_output.c_str());
            EXPECT_EQ(settings.m_output.length(), endPos);
        }
    }

    TEST_F(StringFuncTest, CalculateBranchToken_ValidInput_Success)
    {
        AZStd::string       samplePath = "F:\\w s 1\\dev";
        AZStd::string       expectedToken = "0x68E564C5";
        AZStd::string       resultToken;

        AzFramework::StringFunc::AssetPath::CalculateBranchToken(samplePath, resultToken);

        ASSERT_TRUE(resultToken == expectedToken);
    }

    TEST_F(StringFuncTest, CalculateBranchToken_ValidInputExtra_Success)
    {
        AZStd::string       samplePath = "F:\\w s 1\\dev\\";
        AZStd::string       expectedToken = "0x68E564C5";
        AZStd::string       resultToken;

        AzFramework::StringFunc::AssetPath::CalculateBranchToken(samplePath, resultToken);

        ASSERT_TRUE(resultToken == expectedToken);
    }

    TEST_F(StringFuncTest, HasDrive_EmptyInput_NoDriveFound)
    {
        EXPECT_FALSE(AzFramework::StringFunc::Path::HasDrive(""));
    }

    TEST_F(StringFuncTest, HasDrive_InputContainsDrive_DriveFound)
    {
        AZStd::string input;

#if AZ_TRAIT_OS_USE_WINDOWS_FILE_PATHS
        input = "F:\\test\\to\\get\\drive\\";
#else
        input = "/test/to/get/drive/";
#endif

        EXPECT_TRUE(AzFramework::StringFunc::Path::HasDrive(input.c_str()));
    }

    TEST_F(StringFuncTest, HasDrive_InputDoesNotContainDrive_NoDriveFound)
    {
        AZStd::string input;

#if AZ_TRAIT_OS_USE_WINDOWS_FILE_PATHS
        input = "test\\with\\no\\drive\\";
#else
        input = "test/with/no/drive/";
#endif

        EXPECT_FALSE(AzFramework::StringFunc::Path::HasDrive(input.c_str()));
    }

    TEST_F(StringFuncTest, HasDrive_CheckAllFileSystemFormats_InputContainsDrive_DriveFound)
    {
        AZStd::string input1 = "F:\\test\\to\\get\\drive\\";
        AZStd::string input2 = "/test/to/get/drive/";

        EXPECT_TRUE(AzFramework::StringFunc::Path::HasDrive(input1.c_str(), true));
        EXPECT_TRUE(AzFramework::StringFunc::Path::HasDrive(input2.c_str(), true));
    }

    TEST_F(StringFuncTest, HasDrive_CheckAllFileSystemFormats_InputDoesNotContainDrive_NoDriveFound)
    {
        AZStd::string input1 = "test\\with\\no\\drive\\";
        AZStd::string input2 = "test/with/no/drive/";

        EXPECT_FALSE(AzFramework::StringFunc::Path::HasDrive(input1.c_str(), true));
        EXPECT_FALSE(AzFramework::StringFunc::Path::HasDrive(input2.c_str(), true));
    }

    TEST_F(StringFuncTest, GetDrive_UseSameStringForInOut_Success)
    {
#if AZ_TRAIT_OS_USE_WINDOWS_FILE_PATHS
        AZStd::string input = "F:\\test\\to\\get\\drive\\";
        AZStd::string expectedDriveResult = "F:";
#else
        AZStd::string input = "/test/to/get/drive/";
        AZStd::string expectedDriveResult = "";
#endif //AZ_TRAIT_OS_USE_WINDOWS_FILE_PATHS

        bool result = AzFramework::StringFunc::Path::GetDrive(input.c_str(), input);

        ASSERT_TRUE(result);
        ASSERT_EQ(input, expectedDriveResult);
    }

    //! Strip

    TEST_F(StringFuncTest, Strip_InternalCharactersAll_Success)
    {
        AZStd::string input = "Hello World";
        AZStd::string expectedResult = "Heo Word";
        const char stripToken = 'l';

        AzFramework::StringFunc::Strip(input, stripToken);

        ASSERT_EQ(input, expectedResult);
    }

    TEST_F(StringFuncTest, Strip_InternalCharactersBeginning_NoChange)
    {
        AZStd::string input = "Hello World";
        AZStd::string expectedResult = "Hello World";
        const char stripToken = 'l';

        AzFramework::StringFunc::Strip(input, stripToken, false, true);

        ASSERT_EQ(input, expectedResult);
    }

    TEST_F(StringFuncTest, Strip_InternalCharactersEnd_NoChange)
    {
        AZStd::string input = "Hello World";
        AZStd::string expectedResult = "Hello World";
        const char stripToken = 'l';

        AzFramework::StringFunc::Strip(input, stripToken, false, false, true);

        ASSERT_EQ(input, expectedResult);
    }

    TEST_F(StringFuncTest, Strip_InternalCharacterBeginningEnd_NoChange)
    {
        AZStd::string input = "Hello World";
        AZStd::string expectedResult = "Hello World";
        const char stripToken = 'l';

        AzFramework::StringFunc::Strip(input, stripToken, false, true, true);

        ASSERT_EQ(input, expectedResult);
    }

    TEST_F(StringFuncTest, Strip_InternalCharactersBeginningEndInsensitive_NoChange)
    {
        AZStd::string input = "HeLlo HeLlo HELlO";
        AZStd::string expectedResult = "HeLlo HeLlo HELlO";
        const char stripToken = 'l';

        AzFramework::StringFunc::Strip(input, stripToken, true, true, true);

        ASSERT_EQ(input, expectedResult);
    }

    TEST_F(StringFuncTest, Strip_InternalCharactersBeginningEndString_Success)
    {
        AZStd::string input = "HeLlo HeLlo HELlO";
        AZStd::string expectedResult = " HeLlo ";
        const char* stripToken = "hello";

        AzFramework::StringFunc::Strip(input, stripToken, false, true, true);

        ASSERT_EQ(input, expectedResult);
    }

    TEST_F(StringFuncTest, Strip_Beginning_Success)
    {
        AZStd::string input = "AbrAcadabra";
        AZStd::string expectedResult = "brAcadabra";
        const char stripToken = 'a';

        AzFramework::StringFunc::Strip(input, stripToken, false, true);

        ASSERT_EQ(input, expectedResult);
    }

    TEST_F(StringFuncTest, Strip_End_Success)
    {
        AZStd::string input = "AbrAcadabra";
        AZStd::string expectedResult = "AbrAcadabr";
        const char stripToken = 'a';

        AzFramework::StringFunc::Strip(input, stripToken, false, false, true);

        ASSERT_EQ(input, expectedResult);
    }

    TEST_F(StringFuncTest, Strip_BeginningEnd_Success)
    {
        AZStd::string input = "AbrAcadabra";
        AZStd::string expectedResult = "brAcadabr";
        const char stripToken = 'a';

        AzFramework::StringFunc::Strip(input, stripToken, false, true, true);

        ASSERT_EQ(input, expectedResult);
    }

    TEST_F(StringFuncTest, Strip_BeginningEndCaseSensitive_EndOnly)
    {
        AZStd::string input = "AbrAcadabra";
        AZStd::string expectedResult = "AbrAcadabr";
        const char stripToken = 'a';

        AzFramework::StringFunc::Strip(input, stripToken, true, true, true);

        ASSERT_EQ(input, expectedResult);
    }

    class TestPathStringArgs
    {
    public:
        TestPathStringArgs(const char* input, const char* expected_output)
            : m_input(input),
            m_expected(expected_output)
        {}

        const char* m_input;
        const char* m_expected;

    };

    class StringPathFuncTest
    : public StringFuncTest,
      public ::testing::WithParamInterface<TestPathStringArgs>
    {
    public:
        StringPathFuncTest() = default;
        virtual ~StringPathFuncTest() = default;
    };


    TEST_P(StringPathFuncTest, TestNormalizePath)
    {
        const TestPathStringArgs& param = GetParam();

        AZStd::string input = AZStd::string(param.m_input);
        AZStd::string expected = AZStd::string(param.m_expected);

        bool result = AzFramework::StringFunc::Path::Normalize(input);
        EXPECT_TRUE(result);
        EXPECT_EQ(input, expected);
    }


#if AZ_TRAIT_OS_USE_WINDOWS_FILE_PATHS

    INSTANTIATE_TEST_CASE_P(
        PathWithSingleDotSubFolders,
        StringPathFuncTest, 
        ::testing::Values(
            TestPathStringArgs("F:\\test\\to\\get\\.\\drive\\", "F:\\test\\to\\get\\drive\\")
        ));

    INSTANTIATE_TEST_CASE_P(
        PathWithDoubleDotSubFolders,
        StringPathFuncTest, 
        ::testing::Values(
            TestPathStringArgs("C:\\One\\Two\\..\\Three\\",                                          "C:\\One\\Three\\"),
            TestPathStringArgs("C:\\One\\..\\..\\Two\\",                                             "C:\\Two\\"),
            TestPathStringArgs("C:\\One\\Two\\Three\\..\\",                                          "C:\\One\\Two\\"),
            TestPathStringArgs("F:\\test\\to\\get\\..\\blue\\orchard\\..\\drive\\",                  "F:\\test\\to\\blue\\drive\\"),
            TestPathStringArgs("F:\\test\\to\\.\\.\\get\\..\\.\\.\\drive\\",                         "F:\\test\\to\\drive\\"),
            TestPathStringArgs("F:\\..\\test\\to\\.\\.\\get\\..\\.\\.\\drive\\",                     "F:\\test\\to\\drive\\"),
            TestPathStringArgs("F:\\..\\..\\..\\test\\to\\.\\.\\get\\..\\.\\.\\drive\\",             "F:\\test\\to\\drive\\"),
            TestPathStringArgs("F:\\..\\..\\..\\test\\to\\.\\.\\get\\..\\.\\.\\drive\\..\\..\\..\\", "F:\\"),
            TestPathStringArgs("F:\\..\\",                                                           "F:\\")
   ));
#else
    INSTANTIATE_TEST_CASE_P(
        PathWithSingleDotSubFolders,
        StringPathFuncTest, ::testing::Values(
            TestPathStringArgs("//test/to/get/./drive/", "//test/to/get/drive/")
        ));

    INSTANTIATE_TEST_CASE_P(
        PathWithDoubleDotSubFolders,
        StringPathFuncTest, 
        ::testing::Values(
            TestPathStringArgs("//one/two/../three/",                     "//one/three/"),
            TestPathStringArgs("//one/../../two/",                        "//two/"),
            TestPathStringArgs("//one/two/three/../",                     "//one/two/"),
            TestPathStringArgs("//test/to/get/../blue/orchard/../drive/", "//test/to/blue/drive/"),
            TestPathStringArgs("//test/to/././get.//././.drive/",         "//test/to/get./.drive/"),
            TestPathStringArgs("//../../test/to/././get/../././drive/",   "//test/to/drive/")
    ));
#endif //AZ_TRAIT_OS_USE_WINDOWS_FILE_PATHS
    
}
