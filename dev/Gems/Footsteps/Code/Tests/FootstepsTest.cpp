/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates, or 
* a third party where indicated.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,  
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  
*
*/

#include <AzTest/AzTest.h>
#include <gmock/gmock.h>
#include <ISystem.h>
#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Component/Entity.h>
#include "FootstepComponent.h"

#include <AzFramework/Terrain/TerrainDataRequestBus.h>

namespace Footsteps
{
    using namespace ::testing;

    struct MockGlobalEnvironment
    {
        MockGlobalEnvironment()
        {
            m_stubEnv.pMaterialEffects = nullptr;
            m_stubEnv.p3DEngine = nullptr;
            gEnv = &m_stubEnv;
        }

        ~MockGlobalEnvironment()
        {
            gEnv = nullptr;
        }

    private:
        SSystemGlobalEnvironment m_stubEnv;
    };

    class FootstepsTest
        : public Test
    {
    public:
        FootstepsTest()
            : m_application()
            , m_systemEntity(nullptr)
        {
        }

    protected:
        void SetUp() override
        {
            ::testing::Test::SetUp();

            AZ::ComponentApplication::Descriptor appDesc;
            appDesc.m_memoryBlocksByteSize = 10 * 1024 * 1024;
            appDesc.m_recordingMode = AZ::Debug::AllocationRecords::RECORD_FULL;
            appDesc.m_stackRecordLevels = 20;

            AZ::ComponentApplication::StartupParameters appStartup;
            m_systemEntity = m_application.Create(appDesc, appStartup);
            m_application.RegisterComponentDescriptor(FootstepComponent::CreateDescriptor());
            m_systemEntity->Init();
            m_systemEntity->Activate();
        }

        void TearDown() override
        {
            m_application.Destroy();
        }

        AZ::Entity* m_systemEntity;
        AZ::ComponentApplication m_application;
        MockGlobalEnvironment m_globalEnvironment;
    };

    class MockTerrainHandler
        : public AzFramework::Terrain::TerrainDataRequestBus::Handler
    {
    public:
        MockTerrainHandler()
        {
            AzFramework::Terrain::TerrainDataRequestBus::Handler::BusConnect();
        }
        ~MockTerrainHandler() override
        {
            AzFramework::Terrain::TerrainDataRequestBus::Handler::BusDisconnect();
        }

        MOCK_CONST_METHOD0(GetTerrainGridResolution, AZ::Vector2());
        MOCK_CONST_METHOD0(GetTerrainAabb, AZ::Aabb());
        MOCK_CONST_METHOD3(GetHeight, float(AZ::Vector3, AzFramework::Terrain::TerrainDataRequests::Sampler, bool*));
        MOCK_CONST_METHOD4(GetHeightFromFloats, float(float, float, Sampler, bool*));
        MOCK_CONST_METHOD3(GetMaxSurfaceWeight, AzFramework::SurfaceData::SurfaceTagWeight(AZ::Vector3, Sampler, bool*));
        MOCK_CONST_METHOD4(GetMaxSurfaceWeightFromFloats, AzFramework::SurfaceData::SurfaceTagWeight(float, float, Sampler, bool*));
        MOCK_CONST_METHOD3(GetMaxSurfaceName, const char*(AZ::Vector3, Sampler, bool*));
        MOCK_CONST_METHOD3(GetIsHoleFromFloats, bool(float, float, AzFramework::Terrain::TerrainDataRequests::Sampler));
        MOCK_CONST_METHOD3(GetNormal, AZ::Vector3(AZ::Vector3, AzFramework::Terrain::TerrainDataRequests::Sampler, bool*));
        MOCK_CONST_METHOD4(GetNormalFromFloats, AZ::Vector3(float, float, AzFramework::Terrain::TerrainDataRequests::Sampler, bool*));
    };

    TEST_F(FootstepsTest, Footsteps_Component_FT)
    {
        // given an entity
        AZ::Entity* entity = aznew AZ::Entity("footstep_entity");
        ASSERT_TRUE(entity != nullptr);

        // when a footstep component is added
        entity->CreateComponent<FootstepComponent>();
        m_application.AddEntity(entity);

        // the footstep component is added succesfully
        FootstepComponent* component = entity->FindComponent<FootstepComponent>();
        ASSERT_TRUE(component != nullptr);
    }

    TEST_F(FootstepsTest, Footsteps_Over_Terrain_Hole_Does_Not_Crash_FT)
    {
        // given an entity with a footstep component
        AZ::Entity* entity = aznew AZ::Entity("footstep_entity");
        ASSERT_TRUE(entity != nullptr);

        FootstepComponent* component = entity->CreateComponent<FootstepComponent>();
        ASSERT_TRUE(component != nullptr);

        entity->Init();
        entity->Activate();

        EMotionFX::Integration::MotionEvent motionEvent;
        motionEvent.m_parameter = "footstep";     ///< Optional string parameter (used for fxlib)
        motionEvent.m_actorInstance = nullptr;    ///< Pointer to the actor instance on which the event is playing
        motionEvent.m_motionInstance = nullptr;   ///< Pointer to the motion instance from which the event was fired
        motionEvent.m_time = 0.f;                 ///< Time value of the event, in seconds
        motionEvent.m_eventType = 0;              ///< Type Id of the event. m_eventTypeName stores the string representation.
        motionEvent.m_eventTypeName = "LeftFoot"; ///< Event type in string form - one of the default names for FootstepComponent
        motionEvent.m_globalWeight = 0.f;         ///< Global weight of the event
        motionEvent.m_localWeight = 0.f;          ///< Local weight of the event
        motionEvent.m_isEventStart = false;

        NiceMock<MockTerrainHandler> terrainHandler;

        ON_CALL(terrainHandler, GetMaxSurfaceName(_,_,_))
            .WillByDefault(Return(nullptr));

        // when a motion event is handled by the footstep component 
        EMotionFX::Integration::ActorNotificationBus::Event(entity->GetId(), &EMotionFX::Integration::ActorNotificationBus::Events::OnMotionEvent, motionEvent);
        // (implicit) the footstep component does not cause a crash
    }
}

AZ_UNIT_TEST_HOOK();
