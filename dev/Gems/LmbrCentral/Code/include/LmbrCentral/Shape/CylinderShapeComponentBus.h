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

#include <AzCore/Component/ComponentBus.h>

#include <LmbrCentral/Shape/ShapeComponentBus.h>

namespace LmbrCentral
{
    /// Type ID of CylinderShapeComponent
    static const AZ::Uuid CylinderShapeComponentTypeId = "{B0C6AA97-E754-4E33-8D32-33E267DB622F}";

    /// Type ID of EditorCylinderShapeComponent
    static const AZ::Uuid EditorCylinderShapeComponentTypeId = "{D5FC4745-3C75-47D9-8C10-9F89502487DE}";

    /// Configuration data for CylinderShapeComponent
    class CylinderShapeConfig
        : public ShapeComponentConfig
    {
    public:
        AZ_CLASS_ALLOCATOR(CylinderShapeConfig, AZ::SystemAllocator, 0);
        AZ_RTTI(CylinderShapeConfig, "{53254779-82F1-441E-9116-81E1FACFECF4}", ComponentConfig);
        static void Reflect(AZ::ReflectContext* context);

        void SetHeight(float height)
        {
            AZ_WarningOnce("LmbrCentral", false, "SetHeight Deprecated - Please use m_height");
            m_height = height;
        }

        float GetHeight() const
        {
            AZ_WarningOnce("LmbrCentral", false, "GetHeight Deprecated - Please use m_height");
            return m_height;
        }

        void SetRadius(float radius)
        {
            AZ_WarningOnce("LmbrCentral", false, "SetRadius Deprecated - Please use m_radius");
            m_radius = radius;
        }

        float GetRadius() const
        {
            AZ_WarningOnce("LmbrCentral", false, "GetRadius Deprecated - Please use m_radius");
            return m_radius;
        }

        float m_height = 1.f; ///< The height of this cylinder.
        float m_radius = 0.5f; ///< The radius of this cylinder.
    };

    using CylinderShapeConfiguration = CylinderShapeConfig; ///< @deprecated Use CylinderShapeConfig.

    /// Services provided by the Cylinder Shape Component.
    class CylinderShapeComponentRequests
        : public AZ::ComponentBus
    {
    public:
        virtual CylinderShapeConfig GetCylinderConfiguration() = 0;

        /// @brief Returns the end to end height of the cylinder.
        virtual float GetHeight() = 0;

        /// @brief Returns the radius of the cylinder.
        virtual float GetRadius() = 0;
        
        /// @brief Sets height of the cylinder
        /// @param height new height of the cylinder
        virtual void SetHeight(float height) = 0;

        /// @brief Sets radius of the cylinder
        /// @param radius new radius of the cylinder
        virtual void SetRadius(float radius) = 0;
    };

    // Bus to service the Cylinder Shape component event group
    using CylinderShapeComponentRequestsBus = AZ::EBus<CylinderShapeComponentRequests>;

} // namespace LmbrCentral