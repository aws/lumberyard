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
#include <AzCore/Module/ModuleManagerBus.h>

#include <AzFramework/Asset/SimpleAsset.h>
#include <AzFramework/Asset/AssetCatalogComponent.h>
#include <AzFramework/Asset/AssetSystemComponent.h>
#include <AzFramework/Asset/AssetRegistry.h>
#include <AzFramework/Components/ConsoleBus.h>
#include <AzFramework/Components/TransformComponent.h>
#include <AzFramework/Components/BootstrapReaderComponent.h>
#include <AzFramework/Entity/BehaviorEntity.h>
#include <AzFramework/Entity/EntityContext.h>
#include <AzFramework/Entity/GameEntityContextComponent.h>
#include <AzFramework/FileFunc/FileFunc.h>
#include <AzFramework/Input/System/InputSystemComponent.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzFramework/Network/NetBindingComponent.h>
#include <AzFramework/Network/NetBindingSystemComponent.h>
#include <AzFramework/Script/ScriptRemoteDebugging.h>
#include <AzFramework/Script/ScriptComponent.h>
#include <AzFramework/TargetManagement/TargetManagementComponent.h>
#include <AzFramework/Driller/RemoteDrillerInterface.h>
#include <AzFramework/Network/NetworkContext.h>
#include <AzFramework/Metrics/MetricsPlainTextNameRegistration.h>
#include <GridMate/Memory.h>

#include "Application.h"
#include <AzFramework/AzFrameworkModule.h>
#include <cctype>
#include <stdio.h>

static const char* s_azFrameworkWarningWindow = "AzFramework";

static const char* s_engineConfigFileName = "engine.json";
static const char* s_engineConfigExternalEnginePathKey = "ExternalEnginePath";
static const char* s_engineConfigEngineVersionKey = "LumberyardVersion";

#if defined(AZ_PLATFORM_ANDROID) || defined(AZ_PLATFORM_APPLE)
    #include <unistd.h>
#endif //AZ_PLATFORM_ANDROID  ||  AZ_PLATFORM_APPLE

namespace AzFramework
{
    namespace ApplicationInternal
    {
        // this is used when no argV/ArgC is supplied.
        // in order to have the same memory semantics (writable, non-const)
        // we create a buffer that can be written to (up to AZ_MAX_PATH_LEN) and then
        // pack it with a single param.
        static char  s_commandLineBuffer[AZ_MAX_PATH_LEN] = "no_argv_supplied";
        static char* s_commandLineAsCharPtr = s_commandLineBuffer;
        static char** s_argVUninitialized = &s_commandLineAsCharPtr; // non const, so it is compatible with char***
        static int s_argCUninitialized = 1;

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
            AZ::ObjectStream::FilterDescriptor loadFilter(&AZ::Data::AssetFilterNoAssetLoading, AZ::ObjectStream::FILTERFLAG_IGNORE_UNKNOWN_CLASSES);

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
    : Application(nullptr, nullptr)
    {
    }
    
    Application::Application(int* argc, char*** argv)
        : m_appRootInitialized(false)
    {
        m_configFilePath[0] = '\0';
        m_appRoot[0] = '\0';
        m_assetRoot[0] = '\0';
        m_engineRoot[0] = '\0';
        

        if ((argc) && (argv))
        {
            m_argC = argc;
            m_argV = argv;
        }
        else
        {
            // use a "valid" value here.  This is becuase Qt and potentially other third party libraries require
            // that ArgC be 'at least 1' and that (*argV)[0] be a valid pointer to a real null terminated string.
            m_argC = &ApplicationInternal::s_argCUninitialized;
            m_argV = &ApplicationInternal::s_argVUninitialized;
        }

        ApplicationRequests::Bus::Handler::BusConnect();
        AZ::UserSettingsFileLocatorBus::Handler::BusConnect();
        NetSystemRequestBus::Handler::BusConnect();
    }

    int* Application::GetArgC() const
    {
        return m_argC;
    }

    char*** Application::GetArgV() const
    {
        return m_argV;
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
        const char* modulePathStr = modulePath.c_str();
        if (AzFramework::StringFunc::Path::IsRelative(modulePathStr))
        {
            const char* searchPaths[] = { m_appRoot , m_engineRoot };
            for (const char* searchPath : searchPaths)
            {
                AZStd::string testPath = AZStd::string::format("%s" BINFOLDER_NAME AZ_CORRECT_FILESYSTEM_SEPARATOR_STRING AZ_DYNAMIC_LIBRARY_PREFIX "%s", searchPath, modulePathStr);
                AZStd::string testPathWithPrefixAndExt = AZStd::string::format("%s" AZ_DYNAMIC_LIBRARY_EXTENSION, testPath.c_str(), modulePathStr);
                if (AZ::IO::SystemFile::Exists(testPathWithPrefixAndExt.c_str()))
                {
                    StringFunc::Path::Normalize(testPath);
                    modulePath = testPath.c_str();
                    return;
                }
            }
            // Fallback to the default logic
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
            AZ::OSAllocator::Descriptor osAllocatorDesc;
            osAllocatorDesc.m_custom = startupParameters.m_allocator;
            AZ::AllocatorInstance<AZ::OSAllocator>::Create(osAllocatorDesc);
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
        AZ::Debug::Trace::Error(__FILE__, __LINE__, AZ_FUNCTION_SIGNATURE, "Application", "Failed to find app descriptor file \"%s\".", configFilePath);
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

        AZ::ModuleManagerRequests::LoadModulesResult loadModuleOutcomes;
        AZ::ModuleManagerRequestBus::BroadcastResult(loadModuleOutcomes, &AZ::ModuleManagerRequests::LoadDynamicModules, descriptor.m_modules, AZ::ModuleInitializationSteps::RegisterComponentDescriptors, true);

        for (const auto& loadModuleOutcome : loadModuleOutcomes)
        {
            AZ_Error("Application", loadModuleOutcome.IsSuccess(), loadModuleOutcome.GetError().c_str());
            if (!loadModuleOutcome.IsSuccess())
            {
                return false;
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

        m_pimpl.reset(Implementation::Create());

        if ((m_argC) && (m_argV))
        {
            m_commandLine.Parse(*m_argC, *m_argV);
        }
        else
        {
            m_commandLine.Parse();
        }
        

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

    void Application::PreModuleLoad()
    {
        AZ_Assert(m_appRootInitialized, "App Root not initialized yet");

        // Calculate the engine root by reading the engine.json file
        AZStd::string  engineJsonPath = AZStd::string::format("%s%s", m_appRoot, s_engineConfigFileName);
        AZ::IO::LocalFileIO localFileIO;
        auto readJsonResult = AzFramework::FileFunc::ReadJsonFile(engineJsonPath, &localFileIO);

        if (readJsonResult.IsSuccess())
        {
            rapidjson::Document engineJson = readJsonResult.TakeValue();

            auto externalEnginePath = engineJson.FindMember(s_engineConfigExternalEnginePathKey);
            if (externalEnginePath != engineJson.MemberEnd())
            {
                // Initialize the engine root to the value of the external engine path key in the json file if it exists
                auto engineExternalPathString = externalEnginePath->value.GetString();
                SetRootPath(RootPathType::EngineRoot, engineExternalPathString);
            }
            else
            {
                // If the external engine path key does not exist in the json file, then set the engine root to be the same
                // as the app root
                SetRootPath(RootPathType::EngineRoot, m_appRoot);
            }
            AZ_TracePrintf(s_azFrameworkWarningWindow, "Engine Path: %s", m_engineRoot);
        }
        else
        {
            // If there is any problem reading the engine.json file, then default to engine root to the app root
            AZ_Warning(s_azFrameworkWarningWindow, false, "Unable to read engine.json file '%s' (%s).  Defaulting the engine root to '%s'", engineJsonPath.c_str(), readJsonResult.GetError().c_str(), m_appRoot);
            SetRootPath(RootPathType::EngineRoot, m_appRoot);
        }
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
            m_pimpl.reset();

            /* The following line of code is a temporary fix (CR: https://lumberyard-cr.amazon.com/r/1648621/) 
             * GridMate's ReplicaChunkDescriptor is stored in a global environment variable 'm_globalDescriptorTable'
             * which does not get cleared when Application shuts down. We need to un-reflect here to clear ReplicaChunkDescriptor
             * so that ReplicaChunkDescriptor::m_vdt doesn't get flooded when we repeatedly instantiate Application in unit tests.
             */
            m_reflectionManager->RemoveReflectContext<NetworkContext>();

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
#if !defined(_RELEASE)
            azrtti_typeid<AzFramework::TargetManagementComponent>(),
#endif
            azrtti_typeid<AzFramework::AssetSystem::AssetSystemComponent>(),
            azrtti_typeid<AzFramework::InputSystemComponent>(),
            azrtti_typeid<AzFramework::DrillerNetworkAgentComponent>(),

#if !defined(AZCORE_EXCLUDE_LUA)
            azrtti_typeid<AZ::ScriptSystemComponent>(),
            azrtti_typeid<AzFramework::ScriptComponent>(),
#endif // #if !defined(AZCORE_EXCLUDE_LUA)
        };

        EBUS_EVENT(AzFramework::MetricsPlainTextNameRegistrationBus, RegisterForNameSending, componentUuidsForMetricsCollection);
    }

    void Application::Reflect(AZ::ReflectContext* context)
    {
        AZ::ComponentApplication::Reflect(context);

        AZ::DataPatch::Reflect(context);
        AZ::EntityUtils::Reflect(context);
        AzFramework::BehaviorEntity::Reflect(context);
        AzFramework::EntityContext::Reflect(context);
        AzFramework::SimpleAssetReferenceBase::Reflect(context);
        AzFramework::ConsoleRequests::Reflect(context);

        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            AzFramework::AssetRegistry::ReflectSerialize(serializeContext);
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
            azrtti_typeid<AzFramework::DrillerNetworkAgentComponent>(),

            AZ::Uuid("{624a7be2-3c7e-4119-aee2-1db2bdb6cc89}"), // ScriptDebugAgent
        });

        return components;
    }

    AZStd::string Application::ResolveFilePath(AZ::u32 providerId)
    {
        (void)providerId;

        AZStd::string result;
        AzFramework::StringFunc::Path::Join(GetAppRoot(), "UserSettings.xml", result, /*bJoinOverlapping*/false, /*bCaseInsenitive*/false);
        return result;
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
        if (appRootOverride)
        {
            SetRootPath(RootPathType::AppRoot, appRootOverride);
        }
        else
        {
            CalculateExecutablePath();
#if defined(AZ_RESTRICTED_PLATFORM)
    #if defined(AZ_PLATFORM_XENIA)
        #include "Xenia/Application_cpp_xenia.inl"
    #elif defined(AZ_PLATFORM_PROVO)
        #include "Provo/Application_cpp_provo.inl"
    #endif
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif defined(AZ_PLATFORM_ANDROID)
            // On Android, all file reads should be relative.
            SetRootPath(RootPathType::AppRoot, "");
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
            static const char* engineRootFile = "engine.json";
            static const size_t engineRootFileLength = strlen(engineRootFile);

            CalculateExecutablePath();
            const char* exeFolderPath = GetExecutableFolder();
            azstrncpy(currentDirectory, AZ_ARRAY_SIZE(currentDirectory), exeFolderPath, strlen(exeFolderPath));

            AZ_Assert(strlen(currentDirectory) > 0, "Failed to get current working directory.");

            AZStd::replace(currentDirectory, currentDirectory + AZ_MAX_PATH_LEN, '\\', '/');

            // Add a trailing slash if one does not already exist
            if (currentDirectory[strlen(currentDirectory) - 1] != '/')
            {
                azstrncat(currentDirectory, AZ_ARRAY_SIZE(currentDirectory), "/", 1);
            }

            // this closures assumes that there is a '/' character at the end
            auto cdPathUp = [](char path[])
            {
                size_t separatorCutoff = StringFunc::Find(path, "/", 1, true, false);
                if (separatorCutoff != AZStd::string::npos)
                {
                    path[separatorCutoff + 1] = '\0';
                }

                return separatorCutoff;
            };

            size_t separatorCutoff = AZStd::string::npos;
            do
            {
                // Add the file to check for into the directory path
                azstrncat(currentDirectory, AZ_ARRAY_SIZE(currentDirectory), engineRootFile, engineRootFileLength);

                // Look for the file that identifies the engine root
                bool engineFileExists = AZ::IO::SystemFile::Exists(currentDirectory);

                // remove the filename appended previously to do the engine root file check
                cdPathUp(currentDirectory);

                if (engineFileExists)
                {
                    break;
                }

                separatorCutoff = cdPathUp(currentDirectory);

            } while (separatorCutoff != AZStd::string::npos);

            // Add a trailing slash if one does not already exist
            if (currentDirectory[strlen(currentDirectory) - 1] != '/')
            {
                azstrncat(currentDirectory, AZ_ARRAY_SIZE(currentDirectory), "/", 1);
            }
            SetRootPath(RootPathType::AppRoot, currentDirectory);
#endif
        }

        // Asset root defaults to application root. The SetAssetRoot Ebus message can be used
        // to set a game-specific asset root.
        SetRootPath(RootPathType::AssetRoot, m_appRoot);
    }

    void Application::CreateStaticModules(AZStd::vector<AZ::Module*>& outModules)
    {
        AZ::ComponentApplication::CreateStaticModules(outModules);

        outModules.emplace_back(aznew AzFrameworkModule());
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

    void Application::CreateReflectionManager()
    {
        ComponentApplication::CreateReflectionManager();

        // Setup NetworkContext
        m_reflectionManager->AddReflectContext<NetworkContext>();
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
    NetworkContext* Application::GetNetworkContext()
    {
        return m_reflectionManager ? m_reflectionManager->GetReflectContext<NetworkContext>() : nullptr;
    }

    bool Application::IsEngineExternal() const
    {
        return (azstrnicmp(m_engineRoot, m_appRoot, AZ_ARRAY_SIZE(m_engineRoot)) != 0);
    }

    void Application::ResolveEnginePath(AZStd::string& engineRelativePath) const
    {
        AZStd::string fullPath = AZStd::string(m_engineRoot) + AZStd::string(AZ_CORRECT_FILESYSTEM_SEPARATOR_STRING) + engineRelativePath;
        engineRelativePath = fullPath;
    }

    void Application::CalculateBranchTokenForAppRoot(AZStd::string& token) const
    {
        AzFramework::StringFunc::AssetPath::CalculateBranchToken(AZStd::string(m_appRoot), token);
    }

    ////////////////////////////////////////////////////////////////////////////
    void Application::SetAssetRoot(const char* assetRoot)
    {
        SetRootPath(RootPathType::AssetRoot, assetRoot);
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

    ////////////////////////////////////////////////////////////////////////////
    void Application::PumpSystemEventLoopOnce()
    {
        if (m_pimpl)
        {
            m_pimpl->PumpSystemEventLoopOnce();
        }
    }

    ////////////////////////////////////////////////////////////////////////////
    void Application::PumpSystemEventLoopUntilEmpty()
    {
        if (m_pimpl)
        {
            m_pimpl->PumpSystemEventLoopUntilEmpty();
        }
    }

    ////////////////////////////////////////////////////////////////////////////
    void Application::RunMainLoop()
    {
        while (!m_exitMainLoopRequested)
        {
            PumpSystemEventLoopUntilEmpty();
            Tick();
        }
    }

    void Application::SetRootPath(RootPathType type, const char* source)
    {
        size_t sourceLen = strlen(source);

        // Determine if we need to append a trailing path separator
        bool appendTrailingPathSep = (sourceLen>0) ? ((source[sourceLen - 1] != AZ_CORRECT_FILESYSTEM_SEPARATOR) && (source[sourceLen - 1] != AZ_WRONG_FILESYSTEM_SEPARATOR)) : false;

        // Copy the source path to the intended root path and correct the path separators as well
        switch (type)
        {
        case RootPathType::AppRoot:
            if (sourceLen == 0)
            {
                m_appRoot[0] = '\0';
            }
            else
            {
                AZ_Assert((sourceLen + 1) < AZ_ARRAY_SIZE(m_appRoot) - 1, "String overflow for App Root: %s", source);
                azstrncpy(m_appRoot, AZ_ARRAY_SIZE(m_appRoot) - 1, source, sourceLen);

                AZStd::replace(std::begin(m_appRoot), std::end(m_appRoot), AZ_WRONG_FILESYSTEM_SEPARATOR, AZ_CORRECT_FILESYSTEM_SEPARATOR);
                m_appRoot[sourceLen] = appendTrailingPathSep ? AZ_CORRECT_FILESYSTEM_SEPARATOR : '\0';
                m_appRoot[sourceLen + 1] = '\0';
            }
            m_appRootInitialized = true;
            break;
        case RootPathType::AssetRoot:
            if (sourceLen == 0)
            {
                m_assetRoot[0] = '\0';
            }
            else
            {
                AZ_Assert((sourceLen + 1) < AZ_ARRAY_SIZE(m_assetRoot) - 1, "String overflow for Asset Root: %s", source);
                azstrncpy(m_assetRoot, AZ_ARRAY_SIZE(m_assetRoot) - 1, source, sourceLen);

                AZStd::replace(std::begin(m_assetRoot), std::end(m_assetRoot), AZ_WRONG_FILESYSTEM_SEPARATOR, AZ_CORRECT_FILESYSTEM_SEPARATOR);
                m_assetRoot[sourceLen] = appendTrailingPathSep ? AZ_CORRECT_FILESYSTEM_SEPARATOR : '\0';
                m_assetRoot[sourceLen + 1] = '\0';
            }
            break;
        case RootPathType::EngineRoot:
            if (sourceLen == 0)
            {
                m_engineRoot[0] = '\0';
            }
            else
            {
                AZ_Assert((sourceLen + 1) < AZ_ARRAY_SIZE(m_engineRoot) - 1, "String overflow for Engine Root: %s", source);
                azstrncpy(m_engineRoot, AZ_ARRAY_SIZE(m_engineRoot) - 1, source, sourceLen);

                AZStd::replace(std::begin(m_engineRoot), std::end(m_engineRoot), AZ_WRONG_FILESYSTEM_SEPARATOR, AZ_CORRECT_FILESYSTEM_SEPARATOR);
                m_engineRoot[sourceLen] = appendTrailingPathSep ? AZ_CORRECT_FILESYSTEM_SEPARATOR : '\0';
                m_engineRoot[sourceLen + 1] = '\0';
            }
            break;
            default:
                AZ_Assert(false, "Invalid RootPathType (%d)", static_cast<int>(type));
            }
    }


} // namespace AzFramework
