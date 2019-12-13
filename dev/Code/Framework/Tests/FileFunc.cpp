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

#include <AzCore/Outcome/Outcome.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzFramework/FileFunc/FileFunc.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzFramework/IO/LocalFileIO.h>

#include <QTemporaryDir>
#include <QTextStream>
#include <QDir>
#include <QFileInfo>

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

class FileFuncTest : public ScopedAllocatorSetupFixture
{
public:
    void SetUp()
    {
        m_prevFileIO = AZ::IO::FileIOBase::GetInstance();
        AZ::IO::FileIOBase::SetInstance(&m_fileIO);
    }

    void TearDown() override
    {
        AZ::IO::FileIOBase::SetInstance(m_prevFileIO);
    }

    AZ::IO::LocalFileIO m_fileIO;
    AZ::IO::FileIOBase* m_prevFileIO;
};

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

static bool CreateDummyFile(const QString& fullPathToFile, const QString& tempStr = {})
{
    QFileInfo fi(fullPathToFile);
    QDir fp(fi.path());
    fp.mkpath(".");
    QFile writer(fullPathToFile);
    if (!writer.open(QFile::ReadWrite))
    {
        return false;
    }
    {
        QTextStream stream(&writer);
        stream << tempStr << endl;
    }
    writer.close();
    return true;
}

TEST_F(FileFuncTest, FindFilesTest_EmptyFolder_Failure)
{
    QTemporaryDir tempDir;

    QDir tempPath(tempDir.path());

    const char dependenciesPattern[] = "*_dependencies.xml";
    bool recurse = true;
    AZStd::string folderPath = tempPath.absolutePath().toStdString().c_str();
    AZ::Outcome<AZStd::list<AZStd::string>, AZStd::string> result = AzFramework::FileFunc::FindFileList(folderPath.c_str(),
        dependenciesPattern, recurse);

    ASSERT_TRUE(result.IsSuccess());
    ASSERT_EQ(result.GetValue().size(), 0);
}

TEST_F(FileFuncTest, FindFilesTest_DependenciesWildcards_Success)
{
    QTemporaryDir tempDir;

    QDir tempPath(tempDir.path());

    const char* expectedFileNames[] = { "a_dependencies.xml","b_dependencies.xml" };
    ASSERT_TRUE(CreateDummyFile(tempPath.absoluteFilePath(expectedFileNames[0]), QString("tempdata\n")));
    ASSERT_TRUE(CreateDummyFile(tempPath.absoluteFilePath(expectedFileNames[1]), QString("tempdata\n")));
    ASSERT_TRUE(CreateDummyFile(tempPath.absoluteFilePath("dependencies.xml"), QString("tempdata\n")));

    const char dependenciesPattern[] = "*_dependencies.xml";
    bool recurse = true;
    AZStd::string folderPath = tempPath.absolutePath().toStdString().c_str();
    AZ::Outcome<AZStd::list<AZStd::string>, AZStd::string> result = AzFramework::FileFunc::FindFileList(folderPath.c_str(),
        dependenciesPattern, recurse);

    ASSERT_TRUE(result.IsSuccess());
    ASSERT_EQ(result.GetValue().size(), 2);

    for (size_t i = 0; i < AZ_ARRAY_SIZE(expectedFileNames); ++i)
    {
        auto findElement = AZStd::find_if(result.GetValue().begin(), result.GetValue().end(), [&expectedFileNames, i] (const AZStd::string& thisString)
        {
            AZStd::string thisFileName;
            AzFramework::StringFunc::Path::GetFullFileName(thisString.c_str(), thisFileName);
            return thisFileName == expectedFileNames[i];
        });
        ASSERT_NE(findElement, result.GetValue().end());
    }
}

TEST_F(FileFuncTest, FindFilesTest_DependenciesWildcardsSubfolders_Success)
{
    QTemporaryDir tempDir;

    QDir tempPath(tempDir.path());

    ASSERT_TRUE(CreateDummyFile(tempPath.absoluteFilePath("a_dependencies.xml"), QString("tempdata\n")));
    ASSERT_TRUE(CreateDummyFile(tempPath.absoluteFilePath("b_dependencies.xml"), QString("tempdata\n")));
    ASSERT_TRUE(CreateDummyFile(tempPath.absoluteFilePath("dependencies.xml"), QString("tempdata\n")));

    const char dependenciesPattern[] = "*_dependencies.xml";
    bool recurse = true;
    AZStd::string folderPath = tempPath.absolutePath().toStdString().c_str();

    ASSERT_TRUE(CreateDummyFile(tempPath.absoluteFilePath("subfolder1/c_dependencies.xml"), QString("tempdata\n")));
    ASSERT_TRUE(CreateDummyFile(tempPath.absoluteFilePath("subfolder1/d_dependencies.xml"), QString("tempdata\n")));
    ASSERT_TRUE(CreateDummyFile(tempPath.absoluteFilePath("subfolder1/dependencies.xml"), QString("tempdata\n")));

    AZ::Outcome<AZStd::list<AZStd::string>, AZStd::string> result = AzFramework::FileFunc::FindFileList(folderPath.c_str(),
        dependenciesPattern, recurse);

    ASSERT_TRUE(result.IsSuccess());
    ASSERT_EQ(result.GetValue().size(), 4);

    const char* expectedFileNames[] = { "a_dependencies.xml","b_dependencies.xml", "c_dependencies.xml", "d_dependencies.xml" };
    for (size_t i = 0; i < AZ_ARRAY_SIZE(expectedFileNames); ++i)
    {
        auto findElement = AZStd::find_if(result.GetValue().begin(), result.GetValue().end(), [&expectedFileNames, i](const AZStd::string& thisString)
        {
            AZStd::string thisFileName;
            AzFramework::StringFunc::Path::GetFullFileName(thisString.c_str(), thisFileName);
            return thisFileName == expectedFileNames[i];
        });
        ASSERT_NE(findElement, result.GetValue().end());
    }
}
