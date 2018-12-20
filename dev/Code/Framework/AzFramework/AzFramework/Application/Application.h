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

#ifndef AZFRAMEWORK_APPLICATION_H
#define AZFRAMEWORK_APPLICATION_H

#include <AzCore/base.h>
#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/UserSettings/UserSettings.h>
#include <AzCore/Math/Crc.h>
#include <AzCore/Math/Uuid.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <AzFramework/Network/NetSystemBus.h>
#include <AzFramework/CommandLine/CommandLine.h>
#include <AzFramework/API/ApplicationAPI.h>


#pragma once

namespace AZ
{
    class Component;

    namespace Internal
    {
        class ComponentFactoryInterface;
    }

    namespace IO
    {
        class LocalFileIO;
    }
}

namespace AzFramework
{
    class Application
        : public AZ::ComponentApplication
        , public AZ::UserSettingsFileLocatorBus::Handler
        , public ApplicationRequests::Bus::Handler
        , public NetSystemRequestBus::Handler
    {
    public:
        // Base class for platform specific implementations of the application.
        class Implementation
        {
        public:
            static Implementation* Create();
            virtual ~Implementation() = default;
            virtual void PumpSystemEventLoopOnce() = 0;
            virtual void PumpSystemEventLoopUntilEmpty() = 0;
        };

        AZ_RTTI(Application, "{0BD2388B-F435-461C-9C84-D0A96CAF32E4}", AZ::ComponentApplication);
        AZ_CLASS_ALLOCATOR(Application, AZ::SystemAllocator, 0);

        // Publicized types & methods from base ComponentApplication.
        using AZ::ComponentApplication::Descriptor;
        using AZ::ComponentApplication::StartupParameters;
        using AZ::ComponentApplication::GetSerializeContext;
        using AZ::ComponentApplication::RegisterComponentDescriptor;

        /**
         * You can pass your command line parameters from main here
         * so that they are thus available in GetCommandLine later, and can be retrieved.
         * see notes in GetArgC() and GetArgV() for details about the arguments.
         */
        Application(int* argc, char*** argv);  ///< recommended:  supply &argc and &argv from void main(...) here.
        Application(); ///< for backward compatibility.  If you call this, GetArgC and GetArgV will return nullptr.
        ~Application();

        /**
        * Retrieve the argc passed into the application class on startup, if any was passed in.
        * Note that this could return nullptr if the application was not initialized with any such parameter.
        * This is important to have because different operating systems have different level of access to the command line args
        * and on some operating systems (MacOS) its fairly difficult to reliably retrieve them without resorting to NS libraries
        * and making some assumptions.  Instead, we allow you to pass your args in from void main().
        * Another thing to notice here is that these are non-const pointers to the argc and argv values
        * instead of int, char**, these are int*, char***.  
        * This is becuase some application layers (such as Qt) actually require that the ArgC and ArgV are modifiable, 
        * as they actually patch them to add/remove command line parameters during initialization.
        * but also to highlight the fact that they are pointers to static memory that must remain relevant throughout the existence
        * of the Application object.
        * For best results, simply pass in &argc and &argv from your void main(argc, argv) in here - that memory is 
        * permanently tied to your process and is going to be available at all times during run.
        */
        int* GetArgC() const;

        /**
        * Retrieve the argv parameter passed into the application class on startup.  see the note on ArgC
        * Note that this could return nullptr if the application was not initialized with any such parameter.
        */
        char*** GetArgV() const;

        /**
         * Executes AZ::ComponentApplication::Create with the args given and initializes Application specific constructs.
         */
        virtual void Start(const Descriptor& descriptor, const StartupParameters& startupParameters = StartupParameters());
        virtual void Start(const char* applicationDescriptorFile, const StartupParameters& startupParameters = StartupParameters());

        /**
         * Executes AZ::ComponentApplication::Destroy, and shuts down Application specific constructs.
         */
        void Stop();

        void SaveConfiguration();

        virtual void CalculateAppRoot(const char* appRootOverride = nullptr);

        AZ::ComponentTypeList GetRequiredSystemComponents() const override;
        void CreateStaticModules(AZStd::vector<AZ::Module*>& outModules) override;

        //////////////////////////////////////////////////////////////////////////
        //! ApplicationRequests::Bus::Handler
        const char* GetAssetRoot() const override { return m_assetRoot; }
        const char* GetEngineRoot() const override { return m_engineRoot; }
        const char* GetAppRoot() const override { return m_appRoot; }
        bool IsEngineExternal() const override;
        void ResolveEnginePath(AZStd::string& engineRelativePath) const override;
        void CalculateBranchTokenForAppRoot(AZStd::string& token) const override;


        const CommandLine* GetCommandLine() override { return &m_commandLine; }
        void SetAssetRoot(const char* assetRoot) override;
        void MakePathRootRelative(AZStd::string& fullPath) override;
        void MakePathAssetRootRelative(AZStd::string& fullPath) override;
        void MakePathRelative(AZStd::string& fullPath, const char* rootPath) override;
        void NormalizePath(AZStd::string& path) override;
        void NormalizePathKeepCase(AZStd::string& path) override;
        void PumpSystemEventLoopOnce() override;
        void PumpSystemEventLoopUntilEmpty() override;
        void RunMainLoop() override;
        void ExitMainLoop() { m_exitMainLoopRequested = true; }
        bool WasExitMainLoopRequested() { return m_exitMainLoopRequested; }
        AZ::Uuid GetComponentTypeId(const AZ::EntityId& entityId, const AZ::ComponentId& componentId) override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        //! NetSystemEventBus::Handler
        //////////////////////////////////////////////////////////////////////////
        NetworkContext* GetNetworkContext() override;

        /** ReflectModulesFromAppDescriptor - Utility function to load all the modules in the app descriptor file
        * And ensure that their components are reflected.
        * After calling this function and having it succeed all the modules specified will be
        * loaded into the module system and any reflected components they mention in their
        * module classes will be reflected into the reflection context(s)
        */
        bool ReflectModulesFromAppDescriptor(const char* appDescriptorFilePath);

    protected:
        /**
        * Called by Start(const char*, ...) when the passed in config file path can't be loaded.
        * Returns true to continue Start, false to exit out.
        */
        virtual bool OnFailedToFindConfiguration(const char* configFilePath);

        /**
         * Called by all overloads of Start(...). Override to add custom startup logic.
         */
        virtual void StartCommon(AZ::Entity* systemEntity);

        void PreModuleLoad() override;

        //////////////////////////////////////////////////////////////////////////
        //! AZ::ComponentApplication
        const char* GetAppRoot() override { return m_appRoot; }
        void RegisterCoreComponents() override;
        void Reflect(AZ::ReflectContext* context) override;
        void ResolveModulePath(AZ::OSString& modulePath) override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        //! UserSettingsFileLocatorBus
        AZStd::string ResolveFilePath(AZ::u32 providerId) override
        {
            (void)providerId;

            return AZStd::string(GetAppRoot()) + "/UserSettings.xml";
        }
        //////////////////////////////////////////////////////////////////////////

        AZ::Component* EnsureComponentAdded(AZ::Entity* systemEntity, const AZ::Uuid& typeId);

        template <typename ComponentType>
        AZ::Component* EnsureComponentAdded(AZ::Entity* systemEntity)
        {
            return EnsureComponentAdded(systemEntity, ComponentType::RTTI_Type());
        }

        virtual const char* GetCurrentConfigurationName() const;

        void CreateReflectionManager() override;

        AzFramework::CommandLine m_commandLine;
        char m_configFilePath[AZ_MAX_PATH_LEN];

        bool m_appRootInitialized;
        char m_appRoot[AZ_MAX_PATH_LEN];
        char m_assetRoot[AZ_MAX_PATH_LEN];
        char m_engineRoot[AZ_MAX_PATH_LEN]; ///> Location of the engine root folder that this application is based on

        AZStd::unique_ptr<AZ::IO::LocalFileIO> m_defaultFileIO; ///> Default file IO instance is a LocalFileIO.
        AZStd::unique_ptr<Implementation> m_pimpl;

        bool m_exitMainLoopRequested = false;
        
        // please see note in application constructor for details about these.
        int*    m_argC = nullptr;
        char*** m_argV = nullptr;
        
        enum class RootPathType
        {
            AppRoot,
            AssetRoot,
            EngineRoot
        };
        void SetRootPath(RootPathType type, const char* source);
    };
} // namespace AzFramework

#endif // AZFRAMEWORK_APPLICATION_H
