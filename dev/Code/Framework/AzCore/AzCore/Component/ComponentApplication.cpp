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
#ifndef AZ_UNITY_BUILD

#include <AzCore/PlatformIncl.h>
#include <AzCore/AzCoreModule.h>

#include <AzCore/Casting/numeric_cast.h>

#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Component/TickBus.h>

#include <AzCore/Memory/AllocationRecords.h>

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/ObjectStream.h>
#include <AzCore/Serialization/Utils.h>

#include <AzCore/RTTI/AttributeReader.h>
#include <AzCore/RTTI/BehaviorContext.h>

#include <AzCore/Module/Module.h>
#include <AzCore/Module/ModuleManager.h>

#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/SystemFile.h>

#include <AzCore/Driller/Driller.h>
#include <AzCore/Memory/MemoryDriller.h>
#include <AzCore/Debug/TraceMessagesDriller.h>
#include <AzCore/Debug/ProfilerDriller.h>
#include <AzCore/Debug/EventTraceDriller.h>

#include <AzCore/Script/ScriptSystemBus.h>

#include <AzCore/Math/PolygonPrism.h>
#include <AzCore/Math/Spline.h>
#include <AzCore/Math/VertexContainer.h>

#include <AzCore/UserSettings/UserSettingsComponent.h>

#include <AzCore/XML/rapidxml.h>

#if defined(AZ_ENABLE_DEBUG_TOOLS)
#include <AzCore/Debug/StackTracer.h>
#endif // defined(AZ_ENABLE_DEBUG_TOOLS)

#if defined(AZ_PLATFORM_APPLE)
#   include <mach-o/dyld.h>
#endif

namespace AZ
{
    //=========================================================================
    // ComponentApplication::Descriptor
    // [5/30/2012]
    //=========================================================================
    ComponentApplication::Descriptor::Descriptor()
    {
        m_useExistingAllocator = false;
        m_grabAllMemory = false;
        m_allocationRecords = true;
        m_allocationRecordsSaveNames = false;
        m_allocationRecordsAttemptDecodeImmediately = false;
        m_autoIntegrityCheck = false;
        m_markUnallocatedMemory = true;
        m_doNotUsePools = false;
        m_enableScriptReflection = true;

        m_pageSize = SystemAllocator::Descriptor::Heap::m_defaultPageSize;
        m_poolPageSize = SystemAllocator::Descriptor::Heap::m_defaultPoolPageSize;
        m_memoryBlockAlignment = SystemAllocator::Descriptor::Heap::m_memoryBlockAlignment;
        m_memoryBlocksByteSize = 0;
        m_reservedOS = 0;
        m_reservedDebug = 0;
        m_recordingMode = Debug::AllocationRecords::RECORD_STACK_IF_NO_FILE_LINE;
        m_stackRecordLevels = 5;
        m_enableDrilling = true;

        m_x360IsPhysicalMemory = false; // ACCEPTED_USE
    }

    bool AppDescriptorConverter(SerializeContext& serialize, SerializeContext::DataElementNode& node)
    {
        if (node.GetVersion() < 2)
        {
            int nodeIdx = node.FindElement(AZ_CRC("recordsMode", 0x764c147a));
            if (nodeIdx != -1)
            {
                auto& subNode = node.GetSubElement(nodeIdx);

                char oldValue = 0;
                subNode.GetData(oldValue);
                subNode.Convert<Debug::AllocationRecords::Mode>(serialize);
                subNode.SetData<Debug::AllocationRecords::Mode>(serialize, aznumeric_caster(oldValue));
                subNode.SetName("recordingMode");
            }

            nodeIdx = node.FindElement(AZ_CRC("stackRecordLevels", 0xf8492566));
            if (nodeIdx != -1)
            {
                auto& subNode = node.GetSubElement(nodeIdx);

                unsigned char oldValue = 0;
                subNode.GetData(oldValue);
                subNode.Convert<AZ::u64>(serialize);
                subNode.SetData<AZ::u64>(serialize, aznumeric_caster(oldValue));
            }
        }

        return true;
    };

    //=========================================================================
    // ReflectSerialize
    // [5/30/2012]
    //=========================================================================
    void ComponentApplication::Descriptor::ReflectSerialize(SerializeContext* serializeContext, ComponentApplication* app)
    {
        AZ_Warning("Application", false, "ComponentApplication::Descriptor::ReflectSerialize() is deprecated, use Reflect() instead.");
        ComponentApplication::Descriptor::Reflect(serializeContext, app);
    }

    //=========================================================================
    // Reflect
    //=========================================================================
    void  ComponentApplication::Descriptor::Reflect(ReflectContext* context, ComponentApplication* app)
    {
        DynamicModuleDescriptor::Reflect(context);

        if (auto serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext->Class<Descriptor>(&app->GetDescriptor())
                ->Version(2, AppDescriptorConverter)
                ->Field("useExistingAllocator", &Descriptor::m_useExistingAllocator)
                ->Field("grabAllMemory", &Descriptor::m_grabAllMemory)
                ->Field("allocationRecords", &Descriptor::m_allocationRecords)
                ->Field("allocationRecordsSaveNames", &Descriptor::m_allocationRecordsSaveNames)
                ->Field("allocationRecordsAttemptDecodeImmediately", &Descriptor::m_allocationRecordsAttemptDecodeImmediately)
                ->Field("recordingMode", &Descriptor::m_recordingMode)
                ->Field("stackRecordLevels", &Descriptor::m_stackRecordLevels)
                ->Field("autoIntegrityCheck", &Descriptor::m_autoIntegrityCheck)
                ->Field("markUnallocatedMemory", &Descriptor::m_markUnallocatedMemory)
                ->Field("doNotUsePools", &Descriptor::m_doNotUsePools)
                ->Field("enableScriptReflection", &Descriptor::m_enableScriptReflection)
                ->Field("pageSize", &Descriptor::m_pageSize)
                ->Field("poolPageSize", &Descriptor::m_poolPageSize)
                ->Field("blockAlignment", &Descriptor::m_memoryBlockAlignment)
                ->Field("blockSize", &Descriptor::m_memoryBlocksByteSize)
                ->Field("reservedOS", &Descriptor::m_reservedOS)
                ->Field("reservedDebug", &Descriptor::m_reservedDebug)
                ->Field("enableDrilling", &Descriptor::m_enableDrilling)
                ->Field("modules", &Descriptor::m_modules)
                ->Field("x360PhysicalMemory", &Descriptor::m_x360IsPhysicalMemory) // ACCEPTED_USE
                ;

            if (EditContext* ec = serializeContext->GetEditContext())
            {
                ec->Enum<Debug::AllocationRecords::Mode>("Debug::AllocationRecords::Mode", "Allocator recording mode")
                    ->Value("No records", Debug::AllocationRecords::RECORD_NO_RECORDS)
                    ->Value("No stack trace", Debug::AllocationRecords::RECORD_STACK_NEVER)
                    ->Value("Stack trace when file/line missing", Debug::AllocationRecords::RECORD_STACK_IF_NO_FILE_LINE)
                    ->Value("Stack trace always", Debug::AllocationRecords::RECORD_FULL);
                ec->Class<Descriptor>("System memory settings", "Settings for managing application memory usage")
                    ->ClassElement(Edit::ClassElements::EditorData, "")
                        ->Attribute(Edit::Attributes::AutoExpand, true)
                    ->DataElement(Edit::UIHandlers::CheckBox, &Descriptor::m_grabAllMemory, "Allocate all memory at startup", "Allocate all system memory at startup if enabled, or allocate as needed if disabled")
                    ->DataElement(Edit::UIHandlers::CheckBox, &Descriptor::m_allocationRecords, "Record allocations", "Collect information on each allocation made for debugging purposes (ignored in Release builds)")
                    ->DataElement(Edit::UIHandlers::CheckBox, &Descriptor::m_allocationRecordsSaveNames, "Record allocations with name saving", "Saves names/filenames information on each allocation made, useful for tracking down leaks in dynamic modules (ignored in Release builds)")
                    ->DataElement(Edit::UIHandlers::CheckBox, &Descriptor::m_allocationRecordsAttemptDecodeImmediately, "Record allocations and attempt immediate decode", "Decode callstacks for each allocation when they occur, used for tracking allocations that fail decoding. Very expensive. (ignored in Release builds)")
                    ->DataElement(Edit::UIHandlers::ComboBox, &Descriptor::m_recordingMode, "Stack recording mode", "Stack record mode. (Ignored in final builds)")
                    ->DataElement(Edit::UIHandlers::SpinBox, &Descriptor::m_stackRecordLevels, "Stack entries to record", "Number of stack levels to record for each allocation (ignored in Release builds)")
                        ->Attribute(Edit::Attributes::Step, 1)
                        ->Attribute(Edit::Attributes::Max, 1024)
                    ->DataElement(Edit::UIHandlers::CheckBox, &Descriptor::m_autoIntegrityCheck, "Validate allocations", "Check allocations for integrity on each allocation/free (ignored in Release builds)")
                    ->DataElement(Edit::UIHandlers::CheckBox, &Descriptor::m_markUnallocatedMemory, "Mark freed memory", "Set memory to 0xcd when a block is freed for debugging (ignored in Release builds)")
                    ->DataElement(Edit::UIHandlers::CheckBox, &Descriptor::m_doNotUsePools, "Don't pool allocations", "Pipe pool allocations in system/tree heap (ignored in Release builds)")
                    ->DataElement(Edit::UIHandlers::SpinBox, &Descriptor::m_pageSize, "Page size", "Memory page size in bytes (must be OS page size aligned)")
                        ->Attribute(Edit::Attributes::Step, 1024)
                    ->DataElement(Edit::UIHandlers::SpinBox, &Descriptor::m_poolPageSize, "Pool page size", "Memory pool page size in bytes (must be a multiple of page size)")
                        ->Attribute(Edit::Attributes::Max, &Descriptor::m_pageSize)
                        ->Attribute(Edit::Attributes::Step, 1024)
                    ->DataElement(Edit::UIHandlers::SpinBox, &Descriptor::m_memoryBlockAlignment, "Block alignment", "Memory block alignment in bytes (must be multiple of the page size)")
                        ->Attribute(Edit::Attributes::Step, &Descriptor::m_pageSize)
                    ->DataElement(Edit::UIHandlers::SpinBox, &Descriptor::m_memoryBlocksByteSize, "Block size", "Memory block size in bytes (must be multiple of the page size)")
                        ->Attribute(Edit::Attributes::Step, &Descriptor::m_pageSize)
                    ->DataElement(Edit::UIHandlers::SpinBox, &Descriptor::m_reservedOS, "OS reserved memory", "System memory reserved for OS (used only when 'Allocate all memory at startup' is true)")
                    ->DataElement(Edit::UIHandlers::SpinBox, &Descriptor::m_reservedDebug, "Memory reserved for debugger", "System memory reserved for Debug allocator, like memory tracking (used only when 'Allocate all memory at startup' is true)")
                    ->DataElement(Edit::UIHandlers::CheckBox, &Descriptor::m_enableDrilling, "Enable Driller", "Enable Drilling support for the application (ignored in Release builds)")
                    ->DataElement(Edit::UIHandlers::CheckBox, &Descriptor::m_x360IsPhysicalMemory, "Physical memory", "Used only on X360 to indicate is we want to allocate physical memory") // ACCEPTED_USE
                    ;
            }
        }
    }

    //=========================================================================
    // Create
    //=========================================================================
    void* ComponentApplication::Descriptor::Create(const char* name)
    {
        (void)name;
        return this; /// we the the factory and the object as we are part of the component application
    }

    //=========================================================================
    // Destroy
    //=========================================================================
    void ComponentApplication::Descriptor::Destroy(void* data)
    {
        // do nothing as descriptor is part of the component application
        (void)data;
    }

    //=========================================================================
    // ComponentApplication
    // [5/30/2012]
    //=========================================================================
    ComponentApplication::ComponentApplication()
    {
        m_isStarted = false;
        m_isSystemAllocatorOwner = false;
        m_isOSAllocatorOwner = false;
        m_fixedMemoryBlock = nullptr;
        m_osAllocator = nullptr;
        m_currentTime = AZStd::chrono::system_clock::time_point::max();
        m_drillerManager = nullptr;

        m_deltaTime = 0.f;
        m_exeDirectory[0] = '\0';
    }

    //=========================================================================
    // ~ComponentApplication
    // [5/30/2012]
    //=========================================================================
    ComponentApplication::~ComponentApplication()
    {
        if (m_isStarted)
        {
            Destroy();
        }
    }

    //=========================================================================
    // Create
    // [5/30/2012]
    //=========================================================================
    Entity* ComponentApplication::Create(const char* applicationDescriptorFile,
        const StartupParameters& startupParameters)
    {
        AZ_Assert(!m_isStarted, "Component descriptor already started!");

        m_startupParameters = startupParameters;

        if (!startupParameters.m_allocator)
        {
            if (!AZ::AllocatorInstance<AZ::OSAllocator>::IsReady())
            {
                AZ::AllocatorInstance<AZ::OSAllocator>::Create();
                m_isOSAllocatorOwner = true;
            }
            m_osAllocator = &AZ::AllocatorInstance<AZ::OSAllocator>::Get();
        }
        else
        {
            m_osAllocator = startupParameters.m_allocator;
        }

        // Make the path root-relative unless it's already a full path.
        AZ::OSString fullApplicationDescriptorPath = GetFullPathForDescriptor(applicationDescriptorFile);

        // Open the file
        AZ_Assert(!fullApplicationDescriptorPath.empty(), "Application descriptor filename cannot be NULL!");
        IO::SystemFile cfg;
        cfg.Open(fullApplicationDescriptorPath.c_str(), IO::SystemFile::SF_OPEN_READ_ONLY);
        size_t len = static_cast<size_t>(cfg.Length());
        AZ_Assert(len > 0, "Application descriptor file %s is invalid!", applicationDescriptorFile);
        (void)len;
        IO::SystemFileStream stream(&cfg, false);

        AZ::SystemAllocator::Descriptor desc;

        // Create temporary system allocator to initialize the serialization context, after that the SystemAllocator
        // will be created by CreateSystemAllocator
        const bool isExistingSystemAllocator = AZ::AllocatorInstance<AZ::SystemAllocator>::IsReady();

        if (!isExistingSystemAllocator)
        {
            enum
            {
                TemporaryBlockSize = 10 *  1024 * 1024
            };

            desc.m_heap.m_numFixedMemoryBlocks = 1;
            desc.m_heap.m_fixedMemoryBlocksByteSize[0] = TemporaryBlockSize;
            desc.m_heap.m_fixedMemoryBlocks[0] = m_osAllocator->Allocate(desc.m_heap.m_fixedMemoryBlocksByteSize[0], desc.m_heap.m_memoryBlockAlignment);
            AZ::AllocatorInstance<AZ::SystemAllocator>::Create(desc);
        }

        // Get the system allocator configuration
        {
            SerializeContext sc;
            Descriptor::Reflect(&sc, this);
            AZ::Utils::LoadObjectFromStreamInPlace(stream, m_descriptor, &sc, ObjectStream::FilterDescriptor(&AZ::Data::AssetFilterNoAssetLoading, ObjectStream::FILTERFLAG_IGNORE_UNKNOWN_CLASSES));
        }

        // Destroy the temp system allocator
        if (!isExistingSystemAllocator)
        {
            AZ::AllocatorInstance<AZ::SystemAllocator>::Destroy();
            m_osAllocator->DeAllocate(desc.m_heap.m_fixedMemoryBlocks[0], desc.m_heap.m_fixedMemoryBlocksByteSize[0], desc.m_heap.m_memoryBlockAlignment);
        }

        CreateCommon();

        // Create system entity. Reset the descriptor in preparation for reserializing it.
        m_descriptor = Descriptor();
        stream.Seek(0, IO::GenericStream::ST_SEEK_BEGIN);

        AZ::Entity* systemEntity = nullptr;
        ObjectStream::LoadBlocking(&stream, *GetSerializeContext(), [this, &systemEntity](void* classPtr, const AZ::Uuid& classId, const AZ::SerializeContext* sc) {
            if (ModuleEntity* moduleEntity = sc->Cast<ModuleEntity*>(classPtr, classId))
            {
                m_moduleManager->AddModuleEntity(moduleEntity);
            }
            else if (Entity* entity = sc->Cast<Entity*>(classPtr, classId))
            {
                if (entity->GetId() == AZ::SystemEntityId)
                {
                    systemEntity = entity;
                }
            }
            else if (!sc->Cast<Descriptor*>(classPtr, classId))
            {
                char idStr[Uuid::MaxStringBuffer];
                classId.ToString(idStr, AZ_ARRAY_SIZE(idStr));
                AZ_Error("ComponentApplication", false, "Unknown class type %p %s", classPtr, idStr);
            }
        }, 
            ObjectStream::FilterDescriptor(&AZ::Data::AssetFilterNoAssetLoading, ObjectStream::FILTERFLAG_IGNORE_UNKNOWN_CLASSES));
        // we cannot load assets during system bootstrap since we haven't even got the catalog yet.  But we can definitely read their IDs.

        AZ_Assert(systemEntity, "SystemEntity failed to load!");
        cfg.Close();

        AddRequiredSystemComponents(systemEntity);
        m_isStarted = true;
        return systemEntity;
    }

    //=========================================================================
    // Create
    // [5/31/2012]
    //=========================================================================
    Entity* ComponentApplication::Create(const Descriptor& descriptor,
        const StartupParameters& startupParameters)
    {
        AZ_Assert(!m_isStarted, "Component application already started!");


        if (startupParameters.m_allocator == nullptr)
        {
            if (!AZ::AllocatorInstance<AZ::OSAllocator>::IsReady())
            {
                AZ::AllocatorInstance<AZ::OSAllocator>::Create();
                m_isOSAllocatorOwner = true;
            }
            m_osAllocator = &AZ::AllocatorInstance<AZ::OSAllocator>::Get();
        }
        else
        {
            m_osAllocator = startupParameters.m_allocator;
        }

        m_descriptor = descriptor;
        m_startupParameters = startupParameters;
        CreateCommon();

        AZ::Entity* systemEntity = aznew Entity(SystemEntityId, "SystemEntity");
        AddRequiredSystemComponents(systemEntity);
        m_isStarted = true;
        return systemEntity;
    }

    //=========================================================================
    // CreateCommon
    //=========================================================================
    void ComponentApplication::CreateCommon()
    {
        CalculateExecutablePath();

        CreateDrillers();

        CreateSystemAllocator();

        CreateReflectionManager();

        // Call this and child class's reflects
        m_reflectionManager->Reflect(azrtti_typeid(this), AZStd::bind(&ComponentApplication::Reflect, this, AZStd::placeholders::_1));

        RegisterCoreComponents();

        TickBus::AllowFunctionQueuing(true);
        SystemTickBus::AllowFunctionQueuing(true);

        ComponentApplicationBus::Handler::BusConnect();

        m_currentTime = AZStd::chrono::system_clock::now();
        TickRequestBus::Handler::BusConnect();

#if defined(AZ_ENABLE_DEBUG_TOOLS)
        // Prior to loading more modules, we make sure SymbolStorage
        // is listening for the loads so it can keep track of which
        // modules we may eventually need symbols for
        Debug::SymbolStorage::RegisterModuleListeners();
#endif // defined(AZ_ENABLE_DEBUG_TOOLS)

        PreModuleLoad();

        // Setup the modules list
        m_moduleManager = AZStd::make_unique<ModuleManager>();
        m_moduleManager->SetSystemComponentTags(m_startupParameters.m_systemComponentTags);

        // Load static modules
        ModuleManagerRequestBus::Broadcast(&ModuleManagerRequests::LoadStaticModules, AZStd::bind(&ComponentApplication::CreateStaticModules, this, AZStd::placeholders::_1), ModuleInitializationSteps::RegisterComponentDescriptors);

        // Load dynamic modules if appropriate for the platform
        if (m_startupParameters.m_loadDynamicModules)
        {
            AZ::ModuleManagerRequests::LoadModulesResult loadModuleOutcomes;
            ModuleManagerRequestBus::BroadcastResult(loadModuleOutcomes, &ModuleManagerRequests::LoadDynamicModules, m_descriptor.m_modules, ModuleInitializationSteps::RegisterComponentDescriptors, true);

#if defined(AZ_ENABLE_TRACING)
            for (const auto& loadModuleOutcome : loadModuleOutcomes)
            {
                AZ_Error("ComponentApplication", loadModuleOutcome.IsSuccess(), loadModuleOutcome.GetError().c_str());
            }
#endif
        }
    }

    //=========================================================================
    // Destroy
    // [5/30/2012]
    //=========================================================================
    void ComponentApplication::Destroy()
    {
        // Finish all queued work
        AZ::SystemTickBus::Broadcast(&AZ::SystemTickBus::Events::OnSystemTick);
        
        TickBus::ExecuteQueuedEvents();
        TickBus::AllowFunctionQueuing(false);
        
        SystemTickBus::ExecuteQueuedEvents();
        SystemTickBus::AllowFunctionQueuing(false);

        AZ::UserSettingsComponentRequestBus::Broadcast(&AZ::UserSettingsComponentRequests::Finalize);

        // deactivate all entities
        Entity* systemEntity = nullptr;
        while (!m_entities.empty())
        {
            Entity* entity = m_entities.begin()->second;
            m_entities.erase(m_entities.begin());

            if (entity->GetId() == SystemEntityId)
            {
                systemEntity = entity;
            }
            else
            {
                delete entity;
            }
        }

        // Force full garbage collect after all game entities destroyed, but before modules are unloaded.
        // This is to ensure that all references to reflected classes/ebuses are cleaned up before the data is deleted.
        // This problem could also be solved by using ref-counting for reflected data.
        ScriptSystemRequestBus::Broadcast(&ScriptSystemRequests::GarbageCollect);

        // Deactivate all module entities before the System Entity is deactivated, but do not unload the modules as
        // components on the SystemEntity are allowed to reference module data at this point.
        m_moduleManager->DeactivateEntities();

        // deactivate all system components
        if (systemEntity)
        {
            if (systemEntity->GetState() == Entity::ES_ACTIVE)
            {
                systemEntity->Deactivate();
            }
        }

        m_entities.clear();
        m_entities.rehash(0); // force free all memory

        DestroyReflectionManager();

        // Uninit and unload any dynamic modules.
        m_moduleManager.reset();

        if (systemEntity)
        {
            delete systemEntity;
        }

        // delete all descriptors left for application clean up
        EBUS_EVENT(ComponentDescriptorBus, ReleaseDescriptor);

        // Disconnect from application and tick request buses
        ComponentApplicationBus::Handler::BusDisconnect();
        TickRequestBus::Handler::BusDisconnect();

        if (m_drillerManager)
        {
            Debug::DrillerManager::Destroy(m_drillerManager);
            m_drillerManager = nullptr;
        }

        // Clear the descriptor to deallocate all strings (owned by ModuleDescriptor)
        m_descriptor = Descriptor();

        // kill the system allocator if we created it
        if (m_isSystemAllocatorOwner)
        {
            AZ::AllocatorInstance<AZ::SystemAllocator>::Destroy();

            if (m_fixedMemoryBlock)
            {
                m_osAllocator->DeAllocate(m_fixedMemoryBlock);
            }
            m_fixedMemoryBlock = nullptr;
            m_isSystemAllocatorOwner = false;
        }



        if (m_isOSAllocatorOwner)
        {
            AZ::AllocatorInstance<AZ::OSAllocator>::Destroy();
            m_isOSAllocatorOwner = false;
        }

        m_osAllocator = nullptr;

        m_isStarted = false;

#if defined(AZ_ENABLE_DEBUG_TOOLS)
        // Unregister module listeners after allocators are destroyed
        // so that symbol/stack trace information is available at shutdown
        Debug::SymbolStorage::UnregisterModuleListeners();
#endif // defined(AZ_ENABLE_DEBUG_TOOLS)
    }

    //=========================================================================
    // CreateSystemAllocator
    // [5/30/2012]
    //=========================================================================
    void ComponentApplication::CreateSystemAllocator()
    {
        if (m_descriptor.m_useExistingAllocator || AZ::AllocatorInstance<AZ::SystemAllocator>::IsReady())
        {
            m_isSystemAllocatorOwner = false;
            AZ_Assert(AZ::AllocatorInstance<AZ::SystemAllocator>::IsReady(), "You must setup AZ::SystemAllocator instance, before you can call Create application with m_useExistingAllocator flag set to true");
        }
        else
        {
            // Create the system allocator
            AZ::SystemAllocator::Descriptor desc;
            desc.m_heap.m_pageSize = m_descriptor.m_pageSize;
            desc.m_heap.m_poolPageSize = m_descriptor.m_poolPageSize;
            if (m_descriptor.m_grabAllMemory)
            {
                // grab all available memory
                AZ::u64 availableOS = AZ_CORE_MAX_ALLOCATOR_SIZE;
                AZ::u64 reservedOS = m_descriptor.m_reservedOS;
                AZ::u64 reservedDbg = m_descriptor.m_reservedDebug;
                AZ_Warning("Memory", false, "This platform is not supported for grabAllMemory flag! Provide a valid allocation size and set the m_grabAllMemory flag to false! Using default max memory size %llu!", availableOS);
                AZ_Assert(availableOS > 0, "OS doesn't have any memory available!");
                // compute total memory to grab
                desc.m_heap.m_fixedMemoryBlocksByteSize[0] = static_cast<size_t>(availableOS - reservedOS - reservedDbg);
                // memory block MUST be a multiple of pages
                desc.m_heap.m_fixedMemoryBlocksByteSize[0] = AZ::SizeAlignDown(desc.m_heap.m_fixedMemoryBlocksByteSize[0], m_descriptor.m_pageSize);
            }
            else
            {
                desc.m_heap.m_fixedMemoryBlocksByteSize[0] = static_cast<size_t>(m_descriptor.m_memoryBlocksByteSize);
            }

            if (desc.m_heap.m_fixedMemoryBlocksByteSize[0] > 0) // 0 means one demand memory which we support
            {
                m_fixedMemoryBlock = m_osAllocator->Allocate(desc.m_heap.m_fixedMemoryBlocksByteSize[0], m_descriptor.m_memoryBlockAlignment);
                desc.m_heap.m_fixedMemoryBlocks[0] = m_fixedMemoryBlock;
                desc.m_heap.m_numFixedMemoryBlocks = 1;
            }
            desc.m_allocationRecords = m_descriptor.m_allocationRecords;
            desc.m_stackRecordLevels = aznumeric_caster(m_descriptor.m_stackRecordLevels);
            AZ::AllocatorInstance<AZ::SystemAllocator>::Create(desc);
            AZ::Debug::AllocationRecords* records = AllocatorInstance<SystemAllocator>::Get().GetRecords();
            if (records)
            {
                records->SetMode(m_descriptor.m_recordingMode);
                records->SetSaveNames(m_descriptor.m_allocationRecordsSaveNames);
                records->SetDecodeImmediately(m_descriptor.m_allocationRecordsAttemptDecodeImmediately);
                records->AutoIntegrityCheck(m_descriptor.m_autoIntegrityCheck);
                records->MarkUallocatedMemory(m_descriptor.m_markUnallocatedMemory);
            }

            m_isSystemAllocatorOwner = true;
        }
    }

    //=========================================================================
    // CreateDrillers
    // [2/20/2013]
    //=========================================================================
    void ComponentApplication::CreateDrillers()
    {
        // Create driller manager and register drillers if requested
        if (m_descriptor.m_enableDrilling)
        {
            m_drillerManager = Debug::DrillerManager::Create();
            // Memory driller is responsible for tracking allocations.
            // Tracking type and overhead is determined by app configuration.
            m_drillerManager->Register(aznew Debug::MemoryDriller);
            // Profiler driller will consume resources only when started.
            m_drillerManager->Register(aznew Debug::ProfilerDriller);
            // Trace messages driller will consume resources only when started.
            m_drillerManager->Register(aznew Debug::TraceMessagesDriller);
            m_drillerManager->Register(aznew Debug::EventTraceDriller);
        }
    }

    //=========================================================================
    // RegisterComponentDescriptor
    // [5/30/2012]
    //=========================================================================
    void ComponentApplication::RegisterComponentDescriptor(const ComponentDescriptor* descriptor)
    {
        if (m_reflectionManager)
        {
            m_reflectionManager->Reflect(descriptor->GetUuid(), AZStd::bind(&ComponentDescriptor::Reflect, descriptor, AZStd::placeholders::_1));
        }
    }

    //=========================================================================
    // RegisterComponentDescriptor
    //=========================================================================
    void ComponentApplication::UnregisterComponentDescriptor(const ComponentDescriptor* descriptor)
    {
        if (m_reflectionManager)
        {
            m_reflectionManager->Unreflect(descriptor->GetUuid());
        }
    }

    //=========================================================================
    // AddEntity
    // [5/30/2012]
    //=========================================================================
    bool ComponentApplication::AddEntity(Entity* entity)
    {
        AZ_Error("ComponentApplication", entity, "Input entity is null, cannot add entity");
        if (!entity)
        {
            return false;
        }

        return m_entities.insert(AZStd::make_pair(entity->GetId(), entity)).second;
    }

    //=========================================================================
    // RemoveEntity
    // [5/31/2012]
    //=========================================================================
    bool ComponentApplication::RemoveEntity(Entity* entity)
    {
        AZ_Error("ComponentApplication", entity, "Input entity is null, cannot remove entity");
        if (!entity)
        {
            return false;
        }

        return (m_entities.erase(entity->GetId()) == 1);
    }

    //=========================================================================
    // DeleteEntity
    // [5/31/2012]
    //=========================================================================
    bool ComponentApplication::DeleteEntity(const EntityId& id)
    {
        Entity* entity = FindEntity(id);
        if (entity)
        {
            delete entity;
            return true;
        }
        return false;
    }

    //=========================================================================
    // FindEntity
    // [5/30/2012]
    //=========================================================================
    Entity* ComponentApplication::FindEntity(const EntityId& id)
    {
        EntitySetType::const_iterator it = m_entities.find(id);
        if (it != m_entities.end())
        {
            return it->second;
        }
        return nullptr;
    }

    //=========================================================================
    // GetEntityName
    // [10/17/2016]
    //=========================================================================
    AZStd::string ComponentApplication::GetEntityName(const EntityId& id)
    {
        Entity* entity = FindEntity(id);
        if (entity)
        {
            return entity->GetName();
        }
        return AZStd::string();
    }

    //=========================================================================
    // EnumerateEntities
    //=========================================================================
    void ComponentApplication::EnumerateEntities(const ComponentApplicationRequests::EntityCallback& callback)
    {
        for (const auto& entityIter : m_entities)
        {
            callback(entityIter.second);
        }
    }

    //=========================================================================
    // GetSerializeContext
    //=========================================================================
    AZ::SerializeContext* ComponentApplication::GetSerializeContext()
    {
        return m_reflectionManager ? m_reflectionManager->GetReflectContext<SerializeContext>() : nullptr;
    }

    //=========================================================================
    // GetBehaviorContext
    //=========================================================================
    AZ::BehaviorContext* ComponentApplication::GetBehaviorContext()
    {
        return m_reflectionManager ? m_reflectionManager->GetReflectContext<BehaviorContext>() : nullptr;
    }

    //=========================================================================
    // CreateReflectionManager
    //=========================================================================
    void ComponentApplication::CreateReflectionManager()
    {
        m_reflectionManager = AZStd::make_unique<ReflectionManager>();

        m_reflectionManager->AddReflectContext<SerializeContext>();
        m_reflectionManager->AddReflectContext<BehaviorContext>();
    }

    //=========================================================================
    // DestroyReflectionManager
    //=========================================================================
    void ComponentApplication::DestroyReflectionManager()
    {
        // Must clear before resetting so that calls to GetSerializeContext et al will succeed will unreflecting
        m_reflectionManager->Clear();
        m_reflectionManager.reset();
    }

    //=========================================================================
    // CreateStaticModules
    //=========================================================================
    void ComponentApplication::CreateStaticModules(AZStd::vector<AZ::Module*>& outModules)
    {
        if (m_startupParameters.m_createStaticModulesCallback)
        {
            m_startupParameters.m_createStaticModulesCallback(outModules);
        }

        outModules.emplace_back(aznew AzCoreModule());
    }

    //=========================================================================
    // Tick
    //=========================================================================
    void ComponentApplication::Tick(float deltaOverride /*= -1.f*/)
    {
        {
            AZ_PROFILE_TIMER("System", "Component application simulation tick function");
            AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzCore);

            AZStd::chrono::system_clock::time_point now = AZStd::chrono::system_clock::now();

            m_deltaTime = 0.0f;

            if (now >= m_currentTime)
            {
                AZStd::chrono::duration<float> delta = now - m_currentTime;
                m_deltaTime = deltaOverride >= 0.f ? deltaOverride : delta.count();
            }

            {
                AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::AzCore, "ComponentApplication::Tick:ExecuteQueuedEvents");
                TickBus::ExecuteQueuedEvents();
            }
            m_currentTime = now;
            {
                AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::AzCore, "ComponentApplication::Tick:OnTick");
                EBUS_EVENT(TickBus, OnTick, m_deltaTime, ScriptTimePoint(now));
            }
        }
        if (m_drillerManager)
        {
            m_drillerManager->FrameUpdate();
        }
    }

    //=========================================================================
    // Tick
    //=========================================================================
    void ComponentApplication::TickSystem()
    {
        AZ_PROFILE_TIMER("System", "Component application system tick function");
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzCore);

        SystemTickBus::ExecuteQueuedEvents();
        EBUS_EVENT(SystemTickBus, OnSystemTick);
    }

    //=========================================================================
    // WriteApplicationDescriptor
    // [5/31/2012]
    //=========================================================================
    bool ComponentApplication::WriteApplicationDescriptor(const char* fileName)
    {
        Entity* systemEntity = FindEntity(SystemEntityId);
        if (systemEntity == nullptr)
        {
            return false;
        }

        AZStd::string tmpFileName(fileName);
        tmpFileName += ".tmp";

        AZStd::unique_ptr<IO::FileIOStream> fileStream(AZStd::make_unique<IO::FileIOStream>(tmpFileName.c_str(), IO::OpenMode::ModeWrite));
        if (fileStream == nullptr)
        {
            return false;
        }

        SerializeContext* serializeContext = GetSerializeContext();
        AZ_Assert(serializeContext, "ComponentApplication::m_serializeContext is NULL!");
        ObjectStream* objStream = ObjectStream::Create(fileStream.get(), *serializeContext, ObjectStream::ST_XML);
        bool descWriteOk = objStream->WriteClass(&m_descriptor);
        AZ_Warning("ComponentApplication", descWriteOk, "Failed to write memory descriptor to application descriptor file %s!", fileName);
        bool entityWriteOk = objStream->WriteClass(systemEntity);
        AZ_Warning("ComponentApplication", entityWriteOk, "Failed to write system entity to application descriptor file %s!", fileName);
        bool flushOk = objStream->Finalize();
        AZ_Warning("ComponentApplication", flushOk, "Failed finalizing application descriptor file %s!", fileName);
        fileStream->Close();

        if (descWriteOk && entityWriteOk && flushOk)
        {
            if (IO::SystemFile::Rename(tmpFileName.c_str(), fileName, true))
            {
                return true;
            }
            AZ_Warning("ComponentApplication", false, "Failed to rename %s to %s.", tmpFileName.c_str(), fileName);
        }
        return false;
    }

    //=========================================================================
    // RegisterCoreComponents
    //=========================================================================
    void ComponentApplication::RegisterCoreComponents()
    {
    }

    //=========================================================================
    // GetRequiredSystemComponents
    //=========================================================================
    ComponentTypeList ComponentApplication::GetRequiredSystemComponents() const
    {
        return ComponentTypeList();
    }

    //=========================================================================
    // AddSystemComponents
    //=========================================================================
    void ComponentApplication::AddSystemComponents(AZ::Entity* /*systemEntity*/)
    {
    }

    bool ComponentApplication::ShouldAddSystemComponent(AZ::ComponentDescriptor* descriptor)
    {
        // NOTE: This is different than modules! All system components must be listed in GetRequiredSystemComponents, and then AZ::Edit::Attributes::SystemComponentTags may be used to filter down from there
        if (m_moduleManager->GetSystemComponentTags().empty())
        {
            return true;
        }

        const SerializeContext::ClassData* classData = GetSerializeContext()->FindClassData(descriptor->GetUuid());
        AZ_Warning("ComponentApplication", classData, "Component type %s not reflected to SerializeContext!", descriptor->GetName());

        // Note, if there are no SystemComponentTags on the classData, we will return true
        // in order to maintain backwards compatibility with legacy non-tagged components
        return Edit::SystemComponentTagsMatchesAtLeastOneTag(classData, m_moduleManager->GetSystemComponentTags(), true);
    }

    //=========================================================================
    // AddRequiredSystemComponents
    //=========================================================================
    void ComponentApplication::AddRequiredSystemComponents(AZ::Entity* systemEntity)
    {
        //
        // Gather required system components from all modules and the application.
        //
        for (const Uuid& componentId : GetRequiredSystemComponents())
        {
            ComponentDescriptor* componentDescriptor = nullptr;
            EBUS_EVENT_ID_RESULT(componentDescriptor, componentId, ComponentDescriptorBus, GetDescriptor);
            if (!componentDescriptor)
            {
                AZ_Error("Module", false, "Failed to add system component required by application. No component descriptor found for: %s",
                    componentId.ToString<AZStd::string>().c_str());
                continue;
            }

            if (ShouldAddSystemComponent(componentDescriptor))
            {
                // add component if it's not already present
                if (!systemEntity->FindComponent(componentId))
                {
                    systemEntity->AddComponent(componentDescriptor->CreateComponent());
                }
            }
        }

        //
        // Call legacy AddSystemComponents functions.
        // Issue warnings if they're still being used.
        //

        size_t componentCount = systemEntity->GetComponents().size();
        (void)componentCount; // Only used in Warnings, produces "Unused variable" warning in non-trace builds

        AddSystemComponents(systemEntity);

        AZ_Warning("Module", componentCount == systemEntity->GetComponents().size(),
            "Application implements deprecated function 'AddSystemComponents'. Use 'GetRequiredSystemComponents' instead.");
    }

    //=========================================================================
    // CalculateExecutablePath
    //=========================================================================
    void ComponentApplication::CalculateExecutablePath()
    {
        // Checks if exe directory has already been calculated
        if (strlen(m_exeDirectory) > 0)
        {
            return;
        }

#if defined(AZ_RESTRICTED_PLATFORM)
    #if defined(AZ_PLATFORM_XENIA)
        #include "Xenia/ComponentApplication_cpp_xenia.inl"
    #elif defined(AZ_PLATFORM_PROVO)
        #include "Provo/ComponentApplication_cpp_provo.inl"
    #endif
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif defined(AZ_PLATFORM_ANDROID)
        // On Android, all dlopen calls should be relative.
        azstrcpy(m_exeDirectory, AZ_ARRAY_SIZE(m_exeDirectory), "");
#else

        char exeDirectory[AZ_MAX_PATH_LEN];

        // Platform specific get exe path: http://stackoverflow.com/a/1024937
#if AZ_TRAIT_USE_GET_MODULE_FILE_NAME
        // https://msdn.microsoft.com/en-us/library/windows/desktop/ms683197(v=vs.85).aspx
        DWORD pathLen = GetModuleFileNameA(nullptr, exeDirectory, AZ_ARRAY_SIZE(exeDirectory));
#elif defined(AZ_PLATFORM_APPLE)
        // https://developer.apple.com/library/mac/documentation/Darwin/Reference/ManPages/man3/dyld.3.html
        uint32_t bufSize = AZ_ARRAY_SIZE(exeDirectory);
        _NSGetExecutablePath(exeDirectory, &bufSize);
        // _NSGetExecutablePath doesn't update bufSize
        size_t pathLen = strlen(exeDirectory);
#else
        // http://man7.org/linux/man-pages/man5/proc.5.html
        size_t pathLen = readlink("/proc/self/exe", exeDirectory, AZ_ARRAY_SIZE(exeDirectory));
#endif // MSVC

        char* exeDirEnd = exeDirectory + pathLen;

        AZStd::replace(exeDirectory, exeDirEnd, '\\', '/');

        // exeDirectory currently contains full path to EXE.
        // Modify to end the string after the last '/'
        char* finalSlash = strrchr(exeDirectory, '/');

        // If no slashes found, path invalid.
        if (finalSlash)
        {
            *(finalSlash + 1) = '\0';
        }
        else
        {
            AZ_Error("ComponentApplication", false, "Failed to process exe directory. Path given by OS: %s", exeDirectory);
        }

        azstrcpy(m_exeDirectory, AZ_ARRAY_SIZE(m_exeDirectory), exeDirectory);

        CalculateBinFolder();

#endif
    }
    /// Calculates the Bin folder name where the application is running from (off of the engine root folder)
    void ComponentApplication::CalculateBinFolder()
    {
        azstrcpy(m_binFolder, AZ_ARRAY_SIZE(m_binFolder), "");

#if !defined(AZ_RESTRICTED_PLATFORM) && !defined(AZ_PLATFORM_ANDROID) && !defined(AZ_PLATFORM_APPLE_TV) && !defined(AZ_PLATFORM_APPLE_IOS)
        if (strlen(m_exeDirectory) > 0)
        {
            bool engineMarkerFound = false;
            // Prepare a working mutable path initialized with the current exe path
            char workingExeDirectory[AZ_MAX_PATH_LEN];
            azstrcpy(workingExeDirectory, AZ_ARRAY_SIZE(workingExeDirectory), m_exeDirectory);

            const char* lastCheckFolderName = "";

            // Calculate the bin folder name by walking up the path folders until we find the 'engine.json' file marker
            size_t checkFolderIndex = strlen(workingExeDirectory);
            if (checkFolderIndex > 0)
            {
                checkFolderIndex--; // Skip the right-most trailing slash since exeDirectory represents a folder
                while (true)
                {
                    // Eat up all trailing slashes
                    while ((checkFolderIndex > 0) && workingExeDirectory[checkFolderIndex] == '/')
                    {
                        workingExeDirectory[checkFolderIndex] = '\0';
                        checkFolderIndex--;
                    }
                    // Check if the current index is the drive letter separater
                    if (workingExeDirectory[checkFolderIndex] == ':')
                    {
                        // If we reached a drive letter separator, then use the last check folder 
                        // name as the bin folder since we cant go any higher
                        break;
                    }
                    else
                    {

                        // Find the next path separator
                        while ((checkFolderIndex > 0) && workingExeDirectory[checkFolderIndex] != '/')
                        {
                            checkFolderIndex--;
                        }
                    }
                    // Split the path and folder and test for engine.json
                    if (checkFolderIndex > 0)
                    {
                        lastCheckFolderName = &workingExeDirectory[checkFolderIndex + 1];
                        workingExeDirectory[checkFolderIndex] = '\0';
                        if (CheckPathForEngineMarker(workingExeDirectory))
                        {
                            // engine.json was found at the folder, break now to register the lastCheckFolderName as the bin folder
                            engineMarkerFound = true;

                            // Set the bin folder name only if the engine marker was found
                            azstrcpy(m_binFolder, AZ_ARRAY_SIZE(m_binFolder), lastCheckFolderName);

                            break;
                        }
                        checkFolderIndex--;
                    }
                    else
                    {
                        // We've reached the beginning, break out of the loop
                        break;
                    }
                }
                AZ_Warning("ComponentApplication", engineMarkerFound, "Unable to determine Bin folder. Cannot locate engine marker file 'engine.json'");
            }
        }
#endif // !defined(AZ_RESTRICTED_PLATFORM) && !defined(AZ_PLATFORM_ANDROID) && !defined(AZ_PLATFORM_APPLE_TV) && !defined(AZ_PLATFORM_APPLE_IOS)
    }

    //=========================================================================
    // CheckEngineMarkerFile
    //=========================================================================
    bool ComponentApplication::CheckPathForEngineMarker(const char* fullPath) const
    {
        static const char* engineMarkerFileName = "engine.json";
        char engineMarkerFullPathToCheck[AZ_MAX_PATH_LEN] = "";

        azstrcpy(engineMarkerFullPathToCheck, AZ_ARRAY_SIZE(engineMarkerFullPathToCheck), fullPath);
        azstrcat(engineMarkerFullPathToCheck, AZ_ARRAY_SIZE(engineMarkerFullPathToCheck), "/");
        azstrcat(engineMarkerFullPathToCheck, AZ_ARRAY_SIZE(engineMarkerFullPathToCheck), engineMarkerFileName);

        return AZ::IO::SystemFile::Exists(engineMarkerFullPathToCheck);
    }

    //=========================================================================
    // GetFullPathForDescriptor
    //=========================================================================
    AZ::OSString ComponentApplication::GetFullPathForDescriptor(const char* descriptorPath)
    {
        AZ::OSString fullApplicationDescriptorPath = descriptorPath;

        // We don't want to do lowercase here because it will breaks case-sensitive
        // platforms when descriptorPath is already a full path, and descriptorPath
        // has already been lower-cased in GameApplication::GetGameDescriptorPath.
        NormalizePath(fullApplicationDescriptorPath.begin(), fullApplicationDescriptorPath.end(), false);

        bool isFullPath = false;

#if AZ_TRAIT_OS_USE_WINDOWS_FILE_PATHS
        if (nullptr != strstr(fullApplicationDescriptorPath.c_str(), ":"))
        {
            isFullPath = true;
        }
#else
        isFullPath = fullApplicationDescriptorPath[0] == '/';
#endif

        if (!isFullPath)
        {
            fullApplicationDescriptorPath.insert(0, GetAppRoot());
        }

        return fullApplicationDescriptorPath;
    }

    void ComponentApplication::ResolveModulePath(AZ::OSString& modulePath)
    {
        modulePath = m_exeDirectory + modulePath;
    }

    //=========================================================================
    // GetFrameTime
    // [1/22/2016]
    //=========================================================================
    float ComponentApplication::GetTickDeltaTime()
    {
        return m_deltaTime;
    }

    //=========================================================================
    // GetTime
    // [1/22/2016]
    //=========================================================================
    ScriptTimePoint ComponentApplication::GetTimeAtCurrentTick()
    {
        return ScriptTimePoint(m_currentTime);
    }

    //=========================================================================
    // Reflect
    //=========================================================================
    void ComponentApplication::Reflect(ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
        if (serializeContext)
        {
            // Call deprecated ReflectSerialize() function.
            // Issue a warning if any classes reflected as a result of this call.
#ifdef AZ_ENABLE_TRACING
            int serializedClassCount = 0;
            auto countClassesFn = [&serializedClassCount](const SerializeContext::ClassData*, const Uuid&)
            {
                ++serializedClassCount;
                return true;
            };

            serializeContext->EnumerateAll(countClassesFn);
#endif // AZ_ENABLE_TRACING

            ReflectSerialize();

#ifdef AZ_ENABLE_TRACING
            int previousClassCount = serializedClassCount;
            serializedClassCount = 0;
            serializeContext->EnumerateAll(countClassesFn);
            AZ_Warning("Application", serializedClassCount == previousClassCount,
                "Classes reflected via deprecated ComponentApplication::ReflectSerialize() function, use ComponentApplication::Reflect() instead.");
#endif // AZ_ENABLE_TRACING
        }

        // reflect default entity
        Entity::Reflect(context);
        // reflect module manager
        ModuleManager::Reflect(context);
        // reflect descriptor
        Descriptor::Reflect(context, this);
        // reflect vertex container
        VertexContainerReflect(context);
        // reflect spline and associated data
        SplineReflect(context);
        // reflect polygon prism
        PolygonPrismReflect(context);
    }

} // namespace AZ

#endif // #ifndef AZ_UNITY_BUILD
