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
#include <AzCore/Outcome/Outcome.h>

namespace AzFramework
{
    namespace FileFunc
    {
        namespace Internal
        {
            AZ::Outcome<void,AZStd::string> UpdateCfgContents(AZStd::string& cfgContents, const AZStd::list<AZStd::string>& updateRules);
            AZ::Outcome<void,AZStd::string> UpdateCfgContents(AZStd::string& cfgContents, const AZStd::string& header, const AZStd::string& key, const AZStd::string& value);
        }
    }
}

using namespace UnitTest;

using FileFuncTest = AllocatorsFixture;

TEST_F(FileFuncTest, UpdateCfgContents_InValidInput_Fail)
{
    AZStd::string               cfgContents = "[Foo]\n";
    AZStd::list<AZStd::string>  updateRules;

    updateRules.push_back(AZStd::string("Foo/one*1"));
    auto result = AzFramework::FileFunc::Internal::UpdateCfgContents(cfgContents, updateRules);
    ASSERT_FALSE(result.IsSuccess());
}

TEST_F(FileFuncTest, UpdateCfgContents_ValidInput_Success)
{
    AZStd::string cfgContents =
        "[Foo]\n"
        "one =2 \n"
        "two= 3\n"
        "three = 4\n"
        "\n"
        "[Bar]\n"
        "four=3\n"
        "five=3\n"
        "six=3\n"
        "eight=3\n";

    AZStd::list<AZStd::string> updateRules;

    updateRules.push_back(AZStd::string("Foo/one=1"));
    updateRules.push_back(AZStd::string("Foo/two=2"));
    updateRules.push_back(AZStd::string("three=3"));
    auto result = AzFramework::FileFunc::Internal::UpdateCfgContents(cfgContents, updateRules);

    AZStd::string compareCfgContents =
        "[Foo]\n"
        "one =1\n"
        "two= 2\n"
        "three = 3\n"
        "\n"
        "[Bar]\n"
        "four=3\n"
        "five=3\n"
        "six=3\n"
        "eight=3\n";
    bool equals = cfgContents.compare(compareCfgContents) == 0;
    ASSERT_TRUE(equals);


}

TEST_F(FileFuncTest, UpdateCfgContents_ValidInputNewEntrySameHeader_Success)
{
    AZStd::string cfgContents =
        "[Foo]\n"
        "one =2 \n"
        "two= 3\n"
        "three = 4\n";

    AZStd::string header("[Foo]");
    AZStd::string key("four");
    AZStd::string value("4");
    auto result = AzFramework::FileFunc::Internal::UpdateCfgContents(cfgContents, header, key, value);

    AZStd::string compareCfgContents =
        "[Foo]\n"
        "four = 4\n"
        "one =2 \n"
        "two= 3\n"
        "three = 4\n";
    bool equals = cfgContents.compare(compareCfgContents) == 0;
    ASSERT_TRUE(equals);
}

TEST_F(FileFuncTest, UpdateCfgContents_ValidInputNewEntryDifferentHeader_Success)
{
    AZStd::string cfgContents =
        ";Sample Data\n"
        "[Foo]\n"
        "one =2 \n"
        "two= 3\n"
        "three = 4\n";

    AZStd::list<AZStd::string> updateRules;

    AZStd::string header("[Bar]");
    AZStd::string key("four");
    AZStd::string value("4");
    auto result = AzFramework::FileFunc::Internal::UpdateCfgContents(cfgContents, header, key, value);

    AZStd::string compareCfgContents =
        "[Bar]\n"
        "four = 4\n"
        ";Sample Data\n"
        "[Foo]\n"
        "one =2 \n"
        "two= 3\n"
        "three = 4\n";
    bool equals = cfgContents.compare(compareCfgContents) == 0;
    ASSERT_TRUE(equals);
}
