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

#include "RoadsAndRiversTestCommon.h"

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

    class RoadsAndRiversTestApp
        : public ::testing::Test
    {
    public:
        RoadsAndRiversTestApp()
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
                modules.emplace_back(new RoadsAndRivers::RoadsAndRiversModule);
            };

            m_systemEntity = m_application.Create(appDesc, appStartup);
            m_systemEntity->Init();
            m_systemEntity->Activate();

            m_application.RegisterComponentDescriptor(RoadsAndRiversTest::MockTransformComponent::CreateDescriptor());
            m_application.RegisterComponentDescriptor(RoadsAndRiversTest::MockSplineComponent::CreateDescriptor());
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
        RoadsAndRiversTest::MockGlobalEnvironment m_mocks;
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
        // Trivial test - Create an uninitialized / unactivated River Component
        AZ::Entity* riverEntity = RoadsAndRiversTest::CreateTestEntity<RoadsAndRivers::RiverComponent>(false, false);
        m_application.AddEntity(riverEntity);

        RoadsAndRivers::RiverComponent* riverComp = riverEntity->FindComponent<RoadsAndRivers::RiverComponent>();
        ASSERT_TRUE(riverComp != nullptr);
    }

    TEST_F(RoadsAndRiversTestApp, RoadsAndRivers_RoadComponent)
    {
        // Trivial test - Create an uninitialized / unactivated Road Component
        AZ::Entity* roadEntity = RoadsAndRiversTest::CreateTestEntity<RoadsAndRivers::RoadComponent>(false, false);
        m_application.AddEntity(roadEntity);

        RoadsAndRivers::RoadComponent* roadComp = roadEntity->FindComponent<RoadsAndRivers::RoadComponent>();
        ASSERT_TRUE(roadComp != nullptr);
    }

    TEST_F(RoadsAndRiversTestApp, RoadsAndRivers_RiverRequestBus)
    {
        // Create a River Component and verify that the RiverRequestBus works by querying the default water surface plane and validating the result
        AZ::Entity* riverEntity = RoadsAndRiversTest::CreateTestEntity<RoadsAndRivers::RiverComponent>();

        AZ::Plane surfacePlane;
        RoadsAndRivers::RiverRequestBus::EventResult(surfacePlane, riverEntity->GetId(), &RoadsAndRivers::RiverRequestBus::Events::GetWaterSurfacePlane);
        AZ::Vector4 planeEquation = surfacePlane.GetPlaneEquationCoefficients();
        AZ::Vector4 expectedPlane(0.0f, 0.0f, 1.0f, 0.0f);
        ASSERT_TRUE(planeEquation.IsClose(expectedPlane));
    }

    TEST_F(RoadsAndRiversTestApp, RoadsAndRivers_RoadsAndRiversGeometryRequestsBus)
    {
        // Create a River Component and verify that the RoadsAndRiversGeometryRequestsBus works by querying for the quads generated off of a specific spline.

        // Create a river component that's initialized, but not activated, so that we can add a spline onto it prior to activation.
        AZ::Entity* riverEntity = RoadsAndRiversTest::CreateTestEntity<RoadsAndRivers::RiverComponent>(true, false);

        RoadsAndRiversTest::MockSplineComponent* spline = riverEntity->FindComponent<RoadsAndRiversTest::MockSplineComponent>();
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
