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
#include <IEntityRenderState.h>
#include <I3DEngine.h>

#include "Road.h"

namespace RoadsAndRivers
{    
    static const char* s_baseRoadMaterial = "Materials/Road/defaultRoad";

    Road::Road()
    {
        m_material.SetAssetPath(s_baseRoadMaterial);
    }

    Road& Road::operator=(const Road& rhs)
    {
        SplineGeometry::operator=(rhs);

        m_material = rhs.m_material;
        m_ignoreTerrainHoles = rhs.m_ignoreTerrainHoles;

        return *this;
    }
    
    bool Road::VersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
    {
        // conversion from version 1 to version 2:
        // splines created with v1 should set m_addOverlapBetweenSectors to true
        // new splines created with v2 and beyond will default m_addOverlapBetweenSectors to false
        if (classElement.GetVersion() <= 1)
        {
            classElement.AddElementWithData(context, "AddOverlapBetweenSectors", true);
        }

        return true;
    }

    void Road::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<Road, SplineGeometry>()
                ->Version(2, &VersionConverter)
                ->Field("Material", &Road::m_material)
                ->Field("IgnoreTerrainHoles", &Road::m_ignoreTerrainHoles)
                ->Field("AddOverlapBetweenSectors", &Road::m_addOverlapBetweenSectors);

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<Road>("Road", "Road data")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &Road::m_material, "Road material", "Specify the road material")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &Road::MaterialPropertyChanged)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &Road::m_ignoreTerrainHoles, "Ignore terrain holes", "")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &Road::IgnoreTerrainHolesModified)
                    ->ClassElement(AZ::Edit::ClassElements::Group, "Rendering")
                        ->DataElement(AZ::Edit::UIHandlers::CheckBox, &Road::m_addOverlapBetweenSectors, "Add Overlap Between Sectors", "Adds overlap between sectors when the spline geometry is split into multiple meshes. Can be used to cover small gaps between sectors, but typically results in other artifacts caused by the overlap, so disabled by default.")
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &Road::GeneralPropertyModified)
                    ;
            }
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<RoadRequestBus>("RoadRequestBus")->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::Preview)->
                Event("Rebuild", &RoadRequestBus::Events::Rebuild)->
                Event("SetIgnoreTerrainHoles", &RoadRequestBus::Events::SetIgnoreTerrainHoles)->
                Event("GetIgnoreTerrainHoles", &RoadRequestBus::Events::GetIgnoreTerrainHoles)->
                VirtualProperty("IgnoreTerrainHoles", "GetIgnoreTerrainHoles", "SetIgnoreTerrainHoles")
            ;
        }
    }

    void Road::Activate(AZ::EntityId entityId)
    {
        SplineGeometry::Activate(entityId);

        AZ::TransformNotificationBus::Handler::BusConnect(entityId);
        LmbrCentral::MaterialOwnerRequestBus::Handler::BusConnect(entityId);
        RoadRequestBus::Handler::BusConnect(entityId);

        Rebuild();
    }

    void Road::Deactivate()
    {
        SplineGeometry::Deactivate();

        AZ::TransformNotificationBus::Handler::BusDisconnect();
        LmbrCentral::MaterialOwnerRequestBus::Handler::BusDisconnect();
        RoadRequestBus::Handler::BusDisconnect();

        Clear();
    }

    void Road::SetMaterial(_smart_ptr<IMaterial> material)
    {
        if (material)
        {
            m_material.SetAssetPath(material->GetName());
            for (const auto& node : m_roadRenderNodes)
            {
                node.GetRenderNode()->SetMaterial(material);
            }
        }
    }

    _smart_ptr<IMaterial> Road::GetMaterial()
    {
        auto firstNodeIt = m_roadRenderNodes.begin();
        if (firstNodeIt != m_roadRenderNodes.end())
        {
            return firstNodeIt->GetRenderNode()->GetMaterial();
        }
        return nullptr;
    }

    void Road::SetMaterialHandle(const LmbrCentral::MaterialHandle& materialHandle)
    {
        SetMaterial(materialHandle.m_material);
    }

    LmbrCentral::MaterialHandle Road::GetMaterialHandle()
    {
        auto handle = LmbrCentral::MaterialHandle();
        handle.m_material = GetMaterial();
        return handle;
    }

    void Road::SetIgnoreTerrainHoles(bool ignoreTerrainHoles)
    {
        m_ignoreTerrainHoles = ignoreTerrainHoles;
    }

    bool Road::GetIgnoreTerrainHoles()
    {
        return m_ignoreTerrainHoles;
    }

    void Road::IgnoreTerrainHolesModified()
    {
        RenderingPropertyModified();
        RoadNotificationBus::Event(GetEntityId(), &RoadNotificationBus::Events::OnIgnoreTerrainHolesChanged, m_ignoreTerrainHoles);
    }

    void Road::Rebuild()
    {
        AZ_Assert(m_entityId.IsValid(), "[Road::Rebuild()] Entity id is invalid");

        BuildSplineMesh();
        GenerateRenderNodes();
    }

    void Road::Clear()
    {
        SplineGeometry::Clear();
        m_roadRenderNodes.clear();
    }

    void Road::GeneralPropertyModified()
    {
        Rebuild();
    }

    void Road::WidthPropertyModified()
    {
        Rebuild();
    }

    void Road::RenderingPropertyModified()
    {
        SetRenderProperties();
    }

    void Road::OnTransformChanged(const AZ::Transform & local, const AZ::Transform & world)
    {
        GenerateRenderNodes();
    }

    void Road::MaterialPropertyChanged()
    {
        UpdateRenderNodeWithAssetMaterial();
    }

    void Road::GenerateRenderNodes()
    {
        m_roadRenderNodes.clear();

        auto& geometrySectors = GetGeometrySectors();
        if (geometrySectors.empty())
        {
            return;
        }

        AZ_Assert(m_entityId.IsValid(), "[Road::GenerateRenderNodes()] Entity id is invalid");

        AZ::Transform transform = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(transform, m_entityId, &AZ::TransformBus::Events::GetWorldTM);

        AZ::ConstSplinePtr spline = nullptr;
        LmbrCentral::SplineComponentRequestBus::EventResult(spline, m_entityId, &LmbrCentral::SplineComponentRequests::GetSpline);

        AZ_Assert(spline != nullptr, "[Road::GenerateRenderNodes()] Spline must be attached for the road to work");
        if (!spline)
        {
            return;
        }

        const int MAX_TRAPEZOIDS_IN_CHUNK = 16;
        int chunksCount = geometrySectors.size() / MAX_TRAPEZOIDS_IN_CHUNK + 1;

        for (int chunkId = 0; chunkId < chunksCount; ++chunkId)
        {
            int startSecId = chunkId * MAX_TRAPEZOIDS_IN_CHUNK;
            int sectorsNum = min(MAX_TRAPEZOIDS_IN_CHUNK, static_cast<int>(geometrySectors.size()) - startSecId);

            if (sectorsNum == 0)
            {
                break;
            }

            auto& firstSector = geometrySectors[startSecId];

            RoadRenderNode newRenderNode;
            newRenderNode.SetRenderNode(static_cast<IRoadRenderNode*>(gEnv->p3DEngine->CreateRenderNode(eERType_Road)));
            newRenderNode.GetRenderNode()->m_hasToBeSerialised = false;

            AZStd::vector<AZ::Vector3> vertices;
            vertices.reserve(sectorsNum * 2 + 2);

            for (int i = 0; i < sectorsNum; ++i)
            {
                vertices.push_back(geometrySectors[startSecId + i].points[0]);
                vertices.push_back(geometrySectors[startSecId + i].points[1]);
            }

            auto& lastSector = geometrySectors[startSecId + sectorsNum - 1];            

            if (m_addOverlapBetweenSectors)
            {
                newRenderNode.GetRenderNode()->m_addOverlapBetweenSectors = true;

                // Extend final boundary to cover holes in roads caused by f16 meshes.
                // Overlapping the roads slightly seems to be the nicest way to fix the issue
                auto sectorLastOffset2 = lastSector.points[2] - lastSector.points[0];
                auto sectorLastOffset3 = lastSector.points[3] - lastSector.points[1];

                sectorLastOffset2.Normalize();
                sectorLastOffset3.Normalize();

                const float sectorLastOffset = 0.075f;
                sectorLastOffset2 *= sectorLastOffset;
                sectorLastOffset3 *= sectorLastOffset;
                vertices.push_back(lastSector.points[2] + sectorLastOffset2);
                vertices.push_back(lastSector.points[3] + sectorLastOffset3);
            }
            else 
            {
                newRenderNode.GetRenderNode()->m_addOverlapBetweenSectors = false;

                vertices.push_back(lastSector.points[2]);
                vertices.push_back(lastSector.points[3]);
            }

            AZ_Assert((vertices.size() % 2) == 0, "Road mesh generation failed; Wrong vertices number");

            newRenderNode.GetRenderNode()->SetVertices(vertices, transform,
                fabs(firstSector.t0), fabs(lastSector.t1), fabs(geometrySectors.front().t0), fabs(geometrySectors.back().t1));

            newRenderNode.GetRenderNode()->SetIgnoreTerrainHoles(false);
            newRenderNode.GetRenderNode()->SetPhysicalize(false);
            newRenderNode.GetRenderNode()->m_bAlphaBlendRoadEnds = !spline->IsClosed();

            m_roadRenderNodes.push_back(newRenderNode);
        }

        SetRenderProperties();
        UpdateRenderNodeWithAssetMaterial();
    }

    void Road::SetRenderProperties()
    {
        for(const auto& renderNode : m_roadRenderNodes)
        {
            renderNode.GetRenderNode()->SetRndFlags(ERF_COMPONENT_ENTITY);
            renderNode.GetRenderNode()->SetViewDistanceMultiplier(m_viewDistanceMultiplier); 
            renderNode.GetRenderNode()->SetMinSpec(static_cast<AZ::u32>(m_minSpec));
            renderNode.GetRenderNode()->SetSortPriority(m_sortPriority);
            renderNode.GetRenderNode()->SetIgnoreTerrainHoles(m_ignoreTerrainHoles);

            Utils::UpdateSpecs(renderNode.GetRenderNode().get(), m_minSpec);
        }
    }

    void Road::UpdateRenderNodeWithAssetMaterial()
    {
        _smart_ptr<IMaterial> material = nullptr;

        if (!m_material.GetAssetPath().empty())
        {
            material = gEnv->p3DEngine->GetMaterialManager()->LoadMaterial(m_material.GetAssetPath().c_str());
        }
        
        for (const auto& node : m_roadRenderNodes)
        {
            node.GetRenderNode()->SetMaterial(material);
        }
    }

    AZ_CLASS_ALLOCATOR_IMPL(Road, AZ::SystemAllocator, 0);
}
