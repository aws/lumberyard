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

#include "stdafx.h"
#include "LegacyCompiler.h"
#include <AssetBuilderSDK/AssetBuilderSDK.h>

#include <AzFramework/IO/LocalFileIO.h>
#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/UnitTest/UnitTest.h>
#include <AzToolsFramework/Application/ToolsApplication.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzTest/AzTest.h>

class ResourceCompilerLegacyTest
    : public ::testing::Test
    , public AZ::RC::LegacyCompiler
{
protected:
    void SetUp() override
    {
        AZ::AllocatorInstance<AZ::SystemAllocator>::Create();

        m_app.reset(aznew AZ::ComponentApplication());
        AZ::ComponentApplication::Descriptor desc;
        desc.m_useExistingAllocator = true;
        m_app->Create(desc);

        // Startup default local FileIO (hits OSAllocator) if not already setup.
        if (AZ::IO::FileIOBase::GetInstance() == nullptr)
        {
            AZ::IO::FileIOBase::SetInstance(aznew AZ::IO::LocalFileIO());
        }

        AssetBuilderSDK::InitializeSerializationContext();
    }

    void TearDown() override
    {
        delete AZ::IO::FileIOBase::GetInstance();
        AZ::IO::FileIOBase::SetInstance(nullptr);

        m_app->Destroy();
        m_app = nullptr;

        AZ::AllocatorInstance<AZ::SystemAllocator>::Destroy();
    }

    AZStd::unique_ptr<AssetBuilderSDK::ProcessJobRequest> WrapReadJobRequest(const char* folder) const
    {
        return ReadJobRequest(folder);
    }

    bool WrapWriteResponse(const char* folder, AssetBuilderSDK::ProcessJobResponse& response, bool success = true) const
    {
        return WriteResponse(folder, response, success);
    }

    AZStd::unique_ptr<AZ::ComponentApplication> m_app;
};

TEST_F(ResourceCompilerLegacyTest, ReadJobRequest_NoRequestFile_GenerateProcessJobRequest)
{
    // Tests without ProcessJobRequest.xml
    AZStd::string testFileFolder = m_app->GetExecutableFolder();
    testFileFolder += "/../Code/Tools/RC/ResourceCompilerLegacy/Tests";

    AZ_TEST_START_TRACE_SUPPRESSION;
    ASSERT_FALSE(WrapReadJobRequest(testFileFolder.c_str()));
    // Expected error:
    //  Unable to load ProcessJobRequest. Not enough information to process this file
    AZ_TEST_STOP_TRACE_SUPPRESSION(1);
}

TEST_F(ResourceCompilerLegacyTest, ReadJobRequest_ValidRequestFile_GenerateProcessJobRequest)
{
    // Tests reading ProcessJobRequest.xml

    AZStd::string testFileFolder = m_app->GetExecutableFolder();
    testFileFolder += "/../Code/Tools/RC/ResourceCompilerLegacy/Tests/Output";

    ASSERT_TRUE(WrapReadJobRequest(testFileFolder.c_str()));
}

TEST_F(ResourceCompilerLegacyTest, WriteJobResponse_ValidProcessJobResponse_WriteProcessJobResponseToDisk)
{
    // Tests writing ProcessJobResponse.xml to disk

    AZStd::string testFileFolder = m_app->GetExecutableFolder();
    testFileFolder += "/../Code/Tools/RC/ResourceCompilerLegacy/Tests/Output";

    AssetBuilderSDK::ProcessJobResponse response;
    ASSERT_TRUE(WrapWriteResponse(testFileFolder.c_str(), response));

    AZStd::string responseFilePath;
    AzFramework::StringFunc::Path::ConstructFull(testFileFolder.c_str(), AssetBuilderSDK::s_processJobResponseFileName, responseFilePath);
    ASSERT_TRUE(AZ::IO::FileIOBase::GetInstance()->Exists(responseFilePath.c_str()));
    ASSERT_TRUE(AZ::IO::FileIOBase::GetInstance()->Remove(responseFilePath.c_str()));
}

AZ_UNIT_TEST_HOOK();