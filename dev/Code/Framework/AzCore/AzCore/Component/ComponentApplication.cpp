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

#include <AzCore/Casting/numeric_cast.h>

#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Component/TickBus.h>

#include <AzCore/Memory/AllocationRecords.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/ObjectStream.h>

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/RTTI/AzStdReflectionComponent.h>

#include <AzCore/Module/Module.h>

#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/SystemFile.h>

#include <AzCore/Driller/Driller.h>
#include <AzCore/Memory/MemoryDriller.h>
#include <AzCore/Debug/TraceMessagesDriller.h>
#include <AzCore/Debug/ProfilerDriller.h>
#include <AzCore/Debug/EventTraceDriller.h>

#include <AzCore/Asset/AssetManagerComponent.h>
#include <AzCore/UserSettings/UserSettingsComponent.h>
#include <AzCore/Memory/MemoryComponent.h>
#include <AzCore/Debug/FrameProfilerComponent.h>
#include <AzCore/IO/StreamerComponent.h>
#include <AzCore/Jobs/JobManagerComponent.h>
#include <AzCore/Serialization/ObjectStreamComponent.h>
#include <AzCore/Script/ScriptSystemComponent.h>

#include <AzCore/Slice/SliceComponent.h>
#include <AzCore/Slice/SliceSystemComponent.h>

#include <AzCore/XML/rapidxml.h>

#if defined(AZCORE_ENABLE_MEMORY_TRACKING)
#include <AzCore/Debug/StackTracer.h>
#endif // defined(AZCORE_ENABLE_MEMORY_TRACKING)

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

        m_x360IsPhysicalMemory = false;
    }

    //=========================================================================
    // ReflectSerialize
    // [5/30/2012]
    //=========================================================================
    void ComponentApplication::Descriptor::DynamicModuleDescriptor::ReflectSerialize(SerializeContext* context)
    {
        context->Class<DynamicModuleDescriptor>()->
            Field("dynamicLibraryPath", &DynamicModuleDescriptor::m_dynamicLibraryPath);

        EditContext* ec = context->GetEditContext();
        if (ec)
        {
            ec->Class<DynamicModuleDescriptor>(
                "Dynamic Module descriptor", "Describes a dynamic module (DLL) used by the application")
                ->DataElement(Edit::UIHandlers::Default, &DynamicModuleDescriptor::m_dynamicLibraryPath, "Dynamic library path", "Path to DLL.")
                ;
        }
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
    void  ComponentApplication::Descriptor::ReflectSerialize(SerializeContext* context, ComponentApplication* app)
    {
        DynamicModuleDescriptor::ReflectSerialize(context);

        context->Class<Descriptor>(&app->GetDescriptor())
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
            ->Field("x360PhysicalMemory", &Descriptor::m_x360IsPhysicalMemory)
            ;

        if (EditContext* ec = context->GetEditContext())
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
                ->DataElement(Edit::UIHandlers::CheckBox, &Descriptor::m_x360IsPhysicalMemory, "Physical memory", "Used only on X360 to indicate is we want to allocate physical memory")
                ;
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
    // ModuleData
    //=========================================================================
    ComponentApplication::ModuleData::ModuleData(ModuleData&& rhs)
        : m_dynamicHandle(AZStd::move(rhs.m_dynamicHandle))
    {
        m_module = rhs.m_module;
        rhs.m_module = nullptr;
    }
    
    static_assert(!AZStd::has_trivial_copy<ComponentApplication::ModuleData>::value, "Compiler believes ModuleData is trivially copyable, despite having non-trivially copyable members.\n");

    //=========================================================================
    // DestroyModuleClass
    //=========================================================================
    void ComponentApplication::ModuleData::DestroyModuleClass()
    {
        // If the AZ::Module came from a DLL, destroy it
        // using the DLL's \ref DestroyModuleClassFunction.
        if (m_module && m_dynamicHandle)
        {
            auto destroyFunc = m_dynamicHandle->GetFunction<DestroyModuleClassFunction>(DestroyModuleClassFunctionName);
            AZ_Assert(destroyFunc, "Unable to locate '%s' entry point in module at \"%s\".",
                DestroyModuleClassFunctionName, m_dynamicHandle->GetFilename().c_str());
            if (destroyFunc)
            {
                destroyFunc(m_module);
                m_module = nullptr;
            }
        }

        // If the AZ::Module came from a static LIB, delete it directly.
        if (m_module)
        {
            delete m_module;
            m_module = nullptr;
        }
    }

    //=========================================================================
    // GetDebugName
    //=========================================================================
    const char* ComponentApplication::ModuleData::GetDebugName() const
    {
        // If module is from DLL, return DLL name.
        if (m_dynamicHandle)
        {
            const char* filepath = m_dynamicHandle->GetFilename().c_str();
            const char* lastSlash = strrchr(filepath, '/');
            return lastSlash ? lastSlash + 1 : filepath;
        }
        // If Module has its own RTTI info, return that name
        else if (m_module && !azrtti_istypeof<Module>(m_module))
        {
            return m_module->RTTI_GetTypeName();
        }
        else
        {
            return "module";
        }
    }

    //=========================================================================
    // ~ModuleData
    //=========================================================================
    ComponentApplication::ModuleData::~ModuleData()
    {
        DestroyModuleClass();

        // If dynamic, unload DLL and destroy handle.
        if (m_dynamicHandle)
        {
            m_dynamicHandle->Unload();
            m_dynamicHandle.reset();
        }
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
        m_memoryBlock = nullptr;
        m_osAllocator = nullptr;
        m_serializeContext = nullptr;
        m_behaviorContext = nullptr;
        m_currentTime = AZStd::chrono::system_clock::time_point::max();
        m_drillerManager = nullptr;

        m_deltaTime = 0.f;
        m_exeDirectory[0] = '\0';

        TickBus::AllowFunctionQueuing(true);
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

        // Create temporary system allocator
        bool isExistingSystemAllocator = AZ::AllocatorInstance<AZ::SystemAllocator>::IsReady();

        if (!isExistingSystemAllocator)
        {
            enum
            {
                TemporaryBlockSize = 10 *  1024 * 1024
            };

            desc.m_heap.m_numMemoryBlocks = 1;
            desc.m_heap.m_memoryBlocksByteSize[0] = TemporaryBlockSize;
            desc.m_heap.m_memoryBlocks[0] = m_osAllocator->Allocate(desc.m_heap.m_memoryBlocksByteSize[0], desc.m_heap.m_memoryBlockAlignment);
            AZ::AllocatorInstance<AZ::SystemAllocator>::Create(desc);
        }

        // Get the system allocator configuration
        {
            SerializeContext sc;
            Descriptor::ReflectSerialize(&sc, this);
            ObjectStream::LoadBlocking(&stream, sc, nullptr, ObjectStream::FilterDescriptor(0, ObjectStream::FILTERFLAG_IGNORE_UNKNOWN_CLASSES));
        }

        // Destroy the temp system allocator
        if (!isExistingSystemAllocator)
        {
            AZ::AllocatorInstance<AZ::SystemAllocator>::Destroy();
            m_osAllocator->DeAllocate(desc.m_heap.m_memoryBlocks[0], desc.m_heap.m_memoryBlocksByteSize[0], desc.m_heap.m_memoryBlockAlignment);
        }
        else
        {
            AZ_Assert(m_descriptor.m_useExistingAllocator,
                "%s has useExistingAllocator set to false, but an existing system allocator was found! If you will create the system allocator yourself then set useExistingAllocator to true!",
                applicationDescriptorFile);
        }

        CreateCommon();

        // Create system entity. Reset the descriptor in preparation for reserializing it.
        m_descriptor = Descriptor();
        stream.Seek(0, IO::GenericStream::ST_SEEK_BEGIN);
        AZ::Entity* systemEntity = nullptr;
        ObjectStream::LoadBlocking(&stream, *m_serializeContext, AZStd::bind(&ComponentApplication::OnEntityLoaded, this, AZStd::placeholders::_1, AZStd::placeholders::_2, AZStd::placeholders::_3, &systemEntity));
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
        CreateDrillers();

        CreateSystemAllocator();

        RegisterCoreComponents();

        CreateSerializeContext();
        ReflectSerialize();

        CreateBehaviorContext();

        ComponentApplicationBus::Handler::BusConnect();

        m_currentTime = AZStd::chrono::system_clock::now();
        TickRequestBus::Handler::BusConnect();

#if defined(AZCORE_ENABLE_MEMORY_TRACKING)
        // Prior to loading more modules, we make sure SymbolStorage
        // is listening for the loads so it can keep track of which
        // modules we may eventually need symbols for
        Debug::SymbolStorage::RegisterModuleListeners();
#endif // defined(AZCORE_ENABLE_MEMORY_TRACKING)


        // Called after ComponentApplicationBus::Handler::BusConnect(),
        // so that modules may communicate with the component application.
        InitStaticModules();
        InitDynamicModules();
    }

    //=========================================================================
    // Destroy
    // [5/30/2012]
    //=========================================================================
    void ComponentApplication::Destroy()
    {
        // Finish all queued work
        TickBus::ExecuteQueuedEvents();
        TickBus::AllowFunctionQueuing(false);

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

        // deactivate all system components
        if (systemEntity)
        {
            if (systemEntity->GetState() == Entity::ES_ACTIVE)
            {
                systemEntity->Deactivate();
            }

            delete systemEntity;
        }

        m_entities.clear();
        m_entities.rehash(0); // force free all memory

#ifndef AZ_BEHAVIOR_CONTEXT_TODO_FIX
        // Because of OnDemandReflection, this must be destroyed before modules are unloaded.
        DestroyBehaviorContext();
#endif

        // Uninit and unload any dynamic modules.
        ShutdownAllModules();

#ifdef AZ_BEHAVIOR_CONTEXT_TODO_FIX
        DestroyBehaviorContext();
#endif
        DestroySerializeContext();

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

#if defined (AZ_PLATFORM_X360)
            if (m_descriptor.m_x360IsPhysicalMemory)
            {
                AZ_Assert(m_memoryBlock, "We don't have a valid memory block allocated!");
                // Redacted
            }
            else
#endif
            {
                if (m_memoryBlock != nullptr)
                {
                    m_osAllocator->DeAllocate(m_memoryBlock);
                }
            }
            m_memoryBlock = nullptr;
            m_isSystemAllocatorOwner = false;
        }

        

        if (m_isOSAllocatorOwner)
        {
            AZ::AllocatorInstance<AZ::OSAllocator>::Destroy();
            m_isOSAllocatorOwner = false;
        }

        m_osAllocator = nullptr;

        m_isStarted = false;

#if defined(AZCORE_ENABLE_MEMORY_TRACKING)
        // Unregister module listeners after allocators are destroyed
        // so that symbol/stack trace information is available at shutdown
        Debug::SymbolStorage::UnregisterModuleListeners();
#endif // defined(AZCORE_ENABLE_MEMORY_TRACKING)
    }

    //=========================================================================
    // CreateSystemAllocator
    // [5/30/2012]
    //=========================================================================
    void ComponentApplication::CreateSystemAllocator()
    {
        if (m_descriptor.m_useExistingAllocator)
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
            desc.m_heap.m_numMemoryBlocks = 1;
            if (m_descriptor.m_grabAllMemory)
            {
                // grab all available memory
                AZ::u64 availableOS = AZ_CORE_MAX_ALLOCATOR_SIZE;
                AZ::u64 reservedOS = m_descriptor.m_reservedOS;
                AZ::u64 reservedDbg = m_descriptor.m_reservedDebug;
#if defined (AZ_PLATFORM_X360)
                // Redacted
#else
                AZ_Warning("Memory", false, "This platform is not supported for grabAllMemory flag! Provide a valid allocation size and set the m_grabAllMemory flag to false! Using default max memory size %llu!", availableOS);
#endif
                AZ_Assert(availableOS > 0, "OS doesn't have any memory available!");
                // compute total memory to grab
                desc.m_heap.m_memoryBlocksByteSize[0] = static_cast<size_t>(availableOS - reservedOS - reservedDbg);
                // memory block MUST be a multiple of pages
                desc.m_heap.m_memoryBlocksByteSize[0] = AZ::SizeAlignDown(desc.m_heap.m_memoryBlocksByteSize[0], m_descriptor.m_pageSize);
            }
            else
            {
                desc.m_heap.m_memoryBlocksByteSize[0] = static_cast<size_t>(m_descriptor.m_memoryBlocksByteSize);
            }

#if defined (AZ_PLATFORM_X360)
            // Redacted
#endif
            {
                if (desc.m_heap.m_memoryBlocksByteSize[0] > 0) // 0 means one demand memory which we support
                {
                    m_memoryBlock = m_osAllocator->Allocate(desc.m_heap.m_memoryBlocksByteSize[0], m_descriptor.m_memoryBlockAlignment);
                }
            }

            desc.m_heap.m_memoryBlocks[0] = m_memoryBlock;
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
        // reflect the factory if possible
        descriptor->Reflect(m_serializeContext);

        // reflect into behaviorContext
        if (m_behaviorContext)
        {
            descriptor->Reflect(m_behaviorContext);
        }
    }

    //=========================================================================
    // RegisterComponentDescriptor
    //=========================================================================
    void ComponentApplication::UnregisterComponentDescriptor(const ComponentDescriptor* descriptor)
    {
        if (m_behaviorContext)
        {
            m_behaviorContext->EnableRemoveReflection();
            descriptor->Reflect(m_behaviorContext);
            m_behaviorContext->DisableRemoveReflection();
        }

        if (m_serializeContext)
        {
            // \todo: Unreflect from script context: https://issues.labcollab.net/browse/LMBR-17558.

            // Remove all reflected data from this descriptor
            m_serializeContext->EnableRemoveReflection();
            descriptor->Reflect(m_serializeContext);
            m_serializeContext->DisableRemoveReflection();
        }
    }

    //=========================================================================
    // AddEntity
    // [5/30/2012]
    //=========================================================================
    bool ComponentApplication::AddEntity(Entity* entity)
    {
        EBUS_EVENT(ComponentApplicationEventBus, OnEntityAdded, entity);

        return m_entities.insert(AZStd::make_pair(entity->GetId(), entity)).second;
    }

    //=========================================================================
    // RemoveEntity
    // [5/31/2012]
    //=========================================================================
    bool ComponentApplication::RemoveEntity(Entity* entity)
    {
        EBUS_EVENT(ComponentApplicationEventBus, OnEntityRemoved, entity->GetId());

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
    // CreateSerializeContext
    // [5/30/2012]
    //=========================================================================
    void ComponentApplication::CreateSerializeContext()
    {
        if (m_serializeContext == nullptr)
        {
            m_serializeContext = aznew SerializeContext;
        }
    }

    //=========================================================================
    // DestroySerializeContext
    // [11/9/2012]
    //=========================================================================
    void ComponentApplication::DestroySerializeContext()
    {
        delete m_serializeContext;
        m_serializeContext = nullptr;
    }

    //=========================================================================
    // CreateBehaviorContext
    //=========================================================================
    void ComponentApplication::CreateBehaviorContext()
    {
        if (m_behaviorContext == nullptr)
        {
            m_behaviorContext = aznew BehaviorContext;

            // reflect all registered classes
            EBUS_EVENT(ComponentDescriptorBus, Reflect, m_behaviorContext);
        }
    }

    //=========================================================================
    // DestroyBehaviorContext
    //=========================================================================
    void ComponentApplication::DestroyBehaviorContext()
    {
        delete m_behaviorContext;
        m_behaviorContext = nullptr;
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

        AZ_Assert(m_serializeContext, "ComponentApplication::m_serializeContext is NULL!");
        ObjectStream* objStream = ObjectStream::Create(fileStream.get(), *m_serializeContext, ObjectStream::ST_XML);
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
    // OnEntityLoaded
    // [5/30/2012]
    //=========================================================================
    void ComponentApplication::OnEntityLoaded(void* classPtr, const AZ::Uuid& classId, const AZ::SerializeContext* sc, AZ::Entity** systemEntity)
    {
        Entity* entity = sc->Cast<Entity*>(classPtr, classId);
        if (entity)
        {
            if (entity->GetId() == AZ::SystemEntityId && systemEntity != nullptr)
            {
                *systemEntity = entity;
            }
        }
        else if (!sc->Cast<Descriptor*>(classPtr, classId))
        {
            char idStr[48];
            classId.ToString(idStr, AZ_ARRAY_SIZE(idStr));
            AZ_Error("ComponentApplication", false, "Unknown class type %p %s", classPtr, idStr);
        }
    }

    //=========================================================================
    // RegisterCoreComponents
    // [5/30/2012]
    //=========================================================================
    void ComponentApplication::RegisterCoreComponents()
    {
        RegisterComponentDescriptor(AzStdReflectionComponent::CreateDescriptor());
        RegisterComponentDescriptor(MemoryComponent::CreateDescriptor());
        RegisterComponentDescriptor(StreamerComponent::CreateDescriptor());
        RegisterComponentDescriptor(JobManagerComponent::CreateDescriptor());
        RegisterComponentDescriptor(AssetManagerComponent::CreateDescriptor());
        RegisterComponentDescriptor(ObjectStreamComponent::CreateDescriptor());
        RegisterComponentDescriptor(UserSettingsComponent::CreateDescriptor());
        RegisterComponentDescriptor(Debug::FrameProfilerComponent::CreateDescriptor());
        RegisterComponentDescriptor(SliceComponent::CreateDescriptor());
        RegisterComponentDescriptor(SliceSystemComponent::CreateDescriptor());

#if !defined(AZCORE_EXCLUDE_LUA)
        RegisterComponentDescriptor(ScriptSystemComponent::CreateDescriptor());
#endif // #if !defined(AZCORE_EXCLUDE_LUA)

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

    //=========================================================================
    // AddRequiredSystemComponents
    //=========================================================================
    void ComponentApplication::AddRequiredSystemComponents(AZ::Entity* systemEntity)
    {
        //
        // Gather required system components from all modules and the application.
        //

        // contains pair of component type-id, and name-of-source
        AZStd::vector<AZStd::pair<Uuid, AZStd::string> > requiredComponents;

        for (const Uuid& componentId : GetRequiredSystemComponents())
        {
            requiredComponents.emplace_back(componentId, "application");
        }

        for (const auto& moduleData : m_modules)
        {
            if (moduleData->m_module)
            {
                AZStd::string moduleName = moduleData->GetDebugName();
                for (const Uuid& componentId : moduleData->m_module->GetRequiredSystemComponents())
                {
                    requiredComponents.emplace_back(componentId, moduleName);
                }
            }
        }

        //
        // Add required components to system entity.
        // Note that the system entity may already contain components
        // that were read in from the application-descriptor-file.
        //

        for (const auto& pair : requiredComponents)
        {
            const Uuid& componentTypeId = pair.first;
            const AZStd::string& componentSourceName = pair.second;
            (void)componentSourceName;

            ComponentDescriptor* componentDescriptor = nullptr;
            EBUS_EVENT_ID_RESULT(componentDescriptor, componentTypeId, ComponentDescriptorBus, GetDescriptor);
            if (!componentDescriptor)
            {
                AZ_Error("Module", false, "Failed to add system component required by '%s'. No component descriptor found for: %s",
                    componentSourceName.c_str(),
                    componentTypeId.ToString<AZStd::string>().c_str());
                continue;
            }

            // add component if it's not already present
            if (!systemEntity->FindComponent(componentTypeId))
            {
                systemEntity->AddComponent(componentDescriptor->CreateComponent());
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
        componentCount = systemEntity->GetComponents().size();

        for (auto& moduleData : m_modules)
        {
            if (moduleData->m_module)
            {
                moduleData->m_module->AddSystemComponents(systemEntity);

                AZ_Warning("Module", componentCount == systemEntity->GetComponents().size(),
                    "Module '%s' implements deprecated function 'AddSystemComponents'. Use 'GetRequiredSystemComponents' instead.",
                    moduleData->GetDebugName());
                componentCount = systemEntity->GetComponents().size();
            }
        }
    }

    //=========================================================================
    // Preprocess a dynamic module name.  This is where we will apply any legacy/upgrade
    // of modules if necessary (and provide a warning to update the configuration)
    //=========================================================================
    static AZ::OSString PreProcessModule(const AZ::OSString& moduleName)
    {
        // This is the list of modules that have been deprecated as of version 1.10
        // Update this list accordingly.  
        static const char* legacyModules[] =        { "LmbrCentral", 
                                                      "LmbrCentralEditor" };
        // List of modules (Gems) that represents the upgrade from the legacyModules 
        static const char* legacyUpgradeModules[] = { "Gem.LmbrCentral.ff06785f7145416b9d46fde39098cb0c.v0.1.0",
                                                      "Gem.LmbrCentral.Editor.ff06785f7145416b9d46fde39098cb0c.v0.1.0" };

        // Process out just the module name from a potential path or subpath
        OSString preprocessedName;
        OSString prefix = "";

        auto pathSeparatorIndex = moduleName.find_last_of("/\\");
        if (pathSeparatorIndex != moduleName.npos)
        {
            preprocessedName = moduleName.substr(pathSeparatorIndex + 1);
            prefix = moduleName.substr(0, pathSeparatorIndex + 1);
        }
        else
        {
            preprocessedName = moduleName;
        }

        // Update the module name if it matches an (upgradable) legacy module
        static size_t legacyModuleCount = AZ_ARRAY_SIZE(legacyModules);
        AZ_Assert(legacyModuleCount == AZ_ARRAY_SIZE(legacyUpgradeModules), "Legacy Module update list is not in sync.");

        for (size_t moduleIndex = 0; moduleIndex < legacyModuleCount; moduleIndex++)
        {
            const char* legacyModule = legacyModules[moduleIndex];
            if (preprocessedName.compare(legacyModule) == 0)
            {
                preprocessedName = legacyUpgradeModules[moduleIndex];
                break;
            }
        }

        return AZ::OSString::format("%s%s", prefix.c_str(), preprocessedName.c_str());
    }

    //=========================================================================
    // InitDynamicModules
    //=========================================================================
    void ComponentApplication::InitDynamicModules()
    {
        if (m_startupParameters.m_loadDynamicModules)
        {
            // Load DLLs specified in the application descriptor
            for (const Descriptor::DynamicModuleDescriptor& moduleDescriptor : m_descriptor.m_modules)
            {
                LoadModuleOutcome loadOutcome = LoadDynamicModule(moduleDescriptor);
                AZ_Error("Module", loadOutcome.IsSuccess(), loadOutcome.GetError().c_str());
            }
        }
    }

    void ComponentApplication::ResolveModulePath(AZ::OSString& modulePath)
    {
        modulePath = m_exeDirectory + modulePath;
    }

    //=========================================================================
    // LoadDynamicModule
    //=========================================================================
    ComponentApplication::LoadModuleOutcome ComponentApplication::LoadDynamicModule(const Descriptor::DynamicModuleDescriptor& moduleDescriptor)
    {
        // allow overriding Applications to mutate this path before we attempt to load it.
        AZ::OSString resolvedModulePath(PreProcessModule(moduleDescriptor.m_dynamicLibraryPath));
        ResolveModulePath(resolvedModulePath);

        // moduleData will properly tear itself down if we bail out of this function early.
        // (ex: will unload DLL if it had been loaded)
        auto moduleData = AZStd::unique_ptr<ModuleData>(new ModuleData);

        // Create handle
        moduleData->m_dynamicHandle = DynamicModuleHandle::Create(resolvedModulePath.c_str());
        if (!moduleData->m_dynamicHandle)
        {
            return AZ::Failure(AZStd::string::format("Failed to create AZ::DynamicModuleHandle at path \"%s\".",
                resolvedModulePath.c_str()));
        }

        // Load DLL from disk
        if (!moduleData->m_dynamicHandle->Load(true))
        {
            return AZ::Failure(AZStd::string::format("Failed to load dynamic library at path \"%s\".\nThis can occur if modules have been deleted,\nor if modules have been added to your project configuration but without building the project to create those modules.",
                moduleData->m_dynamicHandle->GetFilename().c_str()));
        }

        // Find function that creates AZ::Module class.
        // It's acceptable for a library not to have this function.
        auto createModuleFunc = moduleData->m_dynamicHandle->GetFunction<CreateModuleClassFunction>(CreateModuleClassFunctionName);
        if (createModuleFunc)
        {
            // Create AZ::Module class
            moduleData->m_module = createModuleFunc();
            if (!moduleData->m_module)
            {
                // It's an error if the library has a CreateModuleClass() function that returns nothing.
                return AZ::Failure(AZStd::string::format("'%s' entry point in module \"%s\" failed to create AZ::Module.",
                    CreateModuleClassFunctionName,
                    moduleData->m_dynamicHandle->GetFilename().c_str()));
            }

            InitModule(*moduleData->m_module);
        }

        // Success! Move ModuleData into a permanent location and return a pointer
        m_modules.emplace_back(AZStd::move(moduleData));
        return AZ::Success(m_modules.back().get());
    }

    //=========================================================================
    // IsModuleLoaded
    //=========================================================================
    bool ComponentApplication::IsModuleLoaded(const Descriptor::DynamicModuleDescriptor& moduleDescriptor)
    {
        AZ::OSString resolvedModulePath(moduleDescriptor.m_dynamicLibraryPath);
        ResolveModulePath(resolvedModulePath);

        for (const auto& moduleData : m_modules)
        {
            if (moduleData->m_dynamicHandle)
            {
                if (azstricmp(moduleData->m_dynamicHandle->GetFilename().c_str(), resolvedModulePath.c_str()) == 0)
                {
                    return true;
                }
            }
        }
        return false;
    }

    //=========================================================================
    // InitModule
    //=========================================================================
    void ComponentApplication::InitModule(AZ::Module& module)
    {
        module.RegisterComponentDescriptors();
    }

    //=========================================================================
    // InitStaticModules
    //=========================================================================
    void ComponentApplication::InitStaticModules()
    {
        // Create static AZ::Modules
        if (m_startupParameters.m_createStaticModulesCallback)
        {
            AZStd::vector<AZ::Module*> staticModules;
            m_startupParameters.m_createStaticModulesCallback(staticModules);

            for (Module* staticModule : staticModules)
            {
                AZ_Assert(staticModule, "AZCreateStaticModules() returned an invalid AZ::Module.");
                if (staticModule)
                {
                    InitModule(*staticModule);

                    // put in m_modules
                    m_modules.emplace_back(new ModuleData);
                    m_modules.back()->m_module = staticModule;
                }
            }
        }
    }

    //=========================================================================
    // ShutdownAllModules
    //=========================================================================
    void ComponentApplication::ShutdownAllModules()
    {
        // Shutdown in reverse order of initialization, just in case the order matters.
        while (!m_modules.empty())
        {
            m_modules.pop_back(); // ~ModuleData() handles shutdown logic
        }
        m_modules.set_capacity(0);
    }

    //=========================================================================
    // ReloadModule
    //=========================================================================
    void ComponentApplication::ReloadModule(const char* moduleFullPath)
    {
        (void)moduleFullPath;
        AZ_Assert(false, "ReloadModule is not yet implemented");
    }

    void ComponentApplication::EnumerateModules(EnumerateModulesCallback cb)
    {
        for (const auto& moduleData : m_modules)
        {
            if (!cb(moduleData->m_module, moduleData->m_dynamicHandle.get()))
            {
                break;
            }
        }
    }

    //=========================================================================
    // CalculateExecutablePath
    //=========================================================================
    void ComponentApplication::CalculateExecutablePath()
    {
#if   defined(AZ_PLATFORM_ANDROID)
        // On Android, all dlopen calls should be relative.
        azstrcpy(m_exeDirectory, AZ_ARRAY_SIZE(m_exeDirectory), "");
#else

        char exeDirectory[AZ_MAX_PATH_LEN];

        // Platform specific get exe path: http://stackoverflow.com/a/1024937
#if defined(AZ_COMPILER_MSVC)
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

        // Iterate until finding last '/'
        char* finalSlash = nullptr;
        for (char* c = exeDirectory; c < exeDirEnd; ++c)
        {
            if (*c == '/')
            {
                finalSlash = c;
            }
        }

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
#endif // AZ_PLATFORM_PS4
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

#if defined (AZ_PLATFORM_X360) || defined (AZ_PLATFORM_WINDOWS) || defined (AZ_PLATFORM_XBONE)
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
    // ReflectSerialize
    // [11/9/2012]
    //=========================================================================
    void ComponentApplication::ReflectSerialize()
    {
        // reflect default entity
        Entity::Reflect(m_serializeContext);
        // reflect default
        Descriptor::ReflectSerialize(m_serializeContext, this);

        // reflect all registered classes
        EBUS_EVENT(ComponentDescriptorBus, Reflect, m_serializeContext);
    }

} // namespace AZ

#endif // #ifndef AZ_UNITY_BUILD
