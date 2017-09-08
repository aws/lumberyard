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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#include "stdafx.h"

#if defined(AZ_PLATFORM_WINDOWS)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

// Must be included only once in DLL module.
#include <platform_implRC.h>

#include <IConvertor.h>
#include <RCUtil.h>

#include <AzCore/base.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetManagerComponent.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/std/string/osstring.h>
#include <AzCore/Slice/SliceAssetHandler.h>
#include <AzCore/Slice/SliceComponent.h>
#include <AzCore/Slice/SliceSystemComponent.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Memory/PoolAllocator.h>
#include <AzCore/Memory/MemoryComponent.h>
#include <AzCore/Memory/AllocationRecords.h>
#include <AzCore/Memory/AllocatorManager.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Script/ScriptSystemComponent.h>
#include <AzCore/Debug/TraceMessageBus.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzFramework/Asset/AssetCatalogComponent.h>
#include <AzFramework/Asset/AssetCatalogBus.h>
#include <AzToolsFramework/Application/ToolsApplication.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <AzFramework/Asset/AssetSystemBus.h>
#include <AzFramework/Asset/AssetSystemComponent.h>
#include <AzToolsFramework/Asset/AssetSystemComponent.h>
#include <AzFramework/Components/BootstrapReaderComponent.h>
#include <AzToolsFramework/ToolsComponents/GenericComponentWrapper.h>
#include <AssetBuilderSDK/AssetBuilderSDK.h>

#include <LyShine/Bus/Tools/UiSystemToolsBus.h>
#include <LyShine/UiComponentTypes.h>

/**
 * Trace hook for asserts/errors/warning.
 * This allows us to report to the RC log, as well as to capture non-fatal errors and exit gracefully.
 */
class TraceDrillerHook
    : public AZ::Debug::TraceMessageBus::Handler
{
public:
    TraceDrillerHook()
    {
        BusConnect();
    }
    virtual ~TraceDrillerHook()
    {
    }
    virtual AZ::Debug::Result OnAssert(const AZ::Debug::TraceMessageParameters& parameters) override
    {
        if (s_errorsWillFailJob)
        {
            RCLogError("Assert failed: %s", CleanMessage(parameters.message).c_str());
            ++s_errorsOccurred;
        }
        else
        {
            RCLogWarning("Assert failed: %s", CleanMessage(parameters.message).c_str());
        }
        return AZ::Debug::Result::Handled;
    }
    virtual AZ::Debug::Result OnError(const AZ::Debug::TraceMessageParameters& parameters) override
    {
        if (s_errorsWillFailJob)
        {
            RCLogError("Error: %s", CleanMessage(parameters.message).c_str());
            ++s_errorsOccurred;
        }
        else
        {
            RCLogWarning("Error: %s", CleanMessage(parameters.message).c_str());
        }
        return AZ::Debug::Result::Handled;
    }
    virtual AZ::Debug::Result OnWarning(const AZ::Debug::TraceMessageParameters& parameters) override
    {
        RCLogWarning("Warning: %s", CleanMessage(parameters.message).c_str());
        return AZ::Debug::Result::Handled;
    }

    static AZ::OSString CleanMessage(const char* rawMessage)
    {
        AZ::OSString clean = rawMessage;

        // RC logs will print with double-newline unless we strip ours off.
        if (clean.size() > 0 && clean[clean.size() - 1] == '\n')
        {
            clean.erase(clean.size() - 1);
        }

        return clean;
    }

    static bool s_errorsWillFailJob;
    static size_t s_errorsOccurred;
};

bool TraceDrillerHook::s_errorsWillFailJob = false;
size_t TraceDrillerHook::s_errorsOccurred = 0;

/**
 * Compiler for UI canvas assets.
 */
class UiCanvasCompiler
    : public ICompiler
{
public:

    /**
     * AZToolsFramework application override for RC.
     * Adds the minimal set of system components required for UI canvas loading & conversion.
     */
    class RCToolApplication
        : public AzToolsFramework::ToolsApplication
    {
    public:
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            AZ::ComponentTypeList components = AZ::ComponentApplication::GetRequiredSystemComponents();

            components.insert(components.end(), {
                azrtti_typeid<AZ::MemoryComponent>(),
                azrtti_typeid<AZ::AssetManagerComponent>(),
                azrtti_typeid<AzFramework::AssetCatalogComponent>(),
                azrtti_typeid<AZ::ScriptSystemComponent>(),
                azrtti_typeid<AzFramework::BootstrapReaderComponent>(),
                azrtti_typeid<AzFramework::AssetSystem::AssetSystemComponent>(),
                azrtti_typeid<AzToolsFramework::AssetSystem::AssetSystemComponent>(),
                azrtti_typeid<AZ::SliceSystemComponent>(),
                azrtti_typeid<AzToolsFramework::Components::GenericComponentUnwrapper>(),
            });

            return components;
        }

        // we live in a subfolder (bin64xxx/RC) so our modules live one folder up.
        void ResolveModulePath(AZ::OSString& modulePath) override
        {
            modulePath = AZ::OSString("../") + modulePath;  // first, pre-pend the .. to it.
            AzToolsFramework::ToolsApplication::ResolveModulePath(modulePath); // the base class will prepend the path to executable.
        }
    };

public:
    UiCanvasCompiler() = default;
    virtual ~UiCanvasCompiler() = default;

    void Release() override                                         { delete this; }
    void BeginProcessing(const IConfig* config) override            {}
    void EndProcessing() override                                   {}

    bool CreateJobs() override;
    virtual bool Process() override;

    IConvertContext* GetConvertContext() override { return &m_CC; }

private:

    enum class WorkType
    {
        CreateJobs, ProcessJob
    };

    //! Sets up the environment and calls DoWork
    template<bool(UiCanvasCompiler::* TFunc)(RCToolApplication&) const>
    bool SetupAndRun(WorkType workType) const;

    template<bool(UiCanvasCompiler::* TFunc)(RCToolApplication&) const>
    bool DoWork(AZ::IO::FileIOBase* fileIO, WorkType workType, RCToolApplication& app) const;

    bool CreateJobAsset(RCToolApplication& app) const;
    bool ProcessAsset(RCToolApplication& app) const;

    ConvertContext m_CC;
};

/**
 * Registered converter for UI canvas assets.
 */
class UiCanvasConverter
    : public IConvertor
{
public:
    UiCanvasConverter() = default;
    virtual ~UiCanvasConverter() = default;

    void Release() override                         { delete this; }
    void DeInit() override                          {}
    ICompiler* CreateCompiler() override            { return new UiCanvasCompiler(); }
    bool SupportsMultithreading() const override    { return false; }
    const char* GetExt(int index) const override;

private:
};

//=========================================================================
// UiCanvasConverter::GetExt
//=========================================================================
const char* UiCanvasConverter::GetExt(int index) const
{
    switch (index)
    {
    case 0:
        return "uicanvas";
    }
    return nullptr;
}

//=========================================================================
// UiCanvasCompiler::CreateJobs
//=========================================================================
bool UiCanvasCompiler::CreateJobs()
{
    return SetupAndRun<&UiCanvasCompiler::CreateJobAsset>(WorkType::CreateJobs);
}

//=========================================================================
// UiCanvasCompiler::Process
//=========================================================================
bool UiCanvasCompiler::Process()
{
    return SetupAndRun<&UiCanvasCompiler::ProcessAsset>(WorkType::ProcessJob);
}

//=========================================================================
// UiCanvasCompiler::SetupAndRun
//=========================================================================
template<bool(UiCanvasCompiler::* TFunc)(UiCanvasCompiler::RCToolApplication&) const>
bool UiCanvasCompiler::SetupAndRun(WorkType workType) const
{
    TraceDrillerHook traceHook;

    AZ::IO::FileIOBase* prevIO = AZ::IO::FileIOBase::GetInstance();
    AZ::IO::FileIOBase::SetInstance(nullptr);
    AZ::IO::FileIOBase* fileIO = m_CC.pRC->GetSystemEnvironment()->pFileIO;
    AZ::IO::FileIOBase::SetInstance(fileIO);

    RCToolApplication app;
    AZ::ComponentApplication::Descriptor appDescriptor;
    app.Start(appDescriptor);

    // This is the only place where the allocator(s) are running, be sure not to use them outside of this section

    bool result = DoWork<TFunc>(fileIO, workType, app);

    app.Stop();

    AZ::IO::FileIOBase::SetInstance(nullptr);
    AZ::IO::FileIOBase::SetInstance(prevIO);

    return result;
}

//=========================================================================
// UiCanvasCompiler::DoWork
//=========================================================================
template<bool(UiCanvasCompiler::* TFunc)(UiCanvasCompiler::RCToolApplication&) const>
bool UiCanvasCompiler::DoWork(AZ::IO::FileIOBase* fileIO, WorkType workType, RCToolApplication& app) const
{
    bool result = false;

    // Allow RC jobs to leak, should not fail the job
    AZ::AllocatorManager::Instance().SetAllocatorLeaking(true);

    const char* oldCachePath = fileIO->GetAlias("@assets@");
    const char* gameFolder = fileIO->GetAlias("@devassets@");
    char configPath[2048];
    sprintf_s(configPath, "%s/config/editor.xml", gameFolder);

    if (!ResourceCompilerUtil::ConnectToAssetProcessor(m_CC.config->GetAsInt("port", 0, 0), "RC UiCanvas Compiler"))
    {
        return false;
    }

    AZStd::unique_ptr<AZ::Entity> extraSystemEntity;

    // Load & reflect modules used by this project.
    // The UI canvas could contain data defined in any of them.
    if (!app.ReflectModulesFromAppDescriptor(configPath))
    {
        return false;
    }

    // Add the LyShineSystemComponent to an entity and activate it so that it connects to
    // the UiSystemToolsBus which we will use to load the canvas.
    // Don't do it if the LyShineSystemComponent hasn't been registered
    extraSystemEntity.reset(aznew AZ::Entity);
    if (AZ::ComponentDescriptorBus::GetNumOfEventHandlers(LyShine::lyShineSystemComponentUuid) > 0)
    {
        extraSystemEntity->CreateComponent(LyShine::lyShineSystemComponentUuid);
        extraSystemEntity->Init();
        extraSystemEntity->Activate();
    }

    if (workType == WorkType::ProcessJob)
    {
        // Override @assets to be the current platform's assets we're compiling on instead of the current
        // platform we're running this program on.

        const IConfig& config = m_CC.multiConfig->getConfig();
        const char* pCurrentPlatform = nullptr;
        if (config.GetKeyValue("p", pCurrentPlatform) && pCurrentPlatform)
        {
            // switch to that cache!
            AZStd::string newCachePath = oldCachePath;
#if defined(AZ_PLATFORM_WINDOWS)
            // replace only the FIRST occurrence:
            AzFramework::StringFunc::Replace(newCachePath, "\\pc\\", AZStd::string::format("\\%s\\", pCurrentPlatform).c_str(), false, true);
#elif defined (AZ_PLATFORM_APPLE_OSX)
            AzFramework::StringFunc::Replace(newCachePath, "/osx_gl/", AZStd::string::format("/%s/", pCurrentPlatform).c_str(), false, true);
#endif
            fileIO->SetAlias("@assets@", newCachePath.c_str());
        }

        EBUS_EVENT(AZ::Data::AssetCatalogRequestBus, LoadCatalog, AZStd::string::format("@assets@/AssetCatalog.xml").c_str());
    }
    else if (workType == WorkType::CreateJobs)
    {
        // Since we're just loading the slice to check dependencies, no need to reflect modules, but we do need to reflect the AssetBuilderSDK structures so we can read/write responses
        AssetBuilderSDK::InitializeSerializationContext();
    }
    else
    {
        RCLogError("Unhandled WorkType");
        return false;
    }

    // Fail job if errors reported while processing asset
    TraceDrillerHook::s_errorsWillFailJob = true;

    result = (this->*TFunc)(app);

    fileIO->SetAlias("@assets@", oldCachePath);

    TraceDrillerHook::s_errorsWillFailJob = false;

    return result;
}

//=========================================================================
// UiCanvasCompiler::CreateJobAsset
//=========================================================================
bool UiCanvasCompiler::CreateJobAsset(RCToolApplication& app) const
{
    const char* outputFileName = "createjobsResponse.xml";

    AZStd::string sourcePath;
    AZStd::string outputPath;
    AzFramework::StringFunc::Path::Join(m_CC.sourceFolder.c_str(), m_CC.sourceFileNameOnly.c_str(), sourcePath, true, true, true);
    AzFramework::StringFunc::Path::Join(m_CC.GetOutputFolder().c_str(), outputFileName, outputPath, true, true, true);

    RCLog("CreateJobs for UI canvas \"%s\" with output path \"%s\".", sourcePath.c_str(), outputPath.c_str());

    // Open the source canvas file
    AZ::IO::FileIOStream stream(sourcePath.c_str(), AZ::IO::OpenMode::ModeRead);
    if (!stream.IsOpen())
    {
        RCLogWarning("CreateJobs for \"%s\" failed because the source file could not be opened.", sourcePath.c_str());
        return true;
    }

    AssetBuilderSDK::CreateJobsRequest createJobsRequest;
    AssetBuilderSDK::CreateJobsResponse createJobsResponse;
    AZStd::string createJobsRequestPath = m_CC.config->GetAsString("createjobs", "", "").c_str();

    if (!AZ::Utils::LoadObjectFromFileInPlace(createJobsRequestPath, createJobsRequest))
    {
        RCLogError("Failed to load CreateJobsRequest from file: %s", createJobsRequestPath);
        return false;
    }

    // Asset filter always returns false to prevent parsing dependencies, but makes note of the slice dependencies
    auto assetFilter = [&createJobsResponse](const AZ::Data::Asset<AZ::Data::AssetData>& asset)
    {
        if (asset.GetType() == AZ::AzTypeInfo<AZ::SliceAsset>::Uuid())
        {
            bool isSliceDependency = (0 == (asset.GetFlags() & static_cast<AZ::u8>(AZ::Data::AssetFlags::OBJECTSTREAM_NO_LOAD)));

            if (isSliceDependency)
            {
                AssetBuilderSDK::SourceFileDependency dependency;
                dependency.m_sourceFileDependencyUUID = asset.GetId().m_guid;

                createJobsResponse.m_sourceFileDependencyList.push_back(dependency);
            }
        }

        return false;
    };

    // Serialize in the canvas from the stream. This uses the LyShineSystemComponent to do it because
    // it does some complex support for old canvas formats
    UiSystemToolsInterface::CanvasAssetHandle* canvasAsset = nullptr;
    EBUS_EVENT_RESULT(canvasAsset, UiSystemToolsBus, LoadCanvasFromStream, stream, AZ::ObjectStream::FilterDescriptor(assetFilter));
    if (!canvasAsset)
    {
        RCLogError("Compiling UI canvas \"%s\" failed due to errors loading editor UI canvas.", sourcePath.c_str());
        return true;
    }

    // Flush asset database events to ensure no asset references are held by closures queued on Ebuses.
    AZ::Data::AssetManager::Instance().DispatchEvents();

    // Fail gracefully if any errors occurred while serializing in the editor UI canvas.
    // i.e. missing assets or serialization errors.
    if (TraceDrillerHook::s_errorsOccurred > 0)
    {
        RCLogError("Compiling UI canvas \"%s\" failed due to errors loading editor UI canvas.", sourcePath.c_str());
        EBUS_EVENT(UiSystemToolsBus, DestroyCanvas, canvasAsset);
        return true;
    }

    const char* compilerVersion = "3";
    const size_t platformCount = createJobsRequest.GetEnabledPlatformsCount();

    for (size_t i = 0; i < platformCount; ++i)
    {
        AssetBuilderSDK::JobDescriptor jobDescriptor;
        jobDescriptor.m_priority = 0;
        jobDescriptor.m_critical = true;
        jobDescriptor.m_jobKey = "RC Slice";
        jobDescriptor.m_platform = createJobsRequest.GetEnabledPlatformAt(i);
        jobDescriptor.m_additionalFingerprintInfo = AZStd::string(compilerVersion).append(azrtti_typeid<AZ::DynamicSliceAsset>().ToString<AZStd::string>());

        createJobsResponse.m_createJobOutputs.push_back(jobDescriptor);
    }

    createJobsResponse.m_result = AssetBuilderSDK::CreateJobsResultCode::Success;
    AZ::Utils::SaveObjectToFile(outputPath, AZ::DataStream::ST_XML, &createJobsResponse);

    EBUS_EVENT(UiSystemToolsBus, DestroyCanvas, canvasAsset);

    return true;
}

//=========================================================================
// UiCanvasCompiler::ProcessAsset
//=========================================================================
bool UiCanvasCompiler::ProcessAsset(RCToolApplication& app) const
{
    // .uicanvas files are converted as they are copied to the cache
    // a) to flatten all prefab instances
    // b) to replace any editor components with runtime components

    AZStd::string sourcePath;
    AZStd::string outputPath;
    AzFramework::StringFunc::Path::Join(m_CC.sourceFolder.c_str(), m_CC.sourceFileNameOnly.c_str(), sourcePath, true, true, true);
    AzFramework::StringFunc::Path::Join(m_CC.GetOutputFolder().c_str(), m_CC.sourceFileNameOnly.c_str(), outputPath, true, true, true);

    RCLog("Processing UI canvas \"%s\" with output path \"%s\".", sourcePath.c_str(), outputPath.c_str());

    // Open the source canvas file
    AZ::IO::FileIOStream stream(sourcePath.c_str(), AZ::IO::OpenMode::ModeRead | AZ::IO::OpenMode::ModeBinary);
    if (!stream.IsOpen())
    {
        RCLogWarning("Compiling UI canvas \"%s\" failed because source file could not be opened.", sourcePath.c_str());
        return true;
    }

    // Serialize in the canvas from the stream. This uses the LyShineSystemComponent to do it because
    // it does some complex support for old canvas formats
    UiSystemToolsInterface::CanvasAssetHandle* canvasAsset = nullptr;
    EBUS_EVENT_RESULT(canvasAsset, UiSystemToolsBus, LoadCanvasFromStream, stream,
        AZ::ObjectStream::FilterDescriptor(AZ::ObjectStream::AssetFilterSlicesOnly));
    if (!canvasAsset)
    {
        RCLogError("Compiling UI canvas \"%s\" failed due to errors loading editor UI canvas.", sourcePath.c_str());
        return true;
    }

    // Flush asset manager events to ensure no asset references are held by closures queued on Ebuses.
    AZ::Data::AssetManager::Instance().DispatchEvents();

    // Fail gracefully if any errors occurred while serializing in the editor UI canvas.
    // i.e. missing assets or serialization errors.
    if (TraceDrillerHook::s_errorsOccurred > 0)
    {
        RCLogError("Compiling UI canvas \"%s\" failed due to errors loading editor UI canvas.", sourcePath.c_str());
        EBUS_EVENT(UiSystemToolsBus, DestroyCanvas, canvasAsset);
        return true;
    }

    // Get the prefab component from the canvas
    AZ::SliceComponent* canvasSlice = nullptr;
    EBUS_EVENT_RESULT(canvasSlice, UiSystemToolsBus, GetRootSliceSliceComponent, canvasAsset);

    if (!canvasSlice)
    {
        RCLogError("Compiling UI canvas \"%s\" failed due to errors loading editor UI canvas.", sourcePath.c_str());
        EBUS_EVENT(UiSystemToolsBus, DestroyCanvas, canvasAsset);
        return true;
    }

    // There are issues if we do not use the sliceAsset handler to load the slice. The asset filter does not
    // get passed down into the slice Instantiate. So serialize it to string and load it back using the asset handler
    AZ::Entity* canvasSliceEntity = canvasSlice->GetEntity();

    AZStd::string prefabBuffer;
    AZ::IO::ByteContainerStream<AZStd::string > prefabStream(&prefabBuffer);
    if (!AZ::Utils::SaveObjectToStream<AZ::Entity>(prefabStream, AZ::ObjectStream::ST_XML, canvasSliceEntity))
    {
        RCLogError("Compiling UI canvas \"%s\" failed due to errors serializing editor UI canvas.", sourcePath.c_str());
        EBUS_EVENT(UiSystemToolsBus, DestroyCanvas, canvasAsset);
        return true;
    }

    prefabStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);
    AZ::Data::Asset<AZ::SliceAsset> asset;
    asset.Create(AZ::Data::AssetId(AZ::Uuid::CreateRandom()));
    AZ::SliceAssetHandler assetHandler;
    assetHandler.LoadAssetData(asset, &prefabStream, AZ::ObjectStream::AssetFilterSlicesOnly);

    // Flush asset manager events to ensure no asset references are held by closures queued on Ebuses.
    AZ::Data::AssetManager::Instance().DispatchEvents();

    // Fail gracefully if any errors occurred while serializing in the editor UI canvas.
    // i.e. missing assets or serialization errors.
    if (TraceDrillerHook::s_errorsOccurred > 0)
    {
        RCLogError("Compiling UI canvas \"%s\" failed due to errors loading editor UI canvas.", sourcePath.c_str());
        asset = nullptr;
        EBUS_EVENT(UiSystemToolsBus, DestroyCanvas, canvasAsset);
        return true;
    }

    // Get the prefab component from the prefab asset
    AZ::SliceComponent* sourceSlice = (asset.Get()) ? asset.Get()->GetComponent() : nullptr;

    // Now create a flattened prefab component from the source one
    if (sourceSlice)
    {
        AZ::SliceComponent::EntityList sourceEntities;
        sourceSlice->GetEntities(sourceEntities);
        AZ::Entity exportSliceEntity;
        AZ::SliceComponent* exportSlice = exportSliceEntity.CreateComponent<AZ::SliceComponent>();
        exportSliceEntity.Init();

        // For export, components can assume they're initialized, but not activated.
        for (AZ::Entity* sourceEntity : sourceEntities)
        {
            if (sourceEntity->GetState() == AZ::Entity::ES_CONSTRUCTED)
            {
                sourceEntity->Init();
            }
        }

        // Prepare entities for export. This involves invoking BuildGameEntity on source
        // entity's components, targeting a separate entity for export.
        bool entityExportSuccessful = true;
        for (AZ::Entity* sourceEntity : sourceEntities)
        {
            AZ::Entity* exportEntity = aznew AZ::Entity(sourceEntity->GetName().c_str());
            exportEntity->SetId(sourceEntity->GetId());

            const AZ::Entity::ComponentArrayType& editorComponents = sourceEntity->GetComponents();
            for (AZ::Component* component : editorComponents)
            {
                auto* asEditorComponent =
                    azrtti_cast<AzToolsFramework::Components::EditorComponentBase*>(component);

                if (asEditorComponent)
                {
                    size_t oldComponentCount = exportEntity->GetComponents().size();
                    asEditorComponent->BuildGameEntity(exportEntity);
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
                    AZ::Component* clonedComponent = app.GetSerializeContext()->CloneObject(component);
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

                RCLogError("For UI canvas \"%s\", Entity \"%s\" [0x%llx] dependency evaluation failed: %s. Compiled canvas cannot be generated.",
                    sourcePath.c_str(), exportEntity->GetName().c_str(), static_cast<AZ::u64>(exportEntity->GetId()),
                    sortResultError);
                EBUS_EVENT(UiSystemToolsBus, DestroyCanvas, canvasAsset);
                return false;
            }

            exportSlice->AddEntity(exportEntity);
        }

        AZ::SliceComponent::EntityList exportEntities;
        exportSlice->GetEntities(exportEntities);

        if (exportEntities.size() != sourceEntities.size())
        {
            RCLogError("Entity export list size must match that of the import list.");
            EBUS_EVENT(UiSystemToolsBus, DestroyCanvas, canvasAsset);
            return false;
        }

        // Save runtime UI canvas to disk.
        AZ::IO::FileIOStream outputStream(outputPath.c_str(), AZ::IO::OpenMode::ModeWrite);
        if (outputStream.IsOpen())
        {
            exportSliceEntity.RemoveComponent(exportSlice);
            EBUS_EVENT(UiSystemToolsBus, ReplaceRootSliceSliceComponent, canvasAsset, exportSlice);
            EBUS_EVENT(UiSystemToolsBus, SaveCanvasToStream, canvasAsset, outputStream);
            outputStream.Close();
        }

        // Finalize entities for export. This will remove any export components temporarily
        // assigned by the source entity's components.
        auto sourceIter = sourceEntities.begin();
        auto exportIter = exportEntities.begin();
        for (; sourceIter != sourceEntities.end(); ++sourceIter, ++exportIter)
        {
            AZ::Entity* sourceEntity = *sourceIter;
            AZ::Entity* exportEntity = *exportIter;

            const AZ::Entity::ComponentArrayType& editorComponents = sourceEntity->GetComponents();
            for (AZ::Component* component : editorComponents)
            {
                auto* asEditorComponent =
                    azrtti_cast<AzToolsFramework::Components::EditorComponentBase*>(component);

                if (asEditorComponent)
                {
                    asEditorComponent->FinishedBuildingGameEntity(exportEntity);
                }
            }
        }

        m_CC.pRC->AddInputOutputFilePair(sourcePath.c_str(), outputPath.c_str());
    }

    asset = nullptr;
    EBUS_EVENT(UiSystemToolsBus, DestroyCanvas, canvasAsset);

    return true;
}

//=========================================================================
// RegisterConvertors
//=========================================================================
extern "C" DLL_EXPORT void __stdcall RegisterConvertors(IResourceCompiler* pRC)
{
    gEnv = pRC->GetSystemEnvironment();

    SetRCLog(pRC->GetIRCLog());

    pRC->RegisterConvertor("UiCanvasCompiler", new UiCanvasConverter());
}
