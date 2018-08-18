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

#include "ScriptCanvasTestApplication.h"
#include "ScriptCanvasTestNodes.h"
#include "EntityRefTests.h"
#include "ScriptCanvasTestUtilities.h"

#include <AzTest/AzTest.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Driller/Driller.h>
#include <AzCore/Memory/MemoryDriller.h>
#include <ScriptCanvas/ScriptCanvasGem.h>
#include <AzCore/Memory/MemoryComponent.h>
#include <AzCore/Asset/AssetManagerComponent.h>
#include <ScriptCanvas/SystemComponent.h>
#include <Asset/RuntimeAssetSystemComponent.h>
#include <AzFramework/IO/LocalFileIO.h>

// disable test bodies to see if there's anything wrong with the system or test framework not related to ScriptCanvas testing
#define TEST_BODIES_ENABLED 1 // 1 = enabled by default, 0 = disabled by default

#define TEST_BODY_DEFAULT (TEST_BODIES_ENABLED)  
#define TEST_BODY_EXCEPTION (!TEST_BODY_DEFAULT)

#if (TEST_BODIES_ENABLED)
#define RETURN_IF_TEST_BODIES_ARE_DISABLED(isException) if (!isException) { ADD_FAILURE(); return; }
#else
#define RETURN_IF_TEST_BODIES_ARE_DISABLED(isException) if (isException) { return; }
#endif

#define SC_EXPECT_DOUBLE_EQ(candidate, reference) EXPECT_NEAR(candidate, reference, 0.001)
#define SC_EXPECT_FLOAT_EQ(candidate, reference) EXPECT_NEAR(candidate, reference, 0.001f)

namespace ScriptCanvasTests
{
    class ScriptCanvasTestFixture
        : public ::testing::Test
    {
        static AZ::Debug::DrillerManager* s_drillerManager;
        static const bool s_enableMemoryLeakChecking;

        static ScriptCanvasTests::Application* GetApplication();

    protected:

        static ScriptCanvasTests::Application* s_application;

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
                modules.emplace_back(new ScriptCanvas::ScriptCanvasModule);
            };

            AZ::ComponentApplication::Descriptor appDesc;
            appDesc.m_enableDrilling = false; // We'll manage our own driller in these tests
            appDesc.m_useExistingAllocator = true; // Use the SystemAllocator we own in this test.

            s_application = aznew ScriptCanvasTests::Application();
            AZ::Entity* systemEntity = s_application->Create(appDesc, appStartup);
            AZ::TickBus::AllowFunctionQueuing(true);

            s_application->RegisterComponentDescriptor(ScriptCanvasTests::TestComponent::CreateDescriptor());
            s_application->RegisterComponentDescriptor(TraceMessageComponent::CreateDescriptor());

            systemEntity->CreateComponent<AZ::MemoryComponent>();
            systemEntity->CreateComponent<AZ::AssetManagerComponent>();
            systemEntity->CreateComponent<ScriptCanvas::SystemComponent>();
            systemEntity->CreateComponent<ScriptCanvas::RuntimeAssetSystemComponent>();
            systemEntity->CreateComponent<TraceMessageComponent>();

            systemEntity->Init();
            systemEntity->Activate();
        }

        static void TearDownTestCase()
        {
            s_application->Destroy();
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

            TestNodes::TestResult::Reflect(m_serializeContext);
            TestNodes::TestResult::Reflect(m_behaviorContext);
        }

        void TearDown() override
        {
            m_serializeContext->EnableRemoveReflection();
            TestNodes::TestResult::Reflect(m_serializeContext);
            m_serializeContext->DisableRemoveReflection();
            m_behaviorContext->EnableRemoveReflection();
            TestNodes::TestResult::Reflect(m_behaviorContext);
            m_behaviorContext->DisableRemoveReflection();
            AZ::IO::FileIOBase::SetInstance(nullptr);
            m_fileIO = nullptr;
        }

        AZStd::unique_ptr<AZ::IO::FileIOBase> m_fileIO;
        AZ::SerializeContext* m_serializeContext;
        AZ::BehaviorContext* m_behaviorContext;
        UnitTestEntityContext m_entityContext;
    };

    class AsyncScriptCanvasTestFixture
        : public ScriptCanvasTestFixture
    {

    public:
        static void SetUpTestCase()
        {
            AsyncScriptCanvasTestFixture::m_asyncOperationActive = false;
            ScriptCanvasTestFixture::SetUpTestCase();
        }

        static AZStd::atomic_bool m_asyncOperationActive;
    };
}