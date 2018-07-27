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
#include <LmbrCentral/Shape/SplineComponentBus.h>
#include <AzCore/Component/Entity.h>
#include <LegacyEntityConversion/LegacyEntityConversion.h>

#include "RoadsAndRiversLegacyConverter.h"

#include "Road.h"
#include "River.h"

#include "Objects/RoadObject.h"
#include "Objects/RiverObject.h"
#include "Material/Material.h"
#include "MathConversion.h"
#include "RoadsAndRivers/RoadsAndRiversBus.h"

namespace RoadsAndRivers
{
    static const char* RoadEditorComponentId = "{940DCC7C-149F-4465-9EDC-517B729A48FA}";
    static const char* RiverEditorComponentId = "{62FF15A5-3B03-4B3D-958A-5F40E94D1577}";
    static const char* SplineEditorComponentId = "{5B29D788-4885-4D56-BD9B-C0C45BE08EC1}";

    static AZ::Color ConvertFogColor(Vec3 legacyColor, float multiplier)
    {
        auto convert = [multiplier](float x)
        {
            x = clamp_tpl(x, 0.0f, 1.0f) * max(multiplier, 0.0f);
            return FtoI(x * 255.0f);
        };

        AZ::Color result;

        result.SetR8(convert(legacyColor.x));
        result.SetG8(convert(legacyColor.y));
        result.SetB8(convert(legacyColor.z));
        result.SetA8(255);

        return result;
    }

    void RoadsAndRiversConverter::TransformSplineNodes(AZ::Entity* entity, const char* componentId, const CRoadObject& legacySpline)
    {
        AZ::Transform transform = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(transform, entity->GetId(), &AZ::TransformBus::Events::GetWorldTM);
        AZ::Transform localFromWorldTransform = transform.GetInverseFull();

        AZStd::vector<AZ::Vector3> vertices;
        vertices.reserve(legacySpline.GetPointCount());
        for (size_t i = 0; i < legacySpline.GetPointCount(); ++i)
        {
            vertices.push_back(LYVec3ToAZVec3(legacySpline.GetPoint(i)));
        }

        LmbrCentral::SplineComponentRequestBus::Event(entity->GetId(), &LmbrCentral::SplineComponentRequests::SetVertices, vertices);
        LmbrCentral::SplineComponentRequestBus::Event(entity->GetId(), &LmbrCentral::SplineComponentRequests::ChangeSplineType, AZ::BezierSpline::RTTI_Type().GetHash());

        AZ::SplinePtr spline = nullptr;
        LmbrCentral::SplineComponentRequestBus::EventResult(spline, entity->GetId(), &LmbrCentral::SplineComponentRequests::GetSpline);

        for (int i = 0; i < legacySpline.m_points.size(); ++i)
        {
            float width = legacySpline.m_points[i].isDefaultWidth ? 0.0f : legacySpline.m_points[i].width - legacySpline.mv_width;
            RoadsAndRiversGeometryRequestsBus::Event(entity->GetId(), &RoadsAndRiversGeometryRequestsBus::Events::SetVariableWidth, i, width);
        }

        RoadsAndRiversGeometryRequestsBus::Event(entity->GetId(), &RoadsAndRiversGeometryRequestsBus::Events::SetGlobalWidth, legacySpline.mv_width);

        using namespace AZ::LegacyConversion::Util;
        SetVarHierarchy<float>(entity, componentId, { AZ_CRC("GlobalWidthSlider") }, legacySpline.mv_width);
    }

    bool RoadsAndRiversConverter::ConvertCommonProperties(AZ::Entity* entity, const char* componentId, CRoadObject& legacySpline)
    {
        using namespace AZ::LegacyConversion::Util;
        bool success = true;

        success &= SetVarHierarchy<float>(entity, componentId, { AZ_CRC("TileLength") }, legacySpline.mv_tileLength);
        success &= SetVarHierarchy<float>(entity, componentId, { AZ_CRC("SegmentLength") }, legacySpline.mv_step);
        success &= SetVarHierarchy<int>(entity, componentId, { AZ_CRC("SortPriority") }, legacySpline.mv_sortPrio);
        success &= SetVarHierarchy<float>(entity, componentId, { AZ_CRC("ViewDistanceMultiplier") }, legacySpline.mv_viewDistanceMultiplier);
        success &= SetVarHierarchy<AZ::u32>(entity, componentId, { AZ_CRC("MinSpec") }, legacySpline.GetMinSpec());

        if (auto material = legacySpline.GetMaterial())
        {
            LmbrCentral::MaterialOwnerRequestBus::Event(entity->GetId(), &LmbrCentral::MaterialOwnerRequestBus::Events::SetMaterial, material->GetMatInfo());
        }

        return success;
    }

    AZ::LegacyConversion::LegacyConversionResult RoadsAndRiversConverter::ConvertRoad(CBaseObject* entityToConvert)
    {
        using namespace AZ::LegacyConversion::Util;

        CRoadObject* roadObject = static_cast<CRoadObject*>(entityToConvert);
        if (!roadObject)
        {
            return AZ::LegacyConversion::LegacyConversionResult::Failed;
        }

        AZ::ComponentTypeList componentList = { RoadEditorComponentId, SplineEditorComponentId };
        AZ::Outcome<AZ::Entity*, AZ::LegacyConversion::CreateEntityResult> result(nullptr);
        AZ::LegacyConversion::LegacyConversionRequestBus::BroadcastResult(result, &AZ::LegacyConversion::LegacyConversionRequests::CreateConvertedEntity, entityToConvert, true, componentList);

        AZ::Entity *entity = result.GetValue();
        TransformSplineNodes(entity, RoadEditorComponentId, *roadObject);

        bool success = true;
        success &= ConvertCommonProperties(entity, RoadEditorComponentId, *roadObject);
        success &= SetVarHierarchy<bool>(entity, RoadEditorComponentId, { AZ_CRC("IgnoreTerrainHoles") }, roadObject->m_ignoreTerrainHoles);

        if (!success)
        {
            return AZ::LegacyConversion::LegacyConversionResult::Failed;
        }

        RoadRequestBus::Event(entity->GetId(), &RoadRequestBus::Events::Rebuild);
        return AZ::LegacyConversion::LegacyConversionResult::HandledDeleteEntity;
    }

    AZ::LegacyConversion::LegacyConversionResult RoadsAndRiversConverter::ConvertRiver(CBaseObject* entityToConvert)
    {
        using namespace AZ::LegacyConversion::Util;

        CRiverObject* riverObject = static_cast<CRiverObject*>(entityToConvert);
        if (!riverObject)
        {
            return AZ::LegacyConversion::LegacyConversionResult::Failed;
        }

        AZ::ComponentTypeList componentList = { RiverEditorComponentId, SplineEditorComponentId };
        AZ::Outcome<AZ::Entity*, AZ::LegacyConversion::CreateEntityResult> result(nullptr);
        AZ::LegacyConversion::LegacyConversionRequestBus::BroadcastResult(result, &AZ::LegacyConversion::LegacyConversionRequests::CreateConvertedEntity, entityToConvert, true, componentList);

        AZ::Entity *entity = result.GetValue();

        TransformSplineNodes(entity, RiverEditorComponentId, *riverObject);

        bool success = true;
        success &= ConvertCommonProperties(entity, RiverEditorComponentId, *riverObject);

        success &= SetVarHierarchy<float>(entity, RiverEditorComponentId, { AZ_CRC("TileLength") }, riverObject->mv_tileLength / AZStd::max(0.001f, (float)riverObject->mv_waterSurfaceUScale));
        success &= SetVarHierarchy<float>(entity, RiverEditorComponentId, { AZ_CRC("TileWidth") }, riverObject->mv_waterSurfaceVScale);
        success &= SetVarHierarchy<float>(entity, RiverEditorComponentId, { AZ_CRC("WaterFogDensity") }, riverObject->mv_waterFogDensity);
        success &= SetVarHierarchy<AZ::Color>(entity, RiverEditorComponentId, { AZ_CRC("FogColor") }, ConvertFogColor(riverObject->mv_waterFogColor, riverObject->mv_waterFogColorMultiplier));
        success &= SetVarHierarchy<bool>(entity, RiverEditorComponentId, { AZ_CRC("FogColorAffectedBySun") }, riverObject->mv_waterFogColorAffectedBySun);
        success &= SetVarHierarchy<float>(entity, RiverEditorComponentId, { AZ_CRC("WaterFogShadowing") }, riverObject->mv_waterFogShadowing);
        success &= SetVarHierarchy<bool>(entity, RiverEditorComponentId, { AZ_CRC("WaterCapFogAtVolumeDepth") }, riverObject->mv_waterCapFogAtVolumeDepth);
        success &= SetVarHierarchy<bool>(entity, RiverEditorComponentId, { AZ_CRC("WaterCaustics") }, riverObject->mv_waterCaustics);
        success &= SetVarHierarchy<float>(entity, RiverEditorComponentId, { AZ_CRC("WaterCausticIntensity") }, riverObject->mv_waterCausticIntensity);
        success &= SetVarHierarchy<float>(entity, RiverEditorComponentId, { AZ_CRC("WaterCausticTiling") }, riverObject->mv_waterCausticTiling);
        success &= SetVarHierarchy<float>(entity, RiverEditorComponentId, { AZ_CRC("WaterCausticHeight") }, riverObject->mv_waterCausticHeight);
        success &= SetVarHierarchy<float>(entity, RiverEditorComponentId, { AZ_CRC("WaterVolumeDepth") }, riverObject->mv_waterVolumeDepth);
        success &= SetVarHierarchy<float>(entity, RiverEditorComponentId, { AZ_CRC("WaterStreamSpeed") }, riverObject->mv_waterStreamSpeed);
        success &= SetVarHierarchy<bool>(entity, RiverEditorComponentId, { AZ_CRC("Physicalize") }, riverObject->mv_tileLength);

        if (!success)
        {
            return AZ::LegacyConversion::LegacyConversionResult::Failed;
        }

        RiverRequestBus::Event(entity->GetId(), &RiverRequestBus::Events::Rebuild);
        return AZ::LegacyConversion::LegacyConversionResult::HandledDeleteEntity;
    }

    RoadsAndRiversConverter::RoadsAndRiversConverter()
    {
        AZ::LegacyConversion::LegacyConversionEventBus::Handler::BusConnect();
    }

    RoadsAndRiversConverter::~RoadsAndRiversConverter()
    {
        AZ::LegacyConversion::LegacyConversionEventBus::Handler::BusDisconnect();
    }

    AZ::LegacyConversion::LegacyConversionResult RoadsAndRiversConverter::ConvertEntity(CBaseObject* entityToConvert)
    {
        using namespace AZ::LegacyConversion::Util;

        if (AZStd::string(entityToConvert->metaObject()->className()) == "CRoadObject")
        {
            return ConvertRoad(entityToConvert);
        }
        else if (AZStd::string(entityToConvert->metaObject()->className()) == "CRiverObject")
        {
            return ConvertRiver(entityToConvert);
        }

        return AZ::LegacyConversion::LegacyConversionResult::Ignored;
    }

    bool RoadsAndRiversConverter::BeforeConversionBegins()
    {
        return true;
    }

    bool RoadsAndRiversConverter::AfterConversionEnds()
    {
        return true;
    }
}