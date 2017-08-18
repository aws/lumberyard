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
#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Module/Module.h>
#include <AzCore/Memory/AllocationRecords.h>
#include "ModuleTestBus.h"

using namespace AZ;

namespace UnitTest
{
    class SystemComponentFromModule
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(SystemComponentFromModule, "{7CDDF71F-4D9E-41B0-8F82-4FFA86513809}")

        void Activate() override {}
        void Deactivate() override {}

        static void Reflect(AZ::ReflectContext* reflectContext)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext))
            {
                serializeContext->Class<SystemComponentFromModule>()->
                    SerializerForEmptyClass();
            }
        }
    };

    class StaticModule
        : public Module
        , public ModuleTestRequestBus::Handler
    {
    public:
        static bool s_loaded;
        static bool s_reflected;

        StaticModule()
        {
            s_loaded = true;
            ModuleTestRequestBus::Handler::BusConnect();
            m_descriptors.insert(m_descriptors.end(), {
                SystemComponentFromModule::CreateDescriptor(),
            });
        }

        ~StaticModule()
        {
            ModuleTestRequestBus::Handler::BusDisconnect();
            s_loaded = false;
        }

        //void Reflect(ReflectContext*) override
        //{
         //   s_reflected = true;
        //}

        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<SystemComponentFromModule>(),
            };
        }

        const char* GetModuleName() override
        {
            return "StaticModule";
        }
    };

    bool StaticModule::s_loaded = false;
    bool StaticModule::s_reflected = false;

    void AZCreateStaticModules(AZStd::vector<AZ::Module*>& modulesOut)
    {
        modulesOut.push_back(new UnitTest::StaticModule());
    }

    TEST(ApplicationModule, Test)
    {
        {
            ComponentApplication app;

            // Create application descriptor
            ComponentApplication::Descriptor appDesc;
            appDesc.m_memoryBlocksByteSize = 10 * 1024 * 1024;
            appDesc.m_recordingMode = Debug::AllocationRecords::RECORD_FULL;

            // AZCoreTestDLL will load as a dynamic module
            appDesc.m_modules.push_back();
            ComponentApplication::Descriptor::DynamicModuleDescriptor& dynamicModuleDescriptor = appDesc.m_modules.back();
            dynamicModuleDescriptor.m_dynamicLibraryPath = "AZCoreTestDLL";

            // StaticModule will load via AZCreateStaticModule(...)

            // Start up application
            ComponentApplication::StartupParameters startupParams;
            startupParams.m_createStaticModulesCallback = AZCreateStaticModules;
            Entity* systemEntity = app.Create(appDesc, startupParams);
            AZ_TEST_ASSERT(systemEntity);
            systemEntity->Init();
            systemEntity->Activate();

            // Check that StaticModule was loaded and reflected
            AZ_TEST_ASSERT(StaticModule::s_loaded);
            // AZ_TEST_ASSERT(StaticModule::s_reflected);

            { // Query both modules via the ModuleTestRequestBus
                EBusAggregateResults<const char*> moduleNames;
                EBUS_EVENT_RESULT(moduleNames, ModuleTestRequestBus, GetModuleName);

                AZ_TEST_ASSERT(moduleNames.values.size() == 2);
                bool foundStaticModule = false;
                bool foundDynamicModule = false;
                for (const char* moduleName : moduleNames.values)
                {
                    if (strcmp(moduleName, "DllModule") == 0)
                    {
                        foundDynamicModule = true;
                    }
                    if (strcmp(moduleName, "StaticModule") == 0)
                    {
                        foundStaticModule = true;
                    }
                }
                AZ_TEST_ASSERT(foundDynamicModule);
                AZ_TEST_ASSERT(foundStaticModule);
            }

            // Check that system component from module was added
            AZ_TEST_ASSERT(systemEntity->FindComponent<SystemComponentFromModule>());

            // shut down application (deletes Modules, unloads DLLs)
            app.Destroy();
        }

        AZ_TEST_ASSERT(!StaticModule::s_loaded);
    }
} // namespace UnitTest
