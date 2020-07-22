/*
* All or portions of this file Copyright(c) Amazon.com, Inc.or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution(the "License").All use of this software is governed by the License,
*or, if provided, by the license below or the license accompanying this file.Do not
* remove or modify any license notices.This file is distributed on an "AS IS" BASIS,
*WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include "precompiled.h"

#include <AssetBuilderSDK/AssetBuilderBusses.h>

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/IOUtils.h>
#include <AzCore/Math/Uuid.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <AzFramework/StringFunc/StringFunc.h>

#include <Builder/ScriptEventsBuilderWorker.h>

#include <Editor/ScriptEventsSystemEditorComponent.h>

#include <ScriptEvents/ScriptEventsAsset.h>

namespace ScriptEventsBuilder
{
    static const char* s_scriptEventsBuilder = "ScriptEventsBuilder";

    Worker::Worker()
    {
    }

    Worker::~Worker()
    {
        Deactivate();
    }

    int Worker::GetVersionNumber() const
    {
        return 1;
    }

    const char* Worker::GetFingerprintString() const
    {
        if (m_fingerprintString.empty())
        {
            // compute it the first time
            const AZStd::string runtimeAssetTypeId = azrtti_typeid<ScriptEvents::ScriptEventsAsset>().ToString<AZStd::string>();
            m_fingerprintString = AZStd::string::format("%i%s", GetVersionNumber(), runtimeAssetTypeId.c_str());
        }
        return m_fingerprintString.c_str();
    }

    void Worker::Activate()
    {
        AZ::Data::AssetType assetType(azrtti_typeid<ScriptEvents::ScriptEventsAsset>());
        // Use AssetCatalog service to register ScriptEvents asset type and extension
        AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequests::AddAssetType, assetType);
        AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequests::EnableCatalogForAsset, assetType);
        AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequests::AddExtension, ".scriptevents");

        m_editorAssetHandler = static_cast<ScriptEventsEditor::ScriptEventAssetHandler*>(AZ::Data::AssetManager::Instance().GetHandler(assetType));
        if (!m_editorAssetHandler)
        {
            m_editorAssetHandler = aznew ScriptEventsEditor::ScriptEventAssetHandler(
                ScriptEvents::ScriptEventsAsset::GetDisplayName(),
                ScriptEvents::ScriptEventsAsset::GetGroup(),
                ".scriptevents"
                );

            AZ::Data::AssetManager::Instance().RegisterHandler(m_editorAssetHandler, assetType);
        }

    }

    void Worker::Deactivate()
    {
        if (m_editorAssetHandler)
        {
            AZ::Data::AssetManager::Instance().UnregisterHandler(m_editorAssetHandler);
            m_editorAssetHandler = nullptr;
        }
    }

    void Worker::ShutDown()
    {
        m_isShuttingDown = true;
    }

    void Worker::CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response) const
    {
        AZStd::string fullPath;
        AzFramework::StringFunc::Path::ConstructFull(request.m_watchFolder.data(), request.m_sourceFile.data(), fullPath, false);
        AzFramework::StringFunc::Path::Normalize(fullPath);

        AZ_TracePrintf(s_scriptEventsBuilder, "CreateJobs for script events\"%s\"\n", fullPath.data());

        if (!m_editorAssetHandler)
        {
            AZ_Error(s_scriptEventsBuilder, false, R"(CreateJobs for %s failed because the ScriptEvents Editor Asset handler is missing.)", fullPath.data());
        }

        AZ::IO::FileIOStream stream(fullPath.c_str(), AZ::IO::OpenMode::ModeRead);
        if (!AZ::IO::RetryOpenStream(stream))
        {
            AZ_Warning(s_scriptEventsBuilder, false, "CreateJobs for \"%s\" failed because the source file could not be opened.", fullPath.data());
            return;
        }

        AZ::Data::Asset<ScriptEvents::ScriptEventsAsset> asset;
        asset.Create(AZ::Data::AssetId(AZ::Uuid::CreateRandom()));
        if (!m_editorAssetHandler->LoadAssetData(asset, &stream, nullptr))
        {
            AZ_Warning(s_scriptEventsBuilder, false, "CreateJobs for \"%s\" failed because the asset data could not be loaded from the file", fullPath.data());
            return;
        }

        // Flush asset database events to ensure no asset references are held by closures queued on Ebuses.
        AZ::Data::AssetManager::Instance().DispatchEvents();

        for (const AssetBuilderSDK::PlatformInfo& info : request.m_enabledPlatforms)
        {
            AssetBuilderSDK::JobDescriptor jobDescriptor;
            jobDescriptor.m_priority = 2;
            jobDescriptor.m_critical = true;
            jobDescriptor.m_jobKey = "Script Events";
            jobDescriptor.SetPlatformIdentifier(info.m_identifier.data());
            jobDescriptor.m_additionalFingerprintInfo = GetFingerprintString();

            response.m_createJobOutputs.push_back(jobDescriptor);
        }

        response.m_result = AssetBuilderSDK::CreateJobsResultCode::Success;
    }

    void Worker::ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response) const
    {
        // A runtime script events component is generated, which creates a .scriptevents_compiled file
        AZStd::string fullPath;
        AZStd::string fileNameOnly;
        AzFramework::StringFunc::Path::GetFullFileName(request.m_sourceFile.c_str(), fileNameOnly);
        fullPath = request.m_fullPath.c_str();
        AzFramework::StringFunc::Path::Normalize(fullPath);

        AZ_TracePrintf(s_scriptEventsBuilder, "Processing script events \"%s\".\n", fullPath.c_str());

        if (!m_editorAssetHandler)
        {
            AZ_Error(s_scriptEventsBuilder, false, R"(Exporting of .ScriptEvents for "%s" file failed as no editor asset handler was registered for scrit events. The ScriptEvents Gem might not be enabled.)", fullPath.data());
            return;
        }

        AZ::IO::FileIOStream stream(fullPath.c_str(), AZ::IO::OpenMode::ModeRead);
        if (!stream.IsOpen())
        {
            AZ_Warning(s_scriptEventsBuilder, false, "Exporting of .ScriptEvents for \"%s\" failed because the source file could not be opened.", fullPath.c_str());
            return;
        }

        AZ::SerializeContext* context{};
        AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationBus::Events::GetSerializeContext);

        AZStd::vector<AssetBuilderSDK::ProductDependency> productDependencies;

        AZ_TracePrintf(s_scriptEventsBuilder, "Script Events Asset preload\n");
        AZ::Data::Asset<ScriptEvents::ScriptEventsAsset> asset;
        asset.Create(AZ::Data::AssetId(AZ::Uuid::CreateRandom()));
        if (!m_editorAssetHandler->LoadAssetData(asset, &stream, nullptr))
        {
            AZ_Error(s_scriptEventsBuilder, false, R"(Loading of ScriptEvents asset for source file "%s" has failed)", fullPath.data());
            return;
        }

        AZ_TracePrintf(s_scriptEventsBuilder, "Script Events Asset loaded successfully\n");

        // Flush asset manager events to ensure no asset references are held by closures queued on Ebuses.
        AZ::Data::AssetManager::Instance().DispatchEvents();

        AZStd::string runtimeScriptEventsOutputPath;
        AzFramework::StringFunc::Path::Join(request.m_tempDirPath.c_str(), fileNameOnly.c_str(), runtimeScriptEventsOutputPath, true, true, true);

        ScriptEvents::ScriptEvent definition = asset.Get()->m_definition;
        definition.Flatten();

        // Populate the runtime Asset 
        AZStd::vector<AZ::u8> byteBuffer;
        AZ::IO::ByteContainerStream<decltype(byteBuffer)> byteStream(&byteBuffer);

        AZ::Data::Asset<ScriptEvents::ScriptEventsAsset> productionAsset;
        productionAsset.Create(AZ::Uuid::CreateRandom());
        productionAsset.Get()->m_definition = AZStd::move(definition);

        m_editorAssetHandler->SetSaveAsBinary(true);

        AZ_TracePrintf(s_scriptEventsBuilder, "Script Events Asset presave to object stream for %s\n", fullPath.c_str());
        bool productionAssetSaved = m_editorAssetHandler->SaveAssetData(productionAsset, &byteStream);

        if (!productionAssetSaved)
        {
            AZ_Error(s_scriptEventsBuilder, productionAssetSaved, "Failed to save runtime Script Events to object stream");
            return;
        }
        AZ_TracePrintf(s_scriptEventsBuilder, "Script Events Asset has been saved to the object stream successfully\n");

        // TODO: make this binary
        AZ::IO::FileIOStream outFileStream(runtimeScriptEventsOutputPath.data(), AZ::IO::OpenMode::ModeWrite);
        if (!outFileStream.IsOpen())
        {
            AZ_Error(s_scriptEventsBuilder, false, "Failed to open output file %s", runtimeScriptEventsOutputPath.data());
            return;
        }

        productionAssetSaved = outFileStream.Write(byteBuffer.size(), byteBuffer.data()) == byteBuffer.size() && productionAssetSaved;
        if (!productionAssetSaved)
        {
            AZ_Error(s_scriptEventsBuilder, productionAssetSaved, "Unable to save runtime Script Events file %s", runtimeScriptEventsOutputPath.data());
            return;
        }

        // ScriptEvents Editor Asset Copy job
        // The SubID is zero as this represents the main asset
        AssetBuilderSDK::JobProduct jobProduct;
        jobProduct.m_productFileName = runtimeScriptEventsOutputPath;
        jobProduct.m_productAssetType = azrtti_typeid<ScriptEvents::ScriptEventsAsset>();
        jobProduct.m_productSubID = 0;
        jobProduct.m_dependenciesHandled = true; // This builder has no product dependencies.
        response.m_outputProducts.push_back(AZStd::move(jobProduct));

        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;

        AZ_TracePrintf(s_scriptEventsBuilder, "Finished processing Script Events %s\n", fullPath.c_str());
    }

    AZ::Uuid Worker::GetUUID()
    {
        return AZ::Uuid::CreateString("{CD64F85A-0147-45EF-B02A-9828E25D99EB}");
    }

}
