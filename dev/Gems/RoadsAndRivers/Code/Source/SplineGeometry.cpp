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
#include "StdAfx.h"
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Math/Spline.h>
#include <AzCore/Math/IntersectSegment.h>
#include <AzCore/Component/TransformBus.h>

#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <LmbrCentral/Shape/SplineComponentBus.h>

#include "SplineGeometry.h"
#include "Road.h"
#include "River.h"

namespace RoadsAndRivers
{
    namespace SplineGeometryMathUtils
    {
        SplineIterationData CaluclateIterationData(const AZ::Spline* spline, float segmentLength)
        {
            SplineIterationData result;

            result.m_splineLength = spline->GetSplineLength();
            result.m_chunksCount = static_cast<int>(ceil(result.m_splineLength / segmentLength));
            result.m_realStepSize = result.m_splineLength / result.m_chunksCount;

            result.m_chunksCount = AZStd::max(0, result.m_chunksCount);
            result.m_realStepSize = AZStd::max(0.0f, result.m_realStepSize);

            return result;
        }

        AZStd::pair<AZ::Vector3, AZ::Vector3> GetPointsAroundSpline(float width, AZ::ConstSplinePtr spline, float splineDist)
        {
            auto addressAtCurDist = spline->GetAddressByDistance(splineDist);

            auto localPos = spline->GetPosition(addressAtCurDist);
            auto localNormal = spline->GetNormal(addressAtCurDist);
            auto halfWidth = width * 0.5f;

            auto right = localPos - halfWidth * localNormal;
            auto left = localPos + halfWidth * localNormal;

            return { left, right };
        }
    }

    static const AZ::Vector2 SegmentLengthRange { 0.5f, 10.0f };

    void SplineGeometryWidthModifier::SetWidthAtIndex(AZ::u32 index, float width)
    {
        if (m_variableWidth.Size() > index)
        {
            m_variableWidth.SetElement(index, width);
        }
    }

    float SplineGeometryWidthModifier::GetWidthAtIndex(AZ::u32 index)
    {
        if (m_variableWidth.Size() > index)
        {
            return m_variableWidth.GetElement(index);
        }
        return 0.0f;
    }

    float SplineGeometryWidthModifier::GetWidthAt(float distance) const
    {
        if (m_dirtyFlag)
        {
            CacheInterpolator();
            m_dirtyFlag = false;
        }

        float splineWidth = m_widthInterpolator.GetWidth(distance);
        return m_globalWidth + splineWidth;
    }

    void SplineGeometryWidthModifier::CacheInterpolator() const
    {
        m_widthInterpolator.Clear();

        AZ::SplinePtr spline = nullptr;
        LmbrCentral::SplineComponentRequestBus::EventResult(spline, m_entityId, &LmbrCentral::SplineComponentRequests::GetSpline);

      
        for (size_t i = 0; i < m_variableWidth.Size(); ++i)
        {
            auto widthModifier = m_variableWidth.GetElement(i);
            auto distanceAlongTheSpline = spline->GetLength(AZ::SplineAddress(i));
            m_widthInterpolator.InsertDistanceWidthKeyFrame(distanceAlongTheSpline, widthModifier);
        }
    }

    void SplineGeometry::Activate(AZ::EntityId entityId)
    {
        LmbrCentral::SplineComponentNotificationBus::Handler::BusConnect(entityId);
        RoadsAndRiversGeometryRequestsBus::Handler::BusConnect(entityId);

        SetEntityId(entityId);
        m_widthModifiers.Activate(entityId);
    }

    void SplineGeometry::Deactivate()
    {
        LmbrCentral::SplineComponentNotificationBus::Handler::BusDisconnect();
        RoadsAndRiversGeometryRequestsBus::Handler::BusDisconnect();

        InvalidateEntityId();
        m_widthModifiers.Deactivate();
    }

    void SplineGeometry::BuildSplineMesh()
    {
        AZ::ConstSplinePtr spline = nullptr;
        LmbrCentral::SplineComponentRequestBus::EventResult(spline, m_entityId, &LmbrCentral::SplineComponentRequests::GetSpline);

        auto addChunkAtSplineDist = [spline, this](float t)
        {
            auto width = m_widthModifiers.GetWidthAt(t);
            auto points = SplineGeometryMathUtils::GetPointsAroundSpline(width, spline, t);

            SplineGeometrySector sector;
            sector.points = { points.first, points.second, points.first, points.second };
            sector.t0 = t / m_tileLength;

            return sector;
        };

        m_roadSectors.clear();

        if (spline->GetSegmentCount() < 1)
        {
            return;
        }

        auto iterateData = SplineGeometryMathUtils::CaluclateIterationData(spline.get(), m_segmentLength);
    
        float t = 0.0f;
        for (int i = 0; i <= iterateData.m_chunksCount; ++i)
        {
            m_roadSectors.push_back(addChunkAtSplineDist(t));
            t += iterateData.m_realStepSize;
        }

        auto curItem = m_roadSectors.begin();
        auto prevItem = curItem;
        while (++curItem != m_roadSectors.end())
        {
            prevItem->points[2] = curItem->points[0];
            prevItem->points[3] = curItem->points[1];
            prevItem->t1 = curItem->t0;

            prevItem++;
        }

        // The last secotr is helper sector to simplify mesh generation code
        if (m_roadSectors.size() > 0)
        {
            m_roadSectors.pop_back();
        }

        // mark end of the road for road alpha fading
        if (m_roadSectors.size() > 0)
        {
            m_roadSectors.back().t1 *= -1.f;
        }
    }

    void SplineGeometry::DrawGeometry(const AZ::Color& meshColor)
    {
        AZ_Assert(m_entityId.IsValid(), "[SplineGeometry::DrawGeometry()] Entity id is invalid");
        AzFramework::EntityDebugDisplayRequests* displayContext = AzFramework::EntityDebugDisplayRequestBus::FindFirstHandler();
        AZ_Assert(displayContext, "[SplineGeometry::DrawGeometry()] Invalid display context");

        displayContext->SetColor(meshColor);

        AZ::Transform transform = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(transform, m_entityId, &AZ::TransformBus::Events::GetWorldTM);
        displayContext->PushMatrix(transform);

        for (auto& sector : m_roadSectors)
        {
            displayContext->DrawLine(sector.points[0], sector.points[1]);
            for (size_t k = 0; k < sector.points.size(); k += 2)
            {
                if (k + 3 < sector.points.size())
                {
                    displayContext->DrawLine(sector.points[k + 1], sector.points[k + 3]);
                    displayContext->DrawLine(sector.points[k + 3], sector.points[k + 2]);
                    displayContext->DrawLine(sector.points[k + 2], sector.points[k]);
                }
            }
        }

        displayContext->PopMatrix();
    }

    void SplineGeometry::Clear()
    {
        m_roadSectors.clear();
    }

    void SplineGeometry::OnSplineChanged()
    {
        m_widthModifiers.SetDirty();
        GeneralPropertyModified();
    }

    void SplineGeometry::SetVariableWidth(AZ::u32 index, float width)
    {
        m_widthModifiers.SetWidthAtIndex(index, width);
        m_widthModifiers.SetDirty();
        WidthPropertyModified();
    }

    float SplineGeometry::GetVariableWidth(AZ::u32 index)
    {
        return m_widthModifiers.GetWidthAtIndex(index);
    }

    void SplineGeometry::SetGlobalWidth(float width)
    {
        m_widthModifiers.m_globalWidth = width;
        WidthPropertyModified();
    }

    float SplineGeometry::GetGlobalWidth()
    {
        return m_widthModifiers.m_globalWidth;
    }

    void SplineGeometry::SetTileLength(float tileLength)
    {
        m_tileLength = tileLength;
        GeneralPropertyModified();
    }

    float SplineGeometry::GetTileLength()
    {
        return m_tileLength;
    }

    void SplineGeometry::SetSegmentLength(float segmentLength)
    {
        m_segmentLength = AZ::GetClamp(segmentLength, SegmentLengthRange.GetX(), SegmentLengthRange.GetY());
        GeneralPropertyModified();
    }

    float SplineGeometry::GetSegmentLength()
    {
        return m_segmentLength;
    }

    AZ::u32 SplineGeometry::WidthPropertyModifiedInternal()
    {
        m_widthModifiers.SetDirty();
        WidthPropertyModified();
        return AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
    }

    AZ::Aabb SplineGeometry::GetAabb()
    {
        AZ::ConstSplinePtr spline = nullptr;
        LmbrCentral::SplineComponentRequestBus::EventResult(spline, m_entityId, &LmbrCentral::SplineComponentRequests::GetSpline);

        AZ::Transform transform = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(transform, m_entityId, &AZ::TransformBus::Events::GetWorldTM);

        AZ::Aabb splineAabb;
        splineAabb.SetNull();
        
        auto iterateData = SplineGeometryMathUtils::CaluclateIterationData(spline.get(), m_segmentLength);
        float t = 0.0f;
        for (int i = 0; i <= iterateData.m_chunksCount; ++i)
        {
            auto width = m_widthModifiers.GetWidthAt(t);
            auto points = SplineGeometryMathUtils::GetPointsAroundSpline(width, spline, t);

            splineAabb.AddPoint(points.first);
            splineAabb.AddPoint(points.second);

            t += iterateData.m_realStepSize;
        }
        
        splineAabb.ApplyTransform(transform);
        return splineAabb;
    }

    bool SplineGeometry::IntersectRay(const AZ::Vector3 &src, const AZ::Vector3 &dir, AZ::VectorFloat& distance)
    {
        AZ::Transform transform = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(transform, m_entityId, &AZ::TransformBus::Events::GetWorldTM);

        for (auto& sector : m_roadSectors)
        {
            auto point00 = transform * sector.points[0];
            auto point01 = transform * sector.points[1];
            auto point02 = transform * sector.points[2];
            auto point03 = transform * sector.points[3];

            const float cameraFrustrumDepth = 10000.0f;
            float t = 0.0f;
            AZ::Vector3 normal;
            AZ::Vector3 endRayPoint = src + dir * cameraFrustrumDepth;

            if (AZ::Intersect::IntersectSegmentTriangle(src, endRayPoint, point00, point02, point03, normal, distance)
                || AZ::Intersect::IntersectSegmentTriangle(src, endRayPoint, point00, point03, point01, normal, distance))
            {
                return true;
            }
        }

        return false;
    }

    void SplineGeometry::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<SplineGeometry>()
                ->Version(1)
                ->Field("Width", &SplineGeometry::m_widthModifiers)
                ->Field("SegmentLength", &SplineGeometry::m_segmentLength)
                ->Field("TileLength", &SplineGeometry::m_tileLength)
                ->Field("SortPriority", &SplineGeometry::m_sortPriority)
                ->Field("ViewDistanceMultiplier", &SplineGeometry::m_viewDistanceMultiplier)
                ->Field("MinSpec", &SplineGeometry::m_minSpec)
            ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<SplineGeometry>("Spline Geometry", "Properties of the spline shape")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &SplineGeometry::m_widthModifiers, "Per-Vertex Width Modifiers", "")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &SplineGeometry::WidthPropertyModifiedInternal)
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->ClassElement(AZ::Edit::ClassElements::Group, "Rendering")
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &SplineGeometry::m_segmentLength, "Segment length", "The length of a segment in meters")
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &SplineGeometry::GeneralPropertyModified)
                            ->Attribute(AZ::Edit::Attributes::Min, SegmentLengthRange.GetX())
                            ->Attribute(AZ::Edit::Attributes::Max, SegmentLengthRange.GetY())
                            ->Attribute(AZ::Edit::Attributes::Step, 0.1f)
                            ->Attribute(AZ::Edit::Attributes::Suffix, " m")
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &SplineGeometry::m_tileLength, "Tile length", "The distance in meters at which the texture will repeat")
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &SplineGeometry::GeneralPropertyModified)
                            ->Attribute(AZ::Edit::Attributes::Min, 0.5f)
                            ->Attribute(AZ::Edit::Attributes::Max, 100.0f)
                            ->Attribute(AZ::Edit::Attributes::Step, 0.1f)
                            ->Attribute(AZ::Edit::Attributes::Suffix, " m")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &SplineGeometry::m_sortPriority, "Sort priority", "")
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &SplineGeometry::RenderingPropertyModified)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &SplineGeometry::m_viewDistanceMultiplier, "View distance multiplier", "")
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &SplineGeometry::RenderingPropertyModified)
                            ->Attribute(AZ::Edit::Attributes::Suffix, "x")
                            ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                        ->DataElement(AZ::Edit::UIHandlers::ComboBox, &SplineGeometry::m_minSpec, "Minimum spec", "Min spec for the object to be active.")
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &SplineGeometry::RenderingPropertyModified)
                            ->EnumAttribute(EngineSpec::Never, "Never")
                            ->EnumAttribute(EngineSpec::VeryHigh, "Very high")
                            ->EnumAttribute(EngineSpec::High, "High")
                            ->EnumAttribute(EngineSpec::Medium, "Medium")
                            ->EnumAttribute(EngineSpec::Low, "Low")
                    ;
            }
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<RoadsAndRiversGeometryRequestsBus>("RoadsAndRiversGeometryRequestsBus")->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::Preview)->
                Event("SetGlobalWidth", &RoadsAndRiversGeometryRequestsBus::Events::SetGlobalWidth)->
                Event("GetGlobalWidth", &RoadsAndRiversGeometryRequestsBus::Events::GetGlobalWidth)->
                VirtualProperty("GlobalWidth", "GetGlobalWidth", "SetGlobalWidth")->

                Event("SetVariableWidth", &RoadsAndRiversGeometryRequestsBus::Events::SetVariableWidth)->
                Event("GetVariableWidth", &RoadsAndRiversGeometryRequestsBus::Events::GetVariableWidth)->

                Event("SetTileLength", &RoadsAndRiversGeometryRequestsBus::Events::SetTileLength)->
                Event("GetTileLength", &RoadsAndRiversGeometryRequestsBus::Events::GetTileLength)->
                VirtualProperty("TileLength", "GetTileLength", "SetTileLength")->

                Event("SetSegmentLength", &RoadsAndRiversGeometryRequestsBus::Events::SetSegmentLength)->
                Event("GetSegmentLength", &RoadsAndRiversGeometryRequestsBus::Events::GetSegmentLength)->
                VirtualProperty("SegmentLength", "GetSegmentLength", "SetSegmentLength")
                ;
        }
    }

    void SplineGeometryWidthModifier::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<SplineGeometryWidthModifier>()
                ->Version(1)
                ->Field("DefaultWidth", &SplineGeometryWidthModifier::m_globalWidth)
                ->Field("WidthAttribute", &SplineGeometryWidthModifier::m_variableWidth)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<SplineGeometryWidthModifier>("Width", "")
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &SplineGeometryWidthModifier::m_globalWidth, "Global width", "")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.1f)
                    ->Attribute(AZ::Edit::Attributes::Max, 50.0f)
                    ->Attribute(AZ::Edit::Attributes::Step, 0.1f)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &SplineGeometryWidthModifier::m_variableWidth, "Per-Vertex Width Modifiers", "")
                    ;
            }
        }
    }

    void SplineGeometryWidthModifier::Activate(AZ::EntityId entityId)
    {
        m_entityId = entityId;
        m_variableWidth.Activate(entityId);
    }

    void SplineGeometryWidthModifier::Deactivate()
    {
        m_variableWidth.Deactivate();
        m_entityId.SetInvalid();
    }

    namespace SplineUtils
    {
        void ReflectHelperClasses(AZ::ReflectContext* context)
        {
            SplineGeometryWidthModifier::Reflect(context);
            SplineGeometry::Reflect(context);
            Road::Reflect(context);
            River::Reflect(context);
        }
    }

    AZ_CLASS_ALLOCATOR_IMPL(SplineGeometryWidthModifier, AZ::SystemAllocator, 0);
    AZ_CLASS_ALLOCATOR_IMPL(SplineGeometry, AZ::SystemAllocator, 0);
}
