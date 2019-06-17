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

#include <Tests/TestTypes.h>
#include <gtest/gtest-param-test.h>

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
