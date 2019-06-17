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

#include <AzCore/RTTI/RTTI.h>
#include <LmbrCentral/Shape/SplineAttribute.h>
#include <AzCore/Memory/Memory.h>
#include <IEntityRenderState.h>

#include "RoadsAndRivers/RoadsAndRiversBus.h"
#include "RoadRiverCommon.h"

namespace AzFramework
{
    class DebugDisplayRequests;
}

namespace RoadsAndRivers
{
    /**
     * Class used to provide variable width at any specific point along the spline
     */
    class SplineGeometryWidthModifier
    {
    public:
        friend class SplineGeometry;

        AZ_RTTI(SplineGeometryWidthModifier, "{F69CC9C6-5B29-4C17-8028-3167165F9EC7}");
        AZ_CLASS_ALLOCATOR_DECL

        SplineGeometryWidthModifier();
        virtual ~SplineGeometryWidthModifier() = default;
        static void Reflect(AZ::ReflectContext* context);

        void Activate(AZ::EntityId entityId);
        void Deactivate();
        
        void SetDirty() const { m_dirtyFlag = true; }

        void SetWidthAtIndex(AZ::u32 index, float width);
        float GetWidthAtIndex(AZ::u32 index);
        float GetWidthAt(float distance) const;
        float GetMaximumWidth() const { return m_widthInterpolator.GetMaximumWidth() + m_globalWidth; }

    private:
        static const float s_maxWidth;
        AZ::EntityId m_entityId;
        LmbrCentral::SplineAttribute<float> m_variableWidth;
        float m_globalWidth = 5.0f;

        mutable RoadWidthInterpolator m_widthInterpolator;
        mutable bool m_dirtyFlag = true;

        void CacheInterpolator() const;
    };

    /**
     * Class to represent mesh with variable width along the spline
     */
    class SplineGeometry 
        : private LmbrCentral::SplineComponentNotificationBus::Handler
        , public RoadsAndRiversGeometryRequestsBus::Handler
    {
    public:
        AZ_RTTI(SplineGeometry, "{1E31B92F-5188-4074-8F71-810A3B59CC6B}");
        AZ_CLASS_ALLOCATOR_DECL;

        // Added to prevent ebus handler propagation and subsequent crash per LY-89510
        // Todo: Replace this with a better solution - LY-89733
        SplineGeometry() = default;
        SplineGeometry(const SplineGeometry&) = delete;
        SplineGeometry(SplineGeometry&&) = delete;
        SplineGeometry& operator=(const SplineGeometry& rhs);
        SplineGeometry& operator=(SplineGeometry&&) = delete;

        virtual ~SplineGeometry() = default;
        static void Reflect(AZ::ReflectContext* context);

        virtual void Activate(AZ::EntityId entityId);
        virtual void Deactivate();

        void SetEntityId(AZ::EntityId entityId) { m_entityId = entityId; };
        AZ::EntityId GetEntityId() const { return m_entityId; }
        void InvalidateEntityId() { m_entityId.SetInvalid(); }

        /**
         * Generates mesh along the spline
         */
        void BuildSplineMesh();
        const AZStd::vector<SplineGeometrySector>& GetGeometrySectors() const { return m_roadSectors; }
        const SplineGeometryWidthModifier& GetWidthModifiers() const { return m_widthModifiers; }

        /**
         * Draws spline geometry in the editor viewport
         */
        void DrawGeometry(AzFramework::DebugDisplayRequests& debugDisplay, const AZ::Color& meshColor);
        virtual void Clear();

        float GetSegmentLength() const { return m_segmentLength; }

        AZ::Aabb GetAabb();
        bool IntersectRay(const AZ::Vector3& src, const AZ::Vector3& dir, AZ::VectorFloat& distance);

    protected:
        AZ::EntityId m_entityId;

        // Rendering properties
        int m_sortPriority = 0;
        float m_viewDistanceMultiplier = 1.0f;
        EngineSpec m_minSpec = EngineSpec::Low;

    private:

        // SplineComponentNotificationBus::Handler
        void OnSplineChanged() override;

        // SplineGeometryRequestsBus::Handler
        void SetVariableWidth(AZ::u32 index, float width) override;
        float GetVariableWidth(AZ::u32 index) override;
        void SetGlobalWidth(float width) override;
        float GetGlobalWidth() override;
        void SetTileLength(float tileLength) override;
        float GetTileLength() override;
        void SetSegmentLength(float segmentLength) override;
        float GetSegmentLength() override;
        AZStd::vector<AZ::Vector3> GetQuadVertices() const override;

        AZStd::vector<SplineGeometrySector> m_roadSectors;
        SplineGeometryWidthModifier m_widthModifiers;

        float m_tileLength = 10.0f;
        float m_segmentLength = 2.0f;

        virtual void GeneralPropertyModified() {};
        virtual void WidthPropertyModified() {};
        virtual void RenderingPropertyModified() {};

        AZ::u32 WidthPropertyModifiedInternal();
        void SegmentLengthModifiedInternal();
        void TileLengthModifiedInternal();
    };

    namespace SplineGeometryMathUtils
    {
        struct SplineIterationData
        {
            int m_chunksCount;
            float m_splineLength;
            float m_realStepSize;
        };

        SplineIterationData CaluclateIterationData(const AZ::Spline* spline, float segmentLength);
        AZStd::pair<AZ::Vector3, AZ::Vector3> GetPointsAroundSpline(float width, AZ::ConstSplinePtr spline, float splineDist);
    }

    namespace SplineUtils
    {
        /**
         * Reflects all the classes needed for Roads and Rivers in the order of dependencies
         */
        void ReflectHelperClasses(AZ::ReflectContext* context);
    }
}
