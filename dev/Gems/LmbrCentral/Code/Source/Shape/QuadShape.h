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

#include <AzCore/Component/TransformBus.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>
#include <LmbrCentral/Shape/QuadShapeComponentBus.h>

namespace AzFramework
{
    class DebugDisplayRequests;
}

namespace LmbrCentral
{
    struct ShapeDrawParams;

    //! Provide QuadShape functionality.
    class QuadShape
        : public ShapeComponentRequestsBus::Handler
        , public QuadShapeComponentRequestBus::Handler
        , public AZ::TransformNotificationBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(QuadShape, AZ::SystemAllocator, 0)
        AZ_RTTI(LmbrCentral::QuadShape, "{4DCA67DA-5CBB-4E6C-8DA2-2B8CB177A301}")

        static void Reflect(AZ::ReflectContext* context);

        void Activate(AZ::EntityId entityId);
        void Deactivate();
        void InvalidateCache(InvalidateShapeCacheReason reason);

        //! ShapeComponentRequestsBus overrides...
        AZ::Crc32 GetShapeType() override { return AZ_CRC("QuadShape", 0x40d75e14); }
        AZ::Aabb GetEncompassingAabb() override;
        void GetTransformAndLocalBounds(AZ::Transform& transform, AZ::Aabb& bounds) override;
        bool IsPointInside(const AZ::Vector3& point)  override;
        float DistanceSquaredFromPoint(const AZ::Vector3& point) override;
        bool IntersectRay(const AZ::Vector3& src, const AZ::Vector3& dir, AZ::VectorFloat& distance) override;

        //! QuadShapeComponentRequestBus overrides...
        QuadShapeConfig GetQuadConfiguration() override;
        void SetQuadWidth(float width) override;
        float GetQuadWidth() override;
        void SetQuadHeight(float height) override;
        float GetQuadHeight() override;
        const AZ::Quaternion& GetQuadOrientation() override;

        //! AZ::TransformNotificationBus overrides...
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

        const QuadShapeConfig& GetQuadConfiguration() const;
        void SetQuadConfiguration(const QuadShapeConfig& quadShapeConfig);
        const AZ::Transform& GetCurrentTransform() const;

        AZStd::array<AZ::Vector3, 4> GetLocalSpaceCorners();

    protected:

        friend class EditorQuadShapeComponent;
        ShapeComponentConfig& ModifyShapeComponent();

    private:
        //! Runtime data - cache potentially expensive operations.
        class QuadIntersectionDataCache
            : public IntersectionTestDataCache<QuadShapeConfig>
        {
            void UpdateIntersectionParamsImpl(
                const AZ::Transform& currentTransform, const QuadShapeConfig& configuration) override;

            friend class QuadShape;

            AZ::Vector3 m_position; //! Position of the center of the quad.
            AZ::Quaternion m_quaternion; //! quaternion of the quad.
            float m_width = 1.0f; //! Width of the quad (including entity scale).
            float m_height = 1.0f; //! Height of the quad (including entity scale).
        };

        QuadShapeConfig m_quadShapeConfig; //! Underlying quad configuration.
        QuadIntersectionDataCache m_intersectionDataCache; //! Caches transient intersection data.
        AZ::Transform m_currentTransform; //! Caches the current world transform.
        AZ::EntityId m_entityId; //! The Id of the entity the shape is attached to.
    };

    void DrawQuadShape(
        const ShapeDrawParams& shapeDrawParams, const QuadShapeConfig& quadShapeConfig,
        AzFramework::DebugDisplayRequests& debugDisplay);

} // namespace LmbrCentral
