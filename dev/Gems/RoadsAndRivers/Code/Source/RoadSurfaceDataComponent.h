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
#include <LmbrCentral/Shape/SplineComponentBus.h>
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
        static const char* s_roadTagName = "road";
    }

    class RoadSurfaceDataConfig
        : public AZ::ComponentConfig
    {
    public:
        AZ_CLASS_ALLOCATOR(RoadSurfaceDataConfig, AZ::SystemAllocator, 0);
        AZ_RTTI(RoadSurfaceDataConfig, "{92F10209-F089-4F24-BE8A-DFF9654191F1}", AZ::ComponentConfig);
        static void Reflect(AZ::ReflectContext* context);
        SurfaceData::SurfaceTagVector m_providerTags;
        bool m_snapToTerrain = true;
    };

    class RoadSurfaceDataComponent
        : public AZ::Component
        , private AZ::TickBus::Handler
        , private AZ::TransformNotificationBus::Handler
        , private LmbrCentral::SplineComponentNotificationBus::Handler
        , private RoadsAndRivers::RoadNotificationBus::Handler
        , private RoadsAndRivers::RoadsAndRiversGeometryNotificationBus::Handler
        , private SurfaceData::SurfaceDataProviderRequestBus::Handler
    {
    public:
        template<typename, typename> friend class LmbrCentral::EditorWrappedComponentBase;
        AZ_COMPONENT(RoadSurfaceDataComponent, "{1C8AC5E9-BF6D-474C-853B-835E4F48B6FE}");
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void Reflect(AZ::ReflectContext* context);

        RoadSurfaceDataComponent(const RoadSurfaceDataConfig& configuration);
        RoadSurfaceDataComponent() = default;
        ~RoadSurfaceDataComponent() = default;

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
        // AZ::TransformNotificationBus
        void OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& /*world*/) override;

        //////////////////////////////////////////////////////////////////////////
        // SplineComponentNotificationBus
        void OnSplineChanged() override;

        //////////////////////////////////////////////////////////////////////////
        // RoadNotificationBus
        void OnIgnoreTerrainHolesChanged(bool ignoreHoles) override;

        //////////////////////////////////////////////////////////////////////////
        // RoadsAndRiversGeometryNotificationBus
        virtual void OnWidthChanged() override;
        virtual void OnSegmentLengthChanged(float segmentLength) override;

        ////////////////////////////////////////////////////////////////////////
        // AZ::TickBus
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;


    private:
        void OnCompositionChanged();
        void UpdateShapeData();

        RoadSurfaceDataConfig m_configuration;

        SurfaceData::SurfaceDataRegistryHandle m_providerHandle = SurfaceData::InvalidSurfaceDataRegistryHandle;

        // cached data
        AZStd::atomic_bool m_refresh{ false };
        mutable AZStd::recursive_mutex m_cacheMutex;
        AZ::Transform m_shapeWorldTM = AZ::Transform::CreateIdentity();
        AZ::Aabb m_shapeBounds = AZ::Aabb::CreateNull();
        bool m_shapeBoundsIsValid = false;
        AZStd::vector<AZ::Vector3> m_shapeVertices;
        bool m_ignoreTerrainHoles = false;
    };
}