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

#include <SliceBuilder/Source/UiSliceBuilderWorker.h>
#include <SliceBuilder/Source/TraceDrillerHook.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzCore/IO/IOUtils.h>
#include <AzCore/Debug/Trace.h>
#include <AzCore/Slice/SliceAsset.h>
#include <AzCore/Slice/SliceAssetHandler.h>
#include <AzCore/Slice/SliceComponent.h>
#include <AzCore/Serialization/ObjectStream.h>
#include <AzCore/Serialization/Utils.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <AzToolsFramework/ToolsComponents/EditorOnlyEntityComponentBus.h>
#include <AzToolsFramework/Slice/SliceCompilation.h>
#include <AzCore/Component/ComponentApplication.h>
#include <LyShine/Bus/Tools/UiSystemToolsBus.h>
#include <LyShine/UiAssetTypes.h>

namespace SliceBuilder
{
    static const char* const s_uiSliceBuilder = "UiSliceBuilder";

    void UiSliceBuilderWorker::ShutDown()
    {
        m_isShuttingDown = true;
    }

    AZ::Uuid UiSliceBuilderWorker::GetUUID()
    {
        return AZ::Uuid::CreateString("{2708874f-52e8-48db-bbc4-4c33fa8ceb2e}");
    }

    void UiSliceBuilderWorker::CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response)
    {
        // Check for shutdown
        if (m_isShuttingDown)
        {
            response.m_result = AssetBuilderSDK::CreateJobsResultCode::ShuttingDown;
            return;
        }

        TraceDrillerHook traceDrillerHook(true);

        AZStd::string fullPath;
        AzFramework::StringFunc::Path::ConstructFull(request.m_watchFolder.c_str(), request.m_sourceFile.c_str(), fullPath, false);
        AzFramework::StringFunc::Path::Normalize(fullPath);

        AZ_TracePrintf(s_uiSliceBuilder, "CreateJobs for UI canvas \"%s\"\n", fullPath.c_str());

        // Open the source canvas file
        AZ::IO::FileIOStream stream(fullPath.c_str(), AZ::IO::OpenMode::ModeRead);
        if (!AZ::IO::RetryOpenStream(stream))
        {
            AZ_Warning(s_uiSliceBuilder, false, "CreateJobs for \"%s\" failed because the source file could not be opened.", fullPath.c_str());
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

        // Serialize in the canvas from the stream. This uses the LyShineSystemComponent to do it because
        // it does some complex support for old canvas formats
        UiSystemToolsInterface::CanvasAssetHandle* canvasAsset = nullptr;
        UiSystemToolsBus::BroadcastResult(canvasAsset, &UiSystemToolsInterface::LoadCanvasFromStream, stream, AZ::ObjectStream::FilterDescriptor(assetFilter, AZ::ObjectStream::FilterFlags::FILTERFLAG_IGNORE_UNKNOWN_CLASSES));
        if (!canvasAsset)
        {
            AZ_Error(s_uiSliceBuilder, false, "Compiling UI canvas \"%s\" failed to load canvas from stream.", fullPath.c_str());
            return;
        }

        // Flush asset database events to ensure no asset references are held by closures queued on Ebuses.
        AZ::Data::AssetManager::Instance().DispatchEvents();

        // Fail gracefully if any errors occurred while serializing in the editor UI canvas.
        // i.e. missing assets or serialization errors.
        if (traceDrillerHook.GetErrorCount() > 0)
        {
            AZ_Error(s_uiSliceBuilder, false, "Compiling UI canvas \"%s\" failed due to errors loading editor UI canvas.", fullPath.c_str());
            UiSystemToolsBus::Broadcast(&UiSystemToolsInterface::DestroyCanvas, canvasAsset);
            return;
        }

        const char* compilerVersion = "3";
        for (const AssetBuilderSDK::PlatformInfo& info : request.m_enabledPlatforms)
        {
            AssetBuilderSDK::JobDescriptor jobDescriptor;
            jobDescriptor.m_priority = 0;
            jobDescriptor.m_critical = true;
            jobDescriptor.m_jobKey = "UI Canvas";
            jobDescriptor.SetPlatformIdentifier(info.m_identifier.c_str());
            jobDescriptor.m_additionalFingerprintInfo = AZStd::string(compilerVersion).append(azrtti_typeid<AZ::DynamicSliceAsset>().ToString<AZStd::string>());

            response.m_createJobOutputs.push_back(jobDescriptor);
        }

        response.m_result = AssetBuilderSDK::CreateJobsResultCode::Success;

        UiSystemToolsBus::Broadcast(&UiSystemToolsInterface::DestroyCanvas, canvasAsset);
    }

    void UiSliceBuilderWorker::ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response) const
    {
        // .uicanvas files are converted as they are copied to the cache
        // a) to flatten all prefab instances
        // b) to replace any editor components with runtime components

        // Check for shutdown
        if (m_isShuttingDown)
        {
            AZ_TracePrintf(AssetBuilderSDK::InfoWindow, "Cancelled job %s because shutdown was requested.\n", request.m_sourceFile.c_str());
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Cancelled;
            return;
        }

        TraceDrillerHook traceDrillerHook(true);

        AZStd::string fullPath;
        AZStd::string fileNameOnly;
        AZStd::string outputPath;
        AzFramework::StringFunc::Path::GetFullFileName(request.m_sourceFile.c_str(), fileNameOnly);
        AzFramework::StringFunc::Path::Join(request.m_tempDirPath.c_str(), fileNameOnly.c_str(), outputPath, true, true, true);
        fullPath = request.m_fullPath.c_str();
        AzFramework::StringFunc::Path::Normalize(fullPath);

        AZ_TracePrintf(s_uiSliceBuilder, "Processing UI canvas \"%s\"\n", fullPath.c_str());

        // Open the source canvas file
        AZ::IO::FileIOStream stream(fullPath.c_str(), AZ::IO::OpenMode::ModeRead | AZ::IO::OpenMode::ModeBinary);
        if (!stream.IsOpen())
        {
            AZ_Warning(s_uiSliceBuilder, false, "Compiling UI canvas \"%s\" failed because source file could not be opened.", fullPath.c_str());
            return;
        }

        AZStd::vector<AssetBuilderSDK::ProductDependency> productDependencies;

        // Asset filter is used to record dependencies.  Only returns true for slices
        auto assetFilter = [&productDependencies](const AZ::Data::Asset<AZ::Data::AssetData>& filterAsset)
        {
            productDependencies.emplace_back(filterAsset.GetId(), 0);

            return AZ::Data::AssetFilterSourceSlicesOnly(filterAsset);
        };

        // Serialize in the canvas from the stream. This uses the LyShineSystemComponent to do it because
        // it does some complex support for old canvas formats
        UiSystemToolsInterface::CanvasAssetHandle* canvasAsset = nullptr;
        UiSystemToolsBus::BroadcastResult(canvasAsset, &UiSystemToolsInterface::LoadCanvasFromStream, stream, AZ::ObjectStream::FilterDescriptor(assetFilter, AZ::ObjectStream::FILTERFLAG_IGNORE_UNKNOWN_CLASSES));
        if (!canvasAsset)
        {
            AZ_Error(s_uiSliceBuilder, false, "Compiling UI canvas \"%s\" failed to load canvas from stream.", fullPath.c_str());
            return;
        }

        // Flush asset manager events to ensure no asset references are held by closures queued on Ebuses.
        AZ::Data::AssetManager::Instance().DispatchEvents();

        // Fail gracefully if any errors occurred while serializing in the editor UI canvas.
        // i.e. missing assets or serialization errors.
        if (traceDrillerHook.GetErrorCount() > 0)
        {
            AZ_Error(s_uiSliceBuilder, false, "Compiling UI canvas \"%s\" failed due to errors loading editor UI canvas.", fullPath.c_str());
            UiSystemToolsBus::Broadcast(&UiSystemToolsInterface::DestroyCanvas, canvasAsset);
            return;
        }

        // Get the prefab component from the canvas
        AZ::Entity* canvasSliceEntity = nullptr;
        UiSystemToolsBus::BroadcastResult(canvasSliceEntity, &UiSystemToolsInterface::GetRootSliceEntity, canvasAsset);

        if (!canvasSliceEntity)
        {
            AZ_Error(s_uiSliceBuilder, false, "Compiling UI canvas \"%s\" failed to find the root slice entity.", fullPath.c_str());
            UiSystemToolsBus::Broadcast(&UiSystemToolsInterface::DestroyCanvas, canvasAsset);
            return;
        }

        AZStd::string prefabBuffer;
        AZ::IO::ByteContainerStream<AZStd::string > prefabStream(&prefabBuffer);
        if (!AZ::Utils::SaveObjectToStream<AZ::Entity>(prefabStream, AZ::ObjectStream::ST_XML, canvasSliceEntity))
        {
            AZ_Error(s_uiSliceBuilder, false, "Compiling UI canvas \"%s\" failed due to errors serializing editor UI canvas.", fullPath.c_str());
            UiSystemToolsBus::Broadcast(&UiSystemToolsInterface::DestroyCanvas, canvasAsset);
            return;
        }

        AZ::SerializeContext* context;
        AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationBus::Events::GetSerializeContext);

        prefabStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);

        {
            AZStd::lock_guard<AZStd::mutex> lock(m_processingMutex);

            AZ::Data::Asset<AZ::SliceAsset> sourceSliceAsset;
            sourceSliceAsset.Create(AZ::Data::AssetId(AZ::Uuid::CreateRandom()));
            AZ::SliceAssetHandler assetHandler(context);
            if (!assetHandler.LoadAssetData(sourceSliceAsset, &prefabStream, &AZ::Data::AssetFilterSourceSlicesOnly))
            {
                AZ_Error(s_uiSliceBuilder, false, "Failed to load the serialized Slice Asset.");
                UiSystemToolsBus::Broadcast(&UiSystemToolsInterface::DestroyCanvas, canvasAsset);
                return;
            }

            // Flush sourceSliceAsset manager events to ensure no sourceSliceAsset references are held by closures queued on Ebuses.
            AZ::Data::AssetManager::Instance().DispatchEvents();

            // Fail gracefully if any errors occurred while serializing in the editor UI canvas.
            // i.e. missing assets or serialization errors.
            if (traceDrillerHook.GetErrorCount() > 0)
            {
                AZ_Error(s_uiSliceBuilder, false, "Compiling UI canvas \"%s\" failed due to errors deserializing editor UI canvas.", fullPath.c_str());
                UiSystemToolsBus::Broadcast(&UiSystemToolsInterface::DestroyCanvas, canvasAsset);
                return;
            }

            // Emulate client flags.
            AZ::PlatformTagSet platformTags = { AZ_CRC("renderer", 0xf199a19c) };

            // Compile the source slice into the runtime slice (with runtime components).
            AzToolsFramework::UiEditorOnlyEntityHandler uiEditorOnlyEntityHandler;
            AzToolsFramework::EditorOnlyEntityHandlers handlers =
            {
                &uiEditorOnlyEntityHandler,
            };

            // Get the prefab component from the prefab sourceSliceAsset
            AZ::SliceComponent* sourceSliceData = (sourceSliceAsset.Get()) ? sourceSliceAsset.Get()->GetComponent() : nullptr;

            AZ::SerializeContext* context;
            AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
            if (!context)
                        {
                AZ_Error(s_uiSliceBuilder, context, "Unable to obtain serialize context");
                UiSystemToolsBus::Broadcast(&UiSystemToolsInterface::DestroyCanvas, canvasAsset);
                return;
            }
            

            AzToolsFramework::SliceCompilationResult sliceCompilationResult = AzToolsFramework::CompileEditorSlice(sourceSliceAsset, platformTags, *context, handlers);

            if (!sliceCompilationResult)
            {
                AZ_Error(s_uiSliceBuilder, false, "Failed to export entities for runtime:\n%s", sliceCompilationResult.GetError().c_str());
                UiSystemToolsBus::Broadcast(&UiSystemToolsInterface::DestroyCanvas, canvasAsset);
                return;
            }

            // Get the canvas entity from the canvas
            AZ::Entity* sourceCanvasEntity = nullptr;
            UiSystemToolsBus::BroadcastResult(sourceCanvasEntity, &UiSystemToolsInterface::GetCanvasEntity, canvasAsset);

            if (!sourceCanvasEntity)
            {
                AZ_Error(s_uiSliceBuilder, false, "Compiling UI canvas \"%s\" failed to find the canvas entity.", fullPath.c_str());
                UiSystemToolsBus::Broadcast(&UiSystemToolsInterface::DestroyCanvas, canvasAsset);
                return;
            }

            // create a new canvas entity that will contain the game components rather than editor components
            AZ::Entity exportCanvasEntity(sourceCanvasEntity->GetName().c_str());
            exportCanvasEntity.SetId(sourceCanvasEntity->GetId());

            const AZ::Entity::ComponentArrayType& editorCanvasComponents = sourceCanvasEntity->GetComponents();
            for (AZ::Component* canvasEntityComponent : editorCanvasComponents)
            {
                auto* asEditorComponent =
                    azrtti_cast<AzToolsFramework::Components::EditorComponentBase*>(canvasEntityComponent);

                if (asEditorComponent)
                {
                    size_t oldComponentCount = exportCanvasEntity.GetComponents().size();
                    asEditorComponent->BuildGameEntity(&exportCanvasEntity);
                    if (exportCanvasEntity.GetComponents().size() > oldComponentCount)
                    {
                        AZ::Component* newComponent = exportCanvasEntity.GetComponents().back();
                        AZ_Error("Export", asEditorComponent->GetId() != AZ::InvalidComponentId, "For entity \"%s\", component \"%s\" doesn't have a valid component id",
                            sourceCanvasEntity->GetName().c_str(), asEditorComponent->RTTI_GetType().ToString<AZStd::string>().c_str());
                        newComponent->SetId(asEditorComponent->GetId());
                    }
                }
                else
                {
                    // The component is already runtime-ready. I.e. it is not an editor component.
                    // Clone the component and add it to the export entity
                    AZ::Component* clonedComponent = context->CloneObject(canvasEntityComponent);
                    exportCanvasEntity.AddComponent(clonedComponent);
                }
            }

            // Save runtime UI canvas to disk.
            AZ::IO::FileIOStream outputStream(outputPath.c_str(), AZ::IO::OpenMode::ModeWrite);
            if (outputStream.IsOpen())
            {
                AZ::Data::Asset<AZ::SliceAsset> exportSliceAsset = sliceCompilationResult.GetValue();
                AZ::Entity* exportSliceAssetEntity = exportSliceAsset.Get()->GetEntity();
                AZ::SliceComponent* exportSliceComponent = exportSliceAssetEntity->FindComponent<AZ::SliceComponent>();
                exportSliceAssetEntity->RemoveComponent(exportSliceComponent);

                UiSystemToolsBus::Broadcast(&UiSystemToolsInterface::ReplaceRootSliceSliceComponent, canvasAsset, exportSliceComponent);
                UiSystemToolsBus::Broadcast(&UiSystemToolsInterface::ReplaceCanvasEntity, canvasAsset, &exportCanvasEntity);
                UiSystemToolsBus::Broadcast(&UiSystemToolsInterface::SaveCanvasToStream, canvasAsset, outputStream);
                outputStream.Close();

                // switch them back after we write the file so that the source canvas entity gets freed.
                UiSystemToolsBus::Broadcast(&UiSystemToolsInterface::ReplaceCanvasEntity, canvasAsset, sourceCanvasEntity);

                AZ_TracePrintf(s_uiSliceBuilder, "Output file %s\n", outputPath.c_str());
            }
            else
            {
                AZ_Error(s_uiSliceBuilder, false, "Failed to open output file %s", outputPath.c_str());
                UiSystemToolsBus::Broadcast(&UiSystemToolsInterface::DestroyCanvas, canvasAsset);
                return;
            }

            AssetBuilderSDK::JobProduct jobProduct(outputPath);
            jobProduct.m_productAssetType = azrtti_typeid<LyShine::CanvasAsset>();
            jobProduct.m_productSubID = 0;
            jobProduct.m_dependencies = AZStd::move(productDependencies);
            response.m_outputProducts.push_back(AZStd::move(jobProduct));
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;

            UiSystemToolsBus::Broadcast(&UiSystemToolsInterface::DestroyCanvas, canvasAsset);
        }

        AZ_TracePrintf(s_uiSliceBuilder, "Finished processing uicanvas %s\n", fullPath.c_str());
    }
}
