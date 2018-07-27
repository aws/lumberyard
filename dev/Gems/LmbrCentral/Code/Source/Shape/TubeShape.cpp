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
#include "TubeShape.h"

#include <AzCore/Math/Transform.h>
#include <Shape/ShapeGeometryUtil.h>

namespace LmbrCentral
{
    float Lerpf(float from, float to, float fraction)
    {
        return AZ::Lerp(from, to, fraction);
    }

    AZ::Vector3 CalculateNormal(const AZ::Vector3& previousNormal, const AZ::Vector3& previousTangent, const AZ::Vector3& currentTangent)
    {
        // Rotates the previous normal by the angle difference between two tangent segments. Ensures
        // The normal is continuous along the tube.
        AZ::Vector3 normal = previousNormal;
        auto cosAngleBetweenTangentSegments = currentTangent.Dot(previousTangent);
        if (cosAngleBetweenTangentSegments.GetAbs().IsLessThan(AZ::VectorFloat::CreateOne()))
        {
            AZ::Vector3 axis = previousTangent.Cross(currentTangent);
            if (!axis.IsZero())
            {
                axis.Normalize();
                float angle = acosf(cosAngleBetweenTangentSegments);
                AZ::Quaternion rotationTangentDelta = AZ::Quaternion::CreateFromAxisAngle(axis, angle);
                normal = rotationTangentDelta * normal;
                normal.Normalize();
            }
        }
        return normal;
    }

    void TubeShape::Reflect(AZ::SerializeContext& context)
    {
        context.Class <TubeShape>()
            ->Field("Radius", &TubeShape::m_radius)
            ->Field("VariableRadius", &TubeShape::m_variableRadius)
            ;

        if (auto editContext = context.GetEditContext())
        {
            editContext->Class<TubeShape>(
                "Tube Shape", "")
                ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->DataElement(AZ::Edit::UIHandlers::Default, &TubeShape::m_radius, "Radius", "Radius of the tube")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.1f)
                    ->Attribute(AZ::Edit::Attributes::Step, 0.5f)
                ->DataElement(AZ::Edit::UIHandlers::Default, &TubeShape::m_variableRadius, "Variable Radius", "Variable radius along the tube")
                ;
        }
    }

    void TubeShape::Activate(AZ::EntityId entityId)
    {
        m_entityId = entityId;

        AZ::TransformNotificationBus::Handler::BusConnect(m_entityId);
        ShapeComponentRequestsBus::Handler::BusConnect(m_entityId);
        TubeShapeComponentRequestsBus::Handler::BusConnect(m_entityId);
        SplineComponentNotificationBus::Handler::BusConnect(m_entityId);

        AZ::TransformBus::EventResult(m_currentTransform, m_entityId, &AZ::TransformBus::Events::GetWorldTM);
        SplineComponentRequestBus::EventResult(m_spline, m_entityId, &SplineComponentRequests::GetSpline);

        m_variableRadius.Activate(entityId);
    }

    void TubeShape::Deactivate()
    {
        m_variableRadius.Deactivate();
        SplineComponentNotificationBus::Handler::BusDisconnect();
        TubeShapeComponentRequestsBus::Handler::BusDisconnect();
        ShapeComponentRequestsBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::Handler::BusDisconnect();
    }

    void TubeShape::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& world)
    {
        m_currentTransform = world;
        ShapeComponentNotificationsBus::Event(
            m_entityId, &ShapeComponentNotificationsBus::Events::OnShapeChanged,
            ShapeComponentNotifications::ShapeChangeReasons::TransformChanged);
    }

    void TubeShape::SetRadius(float radius)
    {
        m_radius = radius;
        ShapeComponentNotificationsBus::Event(
            m_entityId, &ShapeComponentNotificationsBus::Events::OnShapeChanged,
            ShapeComponentNotifications::ShapeChangeReasons::ShapeChanged);
    }

    float TubeShape::GetRadius() const
    {
        return m_radius;
    }

    void TubeShape::SetVariableRadius(int index, float radius)
    {
        m_variableRadius.SetElement(index, radius);
        ShapeComponentNotificationsBus::Event(
            m_entityId, &ShapeComponentNotificationsBus::Events::OnShapeChanged,
            ShapeComponentNotifications::ShapeChangeReasons::ShapeChanged);
    }

    float TubeShape::GetVariableRadius(int index) const
    {
        return m_variableRadius.GetElement(index);
    }

    float TubeShape::GetTotalRadius(const AZ::SplineAddress& address) const
    {
        return m_radius + m_variableRadius.GetElementInterpolated(address, Lerpf);
    }

    const SplineAttribute<float>& TubeShape::GetRadiusAttribute() const
    {
        return m_variableRadius;
    }

    void TubeShape::OnSplineChanged()
    {
        SplineComponentRequestBus::EventResult(m_spline, m_entityId, &SplineComponentRequests::GetSpline);
    }

    AZ::Aabb TubeShape::GetEncompassingAabb()
    {
        if (m_spline == nullptr)
        {
            return AZ::Aabb();
        }

        AZ::Transform worldFromLocalUniformScale = m_currentTransform;
        const AZ::VectorFloat maxScale = worldFromLocalUniformScale.ExtractScale().GetMaxElement();
        worldFromLocalUniformScale *= AZ::Transform::CreateScale(AZ::Vector3(maxScale));

        // approximate aabb - not exact but is guaranteed to encompass tube
        AZ::VectorFloat maxRadius = GetTotalRadius(AZ::SplineAddress(0, 0.0f)) * maxScale;
        for (size_t i = 0; i < m_spline->GetVertexCount(); ++i)
        {
            maxRadius = AZ::GetMax(maxRadius, GetTotalRadius(AZ::SplineAddress(i, 1.0f)) * maxScale);
        }

        AZ::Aabb aabb;
        m_spline->GetAabb(aabb, worldFromLocalUniformScale);
        aabb.Expand(AZ::Vector3(maxRadius, maxRadius, maxRadius));
        return aabb;
    }

    bool TubeShape::IsPointInside(const AZ::Vector3& point)
    {
        if (m_spline == nullptr)
        {
            return false;
        }

        AZ::Transform worldFromLocalNormalized = m_currentTransform;
        const AZ::Vector3 scale = AZ::Vector3(worldFromLocalNormalized.ExtractScale().GetMaxElement());
        const AZ::Transform localFromWorldNormalized = worldFromLocalNormalized.GetInverseFast();
        const AZ::Vector3 localPoint = localFromWorldNormalized * point * scale.GetReciprocal();

        const auto address = m_spline->GetNearestAddressPosition(localPoint).m_splineAddress;
        const float radiusSq = powf(m_radius, 2.0f);
        const float variableRadiusSq =
            powf(m_variableRadius.GetElementInterpolated(
                address.m_segmentIndex, address.m_segmentFraction, Lerpf), 2.0f);

        return (m_spline->GetPosition(address) - localPoint).GetLengthSq() < (radiusSq + variableRadiusSq) *
            scale.GetMaxElement();
    }

    float TubeShape::DistanceSquaredFromPoint(const AZ::Vector3& point)
    {
        AZ::Transform worldFromLocalNormalized = m_currentTransform;
        const AZ::Vector3 maxScale = AZ::Vector3(worldFromLocalNormalized.ExtractScale().GetMaxElement());
        const AZ::Transform localFromWorldNormalized = worldFromLocalNormalized.GetInverseFast();
        const AZ::Vector3 localPoint = localFromWorldNormalized * point * maxScale.GetReciprocal();

        auto splineQueryResult = m_spline->GetNearestAddressPosition(localPoint);
        const float variableRadius = m_variableRadius.GetElementInterpolated(
            splineQueryResult.m_splineAddress.m_segmentIndex, splineQueryResult.m_splineAddress.m_segmentFraction, Lerpf);

        return powf((sqrtf(splineQueryResult.m_distanceSq) - (m_radius + variableRadius)) * maxScale.GetMaxElement(), 2.0f);
    }

    bool TubeShape::IntersectRay(const AZ::Vector3& src, const AZ::Vector3& dir, AZ::VectorFloat& distance)
    {
        AZ::Transform transformUniformScale = m_currentTransform;
        const AZ::VectorFloat maxScale = transformUniformScale.ExtractScale().GetMaxElement();
        transformUniformScale *= AZ::Transform::CreateScale(AZ::Vector3(maxScale));

        const auto splineQueryResult = IntersectSpline(transformUniformScale, src, dir, *m_spline);
        const float variableRadius = m_variableRadius.GetElementInterpolated(
            splineQueryResult.m_splineAddress.m_segmentIndex,
            splineQueryResult.m_splineAddress.m_segmentFraction, Lerpf);

        const float totalRadius = m_radius + variableRadius;
        distance = (splineQueryResult.m_rayDistance - totalRadius) * m_currentTransform.RetrieveScale().GetMaxElement();

        return static_cast<float>(sqrtf(splineQueryResult.m_distanceSq)) < totalRadius;
    }

    /**
     * Generates all vertex positions. Assumes the vertex pointer is valid
     */
    static void GenerateSolidTubeMeshVertices(
        const AZ::SplinePtr& spline, const SplineAttribute<float>& variableRadius,
        const float radius, const AZ::u32 sides, const AZ::u32 capSegments, AZ::Vector3* vertices)
    {
        // start cap
        auto address = spline->GetAddressByFraction(0.0f);
        AZ::Vector3 normal = spline->GetNormal(address);
        AZ::Vector3 previousTangent = spline->GetTangent(address);
        if (capSegments > 0)
        {
            vertices = CapsuleTubeUtil::GenerateSolidStartCap(
                spline->GetPosition(address),
                previousTangent,
                normal,
                radius + variableRadius.GetElementInterpolated(address.m_segmentIndex, address.m_segmentFraction, Lerpf),
                sides, capSegments, vertices);
        }

        // middle segments (body)
        const float stepDelta = 1.0f / static_cast<float>(spline->GetSegmentGranularity());
        const auto endIndex = address.m_segmentIndex + spline->GetSegmentCount();

        while (address.m_segmentIndex < endIndex)
        {
            for (auto step = 0; step <= spline->GetSegmentGranularity(); ++step)
            {
                const AZ::Vector3 currentTangent = spline->GetTangent(address);
                normal = CalculateNormal(normal, previousTangent, currentTangent);

                vertices = CapsuleTubeUtil::GenerateSegmentVertices(
                    spline->GetPosition(address),
                    currentTangent,
                    normal,
                    radius + variableRadius.GetElementInterpolated(address.m_segmentIndex, address.m_segmentFraction, Lerpf),
                    sides, vertices);

                address.m_segmentFraction += stepDelta;
                previousTangent = currentTangent;
            }
            address.m_segmentIndex++;
            address.m_segmentFraction = 0.f;
        }

        // end cap
        if (capSegments > 0)
        {
            const auto endAddress = spline->GetAddressByFraction(1.0f);
            CapsuleTubeUtil::GenerateSolidEndCap(
                spline->GetPosition(endAddress),
                spline->GetTangent(endAddress),
                normal,
                radius + variableRadius.GetElementInterpolated(endAddress.m_segmentIndex, endAddress.m_segmentFraction, Lerpf),
                sides, capSegments, vertices);
        }
    }

    /**
     * Generates vertices and indices for a tube shape
     * Split into two stages:
     * - Generate vertex positions
     * - Generate indices (faces)
     *
     * Heres a rough diagram of how it is built:
     *   ____________
     *  /_|__|__|__|_\
     *  \_|__|__|__|_/
     *
     *  - A single vertex at each end of the tube
     *  - Angled end cap segments
     *  - Middle segments
     */
    void GenerateSolidTubeMesh(
        const AZ::SplinePtr& spline, const SplineAttribute<float>& variableRadius,
        const float radius, const AZ::u32 capSegments, const AZ::u32 sides,
        AZStd::vector<AZ::Vector3>& vertexBufferOut,
        AZStd::vector<AZ::u32>& indexBufferOut)
    {
        const AZ::u32 segments = spline->GetSegmentCount() * spline->GetSegmentGranularity() + spline->GetSegmentCount() - 1;
        const AZ::u32 totalSegments = segments + capSegments * 2;
        const AZ::u32 capSegmentTipVerts = capSegments > 0 ? 2 : 0;
        const size_t numVerts = sides * (totalSegments+1) + capSegmentTipVerts;
        const size_t numTriangles = (sides * totalSegments) * 2 + (sides * capSegmentTipVerts);

        vertexBufferOut.resize(numVerts);
        indexBufferOut.resize(numTriangles * 3);

        GenerateSolidTubeMeshVertices(
            spline, variableRadius, radius,
            sides, capSegments, &vertexBufferOut[0]);

        CapsuleTubeUtil::GenerateSolidMeshIndices(
            sides, segments, capSegments, &indexBufferOut[0]);
    }

    /**
     * Compose Caps, Lines and Loops to produce a final wire mesh matching the style of other
     * debug draw components.
     */
    void GenerateWireTubeMesh(
        const AZ::SplinePtr& spline, const SplineAttribute<float>& variableRadius,
        const float radius, const AZ::u32 capSegments, const AZ::u32 sides,
        AZStd::vector<AZ::Vector3>& vertexBufferOut)
    {
        // notes on vert buffer size
        // total end segments
        // 2 verts for each segment
        //  2 * capSegments for one full half arc
        //   2 arcs per end
        //    2 ends
        // total segments
        // 2 verts for each segment
        //  2 lines - top and bottom
        //   2 lines - left and right
        // total loops
        // 2 verts for each segment
        //  loops == sides
        //   2 loops per segment
        const AZ::u32 segments = spline->GetSegmentCount() * spline->GetSegmentGranularity();
        const AZ::u32 totalEndSegments = capSegments * 2 * 2 * 2 * 2;
        const AZ::u32 totalSegments = segments * 2 * 2 * 2;
        const AZ::u32 totalLoops = 2 * sides * segments * 2;
        const bool hasEnds = capSegments > 0;
        const size_t numVerts = totalEndSegments + totalSegments + totalLoops;
        vertexBufferOut.resize(numVerts);

        AZ::Vector3* vertices = vertexBufferOut.begin();

        // start cap
        auto address = spline->GetAddressByFraction(0.0f);
        AZ::Vector3 side = spline->GetNormal(address);
        AZ::Vector3 nextSide = spline->GetNormal(address);
        AZ::Vector3 previousDirection = spline->GetTangent(address);
        if (hasEnds)
        {
            vertices = CapsuleTubeUtil::GenerateWireCap(
                spline->GetPosition(address),
                -previousDirection,
                side,
                radius + variableRadius.GetElementInterpolated(address.m_segmentIndex, address.m_segmentFraction, Lerpf),
                capSegments,
                vertices);
        }

        // body
        const float stepDelta = 1.0f / static_cast<float>(spline->GetSegmentGranularity());
        auto nextAddress = address;
        const auto endIndex = address.m_segmentIndex + spline->GetSegmentCount();
        while (address.m_segmentIndex < endIndex)
        {
            address.m_segmentFraction = 0.f;
            nextAddress.m_segmentFraction = stepDelta;

            for (auto step = 0; step < spline->GetSegmentGranularity(); ++step)
            {
                const auto position = spline->GetPosition(address);
                const auto nextPosition = spline->GetPosition(nextAddress);
                const auto direction = spline->GetTangent(address);
                const auto nextDirection = spline->GetTangent(nextAddress);
                side = CalculateNormal(side, previousDirection, direction);
                nextSide = CalculateNormal(nextSide, direction, nextDirection);
                const auto up = side.Cross(direction);
                const auto nextUp = nextSide.Cross(nextDirection);
                const auto finalRadius = radius + variableRadius.GetElementInterpolated(address.m_segmentIndex, address.m_segmentFraction, Lerpf);
                const auto nextFinalRadius = radius + variableRadius.GetElementInterpolated(nextAddress.m_segmentIndex, nextAddress.m_segmentFraction, Lerpf);

                // left line
                vertices = WriteVertex(
                    CapsuleTubeUtil::CalculatePositionOnSphere(
                        position, direction, side, finalRadius, 0.0f),
                    vertices);
                vertices = WriteVertex(
                    CapsuleTubeUtil::CalculatePositionOnSphere(
                        nextPosition, nextDirection, nextSide, nextFinalRadius, 0.0f),
                    vertices);
                // right line
                vertices = WriteVertex(
                    CapsuleTubeUtil::CalculatePositionOnSphere(
                        position, -direction, side, finalRadius, AZ::Constants::Pi),
                    vertices);
                vertices = WriteVertex(
                    CapsuleTubeUtil::CalculatePositionOnSphere(
                        nextPosition, -nextDirection, nextSide, nextFinalRadius, AZ::Constants::Pi),
                    vertices);
                // top line
                vertices = WriteVertex(
                    CapsuleTubeUtil::CalculatePositionOnSphere(
                        position, direction, up, finalRadius, 0.0f),
                    vertices);
                vertices = WriteVertex(
                    CapsuleTubeUtil::CalculatePositionOnSphere(
                        nextPosition, nextDirection, nextUp, nextFinalRadius, 0.0f),
                    vertices);
                // bottom line
                vertices = WriteVertex(
                    CapsuleTubeUtil::CalculatePositionOnSphere(
                        position, -direction, up, finalRadius, AZ::Constants::Pi),
                    vertices);
                vertices = WriteVertex(
                    CapsuleTubeUtil::CalculatePositionOnSphere(
                        nextPosition, -nextDirection, nextUp, nextFinalRadius, AZ::Constants::Pi),
                    vertices);

                // loops along each segment
                vertices = CapsuleTubeUtil::GenerateWireLoop(
                    position, direction, side, sides, finalRadius, vertices);

                vertices = CapsuleTubeUtil::GenerateWireLoop(
                    nextPosition, nextDirection, nextSide, sides, nextFinalRadius, vertices);

                address.m_segmentFraction += stepDelta;
                nextAddress.m_segmentFraction += stepDelta;
                previousDirection = direction;
            }

            address.m_segmentIndex++;
            nextAddress.m_segmentIndex++;
        }

        if (hasEnds)
        {
            const auto endAddress = spline->GetAddressByFraction(1.0f);
            const auto endPosition = spline->GetPosition(endAddress);
            const auto endDirection = spline->GetTangent(endAddress);
            const auto endRadius = radius + variableRadius.GetElementInterpolated(endAddress.m_segmentIndex, endAddress.m_segmentFraction, Lerpf);

            // end cap
            CapsuleTubeUtil::GenerateWireCap(
                endPosition, endDirection,
                nextSide, endRadius, capSegments,
                vertices);
        }
    }

    void GenerateTubeMesh(
        const AZ::SplinePtr& spline, const SplineAttribute<float>& variableRadius,
        const float radius, const AZ::u32 capSegments, const AZ::u32 sides,
        AZStd::vector<AZ::Vector3>& vertexBufferOut, AZStd::vector<AZ::u32>& indexBufferOut,
        AZStd::vector<AZ::Vector3>& lineBufferOut)
    {
        GenerateSolidTubeMesh(
            spline, variableRadius, radius,
            capSegments, sides, vertexBufferOut,
            indexBufferOut);

        GenerateWireTubeMesh(
            spline, variableRadius, radius,
            capSegments, sides, lineBufferOut);
    }

    void TubeShapeMeshConfig::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<TubeShapeMeshConfig>()
                ->Version(1)
                ->Field("EndSegments", &TubeShapeMeshConfig::m_endSegments)
                ->Field("Sides", &TubeShapeMeshConfig::m_sides)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<TubeShapeMeshConfig>("Configuration", "Tube Shape Mesh Configutation")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &TubeShapeMeshConfig::m_endSegments, "End Segments", "Number Of segments at each end of the tube in the editor")
                        ->Attribute(AZ::Edit::Attributes::Min, 1)
                        ->Attribute(AZ::Edit::Attributes::Max, 10)
                        ->Attribute(AZ::Edit::Attributes::Step, 1)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &TubeShapeMeshConfig::m_sides, "Sides", "Number of Sides of the tube in the editor")
                        ->Attribute(AZ::Edit::Attributes::Min, 3)
                        ->Attribute(AZ::Edit::Attributes::Max, 32)
                        ->Attribute(AZ::Edit::Attributes::Step, 1)
                        ;
            }
        }
    }
} // namespace LmbrCentral
