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
#include <LmbrCentral/Shape/SplineComponentBus.h>
#include <SurfaceData/SurfaceDataModifierRequestBus.h>
#include <SurfaceData/SurfaceDataProviderRequestBus.h>
#include <SurfaceData/SurfaceDataTypes.h>
#include "RoadsAndRivers/RoadsAndRiversBus.h"

namespace LmbrCentral
{
    template<typename, typename>
    class EditorWrappedComponentBase;
}

namespace RoadsAndRivers
{
    namespace Constants
    {
        static const char* s_underWaterTagName = "underWater";
        static const char* s_waterTagName = "water";
        static const char* s_riverTagName = "river";
    }

    class RiverSurfaceDataConfig
        : public AZ::ComponentConfig
    {
    public:
        AZ_CLASS_ALLOCATOR(RiverSurfaceDataConfig, AZ::SystemAllocator, 0);
        AZ_RTTI(RiverSurfaceDataConfig, "{07E11C06-B846-48C3-B759-FCA047333BFE}", AZ::ComponentConfig);
        static void Reflect(AZ::ReflectContext* context);
        SurfaceData::SurfaceTagVector m_providerTags;
        SurfaceData::SurfaceTagVector m_modifierTags;
        bool m_snapToRiverSurface = true;
    };

    class RiverSurfaceDataComponent
        : public AZ::Component
        , private AZ::TickBus::Handler
        , private AZ::TransformNotificationBus::Handler
        , private CrySystemEventBus::Handler
        , private LmbrCentral::SplineComponentNotificationBus::Handler
        , private RoadsAndRivers::RiverNotificationBus::Handler
        , private RoadsAndRivers::RoadsAndRiversGeometryNotificationBus::Handler
        , private SurfaceData::SurfaceDataModifierRequestBus::Handler
        , private SurfaceData::SurfaceDataProviderRequestBus::Handler
    {
    public:
        template<typename, typename> friend class LmbrCentral::EditorWrappedComponentBase;
        friend class EditorRiverSurfaceDataComponent;
        AZ_COMPONENT(RiverSurfaceDataComponent, "{61B9739A-90FF-41D9-A1F4-B0DA9E021C1A}");
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void Reflect(AZ::ReflectContext* context);

        RiverSurfaceDataComponent(const RiverSurfaceDataConfig& configuration);
        RiverSurfaceDataComponent() = default;
        ~RiverSurfaceDataComponent() = default;

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

        //////////////////////////////////////////////////////////////////////////
        // SplineComponentNotificationBus
        void OnSplineChanged() override;

        //////////////////////////////////////////////////////////////////////////
        // RiverNotificationBus
        void OnWaterVolumeDepthChanged(float depth) override;

        //////////////////////////////////////////////////////////////////////////
        // RoadsAndRiversGeometryNotificationBus
        virtual void OnWidthChanged() override;
        virtual void OnSegmentLengthChanged(float segmentLength) override;

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

        RiverSurfaceDataConfig m_configuration;

        SurfaceData::SurfaceDataRegistryHandle m_providerHandle = SurfaceData::InvalidSurfaceDataRegistryHandle;
        SurfaceData::SurfaceDataRegistryHandle m_modifierHandle = SurfaceData::InvalidSurfaceDataRegistryHandle;

        // cached data
        AZStd::atomic_bool m_refresh{ false };
        mutable AZStd::recursive_mutex m_cacheMutex;
        AZ::Transform m_shapeWorldTM = AZ::Transform::CreateIdentity();
        AZ::Aabb m_surfaceShapeBounds = AZ::Aabb::CreateNull();
        AZ::Aabb m_volumeShapeBounds = AZ::Aabb::CreateNull();
        bool m_surfaceShapeBoundsIsValid = false;
        bool m_volumeShapeBoundsIsValid = false;
        AZ::VectorFloat m_riverDepth = 0.0f;
        AZStd::vector<AZ::Vector3> m_shapeVertices;
        ISystem* m_system = nullptr;
    };
}