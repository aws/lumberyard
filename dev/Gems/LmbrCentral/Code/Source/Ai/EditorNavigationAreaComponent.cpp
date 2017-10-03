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
#include "EditorNavigationAreaComponent.h"

#include <AzCore/Component/TransformBus.h>
#include <AzCore/Math/VectorConversions.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <LmbrCentral/Shape/PolygonPrismShapeComponentBus.h>

#include <IAISystem.h>
#include <MathConversion.h>

namespace LmbrCentral
{
    static AZ_FORCE_INLINE bool NavAgentValid(NavigationAgentTypeID navAgentId)
    {
        return navAgentId != NavigationAgentTypeID();
    }
    
    static AZ_FORCE_INLINE bool NavVolumeValid(NavigationVolumeID navVolumeId)
    {
        return navVolumeId != NavigationVolumeID();
    }

    static AZ_FORCE_INLINE bool NavMeshValid(NavigationMeshID navMeshId)
    {
        return navMeshId != NavigationMeshID();
    }

    /*static*/ void EditorNavigationAreaComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorNavigationAreaComponent>()
                ->Field("AgentTypes", &EditorNavigationAreaComponent::m_agentTypes)
                ->Field("Exclusion", &EditorNavigationAreaComponent::m_exclusion);

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorNavigationAreaComponent>("Navigation Area", "Navigation Area configuration")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "AI")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/Navigation.png")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/Navigation.png")
                        //->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c)) Disabled for v1.11
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::CheckBox, &EditorNavigationAreaComponent::m_exclusion, "Exclusion", "Does this area add or subtract from the Navigation Mesh")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorNavigationAreaComponent::OnNavigationAreaChanged)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorNavigationAreaComponent::m_agentTypes, "Agent Types", "All agents that could potentially be used with this area")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::AddNotify, &EditorNavigationAreaComponent::OnNavigationAreaChanged)
                        ->Attribute(AZ::Edit::Attributes::RemoveNotify, &EditorNavigationAreaComponent::OnNavigationAreaChanged)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorNavigationAreaComponent::OnNavigationAreaChanged);
            }
        }
    }

    EditorNavigationAreaComponent::EditorNavigationAreaComponent()
        : m_navigationAreaChanged([this]() { UpdateMeshes(); ApplyExclusion(); })
    {
    }

    EditorNavigationAreaComponent::~EditorNavigationAreaComponent()
    {
        DestroyArea();
    }

    void EditorNavigationAreaComponent::Activate()
    {
        EditorComponentBase::Activate();

        AZ::EntityId entityId = GetEntityId();
        AZ::TransformNotificationBus::Handler::BusConnect(entityId);
        ShapeComponentNotificationsBus::Handler::BusConnect(entityId);
        NavigationAreaRequestBus::Handler::BusConnect(entityId);

        // use the entity id as unique name to register area
        m_name = GetEntityId().ToString();

        if (IAISystem* aiSystem = gEnv->pAISystem)
        {
            if (INavigationSystem* aiNavigation = aiSystem->GetNavigationSystem())
            {
                aiNavigation->RegisterArea(m_name.c_str());
            }
        }

        // update the area to refresh the nav mesh
        UpdateGameArea();
    }

    void EditorNavigationAreaComponent::Deactivate()
    {
        DestroyArea();
        
        AZ::EntityId entityId = GetEntityId();
        AZ::TransformNotificationBus::Handler::BusDisconnect(entityId);
        ShapeComponentNotificationsBus::Handler::BusDisconnect(entityId);
        NavigationAreaRequestBus::Handler::BusDisconnect(entityId);

        EditorComponentBase::Deactivate();
    }

    void EditorNavigationAreaComponent::OnNavigationEvent(const INavigationSystem::ENavigationEvent event)
    {
        switch (event)
        {
        case INavigationSystem::MeshReloaded:
        case INavigationSystem::NavigationCleared:
            RelinkWithMesh(true);
            break;
        case INavigationSystem::MeshReloadedAfterExporting:
            RelinkWithMesh(false);
            break;
        default:
            AZ_Assert(false, "Unhandled ENavigationEvent: %d", event);
            break;
        }
    }

    void EditorNavigationAreaComponent::OnShapeChanged(ShapeChangeReasons /*changeReason*/)
    {
        UpdateGameArea();
    }

    void EditorNavigationAreaComponent::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& /*world*/)
    {
        UpdateGameArea();
    }

    void EditorNavigationAreaComponent::RefreshArea()
    {
        UpdateGameArea();
    }

    void EditorNavigationAreaComponent::UpdateGameArea()
    {
        using namespace AZ::PolygonPrismUtil;

        AZ::Transform transform = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(transform, GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);

        AZ::ConstPolygonPrismPtr polygonPrismPtr;
        PolygonPrismShapeComponentRequestsBus::EventResult(polygonPrismPtr, GetEntityId(), &PolygonPrismShapeComponentRequests::GetPolygonPrism);
        
        const AZ::PolygonPrism& polygonPrism = *polygonPrismPtr;
        if (IAISystem* aiSystem = gEnv->pAISystem)
        {
            INavigationSystem* aiNavigation = aiSystem->GetNavigationSystem();

            const size_t vertexCount = polygonPrism.m_vertexContainer.Size();
            const AZStd::vector<AZ::Vector2>& verticesLocal = polygonPrism.m_vertexContainer.GetVertices();

            if (vertexCount > 2)
            {
                AZStd::vector<AZ::Vector3> verticesWorld;
                verticesWorld.reserve(vertexCount);

                for (size_t i = 0; i < vertexCount; ++i)
                {
                    verticesWorld.push_back(transform * AZ::Vector2ToVector3(verticesLocal[i]));
                }

                // The volume could be set but if the binary data didn't exist the volume was not correctly recreated
                if (!NavVolumeValid(NavigationVolumeID(m_volume)) || !aiNavigation->ValidateVolume(NavigationVolumeID(m_volume)))
                {
                    CreateVolume(&verticesWorld[0], verticesWorld.size(), NavigationVolumeID(m_volume));
                }
                else
                {
                    AZStd::vector<Vec3> cryVertices;
                    cryVertices.reserve(vertexCount);

                    for (size_t i = 0; i < vertexCount; ++i)
                    {
                        cryVertices.push_back(AZVec3ToLYVec3(verticesWorld[i]));
                    }

                    aiNavigation->SetVolume(NavigationVolumeID(m_volume), &cryVertices[0], cryVertices.size(), polygonPrism.GetHeight());
                }

                UpdateMeshes();
            }
            else if (NavVolumeValid(NavigationVolumeID(m_volume)))
            {
                DestroyVolume();
                UpdateMeshes();
            }

            ApplyExclusion();
        }
    }

    void EditorNavigationAreaComponent::UpdateMeshes()
    {
        if (IAISystem* aiSystem = gEnv->pAISystem)
        {
            if (m_exclusion)
            {
                DestroyMeshes();
            }
            else
            {
                const size_t agentTypeCount = m_agentTypes.size();
                m_meshes.resize(agentTypeCount);

                for (size_t i = 0; i < agentTypeCount; ++i)
                {
                    NavigationMeshID meshID = NavigationMeshID(m_meshes[i]);
                    NavigationAgentTypeID agentTypeID = aiSystem->GetNavigationSystem()->GetAgentTypeID(m_agentTypes[i].c_str());

                    if (NavAgentValid(agentTypeID) && !meshID)
                    {
                        INavigationSystem::CreateMeshParams params; // TODO: expose at least the tile size
                        meshID = aiSystem->GetNavigationSystem()->CreateMesh(m_name.c_str(), agentTypeID, params);
                        aiSystem->GetNavigationSystem()->SetMeshBoundaryVolume(meshID, NavigationVolumeID(m_volume));

                        AZ::Transform transform = AZ::Transform::CreateIdentity();
                        AZ::TransformBus::EventResult(transform, GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);

                        AZ::ConstPolygonPrismPtr polygonPrismPtr;
                        PolygonPrismShapeComponentRequestsBus::EventResult(polygonPrismPtr, GetEntityId(), &PolygonPrismShapeComponentRequests::GetPolygonPrism);
                        
                        const AZ::PolygonPrism& polygonPrism = *polygonPrismPtr;
                        aiSystem->GetNavigationSystem()->QueueMeshUpdate(meshID, AZAabbToLyAABB(AZ::PolygonPrismUtil::CalculateAabb(polygonPrism, transform)));

                        m_meshes[i] = meshID;
                    }
                    else if (!NavAgentValid(agentTypeID) && meshID)
                    {
                        aiSystem->GetNavigationSystem()->DestroyMesh(meshID);
                        m_meshes[i] = 0;
                    }
                }
            }
        }
    }

    void EditorNavigationAreaComponent::ApplyExclusion()
    {
        if (IAISystem* aiSystem = gEnv->pAISystem)
        {
            std::vector<NavigationAgentTypeID> affectedAgentTypes;

            if (m_exclusion)
            {
                const size_t agentTypeCount = m_agentTypes.size();
                affectedAgentTypes.reserve(agentTypeCount);

                for (size_t i = 0; i < agentTypeCount; ++i)
                {
                    NavigationAgentTypeID agentTypeID = aiSystem->GetNavigationSystem()->GetAgentTypeID(m_agentTypes[i].c_str());
                    affectedAgentTypes.push_back(agentTypeID);
                }
            }

            if (affectedAgentTypes.empty())
            {
                aiSystem->GetNavigationSystem()->SetExclusionVolume(0, 0, NavigationVolumeID(m_volume));
            }
            else
            {
                aiSystem->GetNavigationSystem()->SetExclusionVolume(&affectedAgentTypes[0], affectedAgentTypes.size(), NavigationVolumeID(m_volume));
            }
        }
    }

    void EditorNavigationAreaComponent::RelinkWithMesh(bool updateGameArea)
    {
        if (IAISystem* aiSystem = gEnv->pAISystem)
        {
            INavigationSystem* pAINavigation = aiSystem->GetNavigationSystem();
            m_volume = pAINavigation->GetAreaId(m_name.c_str());

            if (!m_exclusion)
            {
                const size_t agentTypeCount = m_agentTypes.size();
                m_meshes.resize(agentTypeCount);

                for (size_t i = 0; i < agentTypeCount; ++i)
                {
                    NavigationAgentTypeID agentTypeID = pAINavigation->GetAgentTypeID(m_agentTypes[i].c_str());
                    m_meshes[i] = pAINavigation->GetMeshID(m_name.c_str(), agentTypeID);
                }
            }

            /*
            FR: We need to update the game area even in the case that after having read
            the data from the binary file the volume was not recreated.
            This happens when there was no mesh associated to the actual volume.
            */
            if (updateGameArea || !pAINavigation->ValidateVolume(NavigationVolumeID(m_volume)))
            {
                UpdateGameArea();
            }
        }
    }

    void EditorNavigationAreaComponent::CreateVolume(AZ::Vector3* vertices, size_t vertexCount, NavigationVolumeID requestedID)
    {
        if (IAISystem* aiSystem = gEnv->pAISystem)
        {
            if (INavigationSystem* aiNavigation = aiSystem->GetNavigationSystem())
            {
                AZStd::vector<Vec3> cryVertices;
                cryVertices.reserve(vertexCount);

                for (size_t i = 0; i < vertexCount; ++i)
                {
                    cryVertices.push_back(AZVec3ToLYVec3(vertices[i]));
                }

                AZ::ConstPolygonPrismPtr polygonPrismPtr;
                PolygonPrismShapeComponentRequestsBus::EventResult(polygonPrismPtr, GetEntityId(), &PolygonPrismShapeComponentRequests::GetPolygonPrism);
                
                const AZ::PolygonPrism& polygonPrism = *polygonPrismPtr;
                m_volume = aiNavigation->CreateVolume(cryVertices.begin(), vertexCount, polygonPrism.GetHeight(), requestedID);
                aiNavigation->RegisterListener(this, m_name.c_str());

                if (!NavVolumeValid(requestedID))
                {
                    aiNavigation->SetAreaId(m_name.c_str(), NavigationVolumeID(m_volume));
                }
            }
        }
    }

    void EditorNavigationAreaComponent::DestroyVolume()
    {
        if (IAISystem* aiSystem = gEnv->pAISystem)
        {
            if (INavigationSystem* aiNavigation = aiSystem->GetNavigationSystem())
            {
                if (NavVolumeValid(NavigationVolumeID(m_volume)))
                {
                    aiNavigation->DestroyVolume(NavigationVolumeID(m_volume));
                    aiNavigation->UnRegisterListener(this);

                    m_volume = NavigationVolumeID();
                }
            }
        }
    }

    void EditorNavigationAreaComponent::DestroyMeshes()
    {
        if (IAISystem* aiSystem = gEnv->pAISystem)
        {
            INavigationSystem* aiNavigation = aiSystem->GetNavigationSystem();

            for (size_t i = 0; i < m_meshes.size(); ++i)
            {
                if (NavMeshValid(NavigationMeshID(m_meshes[i])))
                {
                    aiNavigation->DestroyMesh(NavigationMeshID(m_meshes[i]));
                }
            }

            m_meshes.clear();
        }
    }

    void EditorNavigationAreaComponent::DestroyArea()
    {
        if (IAISystem* aiSystem = gEnv->pAISystem)
        {
            if (INavigationSystem* aiNavigation = aiSystem->GetNavigationSystem())
            {
                aiNavigation->UnRegisterArea(m_name.c_str());
            }
        }

        DestroyMeshes();
        DestroyVolume();
    }

} // namespace LmbrCentral