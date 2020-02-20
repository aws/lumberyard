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

#include <gtest/gtest.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <Integration/Assets/ActorAsset.h>
#include <Integration/Rendering/RenderBackend.h>
#include <Integration/Rendering/RenderActor.h>
#include <Integration/Rendering/RenderActorInstance.h>
#include <Integration/Rendering/RenderBackendManager.h>
#include <Integration/System/SystemCommon.h>
#include <Tests/SystemComponentFixture.h>

namespace EMotionFX
{
    namespace Integration
    {
        class TestRenderActor
            : public RenderActor
        {
        public:
            AZ_RTTI(TestRenderActor, "{560849A4-7767-4654-8C61-EDA9A0059BE1}", RenderActor)
            AZ_CLASS_ALLOCATOR(TestRenderActor, EMotionFXAllocator, 0)
        };

        class TestRenderActorInstance
            : public RenderActorInstance
        {
        public:
            AZ_RTTI(TestRenderActorInstance, "{8F5CD404-9661-4A71-9583-EB8E66F3C0E8}", RenderActorInstance)
            AZ_CLASS_ALLOCATOR(TestRenderActorInstance, EMotionFXAllocator, 0)

            MOCK_METHOD1(OnTick, void(float));
            MOCK_METHOD1(DebugDraw, void(const DebugOptions&));
            MOCK_CONST_METHOD0(IsVisible, bool());
            MOCK_METHOD1(SetIsVisible, void(bool));
            MOCK_METHOD1(SetMaterials, void(const ActorAsset::MaterialList&));
            MOCK_METHOD0(GetWorldBounds, AZ::Aabb());
            MOCK_METHOD0(GetLocalBounds, AZ::Aabb());
        };

        class TestRenderBackend
            : public RenderBackend
        {
        public:
            AZ_RTTI(TestRenderBackend, "{22CC2C55-8019-4302-8DFD-E08E0CA48114}", RenderBackend)
            AZ_CLASS_ALLOCATOR(TestRenderBackend, EMotionFXAllocator, 0)

            MOCK_METHOD1(CreateActor, RenderActor*(ActorAsset*));
            MOCK_METHOD6(CreateActorInstance, RenderActorInstance*(AZ::EntityId,
                const EMotionFXPtr<EMotionFX::ActorInstance>&,
                const AZ::Data::Asset<ActorAsset>&,
                const ActorAsset::MaterialList&,
                SkinningMethod,
                const AZ::Transform&));
        };

        class RenderBackendManagerFixture
            : public SystemComponentFixture
        {
        public:
            void SetUp() override
            {
                SystemComponentFixture::SetUp();
                EXPECT_NE(AZ::Interface<RenderBackendManager>::Get(), nullptr);
            }

            void TearDown() override
            {
                SystemComponentFixture::TearDown();
                EXPECT_EQ(AZ::Interface<RenderBackendManager>::Get(), nullptr);
            }

        public:
            TestRenderBackend* m_renderBackend = nullptr;
        };

        TEST_F(RenderBackendManagerFixture, AdjustRenderBackend)
        {
            RenderBackendManager* renderBackendManager = AZ::Interface<RenderBackendManager>::Get();
            EXPECT_NE(renderBackendManager, nullptr);

            m_renderBackend = aznew TestRenderBackend();
            AZ::Interface<RenderBackendManager>::Get()->SetRenderBackend(m_renderBackend);
            RenderBackend* renderBackend = renderBackendManager->GetRenderBackend();
            EXPECT_NE(renderBackend, nullptr);
            EXPECT_EQ(renderBackend->RTTI_GetType(), azrtti_typeid<TestRenderBackend>());
        }
    } // namespace Integration
} // namespace EMotionFX
