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
#include <AzCore/Component/TickBus.h>
#include <AzCore/Component/TransformBus.h>
#include <CrySystemBus.h>
#include <LmbrCentral/Physics/WaterNotificationBus.h>
#include <SurfaceData/SurfaceDataModifierRequestBus.h>
#include <SurfaceData/SurfaceDataProviderRequestBus.h>
#include <SurfaceData/SurfaceDataTypes.h>

namespace LmbrCentral
{
    template<typename, typename>
    class EditorWrappedComponentBase;
}

namespace Water
{
    class OceanSurfaceDataConfig
        : public AZ::ComponentConfig
    {
    public:
        AZ_CLASS_ALLOCATOR(OceanSurfaceDataConfig, AZ::SystemAllocator, 0);
        AZ_RTTI(OceanSurfaceDataConfig, "{74C6EB97-31FD-4140-BDE5-F5D24E23B681}", AZ::ComponentConfig);
        static void Reflect(AZ::ReflectContext* context);
        SurfaceData::SurfaceTagVector m_providerTags;
        SurfaceData::SurfaceTagVector m_modifierTags;
    };

    class OceanSurfaceDataComponent
        : public AZ::Component
        , private AZ::TickBus::Handler
        , private AZ::TransformNotificationBus::Handler
        , private CrySystemEventBus::Handler
        , private LmbrCentral::WaterNotificationBus::Handler
        , private SurfaceData::SurfaceDataModifierRequestBus::Handler
        , private SurfaceData::SurfaceDataProviderRequestBus::Handler
    {
    public:
        template<typename, typename> friend class LmbrCentral::EditorWrappedComponentBase;
        AZ_COMPONENT(OceanSurfaceDataComponent, "{108A5A6C-1E22-449E-8E6B-DC23D99BD94C}");
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void Reflect(AZ::ReflectContext* context);

        OceanSurfaceDataComponent(const OceanSurfaceDataConfig& configuration);
        OceanSurfaceDataComponent() = default;
        ~OceanSurfaceDataComponent() = default;

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;
        bool ReadInConfig(const AZ::ComponentConfig* baseConfig) override;
        bool WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const override;

        //////////////////////////////////////////////////////////////////////////
        // SurfaceDataProviderRequestBus
        void GetSurfacePoints(const AZ::Vector3& inPosition, SurfaceData::SurfacePointList& surfacePointList) const;

        //////////////////////////////////////////////////////////////////////////
        // SurfaceDataModifierRequestBus
        void ModifySurfacePoints(SurfaceData::SurfacePointList& surfacePointList) const override;

        //////////////////////////////////////////////////////////////////////////
        // AZ::TransformNotificationBus
        void OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& /*world*/) override;

        ////////////////////////////////////////////////////////////////////////////
        // WaterNotificationBus
        void OceanHeightChanged(float height) override;

        ////////////////////////////////////////////////////////////////////////
        // AZ::TickBus
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

        ////////////////////////////////////////////////////////////////////////////
        // CrySystemEvents
        void OnCrySystemInitialized(ISystem& system, const SSystemInitParams& systemInitParams) override;
        void OnCrySystemShutdown(ISystem& system) override;

    private:
        void OnCompositionChanged();
        void UpdateShapeData();

        OceanSurfaceDataConfig m_configuration;

        SurfaceData::SurfaceDataRegistryHandle m_providerHandle = SurfaceData::InvalidSurfaceDataRegistryHandle;
        SurfaceData::SurfaceDataRegistryHandle m_modifierHandle = SurfaceData::InvalidSurfaceDataRegistryHandle;

        // cached data
        AZStd::atomic_bool m_refresh{ false };
        mutable AZStd::recursive_mutex m_cacheMutex;
        AZ::Transform m_shapeWorldTM = AZ::Transform::CreateIdentity();
        AZ::Aabb m_shapeBounds = AZ::Aabb::CreateNull();
        ISystem* m_system = nullptr;
    };
}