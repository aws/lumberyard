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

#include "SurfaceData_precompiled.h"

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/std/sort.h>

#include "SurfaceDataSystemComponent.h"
#include <SurfaceData/SurfaceDataConstants.h>
#include <SurfaceData/SurfaceTag.h>
#include <SurfaceData/SurfaceDataSystemNotificationBus.h>
#include <SurfaceData/SurfaceDataProviderRequestBus.h>
#include <SurfaceData/SurfaceDataModifierRequestBus.h>
#include <SurfaceData/Utility/SurfaceDataUtility.h>


namespace SurfaceData
{
    void SurfaceDataSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        SurfaceTag::Reflect(context);

        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<SurfaceDataSystemComponent, AZ::Component>()
                ->Version(0)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<SurfaceDataSystemComponent>("Surface Data System", "Manages registration of surface data providers and forwards intersection data requests to them")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<SurfacePoint>()
                ->Constructor()
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::Preview)
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Property("entityId", BehaviorValueProperty(&SurfacePoint::m_entityId))
                ->Property("position", BehaviorValueProperty(&SurfacePoint::m_position))
                ->Property("normal", BehaviorValueProperty(&SurfacePoint::m_normal))
                ->Property("masks", BehaviorValueProperty(&SurfacePoint::m_masks))
                ;

            behaviorContext->Class<SurfaceDataSystemComponent>()->RequestBus("SurfaceDataSystemRequestBus");

            behaviorContext->EBus<SurfaceDataSystemRequestBus>("SurfaceDataSystemRequestBus")
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::Preview)
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Event("GetSurfacePoints", &SurfaceDataSystemRequestBus::Events::GetSurfacePoints)
                ;
        }
    }

    void SurfaceDataSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("SurfaceDataSystemService", 0x1d44d25f));
    }

    void SurfaceDataSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("SurfaceDataSystemService", 0x1d44d25f));
    }

    void SurfaceDataSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        (void)required;
    }

    void SurfaceDataSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        (void)dependent;
    }

    void SurfaceDataSystemComponent::Init()
    {
    }

    void SurfaceDataSystemComponent::Activate()
    {
        SurfaceDataSystemRequestBus::Handler::BusConnect();
    }

    void SurfaceDataSystemComponent::Deactivate()
    {
        SurfaceDataSystemRequestBus::Handler::BusDisconnect();
    }

    SurfaceDataRegistryHandle SurfaceDataSystemComponent::RegisterSurfaceDataProvider(const SurfaceDataRegistryEntry& entry)
    {
        const SurfaceDataRegistryHandle handle = RegisterSurfaceDataProviderInternal(entry);
        if (handle != InvalidSurfaceDataRegistryHandle)
        {
            SurfaceDataSystemNotificationBus::Broadcast(&SurfaceDataSystemNotificationBus::Events::OnSurfaceChanged, entry.m_entityId, entry.m_bounds);
        }
        return handle;
    }

    void SurfaceDataSystemComponent::UnregisterSurfaceDataProvider(const SurfaceDataRegistryHandle& handle)
    {
        const SurfaceDataRegistryEntry entry = UnregisterSurfaceDataProviderInternal(handle);
        if (entry.m_entityId.IsValid())
        {
            SurfaceDataSystemNotificationBus::Broadcast(&SurfaceDataSystemNotificationBus::Events::OnSurfaceChanged, entry.m_entityId, entry.m_bounds);
        }
    }

    void SurfaceDataSystemComponent::UpdateSurfaceDataProvider(const SurfaceDataRegistryHandle& handle, const SurfaceDataRegistryEntry& entry, const AZ::Aabb& dirtyBoundsOverride)
    {
        if (UpdateSurfaceDataProviderInternal(handle, entry))
        {
            const auto& bounds = dirtyBoundsOverride.IsValid() ? dirtyBoundsOverride : entry.m_bounds;
            SurfaceDataSystemNotificationBus::Broadcast(&SurfaceDataSystemNotificationBus::Events::OnSurfaceChanged, entry.m_entityId, bounds);
        }
    }

    SurfaceDataRegistryHandle SurfaceDataSystemComponent::RegisterSurfaceDataModifier(const SurfaceDataRegistryEntry& entry)
    {
        const SurfaceDataRegistryHandle handle = RegisterSurfaceDataModifierInternal(entry);
        if (handle != InvalidSurfaceDataRegistryHandle)
        {
            SurfaceDataSystemNotificationBus::Broadcast(&SurfaceDataSystemNotificationBus::Events::OnSurfaceChanged, entry.m_entityId, entry.m_bounds);
        }
        return handle;
    }

    void SurfaceDataSystemComponent::UnregisterSurfaceDataModifier(const SurfaceDataRegistryHandle& handle)
    {
        const SurfaceDataRegistryEntry entry = UnregisterSurfaceDataModifierInternal(handle);
        if (entry.m_entityId.IsValid())
        {
            SurfaceDataSystemNotificationBus::Broadcast(&SurfaceDataSystemNotificationBus::Events::OnSurfaceChanged, entry.m_entityId, entry.m_bounds);
        }
    }

    void SurfaceDataSystemComponent::UpdateSurfaceDataModifier(const SurfaceDataRegistryHandle& handle, const SurfaceDataRegistryEntry& entry, const AZ::Aabb& dirtyBoundsOverride)
    {
        if (UpdateSurfaceDataModifierInternal(handle, entry))
        {
            const auto& bounds = dirtyBoundsOverride.IsValid() ? dirtyBoundsOverride : entry.m_bounds;
            SurfaceDataSystemNotificationBus::Broadcast(&SurfaceDataSystemNotificationBus::Events::OnSurfaceChanged, entry.m_entityId, bounds);
        }
    }

    void SurfaceDataSystemComponent::GetSurfacePoints(const AZ::Vector3& inPosition, const SurfaceTagVector& desiredTags, SurfacePointList& surfacePointList) const
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Entity);

        const bool hasDesiredTags = HasValidTags(desiredTags);
        const bool hasModifierTags = hasDesiredTags && HasMatchingTags(desiredTags, m_registeredModifierTags);

        AZStd::lock_guard<decltype(m_registrationMutex)> registrationLock(m_registrationMutex);

        surfacePointList.clear();
        surfacePointList.reserve(m_registeredSurfaceDataProviders.size());

        //gather all intersecting points
        for (const auto& entryPair : m_registeredSurfaceDataProviders)
        {
            const AZ::u32 entryAddress = entryPair.first;
            const SurfaceDataRegistryEntry& entry = entryPair.second;
            AZ::Vector3 point2d(inPosition.GetX(), inPosition.GetY(), entry.m_bounds.GetMax().GetZ());
            if (!entry.m_bounds.IsValid() || entry.m_bounds.Contains(point2d))
            {
                if (!hasDesiredTags || hasModifierTags || HasMatchingTags(desiredTags, entry.m_tags))
                {
                    SurfaceDataProviderRequestBus::Event(entryAddress, &SurfaceDataProviderRequestBus::Events::GetSurfacePoints, point2d, surfacePointList);
                }
            }
        }

        //modify or annotate reported points
        for (const auto& entryPair : m_registeredSurfaceDataModifiers)
        {
            const AZ::u32 entryAddress = entryPair.first;
            const SurfaceDataRegistryEntry& entry = entryPair.second;
            AZ::Vector3 point2d(inPosition.GetX(), inPosition.GetY(), entry.m_bounds.GetMax().GetZ());
            if (!entry.m_bounds.IsValid() || entry.m_bounds.Contains(point2d))
            {
                SurfaceDataModifierRequestBus::Event(entryAddress, &SurfaceDataModifierRequestBus::Events::ModifySurfacePoints, surfacePointList);
            }
        }

        CombineSortedNeighboringPoints(surfacePointList);

        //remove unwanted points
        if (hasDesiredTags)
        {
            surfacePointList.erase(
                AZStd::remove_if(
                    surfacePointList.begin(),
                    surfacePointList.end(),
                    [&desiredTags](const SurfacePoint& a) { return !HasMatchingTags(a.m_masks, desiredTags); }),
                surfacePointList.end());
        }
    }

    void SurfaceDataSystemComponent::CombineSortedNeighboringPoints(SurfacePointList& sourcePointList) const
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Entity);

        // If there are 0 or 1 points, early out.  There's nothing to combine or sort.
        if (sourcePointList.size() <= 1)
        {
            return;
        }

        //sort by depth/distance before combining points
        AZStd::sort(sourcePointList.begin(), sourcePointList.end(), [](const SurfacePoint& a, const SurfacePoint& b)
        {
            return a.m_position.GetZ() > b.m_position.GetZ();
        });

        //efficient point consolidation requires the points to be pre-sorted so we are only comparing/combining neighbors
        if (!sourcePointList.empty())
        {
            const size_t sourcePointCount = sourcePointList.size();

            m_targetPointList.clear();
            m_targetPointList.reserve(sourcePointCount);
            m_targetPointList.push_back(sourcePointList[0]); //the first point is always unique

            //iterate over subsequent source points for comparison and consolidation with the last added target/unique point
            size_t targetPointIndex = 0;
            for (size_t sourcePointIndex = 1; sourcePointIndex < sourcePointCount; ++sourcePointIndex)
            {
                auto& targetPoint = m_targetPointList[targetPointIndex];
                const auto& sourcePoint = sourcePointList[sourcePointIndex];

                // [LY-90907] need to add a configurable tolerance for comparison
                if (targetPoint.m_position.IsClose(sourcePoint.m_position) &&
                    targetPoint.m_normal.IsClose(sourcePoint.m_normal))
                {
                    //consolidate points with similar attributes by adding masks to the target point and ignoring the source
                    AddMaxValueForMasks(targetPoint.m_masks, sourcePoint.m_masks);
                    continue;
                }

                //if the points were too different, we have add a new target point to compare against
                m_targetPointList.push_back(sourcePoint);
                ++targetPointIndex;
            }

            AZStd::swap(sourcePointList, m_targetPointList);
        }
    }

    SurfaceDataRegistryHandle SurfaceDataSystemComponent::RegisterSurfaceDataProviderInternal(const SurfaceDataRegistryEntry& entry)
    {
        AZStd::lock_guard<decltype(m_registrationMutex)> registrationLock(m_registrationMutex);
        SurfaceDataRegistryHandle handle = ++m_registeredSurfaceDataProviderHandleCounter;
        m_registeredSurfaceDataProviders[handle] = entry;
        return handle;
    }

    SurfaceDataRegistryEntry SurfaceDataSystemComponent::UnregisterSurfaceDataProviderInternal(const SurfaceDataRegistryHandle& handle)
    {
        AZStd::lock_guard<decltype(m_registrationMutex)> registrationLock(m_registrationMutex);
        SurfaceDataRegistryEntry entry;
        auto entryItr = m_registeredSurfaceDataProviders.find(handle);
        if (entryItr != m_registeredSurfaceDataProviders.end())
        {
            entry = entryItr->second;
            m_registeredSurfaceDataProviders.erase(entryItr);
        }
        return entry;
    }

    bool SurfaceDataSystemComponent::UpdateSurfaceDataProviderInternal(const SurfaceDataRegistryHandle& handle, const SurfaceDataRegistryEntry& entry)
    {
        AZStd::lock_guard<decltype(m_registrationMutex)> registrationLock(m_registrationMutex);
        auto entryItr = m_registeredSurfaceDataProviders.find(handle);
        if (entryItr != m_registeredSurfaceDataProviders.end())
        {
            entryItr->second = entry;
            return true;
        }
        return false;
    }

    SurfaceDataRegistryHandle SurfaceDataSystemComponent::RegisterSurfaceDataModifierInternal(const SurfaceDataRegistryEntry& entry)
    {
        AZStd::lock_guard<decltype(m_registrationMutex)> registrationLock(m_registrationMutex);
        SurfaceDataRegistryHandle handle = ++m_registeredSurfaceDataModifierHandleCounter;
        m_registeredSurfaceDataModifiers[handle] = entry;
        m_registeredModifierTags.insert(entry.m_tags.begin(), entry.m_tags.end());
        return handle;
    }

    SurfaceDataRegistryEntry SurfaceDataSystemComponent::UnregisterSurfaceDataModifierInternal(const SurfaceDataRegistryHandle& handle)
    {
        AZStd::lock_guard<decltype(m_registrationMutex)> registrationLock(m_registrationMutex);
        SurfaceDataRegistryEntry entry;
        auto entryItr = m_registeredSurfaceDataModifiers.find(handle);
        if (entryItr != m_registeredSurfaceDataModifiers.end())
        {
            entry = entryItr->second;
            m_registeredSurfaceDataModifiers.erase(entryItr);
        }
        return entry;
    }

    bool SurfaceDataSystemComponent::UpdateSurfaceDataModifierInternal(const SurfaceDataRegistryHandle& handle, const SurfaceDataRegistryEntry& entry)
    {
        AZStd::lock_guard<decltype(m_registrationMutex)> registrationLock(m_registrationMutex);
        auto entryItr = m_registeredSurfaceDataModifiers.find(handle);
        if (entryItr != m_registeredSurfaceDataModifiers.end())
        {
            entryItr->second = entry;
            m_registeredModifierTags.insert(entry.m_tags.begin(), entry.m_tags.end());
            return true;
        }
        return false;
    }

}