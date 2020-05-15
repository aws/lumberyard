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

#pragma once

#include <AzTest/AzTest.h>
#include <Mocks/ITimerMock.h>
#include <Mocks/ICryPakMock.h>
#include <Mocks/IConsoleMock.h>
#include <Mocks/ISystemMock.h>

#include <AzCore/UnitTest/TestTypes.h>

#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Math/Random.h>
#include <AzCore/Math/Spline.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/Memory/SystemAllocator.h>

#include <LmbrCentral/Shape/SplineComponentBus.h>
#include <LmbrCentral/Shape/SplineAttribute.h>


namespace UnitTest
{
    namespace RoadsAndRiversTest
    {
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
            static void Reflect(AZ::ReflectContext* context)
            {
                if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
                {
                    LmbrCentral::SplineAttribute<float>::Reflect(*serializeContext);
                }

            }
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
            bool RemoveVertex(size_t index) override { return false; }
            void SetVertices(const AZStd::vector<AZ::Vector3>& vertices) override {  }
            void ClearVertices() override { m_spline->m_vertexContainer.Clear(); }

            size_t Size() const override { return m_spline->m_vertexContainer.Size(); }
            bool Empty() const override { return m_spline->m_vertexContainer.Empty(); }

        private:
            AZ::SplinePtr m_spline;
        };

        template <class RoadAndRiverComponentType>
        AZ::Entity* CreateTestEntity(bool initEntity = true, bool activateEntity = true)
        {
            AZ::Entity* testEntity = aznew AZ::Entity("test_entity");
            EXPECT_TRUE(testEntity != nullptr);
            testEntity->CreateComponent<MockTransformComponent>();
            testEntity->CreateComponent<MockSplineComponent>();
            testEntity->CreateComponent<RoadAndRiverComponentType>();

            if (initEntity)
            {
                testEntity->Init();

                if (activateEntity)
                {
                    testEntity->Activate();
                }
            }

            return testEntity;
        }
    }
}
