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
#include <Water/WaterOceanComponentData.h>
#include <AzCore/Component/TransformBus.h>

namespace Water
{
    /**
    * Represents the ocean as a component for a game entity
    * @note Only one ocean component should be added to a level
    */
    class WaterOceanComponent
        : public AZ::Component
        , private AZ::TransformNotificationBus::Handler
    {
    public:
        AZ_COMPONENT(WaterOceanComponent, "{517760D2-237E-4582-9CEF-A91EFF641441}");

        static void Reflect(AZ::ReflectContext* context);

        WaterOceanComponent();
        WaterOceanComponent(const WaterOceanComponentData& data);
        
    protected:

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation.
        void Activate() override;
        void Deactivate() override;

        //////////////////////////////////////////////////////////////////////////
        // AZ::TransformNotificationBus::Handler
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

    protected:
        WaterOceanComponentData m_data;
    };
}
