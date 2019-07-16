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
#include "SurfaceData_precompiled.h"

#include <AzTest/AzTest.h>
#include <Mocks/ITimerMock.h>
#include <Mocks/ICryPakMock.h>
#include <Mocks/IConsoleMock.h>
#include <Mocks/ISystemMock.h>

#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Math/Random.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Script/ScriptContext.h>
#include <AzCore/std/chrono/clocks.h>
#include <SurfaceDataSystemComponent.h>
#include <SurfaceDataModule.h>
#include <SurfaceData/SurfaceDataProviderRequestBus.h>
#include <SurfaceData/SurfaceTag.h>
#include <SurfaceData/Utility/SurfaceDataUtility.h>

struct MockGlobalEnvironment
{
    MockGlobalEnvironment()
    {
        m_stubEnv.pTimer = &m_stubTimer;
        m_stubEnv.pCryPak = &m_stubPak;
        m_stubEnv.pConsole = &m_stubConsole;
        m_stubEnv.pSystem = &m_stubSystem;
        m_stubEnv.p3DEngine = nullptr;
        gEnv = &m_stubEnv;
    }

    ~MockGlobalEnvironment()
    {
        gEnv = nullptr;
    }

private:
    SSystemGlobalEnvironment m_stubEnv;
    testing::NiceMock<TimerMock> m_stubTimer;
    testing::NiceMock<CryPakMock> m_stubPak;
    testing::NiceMock<ConsoleMock> m_stubConsole;
    testing::NiceMock<SystemMock> m_stubSystem;
};

TEST(SurfaceDataTest, ComponentsWithComponentApplication)
{
    AZ::ComponentApplication::Descriptor appDesc;
    appDesc.m_memoryBlocksByteSize = 10 * 1024 * 1024;
    appDesc.m_recordingMode = AZ::Debug::AllocationRecords::RECORD_FULL;
    appDesc.m_stackRecordLevels = 20;

    MockGlobalEnvironment mocks;

    AZ::ComponentApplication app;
    AZ::Entity* systemEntity = app.Create(appDesc);
    ASSERT_TRUE(systemEntity != nullptr);
    app.RegisterComponentDescriptor(SurfaceData::SurfaceDataSystemComponent::CreateDescriptor());
    systemEntity->CreateComponent<SurfaceData::SurfaceDataSystemComponent>();

    systemEntity->Init();
    systemEntity->Activate();

    app.Destroy();
    ASSERT_TRUE(true);
}

class SurfaceDataTestApp
    : public ::testing::Test
{
public:
    SurfaceDataTestApp()
        : m_application()
        , m_systemEntity(nullptr)
    {
    }

    void SetUp() override
    {
        AZ::ComponentApplication::Descriptor appDesc;
        appDesc.m_memoryBlocksByteSize = 10 * 1024 * 1024;
        appDesc.m_recordingMode = AZ::Debug::AllocationRecords::RECORD_FULL;
        appDesc.m_stackRecordLevels = 20;

        AZ::ComponentApplication::StartupParameters appStartup;
        appStartup.m_createStaticModulesCallback =
            [](AZStd::vector<AZ::Module*>& modules)
        {
            modules.emplace_back(new SurfaceData::SurfaceDataModule);
        };

        m_systemEntity = m_application.Create(appDesc, appStartup);
        m_systemEntity->Init();
        m_systemEntity->Activate();
    }

    void TearDown() override
    {
        m_application.Destroy();
    }

    AZ::ComponentApplication m_application;
    AZ::Entity* m_systemEntity;
    MockGlobalEnvironment m_mocks;
};

TEST_F(SurfaceDataTestApp, SurfaceData_TestRegisteredTags)
{
    AZStd::vector<AZStd::pair<AZ::u32, AZStd::string>> registeredTags = SurfaceData::SurfaceTag::GetRegisteredTags();

    for (const auto& searchTerm : SurfaceData::Constants::s_allTagNames)
    {
        ASSERT_TRUE(AZStd::find_if(registeredTags.begin(), registeredTags.end(), [searchTerm](decltype(registeredTags)::value_type pair)
        {
            return pair.second == searchTerm;
        }));
    }
}


TEST_F(SurfaceDataTestApp, SurfaceData_TestGetQuadListRayIntersection)
{
    AZStd::vector<AZ::Vector3> quads;
    AZ::Vector3 outPosition;
    AZ::Vector3 outNormal;
    bool result;

    struct RayTest
    {
        // Input quad
        AZ::Vector3 quadVertices[4];
        // Input ray
        AZ::Vector3 rayOrigin;
        AZ::Vector3 rayDirection;
        float       rayMaxRange;
        // Expected outputs
        bool        expectedResult;
        AZ::Vector3 expectedOutPosition;
        AZ::Vector3 expectedOutNormal;
    };

    RayTest tests[] = 
    {
        // Ray intersects quad
        {{AZ::Vector3(0.0f, 0.0f, 0.0f), AZ::Vector3(100.0f, 0.0f, 0.0f), AZ::Vector3(0.0f, 100.0f, 0.0f), AZ::Vector3(100.0f, 100.0f, 0.0f)},
            AZ::Vector3(50.0f, 50.0f, 10.0f), AZ::Vector3(0.0f, 0.0f, -1.0f), 20.0f,  true, AZ::Vector3(50.0f, 50.0f, 0.0f), AZ::Vector3(0.0f, 0.0f, 1.0f)},

        // Ray not long enough to intersect
        {{AZ::Vector3(0.0f, 0.0f, 0.0f), AZ::Vector3(100.0f, 0.0f, 0.0f), AZ::Vector3(0.0f, 100.0f, 0.0f), AZ::Vector3(100.0f, 100.0f, 0.0f)},
            AZ::Vector3(50.0f, 50.0f, 10.0f), AZ::Vector3(0.0f, 0.0f, -1.0f),  5.0f, false, AZ::Vector3( 0.0f,  0.0f, 0.0f), AZ::Vector3(0.0f, 0.0f, 0.0f)},

        // 0-length ray on quad surface
        {{AZ::Vector3(0.0f, 0.0f, 0.0f), AZ::Vector3(100.0f, 0.0f, 0.0f), AZ::Vector3(0.0f, 100.0f, 0.0f), AZ::Vector3(100.0f, 100.0f, 0.0f)},
            AZ::Vector3(50.0f, 50.0f,  0.0f), AZ::Vector3(0.0f, 0.0f, -1.0f),  0.0f,  true, AZ::Vector3(50.0f, 50.0f, 0.0f), AZ::Vector3(0.0f, 0.0f, 1.0f)},
        
        // ray in wrong direction
        {{AZ::Vector3(0.0f, 0.0f, 0.0f), AZ::Vector3(100.0f, 0.0f, 0.0f), AZ::Vector3(0.0f, 100.0f, 0.0f), AZ::Vector3(100.0f, 100.0f, 0.0f)},
            AZ::Vector3(50.0f, 50.0f, 10.0f), AZ::Vector3(0.0f, 0.0f,  1.0f), 20.0f, false, AZ::Vector3( 0.0f,  0.0f, 0.0f), AZ::Vector3(0.0f, 0.0f, 0.0f)},

        // The following tests are specific cases that worked differently when the implementation of GetQuadRayListIntersection used AZ::Intersect::IntersectRayQuad
        // instead of IntersectSegmentTriangle.  We'll keep them here both as good non-trivial tests and to ensure that if anyone ever tries to change the implmentation,
        // they can easily validate whether or not IntersectRayQuad will produce the same results.

        // ray passes IntersectSegmentTriangle but fails IntersectRayQuad
        {{AZ::Vector3(499.553, 688.946, 48.788), AZ::Vector3(483.758, 698.655, 48.788), AZ::Vector3(498.463, 687.181, 48.916), AZ::Vector3(482.701, 696.942, 48.916)},
            AZ::Vector3(485.600, 695.200, 49.501), AZ::Vector3(-0.000f, -0.000f, -1.000f), 18.494f, true, AZ::Vector3(485.600, 695.200, 48.913), AZ::Vector3(0.033, 0.053, 0.998)},

        // ray fails IntersectSegmentTriangle but passes IntersectRayQuad
        // IntersectRayQuad hits with the following position / normal:  AZ::Vector3(480.000, 688.800, 49.295), AZ::Vector3(0.020, 0.032, 0.999)
        {{AZ::Vector3(495.245, 681.984, 49.218), AZ::Vector3(479.450, 691.692, 49.218), AZ::Vector3(494.205, 680.282, 49.292), AZ::Vector3(478.356, 689.902, 49.292)},
            AZ::Vector3(480.000, 688.800, 49.501), AZ::Vector3(-0.000, -0.000, -1.000), 18.494f, false, AZ::Vector3(0.0f,  0.0f, 0.0f), AZ::Vector3(0.0f, 0.0f, 0.0f)},

        // ray passes IntersectSegmentTriangle and IntersectRayQuad, but hits at different positions
        // IntersectRayQuad hits with the following position / normal:  AZ::Vector3(498.400, 700.000, 48.073), AZ::Vector3(0.046, 0.085, 0.995)
        {{AZ::Vector3(504.909, 698.078, 47.913), AZ::Vector3(488.641, 706.971, 47.913), AZ::Vector3(503.867, 696.206, 48.121), AZ::Vector3(487.733, 705.341, 48.121)},
            AZ::Vector3(498.400, 700.000, 49.501), AZ::Vector3(-0.000f, -0.000f, -1.000f), 53.584f, true, AZ::Vector3(498.400, 700.000, 48.062), AZ::Vector3(0.048, 0.084, 0.995)},

        // ray passes IntersectSegmentTriangle and IntersectRayQuad, but hits at different normals
        // IntersectRayQuad hits with the following position / normal:  AZ::Vector3(492.800, 703.200, 48.059), AZ::Vector3(0.046, 0.085, 0.995) 
        {{AZ::Vector3(504.909, 698.078, 47.913), AZ::Vector3(488.641, 706.971, 47.913), AZ::Vector3(503.867, 696.206, 48.121), AZ::Vector3(487.733, 705.341, 48.121)},
            AZ::Vector3(492.800, 703.200, 49.501), AZ::Vector3(-0.000f, -0.000f, -1.000f), 18.494f, true, AZ::Vector3(492.800, 703.200, 48.059), AZ::Vector3(0.053, 0.097, 0.994)},

    };

    for (const auto &test : tests)
    {
        quads.clear();
        outPosition.Set(0.0f, 0.0f, 0.0f);
        outNormal.Set(0.0f, 0.0f, 0.0f);

        quads.push_back(test.quadVertices[0]);
        quads.push_back(test.quadVertices[1]);
        quads.push_back(test.quadVertices[2]);
        quads.push_back(test.quadVertices[3]);

        result = SurfaceData::GetQuadListRayIntersection(quads, test.rayOrigin, test.rayDirection, test.rayMaxRange, outPosition, outNormal);
        ASSERT_TRUE(result == test.expectedResult);
        if (result || test.expectedResult)
        {
            ASSERT_TRUE(outPosition.IsClose(test.expectedOutPosition));
            ASSERT_TRUE(outNormal.IsClose(test.expectedOutNormal));
        }
    }
}


AZ_UNIT_TEST_HOOK();