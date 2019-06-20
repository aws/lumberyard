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
#include "RiverSurfaceDataComponent.h"

#include <AzCore/Debug/Profiler.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <RoadsAndRivers/RoadsAndRiversBus.h>
#include <SurfaceData/SurfaceDataSystemRequestBus.h>
#include <SurfaceData/Utility/SurfaceDataUtility.h>

#include <I3DEngine.h>
#include <ISystem.h>

namespace RoadsAndRivers
{
    namespace Constants
    {
        static const AZ::Crc32 s_underWaterTagCrc = AZ::Crc32(s_underWaterTagName);
        static const AZ::Crc32 s_waterTagCrc = AZ::Crc32(s_waterTagName);
        static const AZ::Crc32 s_riverTagCrc = AZ::Crc32(s_riverTagName);
    }

    void RiverSurfaceDataConfig::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<RiverSurfaceDataConfig, AZ::ComponentConfig>()
                ->Version(1)
                ->Field("ProviderTags", &RiverSurfaceDataConfig::m_providerTags)
                ->Field("ModifierTags", &RiverSurfaceDataConfig::m_modifierTags)
                ->Field("SnapToTerrain", &RiverSurfaceDataConfig::m_snapToRiverSurface)
                ;

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
            {
                edit->Class<RiverSurfaceDataConfig>(
                    "River Surface Tag Emitter", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(0, &RiverSurfaceDataConfig::m_providerTags, "Generated Tags", "Surface tags to add to created points")
                    ->DataElement(0, &RiverSurfaceDataConfig::m_modifierTags, "Extended Tags", "Surface tags to add to contained points")
                    ->DataElement(0, &RiverSurfaceDataConfig::m_snapToRiverSurface, "Snap To River Surface", "Determine whether the emitted surface follows the river surface or the spline geometry")
                    ;
            }
        }
    }

    void RiverSurfaceDataComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("SurfaceDataProviderService", 0xfe9fb95e));
        services.push_back(AZ_CRC("SurfaceDataModifierService", 0x68f8aa72));
    }

    void RiverSurfaceDataComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("SurfaceDataProviderService", 0xfe9fb95e));
        services.push_back(AZ_CRC("SurfaceDataModifierService", 0x68f8aa72));
    }

    void RiverSurfaceDataComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("RiverService", 0xe71a93cc));
    }

    void RiverSurfaceDataComponent::Reflect(AZ::ReflectContext* context)
    {
        RiverSurfaceDataConfig::Reflect(context);

        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<RiverSurfaceDataComponent, AZ::Component>()
                ->Version(0)
                ->Field("Configuration", &RiverSurfaceDataComponent::m_configuration)
                ;
        }
    }

    RiverSurfaceDataComponent::RiverSurfaceDataComponent(const RiverSurfaceDataConfig& configuration)
        : m_configuration(configuration)
    {
    }

    void RiverSurfaceDataComponent::Activate()
    {
        m_system = GetISystem();
        AddItemIfNotFound(m_configuration.m_providerTags, Constants::s_riverTagCrc);
        AddItemIfNotFound(m_configuration.m_providerTags, Constants::s_waterTagCrc);
        AddItemIfNotFound(m_configuration.m_modifierTags, Constants::s_underWaterTagCrc);

        SurfaceData::SurfaceDataRegistryEntry registryEntry;
        registryEntry.m_entityId = GetEntityId();
        registryEntry.m_bounds = m_surfaceShapeBounds;
        registryEntry.m_tags = m_configuration.m_providerTags;

        m_providerHandle = SurfaceData::InvalidSurfaceDataRegistryHandle;
        SurfaceData::SurfaceDataSystemRequestBus::BroadcastResult(m_providerHandle, &SurfaceData::SurfaceDataSystemRequestBus::Events::RegisterSurfaceDataProvider, registryEntry);
        SurfaceData::SurfaceDataProviderRequestBus::Handler::BusConnect(m_providerHandle);

        registryEntry.m_bounds = m_volumeShapeBounds;
        registryEntry.m_tags = m_configuration.m_modifierTags;

        m_modifierHandle = SurfaceData::InvalidSurfaceDataRegistryHandle;
        SurfaceData::SurfaceDataSystemRequestBus::BroadcastResult(m_modifierHandle, &SurfaceData::SurfaceDataSystemRequestBus::Events::RegisterSurfaceDataModifier, registryEntry);
        SurfaceData::SurfaceDataModifierRequestBus::Handler::BusConnect(m_modifierHandle);

        AZ::TransformNotificationBus::Handler::BusConnect(GetEntityId());
        LmbrCentral::SplineComponentNotificationBus::Handler::BusConnect(GetEntityId());
        RiverNotificationBus::Handler::BusConnect(GetEntityId());
        RoadsAndRiversGeometryNotificationBus::Handler::BusConnect(GetEntityId());
        CrySystemEventBus::Handler::BusConnect();

        UpdateShapeData();
        m_refresh = false;
    }

    void RiverSurfaceDataComponent::Deactivate()
    {
        m_system = nullptr;
        m_refresh = false;
        CrySystemEventBus::Handler::BusDisconnect();
        AZ::TickBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::Handler::BusDisconnect();
        RoadsAndRiversGeometryNotificationBus::Handler::BusDisconnect();
        RiverNotificationBus::Handler::BusDisconnect();
        LmbrCentral::SplineComponentNotificationBus::Handler::BusDisconnect();

        SurfaceData::SurfaceDataProviderRequestBus::Handler::BusDisconnect();
        SurfaceData::SurfaceDataSystemRequestBus::Broadcast(&SurfaceData::SurfaceDataSystemRequestBus::Events::UnregisterSurfaceDataProvider, m_providerHandle);
        m_providerHandle = SurfaceData::InvalidSurfaceDataRegistryHandle;

        SurfaceData::SurfaceDataModifierRequestBus::Handler::BusDisconnect();
        SurfaceData::SurfaceDataSystemRequestBus::Broadcast(&SurfaceData::SurfaceDataSystemRequestBus::Events::UnregisterSurfaceDataModifier, m_modifierHandle);
        m_modifierHandle = SurfaceData::InvalidSurfaceDataRegistryHandle;
    }

    bool RiverSurfaceDataComponent::ReadInConfig(const AZ::ComponentConfig* baseConfig)
    {
        if (auto config = azrtti_cast<const RiverSurfaceDataConfig*>(baseConfig))
        {
            m_configuration = *config;
            return true;
        }
        return false;
    }

    bool RiverSurfaceDataComponent::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
    {
        if (auto config = azrtti_cast<RiverSurfaceDataConfig*>(outBaseConfig))
        {
            *config = m_configuration;
            return true;
        }
        return false;
    }

    void RiverSurfaceDataComponent::GetSurfacePoints(const AZ::Vector3& inPosition, SurfaceData::SurfacePointList& surfacePointList) const
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Entity);

        AZStd::lock_guard<decltype(m_cacheMutex)> lock(m_cacheMutex);

        auto engine = m_system ? m_system->GetI3DEngine() : nullptr;
        if (engine && m_surfaceShapeBoundsIsValid)
        {
            const AZ::Vector3 rayOrigin(inPosition.GetX(), inPosition.GetY(), m_surfaceShapeBounds.GetMax().GetZ());
            const AZ::Vector3 rayDirection = -AZ::Vector3::CreateAxisZ();
            AZ::Vector3 resultPosition = rayOrigin;
            AZ::Vector3 resultNormal = AZ::Vector3::CreateAxisZ();
            if (SurfaceData::GetQuadListRayIntersection(m_shapeVertices, rayOrigin, rayDirection, m_surfaceShapeBounds.GetDepth(), resultPosition, resultNormal))
            {
                const float terrainHeight = engine->GetTerrainElevation(resultPosition.GetX(), resultPosition.GetY());
                if (!engine->GetShowTerrainSurface() || !engine->GetTerrainAabb().Contains(resultPosition) || resultPosition.GetZ() >= terrainHeight)
                {
                    SurfaceData::SurfacePoint point;
                    point.m_entityId = GetEntityId();
                    point.m_position = resultPosition;
                    point.m_normal = resultNormal;
                    AddMaxValueForMasks(point.m_masks, m_configuration.m_providerTags, 1.0f);
                    surfacePointList.push_back(point);
                }
            }
        }
    }

    void RiverSurfaceDataComponent::ModifySurfacePoints(SurfaceData::SurfacePointList& surfacePointList) const
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Entity);

        AZStd::lock_guard<decltype(m_cacheMutex)> lock(m_cacheMutex);

        auto engine = m_system ? m_system->GetI3DEngine() : nullptr;
        if (engine && m_volumeShapeBoundsIsValid && !m_configuration.m_modifierTags.empty())
        {
            const AZ::EntityId entityId = GetEntityId();
            for (auto& point : surfacePointList)
            {
                const AZ::VectorFloat maxHeight = m_volumeShapeBounds.GetMax().GetZ();

                //input point must be below highest river point to even test
                if (point.m_entityId != entityId && point.m_position.GetZ() <= maxHeight)
                {
                    const AZ::Vector3 rayOrigin(point.m_position.GetX(), point.m_position.GetY(), maxHeight);
                    if (m_volumeShapeBounds.Contains(rayOrigin))
                    {
                        const AZ::Vector3 rayDirection = -AZ::Vector3::CreateAxisZ();
                        AZ::Vector3 resultPosition = rayOrigin;
                        AZ::Vector3 resultNormal = AZ::Vector3::CreateAxisZ();
                        if (SurfaceData::GetQuadListRayIntersection(m_shapeVertices, rayOrigin, rayDirection, m_volumeShapeBounds.GetDepth(), resultPosition, resultNormal))
                        {
                            //input point must be within river depth at intersection point
                            if ((point.m_position.GetZ() <= resultPosition.GetZ()) && (point.m_position.GetZ() >= (resultPosition.GetZ() - m_riverDepth)))
                            {
                                //intersection point must be above terrain
                                const float terrainHeight = engine->GetTerrainElevation(point.m_position.GetX(), point.m_position.GetY());
                                if (!engine->GetShowTerrainSurface() || !engine->GetTerrainAabb().Contains(point.m_position) || point.m_position.GetZ() >= terrainHeight)
                                {
                                    AddMaxValueForMasks(point.m_masks, m_configuration.m_modifierTags, 1.0f);
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    void RiverSurfaceDataComponent::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& /*world*/)
    {
        OnCompositionChanged();
    }

    void RiverSurfaceDataComponent::OnSplineChanged()
    {
        OnCompositionChanged();
    }

    void RiverSurfaceDataComponent::OnWidthChanged()
    {
        OnCompositionChanged();
    }
    
    void RiverSurfaceDataComponent::OnSegmentLengthChanged(float /*segmentLength*/)
    {
        OnCompositionChanged();
    }

    void RiverSurfaceDataComponent::OnWaterVolumeDepthChanged(float depth)
    {
        if (depth != m_riverDepth)
        {
            OnCompositionChanged();
        }
    }


    void RiverSurfaceDataComponent::OnTick(float /*deltaTime*/, AZ::ScriptTimePoint /*time*/)
    {
        if (m_refresh)
        {
            UpdateShapeData();
            m_refresh = false;
        }
        AZ::TickBus::Handler::BusDisconnect();
    }

    void RiverSurfaceDataComponent::OnCrySystemInitialized(ISystem& system, const SSystemInitParams& systemInitParams)
    {
        m_system = &system;
    }

    void RiverSurfaceDataComponent::OnCrySystemShutdown(ISystem& system)
    {
        m_system = nullptr;
    }

    void RiverSurfaceDataComponent::OnCompositionChanged()
    {
        if (!m_refresh)
        {
            m_refresh = true;
            AZ::TickBus::Handler::BusConnect();
        }
    }

    void RiverSurfaceDataComponent::UpdateShapeData()
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Entity);

        AZ::Aabb dirtySurfaceVolume = AZ::Aabb::CreateNull();
        AZ::Aabb dirtyModifierVolume = AZ::Aabb::CreateNull();

        {
            AZStd::lock_guard<decltype(m_cacheMutex)> lock(m_cacheMutex);

            m_shapeWorldTM = AZ::Transform::CreateIdentity();
            AZ::TransformBus::EventResult(m_shapeWorldTM, GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);

            float depth = 0.0f;
            RoadsAndRivers::RiverRequestBus::EventResult(depth, GetEntityId(), &RoadsAndRivers::RiverRequestBus::Events::GetWaterVolumeDepth);
            m_riverDepth = depth;

            m_shapeVertices.clear();
            RoadsAndRivers::RoadsAndRiversGeometryRequestsBus::EventResult(m_shapeVertices, GetEntityId(), &RoadsAndRivers::RoadsAndRiversGeometryRequestsBus::Events::GetQuadVertices);

            AZ::Plane surfacePlane;
            RoadsAndRivers::RiverRequestBus::EventResult(surfacePlane, GetEntityId(), &RoadsAndRivers::RiverRequestBus::Events::GetWaterSurfacePlane);

            // To make sure we update our surface data even if our surface shrinks or grows, keep track of a bounding box
            // that contains the old surface volume.  At the end, we'll add in our new surface volume.
            if (m_surfaceShapeBoundsIsValid)
            {
                dirtySurfaceVolume.AddAabb(m_surfaceShapeBounds);
            }
            if (m_volumeShapeBoundsIsValid)
            {
                dirtyModifierVolume.AddAabb(m_volumeShapeBounds);
            }

            m_surfaceShapeBounds = AZ::Aabb::CreateNull();
            m_volumeShapeBounds = AZ::Aabb::CreateNull();

            const AZ::Vector4 planeEquation = surfacePlane.GetPlaneEquationCoefficients();
            AZ::VectorFloat planeEquationZ = planeEquation.GetZ().IsZero() ? AZ::g_fltEps : planeEquation.GetZ();

            for (AZ::Vector3& vertex : m_shapeVertices)
            {
                // Remap the river surface geometry from local to world
                vertex = m_shapeWorldTM * vertex;

                if (m_configuration.m_snapToRiverSurface)
                {
                    // Project the spline-based surface geometry down to the water surface plane to match what's rendered and physicalized.
                    // Specifically, we need to match the projection happening in WaterVolumeRenderNode (see MapVertexToFogPlane).  
                    // The expected projection is that we take each XY vertex in our shape geometry and project it vertically along Z
                    // onto the plane.  (We are NOT projecting along the plane's normal)
                    // The easiest way to do this is just to get the plane's Z value where our XY vertex is and set our Z value to that.
                    // Since the plane equation is Ax+By+Cz+D=0, solving for z we get z = -(Ax+By+D)/C

                    AZ::VectorFloat projectedZ = -((planeEquation.GetX() * vertex.GetX()) + (planeEquation.GetY() * vertex.GetY()) + planeEquation.GetW()) / planeEquationZ;
                    vertex.SetZ(projectedZ);
                }

                // Add the point to the overall bounds of the shape, so that we can limit vegetation raycast checks to the proper volume
                m_surfaceShapeBounds.AddPoint(vertex);

                // Add an additional point that represents the vertex projected out to the full depth of the river.
                // Note: River depth is always in absolute Z space, it doesn't rotate.
                AZ::Vector3 depthVertex(vertex.GetX(), vertex.GetY(), vertex.GetZ() - m_riverDepth);
                m_volumeShapeBounds.AddPoint(vertex);
                m_volumeShapeBounds.AddPoint(depthVertex);
            }
            m_surfaceShapeBoundsIsValid = m_surfaceShapeBounds.IsValid();
            m_volumeShapeBoundsIsValid = m_volumeShapeBounds.IsValid();

            // Add our new surface volume to our dirty volume, so that we have a bounding box that encompasses everything that's been added / changed / removed
            if (m_surfaceShapeBoundsIsValid)
            {
                dirtySurfaceVolume.AddAabb(m_surfaceShapeBounds);
            }
            if (m_volumeShapeBoundsIsValid)
            {
                dirtyModifierVolume.AddAabb(m_volumeShapeBounds);
            }
        }

        SurfaceData::SurfaceDataRegistryEntry registryEntry;
        registryEntry.m_entityId = GetEntityId();
        registryEntry.m_bounds = m_surfaceShapeBounds;
        registryEntry.m_tags = m_configuration.m_providerTags;
        SurfaceData::SurfaceDataSystemRequestBus::Broadcast(&SurfaceData::SurfaceDataSystemRequestBus::Events::UpdateSurfaceDataProvider, m_providerHandle, registryEntry, dirtySurfaceVolume);

        registryEntry.m_bounds = m_volumeShapeBounds;
        registryEntry.m_tags = m_configuration.m_modifierTags;
        SurfaceData::SurfaceDataSystemRequestBus::Broadcast(&SurfaceData::SurfaceDataSystemRequestBus::Events::UpdateSurfaceDataModifier, m_modifierHandle, registryEntry, dirtyModifierVolume);
    }
}