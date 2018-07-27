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
#include "Rendering/EntityDebugDisplayComponent.h"
#include "SphereShape.h"

namespace LmbrCentral
{
    class SphereShapeComponent
        : public AZ::Component
    {
    public:
        friend class EditorSphereShapeComponent;

        AZ_COMPONENT(SphereShapeComponent, SphereShapeComponentTypeId);

        // AZ::Component
        void Activate() override;
        void Deactivate() override;
        bool ReadInConfig(const AZ::ComponentConfig* baseConfig) override;
        bool WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const override;

    protected:
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("ShapeService", 0xe86aa5fe));
            provided.push_back(AZ_CRC("SphereShapeService", 0x90c8dc80));
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC("ShapeService", 0xe86aa5fe));
            incompatible.push_back(AZ_CRC("SphereShapeService", 0x90c8dc80));
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC("TransformService", 0x8ee22c50));
        }

        static void Reflect(AZ::ReflectContext* context);

    private:
        SphereShape m_sphereShape; ///< Stores underlying sphere type for this component.
    };

    /**
     * Concrete EntityDebugDisplay implementation for SphereShape.
     */
    class SphereShapeDebugDisplayComponent
        : public EntityDebugDisplayComponent
        , public ShapeComponentNotificationsBus::Handler
    {
    public:
        AZ_COMPONENT(SphereShapeDebugDisplayComponent, "{C3E8DEF0-3786-4765-8B19-BDCB5E966980}", EntityDebugDisplayComponent)

        SphereShapeDebugDisplayComponent() = default;

        static void Reflect(AZ::ReflectContext* context);

        // AZ::Component
        void Activate() override;
        void Deactivate() override;
        bool ReadInConfig(const AZ::ComponentConfig* baseConfig) override;
        bool WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const override;

        // EntityDebugDisplayComponent
        void Draw(AzFramework::EntityDebugDisplayRequests* displayContext) override;

        // ShapeComponentNotificationsBus
        void OnShapeChanged(ShapeChangeReasons changeReason) override;

    private:
        AZ_DISABLE_COPY_MOVE(SphereShapeDebugDisplayComponent)

        SphereShapeConfig m_sphereShapeConfig; ///< Stores configuration data for sphere shape.
    };
} // namespace LmbrCentral
