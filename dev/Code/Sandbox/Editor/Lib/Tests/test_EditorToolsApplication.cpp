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
#include <StdAfx.h>
#include <AzTest/AzTest.h>
#include <Util/EditorUtils.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <EditorToolsApplication.h>
#include <Tests/Utils/Utils.h>
#include <AZCore/StringFunc/StringFunc.h>
#include <LmbrCentral/FileSystem/AliasConfiguration.h>

namespace EditorToolsApplication
{
    class EditorToolsApplicationFixture
        : public testing::Test
    {
    protected:
        void SetUp() override
        {
            AZ::AllocatorInstance<AZ::SystemAllocator>::Create();
            m_localFileIO = aznew AZ::IO::LocalFileIO();
            m_priorFileIO = AZ::IO::FileIOBase::GetInstance();
            // we need to set it to nullptr first because otherwise the 
            // underneath code assumes that we might be leaking the previous instance
            AZ::IO::FileIOBase::SetInstance(nullptr);
            AZ::IO::FileIOBase::SetInstance(m_localFileIO);
            int* argc = 0;
            m_editorsToolApplication = new EditorInternal::EditorToolsApplication(argc, nullptr);
            m_tempDir = new UnitTest::ScopedTemporaryDirectory();
        }

        void TearDown() override
        {
            delete m_tempDir;
            delete m_editorsToolApplication;
            AZ::IO::FileIOBase::SetInstance(nullptr);
            delete m_localFileIO;
            AZ::IO::FileIOBase::SetInstance(m_priorFileIO);
            AZ::AllocatorInstance<AZ::SystemAllocator>::Destroy();
        }

        void SetAliasConfig(bool createAssetCacheFile, bool waitForConnection)
        {
            AZStd::string gameName("Foo");
            SSystemInitParams initParams;
            initParams.waitForConnection = waitForConnection;
            azstrncpy(initParams.rootPathCache, sizeof(initParams.rootPathCache), m_tempDir->GetDirectory(), strlen(m_tempDir->GetDirectory()) + 1); // +1 for the null terminator
            azstrncpy(initParams.assetsPathCache, sizeof(initParams.assetsPathCache), m_tempDir->GetDirectory(), strlen(m_tempDir->GetDirectory()) + 1); // +1 for the null terminator
            azstrncpy(initParams.assetsPath, sizeof(initParams.assetsPath), gameName.c_str(), gameName.length() + 1); // +1 for the null terminator
            AZStd::string filePathToCheck;
            AZ::StringFunc::Path::Join(m_tempDir->GetDirectory(), "engine.json", filePathToCheck);

            if (createAssetCacheFile)
            {
                AZ::IO::HandleType fileHandle = AZ::IO::InvalidHandle;
                if (AZ::IO::FileIOBase::GetInstance()->Open(filePathToCheck.c_str(), AZ::IO::OpenMode::ModeWrite, fileHandle))
                {
                    AZ::IO::FileIOBase::GetInstance()->Close(fileHandle);
                }
            }
            m_editorsToolApplication->SetupAliasConfigStorage(initParams);
        }

        AZ::IO::FileIOBase* m_priorFileIO = nullptr;
        AZ::IO::FileIOBase* m_localFileIO = nullptr;
        EditorInternal::EditorToolsApplication* m_editorsToolApplication;
        UnitTest::ScopedTemporaryDirectory* m_tempDir;
    };

    TEST_F(EditorToolsApplicationFixture, AliasSetup_CreateFile_ValidateAssetPath)
    {
        SetAliasConfig(true, false);
        LmbrCentral::AliasConfiguration config = LmbrCentral::AliasConfigurationStorage::GetConfig();
        ASSERT_TRUE(AZ::StringFunc::Equal(m_tempDir->GetDirectory(), config.m_assetsPath.c_str()));
    }

    TEST_F(EditorToolsApplicationFixture, AliasSetup_DoNotCreateFile_ValidateAssetPath)
    {
        SetAliasConfig(false, false);
        LmbrCentral::AliasConfiguration config = LmbrCentral::AliasConfigurationStorage::GetConfig();
        ASSERT_TRUE(AZ::StringFunc::Equal("Foo", config.m_assetsPath.c_str()));
    }

    TEST_F(EditorToolsApplicationFixture, AliasSetup_DoNotCreateFile_WaitForConnection_True_ValidateAssetPath)
    {
        SetAliasConfig(false, true);
        LmbrCentral::AliasConfiguration config = LmbrCentral::AliasConfigurationStorage::GetConfig();
        ASSERT_TRUE(AZ::StringFunc::Equal(m_tempDir->GetDirectory(), config.m_assetsPath.c_str()));
    }
}
