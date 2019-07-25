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
#include <Cry3DEngine/Environment/OceanEnvironmentBus.h>
#include <SurfaceData/SurfaceDataTagProviderRequestBus.h>


namespace Water
{
    namespace Constants
    {
        static const char* s_waterVolumeTagName = "waterVolume";
        static const char* s_underWaterTagName = "underWater";
        static const char* s_waterTagName = "water";
        static const char* s_oceanTagName = "ocean";
    }

    /**
    * The system component for water component management
    */
    class WaterSystemComponent
        : public AZ::Component
        , public AZ::OceanFeatureToggleBus::Handler
        , private SurfaceData::SurfaceDataTagProviderRequestBus::Handler
    {
    public:
        AZ_COMPONENT(WaterSystemComponent, "{E77EF0DB-92C1-4490-BABA-DE2894FDEB27}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

        ////////////////////////////////////////////////////////////////////////
        // AZ::OceanFeatureToggleBus::Handler implementation
        bool OceanComponentEnabled() const override { return true; }
        ////////////////////////////////////////////////////////////////////////
        // SurfaceDataTagProviderRequestBus
        void GetRegisteredSurfaceTagNames(SurfaceData::SurfaceTagNameSet& names) const override;

    protected:
        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;
    };
}
