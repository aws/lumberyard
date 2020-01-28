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

#include <SliceBuilder/Source/SliceBuilderWorker.h>
#include <AssetBuilderSDK/SerializationDependencies.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzCore/Debug/Trace.h>
#include <AzCore/std/sort.h>
#include <AzCore/IO/IOUtils.h>
#include <AzCore/Slice/SliceAsset.h>
#include <AzCore/Slice/SliceAssetHandler.h>
#include <AzCore/Slice/SliceComponent.h>
#include <AzCore/Serialization/ObjectStream.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Component/ComponentExport.h>
#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <AzToolsFramework/Slice/SliceCompilation.h>
#include <AzToolsFramework/Slice/SliceUtilities.h>
#include "AzFramework/Asset/SimpleAsset.h"

namespace SliceBuilder
{
    namespace
    {
        AzToolsFramework::Fingerprinting::TypeFingerprint CalculateFingerprintForComponentServices(AZ::SliceComponent& slice)
        {
            AzToolsFramework::Fingerprinting::TypeFingerprint result = 0;

            // Ensures hash will change if a service is moved from, say, required to dependent.
            static const size_t kRequiredServiceKey = AZ_CRC("RequiredServiceKey", 0x22e125a6);
            static const size_t kDependentServiceKey = AZ_CRC("DependentServiceKey", 0x380e6c63);
            static const size_t kProvidedServiceKey = AZ_CRC("ProvidedServiceKey", 0xd3cc7058);
            static const size_t kIncompatibleServiceKey = AZ_CRC("IncompatibleServiceKey", 0x95ee560f);

            // Hash service configuration for all components within the slice.
            AZStd::vector<AZ::Entity*> entities;
            slice.GetEntities(entities);
            AZ::ComponentDescriptor::DependencyArrayType services;
            for (AZ::Entity* entity : entities)
            {
                for (AZ::Component* component : entity->GetComponents())
                {
                    AZ::ComponentDescriptor* componentDescriptor = nullptr;
                    AZ::ComponentDescriptorBus::EventResult(componentDescriptor, component->RTTI_GetType(), &AZ::ComponentDescriptorBus::Events::GetDescriptor);
                    if (componentDescriptor)
                    {
                        services.clear();
                        componentDescriptor->GetRequiredServices(services, component);
                        AZStd::sort(services.begin(), services.end());
                        AZStd::hash_combine(result, kRequiredServiceKey);
                        AZStd::hash_range(result, services.begin(), services.end());

                        services.clear();
                        componentDescriptor->GetDependentServices(services, component);
                        AZStd::sort(services.begin(), services.end());
                        AZStd::hash_combine(result, kDependentServiceKey);
                        AZStd::hash_range(result, services.begin(), services.end());

                        services.clear();
                        componentDescriptor->GetProvidedServices(services, component);
                        AZStd::sort(services.begin(), services.end());
                        AZStd::hash_combine(result, kProvidedServiceKey);
                        AZStd::hash_range(result, services.begin(), services.end());

                        services.clear();
                        componentDescriptor->GetIncompatibleServices(services, component);
                        AZStd::sort(services.begin(), services.end());
                        AZStd::hash_combine(result, kIncompatibleServiceKey);
                        AZStd::hash_range(result, services.begin(), services.end());
                    }
                }
            }

            return result;
        }

        AzToolsFramework::Fingerprinting::TypeFingerprint CalculateFingerprintForSlice(AZ::SliceComponent& slice, const AzToolsFramework::Fingerprinting::TypeFingerprinter& typeFingerprinter, AZ::SerializeContext& serializeContext)
        {
            AzToolsFramework::Fingerprinting::TypeFingerprint serializationFingerprint = typeFingerprinter.GenerateFingerprintForAllTypesInObject(&slice);
            AzToolsFramework::Fingerprinting::TypeFingerprint servicesFingerprint = CalculateFingerprintForComponentServices(slice);

            AZStd::hash_combine(serializationFingerprint, servicesFingerprint);
            return serializationFingerprint;
        }
    } // namespace anonymous

    static const char* const s_sliceBuilder = "SliceBuilder";

    SliceBuilderWorker::SliceBuilderWorker()
    {
        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
        AZ_Assert(serializeContext, "SerializeContext not found");
        m_typeFingerprinter = AZStd::make_unique<AzToolsFramework::Fingerprinting::TypeFingerprinter>(*serializeContext);

        AssetBuilderSDK::AssetBuilderCommandBus::Handler::BusConnect(GetUUID());
    }

    SliceBuilderWorker::~SliceBuilderWorker() = default;

    void SliceBuilderWorker::ShutDown()
    {
        m_isShuttingDown = true;
    }

    void SliceBuilderWorker::CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response) const
    {
        // Check for shutdown
        if (m_isShuttingDown)
        {
            response.m_result = AssetBuilderSDK::CreateJobsResultCode::ShuttingDown;
            return;
        }

        AZStd::string fullPath;
        AzFramework::StringFunc::Path::ConstructFull(request.m_watchFolder.c_str(), request.m_sourceFile.c_str(), fullPath, false);
        AzFramework::StringFunc::Path::Normalize(fullPath);

        AssetBuilderSDK::AssertAndErrorAbsorber assertAndErrorAbsorber(true);

        AZ_TracePrintf(s_sliceBuilder, "CreateJobs for slice \"%s\"\n", fullPath.c_str());

        // Serialize in the source slice to determine if we need to generate a .dynamicslice.
        AZ::IO::FileIOStream stream(fullPath.c_str(), AZ::IO::OpenMode::ModeRead);
        if (!AZ::IO::RetryOpenStream(stream))
        {
            AZ_Warning(s_sliceBuilder, false, "CreateJobs for \"%s\" failed because the source file could not be opened.", fullPath.c_str());
            return;
        }

        // Asset filter always returns false to prevent parsing dependencies, but makes note of the slice dependencies
        auto assetFilter = [&response](const AZ::Data::Asset<AZ::Data::AssetData>& asset)
        {
            if (asset.GetType() == AZ::AzTypeInfo<AZ::SliceAsset>::Uuid())
            {
                bool isSliceDependency = (asset.GetAutoLoadBehavior() != AZ::Data::AssetLoadBehavior::NoLoad);

                if (isSliceDependency)
                {
                    AssetBuilderSDK::SourceFileDependency dependency;
                    dependency.m_sourceFileDependencyUUID = asset.GetId().m_guid;

                    response.m_sourceFileDependencyList.push_back(dependency);
                }
            }

            return false;
        };

        AZ::Data::Asset<AZ::SliceAsset> asset;
        asset.Create(AZ::Data::AssetId(AZ::Uuid::CreateRandom()));
        AZ::SliceAssetHandler assetHandler;
        assetHandler.SetFilterFlags(AZ::ObjectStream::FilterFlags::FILTERFLAG_IGNORE_UNKNOWN_CLASSES);
        assetHandler.LoadAssetData(asset, &stream, assetFilter);

        // Flush asset database events to ensure no asset references are held by closures queued on Ebuses.
        AZ::Data::AssetManager::Instance().DispatchEvents();

        // Fail gracefully if any errors occurred while serializing in the editor slice.
        // i.e. missing assets or serialization errors.
        if (assertAndErrorAbsorber.GetErrorCount() > 0)
        {
            AZ_Error("", false, "Exporting of createjobs response for \"%s\" failed due to errors loading editor slice.", fullPath.c_str());
            return;
        }

        AZ::SliceComponent* sourcePrefab = (asset.Get()) ? asset.Get()->GetComponent() : nullptr;

        if (!sourcePrefab)
        {
            AZ_Error(s_sliceBuilder, false, "Failed find the slice component in the slice asset!");
            return; // this should fail!
        }

        if (sourcePrefab->IsDynamic())
        {
            AZ::SerializeContext* context;
            AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
            AzToolsFramework::Fingerprinting::TypeFingerprint sourceSliceTypeFingerprint = CalculateFingerprintForSlice(*sourcePrefab, *m_typeFingerprinter, *context);

            const char* compilerVersion = "8";

            for (const AssetBuilderSDK::PlatformInfo& info : request.m_enabledPlatforms)
            {
                AssetBuilderSDK::JobDescriptor jobDescriptor;
                jobDescriptor.m_priority = 0;
                jobDescriptor.m_critical = true;
                jobDescriptor.m_jobKey = "Dynamic Slice";
                jobDescriptor.SetPlatformIdentifier(info.m_identifier.c_str());
                jobDescriptor.m_additionalFingerprintInfo = AZStd::string(compilerVersion)
                    .append(AZStd::string::format("|%llu", static_cast<uint64_t>(sourceSliceTypeFingerprint)));
                response.m_createJobOutputs.push_back(jobDescriptor);
            }
        }

        response.m_result = AssetBuilderSDK::CreateJobsResultCode::Success;
    }

    void SliceBuilderWorker::ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response) const
    {

        // Check for shutdown
        if (m_isShuttingDown)
        {
            AZ_TracePrintf(AssetBuilderSDK::InfoWindow, "Cancelled job %s because shutdown was requested.\n", request.m_sourceFile.c_str());
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Cancelled;
            return;
        }
        AZStd::string fullPath;
        AZStd::string fileNameOnly;
        AzFramework::StringFunc::Path::GetFullFileName(request.m_sourceFile.c_str(), fileNameOnly);
        fullPath = request.m_fullPath.c_str();
        AzFramework::StringFunc::Path::Normalize(fullPath);

        AZ_TracePrintf(s_sliceBuilder, "Processing slice \"%s\".\n", fullPath.c_str());
        
        // Serialize in the source slice to determine if we need to generate a .dynamicslice.
        AZ::IO::FileIOStream stream(fullPath.c_str(), AZ::IO::OpenMode::ModeRead);
        if (!stream.IsOpen())
        {
            AZ_Warning(s_sliceBuilder, false, "Exporting of .dynamicslice for \"%s\" failed because the source file could not be opened.", fullPath.c_str());
            return;
        }

        AZ::Data::Asset<AZ::SliceAsset> exportSliceAsset;
        AZStd::vector<AssetBuilderSDK::ProductDependency> productDependencies;
        AssetBuilderSDK::ProductPathDependencySet productPathDependencySet;
        AZ::PlatformTagSet platformTags;

        const AZStd::unordered_set<AZStd::string>& platformTagStrings = request.m_platformInfo.m_tags;
        for (const AZStd::string& platformTagString : platformTagStrings)
        {
            platformTags.insert(AZ::Crc32(platformTagString.c_str(), platformTagString.size(), true));
        }

        if (GetDynamicSliceAssetAndDependencies(&stream, fullPath.c_str(), platformTags, exportSliceAsset, productDependencies, productPathDependencySet))
        {
            AZStd::string dynamicSliceOutputPath;
            AzFramework::StringFunc::Path::Join(request.m_tempDirPath.c_str(), fileNameOnly.c_str(), dynamicSliceOutputPath, true, true, true);
            AzFramework::StringFunc::Path::ReplaceExtension(dynamicSliceOutputPath, "dynamicslice");

            // Save runtime slice to disk.
            // Use SaveObjectToFile because it writes to a byte stream first and then to disk
            // which is much faster than SaveObjectToStream(outputStream...) when writing large slices
            if (AZ::Utils::SaveObjectToFile<AZ::Entity>(dynamicSliceOutputPath.c_str(), AzToolsFramework::SliceUtilities::GetSliceStreamFormat(), exportSliceAsset.Get()->GetEntity()))
            {
                AZ_TracePrintf(s_sliceBuilder, "Output file %s", dynamicSliceOutputPath.c_str());
            }
            else
            {
                AZ_Error(s_sliceBuilder, false, "Failed to open output file %s", dynamicSliceOutputPath.c_str());
                return;
            }

            AssetBuilderSDK::JobProduct jobProduct(dynamicSliceOutputPath);
            jobProduct.m_productAssetType = azrtti_typeid<AZ::DynamicSliceAsset>();
            jobProduct.m_productSubID = 2;
            jobProduct.m_dependencies = AZStd::move(productDependencies);
            jobProduct.m_pathDependencies = AZStd::move(productPathDependencySet);
            response.m_outputProducts.push_back(AZStd::move(jobProduct));
        }

        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;

        AZ_TracePrintf(s_sliceBuilder, "Finished processing slice %s\n", fullPath.c_str());
    }

    bool SliceBuilderWorker::GetDynamicSliceAssetAndDependencies(AZ::IO::GenericStream* stream, const char* fullPath, const AZ::PlatformTagSet& platformTags, AZ::Data::Asset<AZ::SliceAsset>& outSliceAsset, AZStd::vector<AssetBuilderSDK::ProductDependency>& outProductDependencies, AssetBuilderSDK::ProductPathDependencySet& productPathDependencySet) const
    {
        AssetBuilderSDK::AssertAndErrorAbsorber assertAndErrorAbsorber(true);

        AZ::SerializeContext* context;
        AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationBus::Events::GetSerializeContext);

        AZ::Data::Asset<AZ::SliceAsset> sourceAsset;
        sourceAsset.Create(AZ::Data::AssetId(AZ::Uuid::CreateRandom()));

        AZ::SliceAssetHandler assetHandler(context);
        assetHandler.LoadAssetData(sourceAsset, stream, &AZ::Data::AssetFilterSourceSlicesOnly);
        sourceAsset.SetHint(fullPath);

        // Flush asset manager events to ensure no asset references are held by closures queued on Ebuses.
        AZ::Data::AssetManager::Instance().DispatchEvents();

        // Fail gracefully if any errors occurred while serializing in the editor slice.
        // i.e. missing assets or serialization errors.
        if (assertAndErrorAbsorber.GetErrorCount() > 0)
        {
            AZ_Error(s_sliceBuilder, false, "Exporting of .dynamicslice for \"%s\" failed due to errors loading editor slice.", fullPath);
            return false;
        }

        // If the slice is designated as dynamic, generate the dynamic slice (.dynamicslice).
        AZ::SliceComponent* sourceSlice = (sourceAsset.Get()) ? sourceAsset.Get()->GetComponent() : nullptr;

        if (!sourceSlice)
        {
            AZ_Error(s_sliceBuilder, false, "Failed to find the slice component in the slice asset!");
            return false; // this should fail!
        }

        if (!sourceSlice->IsDynamic())
        {
            AZ_Error(s_sliceBuilder, false, "Slice must be a dynamicslice in order to be processed: %s\n", fullPath);
            return false;
        }

        if (assertAndErrorAbsorber.GetErrorCount() > 0)
        {
            AZ_Error(s_sliceBuilder, false, "Exporting of .dynamicslice for \"%s\" failed due to errors instantiating entities.", fullPath);
            return false;
        }

        AZ::SliceComponent::EntityList sourceEntities;
        sourceSlice->GetEntities(sourceEntities);

        // Compile the source slice into the runtime slice (with runtime components). Note that
        // we may be handling either world or UI entities, so we need handlers for both.
        AzToolsFramework::WorldEditorOnlyEntityHandler worldEditorOnlyEntityHandler;
        AzToolsFramework::UiEditorOnlyEntityHandler uiEditorOnlyEntityHandler;
        AzToolsFramework::EditorOnlyEntityHandlers handlers =
        {
            &worldEditorOnlyEntityHandler,
            &uiEditorOnlyEntityHandler,
        };
        AzToolsFramework::SliceCompilationResult sliceCompilationResult =
            AzToolsFramework::CompileEditorSlice(sourceAsset, platformTags, *context, handlers);

        if (!sliceCompilationResult)
        {
            AZ_Error("Slice compilation", false, "Slice compilation failed: %s", sliceCompilationResult.GetError().c_str());
            return false;
        }

        outSliceAsset = sliceCompilationResult.GetValue();

        // Gather product dependencies from the compiled asset, not the source asset. In some cases asset references can change during asset compliation.
        AssetBuilderSDK::GatherProductDependencies(*context, outSliceAsset.Get()->GetEntity(), outProductDependencies, productPathDependencySet);

        return true;
    }

    AZ::Uuid SliceBuilderWorker::GetUUID()
    {
        return AZ::Uuid::CreateString("{b92ad60c-d301-4484-8647-bb889ed717a2}");
    }
}
