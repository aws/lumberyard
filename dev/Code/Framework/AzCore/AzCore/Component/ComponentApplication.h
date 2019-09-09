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
#ifndef AZCORE_COMPONENT_APPLICATION_H
#define AZCORE_COMPONENT_APPLICATION_H

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Debug/ProfileModuleInit.h>
#include <AzCore/Memory/AllocationRecords.h>
#include <AzCore/Memory/OSAllocator.h>
#include <AzCore/Module/DynamicModuleHandle.h>
#include <AzCore/Module/ModuleManager.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/RTTI/ReflectionManager.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/string/osstring.h>
#include <AzCore/std/string/conversions.h>

#define BIN_FOLDER_NAME_MAX_SIZE    128

namespace AZ
{
    class BehaviorContext;
    class Module;
    class ModuleManager;

    namespace Debug
    {
        class DrillerManager;
    }

    /**
     * A main class that can be used directly or as a base to start a
     * component based application. It will provide all the proper bootstrap
     * and entity bookkeeping functionality.
     *
     * IMPORTANT: If you use this as a base class you must follow one rule. You can't add
     * data that will allocate memory on construction. This is because the memory managers
     * are NOT yet ready. They will be initialized during the Create call.
     */
    class ComponentApplication
        : public ComponentApplicationBus::Handler
        , public TickRequestBus::Handler
    {
        // or try to use unordered set if we store the ID internally
        typedef AZStd::unordered_map<EntityId, Entity*>  EntitySetType;

    public:
        AZ_RTTI(ComponentApplication, "{1F3B070F-89F7-4C3D-B5A3-8832D5BC81D7}");
        AZ_CLASS_ALLOCATOR(ComponentApplication, SystemAllocator, 0);

        /**
         * Configures the component application.
         * \note This structure may be loaded from a file on disk. Values that
         *       must be set by a running application should go into StartupParameters.
         * \note It's important this structure not contain members that allocate from the system allocator.
         *       Use the OSAllocator only.
         */
        struct Descriptor
            : public SerializeContext::IObjectFactory
        {
            AZ_TYPE_INFO(ComponentApplication::Descriptor, "{70277A3E-2AF5-4309-9BBF-6161AFBDE792}");
            AZ_CLASS_ALLOCATOR(ComponentApplication::Descriptor, SystemAllocator, 0);

            ///////////////////////////////////////////////
            // SerializeContext::IObjectFactory
            void* Create(const char* name) override;
            void  Destroy(void* data) override;
            ///////////////////////////////////////////////

            /// Reflect the descriptor data.
            static void     Reflect(ReflectContext* context, ComponentApplication* app);

            /// @deprecated
            AZ_DEPRECATED(static void ReflectSerialize(SerializeContext* context, ComponentApplication* app), "Function deprecated, use ComponentApplication::Descriptor::Reflect() instead");

            Descriptor();

            bool            m_useExistingAllocator;     ///< True if the user is creating the system allocation and setup tracking modes, if this is true all other parameters are IGNORED. (default: false)
            bool            m_grabAllMemory;            ///< True if we want to grab all available memory minus reserved fields. (default: false)
            bool            m_allocationRecords;        ///< True if we want to track memory allocations, otherwise false. (default: true)
            bool            m_allocationRecordsSaveNames; ///< True if we want to allocate space for saving the name/filename of each allocation so unloaded module memory leaks have valid names to read, otherwise false. (default: false, automatically true with recording mode FULL)
            bool            m_allocationRecordsAttemptDecodeImmediately; ///< True if we want to attempt decoding frames at time of allocation, otherwise false. Very expensive, used specifically for debugging allocations that fail to decode. (default: false)
            bool            m_autoIntegrityCheck;       ///< True to check the heap integrity on each allocation/deallocation. (default: false)
            bool            m_markUnallocatedMemory;    ///< True to mark all memory with 0xcd when it's freed. (default: true)
            bool            m_doNotUsePools;            ///< True of we want to pipe all allocation to a generic allocator (not pools), this can help debugging a memory stomp. (default: false)
            bool            m_enableScriptReflection;   ///< True if we want to enable reflection to the script context.

            unsigned int    m_pageSize;                 ///< Page allocation size must be 1024 bytes aligned. (default: SystemAllocator::Descriptor::Heap::m_defaultPageSize)
            unsigned int    m_poolPageSize;             ///< Page size used to small memory allocations. Must be less or equal to m_pageSize and a multiple of it. (default: SystemAllocator::Descriptor::Heap::m_defaultPoolPageSize)
            unsigned int    m_memoryBlockAlignment;     ///< Alignment of memory block. (default: SystemAllocator::Descriptor::Heap::m_memoryBlockAlignment)
            AZ::u64         m_memoryBlocksByteSize;     ///< Memory block size in bytes if. This parameter is ignored if m_grabAllMemory is set to true. (default: 0 - use memory on demand, no preallocation)
            AZ::u64         m_reservedOS;               ///< Reserved memory for the OS in bytes. Used only when m_grabAllMemory is set to true. (default: 0)
            AZ::u64         m_reservedDebug;            ///< Reserved memory for Debugging (allocation,etc.). Used only when m_grabAllMemory is set to true. (default: 0)
            Debug::AllocationRecords::Mode m_recordingMode; ///< When to record stack traces (default: AZ::Debug::AllocationRecords::RECORD_STACK_IF_NO_FILE_LINE)
            AZ::u64         m_stackRecordLevels;        ///< If stack recording is enabled, how many stack levels to record. (default: 5)
            bool            m_enableDrilling;           ///< True to enabled drilling support for the application. RegisterDrillers will be called. Ignored in release. (default: true)

            ModuleDescriptorList m_modules;             ///< Dynamic modules used by the application.
                                                        ///< These will be loaded on startup.

            ///////////////////////////////////////////////
            /// PLATFORM
            /**
             * Used only on X360 to indicate is we want to allocate physical memory. The page size is determined by m_pageSize // ACCEPTED_USE
             * so if m_pageSize >= 64 KB we will use MEM_LARGE_PAGES otherwise small 4 KB pages.
             * default: false
             */
            bool            m_x360IsPhysicalMemory; // ACCEPTED_USE
            ///////////////////////////////////////////////
        };

        /// Application settings.
        /// Unlike the Descriptor, these values must be set in code and cannot be loaded from a file.
        struct StartupParameters
        {
            StartupParameters() {}

            /// If set, this allocator is used to allocate the temporary bootstrap memory, as well as the main \ref SystemAllocator heap.
            /// If it's left nullptr (default), the \ref OSAllocator will be used.
            IAllocatorAllocate* m_allocator = nullptr;

            /// Callback to create AZ::Modules for the static libraries linked by this application.
            /// Leave null if the application uses no static AZ::Modules.
            /// \note Dynamic AZ::Modules are specified in the ComponentApplication::Descriptor.
            CreateStaticModulesCallback m_createStaticModulesCallback = nullptr;

            /// If set, this is used as the app root folder instead of it being calculated.
            const char* m_appRootOverride = nullptr;

            /// Whether or not to load dynamic modules described by \ref Descriptor::m_modules
            bool m_loadDynamicModules = true;

            /// Specifies which system components to create & activate. If no tags specified, all system components are used. Specify as comma separated list.
            const char* m_systemComponentTags = nullptr;
        };

        ComponentApplication();
        virtual ~ComponentApplication();

        /**
         * Loads the application configuration and systemEntity from 'applicationDescriptorFile' (path relative to AppRoot).
         * It is expected that the first node in the file will be the descriptor, for memory manager creation.
         * \returns pointer to the system entity.
         */
        virtual Entity* Create(const char* applicationDescriptorFile,
            const StartupParameters& startupParameters = StartupParameters());
        /**
         * Create system allocator and system entity. No components are added to the system node.
         * You will need to setup all system components manually.
         */
        virtual Entity* Create(const Descriptor& descriptor,
            const StartupParameters& startupParameters = StartupParameters());
        virtual void Destroy();

        //////////////////////////////////////////////////////////////////////////
        // ComponentApplicationRequests
        void RegisterComponentDescriptor(const ComponentDescriptor* descriptor) override final;
        void UnregisterComponentDescriptor(const ComponentDescriptor* descriptor) override final;
        bool AddEntity(Entity* entity) override;
        bool RemoveEntity(Entity* entity) override;
        bool DeleteEntity(const EntityId& id) override;
        Entity* FindEntity(const EntityId& id) override;
        AZStd::string GetEntityName(const EntityId& id) override;
        void EnumerateEntities(const ComponentApplicationRequests::EntityCallback& callback) override;
        ComponentApplication* GetApplication() override { return this; }
        /// Returns the serialize context that has been registered with the app, if there is one.
        SerializeContext* GetSerializeContext() override;
        /// Returns the behavior context that has been registered with the app, if there is one.
        BehaviorContext* GetBehaviorContext() override;
        /// Returns the working root folder that has been registered with the app, if there is one.
        /// It's expected that derived applications will implement an application root.
        const char* GetAppRoot() override { return ""; }
        /// Returns the path to the folder the executable is in.
        const char* GetExecutableFolder() const override { return m_exeDirectory; }
        /// Returns the bin folder name where the application is running from. The folder is relative to the engine root.
        const char* GetBinFolder() const override { return m_binFolder; }


        /// Returns pointer to the driller manager if it's enabled, otherwise NULL.
        Debug::DrillerManager* GetDrillerManager() override { return m_drillerManager; }
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        /// TickRequestBus
        float GetTickDeltaTime() override;
        ScriptTimePoint GetTimeAtCurrentTick() override;
        //////////////////////////////////////////////////////////////////////////

        Descriptor& GetDescriptor() { return m_descriptor; }

        /**
         * Ticks all components using the \ref AZ::TickBus during simulation time. May not tick if the application is not active (i.e. not in focus)
         */
        virtual void Tick(float deltaOverride = -1.f);

        /**
        * Ticks all using the \ref AZ::SystemTickBus at all times. Should always tick even if the application is not active.
        */
        virtual void TickSystem();

        /**
         * This is utility function, it will store the full state of the descriptor and system entity.
         * This way if you did any changes they will be stored for the next time.
         */
        bool        WriteApplicationDescriptor(const char* fileName);

        /**
         * Application-overridable way to state required system components.
         * These components will be added to the system entity if they were
         * not already provided by the application descriptor.
         * \return the type-ids of required components.
         */
        virtual ComponentTypeList GetRequiredSystemComponents() const;

        /**
         * ResolveModulePath is called whenever LoadDynamicModule wants to resolve a module in order to actually load it.
         * You can override this if you need to load modules from a different path or hijack module loading in some other way.
         * If you do, ensure that you use platform-specific conventions to do so, as this is called by multiple platforms.
         * The default implantation prepends the path to the executable to the module path, but you can override this behavior
         * (Call the base class if you want this behavior to persist in overrides)
         */
        void ResolveModulePath(AZ::OSString& modulePath) override;

    protected:
        virtual void CreateReflectionManager();
        void DestroyReflectionManager();

        /// Perform any additional initialization needed before loading modules
        virtual void PreModuleLoad() {};

        virtual void CreateStaticModules(AZStd::vector<AZ::Module*>& outModules);

        /// Common logic shared between the multiple Create(...) functions.
        void        CreateCommon();

        /// Create the system allocator using the data in the m_descriptor
        void        CreateSystemAllocator();

        /// Create the drillers
        void        CreateDrillers();

        /**
         * This is the function that will be called instantly after the memory
         * manager is created. This is where we should register all core component
         * factories that will participate in the loading of the bootstrap file
         * or all factories in general.
         * When you create your own application this is where you should FIRST call
         * ComponentApplication::RegisterCoreComponents and then register the application
         * specific core components.
         */
        virtual void RegisterCoreComponents();

        /**
         * \deprecated Use GetRequiredSystemComponents instead.
         */
        virtual void AddSystemComponents(AZ::Entity* systemEntity);

        /**
         * @deprecated
         */
        virtual void ReflectSerialize() {}

        /*
         * Reflect classes from this framework to the appropriate context.
         * Subclasses of AZ::Component should not be listed here, they are reflected through the ComponentDescriptorBus.
         */
        virtual void Reflect(ReflectContext* context);

        /// Check if a System Component should be created
        bool ShouldAddSystemComponent(AZ::ComponentDescriptor* descriptor);

        /// Adds system components requested by modules and the application to the system entity.
        void AddRequiredSystemComponents(AZ::Entity* systemEntity);

        /// Calculates the directory the application executable comes from.
        void CalculateExecutablePath();

        /// Calculates the Bin folder name where the application is running from (off of the engine root folder)
        void CalculateBinFolder();

        /// Adjusts an input descriptor path to the app's root path.
        AZ::OSString GetFullPathForDescriptor(const char* descriptorPath);

        /**
         * Check/verify a given path for the engine marker (file) so that we can identify that
         * a given path is the engine root. This is only valid for target platforms that are built
         * for the host platform and not deployable (ie windows, mac). 
         * @param fullPath The full path to look for the engine marker
         * @return true if the input path contains the engine marker file, false if not
         */
        virtual bool CheckPathForEngineMarker(const char* fullPath) const;

        template<typename Iterator>
        static void NormalizePath(Iterator begin, Iterator end, bool doLowercase = true)
        {
            AZStd::replace(begin, end, '\\', '/');
            if (doLowercase)
            {
                AZStd::to_lower(begin, end);
            }
        }

        AZStd::chrono::system_clock::time_point     m_currentTime;
        float                                       m_deltaTime;
        AZStd::unique_ptr<ModuleManager>            m_moduleManager;
        AZStd::unique_ptr<ReflectionManager>        m_reflectionManager;
        Descriptor                                  m_descriptor;
        bool                                        m_isStarted;
        bool                                        m_isSystemAllocatorOwner;
        bool                                        m_isOSAllocatorOwner;
        void*                                       m_fixedMemoryBlock;                  ///< Pointer to the memory block allocator, so we can free it OnDestroy.
        IAllocatorAllocate*                         m_osAllocator;
        EntitySetType                               m_entities;
        char                                        m_exeDirectory[AZ_MAX_PATH_LEN];
        char                                        m_binFolder[BIN_FOLDER_NAME_MAX_SIZE];

        Debug::DrillerManager*                      m_drillerManager;

        StartupParameters                           m_startupParameters;

private:
        AZ::Debug::ProfileModuleInitializer m_moduleProfilerInit;
    };
}

#endif // AZCORE_COMPONENT_APPLICATION_H
#pragma once
