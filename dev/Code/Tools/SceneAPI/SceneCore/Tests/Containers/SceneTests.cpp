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
#include <SceneAPI/SceneCore/Containers/Scene.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace Containers
        {
            TEST(SceneTest, Constructor_StringRef_HasCorrectName)
            {
                AZStd::string sampleSceneName = "testName";
                Scene testScene(sampleSceneName);

                EXPECT_TRUE(sampleSceneName == testScene.GetName());
            }

            TEST(SceneTest, Constructor_StringRefRef_HasCorrectName)
            {
                const char* sampleNameChrStar = "testName";
                AZStd::string sampleSceneName = sampleNameChrStar;
                Scene testScene(AZStd::move(sampleSceneName));

                EXPECT_TRUE(strcmp(sampleNameChrStar, testScene.GetName().c_str()) == 0);
            }

            TEST(SceneTest, Constructor_EmptyStrRef_HasCorrectName)
            {
                AZStd::string sampleSceneName = "";
                Scene testScene(sampleSceneName);

                EXPECT_TRUE(sampleSceneName == testScene.GetName().c_str());
            }

            class SceneFilenameTests
                : public ::testing::Test
            {
            public:
                SceneFilenameTests()
                    : m_testScene("testScene")
                {
                }

            protected:
                void SetUp()
                {
                }

                void TearDown()
                {
                }

                Scene m_testScene;
            };

            TEST_F(SceneFilenameTests, SetSourceFilename_StringRef_SourceFileRegistered)
            {
                AZStd::string testFilename = "testFilename.fbx";
                m_testScene.SetSourceFilename(testFilename);
                const AZStd::string compareFilename = m_testScene.GetSourceFilename();
                EXPECT_STREQ(testFilename.c_str(), compareFilename.c_str());
            }

            TEST_F(SceneFilenameTests, SetSourceFilename_StringRefRef_SourceFileRegistered)
            {
                const char* testChrFilename = "testFilename.fbx";
                AZStd::string testFilename = testChrFilename;
                m_testScene.SetSourceFilename(AZStd::move(testFilename));
                const AZStd::string compareFilename = m_testScene.GetSourceFilename();
                EXPECT_STREQ(testChrFilename, compareFilename.c_str());
            }

            TEST_F(SceneFilenameTests, SetManifestFilename_StringRef_ManifestFileRegistered)
            {
                AZStd::string testFilename = "testFilename.assetinfo";
                m_testScene.SetManifestFilename(testFilename);
                const AZStd::string compareFilename = m_testScene.GetManifestFilename();
                EXPECT_STREQ(testFilename.c_str(), compareFilename.c_str());
            }

            TEST_F(SceneFilenameTests, SetManifestFilename_StringRefRef_ManifestFileRegistered)
            {
                const char* testChrFilename = "testFilename.assetinfo";
                AZStd::string testFilename = testChrFilename;
                m_testScene.SetManifestFilename(AZStd::move(testFilename));
                const AZStd::string compareFilename = m_testScene.GetManifestFilename();
                EXPECT_STREQ(testChrFilename, compareFilename.c_str());
            }
        }
    }
}
