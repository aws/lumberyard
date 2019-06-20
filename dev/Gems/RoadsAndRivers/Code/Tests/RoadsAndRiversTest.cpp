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

#include "StdAfx.h"

#include <AzTest/AzTest.h>
#include <Mocks/ITimerMock.h>
#include <Mocks/ICryPakMock.h>
#include <Mocks/IConsoleMock.h>
#include <Mocks/ISystemMock.h>

#include <Tests/TestTypes.h>

#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Math/Random.h>
#include <AzCore/Math/Spline.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/Memory/SystemAllocator.h>

#include <LmbrCentral/Shape/SplineComponentBus.h>

#include "RoadRiverCommon.h"
#include "RoadsAndRiversModule.h"
#include "RoadComponent.h"
#include "RiverComponent.h"

namespace UnitTest
{
    class WidthInterpolator
        : public AllocatorsFixture
    {
    public:

        void EdgeCases()
        {
            const float tolerance = 1e-4;
            {
                RoadsAndRivers::RoadWidthInterpolator m_interpolator;
                // empty interpolator returns 0.0f
                EXPECT_NEAR(m_interpolator.GetWidth(0.0f), 0.0f, tolerance);
            }

            {
                RoadsAndRivers::RoadWidthInterpolator m_interpolator;
                const float width = 2.0f;
                m_interpolator.InsertDistanceWidthKeyFrame(1.0f, width);

                // interpolator with 1 key frame always returns this keyframe
                EXPECT_NEAR(m_interpolator.GetWidth(0.0f), width, tolerance);
                EXPECT_NEAR(m_interpolator.GetWidth(1.0f), width, tolerance);
                EXPECT_NEAR(m_interpolator.GetWidth(2.0f), width, tolerance);
            }

            {
                RoadsAndRivers::RoadWidthInterpolator m_interpolator;

                m_interpolator.InsertDistanceWidthKeyFrame(1.0f, 2.0f);
                m_interpolator.InsertDistanceWidthKeyFrame(2.0f, 10.0f);

                // distance more than edge points yields edge points
                EXPECT_NEAR(m_interpolator.GetWidth(0.0f), 2.0f, tolerance);
                EXPECT_NEAR(m_interpolator.GetWidth(3.0f), 10.0f, tolerance);
            }
        }

        void Interpolation()
        {
            // Inserting sorted keys
            {
                RoadsAndRivers::RoadWidthInterpolator m_interpolator;

                m_interpolator.InsertDistanceWidthKeyFrame(1.0f, 1.0f);
                m_interpolator.InsertDistanceWidthKeyFrame(2.0f, 10.0f);
                m_interpolator.InsertDistanceWidthKeyFrame(3.0f, 5.0f);

                auto res = m_interpolator.GetWidth(1.5f);
                EXPECT_GT(res, 1.0f);
                EXPECT_LT(res, 10.0f);

                res = m_interpolator.GetWidth(2.5f);
                EXPECT_LT(res, 10.0f);
                EXPECT_GT(res, 5.0f);

                EXPECT_NEAR(m_interpolator.GetWidth(2.0f), 10.0f, 1e-4);
            }

            // Inserting unsorted keys
            {
                RoadsAndRivers::RoadWidthInterpolator m_interpolator;

                m_interpolator.InsertDistanceWidthKeyFrame(3.0f, 5.0f);
                m_interpolator.InsertDistanceWidthKeyFrame(1.0f, 1.0f);
                m_interpolator.InsertDistanceWidthKeyFrame(2.0f, 10.0f);

                auto res = m_interpolator.GetWidth(1.5f);
                EXPECT_GT(res, 1.0f);
                EXPECT_LT(res, 10.0f);

                res = m_interpolator.GetWidth(2.5f);
                EXPECT_LT(res, 10.0f);
                EXPECT_GT(res, 5.0f);

                EXPECT_NEAR(m_interpolator.GetWidth(2.0f), 10.0f, 1e-4);
            }
        }
    };

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

    class RoadsAndRiversTestApp
        : public ::testing::Test
    {
    public:
        RoadsAndRiversTestApp()
            : m_application()
            , m_systemEntity(nullptr)
        {
        }

        class MockTransformComponent
            : public AZ::Component
        {
        public:
            AZ_COMPONENT(MockTransformComponent, "{8F4C932A-6BAD-464B-AFB3-87CC8EA31FB5}", AZ::Component);
            void Activate() override {}
            void Deactivate() override {}
            static void Reflect(AZ::ReflectContext* reflect) { AZ_UNUSED(reflect); }
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
            {
                provided.push_back(AZ_CRC("TransformService", 0x8ee22c50));
            }
        };

        class MockSplineComponent
            : public AZ::Component
            , private LmbrCentral::SplineComponentRequestBus::Handler
        {
        public:
            AZ_COMPONENT(MockSplineComponent, "{F0905297-1E24-4044-BFDA-BDE3583F1E57}", AZ::Component);
            void Activate() override 
            {
                LmbrCentral::SplineComponentRequestBus::Handler::BusConnect(GetEntityId());
            }
            void Deactivate() override 
            {
                LmbrCentral::SplineComponentRequestBus::Handler::BusDisconnect();
            }
            static void Reflect(AZ::ReflectContext* reflect) { AZ_UNUSED(reflect); }
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
            {
                provided.push_back(AZ_CRC("SplineService", 0x2b674d3c));
                provided.push_back(AZ_CRC("VertexContainerService", 0x22cf8e10));
            }

            MockSplineComponent()
            {
                m_spline = AZStd::make_shared<AZ::LinearSpline>();
            }

            AZ::SplinePtr GetSpline() override { return m_spline; }
            void ChangeSplineType(AZ::u64 /*splineType*/) override {}
            void SetClosed(bool /*closed*/) override {}

            // SplineComponentRequestBus/VertexContainerInterface
            bool GetVertex(size_t index, AZ::Vector3& vertex) const override { return false; }
            void AddVertex(const AZ::Vector3& vertex) override { m_spline->m_vertexContainer.AddVertex(vertex); }

            bool UpdateVertex(size_t index, const AZ::Vector3& vertex) override { return false; }
            bool InsertVertex(size_t index, const AZ::Vector3& vertex) override { return false; }
            bool RemoveVertex(size_t index) override { return false;  }
            void SetVertices(const AZStd::vector<AZ::Vector3>& vertices) override {  }
            void ClearVertices() override { m_spline->m_vertexContainer.Clear(); }
            
            size_t Size() const override { return m_spline->m_vertexContainer.Size(); }
            bool Empty() const override { return m_spline->m_vertexContainer.Empty(); }

        private:
            AZ::SplinePtr m_spline;
        };

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
                modules.emplace_back(new RoadsAndRivers::RoadsAndRiversModule);
            };

            m_systemEntity = m_application.Create(appDesc, appStartup);
            m_systemEntity->Init();
            m_systemEntity->Activate();

            m_application.RegisterComponentDescriptor(MockTransformComponent::CreateDescriptor());
            m_application.RegisterComponentDescriptor(MockSplineComponent::CreateDescriptor());
            m_application.RegisterComponentDescriptor(RoadsAndRivers::RoadComponent::CreateDescriptor());
            m_application.RegisterComponentDescriptor(RoadsAndRivers::RiverComponent::CreateDescriptor());
        }

        void TearDown() override
        {
            delete m_systemEntity;
            m_application.Destroy();
        }

        AZ::ComponentApplication m_application;
        AZ::Entity* m_systemEntity;
        MockGlobalEnvironment m_mocks;
    };

    TEST_F(WidthInterpolator, EdgeCases)
    {
        EdgeCases();
    }

    TEST_F(WidthInterpolator, Interpolation)
    {
        Interpolation();
    }

    TEST_F(RoadsAndRiversTestApp, RoadsAndRivers_RiverComponent)
    {
        AZ::Entity* riverEntity = aznew AZ::Entity("river_entity");
        ASSERT_TRUE(riverEntity != nullptr);
        riverEntity->CreateComponent<MockTransformComponent>();
        riverEntity->CreateComponent<MockSplineComponent>();
        riverEntity->CreateComponent<RoadsAndRivers::RiverComponent>();
        m_application.AddEntity(riverEntity);

        RoadsAndRivers::RiverComponent* riverComp = riverEntity->FindComponent<RoadsAndRivers::RiverComponent>();
        ASSERT_TRUE(riverComp != nullptr);
    }

    TEST_F(RoadsAndRiversTestApp, RoadsAndRivers_RoadComponent)
    {
        AZ::Entity* roadEntity = aznew AZ::Entity("road_entity");
        ASSERT_TRUE(roadEntity != nullptr);
        roadEntity->CreateComponent<MockTransformComponent>();
        roadEntity->CreateComponent<MockSplineComponent>();
        roadEntity->CreateComponent<RoadsAndRivers::RoadComponent>();
        m_application.AddEntity(roadEntity);

        RoadsAndRivers::RoadComponent* roadComp = roadEntity->FindComponent<RoadsAndRivers::RoadComponent>();
        ASSERT_TRUE(roadComp != nullptr);
    }

    TEST_F(RoadsAndRiversTestApp, RoadsAndRivers_RiverRequestBus)
    {
        AZ::Entity* riverEntity = aznew AZ::Entity("activated_river_entity");
        ASSERT_TRUE(riverEntity != nullptr);
        riverEntity->CreateComponent<MockTransformComponent>();
        riverEntity->CreateComponent<MockSplineComponent>();
        riverEntity->CreateComponent<RoadsAndRivers::RiverComponent>();

        riverEntity->Init();
        riverEntity->Activate();

        AZ::Plane surfacePlane;
        RoadsAndRivers::RiverRequestBus::EventResult(surfacePlane, riverEntity->GetId(), &RoadsAndRivers::RiverRequestBus::Events::GetWaterSurfacePlane);
        AZ::Vector4 planeEquation = surfacePlane.GetPlaneEquationCoefficients();
        AZ::Vector4 expectedPlane(0.0f, 0.0f, 1.0f, 0.0f);
        ASSERT_TRUE(planeEquation.IsClose(expectedPlane));
    }

    TEST_F(RoadsAndRiversTestApp, RoadsAndRivers_RoadsAndRiversGeometryRequestsBus)
    {
        AZ::Entity* riverEntity = aznew AZ::Entity("activated_river_entity");
        ASSERT_TRUE(riverEntity != nullptr);
        riverEntity->CreateComponent<MockTransformComponent>();
        riverEntity->CreateComponent<MockSplineComponent>();
        riverEntity->CreateComponent<RoadsAndRivers::RiverComponent>();

        riverEntity->Init();

        MockSplineComponent* spline = riverEntity->FindComponent<MockSplineComponent>();
        spline->AddVertex(AZ::Vector3(0.0f, 0.0f, 0.0f));
        spline->AddVertex(AZ::Vector3(5.0f, 0.0f, 0.0f));
        spline->AddVertex(AZ::Vector3(10.0f, 0.0f, 0.0f));
        spline->AddVertex(AZ::Vector3(15.0f, 0.0f, 0.0f));

        riverEntity->Activate();

        AZStd::vector<AZ::Vector3> quadVertices;

        quadVertices.clear();
        RoadsAndRivers::RoadsAndRiversGeometryRequestsBus::EventResult(quadVertices, riverEntity->GetId(), &RoadsAndRivers::RoadsAndRiversGeometryRequestsBus::Events::GetQuadVertices);
        ASSERT_TRUE(quadVertices.size() == 32);
    }

    AZ_UNIT_TEST_HOOK();
}
