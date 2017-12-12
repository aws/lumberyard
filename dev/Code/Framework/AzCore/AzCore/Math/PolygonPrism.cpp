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

#ifndef AZ_UNITY_BUILD

#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/IntersectSegment.h>
#include <AzCore/Math/PolygonPrism.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/VectorConversions.h>

#include <AzCore/Memory/SystemAllocator.h>

#include <AzCore/RTTI/BehaviorContext.h>

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AZ
{
    void PolygonPrismReflect(SerializeContext& context)
    {
        PolygonPrism::Reflect(context);
    }

    /*static*/ void PolygonPrism::Reflect(SerializeContext& serializeContext)
    {
        serializeContext.Class<PolygonPrism>()
            ->Version(1)
            ->Field("Height", &PolygonPrism::m_height)
            ->Field("VertexContainer", &PolygonPrism::m_vertexContainer);

        if (EditContext* editContext = serializeContext.GetEditContext())
        {
            editContext->Class<PolygonPrism>("PolygonPrism", "Polygon prism shape")
                ->ClassElement(Edit::ClassElements::EditorData, "")
                ->Attribute(Edit::Attributes::Visibility, Edit::PropertyVisibility::ShowChildrenOnly)
                ->DataElement(Edit::UIHandlers::Default, &PolygonPrism::m_height, "Height", "Shape Height")
                ->Attribute(Edit::Attributes::Suffix, " m")
                ->Attribute(Edit::Attributes::Step, 0.05f)
                ->Attribute(Edit::Attributes::ChangeNotify, &PolygonPrism::OnChange)
                ->DataElement(Edit::UIHandlers::Default, &PolygonPrism::m_vertexContainer, "Vertices", "Data representing the polygon, in the entity's local coordinate space")
                ->Attribute(Edit::Attributes::ContainerCanBeModified, false)
                ->Attribute(Edit::Attributes::AutoExpand, true);
        }

        if (BehaviorContext* behaviorContext = azrtti_cast<BehaviorContext*>(&serializeContext))
        {
            behaviorContext->Class<PolygonPrism>()
                ->Property("height", BehaviorValueProperty(&PolygonPrism::m_height));
        }
    }

    void PolygonPrism::SetHeight(float height)
    {
        if (!IsCloseMag(height, m_height))
        {
            m_height = height;
            OnChange();
        }
    }

    void PolygonPrism::OnChange() const
    {
        if (m_onChangeCallback)
        {
            m_onChangeCallback();
        }
    }

    void PolygonPrism::SetCallbacks(const AZStd::function<void()>& OnChangeElement, const AZStd::function<void()>& OnChangeContainer)
    {
        m_vertexContainer.SetCallbacks(
            [OnChangeContainer](size_t) { OnChangeContainer(); },
            [OnChangeContainer](size_t) { OnChangeContainer(); },
            OnChangeElement,
            OnChangeContainer,
            OnChangeContainer);

        m_onChangeCallback = OnChangeElement;
    }

    AZ_CLASS_ALLOCATOR_IMPL(PolygonPrism, SystemAllocator, 0)

    namespace PolygonPrismUtil
    {
        Aabb CalculateAabb(const PolygonPrism& polygonPrism, const Transform& transform)
        {
            const VertexContainer<Vector2>& vertexContainer = polygonPrism.m_vertexContainer;

            VectorFloat zero = VectorFloat::CreateZero();
            VectorFloat height = VectorFloat(polygonPrism.GetHeight());

            Aabb aabb = Aabb::CreateNull();
            // check base of prism
            for (const Vector2& vertex : vertexContainer.GetVertices())
            {
                aabb.AddPoint(transform * Vector3(vertex.GetX(), vertex.GetY(), zero));
            }

            // check top of prism
            // set aabb to be height of prism - ensure entire polygon prism shape is enclosed in aabb
            for (const Vector2& vertex : vertexContainer.GetVertices())
            {
                aabb.AddPoint(transform * Vector3(vertex.GetX(), vertex.GetY(), height));
            }

            return aabb;
        }

        bool IsPointInside(const PolygonPrism& polygonPrism, const Vector3& point, const Transform& transform)
        {
            using namespace PolygonPrismUtil;

            const VectorFloat c_epsilon = VectorFloat(0.0001f);
            const VectorFloat c_projectRayLength = VectorFloat(1000.0f);
            const VectorFloat zero = VectorFloat::CreateZero();

            const AZStd::vector<Vector2>& vertices = polygonPrism.m_vertexContainer.GetVertices();
            const size_t vertexCount = vertices.size();

            // transform point to local space
            const Vector3 localPoint = transform.GetInverseFast() * point;

            // ensure the point is not above or below the prism (in its local space)
            if (   localPoint.GetZ().IsLessThan(VectorFloat::CreateZero())
                || localPoint.GetZ().IsGreaterThan(polygonPrism.GetHeight()))
            {
                return false;
            }

            const Vector3 localPointFlattened = Vector3(localPoint.GetX(), localPoint.GetY(), zero);
            const Vector3 localEndFlattened = localPointFlattened + Vector3::CreateAxisX() * c_projectRayLength;

            size_t intersections = 0;
            // use 'crossing test' algorithm to decide if the point lies within the volume or not
            // (odd number of intersections - inside, even number of intersections - outside)
            for (size_t i = 0; i < vertexCount; ++i)
            {
                const Vector3 segmentStart = Vector2ToVector3(vertices[i]);
                const Vector3 segmentEnd = Vector2ToVector3(vertices[(i + 1) % vertexCount]);

                Vector3 closestPosRay, closestPosSegment;
                VectorFloat rayProportion, segmentProportion;
                Intersect::ClosestSegmentSegment(localPointFlattened, localEndFlattened, segmentStart, segmentEnd, rayProportion, segmentProportion, closestPosRay, closestPosSegment);
                const VectorFloat delta = (closestPosRay - closestPosSegment).GetLengthSq();

                // have we crossed/touched a line on the polygon
                if (delta.IsLessThan(c_epsilon))
                {
                    const Vector3 highestVertex = segmentStart.GetY().IsGreaterThan(segmentEnd.GetY()) ? segmentStart : segmentEnd;

                    const VectorFloat threshold = (highestVertex - point).Dot(Vector3::CreateAxisY());
                    if (segmentProportion.IsZero())
                    {
                        // if at beginning of segment, only count intersection if segment is going up (y-axis)
                        // (prevent counting segments twice when intersecting at vertex)
                        if (threshold.IsGreaterThan(zero))
                        {
                            intersections++;
                        }
                    }
                    else
                    {
                        intersections++;
                    }
                }
            }

            // odd inside, even outside - bitwise AND to convert to bool
            return intersections & 1;
        }

        float DistanceSquaredFromPoint(const PolygonPrism& polygonPrism, const Vector3& point, const Transform& transform)
        {
            const VectorFloat zero = VectorFloat::CreateZero();
            const VectorFloat height = VectorFloat(polygonPrism.GetHeight());

            const Vector3 localPoint = transform.GetInverseFast() * point;
            const Vector3 localPointFlattened = Vector3(localPoint.GetX(), localPoint.GetY(), VectorFloat::CreateZero());
            const Vector3 worldPointFlattened = transform * localPointFlattened;

            // first test if the point is contained within the polygon (flatten)
            if (IsPointInside(polygonPrism, worldPointFlattened, transform))
            {
                if (localPoint.GetZ().IsLessThan(zero))
                {
                    // if it's inside the 2d polygon but below the volume
                    const VectorFloat distance = fabsf(localPoint.GetZ());
                    return distance * distance;
                }
                
                if (localPoint.GetZ().IsGreaterThan(height))
                {
                    // if it's inside the 2d polygon but above the volume
                    const VectorFloat distance = localPoint.GetZ() - height;
                    return distance * distance;
                }
                
                // if it's fully contained, return 0
                return zero;
            }

            const AZStd::vector<Vector2>& vertices = polygonPrism.m_vertexContainer.GetVertices();
            const size_t vertexCount = vertices.size();

            // find closest segment
            Vector3 closestPos;
            VectorFloat minDistanceSq = VectorFloat(FLT_MAX);
            for (size_t i = 0; i < vertexCount; ++i)
            {
                Vector3 segmentStart = Vector2ToVector3(vertices[i]);
                Vector3 segmentEnd = Vector2ToVector3(vertices[(i + 1) % vertexCount]);

                Vector3 position;
                VectorFloat proportion;
                Intersect::ClosestPointSegment(localPointFlattened, segmentStart, segmentEnd, proportion, position);

                const VectorFloat distanceSq = (position - localPointFlattened).GetLengthSq();
                if (distanceSq < minDistanceSq)
                {
                    minDistanceSq = distanceSq;
                    closestPos = position;
                }
            }

            // constrain closest pos to [0, height] of volume
            closestPos += Vector3(zero, zero, localPoint.GetZ().GetClamp(zero, height));

            // return distanceSq from closest pos on prism
            return (closestPos - localPoint).GetLengthSq();
        }
    }
}

#endif // #ifndef AZ_UNITY_BUILD