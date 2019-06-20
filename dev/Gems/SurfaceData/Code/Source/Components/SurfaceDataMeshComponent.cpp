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

#include "SurfaceData_precompiled.h"
#include "SurfaceDataMeshComponent.h"

#include <AzCore/Debug/Profiler.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <LmbrCentral/Rendering/MeshAsset.h>

#include <SurfaceData/SurfaceDataSystemRequestBus.h>
#include <SurfaceData/Utility/SurfaceDataUtility.h>

#include <Cry_Matrix34.h>
#include <Cry_GeoIntersect.h>
#include <Cry_Geo.h>
#include <IStatObj.h>
#include <MathConversion.h>

namespace SurfaceData
{
    void SurfaceDataMeshConfig::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<SurfaceDataMeshConfig, AZ::ComponentConfig>()
                ->Version(0)
                ->Field("SurfaceTags", &SurfaceDataMeshConfig::m_tags)
                ;

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
            {
                edit->Class<SurfaceDataMeshConfig>(
                    "Mesh Surface Tag Emitter", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(0, &SurfaceDataMeshConfig::m_tags, "Generated Tags", "")
                    ;
            }
        }
    }

    void SurfaceDataMeshComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("SurfaceDataProviderService", 0xfe9fb95e));
    }

    void SurfaceDataMeshComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("SurfaceDataProviderService", 0xfe9fb95e));
    }

    void SurfaceDataMeshComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("MeshService", 0x71d8a455));
    }

    void SurfaceDataMeshComponent::Reflect(AZ::ReflectContext* context)
    {
        SurfaceDataMeshConfig::Reflect(context);

        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<SurfaceDataMeshComponent, AZ::Component>()
                ->Version(0)
                ->Field("Configuration", &SurfaceDataMeshComponent::m_configuration)
                ;
        }
    }

    SurfaceDataMeshComponent::SurfaceDataMeshComponent(const SurfaceDataMeshConfig& configuration)
        : m_configuration(configuration)
    {
    }

    void SurfaceDataMeshComponent::Activate()
    {
        AZ::TransformNotificationBus::Handler::BusConnect(GetEntityId());
        LmbrCentral::MeshComponentNotificationBus::Handler::BusConnect(GetEntityId());

        SurfaceDataRegistryEntry registryEntry;
        registryEntry.m_entityId = GetEntityId();
        registryEntry.m_bounds = GetSurfaceAabb();
        registryEntry.m_tags = GetSurfaceTags();

        m_providerHandle = InvalidSurfaceDataRegistryHandle;
        SurfaceDataSystemRequestBus::BroadcastResult(m_providerHandle, &SurfaceDataSystemRequestBus::Events::RegisterSurfaceDataProvider, registryEntry);
        SurfaceDataProviderRequestBus::Handler::BusConnect(m_providerHandle);

        UpdateMeshData();
        m_refresh = false;
    }

    void SurfaceDataMeshComponent::Deactivate()
    {
        SurfaceDataSystemRequestBus::Broadcast(&SurfaceDataSystemRequestBus::Events::UnregisterSurfaceDataProvider, m_providerHandle);
        m_providerHandle = InvalidSurfaceDataRegistryHandle;

        SurfaceDataProviderRequestBus::Handler::BusDisconnect();
        AZ::TickBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::Handler::BusDisconnect();
        LmbrCentral::MeshComponentNotificationBus::Handler::BusDisconnect();
        m_refresh = false;
    }

    bool SurfaceDataMeshComponent::ReadInConfig(const AZ::ComponentConfig* baseConfig)
    {
        if (auto config = azrtti_cast<const SurfaceDataMeshConfig*>(baseConfig))
        {
            m_configuration = *config;
            return true;
        }
        return false;
    }

    bool SurfaceDataMeshComponent::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
    {
        if (auto config = azrtti_cast<SurfaceDataMeshConfig*>(outBaseConfig))
        {
            *config = m_configuration;
            return true;
        }
        return false;
    }

    bool SurfaceDataMeshComponent::DoRayTrace(const AZ::Vector3& inPosition, AZ::Vector3& outPosition, AZ::Vector3& outNormal) const
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Entity);

        AZStd::lock_guard<decltype(m_cacheMutex)> lock(m_cacheMutex);

        // test AABB as first pass to claim the point
        const AZ::Vector3 testPosition = AZ::Vector3(
            inPosition.GetX(),
            inPosition.GetY(),
            (m_meshBounds.GetMax().GetZ() + m_meshBounds.GetMin().GetZ()) * 0.5f);

        if (!m_meshBounds.Contains(testPosition))
        {
            return false;
        }

        LmbrCentral::MeshAsset* mesh = m_meshAssetData.GetAs<LmbrCentral::MeshAsset>();
        if (!mesh)
        {
            return false;
        }

        const AZ::Vector3 rayOrigin = AZ::Vector3(inPosition.GetX(), inPosition.GetY(), m_meshBounds.GetMax().GetZ() + s_rayAABBHeightPadding);
        const AZ::Vector3 rayDirection = -AZ::Vector3::CreateAxisZ();
        return GetMeshRayIntersection(*mesh, m_meshWorldTM, m_meshWorldTMInverse, rayOrigin, rayDirection, outPosition, outNormal);
    }


    void SurfaceDataMeshComponent::GetSurfacePoints(const AZ::Vector3& inPosition, SurfacePointList& surfacePointList) const
    {
        AZ::Vector3 hitPosition;
        AZ::Vector3 hitNormal;
        if (DoRayTrace(inPosition, hitPosition, hitNormal))
        {
            SurfacePoint point;
            point.m_entityId = GetEntityId();
            point.m_position = hitPosition;
            point.m_normal = hitNormal;
            AddMaxValueForMasks(point.m_masks, m_configuration.m_tags, 1.0f);
            surfacePointList.push_back(point);
        }
    }

    AZ::Aabb SurfaceDataMeshComponent::GetSurfaceAabb() const
    {
        return m_meshBounds;
    }

    SurfaceTagVector SurfaceDataMeshComponent::GetSurfaceTags() const
    {
        return m_configuration.m_tags;
    }

    void SurfaceDataMeshComponent::OnCompositionChanged()
    {
        if (!m_refresh)
        {
            m_refresh = true;
            AZ::TickBus::Handler::BusConnect();
        }
    }

    void SurfaceDataMeshComponent::OnMeshDestroyed()
    {
        OnCompositionChanged();
    }

    void SurfaceDataMeshComponent::OnMeshCreated(const AZ::Data::Asset<AZ::Data::AssetData>& asset)
    {
        (void)asset;
        OnCompositionChanged();
    }

    void SurfaceDataMeshComponent::OnBoundsReset()
    {
        OnCompositionChanged();
    }

    void SurfaceDataMeshComponent::OnTransformChanged(const AZ::Transform & local, const AZ::Transform & world)
    {
        (void)local;
        (void)world;
        OnCompositionChanged();
    }

    void SurfaceDataMeshComponent::OnTick(float /*deltaTime*/, AZ::ScriptTimePoint /*time*/)
    {
        if (m_refresh)
        {
            UpdateMeshData();
            m_refresh = false;
        }
        AZ::TickBus::Handler::BusDisconnect();
    }

    void SurfaceDataMeshComponent::UpdateMeshData()
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Entity);

        AZ::Aabb dirtyVolume = AZ::Aabb::CreateNull();

        {
            AZStd::lock_guard<decltype(m_cacheMutex)> lock(m_cacheMutex);

            // To make sure we update our surface data even if our surface shrinks or grows, keep track of a bounding box
            // that contains the old surface volume.  At the end, we'll add in our new surface volume.
            if (m_meshBounds.IsValid())
            {
                dirtyVolume.AddAabb(m_meshBounds);
            }

            m_meshAssetData = {};
            LmbrCentral::MeshComponentRequestBus::EventResult(m_meshAssetData, GetEntityId(), &LmbrCentral::MeshComponentRequestBus::Events::GetMeshAsset);

            m_meshBounds = AZ::Aabb::CreateNull();
            LmbrCentral::MeshComponentRequestBus::EventResult(m_meshBounds, GetEntityId(), &LmbrCentral::MeshComponentRequestBus::Events::GetWorldBounds);

            m_meshWorldTM = AZ::Transform::CreateIdentity();
            AZ::TransformBus::EventResult(m_meshWorldTM, GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);
            m_meshWorldTMInverse = m_meshWorldTM.GetInverseFull();

            // Add our new surface volume to our dirty volume, so that we have a bounding box that encompasses everything that's been added / changed / removed
            if (m_meshBounds.IsValid())
            {
                dirtyVolume.AddAabb(m_meshBounds);
            }
        }

        SurfaceDataRegistryEntry registryEntry;
        registryEntry.m_entityId = GetEntityId();
        registryEntry.m_bounds = GetSurfaceAabb();
        registryEntry.m_tags = GetSurfaceTags();
        SurfaceDataSystemRequestBus::Broadcast(&SurfaceDataSystemRequestBus::Events::UpdateSurfaceDataProvider, m_providerHandle, registryEntry, dirtyVolume);
    }
}
