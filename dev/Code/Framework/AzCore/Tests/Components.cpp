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

#include "TestTypes.h"
#include "FileIOBaseTestTypes.h"

#include <AzCore/Math/Crc.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Component/EntityUtils.h>

#include <AzCore/IO/StreamerComponent.h>
#include <AzCore/Serialization/ObjectStream.h>

#include <AzCore/Memory/MemoryComponent.h>
#include <AzCore/UserSettings/UserSettingsComponent.h>
#include <AzCore/IO/SystemFile.h>

#include <AzCore/Debug/FrameProfilerBus.h>
#include <AzCore/Debug/FrameProfilerComponent.h>
#include <AzCore/Debug/TraceMessageBus.h>
#include <AzCore/Memory/AllocationRecords.h>

#include <AzCore/std/parallel/containers/concurrent_unordered_set.h>

using namespace AZ;
using namespace AZ::Debug;

#if defined(AZ_RESTRICTED_PLATFORM)
#include AZ_RESTRICTED_FILE(Components_cpp, AZ_RESTRICTED_PLATFORM)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif defined(AZ_PLATFORM_APPLE_IOS)
#   define AZ_ROOT_TEST_FOLDER  "/Documents/"
#elif defined(AZ_PLATFORM_APPLE_TV)
#   define AZ_ROOT_TEST_FOLDER "/Library/Caches/"
#elif defined(AZ_PLATFORM_LINUX) || defined(AZ_PLATFORM_APPLE_OSX)
#   define AZ_ROOT_TEST_FOLDER  "./"
#elif defined(AZ_PLATFORM_ANDROID)
#   define AZ_ROOT_TEST_FOLDER  "/sdcard/"
#else
#   define AZ_ROOT_TEST_FOLDER  ""
#endif

namespace // anonymous
{
#if defined(AZ_PLATFORM_APPLE_IOS) || defined(AZ_PLATFORM_APPLE_TV)
    AZStd::string GetTestFolderPath()
    {
        return AZStd::string(getenv("HOME")) + AZ_ROOT_TEST_FOLDER;
    }
    void MakePathFromTestFolder(char* buffer, int bufferLen, const char* fileName)
    {
        azsnprintf(buffer, bufferLen, "%s%s%s", getenv("HOME"), AZ_ROOT_TEST_FOLDER, fileName);
    }
#else
    AZStd::string GetTestFolderPath()
    {
        return AZ_ROOT_TEST_FOLDER;
    }
    void MakePathFromTestFolder(char* buffer, int bufferLen, const char* fileName)
    {
        azsnprintf(buffer, bufferLen, "%s%s", AZ_ROOT_TEST_FOLDER, fileName);
    }
#endif
} // anonymous namespace

// This test needs to be outside of a fixture, as it needs to bring up its own allocators
TEST(ComponentApplication, Test)
{
    ComponentApplication app;

    //////////////////////////////////////////////////////////////////////////
    // Create application environment code driven
    ComponentApplication::Descriptor appDesc;
    appDesc.m_memoryBlocksByteSize = 10 * 1024 * 1024;
    appDesc.m_recordingMode = AllocationRecords::RECORD_FULL;
    appDesc.m_stackRecordLevels = 20;
    Entity* systemEntity = app.Create(appDesc);

    systemEntity->CreateComponent<MemoryComponent>();
    systemEntity->CreateComponent<StreamerComponent>();
    systemEntity->CreateComponent("{CAE3A025-FAC9-4537-B39E-0A800A2326DF}"); // JobManager component
    systemEntity->CreateComponent("{D5A73BCC-0098-4d1e-8FE4-C86101E374AC}"); // AssetDatabase component
    systemEntity->CreateComponent("{22FC6380-C34F-4a59-86B4-21C0276BCEE3}"); // ObjectStream component

    systemEntity->Init();
    systemEntity->Activate();

    // store the app state for next time
    char bootstrapFile[AZ_MAX_PATH_LEN];
    MakePathFromTestFolder(bootstrapFile, AZ_MAX_PATH_LEN, "bootstrap.xml");
    UnitTest::TestFileIOBase fileIO;
    UnitTest::SetRestoreFileIOBaseRAII restoreFileIOScope(fileIO);
    bool writeSuccess = app.WriteApplicationDescriptor(bootstrapFile);
    AZ_TEST_ASSERT(writeSuccess);

    app.Destroy();
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // Create application environment data driven
    systemEntity = app.Create(bootstrapFile);
    systemEntity->Init();
    systemEntity->Activate();
    app.Destroy();
    //////////////////////////////////////////////////////////////////////////
}

namespace UnitTest
{
    class Components
        : public AllocatorsFixture
    {
    public:
        Components()
            : AllocatorsFixture(128, false)
        {}
    };

    //////////////////////////////////////////////////////////////////////////
    // Some component message bus, this is not really part of the component framework
    // but this is way components are suppose to communicate... using the EBus
    class SimpleComponentMessages
        : public AZ::EBusTraits
    {
    public:
        virtual ~SimpleComponentMessages() {}
        virtual void DoA(int a) = 0;
        virtual void DoB(int b) = 0;
    };
    typedef AZ::EBus<SimpleComponentMessages> SimpleComponentMessagesBus;
    //////////////////////////////////////////////////////////////////////////

    class SimpleComponent
        : public Component
        , public SimpleComponentMessagesBus::Handler
        , public TickBus::Handler
    {
    public:
        AZ_RTTI(SimpleComponent, "{6DFA17AF-014C-4624-B453-96E1F9807491}", Component)
        AZ_CLASS_ALLOCATOR(SimpleComponent, SystemAllocator, 0)

        SimpleComponent()
            : m_a(0)
            , m_b(0)
            , m_isInit(false)
            , m_isActivated(false)
        {
        }

        //////////////////////////////////////////////////////////////////////////
        // Component base
        void Init() override { m_isInit = true; m_isTicked = false; }
        void Activate() override
        {
            SimpleComponentMessagesBus::Handler::BusConnect();
            // This is a very tricky (but valid example)... here we use the TickBus... thread safe
            // event queue, to queue the connection to be executed from the main thread, just before tick.
            // By using this even though TickBus is executed in single thread mode (main thread) for
            // performance reasons, you can technically issue command from multiple thread.
            // This requires advanced knowledge of the EBus and it's NOT recommended as a schema for
            // generic functionality. You should just call TickBus::Handler::BusConnect(GetEntityId()); in place
            // make sure you are doing this from the main thread.
            EBUS_QUEUE_FUNCTION(TickBus, &TickBus::Handler::BusConnect, this);
            m_isActivated = true;
        }
        void Deactivate() override
        {
            SimpleComponentMessagesBus::Handler::BusDisconnect();
            TickBus::Handler::BusDisconnect();
            m_isActivated = false;
        }
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // SimpleComponentMessagesBus
        void DoA(int a) override { m_a = a; }
        void DoB(int b) override { m_b = b; }
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // TickBus
        void OnTick(float deltaTime, ScriptTimePoint time) override
        {
            m_isTicked = true;
            AZ_TEST_ASSERT(deltaTime >= 0);
            AZ_TEST_ASSERT(time.Get().time_since_epoch().count() != 0);
        }
        //////////////////////////////////////////////////////////////////////////

        int m_a;
        int m_b;
        bool m_isInit;
        bool m_isActivated;
        bool m_isTicked;
    };

    // Example how to implement custom desciptors
    class SimpleComponentDescriptor
        : public ComponentDescriptorHelper<SimpleComponent>
    {
    public:
        void Reflect(ReflectContext* /*reflection*/) const override
        {
        }
    };

    TEST_F(Components, SimpleTest)
    {
        SimpleComponentDescriptor descriptor;
        ComponentApplication componentApp;
        ComponentApplication::Descriptor desc;
        desc.m_useExistingAllocator = true;
        ComponentApplication::StartupParameters startupParams;
        startupParams.m_allocator = &AZ::AllocatorInstance<AZ::SystemAllocator>::Get();
        Entity* systemEntity = componentApp.Create(desc, startupParams);
        AZ_TEST_ASSERT(systemEntity);
        systemEntity->Init();

        Entity* entity = aznew Entity("My Entity");
        AZ_TEST_ASSERT(entity->GetState() == Entity::ES_CONSTRUCTED);

        // Make sure its possible to set the id of the entity before inited.
        AZ::EntityId newId = AZ::Entity::MakeId();
        entity->SetId(newId);
        AZ_TEST_ASSERT(entity->GetId() == newId);

        AZ_TEST_START_ASSERTTEST;
        entity->SetId(SystemEntityId); // this is disallowed.
        AZ_TEST_STOP_ASSERTTEST(1);

        // we can always create components directly when we have the factory
        // but it is intended to be used in generic way...
        SimpleComponent* comp1 = aznew SimpleComponent;
        AZ_TEST_ASSERT(comp1 != nullptr);
        AZ_TEST_ASSERT(comp1->GetEntity() == nullptr);
        AZ_TEST_ASSERT(comp1->GetId() == InvalidComponentId);

        bool result = entity->AddComponent(comp1);
        AZ_TEST_ASSERT(result);

                                                              // try to find it
        SimpleComponent* comp2 = entity->FindComponent<SimpleComponent>();
        AZ_TEST_ASSERT(comp1 == comp2);

        // init entity
        entity->Init();
        AZ_TEST_ASSERT(entity->GetState() == Entity::ES_INIT);
        AZ_TEST_ASSERT(comp1->m_isInit);
        AZ_TEST_ASSERT(comp1->GetEntity() == entity);
        AZ_TEST_ASSERT(comp1->GetId() != InvalidComponentId); // id is set only for attached components

        // Make sure its NOT possible to set the id of the entity after INIT
        newId = AZ::Entity::MakeId();
        AZ::EntityId oldID = entity->GetId();
        AZ_TEST_START_ASSERTTEST;
        entity->SetId(newId); // this should not work because its init.
        AZ_TEST_STOP_ASSERTTEST(1);
        AZ_TEST_ASSERT(entity->GetId() == oldID); // id should be unaffected.

                                                  // try to send a component message, since it's not active nobody should listen to it
        EBUS_EVENT(SimpleComponentMessagesBus, DoA, 1);
        AZ_TEST_ASSERT(comp1->m_a == 0); // it should still be 0

                                         // activate
        entity->Activate();
        AZ_TEST_ASSERT(entity->GetState() == Entity::ES_ACTIVE);
        AZ_TEST_ASSERT(comp1->m_isActivated);

        // now the component should be active responsive to message
        EBUS_EVENT(SimpleComponentMessagesBus, DoA, 1);
        AZ_TEST_ASSERT(comp1->m_a == 1);

        // Make sure its NOT possible to set the id of the entity after Activate
        newId = AZ::Entity::MakeId();
        AZ_TEST_START_ASSERTTEST;
        entity->SetId(newId); // this should not work because its init.
        AZ_TEST_STOP_ASSERTTEST(1);

        // test the tick events
        componentApp.Tick(); // first tick will set-up timers and have 0 delta time
        AZ_TEST_ASSERT(comp1->m_isTicked);
        componentApp.Tick(); // this will dispatch actual valid delta time

                             // make sure we can't remove components while active
        AZ_TEST_START_ASSERTTEST;
        AZ_TEST_ASSERT(entity->RemoveComponent(comp1) == false);
        AZ_TEST_STOP_ASSERTTEST(1);

            // make sure we can't add components while active
            {
                SimpleComponent anotherComp;
                AZ_TEST_START_ASSERTTEST;
                AZ_TEST_ASSERT(entity->AddComponent(&anotherComp) == false);
                AZ_TEST_STOP_ASSERTTEST(1);
            }

            AZ_TEST_START_ASSERTTEST;
            AZ_TEST_ASSERT(entity->CreateComponent<SimpleComponent>() == nullptr);
            AZ_TEST_STOP_ASSERTTEST(1);

            AZ_TEST_START_ASSERTTEST;
            AZ_TEST_ASSERT(entity->CreateComponent(azrtti_typeid<SimpleComponent>()) == nullptr);
            AZ_TEST_STOP_ASSERTTEST(1);

        // deactivate
        entity->Deactivate();
        AZ_TEST_ASSERT(entity->GetState() == Entity::ES_INIT);
        AZ_TEST_ASSERT(comp1->m_isActivated == false);

        // try to send a component message, since it's not active nobody should listen to it
        EBUS_EVENT(SimpleComponentMessagesBus, DoA, 2);
        AZ_TEST_ASSERT(comp1->m_a == 1);

        // make sure we can remove components
        AZ_TEST_ASSERT(entity->RemoveComponent(comp1));
        AZ_TEST_ASSERT(comp1->GetEntity() == nullptr);
        AZ_TEST_ASSERT(comp1->GetId() == InvalidComponentId);

        delete entity;
        descriptor.BusDisconnect(); // disconnect from the descriptor bus (so the app doesn't try to clean us up)
    }

    //////////////////////////////////////////////////////////////////////////
    // Component A
    class ComponentA
        : public Component
    {
    public:
        AZ_CLASS_ALLOCATOR(ComponentA, SystemAllocator, 0)
        AZ_RTTI(ComponentA, "{4E93E03A-0B71-4630-ACCA-C6BB78E6DEB9}", Component)

        void Activate() override {}
        void Deactivate() override {}
    };

    /// Custom descriptor... example
    class ComponentADescriptor
        : public ComponentDescriptorHelper<ComponentA>
    {
    public:
        AZ_CLASS_ALLOCATOR(ComponentADescriptor, SystemAllocator, 0);

        ComponentADescriptor()
            : m_isDependent(false)
        {
        }

        void GetProvidedServices(DependencyArrayType& provided, const Component* instance) const override
        {
            (void)instance;
            provided.push_back(AZ_CRC("ServiceA", 0x808b9021));
        }
        void GetDependentServices(DependencyArrayType& dependent, const Component* instance) const override
        {
            (void)instance;
            if (m_isDependent)
            {
                dependent.push_back(AZ_CRC("ServiceD", 0xf0e164ae));
            }
        }
        void Reflect(ReflectContext* /*reflection*/) const override {}

        bool    m_isDependent;
    };
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // Component B
    class ComponentB
        : public Component
    {
    public:
        AZ_COMPONENT(ComponentB, "{30B266B3-AFD6-4173-8BEB-39134A3167E3}")

        void Activate() override {}
        void Deactivate() override {}

        static void GetProvidedServices(ComponentDescriptor::DependencyArrayType& provided) { provided.push_back(AZ_CRC("ServiceB", 0x1982c19b)); }
        static void GetDependentServices(ComponentDescriptor::DependencyArrayType& dependent) { dependent.push_back(AZ_CRC("ServiceE", 0x87e65438)); }
        static void GetIncompatibleServices(ComponentDescriptor::DependencyArrayType& incompatible) { incompatible.push_back(AZ_CRC("ServiceF", 0x1eef0582)); }
        static void Reflect(ReflectContext* /*reflection*/)  {}
    };

    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////
    // Component C
    class ComponentC
        : public Component
    {
    public:
        AZ_COMPONENT(ComponentC, "{A24C5D97-641F-4A92-90BB-647213A9D054}");

        void Activate() override {}
        void Deactivate() override {}

        static void GetRequiredServices(ComponentDescriptor::DependencyArrayType& required) { required.push_back(AZ_CRC("ServiceB", 0x1982c19b)); }
        static void Reflect(ReflectContext* /*reflection*/) {}
    };

    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////
    // Component D
    class ComponentD
        : public Component
    {
    public:
        AZ_COMPONENT(ComponentD, "{90888AD7-9D15-4356-8B95-C233A2E3083C}");

        void Activate() override {}
        void Deactivate() override {}

        static void GetProvidedServices(ComponentDescriptor::DependencyArrayType& provided) { provided.push_back(AZ_CRC("ServiceD", 0xf0e164ae)); }
        static void Reflect(ReflectContext* /*reflection*/) {}
    };
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // Component E
    class ComponentE
        : public Component
    {
    public:
        AZ_COMPONENT(ComponentE, "{8D28A94A-9F70-4ADA-999E-D8A56A3048FB}", Component);

        void Activate() override {}
        void Deactivate() override {}

        static void GetDependentServices(ComponentDescriptor::DependencyArrayType& dependent) { dependent.push_back(AZ_CRC("ServiceD", 0xf0e164ae)); dependent.push_back(AZ_CRC("ServiceA", 0x808b9021)); }
        static void GetProvidedServices(ComponentDescriptor::DependencyArrayType& provided) { provided.push_back(AZ_CRC("ServiceE", 0x87e65438)); }
        static void Reflect(ReflectContext* /*reflection*/) {}
    };
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // Component F
    class ComponentF
        : public Component
    {
    public:
        AZ_COMPONENT(ComponentF, "{9A04F820-DFB6-42CF-9D1B-F970CEF1A02A}");

        void Activate() override {}
        void Deactivate() override {}

        static void GetIncompatibleServices(ComponentDescriptor::DependencyArrayType& incompatible) { incompatible.push_back(AZ_CRC("ServiceA", 0x808b9021)); }
        static void GetProvidedServices(ComponentDescriptor::DependencyArrayType& provided) { provided.push_back(AZ_CRC("ServiceF", 0x1eef0582)); }
        static void Reflect(ReflectContext* /*reflection*/) {}
    };
    //////////////////////////////////////////////////////////////////////////

    TEST_F(Components, Dependency)
    {
        ComponentADescriptor* descriptorComponentA = aznew ComponentADescriptor;
        ComponentB::DescriptorType* descriptorComponentB = aznew ComponentB::DescriptorType;
        ComponentC::DescriptorType* descriptorComponentC = aznew ComponentC::DescriptorType;
        ComponentD::DescriptorType* descriptorComponentD = aznew ComponentD::DescriptorType;
        ComponentE::DescriptorType* descriptorComponentE = aznew ComponentE::DescriptorType;
        ComponentF::DescriptorType* descriptorComponentF = aznew ComponentF::DescriptorType;
        (void)descriptorComponentB;
        (void)descriptorComponentC;
        (void)descriptorComponentD;
        (void)descriptorComponentE;
        (void)descriptorComponentF;

        ComponentApplication componentApp;
        ComponentApplication::Descriptor desc;
        desc.m_useExistingAllocator = true;
        ComponentApplication::StartupParameters startupParams;
        startupParams.m_allocator = &AZ::AllocatorInstance<AZ::SystemAllocator>::Get();
        Entity* systemEntity = componentApp.Create(desc, startupParams);
        AZ_TEST_ASSERT(systemEntity);
        systemEntity->Init();

        Entity* entity = aznew Entity("My Entity");
        AZ_TEST_ASSERT(entity->GetState() == Entity::ES_CONSTRUCTED);

        ComponentB* componentB = aznew ComponentB;
        ComponentC* componentC = aznew ComponentC;
        ComponentF* componentF = aznew ComponentF;

        ComponentDescriptor::DependencyArrayType requiredServices;

        // Add components and check IsReadyToAddFunction
        entity->CreateComponent<ComponentA>();
        AZ_TEST_ASSERT(entity->IsComponentReadyToAdd(componentC, &requiredServices) == false); // we require B component to be added
        AZ_TEST_ASSERT(requiredServices.size() == 1);
        AZ_TEST_ASSERT(requiredServices[0] == AZ_CRC("ServiceB", 0x1982c19b));
        entity->AddComponent(componentB);
        AZ_TEST_ASSERT(entity->IsComponentReadyToAdd(componentC) == true); // we require B component to be added
        entity->AddComponent(componentC);
        entity->CreateComponent<ComponentD>();
        entity->CreateComponent<ComponentE>();
        // Check compatibility
        Entity::ComponentArrayType incompaibleComponents;
        AZ_TEST_ASSERT(entity->IsComponentReadyToAdd(componentF, nullptr, &incompaibleComponents) == false);
        AZ_TEST_ASSERT(incompaibleComponents.size() == 2);
        AZ_TEST_ASSERT(AZ::RttiIsTypeOf<ComponentA>(incompaibleComponents[0]));
        AZ_TEST_ASSERT(AZ::RttiIsTypeOf<ComponentB>(incompaibleComponents[1]));


        {
            const Entity::ComponentArrayType& components = entity->GetComponents();
            AZ_TEST_ASSERT(components[0]->RTTI_IsTypeOf(AzTypeInfo<ComponentA>::Uuid()));
            AZ_TEST_ASSERT(components[1]->RTTI_IsTypeOf(AzTypeInfo<ComponentB>::Uuid()));
            AZ_TEST_ASSERT(components[2]->RTTI_IsTypeOf(AzTypeInfo<ComponentC>::Uuid()));
            AZ_TEST_ASSERT(components[3]->RTTI_IsTypeOf(AzTypeInfo<ComponentD>::Uuid()));
            AZ_TEST_ASSERT(components[4]->RTTI_IsTypeOf(AzTypeInfo<ComponentE>::Uuid()));
        }

        entity->Init(); // Init should not change the component order

        {
            const Entity::ComponentArrayType& components = entity->GetComponents();
            AZ_TEST_ASSERT(components[0]->RTTI_IsTypeOf(AzTypeInfo<ComponentA>::Uuid()));
            AZ_TEST_ASSERT(components[1]->RTTI_IsTypeOf(AzTypeInfo<ComponentB>::Uuid()));
            AZ_TEST_ASSERT(components[2]->RTTI_IsTypeOf(AzTypeInfo<ComponentC>::Uuid()));
            AZ_TEST_ASSERT(components[3]->RTTI_IsTypeOf(AzTypeInfo<ComponentD>::Uuid()));
            AZ_TEST_ASSERT(components[4]->RTTI_IsTypeOf(AzTypeInfo<ComponentE>::Uuid()));
        }

        entity->Activate(); // here components will be sorted based on order

        {
            const Entity::ComponentArrayType& components = entity->GetComponents();
            AZ_TEST_ASSERT(components[0]->RTTI_IsTypeOf(AzTypeInfo<ComponentA>::Uuid()) || components[0]->RTTI_IsTypeOf(AzTypeInfo<ComponentD>::Uuid()));
            AZ_TEST_ASSERT(components[1]->RTTI_IsTypeOf(AzTypeInfo<ComponentD>::Uuid()) || components[1]->RTTI_IsTypeOf(AzTypeInfo<ComponentA>::Uuid()));
            AZ_TEST_ASSERT(components[2]->RTTI_IsTypeOf(AzTypeInfo<ComponentE>::Uuid()));
            AZ_TEST_ASSERT(components[3]->RTTI_IsTypeOf(AzTypeInfo<ComponentB>::Uuid()));
            AZ_TEST_ASSERT(components[4]->RTTI_IsTypeOf(AzTypeInfo<ComponentC>::Uuid()));
        }

        entity->Deactivate();

        {
            const Entity::ComponentArrayType& components = entity->GetComponents();
            AZ_TEST_ASSERT(components[0]->RTTI_IsTypeOf(AzTypeInfo<ComponentA>::Uuid()) || components[0]->RTTI_IsTypeOf(AzTypeInfo<ComponentD>::Uuid()));
            AZ_TEST_ASSERT(components[1]->RTTI_IsTypeOf(AzTypeInfo<ComponentD>::Uuid()) || components[1]->RTTI_IsTypeOf(AzTypeInfo<ComponentA>::Uuid()));
            AZ_TEST_ASSERT(components[2]->RTTI_IsTypeOf(AzTypeInfo<ComponentE>::Uuid()));
            AZ_TEST_ASSERT(components[3]->RTTI_IsTypeOf(AzTypeInfo<ComponentB>::Uuid()));
            AZ_TEST_ASSERT(components[4]->RTTI_IsTypeOf(AzTypeInfo<ComponentC>::Uuid()));
        }

        descriptorComponentA->m_isDependent = true; // now A should depend on D (but only after we notify the entity of the change)

        entity->Activate();

        {
            // order should be unchanged (because we cache the dependency)
            const Entity::ComponentArrayType& components = entity->GetComponents();
            AZ_TEST_ASSERT(components[0]->RTTI_IsTypeOf(AzTypeInfo<ComponentA>::Uuid()) || components[0]->RTTI_IsTypeOf(AzTypeInfo<ComponentD>::Uuid()));
            AZ_TEST_ASSERT(components[1]->RTTI_IsTypeOf(AzTypeInfo<ComponentD>::Uuid()) || components[1]->RTTI_IsTypeOf(AzTypeInfo<ComponentA>::Uuid()));
            AZ_TEST_ASSERT(components[2]->RTTI_IsTypeOf(AzTypeInfo<ComponentE>::Uuid()));
            AZ_TEST_ASSERT(components[3]->RTTI_IsTypeOf(AzTypeInfo<ComponentB>::Uuid()));
            AZ_TEST_ASSERT(components[4]->RTTI_IsTypeOf(AzTypeInfo<ComponentC>::Uuid()));
        }

        entity->Deactivate();

        entity->InvalidateDependencies();

        entity->Activate();

        {
            // check the new order
            const Entity::ComponentArrayType& components = entity->GetComponents();
            AZ_TEST_ASSERT(components[0]->RTTI_IsTypeOf(AzTypeInfo<ComponentD>::Uuid()));
            AZ_TEST_ASSERT(components[1]->RTTI_IsTypeOf(AzTypeInfo<ComponentA>::Uuid()));
            AZ_TEST_ASSERT(components[2]->RTTI_IsTypeOf(AzTypeInfo<ComponentE>::Uuid()));
            AZ_TEST_ASSERT(components[3]->RTTI_IsTypeOf(AzTypeInfo<ComponentB>::Uuid()));
            AZ_TEST_ASSERT(components[4]->RTTI_IsTypeOf(AzTypeInfo<ComponentC>::Uuid()));
        }

        entity->Deactivate();

        Entity::ComponentArrayType dependentComponents;
        AZ_TEST_ASSERT(entity->IsComponentReadyToRemove(componentB, &dependentComponents) == false); // component C requires us
        AZ_TEST_ASSERT(dependentComponents.size() == 1);
        AZ_TEST_ASSERT(dependentComponents[0] == componentC);
        entity->RemoveComponent(componentC);
        AZ_TEST_ASSERT(entity->IsComponentReadyToRemove(componentB) == true); // we should be ready for remove
        entity->RemoveComponent(componentB);

        delete componentB;
        delete componentC;
        delete componentF;

        delete entity;

        // Example of manually deleted descriptor, the rest we leave for the app to clean up.
        delete descriptorComponentB;
    }

    /**
     * UserSettingsComponent test
     */
     class UserSettingsTestApp
         : public ComponentApplication
         , public UserSettingsFileLocatorBus::Handler
     {
     public:
         void SetExecutableFolder(const char* path)
         {
             azstrcpy(m_exeDirectory, AZ_ARRAY_SIZE(m_exeDirectory), path);
         }

        AZStd::string ResolveFilePath(u32 providerId) override
        {
            AZStd::string filePath;
            if (providerId == UserSettings::CT_GLOBAL)
            {
                filePath.append(m_exeDirectory);
                filePath.append("GlobalUserSettings.xml");
            }
            else if (providerId == UserSettings::CT_LOCAL)
            {
                filePath.append(m_exeDirectory);
                filePath.append("LocalUserSettings.xml");
            }
            return filePath;
        }

        const char* GetAppRoot() override
        {
            return m_rootPath.c_str();
        }

        OSString m_rootPath;
    };

    class MyUserSettings
        : public UserSettings
    {
    public:
        AZ_CLASS_ALLOCATOR(MyUserSettings, SystemAllocator, 0);
        AZ_RTTI(MyUserSettings, "{ACC60C7B-60D8-4491-AD5D-42BA6656CC1F}", UserSettings);

        static void Reflect(AZ::SerializeContext* sc)
        {
            sc->Class<MyUserSettings, UserSettings>()
                ->Field("intOption1", &MyUserSettings::m_intOption1);
        }

        int m_intOption1;
    };

    TEST(UserSettings, Test)
    {
        UserSettingsTestApp app;

        //////////////////////////////////////////////////////////////////////////
        // Create application environment code driven
        ComponentApplication::Descriptor appDesc;
        appDesc.m_memoryBlocksByteSize = 10 * 1024 * 1024;
        Entity* systemEntity = app.Create(appDesc);
        app.SetExecutableFolder(GetTestFolderPath().c_str());
        app.UserSettingsFileLocatorBus::Handler::BusConnect();

        // Make sure user settings file does not exist at this point
        {
            IO::SystemFile::Delete(app.ResolveFilePath(UserSettings::CT_GLOBAL).c_str());
            IO::SystemFile::Delete(app.ResolveFilePath(UserSettings::CT_LOCAL).c_str());
        }

        MyUserSettings::Reflect(app.GetSerializeContext());
        systemEntity->CreateComponent<MemoryComponent>();

        UserSettingsComponent* globalUserSettingsComponent = systemEntity->CreateComponent<UserSettingsComponent>();
        AZ_TEST_ASSERT(globalUserSettingsComponent);
        globalUserSettingsComponent->SetProviderId(UserSettings::CT_GLOBAL);

        UserSettingsComponent* localUserSettingsComponent = systemEntity->CreateComponent<UserSettingsComponent>();
        AZ_TEST_ASSERT(localUserSettingsComponent);
        localUserSettingsComponent->SetProviderId(UserSettings::CT_LOCAL);

        systemEntity->Init();
        systemEntity->Activate();

        AZStd::intrusive_ptr<MyUserSettings> myGlobalUserSettings = UserSettings::CreateFind<MyUserSettings>(AZ_CRC("MyUserSettings", 0x65286904), UserSettings::CT_GLOBAL);
        AZ_TEST_ASSERT(myGlobalUserSettings);
        myGlobalUserSettings->m_intOption1 = 10;
        AZStd::intrusive_ptr<MyUserSettings> storedGlobalSettings = UserSettings::CreateFind<MyUserSettings>(AZ_CRC("MyUserSettings", 0x65286904), UserSettings::CT_GLOBAL);
        AZ_TEST_ASSERT(myGlobalUserSettings == storedGlobalSettings);
        AZ_TEST_ASSERT(storedGlobalSettings->m_intOption1 == 10);

        AZStd::intrusive_ptr<MyUserSettings> myLocalUserSettings = UserSettings::CreateFind<MyUserSettings>(AZ_CRC("MyUserSettings", 0x65286904), UserSettings::CT_LOCAL);
        AZ_TEST_ASSERT(myLocalUserSettings);
        myLocalUserSettings->m_intOption1 = 20;
        AZStd::intrusive_ptr<MyUserSettings> storedLocalSettings = UserSettings::CreateFind<MyUserSettings>(AZ_CRC("MyUserSettings", 0x65286904), UserSettings::CT_LOCAL);
        AZ_TEST_ASSERT(myLocalUserSettings == storedLocalSettings);
        AZ_TEST_ASSERT(storedLocalSettings->m_intOption1 == 20);

        // Deactivating should trigger saving of user options.
        systemEntity->Deactivate();

        // Deactivate() should have cleared all the registered user options
        storedGlobalSettings = UserSettings::Find<MyUserSettings>(AZ_CRC("MyUserSettings", 0x65286904), UserSettings::CT_GLOBAL);
        AZ_TEST_ASSERT(!storedGlobalSettings);
        storedLocalSettings = UserSettings::Find<MyUserSettings>(AZ_CRC("MyUserSettings", 0x65286904), UserSettings::CT_LOCAL);
        AZ_TEST_ASSERT(!storedLocalSettings);

        systemEntity->Activate();

        // Verify that upon re-activation, we successfully loaded all settings saved during deactivation
        storedGlobalSettings = UserSettings::Find<MyUserSettings>(AZ_CRC("MyUserSettings", 0x65286904), UserSettings::CT_GLOBAL);
        AZ_TEST_ASSERT(storedGlobalSettings);
        myGlobalUserSettings = UserSettings::CreateFind<MyUserSettings>(AZ_CRC("MyUserSettings", 0x65286904), UserSettings::CT_GLOBAL);
        AZ_TEST_ASSERT(myGlobalUserSettings == storedGlobalSettings);
        AZ_TEST_ASSERT(storedGlobalSettings->m_intOption1 == 10);

        storedLocalSettings = UserSettings::Find<MyUserSettings>(AZ_CRC("MyUserSettings", 0x65286904), UserSettings::CT_LOCAL);
        AZ_TEST_ASSERT(storedLocalSettings);
        myLocalUserSettings = UserSettings::CreateFind<MyUserSettings>(AZ_CRC("MyUserSettings", 0x65286904), UserSettings::CT_LOCAL);
        AZ_TEST_ASSERT(myLocalUserSettings == storedLocalSettings);
        AZ_TEST_ASSERT(storedLocalSettings->m_intOption1 == 20);

        {
            UnitTest::TestFileIOBase fileIO;
            UnitTest::SetRestoreFileIOBaseRAII restoreFileIOScope(fileIO);
            // NOTE: this string needs to go out of scope before app is destroyed
            AZStd::string bootstrapFile = GetTestFolderPath() + "bootstrap.xml";
            app.WriteApplicationDescriptor(bootstrapFile.c_str());
        }

        myGlobalUserSettings = nullptr;
        storedGlobalSettings = nullptr;
        UserSettings::Release(myLocalUserSettings);
        UserSettings::Release(storedLocalSettings);

        app.Destroy();
        //////////////////////////////////////////////////////////////////////////
    }

    class FrameProfilerComponentTest
        : public AllocatorsFixture
        , public FrameProfilerBus::Handler
    {
    public:
        FrameProfilerComponentTest()
            : AllocatorsFixture(15, false)
        {
        }

        //////////////////////////////////////////////////////////////////////////
        // FrameProfilerDrillerBus
        virtual void OnFrameProfilerData(const FrameProfiler::ThreadDataArray& data)
        {
            for (size_t iThread = 0; iThread < data.size(); ++iThread)
            {
                const FrameProfiler::ThreadData& td = data[iThread];
                FrameProfiler::ThreadData::RegistersMap::const_iterator regIt = td.m_registers.begin();
                size_t numRegisters = m_numRegistersReceived;
                for (; regIt != td.m_registers.end(); ++regIt)
                {
                    const FrameProfiler::RegisterData& rd = regIt->second;

                    AZ_TEST_ASSERT(rd.m_function != NULL);
                    if (strstr(rd.m_function, "ChildFunction") || strstr(rd.m_function, "Profile1")) // filter only the test registers
                    {
                        ++m_numRegistersReceived;

                        EXPECT_GT(rd.m_line, 0);
                        EXPECT_TRUE(rd.m_name == nullptr || strstr(rd.m_name, "Child1") || strstr(rd.m_name, "Custom name"));
                        AZ::u32 unitTestCrc = AZ_CRC("UnitTest", 0x8089cea8);
                        EXPECT_EQ(unitTestCrc, rd.m_systemId);
                        EXPECT_EQ(ProfilerRegister::PRT_TIME, rd.m_type);

                        EXPECT_FALSE(rd.m_frames.empty());
                        const FrameProfiler::FrameData& fd = rd.m_frames.back();
                        EXPECT_GT(fd.m_frameId, 0u);
                        EXPECT_GT(fd.m_timeData.m_time, 0);
                        EXPECT_GT(fd.m_timeData.m_calls, 0);
                    }
                }

                if (numRegisters < m_numRegistersReceived)
                {
                    // we have received valid test registers for this thread, add it to the list
                    ++m_numThreads;
                }
            }
        }
        //////////////////////////////////////////////////////////////////////////

        int ChildFunction(int input)
        {
            AZ_PROFILE_TIMER("UnitTest", nullptr, NamedRegister);
            int result = 5;
            for (int i = 0; i < 10000; ++i)
            {
                result += i % (input + 3);
            }
            AZ_PROFILE_TIMER_END(NamedRegister);
            return result;
        }

        int ChildFunction1(int input)
        {
            AZ_PROFILE_TIMER("UnitTest", "Child1");
            int result = 5;
            for (int i = 0; i < 10000; ++i)
            {
                result += i % (input + 1);
            }
            return result;
        }

        int Profile1(int numIterations)
        {
            AZ_PROFILE_TIMER("UnitTest", "Custom name");
            int result = 0;
            for (int i = 0; i < numIterations; ++i)
            {
                result += ChildFunction(i);
            }

            result += ChildFunction1(numIterations / 3);
            return result;
        }

        void run()
        {
            FrameProfilerBus::Handler::BusConnect();

            ComponentApplication app;
            ComponentApplication::Descriptor desc;
            desc.m_useExistingAllocator = true;
            ComponentApplication::StartupParameters startupParams;
            startupParams.m_allocator = &AZ::AllocatorInstance<AZ::SystemAllocator>::Get();
            Entity* systemEntity = app.Create(desc, startupParams);
            systemEntity->CreateComponent<FrameProfilerComponent>();

            systemEntity->Init();
            systemEntity->Activate(); // start frame component

            m_numThreads = 0;
            m_numRegistersReceived = 0;

            // tick to frame 1 and collect all the samples
            app.Tick();
            EXPECT_EQ(0, m_numThreads);
            EXPECT_EQ(0, m_numRegistersReceived);

            int numIterations = 10000;
            {
                AZStd::thread t1(AZStd::bind(&FrameProfilerComponentTest::Profile1, this, numIterations));
                AZStd::thread t2(AZStd::bind(&FrameProfilerComponentTest::Profile1, this, numIterations));
                AZStd::thread t3(AZStd::bind(&FrameProfilerComponentTest::Profile1, this, numIterations));
                AZStd::thread t4(AZStd::bind(&FrameProfilerComponentTest::Profile1, this, numIterations));

                t1.join();
                t2.join();
                t3.join();
                t4.join();
            }

            // tick to frame 2 and collect all the samples
            app.Tick();

            EXPECT_EQ(4, m_numThreads);
            EXPECT_EQ(m_numThreads * 3, m_numRegistersReceived);

            FrameProfilerBus::Handler::BusDisconnect();

            app.Destroy();
        }

        size_t m_numRegistersReceived;
        size_t m_numThreads;
    };

    TEST_F(FrameProfilerComponentTest, Test)
    {
        run();
    }

    class SimpleEntityRefTestComponent
        : public Component
    {
    public:
        AZ_COMPONENT(SimpleEntityRefTestComponent, "{ED4D3C2A-454D-47B0-B04E-9A26DC55D03B}");

        SimpleEntityRefTestComponent(EntityId useId = EntityId())
            : m_entityId(useId) {}

        void Activate() override {}
        void Deactivate() override {}

        static void Reflect(ReflectContext* reflection)
        {
            SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(reflection);
            if (serializeContext)
            {
                serializeContext->Class<SimpleEntityRefTestComponent>()
                    ->Field("entityId", &SimpleEntityRefTestComponent::m_entityId);
            }
        }

        EntityId m_entityId;
    };

    class ComplexEntityRefTestComponent
        : public Component
    {
    public:
        AZ_COMPONENT(ComplexEntityRefTestComponent, "{BCCCD213-4A77-474C-B432-48DE6DB2FE4D}");

        ComplexEntityRefTestComponent()
            : m_entityIdHashMap(3) // create some buckets to make sure we distribute elements even when we have less than load factor
            , m_entityIdHashSet(3) // create some buckets to make sure we distribute elements even when we have less than load factor
        {
        }

        void Activate() override {}
        void Deactivate() override {}

        static void Reflect(ReflectContext* reflection)
        {
            SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(reflection);
            if (serializeContext)
            {
                serializeContext->Class<ComplexEntityRefTestComponent>()
                    ->Field("entityIds", &ComplexEntityRefTestComponent::m_entityIds)
                    ->Field("entityIdHashMap", &ComplexEntityRefTestComponent::m_entityIdHashMap)
                    ->Field("entityIdHashSet", &ComplexEntityRefTestComponent::m_entityIdHashSet)
                    ->Field("entityId", &ComplexEntityRefTestComponent::m_entityIdIntMap);
            }
        }

        AZStd::vector<EntityId> m_entityIds;
        AZStd::unordered_map<EntityId, int> m_entityIdHashMap;
        AZStd::unordered_set<EntityId> m_entityIdHashSet;
        AZStd::map<EntityId, int> m_entityIdIntMap;
    };

    struct EntityIdRemapContainer
    {
        AZ_TYPE_INFO(EntityIdRemapContainer, "{63854212-37E9-480B-8E46-529682AB9EF7}");
        AZ_CLASS_ALLOCATOR(EntityIdRemapContainer, AZ::SystemAllocator, 0);

        static void Reflect(SerializeContext& serializeContext)
        {
            serializeContext.Class<EntityIdRemapContainer>()
                ->Field("Entity", &EntityIdRemapContainer::m_entity)
                ->Field("Id", &EntityIdRemapContainer::m_id)
                ->Field("otherId", &EntityIdRemapContainer::m_otherId)
                ;
        }
        AZ::Entity* m_entity;
        AZ::EntityId m_id;
        AZ::EntityId m_otherId;
    };

    TEST_F(Components, EntityUtilsTest)
    {
        EntityId id1 = Entity::MakeId();

        {
            EntityId id2 = Entity::MakeId();
            EntityId id3 = Entity::MakeId();
            EntityId id4 = Entity::MakeId();
            EntityId id5 = Entity::MakeId();
            SimpleEntityRefTestComponent testComponent1(id1);
            SimpleEntityRefTestComponent testComponent2(id2);
            SimpleEntityRefTestComponent testComponent3(id3);
            Entity testEntity(id1);
            testEntity.AddComponent(&testComponent1);
            testEntity.AddComponent(&testComponent2);
            testEntity.AddComponent(&testComponent3);

            SerializeContext context;
            const ComponentDescriptor* entityRefTestDescriptor = SimpleEntityRefTestComponent::CreateDescriptor();
            entityRefTestDescriptor->Reflect(&context);
            Entity::Reflect(&context);

                unsigned int nReplaced = EntityUtils::ReplaceEntityRefs(
                &testEntity
                , [=](EntityId key, bool /*isEntityId*/) -> EntityId
            {
                if (key == id1)
                {
                    return id4;
                }
                if (key == id2)
                {
                    return id5;
                }
                return key;
            }
                , &context
                );
            AZ_TEST_ASSERT(nReplaced == 2);
            AZ_TEST_ASSERT(testEntity.GetId() == id1);
            AZ_TEST_ASSERT(testComponent1.m_entityId == id4);
            AZ_TEST_ASSERT(testComponent2.m_entityId == id5);
            AZ_TEST_ASSERT(testComponent3.m_entityId == id3);

            testEntity.RemoveComponent(&testComponent1);
            testEntity.RemoveComponent(&testComponent2);
            testEntity.RemoveComponent(&testComponent3);
            delete entityRefTestDescriptor;
        }

        // Test entity IDs replacement in special containers (that require update as a result of EntityId replacement)
        {
            // special crafted id, so we can change the hashing structure as
            // we replace the entities ID
            EntityId id2(1);
            EntityId id3(13);
            EntityId replace2(14);
            EntityId replace3(3);

            SerializeContext context;
            const ComponentDescriptor* entityRefTestDescriptor = ComplexEntityRefTestComponent::CreateDescriptor();
            entityRefTestDescriptor->Reflect(&context);
            Entity::Reflect(&context);

            ComplexEntityRefTestComponent testComponent1;
            Entity testEntity(id1);
            testEntity.AddComponent(&testComponent1);
            // vector (baseline, it should not change, same with all other AZStd containers not tested below)
            testComponent1.m_entityIds.push_back(id2);
            testComponent1.m_entityIds.push_back(id3);
            testComponent1.m_entityIds.push_back(EntityId(32));
            // hash map
            testComponent1.m_entityIdHashMap.insert(AZStd::make_pair(id2, 1));
            testComponent1.m_entityIdHashMap.insert(AZStd::make_pair(id3, 2));
            testComponent1.m_entityIdHashMap.insert(AZStd::make_pair(EntityId(32), 3));
            testComponent1.m_entityIdHashMap.insert(AZStd::make_pair(EntityId(5), 4));
            testComponent1.m_entityIdHashMap.insert(AZStd::make_pair(EntityId(16), 5));
            // hash set
            testComponent1.m_entityIdHashSet.insert(id2);
            testComponent1.m_entityIdHashSet.insert(id3);
            testComponent1.m_entityIdHashSet.insert(EntityId(32));
            testComponent1.m_entityIdHashSet.insert(EntityId(5));
            testComponent1.m_entityIdHashSet.insert(EntityId(16));
            // map 
            testComponent1.m_entityIdIntMap.insert(AZStd::make_pair(id2, 1));
            testComponent1.m_entityIdIntMap.insert(AZStd::make_pair(id3, 2));
            testComponent1.m_entityIdIntMap.insert(AZStd::make_pair(EntityId(32), 3));
            testComponent1.m_entityIdIntMap.insert(AZStd::make_pair(EntityId(5), 4));
            testComponent1.m_entityIdIntMap.insert(AZStd::make_pair(EntityId(16), 5));
            // set is currently not supported in the serializer, when implemented if it uses the same Associative container storage (which it should) it should just work

                unsigned int nReplaced = EntityUtils::ReplaceEntityRefs(
                &testEntity
                , [=](EntityId key, bool /*isEntityId*/) -> EntityId
            {
                if (key == id2)
                {
                    return replace2;
                }
                if (key == id3)
                {
                    return replace3;
                }
                return key;
            }
                , &context
                );
            AZ_TEST_ASSERT(nReplaced == 8);
            AZ_TEST_ASSERT(testEntity.GetId() == id1);

            AZ_TEST_ASSERT(AZStd::find(testComponent1.m_entityIds.begin(), testComponent1.m_entityIds.end(), id2) == testComponent1.m_entityIds.end());
            AZ_TEST_ASSERT(AZStd::find(testComponent1.m_entityIds.begin(), testComponent1.m_entityIds.end(), replace2) != testComponent1.m_entityIds.end());
            AZ_TEST_ASSERT(AZStd::find(testComponent1.m_entityIds.begin(), testComponent1.m_entityIds.end(), replace3) != testComponent1.m_entityIds.end());
            AZ_TEST_ASSERT(AZStd::find(testComponent1.m_entityIds.begin(), testComponent1.m_entityIds.end(), EntityId(32)) != testComponent1.m_entityIds.end());

            AZ_TEST_ASSERT(testComponent1.m_entityIdHashMap.find(id2) == testComponent1.m_entityIdHashMap.end());
            AZ_TEST_ASSERT(testComponent1.m_entityIdHashMap.find(replace2) != testComponent1.m_entityIdHashMap.end());
            AZ_TEST_ASSERT(testComponent1.m_entityIdHashMap.find(replace3) != testComponent1.m_entityIdHashMap.end());
            AZ_TEST_ASSERT(testComponent1.m_entityIdHashMap.find(EntityId(32)) != testComponent1.m_entityIdHashMap.end());

            AZ_TEST_ASSERT(testComponent1.m_entityIdHashSet.find(id2) == testComponent1.m_entityIdHashSet.end());
            AZ_TEST_ASSERT(testComponent1.m_entityIdHashSet.find(replace2) != testComponent1.m_entityIdHashSet.end());
            AZ_TEST_ASSERT(testComponent1.m_entityIdHashSet.find(replace3) != testComponent1.m_entityIdHashSet.end());
            AZ_TEST_ASSERT(testComponent1.m_entityIdHashSet.find(EntityId(32)) != testComponent1.m_entityIdHashSet.end());

            AZ_TEST_ASSERT(testComponent1.m_entityIdIntMap.find(id2) == testComponent1.m_entityIdIntMap.end());
            AZ_TEST_ASSERT(testComponent1.m_entityIdIntMap.find(replace2) != testComponent1.m_entityIdIntMap.end());
            AZ_TEST_ASSERT(testComponent1.m_entityIdIntMap.find(replace3) != testComponent1.m_entityIdIntMap.end());
            AZ_TEST_ASSERT(testComponent1.m_entityIdIntMap.find(EntityId(32)) != testComponent1.m_entityIdIntMap.end());

            testEntity.RemoveComponent(&testComponent1);
            delete entityRefTestDescriptor;
        }
    }

    TEST_F(Components, EntityIdGeneration)
    {
        // Generate 1 million ids across 100 threads, and ensure that none collide
        AZStd::concurrent_unordered_set<AZ::EntityId> entityIds;
        auto GenerateIdThread = [&entityIds]()
        {
            for (size_t i = 0; i < 10000; ++i)
            {
                EXPECT_TRUE(entityIds.insert(Entity::MakeId()));
            }
        };

        //////////////////////////////////////////////////////////////////////////
        // test generating EntityIDs from multiple threads
        {
            AZStd::vector<AZStd::thread> threads;
            for (size_t i = 0; i < 100; ++i)
            {
                threads.emplace_back(GenerateIdThread);
            }
            for (AZStd::thread& thread : threads)
            {
                thread.join();
            }
        }
    }

    //=========================================================================
    // Component Configuration

    class ConfigurableComponentConfig : public ComponentConfig
    {
    public:
        AZ_RTTI(ConfigurableComponentConfig, "{109C5A93-5571-4D45-BD2F-3938BF63AD83}", ComponentConfig);

        int m_intVal = 0;
    };

    class ConfigurableComponent : public Component
    {
    public:
        AZ_RTTI(ConfigurableComponent, "{E3103830-70F3-47AE-8F22-EF09BF3D57E9}", Component);
        AZ_CLASS_ALLOCATOR(ConfigurableComponent, SystemAllocator, 0);

        int m_intVal = 0;

    protected:
        void Activate() override {}
        void Deactivate() override {}

        bool ReadInConfig(const ComponentConfig* baseConfig) override
        {
            if (auto config = azrtti_cast<const ConfigurableComponentConfig*>(baseConfig))
            {
                m_intVal = config->m_intVal;
                return true;
            }
            return false;
        }

        bool WriteOutConfig(ComponentConfig* outBaseConfig) const override
        {
            if (auto config = azrtti_cast<ConfigurableComponentConfig*>(outBaseConfig))
            {
                config->m_intVal = m_intVal;
                return true;
            }
            return false;
        }
    };

    TEST_F(Components, SetConfiguration_Succeeds)
    {
        ConfigurableComponentConfig config;
        config.m_intVal = 5;

        ConfigurableComponent component;

        EXPECT_TRUE(component.SetConfiguration(config));
        EXPECT_EQ(component.m_intVal, 5);
    }

    TEST_F(Components, SetConfigurationOnActiveEntity_DoesNothing)
    {
        ConfigurableComponentConfig config;
        config.m_intVal = 5;

        Entity entity;
        auto component = entity.CreateComponent<ConfigurableComponent>();
        entity.Init();
        entity.Activate();

        EXPECT_FALSE(component->SetConfiguration(config));
        EXPECT_NE(component->m_intVal, 5);
    }

    TEST_F(Components, SetWrongKindOfConfiguration_DoesNothing)
    {
        ComponentConfig config; // base config type

        ConfigurableComponent component;
        component.m_intVal = 19;

        EXPECT_FALSE(component.SetConfiguration(config));
        EXPECT_EQ(component.m_intVal, 19);
    }

    TEST_F(Components, GetConfiguration_Succeeds)
    {
        ConfigurableComponent component;
        component.m_intVal = 9;

        ConfigurableComponentConfig config;

        component.GetConfiguration(config);
        EXPECT_EQ(component.m_intVal, 9);
    }

    class UnconfigurableComponent : public Component
    {
    public:
        AZ_RTTI(UnconfigurableComponent, "{772E3AA6-67AC-4655-B6C4-70BC45BAFD35}", Component);

        void Activate() override {}
        void Deactivate() override {}
    };

    TEST_F(Components, SetConfigurationOnUnconfigurableComponent_Fails)
    {
        UnconfigurableComponent component;
        ConfigurableComponentConfig config;

        EXPECT_FALSE(component.SetConfiguration(config));
    }

    TEST_F(Components, GetConfigurationOnUnconfigurableComponent_Fails)
    {
        UnconfigurableComponent component;
        ConfigurableComponentConfig config;

        EXPECT_FALSE(component.GetConfiguration(config));
    }

    TEST_F(Components, GenerateNewIdsAndFixRefsExistingMapTest)
    {
        SerializeContext context;
        Entity::Reflect(&context);
        EntityIdRemapContainer::Reflect(context);

        const AZ::EntityId testId(21);
        const AZ::EntityId nonMappedId(5465);
        EntityIdRemapContainer testContainer1;
        testContainer1.m_entity = aznew Entity(testId);
        testContainer1.m_id = testId;
        testContainer1.m_otherId = nonMappedId;

        EntityIdRemapContainer clonedContainer;
        context.CloneObjectInplace(clonedContainer, &testContainer1);
        
        // Check cloned entity has same ids
        EXPECT_NE(nullptr, clonedContainer.m_entity);
        EXPECT_EQ(testContainer1.m_entity->GetId(), clonedContainer.m_entity->GetId());
        EXPECT_EQ(testContainer1.m_id, clonedContainer.m_id);
        EXPECT_EQ(testContainer1.m_otherId, clonedContainer.m_otherId);
        
        // Generated new Ids in the testContainer store the results in the newIdMap
        // The m_entity Entity id values should be remapped to a new value
        AZStd::unordered_map<AZ::EntityId, AZ::EntityId> newIdMap;
        EntityUtils::GenerateNewIdsAndFixRefs(&testContainer1, newIdMap, &context);
        
        EXPECT_EQ(testContainer1.m_entity->GetId(), testContainer1.m_id);
        EXPECT_NE(clonedContainer.m_entity->GetId(), testContainer1.m_entity->GetId());
        EXPECT_NE(clonedContainer.m_id, testContainer1.m_id);
        EXPECT_EQ(clonedContainer.m_otherId, testContainer1.m_otherId);

        // Use the existing newIdMap to generate entityIds for the clonedContainer
        // The testContainer1 and ClonedContainer should now have the same ids again
        EntityUtils::GenerateNewIdsAndFixRefs(&clonedContainer, newIdMap, &context);

        EXPECT_EQ(clonedContainer.m_entity->GetId(), clonedContainer.m_id);
        EXPECT_EQ(testContainer1.m_entity->GetId(), clonedContainer.m_entity->GetId());
        EXPECT_EQ(testContainer1.m_id, clonedContainer.m_id);
        EXPECT_EQ(testContainer1.m_otherId, clonedContainer.m_otherId);

        // Use a new map to generate entityIds for the clonedContainer
        // The testContainer1 and ClonedContainer should have different ids again
        AZStd::map<AZ::EntityId, AZ::EntityId> clonedIdMap; // Using regular map to test that different map types works with GenerateNewIdsAndFixRefs
        EntityUtils::GenerateNewIdsAndFixRefs(&clonedContainer, clonedIdMap, &context);

        EXPECT_EQ(clonedContainer.m_entity->GetId(), clonedContainer.m_id);
        EXPECT_NE(testContainer1.m_entity->GetId(), clonedContainer.m_entity->GetId());
        EXPECT_NE(testContainer1.m_id, clonedContainer.m_id);
        EXPECT_EQ(testContainer1.m_otherId, clonedContainer.m_otherId);
        delete testContainer1.m_entity;
        delete clonedContainer.m_entity;
    }

    //=========================================================================
    // Component Configuration versioning

    // Version 1 of a configuration for a HydraComponent
    class HydraConfigV1
        : public ComponentConfig
    {
    public:
        AZ_RTTI(HydraConfigV1, "{02198FDB-5CDB-4983-BC0B-CF1AA20FF2AF}", ComponentConfig);

        int m_numHeads = 1;
    };

    // To add fields, inherit from previous version.
    class HydraConfigV2
        : public HydraConfigV1
    {
    public:
        AZ_RTTI(HydraConfigV2, "{BC68C167-6B01-489C-8415-626455670C34}", HydraConfigV1);

        int m_numArms = 2; // now the hydra has multiple arms, as well as multiple heads
    };

    // To make a breaking change, start from scratch by inheriting from base ComponentConfig.
    class HydraConfigV3
        : public ComponentConfig
    {
    public:
        AZ_RTTI(HydraConfigV3, "{71C41829-AA51-4179-B8B4-3C278CBB26AA}", ComponentConfig);

        int m_numHeads = 1;
        int m_numArmsPerHead = 2; // now we require each head to have the same number of arms
    };

    // A component with many heads, and many arms
    class HydraComponent
        : public Component
    {
    public:
        AZ_RTTI(HydraComponent, "", Component);
        AZ_CLASS_ALLOCATOR(HydraComponent, AZ::SystemAllocator, 0);

        // serialized data
        HydraConfigV3 m_config;

        // runtime data
        int m_numArms;

        HydraComponent() = default;

        void Activate() override
        {
            m_numArms = m_config.m_numHeads * m_config.m_numArmsPerHead;
        }

        void Deactivate() override {}

        bool ReadInConfig(const ComponentConfig* baseConfig) override
        {
            if (auto v1 = azrtti_cast<const HydraConfigV1*>(baseConfig))
            {
                m_config.m_numHeads = v1->m_numHeads;

                // v2 is based on v1
                if (auto v2 = azrtti_cast<const HydraConfigV2*>(v1))
                {
                    // v2 let user specify the total number of arms, but now we force each head to have same number of arms
                    if (v2->m_numHeads <= 0)
                    {
                        m_config.m_numArmsPerHead = 0;
                    }
                    else
                    {
                        m_config.m_numArmsPerHead = v2->m_numArms / v2->m_numHeads;
                    }
                }
                else
                {
                    // v1 assumed 2 arms per head
                    m_config.m_numArmsPerHead = 2;
                }

                return true;
            }

            if (auto v3 = azrtti_cast<const HydraConfigV3*>(baseConfig))
            {
                m_config = *v3;
                return true;
            }

            return false;
        }

        bool WriteOutConfig(ComponentConfig* outBaseConfig) const override
        {
            if (auto v1 = azrtti_cast<HydraConfigV1*>(outBaseConfig))
            {
                v1->m_numHeads = m_config.m_numHeads;

                // v2 is based on v1
                if (auto v2 = azrtti_cast<HydraConfigV2*>(v1))
                {
                    v2->m_numArms = m_config.m_numHeads * m_config.m_numArmsPerHead;
                }

                return true;
            }
            
            if (auto v3 = azrtti_cast<HydraConfigV3*>(outBaseConfig))
            {
                *v3 = m_config;
                return true;
            }

            return false;
        }
    };

    TEST_F(Components, SetConfigurationV1_Succeeds)
    {
        HydraConfigV1 config;
        config.m_numHeads = 3;

        HydraComponent component;

        EXPECT_TRUE(component.SetConfiguration(config));
        EXPECT_EQ(component.m_config.m_numHeads, 3);
    }

    TEST_F(Components, GetConfigurationV1_Succeeds)
    {
        HydraConfigV1 config;

        HydraComponent component;
        component.m_config.m_numHeads = 8;

        EXPECT_TRUE(component.GetConfiguration(config));
        EXPECT_EQ(config.m_numHeads, component.m_config.m_numHeads);
    }

    TEST_F(Components, SetConfigurationV2_Succeeds)
    {
        HydraConfigV2 config;
        config.m_numHeads = 4;
        config.m_numArms = 12;

        HydraComponent component;

        EXPECT_TRUE(component.SetConfiguration(config));
        EXPECT_EQ(component.m_config.m_numHeads, config.m_numHeads);
        EXPECT_EQ(component.m_config.m_numArmsPerHead, 3);
    }

    TEST_F(Components, GetConfigurationV2_Succeeds)
    {
        HydraConfigV2 config;

        HydraComponent component;
        component.m_config.m_numHeads = 12;
        component.m_config.m_numArmsPerHead = 1;

        EXPECT_TRUE(component.GetConfiguration(config));
        EXPECT_EQ(config.m_numHeads, component.m_config.m_numHeads);
        EXPECT_EQ(config.m_numArms, 12);
    }

    TEST_F(Components, SetConfigurationV3_Succeeds)
    {
        HydraConfigV3 config;
        config.m_numHeads = 2;
        config.m_numArmsPerHead = 4;

        HydraComponent component;

        EXPECT_TRUE(component.SetConfiguration(config));
        EXPECT_EQ(component.m_config.m_numHeads, config.m_numHeads);
        EXPECT_EQ(component.m_config.m_numArmsPerHead, config.m_numArmsPerHead);
    }

    TEST_F(Components, GetConfigurationV3_Succeeds)
    {
        HydraConfigV3 config;

        HydraComponent component;
        component.m_config.m_numHeads = 94;
        component.m_config.m_numArmsPerHead = 18;

        EXPECT_TRUE(component.GetConfiguration(config));
        EXPECT_EQ(config.m_numHeads, component.m_config.m_numHeads);
        EXPECT_EQ(config.m_numArmsPerHead, component.m_config.m_numArmsPerHead);
    }
}
