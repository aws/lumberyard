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

#include "Vegetation_precompiled.h"
#include "StaticVegetationBlockerComponent.h"
#include <AzCore/Component/Component.h>
#include <GradientSignal/Ebuses/SectorDataRequestBus.h>
#include <Terrain/Bus/LegacyTerrainBus.h>
#include <CrySystemBus.h>
#include <AzCore/Math/Color.h>
#include <IRenderAuxGeom.h>
#include <MathConversion.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <Vegetation/Ebuses/AreaSystemRequestBus.h>
#include <I3DEngine.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>
#include <AzCore/Component/TransformBus.h>

namespace Vegetation
{
    void StaticVegetationBlockerConfig::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);

        if (serialize)
        {
            serialize->Class<StaticVegetationBlockerConfig, AreaConfig>()
                ->Field("Bounding Box Padding", &StaticVegetationBlockerConfig::m_globalPadding)
                ->Field("Show Bounding Box", &StaticVegetationBlockerConfig::m_showBoundingBox)
                ->Field("Show Blocked Points", &StaticVegetationBlockerConfig::m_showBlockedPoints)
                ->Field("Max Debug Render Distance", &StaticVegetationBlockerConfig::m_maxDebugRenderDistance)
                ->Version(0);

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
            {
                edit->Class<StaticVegetationBlockerConfig>(
                    "Vegetation Layer Blocker (Static)", "Prevents dynamic vegetation from being placed on top of static vegetation")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Vegetation")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(0, &StaticVegetationBlockerConfig::m_globalPadding, "Bounding Box Padding", "Adds a padding size to every static vegetation entity bounding box")
                    ->DataElement(0, &StaticVegetationBlockerConfig::m_showBoundingBox, "Show Bounding Box", "Show a bounding box around each static vegetation entity")
                    ->DataElement(0, &StaticVegetationBlockerConfig::m_showBlockedPoints, "Show Blocked Points", "Show a bounding box around each point blocked out by a static vegetation instance")
                    ->DataElement(0, &StaticVegetationBlockerConfig::m_maxDebugRenderDistance, "Max Debug Draw Distance", "Max distance from the camera to render debug bounding boxes")
                    ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<StaticVegetationBlockerConfig>()
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::Preview)
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Constructor()
                ->Property("showBoundingBox", BehaviorValueProperty(&StaticVegetationBlockerConfig::m_showBoundingBox))
                ->Property("showBlockedPoints", BehaviorValueProperty(&StaticVegetationBlockerConfig::m_showBlockedPoints))
                ->Property("maxDebugRenderDistance", BehaviorValueProperty(&StaticVegetationBlockerConfig::m_maxDebugRenderDistance))
                ->Property("boundingBoxPadding", BehaviorValueProperty(&StaticVegetationBlockerConfig::m_globalPadding))
                ;
        }
    }

    //////////////////////////////////////////////////////////////////////////

    void StaticVegetationBlockerComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        AreaComponentBase::GetProvidedServices(services);
    }

    void StaticVegetationBlockerComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        AreaComponentBase::GetIncompatibleServices(services);
        services.push_back(AZ_CRC("VegetationModifierService", 0xc551fca6));
        services.push_back(AZ_CRC("VegetationFilterService", 0x9f97cc97));
    }

    void StaticVegetationBlockerComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        AreaComponentBase::GetRequiredServices(services);
        services.push_back(AZ_CRC("ShapeService", 0xe86aa5fe));
    }

    void StaticVegetationBlockerComponent::Reflect(AZ::ReflectContext* context)
    {
        StaticVegetationBlockerConfig::Reflect(context);

        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<StaticVegetationBlockerComponent, AreaComponentBase>()
                ->Field("Configuration", &StaticVegetationBlockerComponent::m_configuration)
                ->Version(0);
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Constant("StaticVegetationBlockerComponentTypeId", BehaviorConstant(StaticVegetationBlockerComponentTypeId));

            behaviorContext->Class<StaticVegetationBlockerComponent>()->RequestBus("StaticVegetationBlockerRequestBus");

            behaviorContext->EBus<StaticVegetationBlockerRequestBus>("StaticVegetationBlockerRequestBus")
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::Preview)
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Event("GetAreaPriority", &StaticVegetationBlockerRequestBus::Events::GetAreaPriority)
                ->Event("SetAreaPriority", &StaticVegetationBlockerRequestBus::Events::SetAreaPriority)
                ->VirtualProperty("AreaPriority", "GetAreaPriority", "SetAreaPriority")
                ->Event("GetAreaLayer", &StaticVegetationBlockerRequestBus::Events::GetAreaLayer)
                ->Event("SetAreaLayer", &StaticVegetationBlockerRequestBus::Events::SetAreaLayer)
                ->VirtualProperty("AreaLayer", "GetAreaLayer", "SetAreaLayer")
                ->Event("GetAreaProductCount", &StaticVegetationBlockerRequestBus::Events::GetAreaProductCount)
                ->Event("GetBoundingBoxPadding", &StaticVegetationBlockerRequestBus::Events::GetBoundingBoxPadding)
                ->Event("SetBoundingBoxPadding", &StaticVegetationBlockerRequestBus::Events::SetBoundingBoxPadding)
                ->VirtualProperty("BoundingBoxPadding", "GetBoundingBoxPadding", "SetBoundingBoxPadding")
                ->Event("GetShowBoundingBox", &StaticVegetationBlockerRequestBus::Events::GetShowBoundingBox)
                ->Event("SetShowBoundingBox", &StaticVegetationBlockerRequestBus::Events::SetShowBoundingBox)
                ->VirtualProperty("ShowBoundingBox", "GetShowBoundingBox", "SetShowBoundingBox")
                ->Event("GetShowBlockedPoints", &StaticVegetationBlockerRequestBus::Events::GetShowBlockedPoints)
                ->Event("SetShowBlockedPoints", &StaticVegetationBlockerRequestBus::Events::SetShowBlockedPoints)
                ->VirtualProperty("ShowBlockedPoints", "GetShowBlockedPoints", "SetShowBlockedPoints")
                ->Event("GetMaxDebugDrawDistance", &StaticVegetationBlockerRequestBus::Events::GetMaxDebugDrawDistance)
                ->Event("SetMaxDebugDrawDistance", &StaticVegetationBlockerRequestBus::Events::SetMaxDebugDrawDistance)
                ->VirtualProperty("MaxDebugDrawDistance", "GetMaxDebugDrawDistance", "SetMaxDebugDrawDistance")
                ;
        }
    }

    StaticVegetationBlockerComponent::StaticVegetationBlockerComponent(const StaticVegetationBlockerConfig& configuration)
        : AreaComponentBase(configuration),
        m_configuration(configuration)
    {

    }

    void StaticVegetationBlockerComponent::Activate()
    {
        AreaNotificationBus::Handler::BusConnect(GetEntityId());
        StaticVegetationNotificationBus::Handler::BusConnect();
        StaticVegetationBlockerRequestBus::Handler::BusConnect(GetEntityId());

        m_cachedBounds = GetEncompassingAabb();

        AreaComponentBase::Activate(); //must activate base last to connect AreaRequestBus once everything else is setup
    }

    void StaticVegetationBlockerComponent::Deactivate()
    {
        AreaComponentBase::Deactivate(); //must deactivate base first to ensure AreaRequestBus disconnect waits for other threads
        
        AreaNotificationBus::Handler::BusConnect(GetEntityId());
        StaticVegetationNotificationBus::Handler::BusDisconnect();
        StaticVegetationBlockerRequestBus::Handler::BusDisconnect();

        AZStd::lock_guard<decltype(m_vegetationMapMutex)> scopedLock(m_vegetationMapMutex);
        m_vegetationMap.clear();
    }

    bool StaticVegetationBlockerComponent::ReadInConfig(const AZ::ComponentConfig* baseConfig)
    {
        AreaComponentBase::ReadInConfig(baseConfig);

        if (auto config = azrtti_cast<const StaticVegetationBlockerConfig*>(baseConfig))
        {
            m_configuration = *config;
            return true;
        }
        return false;
    }

    bool StaticVegetationBlockerComponent::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
    {
        AreaComponentBase::WriteOutConfig(outBaseConfig);

        if (auto config = azrtti_cast<StaticVegetationBlockerConfig*>(outBaseConfig))
        {
            *config = m_configuration;
            return true;
        }
        return false;
    }

    void StaticVegetationBlockerComponent::OnCompositionChanged()
    {
        {
            AZStd::lock_guard<decltype(m_boundsMutex)> scopedLock(m_boundsMutex);

            m_boundsHaveChanged = true;
            m_cachedBounds = GetEncompassingAabb();
        }

        AreaComponentBase::OnCompositionChanged();
    }

    bool StaticVegetationBlockerComponent::PrepareToClaim(EntityIdStack& stackIds)
    {
        return true;
    }

    AZ::Aabb StaticVegetationBlockerComponent::GetPaddedAabb(const AZ::Aabb& aabb, float padding)
    {
        AZ::Vector3 min = aabb.GetMin();
        AZ::Vector3 max = aabb.GetMax();
        AZ::Vector3 halfExtents = (max - min) * 0.5f;

        // Make sure we don't go so far negative that the bounding box inverts
        // Negative signs here are because this is only an issue if padding is < 0
        if (-padding > halfExtents.GetMinElement())
        {
            padding = -halfExtents.GetMinElement();
        }

        AZ::Vector3 paddingVec(padding);

        return AZ::Aabb::CreateFromMinMax(min - paddingVec, max + paddingVec);
    }

    AZ::Vector3 StaticVegetationBlockerComponent::SnapWorldPositionToPoint(const AZ::Vector3& worldPos, float pointsPerMeter)
    {
        AZ::Vector3 result = worldPos * pointsPerMeter;
        result.Set(round(result.GetX()), round(result.GetY()), result.GetZ());
        result /= pointsPerMeter;
        result.SetZ(worldPos.GetZ());

        return result;
    }

    int StaticVegetationBlockerComponent::MetersToPoints(float meters, float pointsPerMeter)
    {
        return int(round(meters * pointsPerMeter));
    }

    AZ::Vector3 StaticVegetationBlockerComponent::PointXYToWorldPos(int x, int y, AZ::Vector3 basePoint, float pointSize)
    {
        return basePoint + AZ::Vector3(x * pointSize, y * pointSize, 0);
    }

    AZStd::pair<int, int> StaticVegetationBlockerComponent::WorldPosToPointXY(AZ::Vector3 pos, float pointsPerMeter)
    {
        return { MetersToPoints(pos.GetX(), pointsPerMeter), MetersToPoints(pos.GetY(), pointsPerMeter) };
    }

    void StaticVegetationBlockerComponent::GetBoundsPointsAndExtents(const AZ::Aabb& aabb, float pointsPerMeter, AZ::Vector3& outMin, AZ::Vector3& outMax, AZ::Vector3& outExtents)
    {
        outMin = SnapWorldPositionToPoint(aabb.GetMin(), pointsPerMeter);
        outMax = SnapWorldPositionToPoint(aabb.GetMax(), pointsPerMeter);
        outExtents = outMax - outMin;
    }

    void StaticVegetationBlockerComponent::IterateIntersectingPoints(const AZ::Aabb& queryAabb, const AZ::Aabb& iterationBounds, float pointsPerMeter, const AZStd::function<void(int, const AZ::Aabb&)>& callback)
    {
        // Early out if the query bounds is outside the iteration bounds
        if (!queryAabb.Overlaps(iterationBounds))
        {
            return;
        }

        AZ::Vector3 iterationCenter = iterationBounds.GetCenter();

        // Adjust the mins & maxes to make sure they fall exactly on a point
        AZ::Vector3 snappedIterMin, snappedIterMax, iterationBoundsSize;
        GetBoundsPointsAndExtents(iterationBounds, pointsPerMeter, snappedIterMin, snappedIterMax, iterationBoundsSize);
        AZ::Vector3 snappedQueryMin = SnapWorldPositionToPoint(queryAabb.GetMin(), pointsPerMeter);
        AZ::Vector3 snappedQueryMax = SnapWorldPositionToPoint(queryAabb.GetMax(), pointsPerMeter);

        AZ_Assert(snappedIterMin.IsLessEqualThan(snappedIterMax) && snappedQueryMin.IsLessEqualThan(snappedQueryMax), "AABB min is > max.");

        // Constrain the min/max to ensure that queryMin >= iterMin && queryMax <= iterMax
        // Essentially we're making sure that the query aabb does not extend outside the iteration aabb
        AZ::Vector3 largestMin = snappedQueryMin.GetMax(snappedIterMin);
        AZ::Vector3 smallestMax = snappedQueryMax.GetMin(snappedIterMax);

        // Now make the min/max relative to the iter bounds
        AZ::Vector3 relativeQueryMin = largestMin - snappedIterMin;
        AZ::Vector3 relativeQueryMax = smallestMax - snappedIterMin;

        // Convert the world coordinates into point coordinates
        int xPointsStart = MetersToPoints(relativeQueryMin.GetX(), pointsPerMeter);
        int yPointsStart = MetersToPoints(relativeQueryMin.GetY(), pointsPerMeter);
        int xPointsEnd = MetersToPoints(relativeQueryMax.GetX(), pointsPerMeter);
        int yPointsEnd = MetersToPoints(relativeQueryMax.GetY(), pointsPerMeter);
        int xPointsIterationBounds = MetersToPoints(iterationBoundsSize.GetX(), pointsPerMeter) + 1; // +1 because we want the number of points, not the index of the last point

        AZ_Assert(xPointsStart <= xPointsEnd, "Start X point must be <= end point. Start: %i, End: %i", xPointsStart, xPointsEnd);
        AZ_Assert(yPointsStart <= yPointsEnd, "Start Y point must be <= end point. Start: %i, End: %i", yPointsStart, yPointsEnd);

        // Loop over all the query points that are contained within the iteration bounds
        for (int y = yPointsStart; y <= yPointsEnd; ++y)
        {
            for (int x = xPointsStart; x <= xPointsEnd && x < xPointsIterationBounds; ++x)
            {
                const float pointSize = 1 / pointsPerMeter;
                const float halfPointSize = pointSize * 0.5f;

                int index = y * xPointsIterationBounds + x;

                AZ::Vector3 pointWorldPos = PointXYToWorldPos(x, y, snappedIterMin, pointSize);
                pointWorldPos.SetZ(iterationCenter.GetZ());
                AZ::Aabb pointAabb = AZ::Aabb::CreateCenterHalfExtents(pointWorldPos, AZ::Vector3(halfPointSize, halfPointSize, iterationBoundsSize.GetZ() * 0.5f));

                if (pointAabb.Overlaps(iterationBounds) && pointAabb.Overlaps(queryAabb))
                {
                    callback(index, pointAabb);
                }
            }
        }
    }

    AZ::Aabb StaticVegetationBlockerComponent::GetCachedBounds()
    {
        AZStd::lock_guard<decltype(m_boundsMutex)> scopedLock(m_boundsMutex);
        return m_cachedBounds;
    }

    void StaticVegetationBlockerComponent::ClaimPositions(EntityIdStack& stackIds, ClaimContext& context)
    {
        float pointsPerMeter = 1.0;

        GradientSignal::SectorDataRequestBus::Broadcast(&GradientSignal::SectorDataRequestBus::Events::GetPointsPerMeter, pointsPerMeter);
        float pointSize = 1 / pointsPerMeter;

        MapHandle mapHandle;
        StaticVegetationRequestBus::BroadcastResult(mapHandle, &StaticVegetationRequestBus::Events::GetStaticVegetation);

        if (!mapHandle)
        {
            return;
        }

        AZ::Aabb boundsAabb = GetCachedBounds();
        AZ::Vector3 minBoundsPoint, maxBoundsPoint, boundsExtents;
        GetBoundsPointsAndExtents(boundsAabb, pointsPerMeter, minBoundsPoint, maxBoundsPoint, boundsExtents);

        int xAxisPoints = MetersToPoints(boundsExtents.GetX(), pointsPerMeter) + 1; // +1 because we want the number of points, not the index of the last point
        int yAxisPoints = MetersToPoints(boundsExtents.GetY(), pointsPerMeter) + 1;
        int size = xAxisPoints * yAxisPoints;

        if (xAxisPoints == 0 || yAxisPoints == 0)
        {
            return;
        }

        AZ::u64 mapUpdateCount = mapHandle.GetUpdateCount();
        // Update the mapping if needed
        if (mapUpdateCount != m_lastMapUpdateCount || m_vegetationMap.size() != size || m_boundsHaveChanged)
        {
            // We're only taking the lock for this narrow scope since this is the only place writing will occur
            // and this is only ever run on 1 thread
            AZStd::lock_guard<decltype(m_vegetationMapMutex)> scopedLock(m_vegetationMapMutex);

            m_vegetationMap.clear();
            m_vegetationMap.resize(size, VegetationPointData());

            for (const auto& pair : *mapHandle.Get())
            {
                AZ::Aabb vegAabb = GetPaddedAabb(pair.second.m_aabb, m_configuration.m_globalPadding);

                IterateIntersectingPoints(vegAabb, boundsAabb, pointsPerMeter, [this](int index, const AZ::Aabb& pointAabb)
                {
                    if (index < m_vegetationMap.size())
                    {
#if(ENABLE_STATIC_VEGETATION_DEBUG_TRACKING)
                        m_vegetationMap[index] = { true, pointAabb.GetCenter() };
#else
                        m_vegetationMap[index] = { true };
#endif
                    }
                });
            }

            m_boundsHaveChanged = false;
        }
        m_lastMapUpdateCount = mapUpdateCount;

        if (m_vegetationMap.empty())
        {
            return;
        }

        int numPoints = context.m_availablePoints.size();
        int baseX = MetersToPoints(minBoundsPoint.GetX(), pointsPerMeter);
        int baseY = MetersToPoints(minBoundsPoint.GetY(), pointsPerMeter);

        InstanceData instanceData;
        instanceData.m_id = GetEntityId();
        instanceData.m_changeIndex = GetChangeIndex();

        // Go through each point and if its inside the mapped area, remove claim points that are blocked out
        for (int i = 0; i < numPoints;)
        {
            // Calculate the index for the given claimPoint
            auto& claimPoint = context.m_availablePoints[i];
            int x = MetersToPoints(claimPoint.m_position.GetX(), pointsPerMeter) - baseX;
            int y = MetersToPoints(claimPoint.m_position.GetY(), pointsPerMeter) - baseY;
            int index = y * xAxisPoints + x;

            // If the claimPoint is within the map bounds and is claimed by static veg, remove the claimPoint
            if (x >= 0 && x < xAxisPoints && y >= 0 && index < m_vegetationMap.size() && m_vegetationMap[index].m_blocked)
            {
#if(ENABLE_STATIC_VEGETATION_DEBUG_TRACKING)
                AZ_Assert(claimPoint.m_position.GetX().IsClose(m_vegetationMap[index].m_worldPosition.GetX(), pointSize * 0.5f), "Points X do not match.  ClaimPoint: %f.  Stored Point: %f",
                    float(claimPoint.m_position.GetX()), float(m_vegetationMap[index].m_worldPosition.GetX()));

                AZ_Assert(claimPoint.m_position.GetY().IsClose(m_vegetationMap[index].m_worldPosition.GetY(), pointSize * 0.5f), "Points Y do not match.  ClaimPoint: %f.  Stored Point: %f",
                    float(claimPoint.m_position.GetY()), float(m_vegetationMap[index].m_worldPosition.GetY()));
#endif

                instanceData.m_position = claimPoint.m_position;
                instanceData.m_normal = claimPoint.m_normal;
                instanceData.m_masks = claimPoint.m_masks;

                context.m_createdCallback(claimPoint, instanceData);

                AZStd::swap(claimPoint, context.m_availablePoints.at(numPoints - 1));
                --numPoints;
            }
            else
            {
                ++i;
            }
        }

        context.m_availablePoints.resize(numPoints);
    }

    void StaticVegetationBlockerComponent::UnclaimPosition(const ClaimHandle handle)
    {
    }

    AZ::Aabb StaticVegetationBlockerComponent::GetEncompassingAabb() const
    {
        AZ::Aabb bounds = AZ::Aabb::CreateNull();
        LmbrCentral::ShapeComponentRequestsBus::EventResult(bounds, GetEntityId(), &LmbrCentral::ShapeComponentRequestsBus::Events::GetEncompassingAabb);
        return bounds;
    }

    AZ::u32 StaticVegetationBlockerComponent::GetProductCount() const
    {
        return 0;
    }

    void StaticVegetationBlockerComponent::InstanceAdded(UniqueVegetationInstancePtr vegetationInstance, AZ::Aabb aabb)
    {
        AZ::Aabb bounds = GetCachedBounds();

        if (aabb.Overlaps(bounds))
        {
            LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
        }
    }

    void StaticVegetationBlockerComponent::InstanceRemoved(UniqueVegetationInstancePtr vegetationInstance, AZ::Aabb aabb)
    {
        AZ::Aabb bounds = GetCachedBounds();
        
        if (aabb.Overlaps(bounds))
        {
            LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
        }
    }

    void StaticVegetationBlockerComponent::VegetationCleared()
    {
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    float StaticVegetationBlockerComponent::GetAreaPriority() const
    {
        return m_configuration.m_priority;
    }

    void StaticVegetationBlockerComponent::SetAreaPriority(float priority)
    {
        m_configuration.m_priority = priority;
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    AreaLayer StaticVegetationBlockerComponent::GetAreaLayer() const
    {
        return m_configuration.m_layer;
    }

    void StaticVegetationBlockerComponent::SetAreaLayer(AreaLayer layer)
    {
        m_configuration.m_layer = layer;
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    AZ::u32 StaticVegetationBlockerComponent::GetAreaProductCount() const
    {
        return GetProductCount();
    }

    float StaticVegetationBlockerComponent::GetBoundingBoxPadding() const
    {
        return m_configuration.m_globalPadding;
    }

    void StaticVegetationBlockerComponent::SetBoundingBoxPadding(float padding)
    {
        m_configuration.m_globalPadding = padding;
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    bool StaticVegetationBlockerComponent::GetShowBoundingBox() const
    {
        return m_configuration.m_showBoundingBox;
    }

    void StaticVegetationBlockerComponent::SetShowBoundingBox(bool value)
    {
        m_configuration.m_showBoundingBox = value;
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    bool StaticVegetationBlockerComponent::GetShowBlockedPoints() const
    {
        return m_configuration.m_showBlockedPoints;
    }

    void StaticVegetationBlockerComponent::SetShowBlockedPoints(bool value)
    {
        m_configuration.m_showBlockedPoints = value;
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    float StaticVegetationBlockerComponent::GetMaxDebugDrawDistance() const
    {
        return m_configuration.m_maxDebugRenderDistance;
    }

    void StaticVegetationBlockerComponent::SetMaxDebugDrawDistance(float distance)
    {
        m_configuration.m_maxDebugRenderDistance = distance;
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }
}