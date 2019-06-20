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

#include <Vegetation/StaticVegetationBus.h>
#include <Vegetation/AreaComponentBase.h>
#include <Vegetation/Ebuses/StaticVegetationBlockerRequestBus.h>

#define ENABLE_STATIC_VEGETATION_DEBUG_TRACKING 0 

struct IRenderAuxGeom;

namespace AZ
{
    class Vector2;
}

namespace LmbrCentral
{
    template<typename, typename>
    class EditorWrappedComponentBase;
}

namespace Vegetation
{
    class StaticVegetationBlockerConfig
        : public AreaConfig
    {
    public:
        AZ_CLASS_ALLOCATOR(StaticVegetationBlockerConfig, AZ::SystemAllocator, 0);
        AZ_RTTI(StaticVegetationBlockerConfig, "{A4D79248-E264-4E44-9302-0FCBF142FFCB}", AreaConfig);
        static void Reflect(AZ::ReflectContext* context);
        
        StaticVegetationBlockerConfig() : AreaConfig() { m_priority = s_priorityMax; m_layer = AreaLayer::Foreground; }
        bool m_showBoundingBox = false;
        bool m_showBlockedPoints = false;
        float m_maxDebugRenderDistance = 50.0f;
        float m_globalPadding = 0.0f;
    };

    static const AZ::Uuid StaticVegetationBlockerComponentTypeId = "{E2420D03-F7F3-4A11-BA44-3328020481AA}";

    class StaticVegetationBlockerComponent
        : public AreaComponentBase
        , private StaticVegetationNotificationBus::Handler
        , private StaticVegetationBlockerRequestBus::Handler
    {
    public:
        friend class EditorStaticVegetationBlockerComponent;
        template<typename, typename> friend class LmbrCentral::EditorWrappedComponentBase;
        AZ_COMPONENT(StaticVegetationBlockerComponent, StaticVegetationBlockerComponentTypeId, AreaComponentBase);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void Reflect(AZ::ReflectContext* context);

        StaticVegetationBlockerComponent() = default;
        StaticVegetationBlockerComponent(const StaticVegetationBlockerConfig& configuration);

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;
        bool ReadInConfig(const AZ::ComponentConfig* baseConfig) override;
        bool WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const override;

        //////////////////////////////////////////////////////////////////////////
        // AreaRequestBus::Handler
        bool PrepareToClaim(EntityIdStack& stackIds) override;
        void ClaimPositions(EntityIdStack& stackIds, ClaimContext& context) override;
        void UnclaimPosition(const ClaimHandle handle) override;

        //////////////////////////////////////////////////////////////////////////
        // AreaInfoBus::Handler
        AZ::Aabb GetEncompassingAabb() const override;
        AZ::u32 GetProductCount() const override;

        //////////////////////////////////////////////////////////////////////////
        // LmbrCentral::DependencyNotificationBus
        void OnCompositionChanged() override;

    protected:

        //////////////////////////////////////////////////////////////////////////
        // StaticVegetationBlockerRequestBus::Handler
        float GetAreaPriority() const override;
        void SetAreaPriority(float priority) override;
        AreaLayer GetAreaLayer() const override;
        void SetAreaLayer(AreaLayer type) override;
        AZ::u32 GetAreaProductCount() const override;
        float GetBoundingBoxPadding() const override;
        void SetBoundingBoxPadding(float padding) override;
        bool GetShowBoundingBox() const override;
        void SetShowBoundingBox(bool value) override;
        bool GetShowBlockedPoints() const override;
        void SetShowBlockedPoints(bool value) override;
        float GetMaxDebugDrawDistance() const override;
        void SetMaxDebugDrawDistance(float distance) override;

    private:

        static AZ::Aabb GetPaddedAabb(const AZ::Aabb& aabb, float padding);
        // Returns the world position of the point nearest to the given world position
        static AZ::Vector3 SnapWorldPositionToPoint(const AZ::Vector3& worldPos, float pointsPerMeter);
        static int MetersToPoints(float meters, float pointsPerMeter);
        static AZ::Vector3 PointXYToWorldPos(int x, int y, AZ::Vector3 basePoint, float pointSize);
        static AZStd::pair<int, int> WorldPosToPointXY(AZ::Vector3 pos, float pointsPerMeter);
        static void GetBoundsPointsAndExtents(const AZ::Aabb& aabb, float pointsPerMeter, AZ::Vector3& outMin, AZ::Vector3& outMax, AZ::Vector3& outExtents);
        static void IterateIntersectingPoints(const AZ::Aabb& queryAabb, const AZ::Aabb& iterationBounds, float pointsPerMeter, const AZStd::function<void(int, const AZ::Aabb&)>& callback);

        AZ::Aabb GetCachedBounds();

        //////////////////////////////////////////////////////////////////////////
        // StaticVegetationNotificationBus::Handler
        void InstanceAdded(UniqueVegetationInstancePtr vegetationInstance, AZ::Aabb aabb) override;
        void InstanceRemoved(UniqueVegetationInstancePtr vegetationInstance, AZ::Aabb aabb) override;
        void VegetationCleared() override;

        StaticVegetationBlockerConfig m_configuration;

        struct VegetationPointData
        {
            VegetationPointData() = default;

#if(ENABLE_STATIC_VEGETATION_DEBUG_TRACKING)
            VegetationPointData(bool blocked, AZ::Vector3 worldPosition) : m_blocked(blocked), m_worldPosition(worldPosition) {}
#else
            VegetationPointData(bool blocked) : m_blocked(blocked) {}
#endif

            bool m_blocked = false;
#if(ENABLE_STATIC_VEGETATION_DEBUG_TRACKING)
            AZ::Vector3 m_worldPosition;
#endif
        };

        // Used to keep track of the occupancy status of every vegetation point within the shape bounds
        AZStd::vector<VegetationPointData> m_vegetationMap;
        AZ::u64 m_lastMapUpdateCount = (AZ::u64)-1;
        AZStd::recursive_mutex m_vegetationMapMutex;

        // Indicates if the veg map needs to be updated due to move/resize of the bounding aabb
        AZStd::atomic_bool m_boundsHaveChanged{false};

        // Copy of the bounds aabb.  Updated on Shape/Transform change to keep it and m_boundsHaveChanged in sync.
        AZ::Aabb m_cachedBounds;
        AZStd::recursive_mutex m_boundsMutex;
    };
}