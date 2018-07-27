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


#include <AzFramework/StringFunc/StringFunc.h>

namespace AzFramework
{
    using namespace UnitTest;

    class StringFuncTest
    : public AllocatorsFixture
    {
    public:
        StringFuncTest()
            : AllocatorsFixture(15, false)
        {
        }

        ~StringFuncTest() = default;

        void SetUp() override
        {
            AllocatorsFixture::SetUp();
        }

        void TearDown() override
        {
            AllocatorsFixture::TearDown();
        }
    };


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
}
