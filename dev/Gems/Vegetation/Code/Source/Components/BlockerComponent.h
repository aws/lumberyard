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

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Transform.h>
#include <LmbrCentral/Dependency/DependencyNotificationBus.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>
#include <Vegetation/EBuses/AreaNotificationBus.h>
#include <Vegetation/Ebuses/BlockerRequestBus.h>
#include <Vegetation/AreaComponentBase.h>

#define VEG_BLOCKER_ENABLE_CACHING 0

namespace LmbrCentral
{
    template<typename, typename>
    class EditorWrappedComponentBase;
}

namespace Vegetation
{
    class BlockerConfig
        : public AreaConfig
    {
    public:
        AZ_CLASS_ALLOCATOR(BlockerConfig, AZ::SystemAllocator, 0);
        AZ_RTTI(BlockerConfig, "{01F6E6C5-707E-42EC-91BB-F674B9F51A40}", AreaConfig);
        static void Reflect(AZ::ReflectContext* context);

        BlockerConfig() : AreaConfig() { m_priority = s_priorityMax; m_layer = AreaLayer::Foreground; }
        bool m_inheritBehavior = true;
    };

    static const AZ::Uuid BlockerComponentTypeId = "{C8A7AAEB-C315-44CE-919D-F304B53ACA4A}";

    /**
    * Blocking claim logic for vegetation in an area
    */
    class BlockerComponent
        : public AreaComponentBase
        , private BlockerRequestBus::Handler
    {
    public:
        friend class EditorBlockerComponent;
        template<typename, typename> friend class LmbrCentral::EditorWrappedComponentBase;
        AZ_COMPONENT(BlockerComponent, BlockerComponentTypeId, AreaComponentBase);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void Reflect(AZ::ReflectContext* context);

        BlockerComponent(const BlockerConfig& configuration);
        BlockerComponent() = default;
        ~BlockerComponent() = default;

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;
        bool ReadInConfig(const AZ::ComponentConfig* baseConfig) override;
        bool WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const override;

        //////////////////////////////////////////////////////////////////////////
        // AreaRequestBus
        bool PrepareToClaim(EntityIdStack& stackIds) override;
        void ClaimPositions(EntityIdStack& stackIds, ClaimContext& context) override;
        void UnclaimPosition(const ClaimHandle handle) override;
        AZ::Aabb GetEncompassingAabb() const override;
        AZ::u32 GetProductCount() const override;

        //////////////////////////////////////////////////////////////////////////
        // AreaNotificationBus
        void OnCompositionChanged() override;

    protected:
        //////////////////////////////////////////////////////////////////////////
        // BlockerRequestBus
        float GetAreaPriority() const override;
        void SetAreaPriority(float priority) override;
        AreaLayer GetAreaLayer() const override;
        void SetAreaLayer(AreaLayer layer) override;
        AZ::u32 GetAreaProductCount() const override;
        bool GetInheritBehavior() const override;
        void SetInheritBehavior(bool value) override;

    private:
        void UpdateShapeParams();
        bool ClaimPosition(EntityIdStack& processedIds, const ClaimPoint& point, InstanceData& instanceData);
        BlockerConfig m_configuration;

        mutable AZStd::recursive_mutex m_shapeMutex;
        AZ::Aabb m_shapeBounds = AZ::Aabb::CreateNull();
        AZ::Transform m_shapeTransformInverse = AZ::Transform::CreateIdentity();

#if VEG_BLOCKER_ENABLE_CACHING
        // cached data
        AZStd::recursive_mutex m_cacheMutex;
        using ClaimInstanceMapping = AZStd::unordered_map<ClaimHandle, bool>;
        ClaimInstanceMapping m_claimCacheMapping;
#endif
    };
}