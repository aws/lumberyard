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
#include "BoxShape.h"

#include <AzCore/Math/Color.h>
#include <AzCore/Math/IntersectSegment.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Matrix3x3.h>
#include <AzCore/Math/Random.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/containers/array.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <Cry_Geo.h>
#include <Cry_GeoOverlap.h>
#include <MathConversion.h>
#include <Shape/ShapeDisplay.h>
#include <random>

namespace LmbrCentral
{
    void BoxShape::Reflect(AZ::ReflectContext* context)
    {
        BoxShapeConfig::Reflect(context);

        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<BoxShape>()
                ->Version(1)
                ->Field("Configuration", &BoxShape::m_boxShapeConfig)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<BoxShape>("Box Shape", "Box shape configuration parameters")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &BoxShape::m_boxShapeConfig, "Box Configuration", "Box shape configuration")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ;
            }
        }
    }

    void BoxShape::Activate(AZ::EntityId entityId)
    {
        m_entityId = entityId;
        m_currentTransform = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(m_currentTransform, m_entityId, &AZ::TransformBus::Events::GetWorldTM);
        m_intersectionDataCache.InvalidateCache(InvalidateShapeCacheReason::ShapeChange);

        AZ::TransformNotificationBus::Handler::BusConnect(m_entityId);
        ShapeComponentRequestsBus::Handler::BusConnect(m_entityId);
        BoxShapeComponentRequestsBus::Handler::BusConnect(m_entityId);
    }

    void BoxShape::Deactivate()
    {
        BoxShapeComponentRequestsBus::Handler::BusDisconnect();
        ShapeComponentRequestsBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::Handler::BusDisconnect();
    }

    void BoxShape::InvalidateCache(InvalidateShapeCacheReason reason)
    {
        m_intersectionDataCache.InvalidateCache(reason);
    }

    void BoxShape::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& world)
    {
        m_currentTransform = world;
        m_intersectionDataCache.InvalidateCache(InvalidateShapeCacheReason::TransformChange);
        ShapeComponentNotificationsBus::Event(
            m_entityId, &ShapeComponentNotificationsBus::Events::OnShapeChanged,
            ShapeComponentNotifications::ShapeChangeReasons::TransformChanged);
    }

    void BoxShape::SetBoxDimensions(const AZ::Vector3& dimensions)
    {
        m_boxShapeConfig.m_dimensions = dimensions;
        m_intersectionDataCache.InvalidateCache(InvalidateShapeCacheReason::ShapeChange);
        ShapeComponentNotificationsBus::Event(
            m_entityId, &ShapeComponentNotificationsBus::Events::OnShapeChanged,
            ShapeComponentNotifications::ShapeChangeReasons::ShapeChanged);
    }

    AZ::Aabb BoxShape::GetEncompassingAabb()
    {
        m_intersectionDataCache.UpdateIntersectionParams(m_currentTransform, m_boxShapeConfig);
        return m_intersectionDataCache.m_aabb;
    }

    bool BoxShape::IsPointInside(const AZ::Vector3& point)
    {
        m_intersectionDataCache.UpdateIntersectionParams(m_currentTransform, m_boxShapeConfig);

        if (m_intersectionDataCache.m_axisAligned)
        {
            return m_intersectionDataCache.m_aabb.Contains(point);
        }

        return Overlap::Point_OBB(
            AZVec3ToLYVec3(point), AZVec3ToLYVec3(m_intersectionDataCache.m_currentPosition),
            AZObbToLyOBB(m_intersectionDataCache.m_obb));
    }

    float BoxShape::DistanceSquaredFromPoint(const AZ::Vector3& point)
    {
        m_intersectionDataCache.UpdateIntersectionParams(m_currentTransform, m_boxShapeConfig);

        if (m_intersectionDataCache.m_axisAligned)
        {
            return m_intersectionDataCache.m_aabb.GetDistanceSq(point);
        }

        // The cached OBB has center point at Zero (object space).
        // create a copy and set the center point from the transform before calculating distance.
        OBB obb = AZObbToLyOBB(m_intersectionDataCache.m_obb);
        obb.c = AZVec3ToLYVec3(m_currentTransform.GetPosition());

        return Distance::Point_OBBSq(AZVec3ToLYVec3(point), obb);
    }

    bool BoxShape::IntersectRay(const AZ::Vector3& src, const AZ::Vector3& dir, AZ::VectorFloat& distance)
    {
        m_intersectionDataCache.UpdateIntersectionParams(m_currentTransform, m_boxShapeConfig);

        if (m_intersectionDataCache.m_axisAligned)
        {
            const float rayLength = 1000.0f;
            AZ::Vector3 scaledDir = dir * AZ::VectorFloat(rayLength);
            AZ::Vector3 startNormal;
            AZ::VectorFloat end;

            AZ::VectorFloat t;
            const bool intersection = AZ::Intersect::IntersectRayAABB(
                src, scaledDir, scaledDir.GetReciprocal(),
                m_intersectionDataCache.m_aabb, t, end, startNormal) > 0;

            distance = rayLength * t;
            return intersection;
        }

        auto& obb = m_intersectionDataCache.m_obb;
        auto boxHalfSize = m_intersectionDataCache.m_dimensions * 0.5f;

        float t;
        const bool intersection = AZ::Intersect::IntersectRayBox(
            src, dir, m_intersectionDataCache.m_currentPosition,
            obb.GetAxisX(), obb.GetAxisY(), obb.GetAxisZ(),
            boxHalfSize.GetX(), boxHalfSize.GetY(), boxHalfSize.GetZ(), t) > 0;
        distance = t;

        return intersection;
    }

    AZ::Vector3 BoxShape::GenerateRandomPointInside(AZ::RandomDistributionType randomDistribution)
    {
        m_intersectionDataCache.UpdateIntersectionParams(m_currentTransform, m_boxShapeConfig);

        float x = 0;
        float y = 0;
        float z = 0;

        const AZ::VectorFloat half = AZ::VectorFloat(0.5f);
        AZ::Vector3 boxMin = m_intersectionDataCache.m_dimensions * -half;
        AZ::Vector3 boxMax = m_intersectionDataCache.m_dimensions * half;

        std::default_random_engine generator;
        generator.seed(static_cast<unsigned int>(time(nullptr)));

        switch(randomDistribution)
        {
        case AZ::RandomDistributionType::Normal:
            {
                // Points should be generated just inside the shape boundary
                boxMin *= 0.999f;
                boxMax *= 0.999f;

                const float mean = 0.0f; //Mean will always be 0

                //stdDev will be the sqrt of the max value (which is the total variation)
                float stdDev = sqrtf(boxMax.GetX());
                std::normal_distribution<float> normalDist =
                    std::normal_distribution<float>(mean, stdDev);
                x = normalDist(generator);
                //Normal distributions can sometimes produce values outside of our desired range
                //We just need to clamp
                x = AZStd::clamp<float>(x, boxMin.GetX(), boxMax.GetX());

                stdDev = sqrtf(boxMax.GetY());
                normalDist = std::normal_distribution<float>(mean, stdDev);
                y = normalDist(generator);

                y = AZStd::clamp<float>(y, boxMin.GetY(), boxMax.GetY());

                stdDev = sqrtf(boxMax.GetZ());
                normalDist = std::normal_distribution<float>(mean, stdDev);
                z = normalDist(generator);

                z = AZStd::clamp<float>(z, boxMin.GetZ(), boxMax.GetZ());
            }
            break;
        case AZ::RandomDistributionType::UniformReal:
            {
                std::uniform_real_distribution<float> uniformRealDist =
                    std::uniform_real_distribution<float>(boxMin.GetX(), boxMax.GetX());
                x = uniformRealDist(generator);

                uniformRealDist = std::uniform_real_distribution<float>(boxMin.GetY(), boxMax.GetY());
                y = uniformRealDist(generator);

                uniformRealDist = std::uniform_real_distribution<float>(boxMin.GetZ(), boxMax.GetZ());
                z = uniformRealDist(generator);
            }
            break;
        default:
            AZ_Warning("BoxShape", false, "Unsupported random distribution type. Returning default vector (0,0,0)");
            break;
        }

        // transform to world space
        return m_currentTransform * AZ::Vector3(x, y, z);
    }

    void BoxShape::BoxIntersectionDataCache::UpdateIntersectionParamsImpl(
        const AZ::Transform& currentTransform, const BoxShapeConfig& configuration)
    {
        const AZ::VectorFloat half = AZ::VectorFloat(0.5f);

        AZ::Transform worldFromLocalNormalized = currentTransform;
        const AZ::VectorFloat entityScale = worldFromLocalNormalized.ExtractScale().GetMaxElement();

        m_currentPosition = worldFromLocalNormalized.GetPosition();
        m_dimensions = configuration.m_dimensions * entityScale;

        AZ::Quaternion worldFromLocalQuaternion = AZ::Quaternion::CreateFromTransform(worldFromLocalNormalized);
        if (worldFromLocalQuaternion.IsClose(AZ::Quaternion::CreateIdentity()))
        {
            AZ::Vector3 boxMin = m_dimensions * -half;
            boxMin = worldFromLocalNormalized * boxMin;

            AZ::Vector3 boxMax = m_dimensions * half;
            boxMax = worldFromLocalNormalized * boxMax;

            m_aabb = AZ::Aabb::CreateFromMinMax(boxMin, boxMax);
            m_obb = AZ::Obb::CreateFromAabb(m_aabb);

            m_axisAligned = true;
        }
        else
        {
            const AZ::Matrix3x3 rotationMatrix = AZ::Matrix3x3::CreateFromTransform(worldFromLocalNormalized);
            const AZ::Vector3 halfLengthVector = m_dimensions * half;
            const AZ::Obb obb = AZ::Obb::CreateFromPositionAndAxes(
                AZ::Vector3::CreateZero(),
                rotationMatrix.GetBasisX(), halfLengthVector.GetX(),
                rotationMatrix.GetBasisY(), halfLengthVector.GetY(),
                rotationMatrix.GetBasisZ(), halfLengthVector.GetZ());

            AZStd::array<AZ::Vector3, 8> boxCorners =
            { {
                AZ::Vector3(-1.0f,-1.0f, 1.0f), AZ::Vector3( 1.0f,-1.0f, 1.0f),
                AZ::Vector3(-1.0f, 1.0f, 1.0f), AZ::Vector3( 1.0f, 1.0f, 1.0f),
                AZ::Vector3(-1.0f,-1.0f,-1.0f), AZ::Vector3( 1.0f,-1.0f,-1.0f),
                AZ::Vector3(-1.0f, 1.0f,-1.0f), AZ::Vector3( 1.0f, 1.0f,-1.0f)
            } };

            for (size_t i = 0; i < boxCorners.size(); i++)
            {
                const AZ::Vector3 unrotatedPosition(
                    obb.GetHalfLengthX() * boxCorners[i].GetX(),
                    obb.GetHalfLengthY() * boxCorners[i].GetY(),
                    obb.GetHalfLengthZ() * boxCorners[i].GetZ());

                const AZ::Vector3 rotatedPosition = obb.GetOrientation() * unrotatedPosition;
                const AZ::Vector3 finalPosition = rotatedPosition + worldFromLocalNormalized.GetPosition();
                boxCorners[i] = finalPosition;
            }

            m_obb = obb;
            m_aabb = AZ::Aabb::CreatePoints(boxCorners.begin(), boxCorners.size());
                
            m_axisAligned = false;
        }
    }

    void DrawBoxShape(
        const ShapeDrawParams& shapeDrawParams, const BoxShapeConfig& boxShapeConfig,
        AzFramework::EntityDebugDisplayRequests& displayContext)
    {
        const AZ::Vector3 boxMin = boxShapeConfig.m_dimensions * -0.5f;
        const AZ::Vector3 boxMax = boxShapeConfig.m_dimensions * 0.5f;

        if (shapeDrawParams.m_filled)
        {
            displayContext.SetColor(shapeDrawParams.m_shapeColor.GetAsVector4());
            displayContext.DrawSolidBox(boxMin, boxMax);
        }

        displayContext.SetColor(shapeDrawParams.m_wireColor.GetAsVector4());
        displayContext.DrawWireBox(boxMin, boxMax);
    }
} // namespace LmbrCentral