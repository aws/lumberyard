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

#include <AzCore/Math/Spline.h>
#include <AzCore/Math/IntersectSegment.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Serialization/EditContext.h>
#include <tuple>

namespace AZ
{
    /*static*/
    const float Spline::c_splineEpsilon = 0.00001f;
    /*static*/ const float SplineAddress::c_segmentFractionEpsilon = 0.001f;
    static const float c_projectRayLength = 1000.0f;
    static const u16 c_minGranularity = 2;
    static const u16 c_maxGranularity = 64;

    void SplineReflect(SerializeContext& context)
    {
        Spline::Reflect(context);
        LinearSpline::Reflect(context);
        BezierSpline::Reflect(context);
        CatmullRomSpline::Reflect(context);
    }

    /**
     * Helper to calculate segment length for generic spline (bezier/catmull-rom etc) using piecewise interpolation.
     * (Break spline into linear segments and sum length of each).
     */
    static float CalculateSegmentLengthPiecewise(const Spline& spline, size_t index)
    {
        const size_t granularity = spline.GetSegmentGranularity();

        float length = 0.0f;
        Vector3 pos = spline.GetPosition(SplineAddress(index, 0.0f));
        for (size_t i = 1; i <= granularity; ++i)
        {
            Vector3 nextPos = spline.GetPosition(SplineAddress(index, i / static_cast<float>(granularity)));
            length += (nextPos - pos).GetLength();
            pos = nextPos;
        }

        return length;
    }

    /**
     * Helper to calculate aabb for generic spline (bezier/catmull-rom etc) using piecewise interpolation.
     */
    static void CalculateAabbPiecewise(const Spline& spline, size_t begin, size_t end, Aabb& aabb, const Transform& transform /*= Transform::CreateIdentity()*/)
    {
        aabb.SetNull();

        const size_t granularity = spline.GetSegmentGranularity();
        for (size_t index = begin; index < end; ++index)
        {
            for (size_t granularStep = 0; granularStep <= granularity; ++granularStep)
            {
                aabb.AddPoint(transform * spline.GetPosition(SplineAddress(index, granularStep / static_cast<float>(granularity))));
            }
        }
    }

    /**
     * Functor to wrap ray query against a line segment - used in conjunction with GetNearestAddressInternal.
     */
    struct RayQuery
    {
        explicit RayQuery(const Vector3& localRaySrc, const Vector3& localRayDir)
            : m_localRaySrc(localRaySrc)
            , m_localRayDir(localRayDir) {}

        std::tuple<float, float> operator()(const Vector3& partBegin, const Vector3& partEnd) const
        {
            const Vector3 localRayEnd = m_localRaySrc + (m_localRayDir * c_projectRayLength);
            Vector3 closestPosRay, closestPosSegmentPart;
            VectorFloat rayProportion, segmentPartProportion;
            Intersect::ClosestSegmentSegment(m_localRaySrc, localRayEnd, partBegin, partEnd, rayProportion, segmentPartProportion, closestPosRay, closestPosSegmentPart);
            return std::make_tuple(((closestPosRay - closestPosSegmentPart).GetLengthSq()), segmentPartProportion);
        }

        Vector3 m_localRaySrc;
        Vector3 m_localRayDir;
    };

    /**
     * Functor to wrap position query against a line segment - used in conjunction with GetNearestAddressInternal.
     */
    struct PosQuery
    {
        explicit PosQuery(const Vector3& localPos)
            : m_localPos(localPos) {}

        std::tuple<float, float> operator()(const Vector3& partBegin, const Vector3& partEnd) const
        {
            Vector3 closestPosSegmentPart;
            VectorFloat segmentPartProportion;
            Intersect::ClosestPointSegment(m_localPos, partBegin, partEnd, segmentPartProportion, closestPosSegmentPart);
            return std::make_tuple((m_localPos - closestPosSegmentPart).GetLengthSq(), segmentPartProportion);
        }

        Vector3 m_localPos;
    };

    /**
     * Template function to do a generic distance query against line segments composing a spline.
     * @param begin Vertex to start iteration on.
     * @param end Vertex to end iteration on.
     * @param granularity Number of line segments making up each segment (tessellation).
     * @param spline Current spline the query is being conducted on - GetPosition call required.
     * @param calcDistfunc The functor responsible for doing the distance query - returning min distance and proportion along segment.
     * @return SplineAddress closest to given query on spline.
     */
    template<typename CalculateDistanceFunc>
    static SplineAddress GetNearestAddressInternal(const Spline& spline, size_t begin, size_t end, size_t granularity, CalculateDistanceFunc calcDistfunc)
    {
        float minDistanceSq = FLT_MAX;
        SplineAddress nearestSplineAddress;
        for (size_t currentVertex = begin; currentVertex < end; ++currentVertex)
        {
            Vector3 segmentPartBegin = spline.GetPosition(SplineAddress(currentVertex, 0.0f));
            for (size_t granularStep = 1; granularStep <= granularity; ++granularStep)
            {
                const Vector3 segmentPartEnd = spline.GetPosition(SplineAddress(currentVertex, granularStep / static_cast<float>(granularity)));

                float distanceSq, proportion;
                std::tie(distanceSq, proportion) = calcDistfunc(segmentPartBegin, segmentPartEnd);

                if (distanceSq < minDistanceSq)
                {
                    minDistanceSq = distanceSq;
                    nearestSplineAddress = SplineAddress(currentVertex, (granularStep - 1) / static_cast<float>(granularity) + proportion / static_cast<float>(granularity));
                }

                segmentPartBegin = segmentPartEnd;
            }
        }

        return nearestSplineAddress;
    }

    /**
     * Util function to calculate SplineAddress from a given distance along the spline.
     * Negative distances will return first address.
     * @param begin Vertex to start iteration on.
     * @param end Vertex to end iteration on.
     * @param spline Current spline the query is being conducted on.
     * @param distance Distance along the spline.
     * @return SplineAddress closest to given distance.
     */
    static SplineAddress GetAddressByDistanceInternal(const Spline& spline, size_t begin, size_t end, float distance)
    {
        if (distance < 0.0f)
        {
            return SplineAddress(begin);
        }

        size_t index = begin;
        float segmentLength = 0.0f;
        float currentSplineLength = 0.0f;
        for (; index < end; ++index)
        {
            segmentLength = spline.GetSegmentLength(index);
            if (currentSplineLength + segmentLength > distance)
            {
                break;
            }

            currentSplineLength += segmentLength;
        }

        if (index == end)
        {
            return SplineAddress(index - 1, 1.0f);
        }
        
        return SplineAddress(index, segmentLength > 0.0f ? ((distance - currentSplineLength) / segmentLength) : 0.0f);
    }

    /**
     * util function to calculate SplineAddress from a given percentage along the spline.
     * implemented in terms of GetAddressByDistanceInternal.
     * @param begin Vertex to start iteration on.
     * @param end Vertex to end iteration on.
     * @param spline Current spline the query is being conducted on.
     * @param fraction Fraction/percentage along the spline.
     * @return SplineAddress closest to given fraction/percentage.
     */
    static SplineAddress GetAddressByFractionInternal(const Spline& spline, size_t begin, size_t end, float fraction)
    {
        AZ_Assert(end > 0, "Invalid end index passed");
        const size_t segmentCount = spline.GetSegmentCount();
        if (fraction <= 0.0f || segmentCount == 0)
        {
            return SplineAddress(begin);
        }
        
        if (fraction >= 1.0f)
        {
            return SplineAddress(end - 1, 1.0f);
        }
        
        return GetAddressByDistanceInternal(spline, begin, end, spline.GetSplineLength() * fraction);
    }

    /**
     * util function to calculate total spline length.
     * @param begin Vertex to start iteration on.
     * @param end Vertex to end iteration on.
     * @param spline Current spline the query is being conducted on.
     * @return Spline length.
     */
    static float GetSplineLengthInternal(const Spline& spline, size_t begin, size_t end)
    {
        float splineLength = 0.0f;
        for (size_t i = begin; i < end; ++i)
        {
            splineLength += spline.GetSegmentLength(i);
        }

        return splineLength;
    }

    /**
     * util function to ensure range wraps correctly when stepping backwards.
     */
    static size_t GetPrevIndexWrapped(size_t index, size_t backwardStep, size_t size)
    {
        AZ_Assert(backwardStep < size, "Do not attempt to step back by the size or more");
        return (index - backwardStep + size) % size;
    }

    /**
     * util function to get segment count of spline based on vertex count and open/closed state.
     */
    static size_t GetSegmentCountInternal(const Spline& spline)
    {
        size_t vertexCount = spline.GetVertexCount();
        size_t additionalSegmentCount = vertexCount >= 2 ? (spline.IsClosed() ? 1 : 0) : 0;
        return vertexCount > 1 ? vertexCount - 1 + additionalSegmentCount : 0;
    }

    /**
     * util function to access the index of the last vertex in the spline.
     */
    static size_t GetLastVertexDefault(bool closed, size_t vertexCount)
    {
        return closed ? vertexCount : vertexCount - 1;
    }

    /**
     * Assign granularity if it is valid - only assign granularity if we know the
     * granularity value is large enough to matter (if it is 1, the spline will have
     * been linear - should keep default granularity)
     */
    static void TryAssignGranularity(u16& granularityOut, u16 granularity)
    {
        if (granularity > 1)
        {
            granularityOut = granularity;
        }
    }

    /*static*/ void Spline::Reflect(SerializeContext& context)
    {
        context.Class<Spline>()
            ->Field("Vertices", &Spline::m_vertexContainer)
            ->Field("Closed", &Spline::m_closed);

        if (EditContext* editContext = context.GetEditContext())
        {
            editContext->Class<Spline>("Spline", "Spline data")
                ->ClassElement(Edit::ClassElements::EditorData, "")
                ->Attribute(Edit::Attributes::Visibility, Edit::PropertyVisibility::ShowChildrenOnly)
                ->Attribute(Edit::Attributes::AutoExpand, true)
                ->Attribute(Edit::Attributes::ContainerCanBeModified, false)
                ->DataElement(Edit::UIHandlers::Default, &Spline::m_vertexContainer, "Vertices", "Data representing the spline, in the entity's local coordinate space")
                ->Attribute(Edit::Attributes::AutoExpand, true)
                ->DataElement(Edit::UIHandlers::CheckBox, &Spline::m_closed, "Closed", "Determine whether a spline is self closing (looping) or not")
                ->Attribute(Edit::Attributes::ChangeNotify, &Spline::OnSplineChanged);
        }
    }

    Spline::Spline()
        : m_vertexContainer(
            [this](size_t index) { OnVertexAdded(index); },
            [this](size_t index) { OnVertexRemoved(index); },
            [this]() { OnSplineChanged(); },
            [this]() { OnVerticesSet(); },
            [this]() { OnVerticesCleared(); })
    {
    }

    Spline::Spline(const Spline& spline)
        : m_vertexContainer(
            [this](size_t index) { OnVertexAdded(index); },
            [this](size_t index) { OnVertexRemoved(index); },
            [this]() { OnSplineChanged(); },
            [this]() { OnVerticesSet(); },
            [this]() { OnVerticesCleared(); })
        , m_closed(spline.m_closed)
    {
        m_vertexContainer.SetVertices(spline.GetVertices());
    }

    void Spline::SetCallbacks(const AZStd::function<void()>& OnChangeElement, const AZStd::function<void()>& OnChangeContainer)
    {
        m_vertexContainer.SetCallbacks(
            [this, OnChangeContainer](size_t index) { OnVertexAdded(index); if (OnChangeContainer) { OnChangeContainer(); } },
            [this, OnChangeContainer](size_t index) { OnVertexRemoved(index); if (OnChangeContainer) { OnChangeContainer(); } },
            [this, OnChangeElement]() { OnSplineChanged(); if (OnChangeElement) { OnChangeElement(); } },
            [this, OnChangeContainer]() { OnVerticesSet(); if (OnChangeContainer) { OnChangeContainer(); } },
            [this, OnChangeContainer]() { OnVerticesCleared(); if (OnChangeContainer) { OnChangeContainer(); } });
    }

    void Spline::SetClosed(bool closed)
    {
        if (closed != m_closed)
        {
            m_closed = closed;
            OnSplineChanged();
        }
    }

    void Spline::OnVertexAdded(size_t /*index*/)
    {
    }

    void Spline::OnVerticesSet()
    {
    }

    void Spline::OnVertexRemoved(size_t /*index*/)
    {
    }

    void Spline::OnVerticesCleared()
    {
    }

    void Spline::OnSplineChanged()
    {
    }

    SplineAddress LinearSpline::GetNearestAddress(const Vector3& localRaySrc, const Vector3& localRayDir) const
    {
        const size_t vertexCount = GetVertexCount();
        return vertexCount > 1
               ? GetNearestAddressInternal(*this, 0, GetLastVertexDefault(m_closed, vertexCount), GetSegmentGranularity(), RayQuery(localRaySrc, localRayDir))
               : SplineAddress();
    }

    SplineAddress LinearSpline::GetNearestAddress(const Vector3& localPos) const
    {
        const size_t vertexCount = GetVertexCount();
        return vertexCount > 1
               ? GetNearestAddressInternal(*this, 0, GetLastVertexDefault(m_closed, vertexCount), GetSegmentGranularity(), PosQuery(localPos))
               : SplineAddress();
    }

    SplineAddress LinearSpline::GetAddressByDistance(float distance) const
    {
        const size_t vertexCount = GetVertexCount();
        return vertexCount > 1
               ? GetAddressByDistanceInternal(*this, 0, GetLastVertexDefault(m_closed, vertexCount), distance)
               : SplineAddress();
    }

    SplineAddress LinearSpline::GetAddressByFraction(float fraction) const
    {
        const size_t vertexCount = GetVertexCount();
        return vertexCount > 1
               ? GetAddressByFractionInternal(*this, 0, GetLastVertexDefault(m_closed, vertexCount), fraction)
               : SplineAddress();
    }

    Vector3 LinearSpline::GetPosition(const SplineAddress& splineAddress) const
    {
        const size_t segmentCount = GetSegmentCount();
        if (segmentCount == 0)
        {
            return Vector3::CreateZero();
        }

        const size_t index = splineAddress.m_segmentIndex;
        if (!m_closed && index >= segmentCount)
        {
            Vector3 lastVertex;
            if (m_vertexContainer.GetLastVertex(lastVertex))
            {
                return lastVertex;
            }

            return Vector3::CreateZero();
        }

        const size_t nextIndex = (index + 1) % GetVertexCount();
        return GetVertex(index).Lerp(GetVertex(nextIndex), splineAddress.m_segmentFraction);
    }

    Vector3 LinearSpline::GetNormal(const SplineAddress& splineAddress) const
    {
        const size_t segmentCount = GetSegmentCount();
        if (segmentCount == 0)
        {
            return Vector3::CreateAxisX();
        }

        const size_t index = GetMin(splineAddress.m_segmentIndex, segmentCount - 1);
        return GetTangent(SplineAddress(index)).ZAxisCross().GetNormalizedSafe(c_splineEpsilon);
    }

    Vector3 LinearSpline::GetTangent(const SplineAddress& splineAddress) const
    {
        const size_t segmentCount = GetSegmentCount();
        if (segmentCount == 0)
        {
            return Vector3::CreateAxisX();
        }

        const size_t index = GetMin(splineAddress.m_segmentIndex, segmentCount - 1);
        const size_t nextIndex = (index + 1) % GetVertexCount();
        return (GetVertex(nextIndex) - GetVertex(index)).GetNormalizedSafe(c_splineEpsilon);
    }

    float LinearSpline::GetSplineLength() const
    {
        size_t vertexCount = GetVertexCount();
        return vertexCount > 1
               ? GetSplineLengthInternal(*this, 0, GetLastVertexDefault(m_closed, vertexCount))
               : 0.0f;
    }

    float LinearSpline::GetSegmentLength(size_t index) const
    {
        if (index >= GetSegmentCount())
        {
            return 0.0f;
        }

        size_t nextIndex = (index + 1) % GetVertexCount();
        return (GetVertex(nextIndex) - GetVertex(index)).GetLength();
    }

    size_t LinearSpline::GetSegmentCount() const
    {
        return GetSegmentCountInternal(*this);
    }

    void LinearSpline::GetAabb(Aabb& aabb, const Transform& transform /*= Transform::CreateIdentity()*/) const
    {
        // For lines, the AABB of the vertices is sufficient
        aabb.SetNull();
        for (const auto& vertex : m_vertexContainer.GetVertices())
        {
            aabb.AddPoint(transform * vertex);
        }
    }

    LinearSpline& LinearSpline::operator=(const Spline& spline)
    {
        Spline::operator=(spline);
        return *this;
    }

    /*static*/ void LinearSpline::Reflect(SerializeContext& context)
    {
        context.Class<LinearSpline, Spline>();

        if (EditContext* editContext = context.GetEditContext())
        {
            editContext->Class<LinearSpline>("Linear Spline", "Spline data")
                ->ClassElement(Edit::ClassElements::EditorData, "")
                ->Attribute(Edit::Attributes::Visibility, Edit::PropertyVisibility::ShowChildrenOnly)
                ->Attribute(Edit::Attributes::AutoExpand, true)
                ->Attribute(Edit::Attributes::ContainerCanBeModified, false);
        }
    }

    BezierSpline::BezierSpline(const Spline& spline)
        : Spline(spline)
    {
        size_t vertexCount = spline.GetVertexCount();

        BezierData bezierData;
        m_bezierData.reserve(vertexCount);
        for (size_t i = 0; i < vertexCount; ++i)
        {
            bezierData.m_forward = GetVertex(i);
            bezierData.m_back = bezierData.m_forward;
            m_bezierData.push_back(bezierData);
        }

        TryAssignGranularity(m_granularity, spline.GetSegmentGranularity());
        
        const size_t iterations = 2;
        CalculateBezierAngles(0, 0, iterations);
    }

    SplineAddress BezierSpline::GetNearestAddress(const Vector3& localRaySrc, const Vector3& localRayDir) const
    {
        size_t vertexCount = GetVertexCount();
        return vertexCount > 1
               ? GetNearestAddressInternal(*this, 0, GetLastVertexDefault(m_closed, vertexCount), GetSegmentGranularity(), RayQuery(localRaySrc, localRayDir))
               : SplineAddress();
    }

    SplineAddress BezierSpline::GetNearestAddress(const Vector3& localPos) const
    {
        size_t vertexCount = GetVertexCount();
        return vertexCount > 1
               ? GetNearestAddressInternal(*this, 0, GetLastVertexDefault(m_closed, vertexCount), GetSegmentGranularity(), PosQuery(localPos))
               : SplineAddress();
    }

    SplineAddress BezierSpline::GetAddressByDistance(float distance) const
    {
        size_t vertexCount = GetVertexCount();
        return vertexCount > 1
               ? GetAddressByDistanceInternal(*this, 0, GetLastVertexDefault(m_closed, vertexCount), distance)
               : SplineAddress();
    }

    SplineAddress BezierSpline::GetAddressByFraction(float fraction) const
    {
        size_t vertexCount = GetVertexCount();
        return vertexCount > 1
               ? GetAddressByFractionInternal(*this, 0, GetLastVertexDefault(m_closed, vertexCount), fraction)
               : SplineAddress();
    }

    Vector3 BezierSpline::GetPosition(const SplineAddress& splineAddress) const
    {
        const size_t segmentCount = GetSegmentCount();
        if (segmentCount == 0)
        {
            return Vector3::CreateZero();
        }

        const size_t index = splineAddress.m_segmentIndex;
        if (!m_closed && index >= segmentCount)
        {
            Vector3 lastVertex;
            if (m_vertexContainer.GetLastVertex(lastVertex))
            {
                return lastVertex;
            }

            return Vector3::CreateZero();
        }

        const size_t nextIndex = (index + 1) % GetVertexCount();

        const float t = splineAddress.m_segmentFraction;
        const float invt = 1.0f - t;
        const float invtSq = invt * invt;
        const float tSq = t * t;

        const Vector3& p0 = GetVertex(index);
        const Vector3& p1 = m_bezierData[index].m_forward;
        const Vector3& p2 = m_bezierData[nextIndex].m_back;
        const Vector3& p3 = GetVertex(nextIndex);

        // B(t) from https://en.wikipedia.org/wiki/B%C3%A9zier_curve#Cubic_B.C3.A9zier_curves
        return (p0 * invtSq * invt) +
               (p1 * 3.0f * t * invtSq) +
               (p2 * 3.0f * tSq * invt) +
               (p3 * tSq * t);
    }

    Vector3 BezierSpline::GetNormal(const SplineAddress& splineAddress) const
    {
        const size_t segmentCount = GetSegmentCount();
        if (segmentCount == 0)
        {
            return Vector3::CreateAxisX();
        }

        size_t index = splineAddress.m_segmentIndex;
        float t = splineAddress.m_segmentFraction;

        if (index >= segmentCount)
        {
            t = 1.0f;
            index = segmentCount - 1;
        }

        Vector3 tangent = GetTangent(SplineAddress(index, t));
        if (tangent.IsZero(c_splineEpsilon))
        {
            return Vector3::CreateAxisX();
        }

        const size_t nextIndex = (index + 1) % GetVertexCount();

        const VectorFloat an1 = m_bezierData[index].m_angle;
        const VectorFloat an2 = m_bezierData[nextIndex].m_angle;

        Vector3 normal;
        if (!an1.IsZero(c_splineEpsilon) || !an2.IsZero(c_splineEpsilon))
        {
            float af = t * 2.0f - 1.0f;
            float ed = 1.0f;
            if (af < 0.0f)
            {
                ed = -1.0f;
            }

            af = ed - af;
            af = af * af * af;
            af = ed - af;
            af = (af + 1.0f) * 0.5f;

            float angle = DegToRad((1.0f - af) * an1 + af * an2);

            tangent.Normalize();
            normal = tangent.ZAxisCross();
            Quaternion quat = Quaternion::CreateFromAxisAngle(tangent, angle);
            normal = quat * normal;
        }
        else
        {
            normal = tangent.ZAxisCross();
        }

        return normal.GetNormalizedSafe(c_splineEpsilon);
    }

    Vector3 BezierSpline::GetTangent(const SplineAddress& splineAddress) const
    {
        size_t segmentCount = GetSegmentCount();
        if (segmentCount == 0)
        {
            return Vector3::CreateAxisX();
        }

        float t = splineAddress.m_segmentFraction;
        size_t index = splineAddress.m_segmentIndex;

        if (index >= segmentCount)
        {
            index = segmentCount - 1;
            t = 1.0f;
        }

        const size_t nextIndex = (index + 1) % GetVertexCount();

        // B'(t) from https://en.wikipedia.org/wiki/B%C3%A9zier_curve#Cubic_B.C3.A9zier_curves
        const float invt = 1.0f - t;
        const float invtSq = invt * invt;
        const float tSq = t * t;

        const Vector3& p0 = GetVertex(index);
        const Vector3& p1 = m_bezierData[index].m_forward;
        const Vector3& p2 = m_bezierData[nextIndex].m_back;
        const Vector3& p3 = GetVertex(nextIndex);

        return (((p1 - p0) * 3.0f * invtSq) +
                ((p2 - p1) * 6.0f * invt * t) +
                ((p3 - p2) * 3.0f * tSq)).GetNormalizedSafe(c_splineEpsilon);
    }

    float BezierSpline::GetSplineLength() const
    {
        size_t vertexCount = GetVertexCount();
        return vertexCount > 1
               ? GetSplineLengthInternal(*this, 0, GetLastVertexDefault(m_closed, vertexCount))
               : 0.0f;
    }

    float BezierSpline::GetSegmentLength(size_t index) const
    {
        return CalculateSegmentLengthPiecewise(*this, index);
    }

    size_t BezierSpline::GetSegmentCount() const
    {
        return GetSegmentCountInternal(*this);
    }

    void BezierSpline::GetAabb(Aabb& aabb, const Transform& transform /*= Transform::CreateIdentity()*/) const
    {
        size_t vertexCount = GetVertexCount();
        if (vertexCount > 1)
        {
            CalculateAabbPiecewise(*this, 0, GetLastVertexDefault(m_closed, vertexCount), aabb, transform);
        }
        else
        {
            aabb.SetNull();
        }
    }

    BezierSpline& BezierSpline::operator=(const BezierSpline& bezierSpline)
    {
        Spline::operator=(bezierSpline);
        if (this != &bezierSpline)
        {
            m_bezierData.clear();
            for (const BezierData& bezierData : bezierSpline.m_bezierData)
            {
                m_bezierData.push_back(bezierData);
            }
        }

        m_granularity = bezierSpline.GetSegmentGranularity();

        return *this;
    }

    BezierSpline& BezierSpline::operator=(const Spline& spline)
    {
        Spline::operator=(spline);
        if (this != &spline)
        {
            BezierData bezierData;
            m_bezierData.clear();

            size_t vertexCount = spline.GetVertexCount();
            m_bezierData.reserve(vertexCount);

            for (size_t i = 0; i < vertexCount; ++i)
            {
                bezierData.m_forward = GetVertex(i);
                bezierData.m_back = bezierData.m_forward;
                m_bezierData.push_back(bezierData);
            }

            TryAssignGranularity(m_granularity, spline.GetSegmentGranularity());

            const size_t iterations = 2;
            CalculateBezierAngles(0, 0, iterations);
        }

        return *this;
    }

    void BezierSpline::OnVertexAdded(size_t index)
    {
        AddBezierDataForIndex(index);
        OnSplineChanged();
    }

    void BezierSpline::OnVerticesSet()
    {
        m_bezierData.clear();

        const size_t vertexCount = GetVertexCount();
        m_bezierData.reserve(vertexCount);

        for (size_t index = 0; index < vertexCount; ++index)
        {
            AddBezierDataForIndex(index);
        }

        const size_t iterations = 2;
        CalculateBezierAngles(0, 0, iterations);
    }

    void BezierSpline::OnVertexRemoved(size_t index)
    {
        m_bezierData.erase(m_bezierData.data() + index);
        OnSplineChanged();
    }

    void BezierSpline::OnVerticesCleared()
    {
        m_bezierData.clear();
    }

    void BezierSpline::OnSplineChanged()
    {
        const size_t startIndex = 1;
        const size_t range = 1;
        const size_t iterations = 2;
        
        CalculateBezierAngles(startIndex, range, iterations);
    }

    void BezierSpline::BezierAnglesCorrectionRange(size_t index, size_t range)
    {
        const size_t vertexCount = GetVertexCount();
        AZ_Assert(range < vertexCount, "Range should be less than vertexCount to prevent wrap around");

        if (m_closed)
        {
            const size_t fullRange = range * 2;
            size_t wrappingIndex = GetPrevIndexWrapped(index, range, vertexCount);
            for (size_t i = 0; i <= fullRange; ++i)
            {
                BezierAnglesCorrection(wrappingIndex);
                wrappingIndex = (wrappingIndex + 1) % vertexCount;
            }
        }
        else
        {
            size_t min = GetMin(index + range, vertexCount - 1);
            for (size_t i = index - range; i <= min; ++i)
            {
                BezierAnglesCorrection(i);
            }
        }
    }

    void BezierSpline::BezierAnglesCorrection(size_t index)
    {
        const size_t vertexCount = GetVertexCount();
        if (vertexCount == 0)
        {
            return;
        }

        const size_t lastVertex = vertexCount - 1;
        if (index > lastVertex)
        {
            return;
        }

        const AZStd::vector<Vector3>& vertices = m_vertexContainer.GetVertices();
        const Vector3& currentVertex = vertices[index];

        Vector3 currentVertexLeftControl = m_bezierData[index].m_back;
        Vector3 currentVertexRightControl = m_bezierData[index].m_forward;

        if (index == 0 && !m_closed)
        {
            currentVertexLeftControl = currentVertex;
            const Vector3& nextVertex = vertices[1];

            if (lastVertex == 1)
            {
                currentVertexRightControl = currentVertex + (nextVertex - currentVertex) / 3.0f;
            }
            else if (lastVertex > 0)
            {
                const Vector3& nextVertexLeftControl = m_bezierData[1].m_back;

                const VectorFloat vertexToNextVertexLeftControlDistance = (nextVertexLeftControl - currentVertex).GetLength();
                const VectorFloat vertexToNextVertexDistance = (nextVertex - currentVertex).GetLength();

                currentVertexRightControl = currentVertex +
                    (nextVertexLeftControl - currentVertex) /
                    (vertexToNextVertexLeftControlDistance / vertexToNextVertexDistance * 3.0f);
            }
        }
        else if (index == lastVertex && !m_closed)
        {
            currentVertexRightControl = currentVertex;
            if (index > 0)
            {
                const Vector3& previousVertex = vertices[index - 1];
                const Vector3& previousVertexRightControl = m_bezierData[index - 1].m_forward;

                const VectorFloat vertexToPreviousVertexRightControlDistance = (previousVertexRightControl - currentVertex).GetLength();
                const VectorFloat vertexToPreviousVertexDistance = (previousVertex - currentVertex).GetLength();

                if (!vertexToPreviousVertexRightControlDistance.IsZero() && !vertexToPreviousVertexDistance.IsZero())
                {
                    currentVertexLeftControl = currentVertex +
                        (previousVertexRightControl - currentVertex) /
                        (vertexToPreviousVertexRightControlDistance / vertexToPreviousVertexDistance * 3.0f);
                }
                else
                {
                    currentVertexLeftControl = currentVertex;
                }
            }
        }
        else
        {
            // If spline is closed, ensure indices wrap correctly
            const size_t prevIndex = GetPrevIndexWrapped(index, 1, vertexCount);
            const size_t nextIndex = (index + 1) % vertexCount;

            const Vector3& previousVertex = vertices[prevIndex];
            const Vector3& nextVertex = vertices[nextIndex];

            const VectorFloat nextVertexToPreviousVertexDistance = (nextVertex - previousVertex).GetLength();
            const VectorFloat previousVertexToCurrentVertexDistance = (previousVertex - currentVertex).GetLength();
            const VectorFloat nextVertexToCurrentVertexDistance = (nextVertex - currentVertex).GetLength();

            if (!nextVertexToPreviousVertexDistance.IsZero())
            {
                currentVertexLeftControl = currentVertex +
                    (previousVertex - nextVertex) *
                    (previousVertexToCurrentVertexDistance / nextVertexToPreviousVertexDistance / 3.0f);
                currentVertexRightControl = currentVertex +
                    (nextVertex - previousVertex) *
                    (nextVertexToCurrentVertexDistance / nextVertexToPreviousVertexDistance / 3.0f);
            }
            else
            {
                currentVertexLeftControl = currentVertex;
                currentVertexRightControl = currentVertex;
            }
        }

        m_bezierData[index].m_back = currentVertexLeftControl;
        m_bezierData[index].m_forward = currentVertexRightControl;
    }

    void BezierSpline::CalculateBezierAngles(size_t startIndex, size_t range, size_t iterations)
    {
        const size_t vertexCount = GetVertexCount();
        for (size_t i = 0; i < iterations; ++i)
        {
            for (size_t v = startIndex; v < vertexCount; ++v)
            {
                BezierAnglesCorrectionRange(v, range);
            }
        }
    }

    void BezierSpline::AddBezierDataForIndex(size_t index)
    {
        BezierData bezierData;
        bezierData.m_forward = GetVertex(index);
        bezierData.m_back = GetVertex(index);
        m_bezierData.insert(m_bezierData.data() + index, bezierData);
    }

    /*static*/ void BezierSpline::BezierData::Reflect(SerializeContext& context)
    {
        context.Class<BezierSpline::BezierData>()
            ->Field("Forward", &BezierSpline::BezierData::m_forward)
            ->Field("Back", &BezierSpline::BezierData::m_back)
            ->Field("Angle", &BezierSpline::BezierData::m_angle);
    }

    /*static*/ void BezierSpline::Reflect(SerializeContext& context)
    {
        BezierData::Reflect(context);

        context.Class<BezierSpline, Spline>()
            ->Field("Bezier Data", &BezierSpline::m_bezierData)
            ->Field("Granularity", &BezierSpline::m_granularity);

        if (EditContext* editContext = context.GetEditContext())
        {
            editContext->Class<BezierSpline>("Bezier Spline", "Spline data")
                ->ClassElement(Edit::ClassElements::EditorData, "")
                ->Attribute(Edit::Attributes::Visibility, Edit::PropertyVisibility::ShowChildrenOnly)
                ->Attribute(Edit::Attributes::AutoExpand, true)
                ->Attribute(Edit::Attributes::ContainerCanBeModified, false)
                ->DataElement(Edit::UIHandlers::Default, &BezierSpline::m_bezierData, "Bezier Data", "Data defining the bezier curve")
                ->Attribute(Edit::Attributes::Visibility, Edit::PropertyVisibility::Hide)
                ->DataElement(Edit::UIHandlers::Slider, &BezierSpline::m_granularity, "Granularity", "Parameter specifying the granularity of each segment in the spline")
                ->Attribute(Edit::Attributes::Min, c_minGranularity)
                ->Attribute(Edit::Attributes::Max, c_maxGranularity);
        }
    }

    CatmullRomSpline::CatmullRomSpline(const Spline& spline)
        : Spline(spline)
    {
        TryAssignGranularity(m_granularity, spline.GetSegmentGranularity());
    }

    static size_t GetLastVertexCatmullRom(bool closed, size_t vertexCount)
    {
        return vertexCount - (closed ? 0 : 2);
    }

    SplineAddress CatmullRomSpline::GetNearestAddress(const Vector3& localRaySrc, const Vector3& localRayDir) const
    {
        size_t vertexCount = GetVertexCount();
        return vertexCount > 3
               ? GetNearestAddressInternal(*this, m_closed ? 0 : 1, GetLastVertexCatmullRom(m_closed, vertexCount), GetSegmentGranularity(), RayQuery(localRaySrc, localRayDir))
               : SplineAddress();
    }

    SplineAddress CatmullRomSpline::GetNearestAddress(const Vector3& localPos) const
    {
        size_t vertexCount = GetVertexCount();
        return vertexCount > 3
               ? GetNearestAddressInternal(*this, m_closed ? 0 : 1, GetLastVertexCatmullRom(m_closed, vertexCount), GetSegmentGranularity(), PosQuery(localPos))
               : SplineAddress();
    }

    SplineAddress CatmullRomSpline::GetAddressByDistance(float distance) const
    {
        size_t vertexCount = GetVertexCount();
        return vertexCount > 3
               ? GetAddressByDistanceInternal(*this, m_closed ? 0 : 1, GetLastVertexCatmullRom(m_closed, vertexCount), distance)
               : SplineAddress();
    }

    SplineAddress CatmullRomSpline::GetAddressByFraction(float fraction) const
    {
        size_t vertexCount = GetVertexCount();
        return vertexCount > 3
               ? GetAddressByFractionInternal(*this, m_closed ? 0 : 1, GetLastVertexCatmullRom(m_closed, vertexCount), fraction)
               : SplineAddress();
    }

    Vector3 CatmullRomSpline::GetPosition(const SplineAddress& splineAddress) const
    {
        const size_t segmentCount = GetSegmentCount();
        if (segmentCount == 0)
        {
            return Vector3::CreateZero();
        }

        const AZStd::vector<Vector3>& vertices = m_vertexContainer.GetVertices();
        const size_t index = splineAddress.m_segmentIndex;
        const size_t vertexCount = GetVertexCount();
        if (!m_closed)
        {
            if (index < 1)
            {
                return vertices[1];
            }

            const size_t lastVertex = vertexCount - 2;
            if (index >= lastVertex)
            {
                return vertices[lastVertex];
            }
        }

        const size_t prevIndex = GetPrevIndexWrapped(index, 1, vertexCount);
        const size_t nextIndex = (index + 1) % vertexCount;
        const size_t nextNextIndex = (index + 2) % vertexCount;

        const Vector3& p0 = vertices[prevIndex];
        const Vector3& p1 = vertices[index];
        const Vector3& p2 = vertices[nextIndex];
        const Vector3& p3 = vertices[nextNextIndex];

        // VectorFloat t0 = 0.0f; // for reference
        const VectorFloat t1 = std::pow(p1.GetDistance(p0), m_knotParameterization) /* + t0 */;
        const VectorFloat t2 = std::pow(p2.GetDistance(p1), m_knotParameterization) + t1;
        const VectorFloat t3 = std::pow(p3.GetDistance(p2), m_knotParameterization) + t2;

        // Transform fraction from [0-1] to [t1,t2]
        const VectorFloat t = t1.Lerp(t2, splineAddress.m_segmentFraction);

        // Barry and Goldman's pyramidal formulation
        const Vector3 a1 = (t1 - t) / t1 * p0 + t / t1 * p1;
        const Vector3 a2 = (t2 - t) / (t2 - t1) * p1 + (t - t1) / (t2 - t1) * p2;
        const Vector3 a3 = (t3 - t) / (t3 - t2) * p2 + (t - t2) / (t3 - t2) * p3;

        const Vector3 b1 = (t2 - t) / t2 * a1 + t / t2 * a2;
        const Vector3 b2 = (t3 - t) / (t3 - t1) * a2 + (t - t1) / (t3 - t1) * a3;

        const Vector3 c = (t2 - t) / (t2 - t1) * b1 + (t - t1) / (t2 - t1) * b2;

        return c;
    }

    Vector3 CatmullRomSpline::GetNormal(const SplineAddress& splineAddress) const
    {
        const size_t segmentCount = GetSegmentCount();
        if (segmentCount == 0)
        {
            return Vector3::CreateAxisX();
        }

        return GetTangent(splineAddress).ZAxisCross().GetNormalizedSafe(c_splineEpsilon);
    }

    Vector3 CatmullRomSpline::GetTangent(const SplineAddress& splineAddress) const
    {
        const size_t segmentCount = GetSegmentCount();
        if (segmentCount == 0)
        {
            return Vector3::CreateAxisX();
        }

        const size_t vertexCount = GetVertexCount();
        size_t index = splineAddress.m_segmentIndex;
        VectorFloat fraction = splineAddress.m_segmentFraction;

        if (!m_closed)
        {
            if (index < 1)
            {
                index = 1;
                fraction = 0.0f;
            }
            else if (index >= vertexCount - 2)
            {
                index = vertexCount - 3;
                fraction = 1.0f;
            }
        }

        const size_t prevIndex = GetPrevIndexWrapped(index, 1, vertexCount);
        const size_t nextIndex = (index + 1) % vertexCount;
        const size_t nextNextIndex = (index + 2) % vertexCount;

        const AZStd::vector<Vector3>& vertices = m_vertexContainer.GetVertices();
        const Vector3& p0 = vertices[prevIndex];
        const Vector3& p1 = vertices[index];
        const Vector3& p2 = vertices[nextIndex];
        const Vector3& p3 = vertices[nextNextIndex];

        // VectorFloat t0 = 0.0f; // for reference
        const VectorFloat t1 = std::pow(p1.GetDistance(p0), m_knotParameterization) /* + t0 */;
        const VectorFloat t2 = std::pow(p2.GetDistance(p1), m_knotParameterization) + t1;
        const VectorFloat t3 = std::pow(p3.GetDistance(p2), m_knotParameterization) + t2;

        // Transform fraction from [0-1] to [t1,t2]
        const VectorFloat t = t1.Lerp(t2, fraction);

        // Barry and Goldman's pyramidal formulation
        const Vector3 a1 = (t1 - t) / t1 * p0 + t / t1 * p1;
        const Vector3 a2 = (t2 - t) / (t2 - t1) * p1 + (t - t1) / (t2 - t1) * p2;
        const Vector3 a3 = (t3 - t) / (t3 - t2) * p2 + (t - t2) / (t3 - t2) * p3;

        const Vector3 b1 = (t2 - t) / t2 * a1 + t / t2 * a2;
        const Vector3 b2 = (t3 - t) / (t3 - t1) * a2 + (t - t1) / (t3 - t1) * a3;

        // Derivative of Barry and Goldman's pyramidal formulation
        // (http://denkovacs.com/2016/02/catmull-rom-spline-derivatives/)
        const Vector3 a1p = (p1 - p0) / t1;
        const Vector3 a2p = (p2 - p1) / (t2 - t1);
        const Vector3 a3p = (p3 - p2) / (t3 - t2);

        const Vector3 b1p = ((a2 - a1) / t2) + (((t2 - t) / t2) * a1p) + ((t / t2) * a2p);
        const Vector3 b2p = ((a3 - a2) / (t3 - t1)) + (((t3 - t) / (t3 - t1)) * a2p) + (((t - t1) / (t3 - t1)) * a3p);

        const Vector3 cp = ((b2 - b1) / (t2 - t1)) + (((t2 - t) / (t2 - t1)) * b1p) + (((t - t1) / (t2 - t1)) * b2p);

        return cp.GetNormalizedSafe(c_splineEpsilon);
    }

    float CatmullRomSpline::GetSplineLength() const
    {
        size_t vertexCount = GetVertexCount();
        return vertexCount > 3
               ? GetSplineLengthInternal(*this, m_closed ? 0 : 1, GetLastVertexCatmullRom(m_closed, vertexCount))
               : 0.0f;
    }

    float CatmullRomSpline::GetSegmentLength(size_t index) const
    {
        return m_closed || index > 0
               ? CalculateSegmentLengthPiecewise(*this, index)
               : 0.0f;
    }

    size_t CatmullRomSpline::GetSegmentCount() const
    {
        // Catmull-Rom spline must have at least four vertices to be valid.
        size_t vertexCount = GetVertexCount();
        return vertexCount > 3 ? (m_closed ? vertexCount : vertexCount - 3) : 0;
    }

    // Gets the Aabb of the vertices in the spline
    void CatmullRomSpline::GetAabb(Aabb& aabb, const Transform& transform /*= Transform::CreateIdentity()*/) const
    {
        size_t vertexCount = GetVertexCount();
        if (vertexCount > 3)
        {
            CalculateAabbPiecewise(*this, m_closed ? 0 : 1, GetLastVertexCatmullRom(m_closed, vertexCount), aabb, transform);
        }
        else
        {
            aabb.SetNull();
        }
    }

    CatmullRomSpline& CatmullRomSpline::operator=(const Spline& spline)
    {
        Spline::operator=(spline);
        TryAssignGranularity(m_granularity, spline.GetSegmentGranularity());
        return *this;
    }

    /*static*/ void CatmullRomSpline::Reflect(SerializeContext& context)
    {
        context.Class<CatmullRomSpline, Spline>()
            ->Field("KnotParameterization", &CatmullRomSpline::m_knotParameterization)
            ->Field("Granularity", &CatmullRomSpline::m_granularity);

        if (EditContext* editContext = context.GetEditContext())
        {
            editContext->Class<CatmullRomSpline>("Catmull Rom Spline", "Spline data")
                ->ClassElement(Edit::ClassElements::EditorData, "")
                ->Attribute(Edit::Attributes::Visibility, Edit::PropertyVisibility::ShowChildrenOnly)
                ->Attribute(Edit::Attributes::AutoExpand, true)
                ->Attribute(Edit::Attributes::ContainerCanBeModified, false)
                ->DataElement(Edit::UIHandlers::Slider, &CatmullRomSpline::m_knotParameterization, "KnotParameterization", "Parameter specifying interpolation of the spline")
                ->Attribute(Edit::Attributes::Min, 0.0f)
                ->Attribute(Edit::Attributes::Max, 1.0f)
                ->DataElement(Edit::UIHandlers::Slider, &CatmullRomSpline::m_granularity, "Granularity", "Parameter specifying the granularity of each segment in the spline")
                ->Attribute(Edit::Attributes::Min, c_minGranularity)
                ->Attribute(Edit::Attributes::Max, c_maxGranularity);
        }
    }

    AZ_CLASS_ALLOCATOR_IMPL(Spline, SystemAllocator, 0)
    AZ_CLASS_ALLOCATOR_IMPL(LinearSpline, SystemAllocator, 0)
    AZ_CLASS_ALLOCATOR_IMPL(BezierSpline, SystemAllocator, 0)
    AZ_CLASS_ALLOCATOR_IMPL(BezierSpline::BezierData, SystemAllocator, 0)
    AZ_CLASS_ALLOCATOR_IMPL(CatmullRomSpline, SystemAllocator, 0)
}

#endif // #ifndef AZ_UNITY_BUILD
