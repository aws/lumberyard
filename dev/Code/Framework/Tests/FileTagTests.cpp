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

#include <AzCore/IO/FileIO.h>
#include <AzCore/std/containers/set.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzFramework/Application/Application.h>
#include <AzFramework/FileTag/FileTag.h>
#include <AzFramework/FileTag/FileTagBus.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzTestShared/Utils/Utils.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <fstream>

namespace UnitTest
{

    const char DummyFile[] = "dummy.txt";
    const char AnotherDummyFile[] = "Foo/Dummy.txt";
    
    const char DummyPattern[] = R"(^(.+)_([a-z]+)\..+$)";
    const char MatchingPatternFile[] = "Foo/dummy_abc.txt";
    const char NonMatchingPatternFile[] = "Foo/dummy_a8c.txt";

    const char DummyWildcard[] = "?oo.txt";
    const char MatchingWildcardFile[] = "Foo.txt";
    const char NonMatchingWildcardFile[] = "Test.txt";

    const char* DummyFileTags[] = { "A", "B", "C", "D", "E", "F", "G" };
    const char* DummyFileTagsLowerCase[] = { "a", "b", "c", "d", "e", "f", "g" };

    enum DummyFileTagIndex
    {
        AIdx = 0,
        BIdx,
        CIdx,
        DIdx,
        EIdx,
        FIdx,
        GIdx
    };

    const char BlackListFile[] = "BlackList";
    const char WhiteListFile[] = "WhiteList";

    class FileTagQueryManagerTest : public AzFramework::FileTag::FileTagQueryManager
    {
    public:
        friend class GTEST_TEST_CLASS_NAME_(FileTagTest, FileTags_QueryFilePlusPatternMatch_Valid);
        friend class GTEST_TEST_CLASS_NAME_(FileTagTest, FileTags_RemoveTag_Valid);

        FileTagQueryManagerTest(AzFramework::FileTag::FileTagType fileTagType)
            :AzFramework::FileTag::FileTagQueryManager(fileTagType)
        {
        }

        void ClearData()
        {
            m_fileTagsMap.clear();
            m_patternTagsMap.clear();
        }
    };

    class FileTagTest
        : public AllocatorsFixture
    {
    public:
        
        void SetUp() override
        {
            AllocatorsFixture::SetUp();
            m_data = AZStd::make_unique<StaticData>();
            using namespace  AzFramework::FileTag;
            AZ::ComponentApplication::Descriptor desc;
            desc.m_enableDrilling = false;
            m_data->m_application.Start(desc);

            m_data->m_blackListQueryManager = AZStd::make_unique<FileTagQueryManagerTest>(FileTagType::BlackList);
            m_data->m_whiteListQueryManager = AZStd::make_unique<FileTagQueryManagerTest>(FileTagType::WhiteList);

            AZStd::vector<AZStd::string> blackListFileTags = { DummyFileTags[DummyFileTagIndex::AIdx], DummyFileTags[DummyFileTagIndex::BIdx] };
            EXPECT_TRUE(m_data->m_fileTagManager.AddFileTags(DummyFile, FileTagType::BlackList, blackListFileTags).IsSuccess());

            AZStd::vector<AZStd::string> whiteListFileTags = { DummyFileTags[DummyFileTagIndex::CIdx], DummyFileTags[DummyFileTagIndex::DIdx] };
            EXPECT_TRUE(m_data->m_fileTagManager.AddFileTags(AnotherDummyFile, FileTagType::WhiteList, whiteListFileTags).IsSuccess());

            AZStd::vector<AZStd::string> blackListPatternTags = { DummyFileTags[DummyFileTagIndex::EIdx], DummyFileTags[DummyFileTagIndex::FIdx] };
            EXPECT_TRUE(m_data->m_fileTagManager.AddFilePatternTags(DummyPattern, FilePatternType::Regex, FileTagType::BlackList, blackListPatternTags).IsSuccess());

            AZStd::vector<AZStd::string> whiteListWildcardTags = { DummyFileTags[DummyFileTagIndex::GIdx] };
            EXPECT_TRUE(m_data->m_fileTagManager.AddFilePatternTags(DummyWildcard, FilePatternType::Wildcard, FileTagType::WhiteList, whiteListWildcardTags).IsSuccess());


            m_data->m_localFileIO = AZStd::make_unique<AZ::IO::LocalFileIO>();
            m_data->m_priorFileIO = AZ::IO::FileIOBase::GetInstance();
            AZ::IO::FileIOBase::SetInstance(nullptr);
            AZ::IO::FileIOBase::SetInstance(m_data->m_localFileIO.get());

            AZ::IO::FileIOBase::GetInstance()->SetAlias("@assets@", GetTestFolderPath().c_str());

            AzFramework::StringFunc::Path::Join(GetTestFolderPath().c_str(), AZStd::string::format("%s.%s", BlackListFile, FileTagAsset::Extension()).c_str(), m_data->m_blackListFile, true);

            AzFramework::StringFunc::Path::Join(GetTestFolderPath().c_str(), AZStd::string::format("%s.%s", WhiteListFile, FileTagAsset::Extension()).c_str(), m_data->m_whiteListFile, true);

            EXPECT_TRUE(m_data->m_fileTagManager.Save(FileTagType::BlackList, m_data->m_blackListFile));
            EXPECT_TRUE(m_data->m_fileTagManager.Save(FileTagType::WhiteList, m_data->m_whiteListFile));

            EXPECT_TRUE(m_data->m_blackListQueryManager->Load(m_data->m_blackListFile));
            EXPECT_TRUE(m_data->m_whiteListQueryManager->Load(m_data->m_whiteListFile));

            AzFramework::StringFunc::Path::Join(GetTestFolderPath().c_str(), "Test_Dependencies.xml", m_data->m_engineDependenciesFile, true);
            std::ofstream outFile(m_data->m_engineDependenciesFile.c_str(), std::ofstream::out | std::ofstream::app);
            outFile << "<EngineDependencies versionnumber=\"1.0.0\"><Dependency path=\"Foo/Dummy.txt\" optional=\"true\"/><Dependency path=\"Foo/dummy_abc.txt\" optional=\"false\"/></EngineDependencies>";
            outFile.close();
        }

        void TearDown() override
        {
            AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();

            if (fileIO->Exists(m_data->m_engineDependenciesFile.c_str()))
            {
                fileIO->Remove(m_data->m_engineDependenciesFile.c_str());
            }

            if (fileIO->Exists(m_data->m_blackListFile.c_str()))
            {
                fileIO->Remove(m_data->m_blackListFile.c_str());
            }

            if (fileIO->Exists(m_data->m_whiteListFile.c_str()))
            {
                fileIO->Remove(m_data->m_whiteListFile.c_str());
            }

            AZ::IO::FileIOBase::SetInstance(nullptr);
            AZ::IO::FileIOBase::SetInstance(m_data->m_priorFileIO);

            m_data->m_application.Stop();
            m_data.reset();
            AllocatorsFixture::TearDown();
        }


        struct StaticData
        {
            AzFramework::Application m_application;
            AzFramework::FileTag::FileTagManager m_fileTagManager;
            AZStd::unique_ptr<FileTagQueryManagerTest> m_blackListQueryManager;
            AZStd::unique_ptr<FileTagQueryManagerTest> m_whiteListQueryManager;
            AZ::IO::FileIOBase* m_priorFileIO = nullptr;
            AZStd::unique_ptr<AZ::IO::FileIOBase> m_localFileIO;
            AZStd::string m_blackListFile;
            AZStd::string m_whiteListFile;
            AZStd::string m_engineDependenciesFile;
        };

        AZStd::unique_ptr<StaticData> m_data;
        
    };

    TEST_F(FileTagTest, FileTags_QueryFile_Valid)
    {
        AZStd::set<AZStd::string> tags = m_data->m_blackListQueryManager->GetTags(DummyFile);

        EXPECT_EQ(tags.size(), 2);
        EXPECT_EQ(tags.count(DummyFileTagsLowerCase[DummyFileTagIndex::AIdx]), 1);
        EXPECT_EQ(tags.count(DummyFileTagsLowerCase[DummyFileTagIndex::BIdx]), 1);

        tags = m_data->m_whiteListQueryManager->GetTags(DummyFile);
        EXPECT_EQ(tags.size(), 0);

        tags = m_data->m_whiteListQueryManager->GetTags(AnotherDummyFile);
        EXPECT_EQ(tags.size(), 2);
        EXPECT_EQ(tags.count(DummyFileTagsLowerCase[DummyFileTagIndex::CIdx]), 1);
        EXPECT_EQ(tags.count(DummyFileTagsLowerCase[DummyFileTagIndex::DIdx]), 1);

        tags = m_data->m_blackListQueryManager->GetTags(AnotherDummyFile);
        EXPECT_EQ(tags.size(), 0);
    }

    TEST_F(FileTagTest, FileTags_QueryPattern_Valid)
    {
        AZStd::set<AZStd::string> tags = m_data->m_blackListQueryManager->GetTags(MatchingPatternFile);

        EXPECT_EQ(tags.size(), 2);
        EXPECT_EQ(tags.count(DummyFileTagsLowerCase[DummyFileTagIndex::EIdx]), 1);
        EXPECT_EQ(tags.count(DummyFileTagsLowerCase[DummyFileTagIndex::FIdx]), 1);

        tags = m_data->m_whiteListQueryManager->GetTags(MatchingPatternFile);
        EXPECT_EQ(tags.size(), 0);

        tags = m_data->m_blackListQueryManager->GetTags(NonMatchingPatternFile);
        EXPECT_EQ(tags.size(), 0);

        tags = m_data->m_whiteListQueryManager->GetTags(NonMatchingPatternFile);
        EXPECT_EQ(tags.size(), 0);
    }

    TEST_F(FileTagTest, FileTags_QueryWildcard_Valid)
    {
        AZStd::set<AZStd::string> tags = m_data->m_whiteListQueryManager->GetTags(MatchingWildcardFile);

        EXPECT_EQ(tags.size(), 1);
        EXPECT_EQ(tags.count(DummyFileTagsLowerCase[DummyFileTagIndex::GIdx]), 1);

        tags = m_data->m_blackListQueryManager->GetTags(MatchingWildcardFile);
        EXPECT_EQ(tags.size(), 0);

        tags = m_data->m_blackListQueryManager->GetTags(NonMatchingWildcardFile);
        EXPECT_EQ(tags.size(), 0);

        tags = m_data->m_whiteListQueryManager->GetTags(NonMatchingWildcardFile);
        EXPECT_EQ(tags.size(), 0);
    }

    TEST_F(FileTagTest, FileTags_LoadEngineDependencies_AddToBlackList)
    {
        m_data->m_blackListQueryManager->ClearData();
        EXPECT_TRUE(m_data->m_blackListQueryManager->LoadEngineDependencies(m_data->m_engineDependenciesFile));

        AZStd::string normalizedFilePath = AnotherDummyFile;
        EXPECT_TRUE(AzFramework::StringFunc::Path::Normalize(normalizedFilePath));

        AZStd::set<AZStd::string> outputTags = m_data->m_blackListQueryManager->GetTags(normalizedFilePath);
        EXPECT_EQ(outputTags.size(), 2);
        EXPECT_EQ(outputTags.count("ignore"), 1);
        EXPECT_EQ(outputTags.count("productdependency"), 1);

        normalizedFilePath = MatchingPatternFile;
        EXPECT_TRUE(AzFramework::StringFunc::Path::Normalize(normalizedFilePath));

        outputTags = m_data->m_blackListQueryManager->GetTags(normalizedFilePath);
        EXPECT_EQ(outputTags.size(), 0);
    }

    TEST_F(FileTagTest, FileTags_QueryFilePlusPatternMatch_Valid)
    {
        using namespace  AzFramework::FileTag;
        AZStd::vector<AZStd::string>  inputTags = { DummyFileTags[DummyFileTagIndex::GIdx] };
        EXPECT_TRUE(m_data->m_fileTagManager.AddFileTags(MatchingWildcardFile, FileTagType::BlackList, inputTags).IsSuccess());

        inputTags = { DummyFileTags[DummyFileTagIndex::AIdx] };
        EXPECT_TRUE(m_data->m_fileTagManager.AddFilePatternTags(DummyWildcard, FilePatternType::Wildcard, FileTagType::BlackList, inputTags).IsSuccess());

        EXPECT_TRUE(m_data->m_fileTagManager.Save(FileTagType::BlackList, m_data->m_blackListFile));

        m_data->m_blackListQueryManager->ClearData();
        EXPECT_TRUE(m_data->m_blackListQueryManager->Load(m_data->m_blackListFile));
        
        AZStd::set<AZStd::string> outputTags = m_data->m_blackListQueryManager->GetTags(MatchingWildcardFile);

        EXPECT_EQ(outputTags.size(), 2);
        EXPECT_EQ(outputTags.count(DummyFileTagsLowerCase[DummyFileTagIndex::AIdx]), 1);
        EXPECT_EQ(outputTags.count(DummyFileTagsLowerCase[DummyFileTagIndex::GIdx]), 1);
    }

    TEST_F(FileTagTest, FileTags_RemoveTag_Valid)
    {
        using namespace  AzFramework::FileTag;
        AZStd::set<AZStd::string> tags = m_data->m_blackListQueryManager->GetTags(DummyFile);

        EXPECT_EQ(tags.size(), 2);
        EXPECT_EQ(tags.count(DummyFileTagsLowerCase[DummyFileTagIndex::AIdx]), 1);
        EXPECT_EQ(tags.count(DummyFileTagsLowerCase[DummyFileTagIndex::BIdx]), 1);

        //remove Tag A
        AZStd::vector<AZStd::string> blackListFileTags = { DummyFileTags[DummyFileTagIndex::AIdx] };
        EXPECT_TRUE(m_data->m_fileTagManager.RemoveFileTags(DummyFile, FileTagType::BlackList, blackListFileTags).IsSuccess());

        EXPECT_TRUE(m_data->m_fileTagManager.Save(FileTagType::BlackList, m_data->m_blackListFile));

        m_data->m_blackListQueryManager->ClearData();
        EXPECT_TRUE(m_data->m_blackListQueryManager->Load(m_data->m_blackListFile));

        tags = m_data->m_blackListQueryManager->GetTags(DummyFile);

        EXPECT_EQ(tags.size(), 1);
        EXPECT_EQ(tags.count(DummyFileTagsLowerCase[DummyFileTagIndex::BIdx]), 1);

        //remove Tag B
        blackListFileTags = { DummyFileTags[DummyFileTagIndex::BIdx] };
        EXPECT_TRUE(m_data->m_fileTagManager.RemoveFileTags(DummyFile, FileTagType::BlackList, blackListFileTags).IsSuccess());

        EXPECT_TRUE(m_data->m_fileTagManager.Save(FileTagType::BlackList, m_data->m_blackListFile));

        m_data->m_blackListQueryManager->ClearData();
        EXPECT_TRUE(m_data->m_blackListQueryManager->Load(m_data->m_blackListFile));

        tags = m_data->m_blackListQueryManager->GetTags(DummyFile);

        EXPECT_EQ(tags.size(), 0);
    }

    TEST_F(FileTagTest, FileTags_Matching_Valid)
    {
        AZStd::vector<AZStd::string> matchFileTags = { DummyFileTags[DummyFileTagIndex::AIdx], DummyFileTags[DummyFileTagIndex::BIdx] };

        bool match = m_data->m_blackListQueryManager->Match(DummyFile, matchFileTags);
        EXPECT_TRUE(match);

        matchFileTags = { DummyFileTags[DummyFileTagIndex::AIdx] };
        match = m_data->m_blackListQueryManager->Match(DummyFile, matchFileTags);
        EXPECT_TRUE(match);

        matchFileTags = { DummyFileTags[DummyFileTagIndex::BIdx] };
        match = m_data->m_blackListQueryManager->Match(DummyFile, matchFileTags);
        EXPECT_TRUE(match);

        matchFileTags = { DummyFileTags[DummyFileTagIndex::CIdx] };
        match = m_data->m_blackListQueryManager->Match(DummyFile, matchFileTags);
        EXPECT_FALSE(match);
    }

    TEST_F(FileTagTest, FileTags_ValidateError_Ok)
    {
        using namespace  AzFramework::FileTag;
        AZStd::set<AZStd::string> tags = m_data->m_blackListQueryManager->GetTags(DummyFile);

        EXPECT_EQ(tags.size(), 2);
        EXPECT_EQ(tags.count(DummyFileTagsLowerCase[DummyFileTagIndex::AIdx]), 1);
        EXPECT_EQ(tags.count(DummyFileTagsLowerCase[DummyFileTagIndex::BIdx]), 1);

        // file tags already exists
        AZStd::vector<AZStd::string> blackListFileTags = { DummyFileTags[DummyFileTagIndex::AIdx], DummyFileTags[DummyFileTagIndex::BIdx] };

        EXPECT_TRUE(m_data->m_fileTagManager.AddFileTags(DummyFile, FileTagType::BlackList, blackListFileTags).IsSuccess());

        //remove Tag C which does not exist
        blackListFileTags = { DummyFileTags[DummyFileTagIndex::CIdx] };
        EXPECT_TRUE(m_data->m_fileTagManager.RemoveFileTags(DummyFile, FileTagType::BlackList, blackListFileTags).IsSuccess());
        // Invalid FilePattern type
        AZStd::vector<AZStd::string> blackListPatternTags = { DummyFileTags[DummyFileTagIndex::EIdx], DummyFileTags[DummyFileTagIndex::FIdx] };
        EXPECT_FALSE(m_data->m_fileTagManager.RemoveFilePatternTags(DummyPattern, FilePatternType::Wildcard, FileTagType::BlackList, blackListPatternTags).IsSuccess());

        // Removing a FilePattern that does not exits
        const char pattern[] = R"(^(.+)_([a-z0-9]+)\..+$)";
        blackListPatternTags = { DummyFileTags[DummyFileTagIndex::EIdx], DummyFileTags[DummyFileTagIndex::FIdx] };
        EXPECT_FALSE(m_data->m_fileTagManager.RemoveFilePatternTags(pattern, FilePatternType::Regex, FileTagType::BlackList, blackListPatternTags).IsSuccess());
    }
}