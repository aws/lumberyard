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
#include <AzCore/Math/Aabb.h>
#include <SurfaceData/SurfaceDataSystemRequestBus.h>

namespace SurfaceData
{
    class SurfaceDataSystemComponent
        : public AZ::Component
        , private SurfaceDataSystemRequestBus::Handler
    {
    public:
        AZ_COMPONENT(SurfaceDataSystemComponent, "{6F334BAA-7BD5-45F8-A9BA-760667D25FA0}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

    protected:
        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        
        
        ////////////////////////////////////////////////////////////////////////
        // SurfaceDataSystemRequestBus implementation
        virtual void GetSurfacePoints(const AZ::Vector3& inPosition, const SurfaceTagVector& masks, SurfacePointList& surfacePointList) const override;

        virtual SurfaceDataRegistryHandle RegisterSurfaceDataProvider(const SurfaceDataRegistryEntry& entry) override;
        virtual void UnregisterSurfaceDataProvider(const SurfaceDataRegistryHandle& handle) override;
        virtual void UpdateSurfaceDataProvider(const SurfaceDataRegistryHandle& handle, const SurfaceDataRegistryEntry& entry, const AZ::Aabb& dirtyBoundsOverride) override;

        virtual SurfaceDataRegistryHandle RegisterSurfaceDataModifier(const SurfaceDataRegistryEntry& entry) override;
        virtual void UnregisterSurfaceDataModifier(const SurfaceDataRegistryHandle& handle) override;
        virtual void UpdateSurfaceDataModifier(const SurfaceDataRegistryHandle& handle, const SurfaceDataRegistryEntry& entry, const AZ::Aabb& dirtyBoundsOverride) override;

    private:
        void CombineSortedNeighboringPoints(SurfacePointList& sourcePointList) const;

        SurfaceDataRegistryHandle RegisterSurfaceDataProviderInternal(const SurfaceDataRegistryEntry& entry);
        SurfaceDataRegistryEntry UnregisterSurfaceDataProviderInternal(const SurfaceDataRegistryHandle& handle);
        bool UpdateSurfaceDataProviderInternal(const SurfaceDataRegistryHandle& handle, const SurfaceDataRegistryEntry& entry);

        SurfaceDataRegistryHandle RegisterSurfaceDataModifierInternal(const SurfaceDataRegistryEntry& entry);
        SurfaceDataRegistryEntry UnregisterSurfaceDataModifierInternal(const SurfaceDataRegistryHandle& handle);
        bool UpdateSurfaceDataModifierInternal(const SurfaceDataRegistryHandle& handle, const SurfaceDataRegistryEntry& entry);

        mutable AZStd::recursive_mutex m_registrationMutex;
        AZStd::unordered_map<SurfaceDataRegistryHandle, SurfaceDataRegistryEntry> m_registeredSurfaceDataProviders;
        AZStd::unordered_map<SurfaceDataRegistryHandle, SurfaceDataRegistryEntry> m_registeredSurfaceDataModifiers;
        SurfaceDataRegistryHandle m_registeredSurfaceDataProviderHandleCounter = InvalidSurfaceDataRegistryHandle;
        SurfaceDataRegistryHandle m_registeredSurfaceDataModifierHandleCounter = InvalidSurfaceDataRegistryHandle;
        AZStd::unordered_set<AZ::u32> m_registeredModifierTags;

        //point vector reserved for reuse
        mutable SurfacePointList m_targetPointList;
    };
}
