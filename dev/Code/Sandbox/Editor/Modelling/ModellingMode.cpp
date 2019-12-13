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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#include "StdAfx.h"
#if defined(AZ_PLATFORM_WINDOWS)
#include <InitGuid.h>
#endif
#include "ModellingMode.h"
#include "Viewport.h"
#include "ViewManager.h"
#include "Objects/SubObjSelection.h"
#include "Objects/GizmoManager.h"
#include "Objects/AxisGizmo.h"

#include "EditMode/SubObjectSelectionReferenceFrameCalculator.h"

#include "Util/BoostPythonHelpers.h"
#include <AzCore/RTTI/BehaviorContext.h>

#define HIT_PIXELS_SIZE (5)

void CModellingModeTool::PySetSelectionType(const char* modeName)
{
    if (!m_pModellingModeTool)
    {
        GetIEditor()->SetEditTool(new CModellingModeTool);
    }
    if (!m_pModellingModeTool)
    {
        return;
    }

    if (_stricmp(modeName, "vertex") == 0)
    {
        m_pModellingModeTool->SetSelectType(SO_ELEM_VERTEX);
    }
    else if (_stricmp(modeName, "edge") == 0)
    {
        m_pModellingModeTool->SetSelectType(SO_ELEM_EDGE);
    }
    else if (_stricmp(modeName, "face") == 0)
    {
        m_pModellingModeTool->SetSelectType(SO_ELEM_FACE);
    }
    else if (_stricmp(modeName, "polygon") == 0)
    {
        m_pModellingModeTool->SetSelectType(SO_ELEM_POLYGON);
    }
    else if (_stricmp(modeName, "uv") == 0)
    {
        m_pModellingModeTool->SetSelectType(SO_ELEM_UV);
    }
}

const char* CModellingModeTool::Debug_GetSelectionType()
{
    if (!m_pModellingModeTool)
    {
        GetIEditor()->SetEditTool(new CModellingModeTool);
    }
    if (!m_pModellingModeTool)
    {
        return "";
    }

    switch (m_pModellingModeTool->m_currSelectionType) 
    {
        case SO_ELEM_VERTEX:
        {
            return "vertex";
        }
        case SO_ELEM_EDGE:
        {
            return "edge";
        }
        case SO_ELEM_FACE:
        {
            return "face";
        }
        case SO_ELEM_POLYGON:
        {
            return "polygon";
        }
        case SO_ELEM_UV:
        {
            return "uv";
        }
        default:
        {
            break;
        }
    }

    return "";
}

namespace AzToolsFramework
{
    void ModellingModeFuncsHandler::Reflect(AZ::ReflectContext* context)
    {
        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            // this will put these methods into the 'azlmbr.legacy.general' module
            auto addLegacyGeneral = [](AZ::BehaviorContext::GlobalMethodBuilder methodBuilder)
            {
                methodBuilder->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Attribute(AZ::Script::Attributes::Category, "Legacy/Editor")
                    ->Attribute(AZ::Script::Attributes::Module, "legacy.general");
            };
            addLegacyGeneral(behaviorContext->Method("set_modelling_selection", CModellingModeTool::PySetSelectionType, nullptr, "Sets the selection type in the modelling mode."));
            addLegacyGeneral(behaviorContext->Method("get_modelling_selection", CModellingModeTool::Debug_GetSelectionType, nullptr, "Gets the selection type in the modelling mode."));

        }
    }
}

REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(CModellingModeTool::PySetSelectionType, general, set_modelling_selection,
    "Sets the selection type in the modelling mode.",
    "general.set_modelling_selection(str modeName)");

//////////////////////////////////////////////////////////////////////////
CModellingModeTool* CModellingModeTool::m_pModellingModeTool = 0;
ESubObjElementType  CModellingModeTool::m_currSelectionType = SO_ELEM_VERTEX;

//////////////////////////////////////////////////////////////////////////
CModellingModeTool::CModellingModeTool()
{
    m_pModellingModeTool = this;
    m_pClassDesc = GetIEditor()->GetClassFactory()->FindClass(MODELLING_MODE_GUID);
    SetStatusText(tr("Modelling Mode"));

    m_commandMode = NothingMode;
    m_pMouseOverObject = NULL;
}

//////////////////////////////////////////////////////////////////////////
CModellingModeTool::~CModellingModeTool()
{
    m_pModellingModeTool = NULL;
    GetIEditor()->ShowTransformManipulator(false);
}

//////////////////////////////////////////////////////////////////////////
void CModellingModeTool::SetSelectType(ESubObjElementType type)
{
    m_currSelectionType = type;

    UpdateSelectionGizmo();
}

//////////////////////////////////////////////////////////////////////////
void CModellingModeTool::BeginEditParams(IEditor* ie, int flags)
{
    GetIEditor()->ClearSelection();
    ((CObjectManager*)GetIEditor()->GetObjectManager())->HideTransformManipulators();

    UpdateSelectionGizmo();
}

//////////////////////////////////////////////////////////////////////////
void CModellingModeTool::EndEditParams()
{
}

//////////////////////////////////////////////////////////////////////////
void CModellingModeTool::Display(struct DisplayContext& dc)
{
}

//////////////////////////////////////////////////////////////////////////
bool CModellingModeTool::HitTest(CViewport* view, const QPoint& point, const QRect& rc, int nFlags)
{
    bool bModifySelection = (nFlags & (SO_HIT_SELECT_ADD | SO_HIT_SELECT_REMOVE)) != 0;
    //////////////////////////////////////////////////////////////////////////
    // Hit test current transform manipulator.
    //////////////////////////////////////////////////////////////////////////
    ITransformManipulator* pManipulator = GetIEditor()->GetTransformManipulator();
    if (pManipulator && !bModifySelection)
    {
        HitContext hc;
        hc.view = view;
        hc.rect = rc;
        hc.point2d = point;
        view->ViewToWorldRay(point, hc.raySrc, hc.rayDir);
        if (pManipulator->HitTestManipulator(hc))
        {
            // Hit axis gizmo.
            GetIEditor()->SetAxisConstraints((AxisConstrains)hc.axis);
            view->SetAxisConstrain(hc.axis);
            return true;
        }
    }

    if (nFlags & SO_HIT_SELECT)
    {
        //gEnv->pLog->LogWithType(ILog::eAlways, "%s(%d): GetIEditor()->BeginUndo();", __FILE__, __LINE__);

        GetIEditor()->BeginUndo();
    }

    bool bAnyHit = false;

    HitContext hit;
    hit.point2d = point;
    hit.rect = rc;
    hit.view = view;
    hit.nSubObjFlags = nFlags;
    view->ViewToWorldRay(point, hit.raySrc, hit.rayDir);

    HitContext hitObject;
    if (view->HitTest(point, hitObject))
    {
        if (hitObject.object)
        {
            hitObject.object->HitTest(hit);
        }
    }

    /*
    for (SelectedObjectList::iterator itObject = this->m_selectedObjects.begin(); itObject != this->m_selectedObjects.end(); ++itObject)
    {
        if ((*itObject)->HitTest(hit))
        {
            if (!(nFlags & SO_HIT_SELECT))
                return true;
            bAnyHit = true;
        }
    }
    */
    if (nFlags & SO_HIT_SELECT)
    {
        //gEnv->pLog->LogWithType(ILog::eAlways, "%s(%d): GetIEditor()->AcceptUndo( \"SubObject Select\" );", __FILE__, __LINE__);

        GetIEditor()->AcceptUndo("SubObject Select");
    }

    UpdateSelectionGizmo();

    return bAnyHit;
}

//////////////////////////////////////////////////////////////////////////
bool CModellingModeTool::MouseCallback(CViewport* view, EMouseEvent event, QPoint& point, int flags)
{
    switch (event)
    {
    case eMouseLDown:
        return OnLButtonDown(view, flags, point);
        break;
    case eMouseLUp:
        return OnLButtonUp(view, flags, point);
        break;
    case eMouseLDblClick:
        return OnLButtonDblClk(view, flags, point);
        break;
    case eMouseMove:
        return OnMouseMove(view, flags, point);
        break;
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
bool CModellingModeTool::OnKeyDown(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags)
{
    if (nChar == 'V')
    {
        m_currSelectionType = SO_ELEM_VERTEX;
        //if(m_pTypePanel)
        //m_pTypePanel->SelectElemtType(m_currSelectionType);
        //Command_SelectVertex();
        view->SetActiveWindow();
        view->SetFocus();
    }
    if (nChar == 'E')
    {
        m_currSelectionType = SO_ELEM_EDGE;
        //if(m_pTypePanel)
        //m_pTypePanel->SelectElemtType(m_currSelectionType);
        //Command_SelectEdge();
        view->SetActiveWindow();
        view->SetFocus();
    }
    if (nChar == 'F')
    {
        m_currSelectionType = SO_ELEM_FACE;
        //if(m_pTypePanel)
        //m_pTypePanel->SelectElemtType(m_currSelectionType);
        //Command_SelectFace();
        view->SetActiveWindow();
        view->SetFocus();
    }
    if (nChar == 'P')
    {
        m_currSelectionType = SO_ELEM_POLYGON;
        //if(m_pTypePanel)
        //m_pTypePanel->SelectElemtType(m_currSelectionType);
        //Command_SelectPolygon();
        view->SetActiveWindow();
        view->SetFocus();
    }

    UpdateSelectionGizmo();

    //pPanel->SelectElemtType(m_currSelectionType);
    return false;
}

//////////////////////////////////////////////////////////////////////////
bool CModellingModeTool::OnKeyUp(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags)
{
    return false;
}

//////////////////////////////////////////////////////////////////////////
bool CModellingModeTool::OnLButtonDown(CViewport* view, int nFlags, const QPoint& point)
{
    // CPointF ptMarker;
    QPoint ptCoord;
    int iCurSel = -1;

    // Save the mouse down position
    m_cMouseDownPos = point;

    view->ResetSelectionRegion();

    Vec3 pos = view->SnapToGrid(view->ViewToWorld(point));

    // Get control key status.
    const bool bAltClick = (Qt::AltModifier & QApplication::queryKeyboardModifiers());
    bool bCtrlClick = (nFlags & MK_CONTROL);
    bool bShiftClick = (nFlags & MK_SHIFT);

    bool bModifySelection = bCtrlClick || bAltClick;
    bool bAddSelect = bCtrlClick;
    bool bUnselect = bAltClick;

    bool bLockSelection = GetIEditor()->IsSelectionLocked();

    int numUnselected = 0;
    int numSelected = 0;

    int nHitFlags = SO_HIT_POINT | SO_HIT_TEST_SELECTED;
    QSize hitSize = QSize(HIT_PIXELS_SIZE, HIT_PIXELS_SIZE);
    QRect hitRC(QPoint(point.x() - hitSize.width(), point.y() - hitSize.height()), 2*hitSize);
    bool bHitSelected = HitTest(view, point, hitRC, nHitFlags);

    int editMode = GetIEditor()->GetEditMode();

    if (editMode == eEditModeMove && !bModifySelection)
    {
        SetCommandMode(MoveMode);
        // Check for Move to position.
        if (bCtrlClick && bShiftClick)
        {
            // Ctrl-Click on terain will move selected objects to specified location.
            //MoveSelectionToPos( view,pos );
            //bLockSelection = true;
        }
        if (bHitSelected)
        {
            bLockSelection = true;
        }
    }
    else if (editMode == eEditModeRotate && !bModifySelection)
    {
        SetCommandMode(RotateMode);
        if (bHitSelected)
        {
            bLockSelection = true;
        }
    }
    else if (editMode == eEditModeScale && !bModifySelection)
    {
        SetCommandMode(ScaleMode);
        if (bHitSelected)
        {
            bLockSelection = true;
        }
    }

    if (!bLockSelection)
    {
        if (!bHitSelected || editMode == eEditModeSelect || bModifySelection)
        {
            SetCommandMode(SelectMode);
        }

        // Now do the actual selection.
        nHitFlags = SO_HIT_SELECT | SO_HIT_POINT;
        if (bAddSelect)
        {
            nHitFlags |= SO_HIT_SELECT_ADD;
        }
        else if (bUnselect)
        {
            nHitFlags |= SO_HIT_SELECT_REMOVE;
        }
        HitTest(view, point, hitRC, nHitFlags);
    }

    if (GetCommandMode() == MoveMode ||
        GetCommandMode() == RotateMode ||
        GetCommandMode() == ScaleMode)
    {
        //gEnv->pLog->LogWithType(ILog::eAlways, "%s(%d): view->BeginUndo();", __FILE__, __LINE__);
        view->BeginUndo();
    }

    //////////////////////////////////////////////////////////////////////////
    view->CaptureMouse();
    //////////////////////////////////////////////////////////////////////////

    ITransformManipulator* pManipulator = GetIEditor()->GetTransformManipulator();
    if (pManipulator)
    {
        view->SetConstructionMatrix(COORDS_LOCAL, pManipulator->GetTransformation(COORDS_LOCAL));
        view->SetConstructionMatrix(COORDS_PARENT, pManipulator->GetTransformation(COORDS_PARENT));
        view->SetConstructionMatrix(COORDS_USERDEFINED, pManipulator->GetTransformation(COORDS_USERDEFINED));
    }

    UpdateSelectionGizmo();

    return true;
}

//////////////////////////////////////////////////////////////////////////
bool CModellingModeTool::OnLButtonUp(CViewport* view, int nFlags, const QPoint& point)
{
    // Reset the status bar caption
    GetIEditor()->SetStatusText("Ready");

    //////////////////////////////////////////////////////////////////////////
    if (view->IsUndoRecording())
    {
        if (GetCommandMode() == MoveMode)
        {
            //gEnv->pLog->LogWithType(ILog::eAlways, "%s(%d): view->AcceptUndo( \"Move SubObj Elemnt\" );", __FILE__, __LINE__);

            view->AcceptUndo("Move SubObj Elemnt");
            for (SelectedObjectList::iterator itObject = this->m_selectedObjects.begin(); itObject != this->m_selectedObjects.end(); ++itObject)
            {
                (*itObject)->AcceptSubObjectModify();
            }
        }
        else if (GetCommandMode() == RotateMode)
        {
            //gEnv->pLog->LogWithType(ILog::eAlways, "%s(%d): view->AcceptUndo( \"Rotate SubObj Element\" );", __FILE__, __LINE__);

            view->AcceptUndo("Rotate SubObj Element");
            for (SelectedObjectList::iterator itObject = this->m_selectedObjects.begin(); itObject != this->m_selectedObjects.end(); ++itObject)
            {
                (*itObject)->AcceptSubObjectModify();
            }
        }
        else if (GetCommandMode() == ScaleMode)
        {
            //gEnv->pLog->LogWithType(ILog::eAlways, "%s(%d): view->AcceptUndo( \"Scale SubObj Element\" );", __FILE__, __LINE__);

            view->AcceptUndo("Scale SubObj Element");
            for (SelectedObjectList::iterator itObject = this->m_selectedObjects.begin(); itObject != this->m_selectedObjects.end(); ++itObject)
            {
                (*itObject)->AcceptSubObjectModify();
            }
        }
        else
        {
            //gEnv->pLog->LogWithType(ILog::eAlways, "%s(%d): view->CancelUndo();", __FILE__, __LINE__);

            view->CancelUndo();
        }
    }
    //////////////////////////////////////////////////////////////////////////

    if (GetCommandMode() == SelectMode && (!GetIEditor()->IsSelectionLocked()))
    {
        bool bAddSelection = (nFlags & MK_CONTROL);
        const bool bUnselect = (Qt::AltModifier & QApplication::queryKeyboardModifiers());
        QRect selectRect = view->GetSelectionRectangle();
        if (!selectRect.isEmpty())
        {
            // Ignore too small rectangles.
            if (selectRect.width() > 5 && selectRect.height() > 5)
            {
                int nHitFlags = SO_HIT_SELECT;
                if (bAddSelection)
                {
                    nHitFlags |= SO_HIT_SELECT_ADD;
                }
                else if (bUnselect)
                {
                    nHitFlags |= SO_HIT_SELECT_REMOVE;
                }
                HitTest(view, point, selectRect, nHitFlags);
            }
        }
    }
    // Release the restriction of the cursor
    view->ReleaseMouse();

    if (GetIEditor()->GetEditMode() != eEditModeSelectArea)
    {
        view->ResetSelectionRegion();
    }
   
    // Reset selected rectangle.
    view->SetSelectionRectangle(QRect());

    // Restore default editor axis constrain.
    if (GetIEditor()->GetAxisConstrains() != view->GetAxisConstrain())
    {
        view->SetAxisConstrain(GetIEditor()->GetAxisConstrains());
    }

    SetCommandMode(NothingMode);

    UpdateSelectionGizmo();

    return true;
}

//////////////////////////////////////////////////////////////////////////
bool CModellingModeTool::OnLButtonDblClk(CViewport* view, int nFlags, const QPoint& point)
{
    // If shift clicked, Move the camera to this place.
    if (nFlags & MK_SHIFT)
    {
        // Get the heightmap coordinates for the click position
        Vec3 v = view->ViewToWorld(point);
        if (!(v.x == 0 && v.y == 0 && v.z == 0))
        {
            Matrix34 tm = view->GetViewTM();
            Vec3 p = tm.GetTranslation();
            float height = p.z - GetIEditor()->GetTerrainElevation(p.x, p.y);
            if (height < 1)
            {
                height = 1;
            }
            p.x = v.x;
            p.y = v.y;
            p.z = GetIEditor()->GetTerrainElevation(p.x, p.y) + height;
            tm.SetTranslation(p);
            view->SetViewTM(tm);
        }
    }

    UpdateSelectionGizmo();

    return true;
}

//////////////////////////////////////////////////////////////////////////
void CModellingModeTool::MoveSelectionToPos(CViewport* view, Vec3& pos)
{
    //gEnv->pLog->LogWithType(ILog::eAlways, "%s(%d): view->BeginUndo();", __FILE__, __LINE__);

    view->BeginUndo();
    // Find center of selection.
    //Vec3 center = GetIEditor()->GetSelection()->GetCenter();
    //GetIEditor()->GetSelection()->Move( pos-center,false,true );
    //gEnv->pLog->LogWithType(ILog::eAlways, "%s(%d): view->AcceptUndo( \"Move Selection\" );", __FILE__, __LINE__);
    view->AcceptUndo("Move Selection");
}

//////////////////////////////////////////////////////////////////////////
bool CModellingModeTool::OnMouseMove(CViewport* view, int nFlags, const QPoint& point)
{
    if (!(nFlags & MK_RBUTTON))
    {
        SetObjectCursor(view, 0);
    }

    if (m_pHighlightedObject)
    {
        // Clear sub object selection of the highlighted object.
        SSubObjSelectionModifyContext ctx;
        ctx.view = view;
        ctx.type = SO_MODIFY_UNSELECT;
        m_pHighlightedObject->ModifySubObjSelection(ctx);
    }

    // get world/local coordinate system setting.
    int coordSys = GetIEditor()->GetReferenceCoordSys();

    Matrix34 modRefFrame;
    modRefFrame.SetIdentity();
    ITransformManipulator* pTransformManipulator = GetIEditor()->GetTransformManipulator();
    if (pTransformManipulator)
    {
        modRefFrame = pTransformManipulator->GetTransformation(GetIEditor()->GetReferenceCoordSys());
    }

    // get current axis constrains.
    if (GetCommandMode() == MoveMode || GetCommandMode() == RotateMode || GetCommandMode() == ScaleMode)
    {
        if (pTransformManipulator)
        {
            //gEnv->pLog->LogWithType(ILog::eAlways, "%s(%d): Transform manipulator", __FILE__, __LINE__);
            QPoint pt = point;
            OnManipulatorDrag(view, pTransformManipulator, m_cMouseDownPos, pt /*by-ref*/, Vec3(0, 0, 0));
        }
        return true;
    }
    else if (GetCommandMode() == SelectMode)
    {
        // Ignore select when selection locked.
        if (GetIEditor()->IsSelectionLocked())
        {
            return true;
        }

        if (GetIEditor()->GetEditMode() == eEditModeSelectArea)
        {
            view->OnDragSelectRectangle(m_cMouseDownPos, point, false);
        }
        else
        {
            view->SetSelectionRectangle(m_cMouseDownPos, point);
        }
    }
    else
    {
        // Highlight element where mouse now points.

        HitContext hit;
        hit.point2d = point;
        QSize hitSize = QSize(HIT_PIXELS_SIZE, HIT_PIXELS_SIZE);
        hit.rect = QRect(QPoint(point.x() - hitSize.width(), point.y() - hitSize.height()), 2 * hitSize);
        hit.nSubObjFlags = SO_HIT_POINT | SO_HIT_ELEM_ALL;

        if (view->HitTest(point, hit))
        {
            if (hit.object)
            {
                m_pHighlightedObject = hit.object;
                view->ViewToWorldRay(point, hit.raySrc, hit.rayDir);
                hit.nSubObjFlags = SO_HIT_POINT | SO_HIT_SELECT | SO_HIT_HIGHLIGHT_ONLY |
                    SO_HIT_ELEM_ALL;
                hit.object->HitTest(hit);
            }
        }
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////
void CModellingModeTool::OnManipulatorDrag(CViewport* view, ITransformManipulator* pManipulator, QPoint& point0, QPoint& point1, const Vec3& value)
{
    // get world/local coordinate system setting.
    RefCoordSys coordSys = GetIEditor()->GetReferenceCoordSys();
    int editMode = GetIEditor()->GetEditMode();

    Matrix34 modRefFrame = pManipulator->GetTransformation(coordSys);

    //gEnv->pLog->LogWithType(ILog::eAlways, "%s(%d): view->IsUndoRecording() = %d", __FILE__, __LINE__, view->IsUndoRecording());

    // get current axis constrains.
    if (editMode == eEditModeMove)
    {
        //gEnv->pLog->LogWithType(ILog::eAlways, "%s(%d): GetIEditor()->RestoreUndo();", __FILE__, __LINE__);

        GetIEditor()->RestoreUndo();

        SSubObjSelectionModifyContext modCtx;
        modCtx.view = view;
        modCtx.type = SO_MODIFY_MOVE;
        modCtx.worldRefFrame = modRefFrame;
        modCtx.vValue = value;

        for (SelectedObjectList::iterator itObject = this->m_selectedObjects.begin(); itObject != this->m_selectedObjects.end(); ++itObject)
        {
            (*itObject)->ModifySubObjSelection(modCtx);
        }
    }
    if (editMode == eEditModeRotate)
    {
        //gEnv->pLog->LogWithType(ILog::eAlways, "%s(%d): GetIEditor()->RestoreUndo();", __FILE__, __LINE__);

        GetIEditor()->RestoreUndo();

        SSubObjSelectionModifyContext modCtx;
        modCtx.view = view;
        modCtx.type = SO_MODIFY_ROTATE;
        modCtx.worldRefFrame = modRefFrame;
        modCtx.vValue = value;

        for (SelectedObjectList::iterator itObject = this->m_selectedObjects.begin(); itObject != this->m_selectedObjects.end(); ++itObject)
        {
            (*itObject)->ModifySubObjSelection(modCtx);
        }
    }
    if (editMode == eEditModeScale)
    {
        //gEnv->pLog->LogWithType(ILog::eAlways, "%s(%d): GetIEditor()->RestoreUndo();", __FILE__, __LINE__);

        GetIEditor()->RestoreUndo();

        Vec3 scl(0, 0, 0);

        Vec3 p1 = view->MapViewToCP(point0);
        Vec3 p2 = view->MapViewToCP(point1);
        if (p1.IsZero() || p2.IsZero())
        {
            return;
        }
        Vec3 v = view->GetCPVector(p1, p2);
        float sign = 1.0f;

        // Find the sign of the major vector axis.
        if (fabs(v.x) > fabs(v.y) && fabs(v.x) > fabs(v.z))
        {
            if (v.x < 0)
            {
                sign = -sign;
            }
        }
        else if (fabs(v.y) > fabs(v.x) && fabs(v.y) > fabs(v.z))
        {
            if (v.y < 0)
            {
                sign = -sign;
            }
        }
        else if (fabs(v.z) > fabs(v.x) && fabs(v.z) > fabs(v.y))
        {
            if (v.z < 0)
            {
                sign = -sign;
            }
        }
        float s = 1.0f + sign * v.GetLength();

        //float ay = 1.0f - 0.01f*(point1.y - point0.y);
        //if (ay < 0.01f) ay = 0.01f;
        switch (view->GetAxisConstrain())
        {
        case AXIS_X:
            scl(s, 1, 1);
            break;
        case AXIS_Y:
            scl(1, s, 1);
            break;
        case AXIS_Z:
            scl(1, 1, s);
            break;
        case AXIS_XY:
            scl(s, s, 1);
            break;
        case AXIS_XZ:
            scl(s, 1, s);
            break;
        case AXIS_YZ:
            scl(1, s, s);
            break;
        default:
            scl(s, s, s);
            break;
        }
        ;

        //m_cMouseDownPos = point;
        SSubObjSelectionModifyContext modCtx;
        modCtx.view = view;
        modCtx.type = SO_MODIFY_SCALE;
        modCtx.worldRefFrame = modRefFrame;
        modCtx.vValue = scl;

        for (SelectedObjectList::iterator itObject = this->m_selectedObjects.begin(); itObject != this->m_selectedObjects.end(); ++itObject)
        {
            (*itObject)->ModifySubObjSelection(modCtx);
        }
    }

    UpdateSelectionGizmo();
}

//////////////////////////////////////////////////////////////////////////
void CModellingModeTool::SetObjectCursor(CViewport* view, CBaseObject* hitObj, bool bChangeNow)
{
    return;

    EStdCursor cursor = STD_CURSOR_SUBOBJ_SEL;
    QString m_cursorStr;
    //HCURSOR hPrevCursor = m_hCurrCursor;
    if (m_pMouseOverObject)
    {
        m_pMouseOverObject->SetHighlight(false);
    }
    m_pMouseOverObject = hitObj;
    bool bHitSelectedObject = false;
    if (m_pMouseOverObject)
    {
        if (GetCommandMode() != SelectMode)
        {
            m_pMouseOverObject->SetHighlight(true);
            m_cursorStr = m_pMouseOverObject->GetName();
            cursor = STD_CURSOR_HIT;
            if (m_pMouseOverObject->IsSelected())
            {
                bHitSelectedObject = true;
            }
        }
    }
    else
    {
        m_cursorStr = "";
        cursor = STD_CURSOR_SUBOBJ_SEL;
    }
    // Get control key status.
    const auto modifiers = QApplication::queryKeyboardModifiers();
    const bool bAltClick = (Qt::AltModifier & modifiers);
    const bool bCtrlClick = (Qt::ControlModifier & modifiers);
    const bool bShiftClick = (Qt::ShiftModifier & modifiers);
    bool bNoRemoveSelection = bCtrlClick || bAltClick;
    bool bUnselect = bAltClick;
    bool bLockSelection = GetIEditor()->IsSelectionLocked();

    if (GetCommandMode() == SelectMode)
    {
        if (bCtrlClick)
        {
            cursor = STD_CURSOR_SUBOBJ_SEL_PLUS;
        }
        if (bAltClick)
        {
            cursor = STD_CURSOR_SUBOBJ_SEL_MINUS;
        }
    }
    else if ((bHitSelectedObject && !bNoRemoveSelection) || bLockSelection)
    {
        int editMode = GetIEditor()->GetEditMode();
        if (editMode == eEditModeMove)
        {
            cursor = STD_CURSOR_MOVE;
        }
        else if (editMode == eEditModeRotate)
        {
            cursor = STD_CURSOR_ROTATE;
        }
        else if (editMode == eEditModeScale)
        {
            cursor = STD_CURSOR_SCALE;
        }
    }
    /*
    if (bChangeNow)
    {
    if (GetCapture() == NULL)
    {
    if (m_hCurrCursor)
    SetCursor( m_hCurrCursor );
    else
    SetCursor( m_hDefaultCursor );
    }
    }
    */
    view->SetCurrentCursor(cursor, m_cursorStr);
}

//////////////////////////////////////////////////////////////////////////
void CModellingModeTool::RegisterTool(CRegistrationContext& rc)
{
    rc.pClassFactory->RegisterClass(new CQtViewClass<CModellingModeTool>("EditTool.ModellingMode", "Select", ESYSTEM_CLASS_EDITTOOL));
    // Register commands.
    CommandManagerHelper::RegisterCommand(rc.pCommandManager, "edit_mode", "modelling_mode", "", "", functor(&CModellingModeTool::Command_ModellingMode));
}

//////////////////////////////////////////////////////////////////////////
void CModellingModeTool::Command_ModellingMode()
{
    if (!m_pModellingModeTool)
    {
        GetIEditor()->SetEditTool(new CModellingModeTool);
    }
    if (m_pModellingModeTool)
    {
        m_pModellingModeTool->SetSelectType(SO_ELEM_VERTEX);
    }
}

//////////////////////////////////////////////////////////////////////////
void CModellingModeTool::UpdateSelectionGizmo()
{
    Matrix34 refFrame;
    if (!GetSelectionReferenceFrame(refFrame))
    {
        GetIEditor()->ShowTransformManipulator(false);
    }
    else
    {
        ITransformManipulator* pManipulator = GetIEditor()->ShowTransformManipulator(true);

        Matrix34 parentTM = m_selectedObjects[0]->GetWorldTM();
        Matrix34 userTM = GetIEditor()->GetViewManager()->GetGrid()->GetMatrix();
        parentTM.SetTranslation(refFrame.GetTranslation());
        userTM.SetTranslation(refFrame.GetTranslation());
        //tm.SetTranslation( m_selectionCenter );
        pManipulator->SetTransformation(COORDS_LOCAL, refFrame);
        pManipulator->SetTransformation(COORDS_PARENT, parentTM);
        pManipulator->SetTransformation(COORDS_USERDEFINED, userTM);
    }
}

//////////////////////////////////////////////////////////////////////////
bool CModellingModeTool::GetSelectionReferenceFrame(Matrix34& refFrame)
{
    SubObjectSelectionReferenceFrameCalculator calculator(this->m_currSelectionType);
    for (SelectedObjectList::iterator itObject = this->m_selectedObjects.begin(); itObject != this->m_selectedObjects.end(); ++itObject)
    {
        (*itObject)->CalculateSubObjectSelectionReferenceFrame(&calculator);
    }

    return calculator.GetFrame(refFrame);
}

#include <Modelling/ModellingMode.moc>
