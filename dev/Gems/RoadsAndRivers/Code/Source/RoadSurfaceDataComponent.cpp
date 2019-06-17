/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensor's.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include "StdAfx.h"
#include "RoadSurfaceDataComponent.h"

#include <AzCore/Debug/Profiler.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <RoadsAndRivers/RoadsAndRiversBus.h>
#include <SurfaceData/SurfaceDataSystemRequestBus.h>
#include <SurfaceData/Utility/SurfaceDataUtility.h>

namespace RoadsAndRivers
{
    namespace Constants
    {
        static const AZ::Crc32 s_roadTagCrc = AZ::Crc32(s_roadTagName);
    }

    void RoadSurfaceDataConfig::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<RoadSurfaceDataConfig, AZ::ComponentConfig>()
                ->Version(1)
                ->Field("ProviderTags", &RoadSurfaceDataConfig::m_providerTags)
                ->Field("SnapToTerrain", &RoadSurfaceDataConfig::m_snapToTerrain)
                ;

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
            {
                edit->Class<RoadSurfaceDataConfig>(
                    "Road Surface Tag Emitter", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(0, &RoadSurfaceDataConfig::m_providerTags, "Generated Tags", "Surface tags to add to created points")
                    ->DataElement(0, &RoadSurfaceDataConfig::m_snapToTerrain, "Snap To Terrain", "Determine whether emitted surface follows the terrain or the spline geometry")
                    ;
            }
        }
    }

    void RoadSurfaceDataComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("SurfaceDataProviderService", 0xfe9fb95e));
    }

    void RoadSurfaceDataComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("SurfaceDataProviderService", 0xfe9fb95e));
    }

    void RoadSurfaceDataComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("RoadService", 0x8e6d7b29));
    }

    void RoadSurfaceDataComponent::Reflect(AZ::ReflectContext* context)
    {
        RoadSurfaceDataConfig::Reflect(context);

        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<RoadSurfaceDataComponent, AZ::Component>()
                ->Version(0)
                ->Field("Configuration", &RoadSurfaceDataComponent::m_configuration)
                ;
        }
    }

    RoadSurfaceDataComponent::RoadSurfaceDataComponent(const RoadSurfaceDataConfig& configuration)
        : m_configuration(configuration)
    {
    }

    void RoadSurfaceDataComponent::Activate()
    {
        AddItemIfNotFound(m_configuration.m_providerTags, Constants::s_roadTagCrc);

        SurfaceData::SurfaceDataRegistryEntry registryEntry;
        registryEntry.m_entityId = GetEntityId();
        registryEntry.m_bounds = m_shapeBounds;
        registryEntry.m_tags = m_configuration.m_providerTags;

        m_providerHandle = SurfaceData::InvalidSurfaceDataRegistryHandle;
        SurfaceData::SurfaceDataSystemRequestBus::BroadcastResult(m_providerHandle, &SurfaceData::SurfaceDataSystemRequestBus::Events::RegisterSurfaceDataProvider, registryEntry);
        SurfaceData::SurfaceDataProviderRequestBus::Handler::BusConnect(m_providerHandle);

        AZ::TransformNotificationBus::Handler::BusConnect(GetEntityId());
        LmbrCentral::SplineComponentNotificationBus::Handler::BusConnect(GetEntityId());
        RoadNotificationBus::Handler::BusConnect(GetEntityId());
        RoadsAndRiversGeometryNotificationBus::Handler::BusConnect(GetEntityId());

        UpdateShapeData();
        m_refresh = false;
    }

    void RoadSurfaceDataComponent::Deactivate()
    {
        m_refresh = false;
        AZ::TickBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::Handler::BusDisconnect();
        RoadsAndRiversGeometryNotificationBus::Handler::BusDisconnect();
        RoadNotificationBus::Handler::BusDisconnect();
        LmbrCentral::SplineComponentNotificationBus::Handler::BusDisconnect();

        SurfaceData::SurfaceDataProviderRequestBus::Handler::BusDisconnect();
        SurfaceData::SurfaceDataSystemRequestBus::Broadcast(&SurfaceData::SurfaceDataSystemRequestBus::Events::UnregisterSurfaceDataProvider, m_providerHandle);
        m_providerHandle = SurfaceData::InvalidSurfaceDataRegistryHandle;
    }

    bool RoadSurfaceDataComponent::ReadInConfig(const AZ::ComponentConfig* baseConfig)
    {
        if (auto config = azrtti_cast<const RoadSurfaceDataConfig*>(baseConfig))
        {
            m_configuration = *config;
            return true;
        }
        return false;
    }

    bool RoadSurfaceDataComponent::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
    {
        if (auto config = azrtti_cast<RoadSurfaceDataConfig*>(outBaseConfig))
        {
            *config = m_configuration;
            return true;
        }
        return false;
    }

    void RoadSurfaceDataComponent::GetSurfacePoints(const AZ::Vector3& inPosition, SurfaceData::SurfacePointList& surfacePointList) const
    {
        AZStd::lock_guard<decltype(m_cacheMutex)> lock(m_cacheMutex);

        if (m_shapeBoundsIsValid)
        {
            const AZ::Vector3 rayOrigin(inPosition.GetX(), inPosition.GetY(), m_shapeBounds.GetMax().GetZ());
            const AZ::Vector3 rayDirection = -AZ::Vector3::CreateAxisZ();
            AZ::Vector3 resultPosition = rayOrigin;
            AZ::Vector3 resultNormal = AZ::Vector3::CreateAxisZ();
            if (SurfaceData::GetQuadListRayIntersection(m_shapeVertices, rayOrigin, rayDirection, m_shapeBounds.GetDepth(), resultPosition, resultNormal))
            {
                SurfaceData::SurfacePoint point;
                point.m_entityId = GetEntityId();

                if (m_configuration.m_snapToTerrain)
                { 
                    auto engine = GetISystem()->GetI3DEngine();
                    if (engine && engine->GetShowTerrainSurface() && engine->GetTerrainAabb().Contains(inPosition))
                    {
                        const float x = inPosition.GetX();
                        const float y = inPosition.GetY();

                        if (m_ignoreTerrainHoles || !engine->GetTerrainHole(int(x), int(y)))
                        {
                            const Vec3 positionLy = AZVec3ToLYVec3(inPosition);
                            const float terrainHeight = engine->GetTerrainElevation(x, y);

                            point.m_position = AZ::Vector3(x, y, terrainHeight);
                            point.m_normal = LYVec3ToAZVec3(engine->GetTerrainSurfaceNormal(positionLy));
                            AddMaxValueForMasks(point.m_masks, m_configuration.m_providerTags, 1.0f);
                            surfacePointList.push_back(point);
                        }
                    }
                }
                else
                {
                    point.m_position = resultPosition;
                    point.m_normal = resultNormal;
                    AddMaxValueForMasks(point.m_masks, m_configuration.m_providerTags, 1.0f);
                    surfacePointList.push_back(point);
                }

            }
        }
    }

    void RoadSurfaceDataComponent::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& /*world*/)
    {
        OnCompositionChanged();
    }

    void RoadSurfaceDataComponent::OnSplineChanged()
    {
        OnCompositionChanged();
    }

    void RoadSurfaceDataComponent::OnWidthChanged()
    {
        OnCompositionChanged();
    }

    void RoadSurfaceDataComponent::OnSegmentLengthChanged(float /*segmentLength*/)
    {
        OnCompositionChanged();
    }

    void RoadSurfaceDataComponent::OnIgnoreTerrainHolesChanged(bool ignoreHoles)
    {
        OnCompositionChanged();
    }

    void RoadSurfaceDataComponent::OnTick(float /*deltaTime*/, AZ::ScriptTimePoint /*time*/)
    {
        if (m_refresh)
        {
            UpdateShapeData();
            m_refresh = false;
        }
        AZ::TickBus::Handler::BusDisconnect();
    }

    void RoadSurfaceDataComponent::OnCompositionChanged()
    {
        if (!m_refresh)
        {
            m_refresh = true;
            AZ::TickBus::Handler::BusConnect();
        }
    }

    void RoadSurfaceDataComponent::UpdateShapeData()
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Entity);

        AZ::Aabb dirtyVolume = AZ::Aabb::CreateNull();

        {
            AZStd::lock_guard<decltype(m_cacheMutex)> lock(m_cacheMutex);

            // To make sure we update our surface data even if our surface shrinks or grows, keep track of a bounding box
            // that contains the old surface volume.  At the end, we'll add in our new surface volume.
            if (m_shapeBoundsIsValid)
            {
                dirtyVolume.AddAabb(m_shapeBounds);
            }

            m_shapeWorldTM = AZ::Transform::CreateIdentity();
            AZ::TransformBus::EventResult(m_shapeWorldTM, GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);

            RoadsAndRivers::RoadRequestBus::EventResult(m_ignoreTerrainHoles, GetEntityId(), &RoadsAndRivers::RoadRequestBus::Events::GetIgnoreTerrainHoles);


            m_shapeVertices.clear();
            RoadsAndRivers::RoadsAndRiversGeometryRequestsBus::EventResult(m_shapeVertices, GetEntityId(), &RoadsAndRivers::RoadsAndRiversGeometryRequestsBus::Events::GetQuadVertices);

            m_shapeBounds = AZ::Aabb::CreateNull();
            for (AZ::Vector3& vertex : m_shapeVertices)
            {
                vertex = m_shapeWorldTM * vertex;
                m_shapeBounds.AddPoint(vertex);
            }
            m_shapeBoundsIsValid = m_shapeBounds.IsValid();

            // Add our new surface volume to our dirty volume, so that we have a bounding box that encompasses everything that's been added / changed / removed
            if (m_shapeBoundsIsValid)
            {
                dirtyVolume.AddAabb(m_shapeBounds);
            }
        }

        SurfaceData::SurfaceDataRegistryEntry registryEntry;
        registryEntry.m_entityId = GetEntityId();
        registryEntry.m_bounds = m_shapeBounds;
        registryEntry.m_tags = m_configuration.m_providerTags;
        SurfaceData::SurfaceDataSystemRequestBus::Broadcast(&SurfaceData::SurfaceDataSystemRequestBus::Events::UpdateSurfaceDataProvider, m_providerHandle, registryEntry, dirtyVolume);
    }
}