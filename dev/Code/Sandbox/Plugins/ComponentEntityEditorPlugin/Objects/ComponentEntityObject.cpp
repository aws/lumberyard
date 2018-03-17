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

#include "ComponentEntityObject.h"
#include "IIconManager.h"

#include <AzCore/Component/Entity.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/std/string/string.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Slice/SliceComponent.h>

#include <AzFramework/Entity/EntityContextBus.h>

#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/ToolsComponents/EditorLockComponent.h>
#include <AzToolsFramework/ToolsComponents/EditorVisibilityComponent.h>
#include <AzToolsFramework/ToolsComponents/SelectionComponent.h>
#include <AzToolsFramework/ToolsComponents/TransformComponent.h>
#include <AzToolsFramework/ToolsComponents/GenericComponentWrapper.h>
#include <AzToolsFramework/ToolsComponents/EditorSelectionAccentSystemComponent.h>
#include <AzToolsFramework/ToolsComponents/EditorEntityIconComponentBus.h>

#include <LmbrCentral/Rendering/RenderNodeBus.h>
#include <LmbrCentral/Rendering/MeshComponentBus.h>
#include <LmbrCentral/Rendering/MaterialOwnerBus.h>

#include <MathConversion.h>

#include <TrackView/TrackViewSequenceManager.h>
#include <TrackView/TrackViewAnimNode.h>

#include <IDisplayViewport.h>
#include <Viewport.h>

#include <HyperGraph/FlowGraphHelpers.h>
#include <Material/MaterialManager.h>
#include <Objects/ObjectLayer.h>

#include <AzToolsFramework/Metrics/LyEditorMetricsBus.h>

/**
 * Scalars for icon drawing behavior.
 */
static const int s_kIconSize              = 36;       /// Icon display size (in pixels)
static const float s_kIconMaxWorldDist    = 200.f;    /// Icons are culled past this range
static const float s_kIconMinScale        = 0.1f;     /// Minimum scale for icons in the distance
static const float s_kIconMaxScale        = 1.0f;     /// Maximum scale for icons near the camera
static const float s_kIconCloseDist       = 3.f;      /// Distance at which icons are at maximum scale
static const float s_kIconFarDist         = 40.f;     /// Distance at which icons are at minimum scale

CComponentEntityObject::CComponentEntityObject()
    : m_hasIcon(false)
    , m_entityIconVisible(false)
    , m_iconOnlyHitTest(false)
    , m_drawAccents(true)
    , m_accentType(AzToolsFramework::EntityAccentType::None)
    , m_isIsolated(false)
    , m_iconTexture(nullptr)
{
}

CComponentEntityObject::~CComponentEntityObject()
{
    DeleteEntity();
}

bool CComponentEntityObject::Init(IEditor* ie, CBaseObject* copyFrom, const QString& file)
{
    SetColor(RGB(0, 255, 0));
    SetTextureIcon(GetClassDesc()->GetTextureIconId());

    // Sandbox does not serialize this object with others in the layer.
    SetFlags(OBJFLAG_DONT_SAVE);

    const bool result = CEntityObject::Init(ie, copyFrom, file);

    return result;
}

void CComponentEntityObject::AssignEntity(AZ::Entity* entity, bool destroyOld)
{
    const AZ::EntityId newEntityId = entity ? entity->GetId() : AZ::EntityId();

    if (m_entityId.IsValid())
    {
        AzToolsFramework::EntitySelectionEvents::Bus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::Handler::BusDisconnect();
        LmbrCentral::MeshComponentNotificationBus::Handler::BusDisconnect();
        AzToolsFramework::ComponentEntityEditorRequestBus::Handler::BusDisconnect();
        AZ::EntityBus::Handler::BusDisconnect();
        AzToolsFramework::ComponentEntityObjectRequestBus::Handler::BusDisconnect();
        AzToolsFramework::EditorLockComponentNotificationBus::Handler::BusDisconnect();
        AzToolsFramework::EditorVisibilityNotificationBus::Handler::BusDisconnect();
        AzToolsFramework::EditorEntityIconComponentNotificationBus::Handler::BusDisconnect();

        if (destroyOld && m_entityId != newEntityId)
        {
            // Delete Entity metrics event (Button Click "Delete Selected" in Object Selector)
            EBUS_EVENT(AzToolsFramework::EditorMetricsEventsBus, EntityDeleted, m_entityId);

            EBUS_EVENT(AzToolsFramework::EditorEntityContextRequestBus, DestroyEditorEntity, m_entityId);
        }

        m_entityId.SetInvalid();
    }

    if (entity)
    {
        m_entityId = entity->GetId();

        // Synchronize sandbox name to new entity's name.
        {
            EditorActionScope nameChange(m_nameReentryGuard);
            SetName(QString(entity->GetName().c_str()));
        }

        EBUS_EVENT(AzToolsFramework::EditorEntityContextRequestBus, AddRequiredComponents, *entity);

        // OBJFLAG_FROZEN should match EditorLockComponent's Locked state.
        bool locked = false;
        AzToolsFramework::EditorLockComponentRequestBus::EventResult(locked, entity->GetId(), &AzToolsFramework::EditorLockComponentRequests::GetLocked);
        if (locked)
        {
            SetFlags(OBJFLAG_FROZEN);
        }
        else
        {
            ClearFlags(OBJFLAG_FROZEN);
        }

        // OBJFLAG_HIDDEN should match EditorVisibilityComponent's VisibilityFlag.
        bool visibilityFlag;
        AzToolsFramework::EditorVisibilityRequestBus::EventResult(visibilityFlag, entity->GetId(), &AzToolsFramework::EditorVisibilityRequests::GetVisibilityFlag);
        if (visibilityFlag)
        {
            ClearFlags(OBJFLAG_HIDDEN);
        }
        else
        {
            SetFlags(OBJFLAG_HIDDEN);
        }

        AzToolsFramework::EntitySelectionEvents::Bus::Handler::BusConnect(m_entityId);
        AZ::TransformNotificationBus::Handler::BusConnect(m_entityId);
        LmbrCentral::MeshComponentNotificationBus::Handler::BusConnect(m_entityId);
        AzToolsFramework::ComponentEntityEditorRequestBus::Handler::BusConnect(m_entityId);
        AZ::EntityBus::Handler::BusConnect(m_entityId);
        AzToolsFramework::ComponentEntityObjectRequestBus::Handler::BusConnect(this);
        AzToolsFramework::EditorLockComponentNotificationBus::Handler::BusConnect(m_entityId);
        AzToolsFramework::EditorVisibilityNotificationBus::Handler::BusConnect(m_entityId);
        AzToolsFramework::EditorEntityIconComponentNotificationBus::Handler::BusConnect(m_entityId);

        // Synchronize transform to Sandbox.
        AzToolsFramework::Components::TransformComponent* transformComponent =
            entity->FindComponent<AzToolsFramework::Components::TransformComponent>();
        if (transformComponent)
        {
            const AZ::Transform& worldTransform = transformComponent->GetWorldTM();
            OnTransformChanged(transformComponent->GetLocalTM(), transformComponent->GetWorldTM());
        }
    }
}

void CComponentEntityObject::SetName(const QString& name)
{
    if (m_nameReentryGuard)
    {
        EditorActionScope nameChange(m_nameReentryGuard);

        AZ::Entity* entity = nullptr;
        EBUS_EVENT_RESULT(entity, AZ::ComponentApplicationBus, FindEntity, m_entityId);

        if (entity)
        {
            entity->SetName(name.toUtf8().data());
        }
    }

    CEntityObject::SetName(name);
}

void CComponentEntityObject::DeleteEntity()
{
    AssignEntity(nullptr);

    CEntityObject::DeleteEntity();
}

float CComponentEntityObject::GetRadius()
{
    static const float s_defaultRadius = 0.5f;
    return s_defaultRadius;
}

void CComponentEntityObject::SetSelected(bool bSelect)
{
    CEntityObject::SetSelected(bSelect);

    if (m_selectionReentryGuard)
    {
        // Ignore event when received from the tools app, since the action is originating in Sandbox.
        EditorActionScope selectionChange(m_selectionReentryGuard);

        // Pass the action to the tools application.
        if (bSelect)
        {
            EBUS_EVENT(AzToolsFramework::ToolsApplicationRequests::Bus, MarkEntitySelected, m_entityId);
        }
        else
        {
            EBUS_EVENT(AzToolsFramework::ToolsApplicationRequests::Bus, MarkEntityDeselected, m_entityId);
        }
    }
}

void CComponentEntityObject::SetHighlight(bool bHighlight)
{
    CEntityObject::SetHighlight(bHighlight);

    if (m_entityId.IsValid())
    {
        EBUS_EVENT(AzToolsFramework::ToolsApplicationRequests::Bus, SetEntityHighlighted, m_entityId, bHighlight);
    }
}

IRenderNode* CComponentEntityObject::GetEngineNode() const
{
    // It's possible for AZ::Entities to have multiple IRenderNodes.
    // However, the editor currently expects a single IRenderNode per "editor object".
    // Therefore, return the highest priority handler.
    if (auto* renderNodeHandler = LmbrCentral::RenderNodeRequestBus::FindFirstHandler(m_entityId))
    {
        return renderNodeHandler->GetRenderNode();
    }
    return nullptr;
}

void CComponentEntityObject::OnEntityNameChanged(const AZStd::string& name)
{
    if (m_nameReentryGuard)
    {
        EditorActionScope selectionChange(m_nameReentryGuard);

        SetName(QString(name.c_str()));

        UpdateEditParams(); // Ensures the legacy object panel reflects the name change.
    }
}

void CComponentEntityObject::OnSelected()
{
    if (m_selectionReentryGuard)
    {
        EditorActionScope selectionChange(m_selectionReentryGuard);

        // If we get here and we're not already selected in sandbox land it means
        // the selection started in AZ land and we need to clear any edit tool
        // the user may have selected from the rollup bar
        if (GetIEditor()->GetEditTool() && !IsSelected())
        {
            GetIEditor()->SetEditTool(0);
        }


        // Invoked when selected via tools application, so we notify sandbox.
        GetIEditor()->GetObjectManager()->SelectObject(this);
    }
}

void CComponentEntityObject::OnDeselected()
{
    if (m_selectionReentryGuard)
    {
        EditorActionScope selectionChange(m_selectionReentryGuard);

        // Invoked when selected via tools application, so we notify sandbox.
        GetIEditor()->GetObjectManager()->UnselectObject(this);
    }
}

void CComponentEntityObject::AttachChild(CBaseObject* child, bool /*bKeepPos*/)
{
    if (child->GetType() == OBJTYPE_AZENTITY)
    {
        CComponentEntityObject* childComponentEntity = static_cast<CComponentEntityObject*>(child);
        AZ::EntityId childEntityId = childComponentEntity->GetAssociatedEntityId();
        if (childEntityId.IsValid())
        {
            // The action is originating from Sandbox, so ignore the return event.
            EditorActionScope parentChange(childComponentEntity->m_parentingReentryGuard);

            {
                AzToolsFramework::ScopedUndoBatch undoBatch("Editor Parent");
                EBUS_EVENT_ID(childEntityId, AZ::TransformBus, SetParent, m_entityId);
                undoBatch.MarkEntityDirty(childEntityId);
            }

            EBUS_EVENT(AzToolsFramework::ToolsApplicationEvents::Bus, InvalidatePropertyDisplay, AzToolsFramework::Refresh_Values);
        }
    }
}

void CComponentEntityObject::DetachAll(bool /*bKeepPos*/)
{
}

void CComponentEntityObject::DetachThis(bool /*bKeepPos*/)
{
    if (m_parentingReentryGuard)
    {
        EditorActionScope parentChange(m_parentingReentryGuard);

        if (m_entityId.IsValid())
        {
            AzToolsFramework::ScopedUndoBatch undoBatch("Editor Unparent");
            EBUS_EVENT_ID(m_entityId, AZ::TransformBus, SetParent, AZ::EntityId());
            undoBatch.MarkEntityDirty(m_entityId);
        }

        EBUS_EVENT(AzToolsFramework::ToolsApplicationEvents::Bus, InvalidatePropertyDisplay, AzToolsFramework::Refresh_Values);
    }
}

CBaseObject* CComponentEntityObject::GetLinkParent() const
{
    AZ::EntityId parentId;
    EBUS_EVENT_ID_RESULT(parentId, m_entityId, AZ::TransformBus, GetParentId);

    return CComponentEntityObject::FindObjectForEntity(parentId);
}

bool CComponentEntityObject::IsFrozen() const
{
    return CheckFlags(OBJFLAG_FROZEN);
}

void CComponentEntityObject::SetFrozen(bool bFrozen)
{
    CEntityObject::SetFrozen(bFrozen);

    // EditorLockComponent's locked state should match OBJFLAG_FROZEN.
    if (m_lockedReentryGuard)
    {
        EditorActionScope flagChange(m_lockedReentryGuard);
        EBUS_EVENT_ID(m_entityId, AzToolsFramework::EditorLockComponentRequestBus, SetLocked, CheckFlags(OBJFLAG_FROZEN));
    }
}

void CComponentEntityObject::OnEntityLockChanged(bool locked)
{
    if (m_lockedReentryGuard)
    {
        EditorActionScope flagChange(m_lockedReentryGuard);
        SetFrozen(locked);
    }
}

void CComponentEntityObject::SetHidden(bool bHidden, uint64 hiddenId/*=CBaseObject::s_invalidHiddenID*/, bool bAnimated/*=false*/)
{
    CEntityObject::SetHidden(bHidden, hiddenId, bAnimated);

    // EditorVisibilityComponent's VisibilityFlag should match OBJFLAG_HIDDEN.
    if (m_visibilityFlagReentryGuard)
    {
        EditorActionScope flagChange(m_visibilityFlagReentryGuard);
        EBUS_EVENT_ID(m_entityId, AzToolsFramework::EditorVisibilityRequestBus, SetVisibilityFlag, !CheckFlags(OBJFLAG_HIDDEN));
    }
}

void CComponentEntityObject::OnEntityVisibilityFlagChanged(bool visible)
{
    if (m_visibilityFlagReentryGuard)
    {
        EditorActionScope flagChange(m_visibilityFlagReentryGuard);
        SetHidden(!visible);
    }
}

void CComponentEntityObject::OnEntityIconChanged(const AZ::Data::AssetId& entityIconAssetId)
{
    (void)entityIconAssetId;
    SetupEntityIcon();
}

void CComponentEntityObject::UpdateVisibility(bool bVisible)
{
    CEntityObject::UpdateVisibility(bVisible);

    AzToolsFramework::EditorVisibilityRequestBus::Event(m_entityId, &AzToolsFramework::EditorVisibilityRequests::SetCurrentVisibility, m_bVisible != 0);
}

void CComponentEntityObject::OnLayerChanged(CObjectLayer* layer)
{
    (void)layer;
}

void CComponentEntityObject::OnParentChanged(AZ::EntityId oldParent, AZ::EntityId newParent)
{
    return;

    if (m_parentingReentryGuard) // Ignore if action originated from Sandbox.
    {
        EditorActionScope parentChange(m_parentingReentryGuard);

        CComponentEntityObject* currentParent = static_cast<CComponentEntityObject*>(GetParent());

        if (!currentParent && !newParent.IsValid())
        {
            // No change in parent.
            return;
        }

        if (currentParent && currentParent->GetAssociatedEntityId() == newParent)
        {
            // No change in parent.
            return;
        }

        DetachThis();

        if (newParent.IsValid())
        {
            CComponentEntityObject* componentEntity = CComponentEntityObject::FindObjectForEntity(newParent);

            if (componentEntity)
            {
                // The action is originating from Sandbox, so ignore the return events.
                EditorActionScope transformChange(m_transformReentryGuard);

                componentEntity->AttachChild(this);
            }
        }

        InvalidateTM(0);
    }
}

void CComponentEntityObject::OnMeshCreated(const AZ::Data::Asset<AZ::Data::AssetData>& asset)
{
    (void)asset;

    // Need to recalculate bounds when the mesh changes.
    OnBoundsReset();
}

void CComponentEntityObject::OnBoundsReset()
{
    CalcBBox();
    CEntityObject::InvalidateTM(0);
}

void CComponentEntityObject::SetSandboxObjectAccent(AzToolsFramework::EntityAccentType accent)
{
    m_accentType = accent;
}

void CComponentEntityObject::SetSandBoxObjectIsolated(bool isIsolated)
{
    m_isIsolated = isIsolated;
    GetIEditor()->GetObjectManager()->InvalidateVisibleList();
}

bool CComponentEntityObject::IsSandBoxObjectIsolated()
{
    return m_isIsolated;
}

bool CComponentEntityObject::SetPos(const Vec3& pos, int flags)
{
    if ((flags & eObjectUpdateFlags_MoveTool) || (flags & eObjectUpdateFlags_UserInput))
    {
        // If we have a parent also in the selection set, don't allow the move tool to manipulate our position.
        if (IsAncestorSelected())
        {
            return false;
        }
    }

    return CEntityObject::SetPos(pos, flags);
}

bool CComponentEntityObject::SetRotation(const Quat& rotate, int flags)
{
    if (flags & eObjectUpdateFlags_UserInput)
    {
        // If we have a parent also in the selection set, don't allow the rotate tool to manipulate our position.
        if (IsAncestorSelected())
        {
            return false;
        }
    }

    return CEntityObject::SetRotation(rotate, flags);
}

bool CComponentEntityObject::SetScale(const Vec3& scale, int flags)
{
    if ((flags & eObjectUpdateFlags_ScaleTool) || (flags & eObjectUpdateFlags_UserInput))
    {
        // If we have a parent also in the selection set, don't allow the scale tool to manipulate our position.
        if (IsAncestorSelected())
        {
            return false;
        }
    }

    return CEntityObject::SetScale(scale, flags);
}

bool CComponentEntityObject::IsAncestorSelected() const
{
    AZ::EntityId parentId;
    AZ::TransformBus::EventResult(parentId, m_entityId, &AZ::TransformBus::Events::GetParentId);
    while (parentId.IsValid())
    {
        CComponentEntityObject* parentObject = CComponentEntityObject::FindObjectForEntity(parentId);
        if (parentObject && parentObject->IsSelected())
        {
            return true;
        }

        AZ::EntityId currentParentId = parentId;
        parentId.SetInvalid();
        AZ::TransformBus::EventResult(parentId, currentParentId, &AZ::TransformBus::Events::GetParentId);
    }

    return false;
}

bool CComponentEntityObject::IsAncestorIconDrawingAtSameLocation() const
{
    if (m_entityId.IsValid())
    {
        AZ::EntityId parentId;
        AZ::TransformBus::EventResult(parentId, m_entityId, &AZ::TransformBus::Events::GetParentId);
        if (!parentId.IsValid())
        {
            return false;
        }

        AZ::Vector3 worldTranslation;
        AZ::TransformBus::EventResult(worldTranslation, m_entityId, &AZ::TransformBus::Events::GetWorldTranslation);

        while (parentId.IsValid())
        {
            AZ::Vector3 parentTranslation;
            AZ::TransformBus::EventResult(parentTranslation, parentId, &AZ::TransformBus::Events::GetWorldTranslation);

            if (parentTranslation.GetDistanceSq(worldTranslation) < 0.01f)
            {
                CComponentEntityObject* parentObject = CComponentEntityObject::FindObjectForEntity(parentId);
                if (parentObject && !parentObject->IsSelected() && parentObject->IsEntityIconVisible())
                {
                    // An ancestor in the same location that's not selected and has icon visible has been found
                    return true;
                }
            }

            AZ::EntityId currentParentId = parentId;
            parentId.SetInvalid();
            AZ::TransformBus::EventResult(parentId, currentParentId, &AZ::TransformBus::Events::GetParentId);
        }
    }
    return false;
}

bool CComponentEntityObject::IsDescendantSelectedAtSameLocation() const
{
    if (m_entityId.IsValid())
    {
        if (GetObjectManager()->GetSelection() && GetObjectManager()->GetSelection()->GetCount() == 0)
        {
            return false;
        }

        AZ::Vector3 worldTranslation;
        AZ::TransformBus::EventResult(worldTranslation, m_entityId, &AZ::TransformBus::Events::GetWorldTranslation);

        // For each descendant, check if it's selected and if so if it's located at the same location as we are
        AzToolsFramework::EntityIdList descendantIds;
        AZ::TransformBus::EventResult(descendantIds, m_entityId, &AZ::TransformBus::Events::GetAllDescendants);
        for (AZ::EntityId entityId : descendantIds)
        {
            CComponentEntityObject* descendantObject = CComponentEntityObject::FindObjectForEntity(entityId);
            if (descendantObject && descendantObject->IsSelected())
            {
                // Check if this entity is at the exact location of us
                AZ::Vector3 entityTranslation;
                AZ::TransformBus::EventResult(entityTranslation, entityId, &AZ::TransformBus::Events::GetWorldTranslation);
                if (entityTranslation.GetDistanceSq(worldTranslation) < 0.01f)
                {
                    return true;
                }
            }
        }
    }
    return false;
}

void CComponentEntityObject::InvalidateTM(int nWhyFlags)
{
    CEntityObject::InvalidateTM(nWhyFlags);

    if (m_transformReentryGuard)
    {
        EditorActionScope transformChange(m_transformReentryGuard);

        if (m_entityId.IsValid())
        {
            Matrix34 worldTransform = GetWorldTM();
            EBUS_EVENT_ID(m_entityId, AZ::TransformBus, SetWorldTM, LYTransformToAZTransform(worldTransform));

            // When transformed via the editor, make sure the entity is marked dirty for undo capture.
            EBUS_EVENT(AzToolsFramework::ToolsApplicationRequests::Bus, AddDirtyEntity, m_entityId);

            if (CheckFlags(OBJFLAG_SELECTED))
            {
                EBUS_EVENT(AzToolsFramework::ToolsApplicationEvents::Bus, InvalidatePropertyDisplay, AzToolsFramework::Refresh_Values);
            }
        }
    }
}

void CComponentEntityObject::OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world)
{
    if (m_transformReentryGuard) // Ignore if action originated from Sandbox.
    {
        EditorActionScope transformChange(m_transformReentryGuard);
        Matrix34 worlTM = AZTransformToLYTransform(world);
        SetLocalTM(worlTM, eObjectUpdateFlags_Animated);
    }
}

int CComponentEntityObject::MouseCreateCallback(CViewport* view, EMouseEvent event, QPoint& point, int flags)
{
    if (event == eMouseMove || event == eMouseLDown)
    {
        Vec3 pos;
        if (GetIEditor()->GetAxisConstrains() != AXIS_TERRAIN)
        {
            pos = view->MapViewToCP(point);
        }
        else
        {
            // Snap to terrain.
            bool hitTerrain;
            pos = view->ViewToWorld(point, &hitTerrain);
            if (hitTerrain)
            {
                pos.z = GetIEditor()->GetTerrainElevation(pos.x, pos.y);
            }
            pos = view->SnapToGrid(pos);
        }

        pos = view->SnapToGrid(pos);
        SetPos(pos);

        if (event == eMouseLDown)
        {
            return MOUSECREATE_OK;
        }

        return MOUSECREATE_CONTINUE;
    }

    return CBaseObject::MouseCreateCallback(view, event, point, flags);
}

bool CComponentEntityObject::HitHelperTest(HitContext& hc)
{
    bool hit = CEntityObject::HitHelperTest(hc);
    if (!hit && m_entityId.IsValid())
    {
        // Pick against icon in screen space.
        if (IsEntityIconVisible())
        {
            const QPoint entityScreenPos = hc.view->WorldToView(GetWorldPos());
            const float screenPosX = entityScreenPos.x();
            const float screenPosY = entityScreenPos.y();
            const float iconRange = static_cast<float>(s_kIconSize / 2);

            if ((hc.point2d.x() >= screenPosX - iconRange && hc.point2d.x() <= screenPosX + iconRange)
                && (hc.point2d.y() >= screenPosY - iconRange && hc.point2d.y() <= screenPosY + iconRange))
            {
                hc.dist = hc.raySrc.GetDistance(GetWorldPos());
                hc.iconHit = true;
                return true;
            }
        }
    }
    return hit;
}

bool CComponentEntityObject::HitTest(HitContext& hc)
{
    if (m_iconOnlyHitTest)
    {
        return false;
    }

    if (m_entityId.IsValid())
    {
        // Pick against bounding box/geometry
        AABB bounds(AABB::RESET);
        GetBoundBox(bounds);
        if (!bounds.IsReset())
        {
            Vec3 hitPos;
            if (Intersect::Ray_AABB(Ray(hc.raySrc, hc.rayDir), bounds, hitPos))
            {
                IStatObj* geometry = nullptr;
                EBUS_EVENT_ID_RESULT(geometry, m_entityId, LmbrCentral::LegacyMeshComponentRequestBus, GetStatObj);

                // If we have a mesh, raycast against it.
                if (geometry)
                {
                    const Matrix34 inverseTM = GetWorldTM().GetInverted();
                    const Vec3 raySrcLocal = inverseTM.TransformPoint(hc.raySrc);
                    const Vec3 rayDirLocal = inverseTM.TransformVector(hc.rayDir).GetNormalized();

                    SRayHitInfo hi;
                    hi.inReferencePoint = raySrcLocal;
                    hi.inRay = Ray(raySrcLocal, rayDirLocal);
                    if (geometry->RayIntersection(hi))
                    {
                        const Vec3 worldHitPos = GetWorldTM().TransformPoint(hi.vHitPos);
                        hc.dist = hc.raySrc.GetDistance(worldHitPos);
                        hc.object = this;
                        return true;
                    }

                    return false;
                }
                else
                {
                    // Otherwise accept the AABB hit.
                    hc.dist = (hitPos - hc.raySrc).GetLength();
                    return true;
                }
            }

            return false;
        }
    }

    const Vec3 origin = GetWorldPos();
    const float radius = GetRadius();

    Vec3 w = origin - hc.raySrc;
    Vec3 wcross = hc.rayDir.Cross(w);
    const float d = wcross.GetLengthSquared();

    if (d < square(radius) + hc.distanceTolerance &&
        w.GetLengthSquared() > square(radius))
    {
        Vec3 i0;
        hc.object = this;
        if (Intersect::Ray_SphereFirst(Ray(hc.raySrc, hc.rayDir), Sphere(origin, radius), i0))
        {
            hc.dist = hc.raySrc.GetDistance(i0);
            return true;
        }
        hc.dist = hc.raySrc.GetDistance(origin);
        return true;
    }

    return false;
}

void CComponentEntityObject::GetBoundBox(AABB& box)
{
    CBaseObject::GetBoundBox(box);
}

void CComponentEntityObject::GetLocalBounds(AABB& box)
{
    box.Reset();

    if (m_entityId.IsValid())
    {
        AZ::Aabb aabb = AZ::Aabb::CreateNull();
        EBUS_EVENT_ID_RESULT(aabb, m_entityId, LmbrCentral::MeshComponentRequestBus, GetLocalBounds);
        if (aabb.IsValid())
        {
            box.Add(AZVec3ToLYVec3(aabb.GetMin()));
            box.Add(AZVec3ToLYVec3(aabb.GetMax()));
            return;
        }
    }

    float r = GetRadius();
    box.min = -Vec3(r, r, r);
    box.max = Vec3(r, r, r);
}

XmlNodeRef CComponentEntityObject::Export(const QString& levelPath, XmlNodeRef& xmlNode)
{
    // All component entities are serialized out in a separate pass, so they can be
    // loaded en-masse rather than individually. As such, we don't export them
    // alongside Cry Entities.
    return XmlNodeRef();
}

CComponentEntityObject* CComponentEntityObject::FindObjectForEntity(AZ::EntityId id)
{
    CEntityObject* object = nullptr;
    EBUS_EVENT_ID_RESULT(object, id, AzToolsFramework::ComponentEntityEditorRequestBus, GetSandboxObject);

    if (object && (object->GetType() == OBJTYPE_AZENTITY))
    {
        return static_cast<CComponentEntityObject*>(object);
    }

    return nullptr;
}

void CComponentEntityObject::Display(DisplayContext& dc)
{
    if (!(dc.flags & DISPLAY_2D))
    {
        m_entityIconVisible = false;
    }

    bool displaySelectionHelper = false;
    if (!CanBeDrawn(dc, displaySelectionHelper))
    {
        return;
    }

    DrawDefault(dc);

    bool showIcons = m_hasIcon;
    if (showIcons)
    {
        SEditorSettings* editorSettings = GetIEditor()->GetEditorSettings();
        if (!editorSettings->viewports.bShowIcons && !editorSettings->viewports.bShowSizeBasedIcons)
        {
            showIcons = false;
        }
    }

    if (m_entityId.IsValid())
    {
        // Do geometry debug draw if geometry highlighting is enabled (editor feature).
        if ((!IsSelected() && m_accentType == AzToolsFramework::EntityAccentType::Hover && GetIEditor()->GetEditorSettings()->viewports.bHighlightMouseOverGeometry) ||
            (IsSelected() && GetIEditor()->GetEditorSettings()->viewports.bHighlightSelectedGeometry))
        {
            SGeometryDebugDrawInfo dd;
            dd.tm = GetWorldTM();
            dd.bExtrude = true;
            dd.color = ColorB(250, 0, 250, 30);
            dd.lineColor = ColorB(255, 255, 0, 160);
            IStatObj* geometry = nullptr;
            EBUS_EVENT_ID_RESULT(geometry, m_entityId, LmbrCentral::LegacyMeshComponentRequestBus, GetStatObj);
            if (geometry)
            {
                geometry->DebugDraw(dd);
            }
        }

        // Draw link to parent if this or the parent object are selected.
        {
            AZ::EntityId parentId;
            EBUS_EVENT_ID_RESULT(parentId, m_entityId, AZ::TransformBus, GetParentId);
            if (parentId.IsValid())
            {
                CComponentEntityObject* parentObject = CComponentEntityObject::FindObjectForEntity(parentId);
                if (IsSelected() || (parentObject && parentObject->IsSelected()))
                {
                    const QColor kLinkColorParent(0, 255, 255);
                    const QColor kLinkColorChild(0, 0, 255);

                    AZ::Vector3 parentTranslation;
                    EBUS_EVENT_ID_RESULT(parentTranslation, parentId, AZ::TransformBus, GetWorldTranslation);
                    dc.DrawLine(AZVec3ToLYVec3(parentTranslation), GetWorldTM().GetTranslation(), kLinkColorParent, kLinkColorChild);
                }
            }
        }

        // Don't draw icons if we have an ancestor in the same location that has an icon - makes sure
        // ancestor icons draw on top and are able to be selected over children. Also check if a descendant
        // is selected at the same location. In cases of entity hierarchies where numerous ancestors have
        // no position offset, we need this so the ancestors don't draw over us when we're selected
        if (showIcons)
        {
            if ((dc.flags & DISPLAY_2D) ||
                IsSelected() || 
                IsAncestorIconDrawingAtSameLocation() ||
                IsDescendantSelectedAtSameLocation())
            {
                showIcons = false;
            }
        }

        // Allow components to override in-editor visualization.
        bool displayHandled = false;
        EBUS_EVENT(AzFramework::EntityDebugDisplayRequestBus, SetDC, &dc);
        EBUS_EVENT_ID(m_entityId, AzFramework::EntityDebugDisplayEventBus, DisplayEntity, displayHandled);
        if (showIcons)
        {
            if (!displaySelectionHelper && !IsSelected())
            {
                m_entityIconVisible = DisplayEntityIcon(dc);
            }
        }
        EBUS_EVENT(AzFramework::EntityDebugDisplayRequestBus, SetDC, nullptr);

        if (displayHandled || (dc.flags & DISPLAY_2D))
        {
            return;
        }
    }

    const Matrix34& wtm = GetWorldTM();

    Vec3 dir = wtm.TransformVector(Vec3(0, 1, 0));
    Vec3 wp = wtm.GetTranslation();
    if (IsFrozen())
    {
        dc.SetFreezeColor();
    }
    else
    {
        dc.SetColor(GetColor(), 0.8f);
    }

    // Draw a ball for easy selection (needed only if showing icons is desired but none are drawing).
    if (!IsSelected() && !IsEntityIconVisible() && !displaySelectionHelper && showIcons)
    {
        dc.DrawBall(wp, GetRadius());
    }

    if (IsSelected())
    {
        dc.SetSelectedColor(0.6f);
    }
}

void CComponentEntityObject::DrawDefault(DisplayContext& dc, const QColor& labelColor)
{
    CEntityObject::DrawDefault(dc, labelColor);

    DrawAccent(dc);
}

IStatObj* CComponentEntityObject::GetIStatObj()
{
    IStatObj* statObj = nullptr;
    LmbrCentral::LegacyMeshComponentRequestBus::EventResult(statObj, m_entityId, &LmbrCentral::LegacyMeshComponentRequests::GetStatObj);
    return statObj;
}

bool CComponentEntityObject::IsIsolated() const
{
    return m_isIsolated;
}

void CComponentEntityObject::OnContextMenu(QMenu* /*pMenu*/)
{
    // Deliberately bypass the base class implementation (CEntityObject::OnContextMenu()).
}

bool CComponentEntityObject::DisplayEntityIcon(DisplayContext& editorDisplay)
{
    if (!m_hasIcon)
    {
        return false;
    }

    auto* displayInterface = AzFramework::EntityDebugDisplayRequestBus::FindFirstHandler();
    AZ_Assert(displayInterface, "Invalid display context.");

    const QPoint entityScreenPos = editorDisplay.GetView()->WorldToView(GetWorldPos());

    const Vec3 worldPos = GetWorldPos();
    const CCamera& camera = gEnv->pRenderer->GetCamera();
    const Vec3 cameraToEntity = (worldPos - camera.GetMatrix().GetTranslation());
    const float distSq = cameraToEntity.GetLengthSquared();
    if (distSq > square(s_kIconMaxWorldDist))
    {
        return false;
    }

    int iconFlags = (int) DisplayContext::ETextureIconFlags::TEXICON_ON_TOP; // Draw component icons on top of meshes (no depth testing)
    SetDrawTextureIconProperties(editorDisplay, worldPos, 1.0f, iconFlags);

    const float iconScale = s_kIconMinScale + (s_kIconMaxScale - s_kIconMinScale) * (1.f - clamp_tpl(max(0.f, sqrt_tpl(distSq) - s_kIconCloseDist) / s_kIconFarDist, 0.0f, 1.f));
    const float worldDistToScreenScaleFraction = 0.045f;
    const float screenScale = editorDisplay.GetView()->GetScreenScaleFactor(GetWorldPos()) * worldDistToScreenScaleFraction;

    displayInterface->DrawTextureLabel(m_iconTexture, LYVec3ToAZVec3(worldPos), s_kIconSize * iconScale, s_kIconSize * iconScale, GetTextureIconFlags());

    return true;
}

void CComponentEntityObject::SetupEntityIcon()
{
    bool hideIconInViewport = false;
    m_hasIcon = false;

    AzToolsFramework::EditorEntityIconComponentRequestBus::EventResult(hideIconInViewport, m_entityId, &AzToolsFramework::EditorEntityIconComponentRequestBus::Events::IsEntityIconHiddenInViewport);
    
    if (!hideIconInViewport)
    {
        AzToolsFramework::EditorEntityIconComponentRequestBus::EventResult(m_icon, m_entityId, &AzToolsFramework::EditorEntityIconComponentRequestBus::Events::GetEntityIconPath);
        
        if (!m_icon.empty())
        {
            m_hasIcon = true;
            
            int textureId = GetIEditor()->GetIconManager()->GetIconTexture(m_icon.c_str());
            m_iconTexture = GetIEditor()->GetRenderer()->EF_GetTextureByID(textureId);
        }
    }
}

void CComponentEntityObject::DrawAccent(DisplayContext& dc)
{
    if (!m_drawAccents)
    {
        return;
    }

    switch (m_accentType)
    {
        case AzToolsFramework::EntityAccentType::None:
        {
            if (dc.flags & DISPLAY_2D)
            {
                dc.SetColor(0.941f, 0.764f, 0.176f); // Yellow
            }
            else
            {
                return;
            }
            break;
        }
        case AzToolsFramework::EntityAccentType::Hover:
        {
            dc.SetColor(0, 1, 0); // Green
            break;
        }
        case AzToolsFramework::EntityAccentType::Selected:
        {
            dc.SetColor(1, 0, 0); // Red
            break;
        }
        case AzToolsFramework::EntityAccentType::ParentSelected:
        {
            dc.SetColor(1, 0.549, 0); // Orange
            break;
        }
        case AzToolsFramework::EntityAccentType::SliceSelected:
        {
            dc.SetColor(0.117, 0.565, 1); // Blue
            break;
        }
        default:
        {
            dc.SetColor(1, 0.0784, 0.576); // Pink
            break;
        }
    }

    AABB box;
    GetLocalBounds(box);
    dc.PushMatrix(GetWorldTM());
    dc.DrawWireBox(box.min, box.max);
    dc.PopMatrix();
}

void CComponentEntityObject::SetMaterial(CMaterial* material)
{
    AZ::Entity* entity = nullptr;
    EBUS_EVENT_RESULT(entity, AZ::ComponentApplicationBus, FindEntity, m_entityId);
    if (entity)
    {
        if (material)
        {
            EBUS_EVENT_ID(m_entityId, LmbrCentral::MaterialOwnerRequestBus, SetMaterial, material->GetMatInfo());
        }
        else
        {
            EBUS_EVENT_ID(m_entityId, LmbrCentral::MaterialOwnerRequestBus, SetMaterial, nullptr);
        }
    }
}

CMaterial* CComponentEntityObject::GetMaterial() const
{
    _smart_ptr<IMaterial> material = nullptr;
    EBUS_EVENT_ID_RESULT(material, m_entityId, LmbrCentral::MaterialOwnerRequestBus, GetMaterial);
    return GetIEditor()->GetMaterialManager()->FromIMaterial(material);
}

CMaterial* CComponentEntityObject::GetRenderMaterial() const
{
    AZ::Entity* entity = nullptr;
    EBUS_EVENT_RESULT(entity, AZ::ComponentApplicationBus, FindEntity, m_entityId);
    if (entity)
    {
        _smart_ptr<IMaterial> material = nullptr;
        EBUS_EVENT_ID_RESULT(material, m_entityId, LmbrCentral::MaterialOwnerRequestBus, GetMaterial);

        if (material)
        {
            return GetIEditor()->GetMaterialManager()->LoadMaterial(material->GetName(), false);
        }
    }

    return nullptr;
}

#include <Objects/ComponentEntityObject.moc>
