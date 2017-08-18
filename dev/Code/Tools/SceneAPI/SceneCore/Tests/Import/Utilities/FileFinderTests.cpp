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

#include <AzTest/AzTest.h>
#include <SceneAPI/SceneCore/Import/Utilities/FileFinder.h>
#include <AzCore/IO/SystemFile.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace Import
        {
            namespace Utilities
            {
                TEST(FileFinderTest, IsValidSceneFile_MatchedSceneFileExtension_ReturnsTrue)
                {
                    EXPECT_TRUE(FileFinder::IsValidSceneFile("test.fbx", "fbx"));
                }

                TEST(FileFinderTest, IsValidSceneFile_MatchedSceneFileExtensionWithDot_ReturnsTrue)
                {
                    EXPECT_TRUE(FileFinder::IsValidSceneFile("test.fbx", ".fbx"));
                }

                TEST(FileFinderTest, IsValidSceneFile_NotMatchedSceneFileExtensions_ReturnsFalse)
                {
                    EXPECT_FALSE(FileFinder::IsValidSceneFile("test.ma", "fbx"));
                }

                TEST(FileFinderTest, IsValidFbxSceneFile_IsValidFbxSceneFileExtension_ReturnsTrue)
                {
                    EXPECT_TRUE(FileFinder::IsValidFbxSceneFile("test.fbx"));
                }

                TEST(FileFinderTest, IsValidFbxSceneFile_IsValidFbxSceneFileExtension_ReturnsFalse)
                {
                    EXPECT_FALSE(FileFinder::IsValidFbxSceneFile("test.ma"));
                }

                class FileFinderIOTests
                    : public ::testing::Test
                {
                public:
                    virtual ~FileFinderIOTests() = default;

                protected:
                    virtual void SetUp()
                    {
                        m_testFileNames =
                        {
                            "test1.fbx",
                            "test1.scenesettings",
                            "test1.xml"
                        };
                        AZ::IO::SystemFile testFile;
                        for (const std::string& fileName : m_testFileNames)
                        {
                            bool result = testFile.Open(fileName.c_str(), AZ::IO::SystemFile::SF_OPEN_CREATE | AZ::IO::SystemFile::SF_OPEN_CREATE_PATH);
                            AZ_Assert(result, "Test file %s is not created successfully", fileName.c_str());
                            testFile.Close();
                        }
                    }

                    virtual void TearDown()
                    {
                        for (const std::string& fileName : m_testFileNames)
                        {
                            AZ_Assert(AZ::IO::SystemFile::Delete(fileName.c_str()), "Test file %s is not deleted successfully", fileName.c_str());
                        }
                    }

                    std::vector<std::string> m_testFileNames;
                };

                TEST_F(FileFinderIOTests, FindMatchingFileName_MatchedFileExists_ReturnsTrue)
                {
                    std::string outFileName;
                    EXPECT_TRUE(FileFinder::FindMatchingFileName(outFileName, "test1.fbx", "scenesettings"));
                    EXPECT_EQ("test1.scenesettings", outFileName);
                }

                TEST_F(FileFinderIOTests, FindMatchingFileName_MatchedFileExistsExtensionWithDot_ReturnsTrue)
                {
                    std::string outFileName;
                    EXPECT_TRUE(FileFinder::FindMatchingFileName(outFileName, "test1.fbx", ".scenesettings"));
                    EXPECT_EQ("test1.scenesettings", outFileName);
                }

                TEST_F(FileFinderIOTests, FindMatchingFileName_MatchedFileNotExists_ReturnsFalse)
                {
                    std::string outFileName;
                    EXPECT_FALSE(FileFinder::FindMatchingFileName(outFileName, "test1.fbx", "notExistExtension"));
                    EXPECT_EQ("", outFileName);
                }

                TEST_F(FileFinderIOTests, FindMatchingFileNames_SomeMatchedFilesExist_GetsExistedFileName)
                {
                    std::vector<std::string> outputFileNames, extensions = {"scenesettings", "xml", "txt"};
                    FileFinder::FindMatchingFileNames(outputFileNames, "test1.fbx", extensions);
                    EXPECT_EQ(2, outputFileNames.size());
                    EXPECT_EQ("test1.scenesettings", outputFileNames[0]);
                    EXPECT_EQ("test1.xml", outputFileNames[1]);
                }
            }
        }
    }
}