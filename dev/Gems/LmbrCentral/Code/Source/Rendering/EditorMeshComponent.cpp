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

#include <IAISystem.h>

#include "EditorMeshComponent.h"
#include <AzFramework/Viewport/ViewportColors.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/std/string/string_view.h>

#include <MathConversion.h>

#include <INavigationSystem.h> // For updating nav tiles on creation of editor physics.
#include <IPhysics.h> // For basic physicalization at edit-time for object snapping.
#include <IEditor.h>
#include <Settings.h>

namespace LmbrCentral
{
    void EditorMeshComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorMeshComponent, EditorComponentBase>()
                ->Version(1)
                ->Field("Static Mesh Render Node", &EditorMeshComponent::m_mesh)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorMeshComponent>("Mesh", "The Mesh component is the primary method of adding visual geometry to entities")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Rendering")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/StaticMesh.png")
                        ->Attribute(AZ::Edit::Attributes::PrimaryAssetType, AZ::AzTypeInfo<LmbrCentral::MeshAsset>::Uuid())
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/StaticMesh.png")
                        ->Attribute(AZ::Edit::Attributes::DynamicIconOverride, &EditorMeshComponent::GetMeshViewportIconPath)
                        ->Attribute(AZ::Edit::Attributes::PreferNoViewportIcon, true)
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://docs.aws.amazon.com/lumberyard/latest/userguide/component-static-mesh.html")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorMeshComponent::m_mesh)
                    ;

                editContext->Class<MeshComponentRenderNode::MeshRenderOptions>(
                    "Render Options", "Rendering options for the mesh.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ_CRC("PropertyVisibility_ShowChildrenOnly", 0xef428f20))

                    ->ClassElement(AZ::Edit::ClassElements::Group, "Options")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, false)

                    ->DataElement(AZ::Edit::UIHandlers::Slider, &MeshComponentRenderNode::MeshRenderOptions::m_opacity, "Opacity", "Opacity value")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.f)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.1f)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &MeshComponentRenderNode::MeshRenderOptions::OnMinorChanged)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &MeshComponentRenderNode::MeshRenderOptions::m_maxViewDist, "Max view distance", "Maximum view distance in meters.")
                        ->Attribute(AZ::Edit::Attributes::Suffix, " m")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                        ->Attribute(AZ::Edit::Attributes::Max, &MeshComponentRenderNode::GetDefaultMaxViewDist)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.1f)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &MeshComponentRenderNode::MeshRenderOptions::OnMinorChanged)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &MeshComponentRenderNode::MeshRenderOptions::m_viewDistMultiplier, "View distance multiplier", "Adjusts max view distance. If 1.0 then default is used. 1.1 would be 10% further than default.")
                        ->Attribute(AZ::Edit::Attributes::Suffix, "x")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &MeshComponentRenderNode::MeshRenderOptions::OnMinorChanged)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &MeshComponentRenderNode::MeshRenderOptions::m_lodRatio, "LOD distance ratio", "Controls LOD ratio over distance.")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &MeshComponentRenderNode::MeshRenderOptions::OnMinorChanged)
                        ->Attribute(AZ::Edit::Attributes::Min, 0)
                        ->Attribute(AZ::Edit::Attributes::Max, 255)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &MeshComponentRenderNode::MeshRenderOptions::m_castShadows, "Cast shadows", "Casts shadows.")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &MeshComponentRenderNode::MeshRenderOptions::OnMinorChanged)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &MeshComponentRenderNode::MeshRenderOptions::m_useVisAreas, "Use VisAreas", "Allow VisAreas to control this component's visibility.")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &MeshComponentRenderNode::MeshRenderOptions::OnMinorChanged)

                    ->ClassElement(AZ::Edit::ClassElements::Group, "Advanced")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, false)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &MeshComponentRenderNode::MeshRenderOptions::m_rainOccluder, "Rain occluder", "Occludes dynamic raindrops.")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &MeshComponentRenderNode::MeshRenderOptions::OnMinorChanged)
                        ->Attribute(AZ::Edit::Attributes::Visibility, &MeshComponentRenderNode::MeshRenderOptions::StaticPropertyVisibility)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &MeshComponentRenderNode::MeshRenderOptions::m_affectDynamicWater, "Affect dynamic water", "Will generate ripples in dynamic water.")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &MeshComponentRenderNode::MeshRenderOptions::OnMinorChanged)
                        ->Attribute(AZ::Edit::Attributes::Visibility, &MeshComponentRenderNode::MeshRenderOptions::StaticPropertyVisibility)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &MeshComponentRenderNode::MeshRenderOptions::m_receiveWind, "Receive wind", "Receives wind.")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &MeshComponentRenderNode::MeshRenderOptions::OnMajorChanged)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &MeshComponentRenderNode::MeshRenderOptions::m_acceptDecals, "Accept decals", "Can receive decals.")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &MeshComponentRenderNode::MeshRenderOptions::OnMinorChanged)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &MeshComponentRenderNode::MeshRenderOptions::m_affectNavmesh, "Affect navmesh", "Will affect navmesh generation.")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &MeshComponentRenderNode::MeshRenderOptions::OnMinorChanged)
                        ->Attribute(AZ::Edit::Attributes::Visibility, &MeshComponentRenderNode::MeshRenderOptions::StaticPropertyVisibility)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &MeshComponentRenderNode::MeshRenderOptions::m_visibilityOccluder, "Visibility occluder", "Is appropriate for occluding visibility of other objects.")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &MeshComponentRenderNode::MeshRenderOptions::OnMinorChanged)
                        ->Attribute(AZ::Edit::Attributes::Visibility, &MeshComponentRenderNode::MeshRenderOptions::StaticPropertyVisibility)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &MeshComponentRenderNode::MeshRenderOptions::m_dynamicMesh, "Deformable mesh", "Enables vertex deformation on mesh.")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &MeshComponentRenderNode::MeshRenderOptions::OnMajorChanged)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &MeshComponentRenderNode::MeshRenderOptions::m_affectGI, "Affects GI", "Affects the global illumination results.")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &MeshComponentRenderNode::MeshRenderOptions::OnMinorChanged)
                        ->Attribute(AZ::Edit::Attributes::Visibility, &MeshComponentRenderNode::MeshRenderOptions::StaticPropertyVisibility)
                        ;

                editContext->Class<MeshComponentRenderNode>(
                    "Mesh Rendering", "Attach geometry to the entity.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ_CRC("PropertyVisibility_ShowChildrenOnly", 0xef428f20))
                    ->DataElement(AZ::Edit::UIHandlers::Default, &MeshComponentRenderNode::m_visible, "Visible", "Is currently visible.")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &MeshComponentRenderNode::RefreshRenderState)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &MeshComponentRenderNode::m_meshAsset, "Mesh asset", "Mesh asset reference")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &MeshComponentRenderNode::OnAssetPropertyChanged)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &MeshComponentRenderNode::m_material, "Material override", "Optionally specify an override material.")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &MeshComponentRenderNode::OnAssetPropertyChanged)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &MeshComponentRenderNode::m_renderOptions, "Render options", "Render/draw options.")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &MeshComponentRenderNode::RefreshRenderState)
                        ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<EditorMeshComponent>()->RequestBus("MeshComponentRequestBus");
        }
    }

    void EditorMeshComponent::Activate()
    {
        EditorComponentBase::Activate();

        m_mesh.AttachToEntity(m_entity->GetId());
        bool isStatic = false;
        AZ::TransformBus::EventResult(isStatic, m_entity->GetId(), &AZ::TransformBus::Events::IsStaticTransform);
        m_mesh.SetTransformStaticState(isStatic);

        bool currentVisibility = true;
        AzToolsFramework::EditorVisibilityRequestBus::EventResult(
            currentVisibility, GetEntityId(), &AzToolsFramework::EditorVisibilityRequests::GetCurrentVisibility);
        m_mesh.UpdateAuxiliaryRenderFlags(!currentVisibility, ERF_HIDDEN);

        // Note we are purposely connecting to buses before calling m_mesh.CreateMesh().
        // m_mesh.CreateMesh() can result in events (eg: OnMeshCreated) that we want receive.
        MaterialOwnerRequestBus::Handler::BusConnect(m_entity->GetId());
        MeshComponentRequestBus::Handler::BusConnect(m_entity->GetId());
        MeshComponentNotificationBus::Handler::BusConnect(m_entity->GetId());
        LegacyMeshComponentRequestBus::Handler::BusConnect(m_entity->GetId());
        RenderNodeRequestBus::Handler::BusConnect(m_entity->GetId());
        AZ::TransformNotificationBus::Handler::BusConnect(m_entity->GetId());
        AzToolsFramework::EditorVisibilityNotificationBus::Handler::BusConnect(GetEntityId());
        AzFramework::EntityDebugDisplayEventBus::Handler::BusConnect(GetEntityId());
        CryPhysicsComponentRequestBus::Handler::BusConnect(GetEntityId());
        AzToolsFramework::EditorComponentSelectionRequestsBus::Handler::BusConnect(GetEntityId());
        AzToolsFramework::EditorComponentSelectionNotificationsBus::Handler::BusConnect(GetEntityId());

        m_mesh.m_renderOptions.m_changeCallback =
            [this]()
            {
                m_mesh.RefreshRenderState();
                AffectNavmesh();
            };

        m_mesh.CreateMesh();
    }

    void EditorMeshComponent::Deactivate()
    {
        CryPhysicsComponentRequestBus::Handler::BusDisconnect();
        MaterialOwnerRequestBus::Handler::BusDisconnect();
        MeshComponentRequestBus::Handler::BusDisconnect();
        MeshComponentNotificationBus::Handler::BusDisconnect();
        LegacyMeshComponentRequestBus::Handler::BusDisconnect();
        RenderNodeRequestBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::Handler::BusDisconnect();
        AzToolsFramework::EditorVisibilityNotificationBus::Handler::BusDisconnect();
        AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect();
        AzToolsFramework::EditorComponentSelectionRequestsBus::Handler::BusDisconnect();
        AzToolsFramework::EditorComponentSelectionNotificationsBus::Handler::BusDisconnect();

        DestroyEditorPhysics();

        m_mesh.m_renderOptions.m_changeCallback = nullptr;

        m_mesh.DestroyMesh();
        m_mesh.AttachToEntity(AZ::EntityId());

        EditorComponentBase::Deactivate();
    }

    IPhysicalEntity* EditorMeshComponent::GetPhysicalEntity()
    {
        return m_physicalEntity;
    }

    void EditorMeshComponent::GetPhysicsParameters(pe_params& outParameters)
    {
        if (m_physicalEntity)
        {
            m_physicalEntity->GetParams(&outParameters);
        }
    }

    void EditorMeshComponent::SetPhysicsParameters(const pe_params& parameters)
    {
        if (m_physicalEntity)
        {
            m_physicalEntity->SetParams(&parameters);
        }
    }

    void EditorMeshComponent::GetPhysicsStatus(pe_status& outStatus)
    {
        if (m_physicalEntity)
        {
            m_physicalEntity->GetStatus(&outStatus);
        }
    }

    void EditorMeshComponent::ApplyPhysicsAction(const pe_action& action, bool threadSafe)
    {
        if (m_physicalEntity)
        {
            m_physicalEntity->Action(&action, threadSafe);
        }
    }

    void EditorMeshComponent::OnMeshCreated(const AZ::Data::Asset<AZ::Data::AssetData>& asset)
    {
        AZ::Data::AssetBus::Handler::BusConnect(asset.GetId());

        CreateEditorPhysics();

        if (m_physicalEntity)
        {
            OnTransformChanged(GetTransform()->GetLocalTM(), GetTransform()->GetWorldTM());
        }
    }

    void EditorMeshComponent::OnMeshDestroyed()
    {
        AZ::Data::AssetBus::Handler::BusDisconnect();
        DestroyEditorPhysics();
    }

    IRenderNode* EditorMeshComponent::GetRenderNode()
    {
        return &m_mesh;
    }

    float EditorMeshComponent::GetRenderNodeRequestBusOrder() const
    {
        return s_renderNodeRequestBusOrder;
    }

    void EditorMeshComponent::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& world)
    {
        if (m_physicalEntity)
        {
            const AZ::Vector3 scale = world.RetrieveScale();

            if (!m_physScale.IsClose(scale))
            {
                // Scale changes require re-physicalizing.
                DestroyEditorPhysics();
                CreateEditorPhysics();
            }

            Matrix34 transform = AZTransformToLYTransform(world);

            pe_params_pos par_pos;
            par_pos.pMtx3x4 = &transform;
            m_physicalEntity->SetParams(&par_pos);
        }
    }

    void EditorMeshComponent::OnStaticChanged(bool isStatic)
    {
        m_mesh.SetTransformStaticState(isStatic);
        if (m_mesh.m_renderOptions.m_changeCallback)
        {
            m_mesh.m_renderOptions.m_changeCallback();
        }
        AffectNavmesh();
    }

    AZ::Aabb EditorMeshComponent::GetWorldBounds()
    {
        return m_mesh.CalculateWorldAABB();
    }

    AZ::Aabb EditorMeshComponent::GetLocalBounds()
    {
        return m_mesh.CalculateLocalAABB();
    }

    void EditorMeshComponent::SetMeshAsset(const AZ::Data::AssetId& id)
    {
        m_mesh.SetMeshAsset(id);
        AzToolsFramework::ToolsApplicationRequests::Bus::Broadcast(
            &AzToolsFramework::ToolsApplicationRequests::AddDirtyEntity,
            GetEntityId());
    }

    void EditorMeshComponent::SetMaterial(_smart_ptr<IMaterial> material)
    {
        m_mesh.SetMaterial(material);

        AzToolsFramework::ToolsApplicationEvents::Bus::Broadcast(
            &AzToolsFramework::ToolsApplicationEvents::InvalidatePropertyDisplay,
            AzToolsFramework::Refresh_AttributesAndValues);
    }

    _smart_ptr<IMaterial> EditorMeshComponent::GetMaterial()
    {
        return m_mesh.GetMaterial();
    }

    void EditorMeshComponent::SetPrimaryAsset(const AZ::Data::AssetId& assetId)
    {
        SetMeshAsset(assetId);
    }

    void EditorMeshComponent::OnEntityVisibilityChanged(bool visibility)
    {
        m_mesh.UpdateAuxiliaryRenderFlags(!visibility, ERF_HIDDEN);
        m_mesh.RefreshRenderState();
    }

    static void DecideColor(
        const bool selected, const bool mouseHovered, const bool visible,
        ColorB& triangleColor, ColorB& lineColor)
    {
        const ColorB translucentPurple = ColorB(250, 0, 250, 30);

        // default both colors to hidden
        triangleColor = ColorB(AZ::u32(0));
        lineColor = ColorB(AZ::u32(0));

        if (selected)
        {
            if (!visible)
            {
                lineColor = Col_Black;

                if (mouseHovered)
                {
                    triangleColor = translucentPurple;
                }
            }
        }
        else
        {
            if (mouseHovered)
            {
                triangleColor = translucentPurple;
                lineColor = AZColorToLYColorF(AzFramework::ViewportColors::HoverColor);
            }
        }
    }

    void EditorMeshComponent::DisplayEntity(bool& handled)
    {
        const bool mouseHovered = m_accentType == AzToolsFramework::EntityAccentType::Hover;

        IEditor* editor = nullptr;
        AzToolsFramework::EditorRequests::Bus::BroadcastResult(editor, &AzToolsFramework::EditorRequests::GetEditor);

        const bool highlightGeometryOnMouseHover = editor->GetEditorSettings()->viewports.bHighlightMouseOverGeometry;
        // if the mesh component is not visible, when selected we still draw the wireframe to indicate the shapes extent and position
        const bool highlightGeometryWhenSelected = editor->GetEditorSettings()->viewports.bHighlightSelectedGeometry || !GetVisibility();

        if ((!IsSelected() && mouseHovered && highlightGeometryOnMouseHover) || (IsSelected() && highlightGeometryWhenSelected))
        {
            AZ::Transform transform = AZ::Transform::CreateIdentity();
            AZ::TransformBus::EventResult(transform, GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);

            ColorB triangleColor, lineColor;
            DecideColor(IsSelected(), mouseHovered, GetVisibility(), triangleColor, lineColor);

            SGeometryDebugDrawInfo dd;
            dd.tm = AZTransformToLYTransform(transform);
            dd.bExtrude = true;
            dd.color = triangleColor;
            dd.lineColor = lineColor;
            
            if (IStatObj* geometry = GetStatObj())
            {
                geometry->DebugDraw(dd);
            }
        }

        if (m_mesh.HasMesh())
        {
            // Only allow Sandbox to draw the default sphere if we don't have a
            // visible mesh.
            handled = true;
        }
    }

    void EditorMeshComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        if (auto meshComponent = gameEntity->CreateComponent<MeshComponent>())
        {
            m_mesh.CopyPropertiesTo(meshComponent->m_meshRenderNode);
            // ensure we do not copy across the edit time entity id
            meshComponent->m_meshRenderNode.m_renderOptions.m_attachedToEntityId = AZ::EntityId();
        }
    }

    void EditorMeshComponent::CreateEditorPhysics()
    {
        DestroyEditorPhysics();

        AZ::TransformInterface* transformInterface = GetTransform();
        if (transformInterface == nullptr)
        {
            return;
        }

        IStatObj* geometry = m_mesh.GetEntityStatObj();
        if (!geometry)
        {
            return;
        }

        if (gEnv->pPhysicalWorld)
        {
            m_physicalEntity = gEnv->pPhysicalWorld->CreatePhysicalEntity(PE_STATIC, nullptr, &m_mesh, PHYS_FOREIGN_ID_STATIC);
            m_physicalEntity->AddRef();

            pe_geomparams params;
            geometry->Physicalize(m_physicalEntity, &params);

            // Immediately set transform, otherwise physics doesn't propgate the world change.
            const AZ::Transform& transform = transformInterface->GetWorldTM();
            Matrix34 cryTransform = AZTransformToLYTransform(transform);
            pe_params_pos par_pos;
            par_pos.pMtx3x4 = &cryTransform;
            m_physicalEntity->SetParams(&par_pos);

            // Store scale at point of physicalization so we know when to re-physicalize.
            // CryPhysics doesn't support dynamic scale changes.
            m_physScale = transform.RetrieveScale();

            AffectNavmesh();
        }
    }

    void EditorMeshComponent::DestroyEditorPhysics()
    {
        // If physics is completely torn down, all physical entities are by extension completely invalid (dangling pointers).
        // It doesn't matter that we held a reference.
        if (gEnv->pPhysicalWorld)
        {
            if (m_physicalEntity)
            {
                m_physicalEntity->Release();
                gEnv->pPhysicalWorld->DestroyPhysicalEntity(m_physicalEntity);
            }
        }

        m_physicalEntity = nullptr;
    }

    IStatObj* EditorMeshComponent::GetStatObj()
    {
        return m_mesh.GetEntityStatObj();
    }

    bool EditorMeshComponent::GetVisibility()
    {
        return m_mesh.GetVisible();
    }

    void EditorMeshComponent::SetVisibility(bool visible)
    {
        m_mesh.SetVisible(visible);
    }

    void EditorMeshComponent::AffectNavmesh()
    {
        if ( m_physicalEntity )
        {
            pe_params_foreign_data foreignData;
            m_physicalEntity->GetParams(&foreignData);

            if (m_mesh.m_renderOptions.m_affectNavmesh && m_mesh.m_renderOptions.IsStatic())
            {
                foreignData.iForeignFlags &= ~PFF_EXCLUDE_FROM_STATIC;
            }
            else
            {
                foreignData.iForeignFlags |= PFF_EXCLUDE_FROM_STATIC;
            }
            m_physicalEntity->SetParams(&foreignData);

            // Refresh the nav tile when the flag changes.
            INavigationSystem* pNavigationSystem = gEnv->pAISystem ? gEnv->pAISystem->GetNavigationSystem() : nullptr;
            if (pNavigationSystem)
            {
                pNavigationSystem->WorldChanged(AZAabbToLyAABB(GetWorldBounds()));
            }
        }
    }
    AZStd::string_view staticViewportIcon = "Editor/Icons/Components/Viewport/StaticMesh.png";
    AZStd::string_view dynamicViewportIcon = "Editor/Icons/Components/Viewport/DynamicMesh.png";
    AZStd::string EditorMeshComponent::GetMeshViewportIconPath() const
    {
        if (m_mesh.m_renderOptions.IsStatic())
        {
            return staticViewportIcon;
        }
        
        return dynamicViewportIcon;
    }

    void EditorMeshComponent::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> /*asset*/)
    {
        CreateEditorPhysics();
    }

    AZ::Aabb EditorMeshComponent::GetEditorSelectionBounds()
    {
        return GetWorldBounds();
    }

    bool EditorMeshComponent::EditorSelectionIntersectRay(
        const AZ::Vector3& src, const AZ::Vector3& dir, AZ::VectorFloat& distance)
    {
        if (IStatObj* geometry = GetStatObj())
        {
            AZ::Transform transform = AZ::Transform::CreateIdentity();
            AZ::TransformBus::EventResult(transform, GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);
            auto legacyTransform = AZTransformToLYTransform(transform);
            const auto legacySrc = AZVec3ToLYVec3(src);
            const auto legacyDir = AZVec3ToLYVec3(dir);

            const Matrix34 inverseTM = legacyTransform.GetInverted();
            const Vec3 raySrcLocal = inverseTM.TransformPoint(legacySrc);
            const Vec3 rayDirLocal = inverseTM.TransformVector(legacyDir).GetNormalized();

            SRayHitInfo hi;
            hi.inReferencePoint = raySrcLocal;
            hi.inRay = Ray(raySrcLocal, rayDirLocal);
            if (geometry->RayIntersection(hi))
            {
                const Vec3 worldHitPos = legacyTransform.TransformPoint(hi.vHitPos);
                distance = legacySrc.GetDistance(worldHitPos);
                return true;
            }
        }

        return false;
    }

    void EditorMeshComponent::OnAccentTypeChanged(AzToolsFramework::EntityAccentType accent)
    {
        m_accentType = accent;
    }
} // namespace LmbrCentral
