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

#include <AzCore/IO/SystemFile.h>
#include <AzCore/Math/Crc.h>
#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Memory/MemoryComponent.h>
#include <AzCore/Slice/SliceSystemComponent.h>
#include <AzCore/Jobs/JobManagerComponent.h>
#include <AzCore/IO/StreamerComponent.h>
#include <AzCore/Asset/AssetManagerComponent.h>
#include <AzCore/UserSettings/UserSettingsComponent.h>
#include <AzCore/Script/ScriptSystemComponent.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/Serialization/DataPatch.h>
#include <AzCore/Serialization/ObjectStreamComponent.h>
#include <AzCore/Debug/FrameProfilerComponent.h>
#include <AzCore/PlatformIncl.h>

#include <AzFramework/Asset/SimpleAsset.h>
#include <AzFramework/Asset/AssetCatalogComponent.h>
#include <AzFramework/Asset/AssetSystemComponent.h>
#include <AzFramework/Components/TransformComponent.h>
#include <AzFramework/Components/BootstrapReaderComponent.h>
#include <AzFramework/Entity/EntityContext.h>
#include <AzFramework/Entity/GameEntityContextComponent.h>
#include <AzFramework/Entity/EntityReference.h>
#include <AzFramework/Input/System/InputSystemComponent.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzFramework/Network/NetBindingComponent.h>
#include <AzFramework/Network/NetBindingSystemComponent.h>
#include <AzFramework/Script/ScriptRemoteDebugging.h>
#include <AzFramework/Script/ScriptComponent.h>
#include <AzFramework/TargetManagement/TargetManagementComponent.h>
#include <AzFramework/Network/NetworkContext.h>
#include <AzFramework/Metrics/MetricsPlainTextNameRegistration.h>
#include <GridMate/Memory.h>

#include "Application.h"
#include <cctype>
#include <stdio.h>

#if defined(AZ_PLATFORM_ANDROID) || defined(AZ_PLATFORM_APPLE)
    #include <unistd.h>
#endif //AZ_PLATFORM_ANDROID  ||  AZ_PLATFORM_APPLE

namespace AzFramework
{
    namespace ApplicationInternal
    {
        // A Helper function that can load an app descriptor from file.

        AZ::Outcome<AZStd::unique_ptr<AZ::ComponentApplication::Descriptor>, AZStd::string> LoadDescriptorFromFilePath(const char* appDescriptorFilePath, AZ::SerializeContext& serializeContext)
        {
            AZStd::unique_ptr<AZ::ComponentApplication::Descriptor> loadedDescriptor;

            AZ::IO::SystemFile appDescriptorFile;
            if (!appDescriptorFile.Open(appDescriptorFilePath, AZ::IO::SystemFile::SF_OPEN_READ_ONLY))
            {
                return AZ::Failure(AZStd::string::format("Failed to open file: %s", appDescriptorFilePath));
            }

            AZ::IO::SystemFileStream appDescriptorFileStream(&appDescriptorFile, true);
            if (!appDescriptorFileStream.IsOpen())
            {
                return AZ::Failure(AZStd::string::format("Failed to stream file: %s", appDescriptorFilePath));
            }

            // Callback function for allocating the root elements in the file.
            AZ::ObjectStream::InplaceLoadRootInfoCB inplaceLoadCb =
                [](void** rootAddress, const AZ::SerializeContext::ClassData**, const AZ::Uuid& classId, AZ::SerializeContext*)
            {
                if (rootAddress && classId == azrtti_typeid<AZ::ComponentApplication::Descriptor>())
                {
                    // ComponentApplication::Descriptor is normally a singleton.
                    // Force a unique instance to be created.
                    *rootAddress = aznew AZ::ComponentApplication::Descriptor();
                }
            };

            // Callback function for saving the root elements in the file.
            AZ::ObjectStream::ClassReadyCB classReadyCb =
                [&loadedDescriptor](void* classPtr, const AZ::Uuid& classId, AZ::SerializeContext* context)
            {
                // Save descriptor, delete anything else loaded from file.
                if (classId == azrtti_typeid<AZ::ComponentApplication::Descriptor>())
                {
                    loadedDescriptor.reset(static_cast<AZ::ComponentApplication::Descriptor*>(classPtr));
                }
                else if (const AZ::SerializeContext::ClassData* classData = context->FindClassData(classId))
                {
                    classData->m_factory->Destroy(classPtr);
                }
                else
                {
                    AZ_Error("Application", false, "Unexpected type %s found in application descriptor file. This memory will leak.",
                        classId.ToString<AZStd::string>().c_str());
                }
            };

            // There's other stuff in the file we may not recognize (system components), but we're not interested in that stuff.
            AZ::ObjectStream::FilterDescriptor loadFilter(nullptr, AZ::ObjectStream::FILTERFLAG_IGNORE_UNKNOWN_CLASSES);

            if (!AZ::ObjectStream::LoadBlocking(&appDescriptorFileStream, serializeContext, classReadyCb, loadFilter, inplaceLoadCb))
            {
                return AZ::Failure(AZStd::string::format("Failed to load objects from file: %s", appDescriptorFilePath));
            }

            if (!loadedDescriptor)
            {
                return AZ::Failure(AZStd::string::format("Failed to find descriptor object in file: %s", appDescriptorFilePath));
            }

            return AZ::Success(AZStd::move(loadedDescriptor));
        }
    }
    Application::Application()
        : m_lifecycleEventsHandler(nullptr)
    {
        m_configFilePath[0] = '\0';
        m_appRoot[0] = '\0';
        m_assetRoot[0] = '\0';

        ApplicationRequests::Bus::Handler::BusConnect();
        AZ::UserSettingsFileLocatorBus::Handler::BusConnect();
        NetSystemRequestBus::Handler::BusConnect();
    }

    Application::~Application()
    {
        if (m_isStarted)
        {
            Stop();
        }

        NetSystemRequestBus::Handler::BusDisconnect();
        AZ::UserSettingsFileLocatorBus::Handler::BusDisconnect();
        ApplicationRequests::Bus::Handler::BusDisconnect();
    }

    void Application::ResolveModulePath(AZ::OSString& modulePath)
    {
        if (AzFramework::StringFunc::Path::IsRelative(modulePath.c_str()))
        {
            ComponentApplication::ResolveModulePath(modulePath);
        }
    }

    void Application::Start(const Descriptor& descriptor, const StartupParameters& startupParameters /*= StartupParameters()*/)
    {
        CalculateAppRoot(startupParameters.m_appRootOverride);
        AZ::Entity* systemEntity = Create(descriptor, startupParameters);
        StartCommon(systemEntity);
    }

    void Application::Start(const char* applicationDescriptorFile, const StartupParameters& startupParameters /*= StartupParameters()*/)
    {
        if (!AZ::AllocatorInstance<AZ::OSAllocator>::IsReady())
        {
            AZ::AllocatorInstance<AZ::OSAllocator>::Create();
            m_isOSAllocatorOwner = true;
        }

        CalculateAppRoot(startupParameters.m_appRootOverride);

        AZ::OSString descriptorFile = GetFullPathForDescriptor(applicationDescriptorFile);
        azstrcpy(m_configFilePath, AZ_MAX_PATH_LEN, descriptorFile.c_str());

        AZ::Entity* systemEntity = nullptr;

        bool descriptorFound = true;
        if (!AZ::IO::SystemFile::Exists(m_configFilePath))
        {
            descriptorFound = false;

            const bool continueStartup = OnFailedToFindConfiguration(m_configFilePath);
            if (!continueStartup)
            {
                return;
            }
        }

        if (descriptorFound)
        {
            // Pass applicationDescriptorFile instead of m_configFilePath because Create() expects AppRoot-relative path.
            systemEntity = Create(applicationDescriptorFile, startupParameters);
        }
        else
        {
            systemEntity = Create(Descriptor(), startupParameters);
        }
        StartCommon(systemEntity);
    }

    bool Application::OnFailedToFindConfiguration(const char* configFilePath)
    {
        AZ_Error("Application", false, "Failed to find app descriptor file \"%s\".", configFilePath);
        return true;
    }

    bool Application::ReflectModulesFromAppDescriptor(const char* appDescriptorFilePath)
    {
        auto loadDescriptorOutcome = ApplicationInternal::LoadDescriptorFromFilePath(appDescriptorFilePath, *GetSerializeContext());
        if (!loadDescriptorOutcome.IsSuccess())
        {
            AZ_Error("Application", false, loadDescriptorOutcome.GetError().c_str());
            return false;
        }

        AZ::ComponentApplication::Descriptor& descriptor = *loadDescriptorOutcome.GetValue();
        for (AZ::ComponentApplication::Descriptor::DynamicModuleDescriptor& moduleDescriptor : descriptor.m_modules)
        {
            if (!IsModuleLoaded(moduleDescriptor))
            {
                AZ::ComponentApplication::LoadModuleOutcome loadModuleOutcome = LoadDynamicModule(moduleDescriptor);
                AZ_Error("Application", loadModuleOutcome.IsSuccess(), loadModuleOutcome.GetError().c_str());
                if (!loadModuleOutcome.IsSuccess())
                {
                    return false;
                }
            }
        }

        return true;
    }


    void Application::StartCommon(AZ::Entity* systemEntity)
    {
        // Startup default local FileIO (hits OSAllocator) if not already setup.
        if (AZ::IO::FileIOBase::GetInstance() == nullptr)
        {
            m_defaultFileIO.reset(aznew AZ::IO::LocalFileIO());
            AZ::IO::FileIOBase::SetInstance(m_defaultFileIO.get());
        }

        CreateNetworkContext();

        m_lifecycleEventsHandler.reset(aznew ApplicationLifecycleEventsHandler());

        m_commandLine.Parse();

        systemEntity->Init();
        systemEntity->Activate();
        AZ_Assert(systemEntity->GetState() == AZ::Entity::ES_ACTIVE, "System Entity failed to activate.");

        // TEMP: Moved GridMateAllocatorMP creation into AzFramework::Application due to a shutdown issue
        // with allocator environment variables. AzFramework::Application will create GridMateAllocatorMP from
        // the main process.
        // See LMBR-20396 for more details.
        GridMate::GridMateAllocatorMP::Descriptor allocDesc;
        allocDesc.m_custom = &AZ::AllocatorInstance<AZ::SystemAllocator>::Get();
        AZ::AllocatorInstance<GridMate::GridMateAllocatorMP>::Create(allocDesc);

        m_isStarted = true;
    }

    void Application::SaveConfiguration()
    {
        if (m_configFilePath[0])
        {
            WriteApplicationDescriptor(m_configFilePath);
        }
    }

    void Application::Stop()
    {
        if (m_isStarted)
        {
            m_lifecycleEventsHandler.reset();

            /* The following three lines of code are a temporary fix (CR: https://lumberyard-cr.amazon.com/r/1648621/) 
             * GridMate's ReplicaChunkDescriptor is stored in a global environment variable 'm_globalDescriptorTable'
             * which does not get cleared when Application shuts down. We need to un-reflect here to clear ReplicaChunkDescriptor
             * so that ReplicaChunkDescriptor::m_vdt doesn't get flooded when we repeatedly instantiate Application in unit tests. 
             */
            m_networkContext->EnableRemoveReflection();
            EBUS_EVENT(AZ::ComponentDescriptorBus, Reflect, m_networkContext.get());
            m_networkContext->DisableRemoveReflection();

            m_networkContext.reset();

            if (AZ::IO::FileIOBase::GetInstance() == m_defaultFileIO.get())
            {
                AZ::IO::FileIOBase::SetInstance(nullptr);
            }
            m_defaultFileIO.reset();

            // Free any memory owned by the command line container.
            m_commandLine = CommandLine();

            // TEMP: Moved GridMateAllocatorMP creation into AzFramework::Application due to a shutdown issue
            // with allocator environment variables. AzFramework::Application will create GridMateAllocatorMP from
            // the main process.
            // See LMBR-20396 for more details.
            AZ::AllocatorInstance<GridMate::GridMateAllocatorMP>::Destroy();

            Destroy();

            m_isStarted = false;
        }
    }

    void Application::RegisterCoreComponents()
    {
        AZ::ComponentApplication::RegisterCoreComponents();

        RegisterComponentDescriptor(AzFramework::BootstrapReaderComponent::CreateDescriptor());
        RegisterComponentDescriptor(AzFramework::AssetCatalogComponent::CreateDescriptor());
        RegisterComponentDescriptor(AzFramework::NetBindingComponent::CreateDescriptor());
        RegisterComponentDescriptor(AzFramework::NetBindingSystemComponent::CreateDescriptor());
        RegisterComponentDescriptor(AzFramework::TransformComponent::CreateDescriptor());        
        RegisterComponentDescriptor(AzFramework::GameEntityContextComponent::CreateDescriptor());
        RegisterComponentDescriptor(AzFramework::TargetManagementComponent::CreateDescriptor());
        RegisterComponentDescriptor(AzFramework::CreateScriptDebugAgentFactory());
        RegisterComponentDescriptor(AzFramework::AssetSystem::AssetSystemComponent::CreateDescriptor());
        RegisterComponentDescriptor(AzFramework::InputSystemComponent::CreateDescriptor());

#if !defined(AZCORE_EXCLUDE_LUA)
        RegisterComponentDescriptor(AzFramework::ScriptComponent::CreateDescriptor());
#endif

        // This is internal Amazon code, so register it's components for metrics tracking, otherwise the name of the component won't get sent back.
        AZStd::vector<AZ::Uuid> componentUuidsForMetricsCollection
        {
            azrtti_typeid<AZ::MemoryComponent>(),
            azrtti_typeid<AZ::StreamerComponent>(),
            azrtti_typeid<AZ::JobManagerComponent>(),
            azrtti_typeid<AZ::AssetManagerComponent>(),
            azrtti_typeid<AZ::ObjectStreamComponent>(),
            azrtti_typeid<AZ::UserSettingsComponent>(),
            azrtti_typeid<AZ::Debug::FrameProfilerComponent>(),
            azrtti_typeid<AZ::SliceComponent>(),
            azrtti_typeid<AZ::SliceSystemComponent>(),

            azrtti_typeid<AzFramework::AssetCatalogComponent>(),
            azrtti_typeid<AzFramework::NetBindingComponent>(),
            azrtti_typeid<AzFramework::NetBindingSystemComponent>(),
            azrtti_typeid<AzFramework::TransformComponent>(),
            azrtti_typeid<AzFramework::GameEntityContextComponent>(),
            azrtti_typeid<AzFramework::TargetManagementComponent>(),
            azrtti_typeid<AzFramework::AssetSystem::AssetSystemComponent>(),
            azrtti_typeid<AzFramework::InputSystemComponent>(),

#if !defined(AZCORE_EXCLUDE_LUA)
            azrtti_typeid<AZ::ScriptSystemComponent>(),
            azrtti_typeid<AzFramework::ScriptComponent>(),
#endif // #if !defined(AZCORE_EXCLUDE_LUA)
        };

        EBUS_EVENT(AzFramework::MetricsPlainTextNameRegistrationBus, RegisterForNameSending, componentUuidsForMetricsCollection);
    }

    void Application::ReflectSerialize()
    {
        AZ::ComponentApplication::ReflectSerialize();
        AZ::DataPatch::Reflect(*m_serializeContext);
        AZ::EntityUtils::Reflect(*m_serializeContext);
        AzFramework::EntityContext::ReflectSerialize(*m_serializeContext);
        AzFramework::SimpleAssetReferenceBase::Reflect(*m_serializeContext);
        AzFramework::EntityReference::Reflect(m_serializeContext);
    }

    void Application::RegisterComponentDescriptor(const AZ::ComponentDescriptor* descriptor)
    {
        AZ::ComponentApplication::RegisterComponentDescriptor(descriptor);
        if (m_networkContext)
        {
            descriptor->Reflect(m_networkContext.get());
        }
    }

    void Application::UnregisterComponentDescriptor(const AZ::ComponentDescriptor* descriptor)
    {
        AZ::ComponentApplication::UnregisterComponentDescriptor(descriptor);
        if (m_networkContext)
        {
            m_networkContext->EnableRemoveReflection();
            descriptor->Reflect(m_networkContext.get());
            m_networkContext->DisableRemoveReflection();
        }
    }

    AZ::ComponentTypeList Application::GetRequiredSystemComponents() const
    {
        AZ::ComponentTypeList components = ComponentApplication::GetRequiredSystemComponents();

        components.insert(components.end(), {
            azrtti_typeid<AZ::MemoryComponent>(),
            azrtti_typeid<AZ::StreamerComponent>(),
            azrtti_typeid<AZ::AssetManagerComponent>(),
            azrtti_typeid<AZ::UserSettingsComponent>(),
            azrtti_typeid<AZ::ScriptSystemComponent>(),
            azrtti_typeid<AZ::JobManagerComponent>(),
            azrtti_typeid<AZ::SliceSystemComponent>(),

            azrtti_typeid<AzFramework::BootstrapReaderComponent>(),
            azrtti_typeid<AzFramework::AssetCatalogComponent>(),
            azrtti_typeid<AzFramework::GameEntityContextComponent>(),
            azrtti_typeid<AzFramework::AssetSystem::AssetSystemComponent>(),
            azrtti_typeid<AzFramework::InputSystemComponent>(),

            AZ::Uuid("{624a7be2-3c7e-4119-aee2-1db2bdb6cc89}"), // ScriptDebugAgent
        });

        return components;
    }

    AZ::Component* Application::EnsureComponentAdded(AZ::Entity* systemEntity, const AZ::Uuid& typeId)
    {
        AZ::Component* component = systemEntity->FindComponent(typeId);

        if (!component)
        {
            if (systemEntity->IsComponentReadyToAdd(typeId))
            {
                component = systemEntity->CreateComponent(typeId);
            }
            else
            {
                AZ_Assert(false, "Failed to add component of type %s because conditions are not met.", typeId.ToString<AZStd::string>().c_str());
            }
        }

        return component;
    }

    void Application::CalculateAppRoot(const char* appRootOverride) // = nullptr
    {
        CalculateExecutablePath();

        if (appRootOverride)
        {
            azstrcpy(m_appRoot, AZ_ARRAY_SIZE(m_appRoot), appRootOverride);
        }
        else
        {
#if   defined(AZ_PLATFORM_ANDROID)
            // On Android, all file reads should be relative.
            azstrcpy(m_appRoot, AZ_ARRAY_SIZE(m_appRoot), "");
#elif defined(AZ_PLATFORM_APPLE_OSX)
            azstrcpy(m_appRoot, AZ_ARRAY_SIZE(m_appRoot), m_exeDirectory); //We could use this for all platforms.
#elif defined(AZ_PLATFORM_APPLE_IOS) || defined(AZ_PLATFORM_APPLE_TV)
            AZ_Assert(appRootOverride != nullptr, "App root must be set explicitly for apple platforms.");
#else
            // We need to reliably calculate the root path here prior to global engine init.
            // The system allocator is not yet initialized, so it's important we don't try
            // to allocate from the heap or use AZStd::string in here.

            // getcwd (and GetCurrentDirectoryA) return the directory from which the
            // application was launched, not the directory containing the executable.
            // This should be changed, but this will need to be implemented for each
            // platform that we support:
            // http://stackoverflow.com/questions/143174/how-do-i-get-the-directory-that-a-program-is-running-from
            // http://stackoverflow.com/questions/1023306/finding-current-executables-path-without-proc-self-exe/1024937#1024937
            static char currentDirectory[AZ_MAX_PATH_LEN];
#   if defined(AZ_COMPILER_MSVC)
            GetCurrentDirectoryA(sizeof(currentDirectory), currentDirectory);
#   else
            getcwd(currentDirectory, sizeof(currentDirectory));
#   endif // defined(AZ_COMPILER_MSVC)

            AZStd::replace(currentDirectory, currentDirectory + AZ_MAX_PATH_LEN, '\\', '/');

#ifdef BINFOLDER_NAME
            const size_t binRootCutoff = StringFunc::Find(currentDirectory, "/" BINFOLDER_NAME, 0, true, false);
#else
            const size_t binRootCutoff = StringFunc::Find(currentDirectory, "/bin", 0, true, false);
#endif // BINFOLDER_NAME
            if (AZStd::string::npos != binRootCutoff)
            {
                currentDirectory[binRootCutoff + 1] = '\0';
            }

            // Add a trailing slash if one does not already exist
            if (currentDirectory[strlen(currentDirectory) - 1] != '/')
            {
                azstrncat(currentDirectory, AZ_ARRAY_SIZE(currentDirectory), "/", 1);
            }

            azstrcpy(m_appRoot, AZ_ARRAY_SIZE(m_appRoot), currentDirectory);
#endif
        }

        // Asset root defaults to application root. The SetAssetRoot Ebus message can be used
        // to set a game-specific asset root.
        azstrcpy(m_assetRoot, AZ_ARRAY_SIZE(m_assetRoot), m_appRoot);

        ComponentApplication::NormalizePath(std::begin(m_assetRoot), std::end(m_assetRoot), true);
    }

    const char* Application::GetCurrentConfigurationName() const
    {
#if defined(_RELEASE)
        return "Release";
#elif defined(_DEBUG)
        return "Debug";
#else
        return "Profile";
#endif
    }
    void Application::CreateNetworkContext()
    {
        // Setup NetworkContext
        m_networkContext.reset(aznew NetworkContext());
        EBUS_EVENT(AZ::ComponentDescriptorBus, Reflect, m_networkContext.get());
    }

    ////////////////////////////////////////////////////////////////////////////
    AZ::Uuid Application::GetComponentTypeId(const AZ::EntityId& entityId, const AZ::ComponentId& componentId)
    {
        AZ::Uuid uuid(AZ::Uuid::CreateNull());
        AZ::Entity* entity = nullptr;
        EBUS_EVENT_RESULT(entity, AZ::ComponentApplicationBus, FindEntity, entityId);
        if (entity)
        {
            AZ::Component* component = entity->FindComponent(componentId);
            if (component)
            {
                uuid = component->RTTI_GetType();
            }
        }
        return uuid;
    }

    ////////////////////////////////////////////////////////////////////////////
    void Application::SetAssetRoot(const char* assetRoot)
    {
        const size_t len = strlen(assetRoot);
        if (len >= AZ_ARRAY_SIZE(m_assetRoot))
        {
            AZ_Assert(false, "Asset path is too long: %s", assetRoot);
            return;
        }

        // work in temporary buffer, copy to m_assetRoot if everything goes well.
        char tmpAssetRoot[AZ_ARRAY_SIZE(m_assetRoot)];
        azstrcpy(tmpAssetRoot, AZ_ARRAY_SIZE(m_assetRoot), assetRoot);

        // Ensure a trailing slash.
        if (len > 0)
        {
            if (tmpAssetRoot[len - 1] != '/')
            {
                if (len + 1 >= AZ_ARRAY_SIZE(m_assetRoot))
                {
                    AZ_Assert(false, "Asset path is too long: %s", tmpAssetRoot);
                    return;
                }

                tmpAssetRoot[len] = '/';
                tmpAssetRoot[len + 1] = '\0';
            }
        }

        azstrcpy(m_assetRoot, AZ_ARRAY_SIZE(m_assetRoot), tmpAssetRoot);
        
        ComponentApplication::NormalizePath(std::begin(m_assetRoot), std::end(m_assetRoot), true);
    }

    ////////////////////////////////////////////////////////////////////////////
    void Application::MakePathRootRelative(AZStd::string& fullPath) 
    { 
        MakePathRelative(fullPath, m_appRoot); 
    }

    ////////////////////////////////////////////////////////////////////////////
    void Application::MakePathAssetRootRelative(AZStd::string& fullPath) 
    { 
        MakePathRelative(fullPath, m_assetRoot); 
    }

    ////////////////////////////////////////////////////////////////////////////
    void Application::MakePathRelative(AZStd::string& fullPath, const char* rootPath)
    {
        AZ_Assert(rootPath, "Provided root path is null.");

        NormalizePath(fullPath);

        if (strstr(fullPath.c_str(), rootPath) == fullPath.c_str())
        {
            const size_t len = strlen(rootPath);
            fullPath = fullPath.substr(len);
        }

        while (!fullPath.empty() && fullPath[0] == '/')
        {
            fullPath.erase(fullPath.begin());
        }
    }

    ////////////////////////////////////////////////////////////////////////////
    void Application::NormalizePath(AZStd::string& path)
    {
        ComponentApplication::NormalizePath(path.begin(), path.end(), true);
    }

    ////////////////////////////////////////////////////////////////////////////
    void Application::NormalizePathKeepCase(AZStd::string& path)
    {
        ComponentApplication::NormalizePath(path.begin(), path.end(), false);
    }

} // namespace AzFramework
