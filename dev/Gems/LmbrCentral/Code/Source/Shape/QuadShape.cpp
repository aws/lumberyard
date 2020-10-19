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

#include "LmbrCentral_precompiled.h"
#include "QuadShape.h"

#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/IntersectPoint.h>
#include <AzCore/Math/IntersectSegment.h>
#include <AzCore/Math/Obb.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <LmbrCentral/Shape/QuadShapeComponentBus.h>
#include <Shape/ShapeDisplay.h>

namespace LmbrCentral
{
    void QuadShape::Reflect(AZ::ReflectContext* context)
    {
        QuadShapeConfig::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<QuadShape>()
                ->Version(1)
                ->Field("Configuration", &QuadShape::m_quadShapeConfig)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<QuadShape>("Quad Shape", "Quad shape configuration parameters")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &QuadShape::m_quadShapeConfig, "Quad Configuration", "Quad shape configuration")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    void QuadShape::Activate(AZ::EntityId entityId)
    {
        m_entityId = entityId;
        m_currentTransform = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(m_currentTransform, m_entityId, &AZ::TransformBus::Events::GetWorldTM);
        m_intersectionDataCache.InvalidateCache(InvalidateShapeCacheReason::ShapeChange);

        AZ::TransformNotificationBus::Handler::BusConnect(m_entityId);
        ShapeComponentRequestsBus::Handler::BusConnect(m_entityId);
        QuadShapeComponentRequestBus::Handler::BusConnect(m_entityId);
    }

    void QuadShape::Deactivate()
    {
        QuadShapeComponentRequestBus::Handler::BusDisconnect();
        ShapeComponentRequestsBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::Handler::BusDisconnect();
    }

    void QuadShape::InvalidateCache(InvalidateShapeCacheReason reason)
    {
        m_intersectionDataCache.InvalidateCache(reason);
    }

    void QuadShape::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& world)
    {
        m_currentTransform = world;
        m_intersectionDataCache.InvalidateCache(InvalidateShapeCacheReason::TransformChange);
        ShapeComponentNotificationsBus::Event(
            m_entityId, &ShapeComponentNotificationsBus::Events::OnShapeChanged,
            ShapeComponentNotifications::ShapeChangeReasons::TransformChanged);
    }

    QuadShapeConfig QuadShape::GetQuadConfiguration()
    {
        return m_quadShapeConfig;
    }

    void QuadShape::SetQuadWidth(float width)
    {
        m_quadShapeConfig.m_width = width;
        m_intersectionDataCache.InvalidateCache(InvalidateShapeCacheReason::ShapeChange);
        ShapeComponentNotificationsBus::Event(
            m_entityId, &ShapeComponentNotificationsBus::Events::OnShapeChanged,
            ShapeComponentNotifications::ShapeChangeReasons::ShapeChanged);
    }

    float QuadShape::GetQuadWidth()
    {
        return m_quadShapeConfig.m_width;
    }

    void QuadShape::SetQuadHeight(float height)
    {
        m_quadShapeConfig.m_height = height;
        m_intersectionDataCache.InvalidateCache(InvalidateShapeCacheReason::ShapeChange);
        ShapeComponentNotificationsBus::Event(
            m_entityId, &ShapeComponentNotificationsBus::Events::OnShapeChanged,
            ShapeComponentNotifications::ShapeChangeReasons::ShapeChanged);
    }

    float QuadShape::GetQuadHeight()
    {
        return m_quadShapeConfig.m_height;
    }

    const AZ::Quaternion& QuadShape::GetQuadOrientation()
    {
        m_intersectionDataCache.UpdateIntersectionParams(m_currentTransform, m_quadShapeConfig);
        return m_intersectionDataCache.m_quaternion;
    }

    AZ::Aabb QuadShape::GetEncompassingAabb()
    {
        AZ::Aabb aabb = AZ::Aabb::CreateNull();
        auto corners = m_quadShapeConfig.GetCorners();

        for (AZ::Vector3& corner : corners)
        {
            aabb.AddPoint(m_currentTransform * corner);
        }

        return aabb;
    }

    void QuadShape::GetTransformAndLocalBounds(AZ::Transform& transform, AZ::Aabb& bounds)
    {
        bounds = AZ::Aabb::CreateCenterHalfExtents(
            AZ::Vector3(0.0f, 0.0f, 0.0f),
            AZ::Vector3(m_quadShapeConfig.m_width * 0.5f, m_quadShapeConfig.m_height * 0.5f, 0.0f)
        );
        transform = m_currentTransform;
    }

    bool QuadShape::IsPointInside(const AZ::Vector3& point)
    {
        return false; // 2D object cannot have points that are strictly inside in 3d space.
    }

    float QuadShape::DistanceSquaredFromPoint(const AZ::Vector3& point)
    {
        m_intersectionDataCache.UpdateIntersectionParams(m_currentTransform, m_quadShapeConfig);

        // transform the point into the space of the quad.
        AZ::Vector3 tPoint = point * m_currentTransform;

        float halfWidth = m_intersectionDataCache.m_width * 0.5f;
        float halfHeight = m_intersectionDataCache.m_height * 0.5f;

        // Get the distances in x, y, and z.
        float xDist = AZ::GetMax<float>(AZ::GetMax<float>(-halfWidth - tPoint.GetX(), 0.0f), tPoint.GetX() - halfWidth);
        float yDist = AZ::GetMax<float>(AZ::GetMax<float>(-halfHeight - tPoint.GetY(), 0.0f), tPoint.GetY() - halfHeight); 
        float zDist = tPoint.GetZ();

        return xDist * xDist + yDist * yDist + zDist * zDist;
    }

    bool QuadShape::IntersectRay(const AZ::Vector3& src, const AZ::Vector3& dir, AZ::VectorFloat& distance)
    {
        auto corners = m_quadShapeConfig.GetCorners();

        for (AZ::Vector3& corner : corners)
        {
            corner = m_currentTransform * corner;
        }

        float floatDistance;
        bool hit = AZ::Intersect::IntersectRayQuad(src, dir, corners[0], corners[1], corners[2], corners[3], floatDistance) > 0;
        distance = floatDistance;
        return hit;
    }

    void QuadShape::QuadIntersectionDataCache::UpdateIntersectionParamsImpl(
        const AZ::Transform& currentTransform, const QuadShapeConfig& configuration)
    {
        m_position = currentTransform.GetPosition();
        m_quaternion = AZ::Quaternion::CreateFromTransform(currentTransform);
        m_width = configuration.m_width * currentTransform.RetrieveScale().GetX();
        m_height = configuration.m_height * currentTransform.RetrieveScale().GetY();
    }

    const QuadShapeConfig& QuadShape::GetQuadConfiguration() const
    {
        return m_quadShapeConfig;
    }

    void QuadShape::SetQuadConfiguration(const QuadShapeConfig& QuadShapeConfig)
    {
        m_quadShapeConfig = QuadShapeConfig;
    }

    const AZ::Transform& QuadShape::GetCurrentTransform() const
    {
        return m_currentTransform;
    }

    ShapeComponentConfig& QuadShape::ModifyShapeComponent()
    {
        return m_quadShapeConfig;
    }

    void DrawQuadShape(
        const ShapeDrawParams& shapeDrawParams, const QuadShapeConfig& quadConfig,
        AzFramework::DebugDisplayRequests& debugDisplay)
    {
        // By default, debugDisplay draws quads facing the y axis, but we need it facing z.
        debugDisplay.PushMatrix(AZ::Transform::CreateRotationX(AZ::Constants::HalfPi));

        if (shapeDrawParams.m_filled)
        {
            debugDisplay.SetColor(shapeDrawParams.m_shapeColor.GetAsVector4());
            debugDisplay.DrawQuad(quadConfig.m_width, quadConfig.m_height);
        }

        debugDisplay.SetColor(shapeDrawParams.m_wireColor.GetAsVector4());
        debugDisplay.DrawWireQuad(quadConfig.m_width, quadConfig.m_height);

        debugDisplay.PopMatrix();

        // Draw line from center indicating facing direction.
        float normalLength = sqrt(quadConfig.m_width * quadConfig.m_width + quadConfig.m_height * quadConfig.m_height) * 0.1f;
        debugDisplay.DrawLine(AZ::Vector3::CreateZero(), AZ::Vector3(0.0f, 0.0f, normalLength));
    }
} // namespace LmbrCentral
