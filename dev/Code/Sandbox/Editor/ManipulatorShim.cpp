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
#include "ManipulatorShim.h"

#include "IEditor.h"
#include "Editor/Objects/GizmoManager.h"
#include "Editor/Objects/ObjectManager.h"
#include "Editor/Objects/AxisGizmo.h"
#include "ViewManager.h"

#include <AzCore/Component/TransformBus.h>
#include <AzToolsFramework/Manipulators/ManipulatorBus.h>
#include <AzToolsFramework/Manipulators/ManipulatorSnapping.h>

template<typename Func>
static void ProcessSelectedEntities(Func request)
{
    AzToolsFramework::EntityIdList selectedEntityList;
    AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(
        selectedEntityList, &AzToolsFramework::ToolsApplicationRequests::GetSelectedEntities);

    for (const auto entity : selectedEntityList)
    {
        request(entity);
    }
}

static AZ_FORCE_INLINE bool ShiftCtrlHeld(const int flags)
{
    return (flags & MK_CONTROL) != 0 && (flags & MK_SHIFT) != 0;
}

ManipulatorShim::ManipulatorShim()
{
    // hide current gizmo for entity (translate/rotate/scale)
    IGizmoManager* gizmoManager = GetIEditor()->GetObjectManager()->GetGizmoManager();
    for (size_t i = 0; i < static_cast<size_t>(gizmoManager->GetGizmoCount()); ++i)
    {
        gizmoManager->RemoveGizmo(gizmoManager->GetGizmoByIndex(i));
    }
}

const GUID& ManipulatorShim::GetClassID()
{
    // {E90749BE-6318-46D5-A499-A4317E3DD452}
    static const GUID guid =
    {
        0xE90749BE, 0x6318, 0x46D5,{ 0xA4, 0x99, 0xA4, 0x31, 0x7E, 0x3D, 0xD4, 0x52 }
    };

    return guid;
}

void ManipulatorShim::RegisterTool(CRegistrationContext& rc)
{
    rc.pClassFactory->RegisterClass(
        new CQtViewClass<ManipulatorShim>("EditTool.Manipulator", "Select", ESYSTEM_CLASS_EDITTOOL));
}

bool ManipulatorShim::MouseCallback(CViewport* view, const EMouseEvent event, QPoint& point, const int flags)
{
    switch (event)
    {
    case eMouseLDown:
        {
            if (ShiftCtrlHeld(flags))
            {
                AzToolsFramework::ScopedUndoBatch surfaceSnapUndo("SurfaceSnap");
                ProcessSelectedEntities([view, &point](AZ::EntityId entityId)
                {
                    AzToolsFramework::ScopedUndoBatch::MarkEntityDirty(entityId);

                    const int viewportId = view->GetViewportId();
                    // get unsnapped terrain position (world space)
                    AZ::Vector3 worldSurfacePosition;
                    AzToolsFramework::ViewportInteractionRequestBus::EventResult(
                        worldSurfacePosition, viewportId,
                        &AzToolsFramework::ViewportInteractionRequestBus::Events::PickSurface,
                        AZ::Vector2(point.x(), point.y()));

                    AZ::Transform worldFromLocal;
                    AZ::TransformBus::EventResult(worldFromLocal, entityId, &AZ::TransformBus::Events::GetWorldTM);
                    const AZ::Transform localFromWorld = worldFromLocal.GetInverseFast();

                    // convert to local space - snap if enabled
                    const AZ::Vector3 localFinalSurfacePosition = AzToolsFramework::GridSnapping(viewportId)
                        ? AzToolsFramework::CalculateSnappedSurfacePosition(
                            worldSurfacePosition, worldFromLocal, viewportId, AzToolsFramework::GridSize(viewportId))
                        : localFromWorld * worldSurfacePosition;

                    AzToolsFramework::ManipulatorRequestBus::Event(
                        entityId, &AzToolsFramework::ManipulatorRequestBus::Events::SetSelectedPosition,
                        localFinalSurfacePosition);
                });

                return true;
            }

            ProcessSelectedEntities([](AZ::EntityId entityId)
            {
                AzToolsFramework::ManipulatorRequestBus::Event(
                    entityId, &AzToolsFramework::ManipulatorRequestBus::Events::ClearSelected);
            });

            // when clicking off - accept current undo state if recording, otherwise
            // cancel undo will be triggered and selected state will be lost
            if (GetIEditor()->IsUndoRecording())
            {
                GetIEditor()->AcceptUndo(QString("ManipulatorShimDeslect"));
            }

            // show translate/rotate/scale gizmo again
            if (IGizmoManager* gizmoManager = GetIEditor()->GetObjectManager()->GetGizmoManager())
            {
                if (CBaseObject* selectedObject = GetIEditor()->GetSelectedObject())
                {
                    gizmoManager->AddGizmo(new CAxisGizmo(selectedObject));
                }
            }

            GetIEditor()->SetEditTool(nullptr);
        }
        return true;
    default:
        return false;
    }
}

bool ManipulatorShim::OnKeyDown(CViewport* /*view*/, const uint32 nChar, uint32 /*nRepCnt*/, uint32 /*nFlags*/)
{
    switch (nChar)
    {
    case VK_DELETE:
        {
            AzToolsFramework::ScopedUndoBatch deleteUndo("DeleteUndo");
            ProcessSelectedEntities([](AZ::EntityId entityId)
            {
                AzToolsFramework::ScopedUndoBatch::MarkEntityDirty(entityId);
                AzToolsFramework::DestructiveManipulatorRequestBus::Event(
                    entityId, &AzToolsFramework::DestructiveManipulatorRequestBus::Events::DestroySelected);
            });

            GetIEditor()->SetEditTool(nullptr);
        }
        return true;
    default:
        return false;
    }
}

#include <ManipulatorShim.moc>