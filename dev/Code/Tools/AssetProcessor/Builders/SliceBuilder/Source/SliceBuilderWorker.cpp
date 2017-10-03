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

#include <SliceBuilder/Source/SliceBuilderWorker.h>
#include <SliceBuilder/Source/TraceDrillerHook.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzCore/Debug/Trace.h>
#include <AzCore/Slice/SliceAsset.h>
#include <AzCore/Slice/SliceAssetHandler.h>
#include <AzCore/Slice/SliceComponent.h>
#include <AzCore/Serialization/ObjectStream.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/std/sort.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <AzCore/Component/ComponentApplication.h>

namespace SliceBuilder
{
    namespace
    {
        TypeFingerprint GetFingerprintForAllTypesInSlice(const AZ::SliceComponent& slice, const TypeFingerprinter& typeFingerprinter, AZ::SerializeContext& serializeContext)
        {
            // Find all unique types contained in slice.
            AZStd::unordered_set<AZ::TypeId> typesInSlice;
            AZ::SerializeContext::BeginElemEnumCB elementCallback =
                [&typesInSlice](void* /* instance pointer */, const AZ::SerializeContext::ClassData* classData, const AZ::SerializeContext::ClassElement* /* classElement*/)
                {
                    typesInSlice.insert(classData->m_typeId);
                    return true;
                };

            serializeContext.EnumerateObject(&slice, elementCallback, nullptr, AZ::SerializeContext::ENUM_ACCESS_FOR_READ);

            // TODO: find all types contained in DataPatches

            // Sort all types before fingerprinting
            AZStd::vector<AZ::TypeId> sortedTypes;
            sortedTypes.reserve(typesInSlice.size());
            sortedTypes.insert(sortedTypes.begin(), typesInSlice.begin(), typesInSlice.end());
            AZStd::sort(sortedTypes.begin(), sortedTypes.end());
        
            // Create a fingerprint from all types found in the slice.
            TypeFingerprint fingerprint = 0;
            for (const AZ::TypeId& typeId : sortedTypes)
            {
                AZStd::hash_combine(fingerprint, typeFingerprinter.GetFingerprint(typeId));
            }

            return fingerprint;
        }
    } // namespace anonymous

    static const char* const s_sliceBuilder = "SliceBuilder";

    SliceBuilderWorker::SliceBuilderWorker()
    {
        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
        AZ_Assert(serializeContext, "SerializeContext not found");
        if (serializeContext)
        {
            m_typeFingerprinter.CreateFingerprintsForAllTypes(*serializeContext);
        }

        AssetBuilderSDK::AssetBuilderCommandBus::Handler::BusConnect(GetUUID());
    }

    void SliceBuilderWorker::ShutDown()
    {
        m_isShuttingDown = true;
    }

    void SliceBuilderWorker::CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response) const
    {
        AZStd::string fullPath;
        AzFramework::StringFunc::Path::ConstructFull(request.m_watchFolder.c_str(), request.m_sourceFile.c_str(), fullPath, false);
        AzFramework::StringFunc::Path::Normalize(fullPath);

        TraceDrillerHook traceDrillerHook(true);

        AZ_TracePrintf(s_sliceBuilder, "CreateJobs for slice \"%s\"", fullPath.c_str());

        // Serialize in the source slice to determine if we need to generate a .dynamicslice.
        AZ::IO::FileIOStream stream(fullPath.c_str(), AZ::IO::OpenMode::ModeRead);
        if (!stream.IsOpen())
        {
            AZ_Warning(s_sliceBuilder, false, "CreateJobs for \"%s\" failed because the source file could not be opened.", fullPath.c_str());
            return;
        }

        // Asset filter always returns false to prevent parsing dependencies, but makes note of the slice dependencies
        auto assetFilter = [&response](const AZ::Data::Asset<AZ::Data::AssetData>& asset)
        {
            if (asset.GetType() == AZ::AzTypeInfo<AZ::SliceAsset>::Uuid())
            {
                bool isSliceDependency = (0 == (asset.GetFlags() & static_cast<AZ::u8>(AZ::Data::AssetFlags::OBJECTSTREAM_NO_LOAD)));

                if (isSliceDependency)
                {
                    AssetBuilderSDK::SourceFileDependency dependency;
                    dependency.m_sourceFileDependencyUUID = asset.GetId().m_guid;

                    response.m_sourceFileDependencyList.push_back(dependency);
                }
            }
            else if (asset.GetType() == AZ::Uuid("{FA10C3DA-0717-4B72-8944-CD67D13DFA2B}")) // ScriptCanvasAsset
            {
                AssetBuilderSDK::SourceFileDependency dependency;
                dependency.m_sourceFileDependencyUUID = asset.GetId().m_guid;

                response.m_sourceFileDependencyList.push_back(dependency);
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
        if (traceDrillerHook.GetErrorCount() > 0)
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
            TypeFingerprint sourceSliceTypeFingerprint = GetFingerprintForAllTypesInSlice(*sourcePrefab, m_typeFingerprinter, *context);

            const char* compilerVersion = "3";
            const size_t platformCount = request.GetEnabledPlatformsCount();

            for (size_t i = 0; i < platformCount; ++i)
            {
                AssetBuilderSDK::JobDescriptor jobDescriptor;
                jobDescriptor.m_priority = 0;
                jobDescriptor.m_critical = true;
                jobDescriptor.m_jobKey = "RC Slice";
                jobDescriptor.m_platform = request.GetEnabledPlatformAt(i);
                jobDescriptor.m_additionalFingerprintInfo = AZStd::string(compilerVersion)
                    .append(AZStd::string::format("|%llu", static_cast<uint64_t>(sourceSliceTypeFingerprint)));

                response.m_createJobOutputs.push_back(jobDescriptor);
            }
        }

        response.m_result = AssetBuilderSDK::CreateJobsResultCode::Success;
    }

    class ScopedProcessJobCleanup
    {
    public:
        ScopedProcessJobCleanup(AZStd::function<void()> cleanupFunc)
            : m_called(false)
            , m_cleanupFunc(cleanupFunc)
        {
        }

        ~ScopedProcessJobCleanup()
        {
            Call();
        }

        void Call()
        {
            if (!m_called)
            {
                m_called = true;
                m_cleanupFunc();
            }
        }

        bool m_called;
        AZStd::function<void()> m_cleanupFunc;
    };

    void SliceBuilderWorker::ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response) const
    {
        // Source slice is copied directly to the cache as-is, for use in-editor.
        // If slice is tagged as dynamic, we also conduct a runtime export, which converts editor->runtime components and generates a new .dynamicslice file.

        TraceDrillerHook traceDrillerHook(true);

        AZStd::string fullPath;
        AZStd::string fileNameOnly;
        AzFramework::StringFunc::Path::GetFileName(request.m_sourceFile.c_str(), fileNameOnly);
        AzFramework::StringFunc::Path::ConstructFull(request.m_watchFolder.c_str(), request.m_sourceFile.c_str(), fullPath, false);
        AzFramework::StringFunc::Path::Normalize(fullPath);

        AZ_TracePrintf(s_sliceBuilder, "Processing slice \"%s\".", fullPath.c_str());
        
        // Serialize in the source slice to determine if we need to generate a .dynamicslice.
        AZ::IO::FileIOStream stream(fullPath.c_str(), AZ::IO::OpenMode::ModeRead);
        if (!stream.IsOpen())
        {
            AZ_Warning(s_sliceBuilder, false, "Exporting of .dynamicslice for \"%s\" failed because the source file could not be opened.", fullPath.c_str());
            return;
        }

        AZ::SerializeContext* context;
        AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationBus::Events::GetSerializeContext);

        AZ::Data::Asset<AZ::SliceAsset> asset;
        asset.Create(AZ::Data::AssetId(AZ::Uuid::CreateRandom()));
        AZ::SliceAssetHandler assetHandler(context);

        {
            AZStd::lock_guard<AZStd::mutex> lock(m_processingMutex);
            assetHandler.LoadAssetData(asset, &stream, AZ::ObjectStream::AssetFilterSlicesOnly);
        }

        // Flush asset manager events to ensure no asset references are held by closures queued on Ebuses.
        AZ::Data::AssetManager::Instance().DispatchEvents();

        // Fail gracefully if any errors occurred while serializing in the editor slice.
        // i.e. missing assets or serialization errors.
        if (traceDrillerHook.GetErrorCount() > 0)
        {
            AZ_Error(s_sliceBuilder, false, "Exporting of .dynamicslice for \"%s\" failed due to errors loading editor slice.", fullPath.c_str());
            return;
        }

        // If the slice is designated as dynamic, generate the dynamic slice (.dynamicslice).
        AZ::SliceComponent* sourceSlice = (asset.Get()) ? asset.Get()->GetComponent() : nullptr;

        if (!sourceSlice)
        {
            AZ_Error(s_sliceBuilder, false, "Failed find the slice component in the slice asset!");
            return; // this should fail!
        }

        if (sourceSlice->IsDynamic())
        {
            AZStd::string dynamicSliceOutputPath;
            AzFramework::StringFunc::Path::Join(request.m_tempDirPath.c_str(), fileNameOnly.c_str(), dynamicSliceOutputPath, true, true, true);
            AzFramework::StringFunc::Path::ReplaceExtension(dynamicSliceOutputPath, "dynamicslice");

            AZ::SliceComponent::EntityList sourceEntities;
            sourceSlice->GetEntities(sourceEntities);
            AZ::Entity exportSliceEntity;
            AZ::SliceComponent* exportSlice = exportSliceEntity.CreateComponent<AZ::SliceComponent>();

            if (traceDrillerHook.GetErrorCount() > 0)
            {
                AZ_Error(s_sliceBuilder, false, "Exporting of .dynamicslice for \"%s\" failed due to errors instantiating entities.", fullPath.c_str());
                return;
            }

            // For export, components can assume they're initialized, but not activated.
            for (AZ::Entity* sourceEntity : sourceEntities)
            {
                if (sourceEntity->GetState() == AZ::Entity::ES_CONSTRUCTED)
                {
                    sourceEntity->Init();
                }
            }

            // Create cleanup function to ensure all BuildGameEntity calls are matched with FinishedBuildingGameEntity calls
            // even if we early return due to errors - we need to do this to prevent memory leaks/crashes caused by double deletions/missing deletions
            typedef AZStd::pair<AzToolsFramework::Components::EditorComponentBase*, AZ::Entity*> EditorComponentToEntityPair;
            AZStd::vector<EditorComponentToEntityPair> sourceComponentToBuiltGameEntityMapping;
            auto cleanupFunc = [&sourceComponentToBuiltGameEntityMapping]()
            {
                // Finalize entities for export. This will remove any export components temporarily
                // assigned by the source entity's components.
                for (auto& sourceComponentToGameEntityPair : sourceComponentToBuiltGameEntityMapping)
                {
                    sourceComponentToGameEntityPair.first->FinishedBuildingGameEntity(sourceComponentToGameEntityPair.second);
                }
            };
            ScopedProcessJobCleanup scopedBuildGameEntityCleanup(cleanupFunc);

            // Prepare entities for export. This involves invoking BuildGameEntity on source
            // entity's components, targeting a separate entity for export.
            bool entityExportSuccessful = true;
            for (AZ::Entity* sourceEntity : sourceEntities)
            {
                AZ::Entity* exportEntity = aznew AZ::Entity(sourceEntity->GetId(), sourceEntity->GetName().c_str());
                exportEntity->SetRuntimeActiveByDefault(sourceEntity->IsRuntimeActiveByDefault());

                const AZ::Entity::ComponentArrayType& editorComponents = sourceEntity->GetComponents();
                for (AZ::Component* component : editorComponents)
                {
                    auto* asEditorComponent =
                        azrtti_cast<AzToolsFramework::Components::EditorComponentBase*>(component);

                    if (asEditorComponent)
                    {
                        size_t oldComponentCount = exportEntity->GetComponents().size();
                        asEditorComponent->BuildGameEntity(exportEntity);
                        sourceComponentToBuiltGameEntityMapping.push_back(EditorComponentToEntityPair(asEditorComponent, exportEntity));
                        if (exportEntity->GetComponents().size() > oldComponentCount)
                        {
                            AZ::Component* newComponent = exportEntity->GetComponents().back();
                            AZ_Error("Export", asEditorComponent->GetId() != AZ::InvalidComponentId, "For entity \"%s\", component \"%s\" doesn't have a valid component id",
                                sourceEntity->GetName().c_str(), asEditorComponent->RTTI_GetType().ToString<AZStd::string>().c_str());
                            newComponent->SetId(asEditorComponent->GetId());
                        }
                    }
                    else
                    {
                        // The component is already runtime-ready. I.e. it is not an editor component.
                        // Clone the component and add it to the export entity
                        AZ::Component* clonedComponent = context->CloneObject(component);
                        exportEntity->AddComponent(clonedComponent);
                    }
                }

                // Pre-sort prior to exporting so it isn't required at instantiation time.
                const AZ::Entity::DependencySortResult sortResult = exportEntity->EvaluateDependencies();
                if (AZ::Entity::DSR_OK != sortResult)
                {
                    const char* sortResultError = "";
                    switch (sortResult)
                    {
                    case AZ::Entity::DSR_CYCLIC_DEPENDENCY:
                        sortResultError = "Cyclic dependency found";
                        break;
                    case AZ::Entity::DSR_MISSING_REQUIRED:
                        sortResultError = "Required services missing";
                        break;
                    }

                    AZ_Error("", false, "For slice \"%s\", Entity \"%s\" [0x%llx] dependency evaluation failed: %s. Dynamic slice cannot be generated.",
                        fullPath.c_str(), exportEntity->GetName().c_str(), static_cast<AZ::u64>(exportEntity->GetId()),
                        sortResultError);
                    return;
                }

                exportSlice->AddEntity(exportEntity);
            }

            AZ::SliceComponent::EntityList exportEntities;
            exportSlice->GetEntities(exportEntities);

            if (exportEntities.size() != sourceEntities.size())
            {
                AZ_Error(s_sliceBuilder, false, "Entity export list size must match that of the import list.");
                return;
            }

            // Save runtime slice to disk.
            // Use SaveObjectToFile because it writes to a byte stream first and then to disk
            // which is much faster than SaveObjectToStream(outputStream...) when writing large slices
            if (AZ::Utils::SaveObjectToFile<AZ::Entity>(dynamicSliceOutputPath.c_str(), AZ::DataStream::ST_XML, &exportSliceEntity))
            {
                AZ_TracePrintf(s_sliceBuilder, "Output file %s", dynamicSliceOutputPath.c_str());
            }
            else
            {
                AZ_Error(s_sliceBuilder, false, "Failed to open output file %s", dynamicSliceOutputPath.c_str());
                return;
            }

            // Finalize entities for export. This will remove any export components temporarily
            // assigned by the source entity's components.
            scopedBuildGameEntityCleanup.Call();

            AssetBuilderSDK::JobProduct jobProduct(dynamicSliceOutputPath);
            jobProduct.m_productAssetType = azrtti_typeid<AZ::DynamicSliceAsset>();
            jobProduct.m_productSubID = 2;
            response.m_outputProducts.push_back(jobProduct);
        }

        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;

        AZ_TracePrintf(s_sliceBuilder, "Finished processing slice %s", fullPath.c_str());
    }

    AZ::Uuid SliceBuilderWorker::GetUUID()
    {
        return AZ::Uuid::CreateString("{b92ad60c-d301-4484-8647-bb889ed717a2}");
    }
}
