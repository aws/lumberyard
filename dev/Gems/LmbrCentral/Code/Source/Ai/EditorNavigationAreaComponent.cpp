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

#include "LmbrCentral_precompiled.h"
#include "EditorNavigationAreaComponent.h"

#include "EditorNavigationUtil.h"
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Math/VectorConversions.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <LmbrCentral/Shape/PolygonPrismShapeComponentBus.h>
#include <Shape/PolygonPrismShape.h>
#include <IAISystem.h>
#include <MathConversion.h>

namespace LmbrCentral
{
    static bool NavAgentValid(NavigationAgentTypeID navAgentId)
    {
        return navAgentId != NavigationAgentTypeID();
    }

    static bool NavVolumeValid(NavigationVolumeID navVolumeId)
    {
        return navVolumeId != NavigationVolumeID();
    }

    static bool NavMeshValid(NavigationMeshID navMeshId)
    {
        return navMeshId != NavigationMeshID();
    }

    void EditorNavigationAreaComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorNavigationAreaComponent, AzToolsFramework::Components::EditorComponentBase>()
                ->Field("AgentTypes", &EditorNavigationAreaComponent::m_agentTypes)
                ->Field("Exclusion", &EditorNavigationAreaComponent::m_exclusion)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorNavigationAreaComponent>("Navigation Area", "Navigation Area configuration")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "AI")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/NavigationArea.png")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/NavigationArea.png")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                        ->Attribute(AZ::Edit::Attributes::HelpPageURL, "http://docs.aws.amazon.com/console/lumberyard/userguide/nav-area-component")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::CheckBox, &EditorNavigationAreaComponent::m_exclusion, "Exclusion", "Does this area add or subtract from the Navigation Mesh")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorNavigationAreaComponent::OnNavigationAreaChanged)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorNavigationAreaComponent::m_agentTypes, "Agent Types", "All agents that could potentially be used with this area")
                        ->ElementAttribute(AZ::Edit::UIHandlers::Handler, AZ::Edit::UIHandlers::ComboBox)
                        ->ElementAttribute(AZ::Edit::Attributes::StringList, &PopulateAgentTypeList)
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::AddNotify, &EditorNavigationAreaComponent::OnNavigationAreaChanged)
                        ->Attribute(AZ::Edit::Attributes::RemoveNotify, &EditorNavigationAreaComponent::OnNavigationAreaChanged)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorNavigationAreaComponent::OnNavigationAreaChanged)
                        ;
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

        const AZ::EntityId entityId = GetEntityId();
        AZ::TransformNotificationBus::Handler::BusConnect(entityId);
        ShapeComponentNotificationsBus::Handler::BusConnect(entityId);
        NavigationAreaRequestBus::Handler::BusConnect(entityId);
        AzToolsFramework::EditorEntityContextNotificationBus::Handler::BusConnect();
        AzToolsFramework::EntityCompositionNotificationBus::Handler::BusConnect();
        AZ::TickBus::Handler::BusConnect();

        // use the entity id as unique name to register area
        m_name = GetEntityId().ToString();

        if (IAISystem* aiSystem = gEnv->pAISystem)
        {
            if (INavigationSystem* aiNavigation = aiSystem->GetNavigationSystem())
            {
                // we only wish to register new areas (this area may have been
                // registered when the navmesh was loaded at level load)
                if (!aiNavigation->IsAreaPresent(m_name.c_str()))
                {
                    aiNavigation->RegisterArea(m_name.c_str());
                }
            }
        }

        // reset switching to game mode on activate
        m_switchingToGameMode = false;

        // We must relink during entity activation or the NavigationSystem will throw 
        // errors in SpawnJob. Don't force an unnecessary update of the game area.  
        // RelinkWithMesh will still update the game area if the volume hasn't been created.
        const bool updateGameArea = false;
        RelinkWithMesh(updateGameArea);
    }

    void EditorNavigationAreaComponent::Deactivate()
    {
        // only destroy the area if we know we're not currently switching to game mode
        // or changing our composition during scrubbing
        if (!m_switchingToGameMode && !m_compositionChanging)
        {
            DestroyArea();
        }

        const AZ::EntityId entityId = GetEntityId();
        AZ::TransformNotificationBus::Handler::BusDisconnect(entityId);
        ShapeComponentNotificationsBus::Handler::BusDisconnect(entityId);
        NavigationAreaRequestBus::Handler::BusDisconnect(entityId);
        AzToolsFramework::EditorEntityContextNotificationBus::Handler::BusDisconnect();
        AzToolsFramework::EntityCompositionNotificationBus::Handler::BusDisconnect();
        AZ::TickBus::Handler::BusDisconnect();

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
        using namespace PolygonPrismUtil;

        AZ::Transform transform = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(transform, GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);

        AZ::ConstPolygonPrismPtr polygonPrismPtr;
        PolygonPrismShapeComponentRequestBus::EventResult(polygonPrismPtr, GetEntityId(), &PolygonPrismShapeComponentRequests::GetPolygonPrism);

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
                ApplyExclusion();
            }
            else if (NavVolumeValid(NavigationVolumeID(m_volume)))
            {
                DestroyArea();
            }
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
                    NavigationMeshID meshId = NavigationMeshID(m_meshes[i]);
                    const NavigationAgentTypeID agentTypeId = aiSystem->GetNavigationSystem()->GetAgentTypeID(m_agentTypes[i].c_str());

                    if (NavAgentValid(agentTypeId) && !meshId)
                    {
                        INavigationSystem::CreateMeshParams params; // TODO: expose at least the tile size
                        meshId = aiSystem->GetNavigationSystem()->CreateMesh(m_name.c_str(), agentTypeId, params);
                        aiSystem->GetNavigationSystem()->SetMeshBoundaryVolume(meshId, NavigationVolumeID(m_volume));

                        AZ::Transform transform = AZ::Transform::CreateIdentity();
                        AZ::TransformBus::EventResult(transform, GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);

                        AZ::ConstPolygonPrismPtr polygonPrismPtr;
                        PolygonPrismShapeComponentRequestBus::EventResult(polygonPrismPtr, GetEntityId(), &PolygonPrismShapeComponentRequests::GetPolygonPrism);

                        const AZ::PolygonPrism& polygonPrism = *polygonPrismPtr;
                        aiSystem->GetNavigationSystem()->QueueMeshUpdate(meshId, AZAabbToLyAABB(PolygonPrismUtil::CalculateAabb(polygonPrism, transform)));

                        m_meshes[i] = meshId;
                    }
                    else if (!NavAgentValid(agentTypeId) && meshId)
                    {
                        aiSystem->GetNavigationSystem()->DestroyMesh(meshId);
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
                // this will remove this volume from all agent type and mesh exclusion containers
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
            INavigationSystem* aiNavigation = aiSystem->GetNavigationSystem();
            m_volume = aiNavigation->GetAreaId(m_name.c_str());

            if (!m_exclusion)
            {
                const size_t agentTypeCount = m_agentTypes.size();
                m_meshes.resize(agentTypeCount);

                for (size_t i = 0; i < agentTypeCount; ++i)
                {
                    const NavigationAgentTypeID agentTypeId = aiNavigation->GetAgentTypeID(m_agentTypes[i].c_str());
                    m_meshes[i] = aiNavigation->GetMeshID(m_name.c_str(), agentTypeId);
                }
            }

            // Update the game area if requested or in the case that the volume doesn't exist yet.
            // This can happen when a volume doesn't have an associated mesh which is always the  
            // case with exclusion volumes.
            if (updateGameArea || !aiNavigation->ValidateVolume(NavigationVolumeID(m_volume)))
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
                PolygonPrismShapeComponentRequestBus::EventResult(polygonPrismPtr, GetEntityId(), &PolygonPrismShapeComponentRequests::GetPolygonPrism);

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
        if (gEnv)
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
    }

    void EditorNavigationAreaComponent::OnStartPlayInEditorBegin()
    {
        m_switchingToGameMode = true;
    }

    void EditorNavigationAreaComponent::OnEntityCompositionChanging(const AzToolsFramework::EntityIdList& entityIds)
    {
        if (AZStd::find(entityIds.begin(), entityIds.end(), GetEntityId()) != entityIds.end())
        {
            m_compositionChanging = true;
        }
    }

    void EditorNavigationAreaComponent::OnEntityCompositionChanged(const AzToolsFramework::EntityIdList& entityIds)
    {
        if (AZStd::find(entityIds.begin(), entityIds.end(), GetEntityId()) != entityIds.end())
        {
            m_compositionChanging = false;
        }
    }

    void EditorNavigationAreaComponent::OnTick(float /*deltaTime*/, AZ::ScriptTimePoint /*time*/)
    {
        m_compositionChanging = false;

        // disconnect from the composition and tick bus because we no longer need to
        // be concerned with entity scrubbing causing our navigation area to get rebuilt
        AzToolsFramework::EntityCompositionNotificationBus::Handler::BusDisconnect();
        AZ::TickBus::Handler::BusDisconnect();
    }

} // namespace LmbrCentral