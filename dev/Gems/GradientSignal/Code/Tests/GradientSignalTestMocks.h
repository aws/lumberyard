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
#pragma once

#include <AzTest/AzTest.h>
#include <AzCore/std/hash.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Component/TransformBus.h>

#include <GradientSignal/Ebuses/GradientRequestBus.h>
#include <GradientSignal/GradientSampler.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>
#include <SurfaceData/SurfaceDataSystemRequestBus.h>
#include <AzCore/Casting/lossy_cast.h>

namespace UnitTest
{
    struct GradientSignalTest
        : public ::testing::Test
    {
    protected:
        AZ::ComponentApplication m_app;
        AZ::Entity* m_systemEntity = nullptr;

        void SetUp() override
        {
            AZ::ComponentApplication::Descriptor appDesc;
            appDesc.m_memoryBlocksByteSize = 32 * 1024 * 1024;
            m_systemEntity = m_app.Create(appDesc);
            m_app.AddEntity(m_systemEntity);
        }

        void TearDown() override
        {
            m_app.Destroy();
            m_systemEntity = nullptr;
        }

        void TestFixedDataSampler(const AZStd::vector<float>& expectedOutput, int size, AZ::EntityId gradientEntityId)
        {
            GradientSignal::GradientSampler gradientSampler;
            gradientSampler.m_gradientId = gradientEntityId;

            for(int y = 0; y < size; ++y)
            {
                for (int x = 0; x < size; ++x)
                {
                    GradientSignal::GradientSampleParams params;
                    params.m_position = AZ::Vector3(static_cast<float>(x), static_cast<float>(y), 0.0f);

                    const int index = y * size + x;
                    float actualValue = gradientSampler.GetValue(params);
                    float expectedValue = expectedOutput[index];

                    EXPECT_NEAR(actualValue, expectedValue, 0.01f);
                }
            }
        }

        AZStd::unique_ptr<AZ::Entity> CreateEntity()
        {
            return AZStd::make_unique<AZ::Entity>();
        }

        void ActivateEntity(AZ::Entity* entity)
        {
            entity->Init();
            EXPECT_EQ(AZ::Entity::ES_INIT, entity->GetState());

            entity->Activate();
            EXPECT_EQ(AZ::Entity::ES_ACTIVE, entity->GetState());
        }

        template <typename Component, typename Configuration>
        AZ::Component* CreateComponent(AZ::Entity* entity, const Configuration& config)
        {
            m_app.RegisterComponentDescriptor(Component::CreateDescriptor());
            return entity->CreateComponent<Component>(config);
        }

        template <typename Component>
        AZ::Component* CreateComponent(AZ::Entity* entity)
        {
            m_app.RegisterComponentDescriptor(Component::CreateDescriptor());
            return entity->CreateComponent<Component>();
        }
    };

    struct MockGradientRequestsBus
        : public GradientSignal::GradientRequestBus::Handler
    {
        MockGradientRequestsBus(AZ::EntityId& id)
        {
            BusConnect(id);
        }

        ~MockGradientRequestsBus()
        {
            BusDisconnect();
        }

        float m_GetValue = 0.0f;
        float GetValue(const GradientSignal::GradientSampleParams& sampleParams) const override
        {
            return m_GetValue;
        }

        bool IsEntityInHierarchy(const AZ::EntityId &) const override
        {
            return false;
        }
    };

    struct MockGradientArrayRequestsBus
        : public GradientSignal::GradientRequestBus::Handler
    {
        MockGradientArrayRequestsBus(AZ::EntityId& id, const AZStd::vector<float>& data, int dataSize)
            : m_getValue(data), m_dataSize(dataSize)
        {
            BusConnect(id);
        }

        ~MockGradientArrayRequestsBus()
        {
            BusDisconnect();
        }

        float GetValue(const GradientSignal::GradientSampleParams& sampleParams) const override
        {
            const auto& pos = sampleParams.m_position;
            const int index = azlossy_caster<float>(pos.GetY() * float(m_dataSize) + pos.GetX());
            
            return m_getValue[index];
        }

        bool IsEntityInHierarchy(const AZ::EntityId &) const override
        {
            return false;
        }

        AZStd::vector<float> m_getValue;
        int m_dataSize;
    };

    struct MockShapeComponentHandler
        : public LmbrCentral::ShapeComponentRequestsBus::Handler
    {
        MockShapeComponentHandler(const AZ::EntityId& id)
        {
            BusConnect(id);
        }

        ~MockShapeComponentHandler() override
        {
            BusDisconnect();
        }

        AZ::Aabb m_GetLocalBounds = AZ::Aabb::CreateCenterRadius(AZ::Vector3::CreateZero(), 0.5f);
        AZ::Transform m_GetTransform = AZ::Transform::CreateIdentity();
        void GetTransformAndLocalBounds(AZ::Transform& transform, AZ::Aabb& bounds) override
        {
            transform = m_GetTransform;
            bounds = m_GetLocalBounds;
        }

        AZ::Crc32 m_GetShapeType = AZ_CRC("MockShapeComponentHandler", 0x5189d279);
        AZ::Crc32 GetShapeType() override
        {
            return m_GetShapeType;
        }

        AZ::Aabb m_GetEncompassingAabb = AZ::Aabb::CreateCenterRadius(AZ::Vector3::CreateZero(), 0.5f);
        AZ::Aabb GetEncompassingAabb() override
        {
            return m_GetEncompassingAabb;
        }

        bool IsPointInside(const AZ::Vector3& point) override
        {
            return m_GetEncompassingAabb.Contains(point);
        }

        float DistanceSquaredFromPoint(const AZ::Vector3& point) override
        {
            return m_GetEncompassingAabb.GetDistanceSq(point);
        }
    };

    struct MockShapeComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(MockShapeComponent, "{DD9590BC-916B-4EFA-98B8-AC5023941672}", AZ::Component);

        void Activate() override {}
        void Deactivate() override {}

        static void Reflect(AZ::ReflectContext* reflect) { AZ_UNUSED(reflect); }
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("ShapeService", 0xe86aa5fe));
        }
    };

    struct MockTransformHandler
        : public AZ::TransformBus::Handler
    {
        ~MockTransformHandler()
        {
            AZ::TransformBus::Handler::BusDisconnect();
        }

        AZ::Transform m_GetLocalTMOutput = AZ::Transform::CreateIdentity();
        const AZ::Transform & GetLocalTM() override
        {
            return m_GetLocalTMOutput;
        }

        AZ::Transform m_GetWorldTMOutput = AZ::Transform::CreateIdentity();
        const AZ::Transform & GetWorldTM() override
        {
            return m_GetWorldTMOutput;
        }

        bool IsStaticTransform() override
        {
            return false;
        }

        bool IsPositionInterpolated() override
        {
            return false;
        }

        bool IsRotationInterpolated() override
        {
            return false;
        }
    };

    struct MockSurfaceDataSystem
        : public SurfaceData::SurfaceDataSystemRequestBus::Handler
    {
        MockSurfaceDataSystem()
        {
            BusConnect();
        }

        ~MockSurfaceDataSystem()
        {
            BusDisconnect();
        }

        AZStd::unordered_map<AZStd::pair<float, float>, SurfaceData::SurfacePointList> m_GetSurfacePoints;
        void GetSurfacePoints(const AZ::Vector3& inPosition, const SurfaceData::SurfaceTagVector& masks, SurfaceData::SurfacePointList& surfacePointList) const override
        {
            auto surfacePoints = m_GetSurfacePoints.find(AZStd::make_pair(inPosition.GetX(), inPosition.GetY()));

            if (surfacePoints != m_GetSurfacePoints.end())
            {
                surfacePointList = surfacePoints->second;
            }
        }

        SurfaceData::SurfaceDataRegistryHandle RegisterSurfaceDataProvider(const SurfaceData::SurfaceDataRegistryEntry& entry) override
        {
            return {};
        }

        void UnregisterSurfaceDataProvider(const SurfaceData::SurfaceDataRegistryHandle& handle) override
        {
        }

        SurfaceData::SurfaceDataRegistryHandle RegisterSurfaceDataModifier(const SurfaceData::SurfaceDataRegistryEntry& entry) override
        {
            return {};
        }

        void UnregisterSurfaceDataModifier(const SurfaceData::SurfaceDataRegistryHandle& handle) override
        {
        }

        void UpdateSurfaceDataModifier(const SurfaceData::SurfaceDataRegistryHandle& handle, const SurfaceData::SurfaceDataRegistryEntry& entry, const AZ::Aabb& dirtyBoundsOverride) override
        {
        }

        void UpdateSurfaceDataProvider(const SurfaceData::SurfaceDataRegistryHandle& handle, const SurfaceData::SurfaceDataRegistryEntry& entry, const AZ::Aabb& dirtyBoundsOverride) override
        {
        }

    };
}