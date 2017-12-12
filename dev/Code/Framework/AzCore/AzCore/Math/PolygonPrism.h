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

#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/VertexContainer.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>

namespace AZ
{
    class Aabb;
    class SerializeContext;
    void PolygonPrismReflect(SerializeContext& context);

    /**
     * Formal Definition: A (right) polygonal prism is a 3-dimensional prism made from two translated polygons connected by rectangles. Parallelogram sides are not allowed.
     * Here the representation is defined by one polygon (internally represented as a vertex container - list of vertices) and a height (extrusion) property.
     * All points lie on the local plane Z = 0.
     */
    class PolygonPrism
    {
    public:
        AZ_RTTI(PolygonPrism, "{F01C8BDD-6F24-4344-8945-521A8750B30B}")
        AZ_CLASS_ALLOCATOR_DECL

        PolygonPrism() = default;
        virtual ~PolygonPrism() = default;

        void SetHeight(float height);
        float GetHeight() const { return m_height; }

        /**
         * Override callbacks to be used when polygon prism changes/is modified.
         */
        void SetCallbacks(const AZStd::function<void()>& OnChangeElement, const AZStd::function<void()>& OnChangeContainer);

        static void Reflect(SerializeContext& context);

        VertexContainer<Vector2> m_vertexContainer; ///< Reference to underlying vertex data.

    private:
        AZStd::function<void()> m_onChangeCallback = nullptr; ///< Callback for when height is changed.
        float m_height = 1.0f; ///< Height of polygon prism (about local Z) - default to 1m.

        /**
         * Internally used to call OnChangeCallback when values are modified in the property grid.
         */
        void OnChange() const;
    };

    using PolygonPrismPtr = AZStd::shared_ptr<PolygonPrism>;
    using ConstPolygonPrismPtr = AZStd::shared_ptr<const PolygonPrism>;

    /**
     * Small set of util functions for polygon prism
     */
    namespace PolygonPrismUtil
    {
        /**
         * Routine to calculate Aabb for orientated polygon prism shape
         */
        Aabb CalculateAabb(const PolygonPrism& polygonPrism, const Transform& transform);

        /**
         * Return if a point in world space is contained within a polygon prism shape
         */
        bool IsPointInside(const PolygonPrism& polygonPrism, const Vector3& point, const Transform& transform);

        /**
         * Return distance squared from point in world space from polygon prism shape
         */
        float DistanceSquaredFromPoint(const PolygonPrism& polygonPrism, const Vector3& point, const Transform& transform);
    }
}
