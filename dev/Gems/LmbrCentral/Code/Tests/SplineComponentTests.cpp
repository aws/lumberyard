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

#include "LmbrCentral_precompiled.h"
#include <AzTest/AzTest.h>

#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Math/Spline.h>
#include <AzCore/Math/VertexContainerInterface.h>
#include <AzCore/std/smart_ptr/make_shared.h>

#include <AzFramework/Application/Application.h>
#include <AzFramework/Components/TransformComponent.h>

#include <Tests/TestTypes.h>
#include <Shape/SplineComponent.h>

using namespace AZ;

namespace UnitTest
{
    class SplineComponentTests
        : public AllocatorsFixture
    {
        AZStd::unique_ptr<SerializeContext> m_serializeContext;
        AZStd::unique_ptr<ComponentDescriptor> m_transformComponentDescriptor;
        AZStd::unique_ptr<ComponentDescriptor> m_splineComponentDescriptor;

    public:
        void SetUp() override
        {
            AllocatorsFixture::SetUp();
            m_serializeContext = AZStd::make_unique<SerializeContext>();

            m_transformComponentDescriptor = AZStd::unique_ptr<ComponentDescriptor>(AzFramework::TransformComponent::CreateDescriptor());
            m_transformComponentDescriptor->Reflect(&(*m_serializeContext));
            m_splineComponentDescriptor = AZStd::unique_ptr<ComponentDescriptor>(LmbrCentral::SplineComponent::CreateDescriptor());
            m_splineComponentDescriptor->Reflect(&(*m_serializeContext));
        }

        void TearDown() override
        {
            m_serializeContext.reset();
            AllocatorsFixture::TearDown();
        }

        void Spline_AddUpdate() const
        {
            Entity entity;
            entity.CreateComponent<LmbrCentral::SplineComponent>();
            entity.CreateComponent<AzFramework::TransformComponent>();

            entity.Init();
            entity.Activate();

            LmbrCentral::SplineComponentRequestBus::Event(entity.GetId(), &LmbrCentral::SplineComponentRequests::AddVertex, Vector3(0.0f, 0.0f, 0.0f));
            LmbrCentral::SplineComponentRequestBus::Event(entity.GetId(), &LmbrCentral::SplineComponentRequests::AddVertex, Vector3(0.0f, 10.0f, 0.0f));
            LmbrCentral::SplineComponentRequestBus::Event(entity.GetId(), &LmbrCentral::SplineComponentRequests::AddVertex, Vector3(10.0f, 10.0f, 0.0f));
            LmbrCentral::SplineComponentRequestBus::Event(entity.GetId(), &LmbrCentral::SplineComponentRequests::AddVertex, Vector3(10.0f, 0.0f, 0.0f));

            ConstSplinePtr spline;
            LmbrCentral::SplineComponentRequestBus::EventResult(spline, entity.GetId(), &LmbrCentral::SplineComponentRequests::GetSpline);

            AZ_TEST_ASSERT(spline->GetVertexCount() == 4);

            LmbrCentral::SplineComponentRequestBus::Event(entity.GetId(), &LmbrCentral::SplineComponentRequests::RemoveVertex, 0);
            AZ_TEST_ASSERT(spline->GetVertexCount() == 3);

            LmbrCentral::SplineComponentRequestBus::Event(entity.GetId(), &LmbrCentral::SplineComponentRequests::UpdateVertex, 0, Vector3(10.0f, 10.0f, 10.0f));
            AZ_TEST_ASSERT(spline->GetVertex(0).IsClose(Vector3(10.0f, 10.0f, 10.0f)));

            LmbrCentral::SplineComponentRequestBus::Event(entity.GetId(), &LmbrCentral::SplineComponentRequests::InsertVertex, 1, Vector3(20.0f, 20.0f, 20.0f));
            AZ_TEST_ASSERT(spline->GetVertex(1).IsClose(Vector3(20.0f, 20.0f, 20.0f)));
            AZ_TEST_ASSERT(spline->GetVertexCount() == 4);
            AZ_TEST_ASSERT(spline->GetVertex(2).IsClose(Vector3(10.0f, 10.0f, 0.0f)));
        }

        void Spline_CopyModify() const
        {
            Entity entity;
            // add spline component - default to Linear
            entity.CreateComponent<LmbrCentral::SplineComponent>();
            entity.CreateComponent<AzFramework::TransformComponent>();

            entity.Init();
            entity.Activate();

            // set vertices via vertex container bus
            LmbrCentral::SplineComponentRequestBus::Event(entity.GetId(), &LmbrCentral::SplineComponentRequests::SetVertices, AZStd::vector<Vector3>{ Vector3(0.0f, 0.0f, 0.0f), Vector3(0.0f, 10.0f, 0.0f), Vector3(10.0f, 10.0f, 0.0f), Vector3(10.0f, 0.0f, 0.0f) });

            // get linear spline from entity
            ConstSplinePtr linearSplinePtr;
            LmbrCentral::SplineComponentRequestBus::EventResult(linearSplinePtr, entity.GetId(), &LmbrCentral::SplineComponentRequests::GetSpline);
            AZ_TEST_ASSERT(linearSplinePtr->GetVertexCount() == 4);

            // clear vertices via vertex container bus
            LmbrCentral::SplineComponentRequestBus::Event(entity.GetId(), &LmbrCentral::SplineComponentRequests::ClearVertices);
            AZ_TEST_ASSERT(linearSplinePtr->GetVertexCount() == 0);

            // set vertices via vertex container bus
            LmbrCentral::SplineComponentRequestBus::Event(entity.GetId(), &LmbrCentral::SplineComponentRequests::SetVertices, AZStd::vector<Vector3>{ Vector3(0.0f, 0.0f, 0.0f), Vector3(0.0f, 5.0f, 0.0f), Vector3(5.0f, 5.0f, 0.0f), Vector3(5.0f, 0.0f, 0.0f) });
            AZ_TEST_ASSERT(linearSplinePtr->GetVertexCount() == 4);

            // change spline type to Bezier
            LmbrCentral::SplineComponentRequestBus::Event(entity.GetId(), &LmbrCentral::SplineComponentRequests::ChangeSplineType, BezierSpline::RTTI_Type().GetHash());

            // check data was created after change correctly
            ConstSplinePtr bezierSplinePtr;
            LmbrCentral::SplineComponentRequestBus::EventResult(bezierSplinePtr, entity.GetId(), &LmbrCentral::SplineComponentRequests::GetSpline);
            if (const BezierSpline* bezierSpline = azrtti_cast<const BezierSpline*>(bezierSplinePtr.get()))
            {
                AZ_TEST_ASSERT(bezierSpline->GetBezierData().size() == 4);
                AZ_TEST_ASSERT(bezierSpline->GetVertexCount() == 4);
            }
            else
            {
                AZ_TEST_ASSERT(false);
            }

            // check copy constructor
            {
                SplinePtr newBezierSplinePtr = AZStd::make_shared<BezierSpline>(*bezierSplinePtr.get());
                if (const BezierSpline* bezierSpline = azrtti_cast<const BezierSpline*>(newBezierSplinePtr.get()))
                {
                    AZ_TEST_ASSERT(bezierSpline->GetBezierData().size() == 4);
                    AZ_TEST_ASSERT(bezierSpline->GetVertexCount() == 4);
                }
                else
                {
                    AZ_TEST_ASSERT(false);
                }
            }

            // check assignment operator
            {
                if (const BezierSpline* bezierSpline = azrtti_cast<const BezierSpline*>(bezierSplinePtr.get()))
                {
                    BezierSpline newBezierSpline;
                    newBezierSpline = *bezierSplinePtr.get();

                    AZ_TEST_ASSERT(newBezierSpline.GetBezierData().size() == 4);
                    AZ_TEST_ASSERT(newBezierSpline.GetVertexCount() == 4);
                }
                else
                {
                    AZ_TEST_ASSERT(false);
                }
            }

            // set vertices for Bezier spline
            LmbrCentral::SplineComponentRequestBus::Event(entity.GetId(), &LmbrCentral::SplineComponentRequests::SetVertices, AZStd::vector<Vector3>{ Vector3(0.0f, 0.0f, 0.0f), Vector3(0.0f, 0.0f, 10.0f) });
            if (const BezierSpline* bezierSpline = azrtti_cast<const BezierSpline*>(bezierSplinePtr.get()))
            {
                AZ_TEST_ASSERT(bezierSpline->GetBezierData().size() == 2);
                AZ_TEST_ASSERT(bezierSpline->GetVertexCount() == 2);
            }
            else
            {
                AZ_TEST_ASSERT(false);
            }

            // change spline type to CatmullRom
            LmbrCentral::SplineComponentRequestBus::Event(entity.GetId(), &LmbrCentral::SplineComponentRequests::ChangeSplineType, CatmullRomSpline::RTTI_Type().GetHash());
            
            ConstSplinePtr catmullRomSplinePtr;
            LmbrCentral::SplineComponentRequestBus::EventResult(catmullRomSplinePtr, entity.GetId(), &LmbrCentral::SplineComponentRequests::GetSpline);
            if (const CatmullRomSpline* catmullRomSpline = azrtti_cast<const CatmullRomSpline*>(catmullRomSplinePtr.get()))
            {
                AZ_TEST_ASSERT(catmullRomSpline->GetVertexCount() == 2);
            }
            else
            {
                AZ_TEST_ASSERT(false);
            }
        }
    };

    TEST_F(SplineComponentTests, Spline_AddUpdate)
    {
        Spline_AddUpdate();
    }

    TEST_F(SplineComponentTests, Spline_CopyModify)
    {
        Spline_CopyModify();
    }
}