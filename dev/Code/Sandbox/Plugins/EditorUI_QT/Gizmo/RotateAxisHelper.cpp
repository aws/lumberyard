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
#include "stdafx.h"

//Editor
#include <Objects/DisplayContext.h>
#include <HitContext.h>
#include <Util/Math.h>
#include <IDisplayViewport.h>

//Local
#include "RotateAxisHelper.h"

// This constant is used with GetScreenScaleFactor and was found experimentally.
static const float kViewDistanceScaleFactor = 0.06f;

namespace RotationDrawHelper
{
    Axis::Axis(const ColorF& defaultColor, const ColorF& highlightColor)
    {
        SetColors(defaultColor, highlightColor);
    }

    void Axis::SetColors(const ColorF& defaultColor, const ColorF& highlightColor)
    {
        m_colors[StateDefault] = defaultColor;
        m_colors[StateHighlight] = highlightColor;
    }

    void Axis::Draw(DisplayContext& dc, const Vec3& position, const Vec3& axis, float angleRadians, float angleStepRadians, float radius, bool highlighted, float screenScale, bool drawInFront)
    {
        screenScale *= kViewDistanceScaleFactor;

        if (drawInFront)
        {
            bool set = dc.SetDrawInFrontMode(true);

            // Draw the front facing arc
            dc.SetColor(!highlighted ? m_colors[StateDefault] : m_colors[StateHighlight]);
            dc.DrawArc(position, radius * screenScale, 0.f, 360.f, RAD2DEG(angleStepRadians), axis);

            dc.SetDrawInFrontMode(set);
        }
        else
        {
            // Draw the front facing arc
            dc.SetColor(!highlighted ? m_colors[StateDefault] : m_colors[StateHighlight]);
            dc.DrawArc(position, radius * screenScale, RAD2DEG(angleRadians) - 90.f, 180.f, RAD2DEG(angleStepRadians), axis);

            // Draw the back side
            dc.SetColor(!highlighted ? Col_Gray : m_colors[StateHighlight]);
            dc.DrawArc(position, radius * screenScale, RAD2DEG(angleRadians) + 90.f, 180.f, RAD2DEG(angleStepRadians), axis);
        }

        static bool drawAxisMidPoint = false;
        if (drawAxisMidPoint)
        {
            const float kBallRadius = 0.085f;
            Vec3 a;
            Vec3 b;
            GetBasisVectors(axis, a, b);

            float cosAngle = cos(angleRadians);
            float sinAngle = sin(angleRadians);

            Vec3 offset;
            offset.x = position.x + (cosAngle * a.x + sinAngle * b.x) * screenScale * radius;
            offset.y = position.y + (cosAngle * a.y + sinAngle * b.y) * screenScale * radius;
            offset.z = position.z + (cosAngle * a.z + sinAngle * b.z) * screenScale * radius;

            dc.SetColor(!highlighted ? m_colors[StateDefault] : m_colors[StateHighlight]);
            dc.DrawBall(offset, kBallRadius * screenScale);
        }
    }

    void Axis::GenerateHitTestGeometry(HitContext& hc, const Vec3& position, float radius, float angleStepRadians, const Vec3& axis, float screenScale)
    {
        m_vertices.clear();

        // The number of vertices relies on the angleStepRadians, the smaller the angle, the higher the vertex count.
        int numVertices = static_cast<int>(std::ceil(g_PI2 / angleStepRadians));

        Vec3 a;
        Vec3 b;
        GetBasisVectors(axis, a, b);

        // The geometry is calculated by computing a circle aligned to the specified axis.
        float angle = 0.f;
        for (int i = 0; i < numVertices; ++i)
        {
            float cosAngle = cos(angle);
            float sinAngle = sin(angle);

            Vec3 p;
            p.x = position.x + (cosAngle * a.x + sinAngle * b.x) * radius * screenScale;
            p.y = position.y + (cosAngle * a.y + sinAngle * b.y) * radius * screenScale;
            p.z = position.z + (cosAngle * a.z + sinAngle * b.z) * radius * screenScale;
            m_vertices.push_back(p);

            angle += angleStepRadians;
        }
    }

    bool Axis::IntersectRayWithQuad(const Ray& ray, Vec3 quad[4], Vec3& contact)
    {
        contact = Vec3();

        // Tests ray vs. two quads, the front facing quad and a back facing quad.
        // will return true if an intersection occurs and the world space position of the contact.
        return (Intersect::Ray_Triangle(ray, quad[0], quad[1], quad[2], contact) || Intersect::Ray_Triangle(ray, quad[0], quad[2], quad[3], contact) ||
                Intersect::Ray_Triangle(ray, quad[0], quad[2], quad[1], contact) || Intersect::Ray_Triangle(ray, quad[0], quad[3], quad[2], contact));
    }

    bool Axis::HitTest(Vec3& position, HitContext& hc, float radius, float angleStepRadians, const Vec3& axis, float screenScale)
    {
        screenScale *= kViewDistanceScaleFactor;

        // Generate intersection testing geometry
        GenerateHitTestGeometry(hc, position, radius, angleStepRadians, axis, screenScale);

        Ray ray;
        ray.origin = hc.raySrc;
        ray.direction = hc.rayDir;

        // Calculate the face normal with the first two vertices in the intersection geometry.
        Vec3 vdir0 = (m_vertices[0] - m_vertices[1]).GetNormalized();
        Vec3 vdir1 = (m_vertices[2] - m_vertices[1]).GetNormalized();
        Vec3 normal = vdir0.Cross(vdir1);

        float shortestDistance = std::numeric_limits<float>::max();
        size_t numVertices = m_vertices.size();
        for (size_t i = 0; i < numVertices; ++i)
        {
            const Vec3& v0 = m_vertices[i];
            const Vec3& v1 = m_vertices[(i + 1) % numVertices];
            Vec3 right = (v0 - v1).Cross(normal).GetNormalized() * screenScale * m_hitTestWidth;

            // Calculates the quad vertices aligned to the face normal.
            Vec3 quad[4];
            quad[0] = v0 + right;
            quad[1] = v1 + right;
            quad[2] = v1 - right;
            quad[3] = v0 - right;

            Vec3 contact;
            if (IntersectRayWithQuad(ray, quad, contact))
            {
                Vec3 intersectionPoint;
                if (PointToLineDistance(v0, v1, contact, intersectionPoint))
                {
                    // Ensure the intersection is within the quad's extents
                    float distanceToIntersection = intersectionPoint.GetDistance(contact);
                    if (distanceToIntersection < shortestDistance)
                    {
                        shortestDistance = distanceToIntersection;
                    }
                }
            }
        }

        // if shortestDistance is less than the maximum possible distance, we have an intersection.
        if (shortestDistance < std::numeric_limits<float>::max() - FLT_EPSILON)
        {
            hc.dist = shortestDistance;
            return true;
        }

        return false;
    }

    void Axis::DebugDrawHitTestSurface(DisplayContext& dc, HitContext& hc, const Vec3& position, float radius, float angleStepRadians, const Vec3& axis, float screenScale)
    {
        // Generate the geometry for rendering.
        GenerateHitTestGeometry(hc, position, radius, angleStepRadians, axis, screenScale);

        // Calculate the face normal with the first two vertices in the intersection geometry.
        Vec3 vdir0 = (m_vertices[0] - m_vertices[1]).GetNormalized();
        Vec3 vdir1 = (m_vertices[2] - m_vertices[1]).GetNormalized();
        Vec3 normal = vdir0.Cross(vdir1);

        float shortestDistance = std::numeric_limits<float>::max();

        Ray ray;
        ray.origin = hc.raySrc;
        ray.direction = hc.rayDir;

        size_t numVertices = m_vertices.size();
        for (size_t i = 0; i < numVertices; ++i)
        {
            const Vec3& v0 = m_vertices[i];
            const Vec3& v1 = m_vertices[(i + 1) % numVertices];
            Vec3 right = (v0 - v1).Cross(normal).GetNormalized() * screenScale * m_hitTestWidth;

            // Calculates the quad vertices aligned to the face normal.
            Vec3 quad[4];
            quad[0] = v0 + right;
            quad[1] = v1 + right;
            quad[2] = v1 - right;
            quad[3] = v0 - right;

            // Draw double sided quad to ensure it is always visible regardless of camera orientation.
            dc.DrawQuad(quad[0], quad[1], quad[2], quad[3]);
            dc.DrawQuad(quad[3], quad[2], quad[1], quad[0]);

            Vec3 contact;
            if (IntersectRayWithQuad(ray, quad, contact))
            {
                Vec3 intersectionPoint;
                if (PointToLineDistance(v0, v1, contact, intersectionPoint))
                {
                    // Ensure the intersection is within the quad's extents
                    float distanceToIntersection = intersectionPoint.GetDistance(contact);
                    if (distanceToIntersection < shortestDistance)
                    {
                        shortestDistance = distanceToIntersection;

                        // Highlight the quad at which an intersection occurred.
                        auto c = dc.GetColor();
                        dc.SetColor(Col_Red);
                        dc.DrawQuad(quad[0], quad[1], quad[2], quad[3]);
                        dc.DrawQuad(quad[3], quad[2], quad[1], quad[0]);
                        dc.SetColor(c);
                    }
                }
            }
        }
    }
};