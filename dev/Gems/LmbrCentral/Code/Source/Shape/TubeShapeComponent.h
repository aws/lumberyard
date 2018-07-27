/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.EditorTubeShapeComponent
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
#include "TubeShape.h"

namespace LmbrCentral
{
    class TubeShapeComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(TubeShapeComponent, TubeShapeComponentTypeId);
        static void Reflect(AZ::ReflectContext* context);

        TubeShapeComponent() = default;
        explicit TubeShapeComponent(const TubeShape& tubeShape);

        // AZ::Component
        void Activate() override;
        void Deactivate() override;

    protected:
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("ShapeService", 0xe86aa5fe));
            provided.push_back(AZ_CRC("TubeShapeService", 0x3fe791b4));
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC("ShapeService", 0xe86aa5fe));
            incompatible.push_back(AZ_CRC("TubeShapeService", 0x3fe791b4));
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC("TransformService", 0x8ee22c50));
            required.push_back(AZ_CRC("SplineService", 0x2b674d3c));
        }

    private:
        TubeShape m_tubeShape;
    };

    /**
     * Concrete EntityDebugDisplay implementation for TubeShape.
     */
    class TubeShapeDebugDisplayComponent
        : public EntityDebugDisplayComponent
        , public ShapeComponentNotificationsBus::Handler
    {
    public:
        AZ_COMPONENT(TubeShapeDebugDisplayComponent, "{FC8D0C5A-FEED-4C79-A4C6-E18A966EE8CE}", EntityDebugDisplayComponent)

        TubeShapeDebugDisplayComponent() = default;
        explicit TubeShapeDebugDisplayComponent(const TubeShapeMeshConfig& tubeShapeMeshConfig)
            : m_tubeShapeMeshConfig(tubeShapeMeshConfig) {}

        static void Reflect(AZ::ReflectContext* context);

        // AZ::Component
        void Activate() override;
        void Deactivate() override;

        // EntityDebugDisplayComponent
        void Draw(AzFramework::EntityDebugDisplayRequests* displayContext) override;

        // ShapeComponentNotificationsBus
        void OnShapeChanged(ShapeChangeReasons changeReason) override;

    private:
        AZ_DISABLE_COPY_MOVE(TubeShapeDebugDisplayComponent)

        void GenerateVertices();

        ShapeMesh m_tubeShapeMesh; ///< Buffer to hold index and vertex data for TubeShape when drawing.
        TubeShapeMeshConfig m_tubeShapeMeshConfig; ///< Configuration to control how the TubeShape should look.
        AZ::SplinePtr m_spline = nullptr; ///< Reference to Spline.
        SplineAttribute<float> m_radiusAttribute; ///< Radius Attribute.
        float m_radius = 0.0f; ///< Global radius for Tube.
    };
} // namespace LmbrCentral