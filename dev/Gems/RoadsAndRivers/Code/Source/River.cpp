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

#include <AzCore/Component/TransformBus.h>
#include <AzCore/Math/Vector2.h>

#include <IEntityRenderState.h>
#include <I3DEngine.h>

#include "River.h"
#include "MathConversion.h"

namespace RoadsAndRivers
{
    static bool IsMaterialGoodForRiver(_smart_ptr<IMaterial> material)
    {
#if defined(ROADS_RIVERS_EDITOR)
        if (material)
        {
            const auto& si = material->GetShaderItem();
            if (si.m_pShader && si.m_pShader->GetShaderType() != eST_Water)
            {
                AZ_Warning("RiverComponent", false, "Incorrect material set for the river volume!");
                return false;
            }
        }
#endif
        return true;
    }

    static const char* s_baseRiverMaterial = "Materials/River/defaultRiver";

    River::River()
    {
        m_material.SetAssetPath(s_baseRiverMaterial);
    }

    void River::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<River, SplineGeometry>()
                ->Version(1)
                ->Field("WaterVolumeDepth", &River::m_waterVolumeDepth)
                ->Field("Material", &River::m_material)
                ->Field("TileWidth", &River::m_tileWidth)
                ->Field("WaterCapFogAtVolumeDepth", &River::m_waterCapFogAtVolumeDepth)
                ->Field("WaterFogDensity", &River::m_waterFogDensity)
                ->Field("FogColor", &River::m_fogColor)
                ->Field("FogColorAffectedBySun", &River::m_fogColorAffectedBySun)
                ->Field("WaterFogShadowing", &River::m_waterFogShadowing)
                ->Field("WaterCaustics", &River::m_waterCaustics)
                ->Field("WaterCausticIntensity", &River::m_waterCausticIntensity)
                ->Field("WaterCausticHeight", &River::m_waterCausticHeight)
                ->Field("WaterCausticTiling", &River::m_waterCausticTiling)
                ->Field("Physicalize", &River::m_physicalize)
                ->Field("WaterStreamSpeed", &River::m_waterStreamSpeed)
            ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<River>("River", "River data")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &River::m_waterVolumeDepth, "Depth", "Specifies the Depth of the River.")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &River::PhysicsPropertyModified)
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 250.0f)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.1f)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &River::m_material, "River material", "Specify the river material")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &River::MaterialChanged)
                    ->ClassElement(AZ::Edit::ClassElements::Group, "Rendering")
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &River::m_tileWidth, "Tile width", "")
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &River::TilingPropertyModified)
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 100.0f)
                            ->Attribute(AZ::Edit::Attributes::Step, 0.1f)
                    ->ClassElement(AZ::Edit::ClassElements::Group, "Underwater Fog")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &River::m_waterCapFogAtVolumeDepth, "Cap at Depth", "If false, fog will continue to render below the specified Depth of the river.")
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &River::RenderingPropertyModified)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &River::m_waterFogDensity, "Density", "Specifies how dense the fog appears. Larger number will result in \"thicker\" fog.")
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &River::RenderingPropertyModified)
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 10.0f)
                            ->Attribute(AZ::Edit::Attributes::Step, 0.1f)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &River::m_fogColor, "Color", "Sets the fog color.")
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &River::RenderingPropertyModified)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &River::m_fogColorAffectedBySun, "Affected by Sun", "If true, SunColor value set in the Time of Day will affect fog color of the river.")
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &River::RenderingPropertyModified)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &River::m_waterFogShadowing, "Shadowing", 
                            "If enabled, surface of river will receive shadows. You can control the shadow darkness, valid input 0-1.\nr_FogShadowsWater needs to be set to '1' for this to function")
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &River::RenderingPropertyModified)
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                            ->Attribute(AZ::Edit::Attributes::Step, 0.001f)
                    ->ClassElement(AZ::Edit::ClassElements::Group, "Caustics")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &River::m_waterCaustics, "Enabled", "Enables Caustics.")
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &River::RenderingPropertyModified)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &River::m_waterCausticIntensity, "Intensity", "Intensity of normals during caustics generation.")
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &River::RenderingPropertyModified)
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 10.0f)
                            ->Attribute(AZ::Edit::Attributes::Step, 0.01f)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &River::m_waterCausticHeight, "Height", "Height above water surface caustics are visible.")
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &River::RenderingPropertyModified)
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 250.0f)
                            ->Attribute(AZ::Edit::Attributes::Step, 0.01f)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &River::m_waterCausticTiling, "Tiling", "Tilling of normals during caustics generation.")
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &River::RenderingPropertyModified)
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 10.0f)
                            ->Attribute(AZ::Edit::Attributes::Step, 0.01f)
                    ->ClassElement(AZ::Edit::ClassElements::Group, "Physics")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &River::m_physicalize, "Enabled", "Bind river with CryPhysics")
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &River::PhysicsPropertyModified)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &River::m_waterStreamSpeed, "Speed", "Defines how fast physicalized objects will be moved along the river. Use negative values to move in the opposite direction.")
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &River::PhysicsPropertyModified)
                ;
            }
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<RiverRequestBus>("RiverRequestBus")->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::Preview)->

                Event("SetWaterVolumeDepth", &RiverRequestBus::Events::SetWaterVolumeDepth)->
                Event("GetWaterVolumeDepth", &RiverRequestBus::Events::GetWaterVolumeDepth)->
                VirtualProperty("WaterVolumeDepth", "GetWaterVolumeDepth", "SetWaterVolumeDepth")->

                Event("SetTileWidth", &RiverRequestBus::Events::SetTileWidth)->
                Event("GetTileWidth", &RiverRequestBus::Events::GetTileWidth)->
                VirtualProperty("TileWidth", "GetTileWidth", "SetTileWidth")->

                Event("SetWaterCapFogAtVolumeDepth", &RiverRequestBus::Events::SetWaterCapFogAtVolumeDepth)->
                Event("GetWaterCapFogAtVolumeDepth", &RiverRequestBus::Events::GetWaterCapFogAtVolumeDepth)->
                VirtualProperty("WaterCapFogAtVolumeDepth", "GetWaterCapFogAtVolumeDepth", "SetWaterCapFogAtVolumeDepth")->

                Event("SetWaterFogDensity", &RiverRequestBus::Events::SetWaterFogDensity)->
                Event("GetWaterFogDensity", &RiverRequestBus::Events::GetWaterFogDensity)->
                VirtualProperty("WaterFogDensity", "GetWaterFogDensity", "SetWaterFogDensity")->

                Event("SetFogColor", &RiverRequestBus::Events::SetFogColor)->
                Event("GetFogColor", &RiverRequestBus::Events::GetFogColor)->
                VirtualProperty("FogColor", "GetFogColor", "SetFogColor")->

                Event("SetFogColorAffectedBySun", &RiverRequestBus::Events::SetFogColorAffectedBySun)->
                Event("GetFogColorAffectedBySun", &RiverRequestBus::Events::GetFogColorAffectedBySun)->
                VirtualProperty("FogColorAffectedBySun", "GetFogColorAffectedBySun", "SetFogColorAffectedBySun")->

                Event("SetWaterFogShadowing", &RiverRequestBus::Events::SetWaterFogShadowing)->
                Event("GetWaterFogShadowing", &RiverRequestBus::Events::GetWaterFogShadowing)->
                VirtualProperty("WaterFogShadowing", "GetWaterFogShadowing", "SetWaterFogShadowing")->

                Event("SetWaterCaustics", &RiverRequestBus::Events::SetWaterCaustics)->
                Event("GetWaterCaustics", &RiverRequestBus::Events::GetWaterCaustics)->
                VirtualProperty("WaterCaustics", "GetWaterCaustics", "SetWaterCaustics")->

                Event("SetWaterCausticIntensity", &RiverRequestBus::Events::SetWaterCausticIntensity)->
                Event("GetWaterCausticIntensity", &RiverRequestBus::Events::GetWaterCausticIntensity)->
                VirtualProperty("WaterCausticIntensity", "GetWaterCausticIntensity", "SetWaterCausticIntensity")->

                Event("SetWaterCausticHeight", &RiverRequestBus::Events::SetWaterCausticHeight)->
                Event("GetWaterCausticHeight", &RiverRequestBus::Events::GetWaterCausticHeight)->
                VirtualProperty("WaterCausticHeight", "GetWaterCausticHeight", "SetWaterCausticHeight")->

                Event("SetWaterCausticTiling", &RiverRequestBus::Events::SetWaterCausticTiling)->
                Event("GetWaterCausticTiling", &RiverRequestBus::Events::GetWaterCausticTiling)->
                VirtualProperty("WaterCausticTiling", "GetWaterCausticTiling", "SetWaterCausticTiling")->

                Event("SetPhysicalize", &RiverRequestBus::Events::SetPhysicsEnabled)->
                Event("GetPhysicalize", &RiverRequestBus::Events::GetPhysicsEnabled)->
                VirtualProperty("Physicalize", "GetPhysicalize", "SetPhysicalize")->

                Event("SetWaterStreamSpeed", &RiverRequestBus::Events::SetWaterStreamSpeed)->
                Event("GetWaterStreamSpeed", &RiverRequestBus::Events::GetWaterStreamSpeed)->
                VirtualProperty("WaterStreamSpeed", "GetWaterStreamSpeed", "SetWaterStreamSpeed")->

                Event("Rebuild", &RiverRequestBus::Events::Rebuild)
            ;
        }
    }

    void River::Activate(AZ::EntityId entityId)
    {
        SplineGeometry::Activate(entityId);

        AZ::TransformNotificationBus::Handler::BusConnect(entityId);
        LmbrCentral::MaterialOwnerRequestBus::Handler::BusConnect(entityId);
        RiverRequestBus::Handler::BusConnect(entityId);

        Rebuild();
    }

    void River::Deactivate()
    {
        SplineGeometry::Deactivate();

        AZ::TransformNotificationBus::Handler::BusDisconnect();
        LmbrCentral::MaterialOwnerRequestBus::Handler::BusDisconnect();
        RiverRequestBus::Handler::BusDisconnect();

        Clear();
    }

    void River::SetMaterial(_smart_ptr<IMaterial> material)
    {
        if (material && IsMaterialGoodForRiver(material))
        {
            m_material.SetAssetPath(material->GetName());
            for (const auto& node : m_renderNodes)
            {
                node.GetRenderNode()->SetMaterial(material);
            }
        }
    }

    _smart_ptr<IMaterial> River::GetMaterial()
    {
        auto firstNodeIt = m_renderNodes.begin();
        if (firstNodeIt != m_renderNodes.end())
        {
            return firstNodeIt->GetRenderNode()->GetMaterial();
        }
        return nullptr;
    }

    void River::SetMaterialHandle(const LmbrCentral::MaterialHandle& materialHandle)
    {
        SetMaterial(materialHandle.m_material);
    }

    LmbrCentral::MaterialHandle River::GetMaterialHandle()
    {
        auto handle = LmbrCentral::MaterialHandle();
        handle.m_material = GetMaterial();
        return handle;
    }

    void River::SetWaterVolumeDepth(float waterVolumeDepth)
    {
        m_waterVolumeDepth = waterVolumeDepth;
        PhysicsPropertyModified();
    }

    float River::GetWaterVolumeDepth()
    {
        return m_waterVolumeDepth;
    }

    void River::SetTileWidth(float tileWidth)
    {
        m_tileWidth = tileWidth;
        TilingPropertyModified();
    }

    float River::GetTileWidth()
    {
        return m_tileWidth;
    }

    void River::SetWaterCapFogAtVolumeDepth(bool waterCapFogAtVolumeDepth)
    {
        m_waterCapFogAtVolumeDepth = waterCapFogAtVolumeDepth;
        RenderingPropertyModified();
    }

    bool River::GetWaterCapFogAtVolumeDepth()
    {
        return m_waterCapFogAtVolumeDepth;
    }

    void River::SetWaterFogDensity(float waterFogDensity)
    {
        m_waterFogDensity = waterFogDensity;
        RenderingPropertyModified();
    }

    float River::GetWaterFogDensity()
    {
        return m_waterFogDensity;
    }

    void River::SetFogColor(AZ::Color fogColor)
    {
        m_fogColor = fogColor;
        RenderingPropertyModified();
    }

    AZ::Color River::GetFogColor()
    {
        return m_fogColor;
    }

    void River::SetFogColorAffectedBySun(bool fogColorAffectedBySun)
    {
        m_fogColorAffectedBySun = fogColorAffectedBySun;
        RenderingPropertyModified();
    }

    bool River::GetFogColorAffectedBySun()
    {
        return m_fogColorAffectedBySun;
    }

    void River::SetWaterFogShadowing(float waterFogShadowing)
    {
        m_waterFogShadowing = waterFogShadowing;
        RenderingPropertyModified();
    }

    float River::GetWaterFogShadowing()
    {
        return m_waterFogShadowing;
    }

    void River::SetWaterCaustics(bool waterCaustics)
    {
        m_waterCaustics = waterCaustics;
        RenderingPropertyModified();
    }

    bool River::GetWaterCaustics()
    {
        return m_waterCaustics;
    }

    void River::SetWaterCausticIntensity(float waterCausticIntensity)
    {
        m_waterCausticIntensity = waterCausticIntensity;
        RenderingPropertyModified();
    }

    float River::GetWaterCausticIntensity()
    {
        return m_waterCausticIntensity;
    }

    void River::SetWaterCausticHeight(float waterCausticHeight)
    {
        m_waterCausticHeight = waterCausticHeight;
        RenderingPropertyModified();
    }

    float River::GetWaterCausticHeight()
    {
        return m_waterCausticHeight;
    }

    void River::SetWaterCausticTiling(float waterCausticTiling)
    {
        m_waterCausticTiling = waterCausticTiling;
        RenderingPropertyModified();
    }

    float River::GetWaterCausticTiling()
    {
        return m_waterCausticTiling;
    }

    void River::SetPhysicsEnabled(bool physicalize)
    {
        m_physicalize = physicalize;
        PhysicsPropertyModified();
    }

    bool River::GetPhysicsEnabled()
    {
        return m_physicalize;
    }

    void River::SetWaterStreamSpeed(float waterStreamSpeed)
    {
        m_waterStreamSpeed = waterStreamSpeed;
        PhysicsPropertyModified();
    }

    float River::GetWaterStreamSpeed()
    {
        return m_waterStreamSpeed;
    }

    void River::Rebuild()
    {
        AZ_Assert(m_entityId.IsValid(), "[River::Rebuild()] Entity id is invalid");

        BuildSplineMesh();
        GenerateRenderNodes();
    }

    void River::Clear()
    {
        SplineGeometry::Clear();
        m_renderNodes.clear();
    }

    void River::GeneralPropertyModified()
    {
        Rebuild();
    }

    void River::WidthPropertyModified()
    {
        Rebuild();
    }

    void River::RenderingPropertyModified()
    {
        SetRenderProperties();
    }

    void River::OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world)
    {
        GenerateRenderNodes();
    }

    void River::MaterialChanged()
    {
        UpdateRenderNodeWithAssetMaterial();
    }

    void River::PhysicsPropertyModified()
    {
        SetPhysicalProperties();
    }

    void River::TilingPropertyModified()
    {
        GenerateRenderNodes();
    }

    void River::SetRenderProperties()
    {
        for (const auto& node : m_renderNodes)
        {
            auto renderingNode = node.GetRenderNode();
            renderingNode->SetRndFlags(ERF_COMPONENT_ENTITY);
            renderingNode->SetViewDistanceMultiplier(m_viewDistanceMultiplier);
            renderingNode->SetMinSpec(static_cast<int>(m_minSpec));
            renderingNode->SetFogDensity(m_waterFogDensity);
            renderingNode->SetFogColor(AZColorToLYVec3(m_fogColor));
            renderingNode->SetFogColorAffectedBySun(m_fogColorAffectedBySun);
            renderingNode->SetFogShadowing(m_waterFogShadowing);
            renderingNode->SetCapFogAtVolumeDepth(m_waterCapFogAtVolumeDepth);
            renderingNode->SetCaustics(m_waterCaustics);
            renderingNode->SetCausticIntensity(m_waterCausticIntensity);
            renderingNode->SetCausticTiling(m_waterCausticTiling);
            renderingNode->SetCausticHeight(m_waterCausticHeight);

            Utils::UpdateSpecs(renderingNode.get(), m_minSpec);
        }
    }

    void River::UpdateRenderNodeWithAssetMaterial()
    {
        _smart_ptr<IMaterial> material = nullptr;
        if (!m_material.GetAssetPath().empty())
        {
            material = gEnv->p3DEngine->GetMaterialManager()->LoadMaterial(m_material.GetAssetPath().c_str());
            if (!IsMaterialGoodForRiver(material))
            {
                return;
            }
        }

        for (const auto& node : m_renderNodes)
        {
            node.GetRenderNode()->SetMaterial(material);
        }
    }

    bool NoDegenerateTriangles(const SplineGeometrySector& sector)
    {
        return sector.points[0] != sector.points[1] && 
            sector.points[0] != sector.points[2] &&
            sector.points[1] != sector.points[2] && 
            sector.points[2] != sector.points[3] && 
            sector.points[1] != sector.points[3];
    }

    void River::GenerateRenderNodes()
    {
        m_renderNodes.clear();

        auto& geometrySectors = GetGeometrySectors();
        if (geometrySectors.empty())
        {
            return;
        }

        AZ_Assert(m_entityId.IsValid(), "[River::GenerateRenderNodes()] Entity id is invalid");

        AZ::Transform transform = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(transform, m_entityId, &AZ::TransformBus::Events::GetWorldTM);

        m_fogPlane = AZ::Plane::CreateFromNormalAndPoint(transform.GetColumn(2).GetNormalized(), transform * geometrySectors[0].points[0]);

        for (const auto& sector : geometrySectors)
        {
            if (NoDegenerateTriangles(sector))
            {
                RiverRenderNode riverNode;
                riverNode.SetRenderNode(static_cast<IWaterVolumeRenderNode*>(gEnv->p3DEngine->CreateRenderNode(eERType_WaterVolume)));
                riverNode.GetRenderNode()->CreateRiver(static_cast<uint64>(m_entityId), sector.points, transform, sector.t0, sector.t1, AZ::Vector2(1.0f, m_tileWidth), m_fogPlane, true);
                riverNode.GetRenderNode()->m_hasToBeSerialised = false;

                m_renderNodes.push_back(riverNode);
            }
        }

        UpdateRenderNodeWithAssetMaterial();
        SetRenderProperties();
        SetPhysicalProperties();
    }

    void River::SetPhysicalProperties()
    {
        for (const auto& sector : m_renderNodes)
        {
            sector.GetRenderNode()->SetVolumeDepth(m_waterVolumeDepth);
            sector.GetRenderNode()->SetStreamSpeed(m_waterStreamSpeed);
        }

        BindWithPhysics();
    }

    void River::BindWithPhysics()
    {
        auto& geometrySectors = GetGeometrySectors();
        if (!m_renderNodes.empty())
        {
            // Remove any existing physics representation
            const auto& firstNode = m_renderNodes.front();
            firstNode.GetRenderNode()->Dephysicalize();

            // If we should be physicalized, build a new physics representation
            if (m_physicalize)
            {
                auto numRiverSegments = geometrySectors.size();
                AZStd::vector<AZ::Vector3> outline;
                outline.reserve(numRiverSegments * 2 + 2);

                for (const auto& riverSegment : geometrySectors)
                {
                    outline.push_back(riverSegment.points[1]);
                }

                outline.push_back(geometrySectors.back().points[3]);
                outline.push_back(geometrySectors.back().points[2]);

                for (int i = numRiverSegments - 1; i >= 0; --i)
                {
                    outline.push_back(geometrySectors[i].points[0]);
                }

                AZ::Transform transform = AZ::Transform::CreateIdentity();
                AZ::TransformBus::EventResult(transform, m_entityId, &AZ::TransformBus::Events::GetWorldTM);

                firstNode.GetRenderNode()->SetRiverPhysicsArea(outline, transform, true);
                firstNode.GetRenderNode()->Physicalize();
            }
        }
    }

    AZ_CLASS_ALLOCATOR_IMPL(River, AZ::SystemAllocator, 0);
}