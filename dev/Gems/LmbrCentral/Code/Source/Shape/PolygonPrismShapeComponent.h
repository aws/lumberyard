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

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/VertexContainerInterface.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>
#include <LmbrCentral/Shape/PolygonPrismShapeComponentBus.h>

namespace LmbrCentral
{
    /**
     * Configuration data for PolygonPrismShapeComponent.
     * Internally represented as a vertex list with a height (extrusion) property.
     * All vertices must lie on the same plane to form a specialized type of prism, a polygon prism.
     * A Vector2 is used to enforce this.
     */
    class PolygonPrismCommon
    {
    public:
        AZ_CLASS_ALLOCATOR(PolygonPrismCommon, AZ::SystemAllocator, 0);
        AZ_RTTI(PolygonPrismCommon, "{BDB453DE-8A51-42D0-9237-13A9193BE724}");

        PolygonPrismCommon();
        virtual ~PolygonPrismCommon() = default;

        static void Reflect(AZ::ReflectContext* context);

        AZ::PolygonPrismPtr m_polygonPrism; ///< Reference to the underlying polygon prism data.
    };

    /**
     * Component interface for Polygon Prism.
     * Formal Definition: A polygonal prism is a 3-dimensional prism made from two translated polygons connected by rectangles.
     * Here the representation is defined by one polygon (internally represented as a vertex container - list of vertices) and a height (extrusion) property.
     * All points lie on the local plane Z = 0.
     */
    class PolygonPrismShapeComponent
        : public AZ::Component
        , private ShapeComponentRequestsBus::Handler
        , private PolygonPrismShapeComponentRequestsBus::Handler
        , private AZ::TransformNotificationBus::Handler
    {
    public:

        friend class EditorPolygonPrismShapeComponent;

        AZ_COMPONENT(PolygonPrismShapeComponent, "{AD882674-1D5D-4E40-B079-449B47D2492C}");

        PolygonPrismShapeComponent() = default;
        ~PolygonPrismShapeComponent() override = default;

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // ShapeComponent::Handler implementation
        AZ::Crc32 GetShapeType() override { return AZ_CRC("PolygonPrism", 0xd6b50036); }
        AZ::Aabb GetEncompassingAabb() override;
        bool IsPointInside(const AZ::Vector3& point) override;
        float DistanceSquaredFromPoint(const AZ::Vector3& point) override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // PolygonShapeShapeComponentRequestBus::Handler implementation
        AZ::ConstPolygonPrismPtr GetPolygonPrism() override;
        void SetHeight(float height) override;
        
        void AddVertex(const AZ::Vector2& vertex) override;
        void UpdateVertex(size_t index, const AZ::Vector2& vertex) override;
        void InsertVertex(size_t index, const AZ::Vector2& vertex) override;
        void RemoveVertex(size_t index) override;
        void SetVertices(const AZStd::vector<AZ::Vector2>& vertices) override;
        void ClearVertices() override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////////////
        // Transform notification bus listener
        // Called when the local transform of the entity has changed. Local transform update always implies world transform change too.
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;
        //////////////////////////////////////////////////////////////////////////////////

    protected:

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("ShapeService", 0xe86aa5fe));
            provided.push_back(AZ_CRC("PolygonPrismShapeService", 0x1cbc4ed4));
            provided.push_back(AZ_CRC("VertexContainerService", 0x22cf8e10));
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC("ShapeService", 0xe86aa5fe));
            incompatible.push_back(AZ_CRC("PolygonPrismShapeService", 0x1cbc4ed4));
            incompatible.push_back(AZ_CRC("VertexContainerService", 0x22cf8e10));
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC("TransformService", 0x8ee22c50));
        }

        static void Reflect(AZ::ReflectContext* context);

    private:

        PolygonPrismCommon m_polygonPrismCommon; ///< Stores configuration of a Polygon Prism Shape for this component

        /**
         * Runtime data - cache potentially expensive operations
         */
        class PolygonPrismIntersectionDataCache
            : public IntersectionTestDataCache<AZ::PolygonPrism>
        {
        public:

            virtual ~PolygonPrismIntersectionDataCache() = default;

            void UpdateIntersectionParams(const AZ::Transform& currentTransform,
                const AZ::PolygonPrism& polygonPrism) override;

            const AZ::Aabb& GetAabb() const
            {
                return m_aabb;
            }

        private:

            AZ::Aabb m_aabb; ///< Aabb of polygon prism shape.
        };

        PolygonPrismIntersectionDataCache m_intersectionDataCache; ///< Caches transient intersection data.

        AZ::Transform m_currentTransform; ///< Caches the current transform for the entity on which this component lives.
    };

    /**
     * Util function to wrap ShapeChanged notification Ebus call.
     */
    void ShapeChangedNotification(AZ::EntityId entityId);
} // namespace LmbrCentral