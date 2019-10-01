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
#include "ImageProcessing_precompiled.h"
#include "Source/AtlasBuilder/AtlasBuilderWorker.h"
#include "Source/ImageBuilderComponent.h"

#include <BuilderSettings/BuilderSettingManager.h>
#include <Processing/PixelFormatInfo.h>

#include <AzCore/Component/ComponentApplication.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzTest/AzTest.h>


using namespace TextureAtlasBuilder;
using namespace ImageProcessing;

#if defined(AZ_PLATFORM_APPLE_OSX)
#   define AZ_ROOT_TEST_FOLDER "./"
#else
#   define AZ_ROOT_TEST_FOLDER ""
#endif

namespace UnitTest
{
    class AtlasBuilderTest
        : public ::testing::Test
        , public AzToolsFramework::AssetSystemRequestBus::Handler
    {
    protected:
        void SetUp() override
        {
            AZ::AllocatorInstance<AZ::SystemAllocator>::Create();

            m_app.reset(aznew AZ::ComponentApplication());
            AZ::ComponentApplication::Descriptor desc;
            desc.m_useExistingAllocator = true;
            m_app->Create(desc);

            BuilderSettingManager::CreateInstance();

            m_context = AZStd::make_unique<AZ::SerializeContext>();
            BuilderPluginComponent::Reflect(m_context.get());

            // Startup default local FileIO (hits OSAllocator) if not already setup.
            if (AZ::IO::FileIOBase::GetInstance() == nullptr)
            {
                AZ::IO::FileIOBase::SetInstance(aznew AZ::IO::LocalFileIO());
            }

            AzToolsFramework::AssetSystemRequestBus::Handler::BusConnect();
        }

        void TearDown() override
        {
            AzToolsFramework::AssetSystemRequestBus::Handler::BusDisconnect();

            delete AZ::IO::FileIOBase::GetInstance();
            AZ::IO::FileIOBase::SetInstance(nullptr);

            m_app->Destroy();
            m_app = nullptr;

            m_context.release();
            BuilderSettingManager::DestroyInstance();
            CPixelFormats::DestroyInstance();

            AZ::AllocatorInstance<AZ::SystemAllocator>::Destroy();
        }

        AZStd::string GetFullPath(AZStd::string_view fileName)
        {
            AZStd::string testsFolder = m_app->GetExecutableFolder();

            if (testsFolder.ends_with("/"))
            {
                testsFolder = testsFolder.substr(0, testsFolder.length() - 1);
            }

            // Get rid of strings like "/Bin64vc141.Debug.Test" at the end of the executable folder and generate the actual full path
            size_t found = testsFolder.find_last_of("/");
            if (found != AZStd::string::npos)
            {
                testsFolder = testsFolder.substr(0, found);
            }

            testsFolder += "/Gems/ImageProcessing/Code/Tests/";
            return AZStd::string::format("%s%.*s", testsFolder.c_str(), fileName.size(), fileName.data());
        }

        AZStd::unique_ptr<AZ::SerializeContext> m_context;
        AZStd::unique_ptr<AZ::ComponentApplication> m_app;

    public:
        AssetBuilderSDK::ProcessJobRequest CreateTestJobRequest(
            const AZStd::string& testFileName, 
            const AZStd::string& watchFolder,
            const AZStd::string& tempDirPath,
            bool critical, 
            QString platform, 
            AZ::s64 jobId = 0)
        {
            AZStd::string fullPath;
            AzFramework::StringFunc::Path::Join(
                watchFolder.c_str(), testFileName.c_str(), fullPath, true, true, true);

            bool valid = true;
            AtlasBuilderInput testInput = AtlasBuilderInput::ReadFromFile(fullPath, watchFolder, valid);

            AssetBuilderSDK::ProcessJobRequest request;
            request.m_sourceFile = testFileName;
            request.m_fullPath = fullPath;
            request.m_tempDirPath = tempDirPath;
            request.m_jobId = jobId;
            request.m_platformInfo.m_identifier = platform.toUtf8().constData();
            request.m_jobDescription = AtlasBuilderWorker::GetJobDescriptor(testFileName, testInput);

            return request;
        }

        AZStd::string GetTestFolderPath()
        {
            return AZ_ROOT_TEST_FOLDER;
        }

        //////////////////////////////////////////////////////////////////////////
        // AssetSystemRequestBus
        bool GetAbsoluteAssetDatabaseLocation(AZStd::string& result) override { return false; };
        const char* GetAbsoluteDevGameFolderPath() override { return ""; };
        const char* GetAbsoluteDevRootFolderPath() override { return ""; };
        bool GetRelativeProductPathFromFullSourceOrProductPath(const AZStd::string& fullPath, AZStd::string& outputPath) override { return false; };
        bool GetFullSourcePathFromRelativeProductPath(const AZStd::string& relPath, AZStd::string& fullPath) override { return false; };
        bool GetAssetInfoById(const AZ::Data::AssetId& assetId, const AZ::Data::AssetType& assetType, AZ::Data::AssetInfo& assetInfo, AZStd::string& rootFilePath) override { return false; };
        bool GetSourceInfoBySourcePath(const char* sourcePath, AZ::Data::AssetInfo& assetInfo, AZStd::string& watchFolder) override
        {
            assetInfo.m_relativePath = sourcePath;
            watchFolder = GetFullPath("TestAssets");
            return true;
        };
        bool GetSourceInfoBySourceUUID(const AZ::Uuid& sourceUuid, AZ::Data::AssetInfo& assetInfo, AZStd::string& watchFolder) override { return false; };
        bool GetScanFolders(AZStd::vector<AZStd::string>& scanFolders) override { return false; };
        bool GetAssetSafeFolders(AZStd::vector<AZStd::string>& assetSafeFolders) override { return false; };
        bool IsAssetPlatformEnabled(const char* platform) override { return false; };
        int GetPendingAssetsForPlatform(const char* platform) override { return -1; };
        bool GetAssetsProducedBySourceUUID(const AZ::Uuid& sourceUuid, AZStd::vector<AZ::Data::AssetInfo>& productsAssetInfo) override { return false; };
    };

    TEST_F(AtlasBuilderTest, ProcessJob_ProcessValidTextureAtlas_OutputProductDependencies)
    {
        AZStd::string builderSetting("../Gems/ImageProcessing/Code/Source/ImageBuilderDefaultPresets.settings");
        auto outcome = BuilderSettingManager::Instance()->LoadBuilderSettings(builderSetting, m_context.get());

        // create the test job
        AssetBuilderSDK::ProcessJobRequest request = CreateTestJobRequest(
            "TextureAtlasTest.texatlas", GetFullPath("TestAssets"), GetTestFolderPath(), false, BuilderSettingManager::s_defaultPlatform.c_str(), 1);

        AssetBuilderSDK::ProcessJobResponse response;

        AtlasBuilderWorker testBuilder;
        testBuilder.ProcessJob(request, response);

        ASSERT_EQ(response.m_resultCode, AssetBuilderSDK::ProcessJobResultCode::ProcessJobResult_Success);

        // texture atlas builder only has two output products
        ASSERT_EQ(response.m_outputProducts.size(), 2);
        
        // textureatlasidx depends on dds its paired with, but not the other way around
        AZ::Data::AssetId ddsProductAssetId(request.m_sourceFileUUID, response.m_outputProducts[static_cast<int>(Product::DdsProduct)].m_productSubID);
        AZStd::vector<AssetBuilderSDK::ProductDependency> textureatlasidxProductDependencies = response.m_outputProducts[static_cast<int>(Product::TexatlasidxProduct)].m_dependencies;
        ASSERT_EQ(textureatlasidxProductDependencies.size(), 1);
        ASSERT_EQ(textureatlasidxProductDependencies[0].m_dependencyId, ddsProductAssetId);

        AZStd::vector<AssetBuilderSDK::ProductDependency> ddsProductDependencies = response.m_outputProducts[static_cast<int>(Product::DdsProduct)].m_dependencies;
        ASSERT_EQ(ddsProductDependencies.size(), 0);
    }
}