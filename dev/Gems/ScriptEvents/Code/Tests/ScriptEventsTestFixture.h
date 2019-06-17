/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright EntityRef license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#include "ScriptEventsTestApplication.h"

#include <AzTest/AzTest.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Driller/Driller.h>
#include <AzCore/Memory/MemoryDriller.h>
#include <AzCore/Memory/MemoryComponent.h>
#include <AzCore/Asset/AssetManagerComponent.h>
#include <AzFramework/IO/LocalFileIO.h>

#include <ScriptEvents/ScriptEventsGem.h>

#include <ScriptEvents/Internal/VersionedProperty.h>
#include <ScriptEvents/ScriptEventParameter.h>
#include <ScriptEvents/ScriptEventMethod.h>
#include <ScriptEvents/ScriptEventsAsset.h>

#include "ScriptEventTestUtilities.h"
#include <AzCore/Math/MathReflection.h>

namespace ScriptEventsTests
{
    class ScriptEventsTestFixture
        : public ::testing::Test
    {
        static AZ::Debug::DrillerManager* s_drillerManager;
        static const bool s_enableMemoryLeakChecking;

        static ScriptEventsTests::Application* GetApplication();

    protected:

        static ScriptEventsTests::Application* s_application;

        static void SetUpTestCase()
        {
            if (!s_drillerManager)
            {
                s_drillerManager = AZ::Debug::DrillerManager::Create();
                // Memory driller is responsible for tracking allocations. 
                // Tracking type and overhead is determined by app configuration.
                s_drillerManager->Register(aznew AZ::Debug::MemoryDriller);
            }

            AZ::SystemAllocator::Descriptor systemAllocatorDesc;
            systemAllocatorDesc.m_allocationRecords = s_enableMemoryLeakChecking;
            systemAllocatorDesc.m_stackRecordLevels = 12;
            AZ::AllocatorInstance<AZ::SystemAllocator>::Create(systemAllocatorDesc);

            AZ::Debug::AllocationRecords* records = AZ::AllocatorInstance<AZ::SystemAllocator>::Get().GetRecords();
            if (records && s_enableMemoryLeakChecking)
            {
                records->SetMode(AZ::Debug::AllocationRecords::RECORD_FULL);
            }

            AZ::ComponentApplication::StartupParameters appStartup;

            appStartup.m_createStaticModulesCallback =
                [](AZStd::vector<AZ::Module*>& modules)
            {
                modules.emplace_back(new ScriptEvents::Module);
            };

            AZ::ComponentApplication::Descriptor appDesc;
            appDesc.m_enableDrilling = false; // We'll manage our own driller in these tests
            appDesc.m_useExistingAllocator = true; // Use the SystemAllocator we own in this test.

            s_application = aznew ScriptEventsTests::Application();
            s_application->Start(appDesc, appStartup);

            AZ::TickBus::AllowFunctionQueuing(true);

            AZ::SerializeContext* serializeContext = s_application->GetSerializeContext();
            serializeContext->RegisterGenericType<AZStd::string>();
            serializeContext->RegisterGenericType<AZStd::any>();

            AZ::BehaviorContext* behaviorContext = s_application->GetBehaviorContext();

            Utilities::Reflect(behaviorContext);

            AZ::Entity* systemEntity = s_application->FindEntity(AZ::SystemEntityId);
            AZ_Assert(systemEntity, "SystemEntity must exist");
            systemEntity->FindComponent<ScriptEvents::SystemComponent>()->RegisterAssetHandler();
        }

        static void TearDownTestCase()
        {
            AZ::Data::AssetManager::Instance().DispatchEvents();

            AZ::Entity* systemEntity = s_application->FindEntity(AZ::SystemEntityId);
            AZ_Assert(systemEntity, "SystemEntity must exist");
            systemEntity->FindComponent<ScriptEvents::SystemComponent>()->UnregisterAssetHandler();

            s_application->Stop();
            delete s_application;
            s_application = nullptr;

            // This allows us to print the raw dump of allocations that have not been freed, but is not required. 
            // Leaks will be reported when we destroy the allocator.
            const bool printRecords = false;
            AZ::Debug::AllocationRecords* records = AZ::AllocatorInstance<AZ::SystemAllocator>::Get().GetRecords();
            if (records && printRecords)
            {
                records->EnumerateAllocations(AZ::Debug::PrintAllocationsCB(false));
            }

            AZ::AllocatorInstance<AZ::SystemAllocator>::Destroy();

            if (s_drillerManager)
            {
                AZ::Debug::DrillerManager::Destroy(s_drillerManager);
                s_drillerManager = nullptr;
            }
        }

        void SetUp() override
        {
            m_serializeContext = s_application->GetSerializeContext();
            m_behaviorContext = s_application->GetBehaviorContext();

            if (!AZ::IO::FileIOBase::GetInstance())
            {
                m_fileIO.reset(aznew AZ::IO::LocalFileIO());
                AZ::IO::FileIOBase::SetInstance(m_fileIO.get());
            }
            AZ_Assert(AZ::IO::FileIOBase::GetInstance(), "File IO was not properly installed");
        }

        void TearDown() override
        {
            AZ::IO::FileIOBase::SetInstance(nullptr);
            m_fileIO = nullptr;
        }

        AZStd::unique_ptr<AZ::IO::FileIOBase> m_fileIO;
        AZ::SerializeContext* m_serializeContext;
        AZ::BehaviorContext* m_behaviorContext;
    };

}

