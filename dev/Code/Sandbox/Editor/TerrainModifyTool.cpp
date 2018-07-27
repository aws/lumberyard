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

// Description : Terrain Modification Tool implementation.


#include "StdAfx.h"
#ifdef AZ_PLATFORM_WINDOWS
#include "INITGUID.H"
#endif
#include "TerrainModifyTool.h"
#include "TerrainPanel.h"
#include "Viewport.h"
#include "TerrainModifyPanel.h"
#include "Terrain/Heightmap.h"
#include "VegetationMap.h"
#include "Objects/DisplayContext.h"
#include "Util/BoostPythonHelpers.h"
#include "QtUtilWin.h"
#include "MainWindow.h"
#include <ITerrain.h>
#include <QVBoxLayout>

#ifdef LoadCursor
#undef LoadCursor
#endif

// {9BC941A8-44F1-4b91-B270-E7EF55167E48}
DEFINE_GUID(TERRAIN_MODIFY_TOOL_GUID, 0x9bc941a8, 0x44f1, 0x4b91, 0xb2, 0x70, 0xe7, 0xef, 0x55, 0x16, 0x7e, 0x48);

enum EPaintMode
{
    ePaintMode_None = 0,
    ePaintMode_Ready,
    ePaintMode_InProgress,
};

REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(CTerrainModifyTool::Command_Flatten, terrain, set_tool_flatten,
    "Sets the terrain flatten tool.",
    "terrain.set_tool_flatten()");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(CTerrainModifyTool::Command_Smooth, terrain, set_tool_smooth,
    "Sets the terrain smooth tool.",
    "terrain.set_tool_smooth()");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(CTerrainModifyTool::Command_RiseLower, terrain, set_tool_riselower,
    "Sets the terrain rise/lower tool.",
    "terrain.set_tool_riselower()");

//////////////////////////////////////////////////////////////////////////

CTerrainBrush CTerrainModifyTool::m_brush[eBrushTypeLast];
BrushType           CTerrainModifyTool::m_currentBrushType = eBrushFlatten;
bool                    CTerrainModifyTool::m_bSyncBrushRadiuses = false;

//////////////////////////////////////////////////////////////////////////
CTerrainModifyTool::CTerrainModifyTool()
{
    m_pClassDesc = GetIEditor()->GetClassFactory()->FindClass(TERRAIN_MODIFY_TOOL_GUID);

    SetStatusText(tr("Modify Terrain Heightmap"));
    m_panelId = 0;
    m_panel = 0;
    m_prevBrush = 0;

    m_bSmoothOverride = false;
    m_bQueryHeightMode = false;
    m_nPaintingMode = ePaintMode_None;

    m_isCtrlPressed = false;
    m_isAltPressed = false;

    m_brush[eBrushFlatten].type = eBrushFlatten;
    m_brush[eBrushSmooth].type = eBrushSmooth;
    m_brush[eBrushRiseLower].type = eBrushRiseLower;
    m_brush[eBrushPickHeight].type = eBrushPickHeight;
    m_pBrush = &m_brush[m_currentBrushType];

    m_pointerPos(0, 0, 0);
    GetIEditor()->ClearSelection();

    m_brush[eBrushFlatten].heightRange.y = GetIEditor()->GetHeightmap()->GetMaxHeight();
    m_brush[eBrushRiseLower].heightRange.x = -50;
    m_brush[eBrushRiseLower].heightRange.y =  50;
    m_brush[eBrushPickHeight].heightRange.y = GetIEditor()->GetHeightmap()->GetMaxHeight();
    m_brush[eBrushPickHeight].radius = 0;
    m_brush[eBrushPickHeight].hardness = 0;

    m_hPickCursor = CMFCUtils::LoadCursor(IDC_POINTER_GET_HEIGHT);
    m_hPaintCursor = CMFCUtils::LoadCursor(IDC_HAND_INTERNAL);
    m_hFlattenCursor = CMFCUtils::LoadCursor(IDC_POINTER_FLATTEN);
    m_hSmoothCursor = CMFCUtils::LoadCursor(IDC_POINTER_SMOOTH);
}

//////////////////////////////////////////////////////////////////////////
CTerrainModifyTool::~CTerrainModifyTool()
{
    m_pointerPos(0, 0, 0);

    if (GetIEditor()->IsUndoRecording())
    {
        GetIEditor()->CancelUndo();
    }
}

//////////////////////////////////////////////////////////////////////////
void CTerrainModifyTool::BeginEditParams(IEditor* ie, int flags)
{
    if (!m_panelId && !m_panel)
    {
        // This looks eerily similar to MainFrm::InsertDisplayWidget and I'd have
        // preferred to use that, but we go through IEditor here and it was
        // different enough to make it impossible to use.
        m_panel = new CTerrainModifyPanel;
        m_panel->SetModifyTool(this);
        m_panelId = GetIEditor()->AddRollUpPage(ROLLUP_TERRAIN, "Modify Terrain", m_panel);
        MainWindow::instance()->setFocus();

        UpdateUI();
        GetIEditor()->UpdateViews(eUpdateHeightmap);
    }
}

//////////////////////////////////////////////////////////////////////////
void CTerrainModifyTool::EndEditParams()
{
    if (m_panelId)
    {
        GetIEditor()->RemoveRollUpPage(ROLLUP_TERRAIN, m_panelId);
        m_panel = 0;
    }
}

//////////////////////////////////////////////////////////////////////////
void CTerrainModifyTool::SetExternalUIPanel(class CTerrainModifyPanel* pPanel)
{
    assert(pPanel);
    EndEditParams();
    m_panel = pPanel;
    UpdateUI();
}

//////////////////////////////////////////////////////////////////////////
bool CTerrainModifyTool::MouseCallback(CViewport* view, EMouseEvent event, QPoint& point, int flags)
{
    m_bSmoothOverride = false;
    if (Qt::AltModifier & QApplication::queryKeyboardModifiers())
    {
        m_bSmoothOverride = true;
    }

    if (m_nPaintingMode == ePaintMode_InProgress)
    {
        m_nPaintingMode = ePaintMode_Ready;
    }

    m_bQueryHeightMode = false;

    if (flags & MK_CONTROL && !m_isCtrlPressed) // Can happen when not viewport window was in focus
    {
        if (m_pBrush->type == eBrushFlatten)
        {
            SetCurBrushType(eBrushPickHeight);
        }
        else if (m_pBrush->type == eBrushRiseLower)
        {
            m_pBrush->height = -m_pBrush->height;
        }
        m_isCtrlPressed = true;
    }

    bool bCollideWithTerrain = true;
    m_pointerPos = view->ViewToWorld(point, &bCollideWithTerrain, true);
    if ((event == eMouseLDown || (event == eMouseMove && (flags & MK_LBUTTON))) && m_pBrush->type == eBrushPickHeight)
    {
        m_pBrush->height = GetIEditor()->GetTerrainElevation(m_pointerPos.x, m_pointerPos.y);
        m_brush[eBrushFlatten].height = m_pBrush->height;

        char str[256];
        sprintf_s(str, "Picked height: %f\n", m_pBrush->height);
        OutputDebugString(str);

        UpdateUI();
    }
    else if (event == eMouseLDown && bCollideWithTerrain)
    {
        if (!GetIEditor()->IsUndoRecording())
        {
            GetIEditor()->BeginUndo();
        }
        Paint();
        m_nPaintingMode = ePaintMode_InProgress;
    }

    if (m_nPaintingMode == ePaintMode_Ready && event == eMouseMove && (flags & MK_LBUTTON) && !m_bQueryHeightMode && bCollideWithTerrain)
    {
        Paint();
        m_nPaintingMode = ePaintMode_InProgress;
    }

    if (event == eMouseMDown)
    {
        // When middle mouse button down.
        m_MButtonDownPos = point;
        m_prevRadius = m_pBrush->radius;
        m_prevRadiusInside = m_pBrush->radiusInside;
        m_prevHeight = m_pBrush->height;
    }

    if (event == eMouseMove && (flags & MK_MBUTTON) && (flags & MK_CONTROL) && m_nPaintingMode != ePaintMode_InProgress)
    {
        // Change brush height.
        QPoint p = point - m_MButtonDownPos;
        float dist = sqrtf(p.x() * p.x() + p.y() * p.y());
        m_pBrush->height = min(max(m_prevHeight - p.y() * 0.1f, m_pBrush->heightRange.x), m_pBrush->heightRange.y);
        if (m_pBrush->type == eBrushPickHeight)
        {
            m_brush[eBrushFlatten].height = m_pBrush->height;
        }
        UpdateUI();
    }

    // Don't invalidate the paint mode on mouse move, since they could be
    // painting and accidentally go outside the terrain boundry but then
    // come back without releasing the mouse
    if ((m_nPaintingMode == ePaintMode_Ready && event != eMouseMove) || event == eMouseLUp)
    {
        if (GetIEditor()->IsUndoRecording())
        {
            GetIEditor()->AcceptUndo("Terrain Modify");
        }
        m_nPaintingMode = ePaintMode_None;
    }

    // Show status help.
    if (m_pBrush->type == eBrushRiseLower)
    {
#ifdef AZ_PLATFORM_APPLE
        GetIEditor()->SetStatusText("⌘:Inverse Height  ⌥:Smooth  LMB:Rise/Lower/Smooth  [ ]:Change Brush Radius  <, >.:Change Height");
#else
        GetIEditor()->SetStatusText("CTRL:Inverse Height  ALT:Smooth  LMB:Rise/Lower/Smooth  [ ]:Change Brush Radius  <, >.:Change Height");
#endif
    }
    else
    {
#ifdef AZ_PLATFORM_APPLE
        GetIEditor()->SetStatusText("⌘:Query Height  ⌥:Smooth  LMB:Flatten/Smooth  [ ]:Change Brush Radius  <, >.:Change Height/Hardness");
#else
        GetIEditor()->SetStatusText("CTRL:Query Height  ALT:Smooth  LMB:Flatten/Smooth  [ ]:Change Brush Radius  <, >.:Change Height/Hardness");
#endif
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////
void CTerrainModifyTool::Display(DisplayContext& dc)
{
    CTerrainBrush* pBrush = m_pBrush;
    if (m_bSmoothOverride)
    {
        pBrush = &m_brush[eBrushSmooth];
    }

    if (dc.view)
    {
        // Check if mouse cursor is out of viewport
        QPoint cursor = QCursor::pos();
        dc.view->ScreenToClient(cursor);
        int width, height;
        dc.view->GetDimensions(&width, &height);
        if (cursor.x() < 0 || cursor.y() < 0 || cursor.x() > width || cursor.y() > height)
        {
            return;
        }
    }

    if (pBrush->type != eBrushSmooth)
    {
        dc.SetColor(0.5f, 1, 0.5f, 1);
        dc.DrawTerrainCircle(m_pointerPos, pBrush->radiusInside, 0.2f);
    }
    dc.SetColor(0, 1, 0, 1);
    dc.DrawTerrainCircle(m_pointerPos, pBrush->radius, 0.2f);
    if (m_pointerPos.z < pBrush->height)
    {
        if (pBrush->type != eBrushSmooth)
        {
            Vec3 p = m_pointerPos;
            p.z = pBrush->height;
            dc.SetColor(1, 1, 0, 1);
            if (pBrush->type == eBrushFlatten)
            {
                dc.DrawTerrainCircle(p, pBrush->radiusInside, pBrush->height - m_pointerPos.z);
            }
            dc.DrawLine(m_pointerPos, p);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
bool CTerrainModifyTool::OnKeyDown(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags)
{
    bool bProcessed = false;
    CTerrainBrush* pBrush = m_pBrush;
    BrushType eBrushType = m_pBrush->type;
    BrushType type = m_bSmoothOverride ? eBrushSmooth : eBrushNotSmooth;
    if (nChar == VK_OEM_PERIOD)
    {
        if (m_bSmoothOverride)
        {
            eBrushType = eBrushSmooth;
        }
        if (eBrushType == eBrushSmooth)
        {
            m_brush[eBrushType].hardness += 0.05f;
        }
        else
        {
            m_brush[eBrushType].height += 1;
        }
        bProcessed = true;
    }
    if (nChar == VK_OEM_COMMA)
    {
        if (m_bSmoothOverride)
        {
            eBrushType = eBrushSmooth;
        }
        if (eBrushType == eBrushSmooth)
        {
            if (m_brush[eBrushType].hardness > 0)
            {
                m_brush[eBrushType].hardness -= 0.05f;
            }
        }
        else
        {
            if (m_brush[eBrushType].height > 0)
            {
                m_brush[eBrushType].height -= 1;
            }
        }
        bProcessed = true;
    }

    if (nChar == VK_OEM_6 && Qt::ShiftModifier & QApplication::queryKeyboardModifiers())
    {
        pBrush->radiusInside += 1;
        bProcessed = true;
    }
    else if (nChar == VK_OEM_6)
    {
        pBrush->radius += 1;
        bProcessed = true;
    }

    if (nChar == VK_OEM_4 && Qt::ShiftModifier & QApplication::queryKeyboardModifiers())
    {
        if (pBrush->radiusInside > 1)
        {
            pBrush->radiusInside -= 1;
        }
        bProcessed = true;
    }
    else if (nChar == VK_OEM_4)
    {
        if (pBrush->radius > 1)
        {
            pBrush->radius -= 1;
        }
        bProcessed = true;
    }

    if (nChar == VK_MENU && !m_isAltPressed)
    {
        if (m_isAltPressed && m_pBrush->type == eBrushRiseLower)
        {
            m_pBrush->height = -m_pBrush->height;
        }
        m_isAltPressed = true;
        SetCurBrushType(eBrushSmooth);
        bProcessed = true;
    }

    if (nChar == VK_CONTROL && !m_isCtrlPressed)
    {
        if (m_pBrush->type == eBrushFlatten)
        {
            SetCurBrushType(eBrushPickHeight);
            bProcessed = true;
        }
        else if (m_pBrush->type == eBrushRiseLower)
        {
            m_pBrush->height = -m_pBrush->height;
            bProcessed = true;
        }
        m_isCtrlPressed = true;
    }
    if (bProcessed)
    {
        UpdateUI();
    }
    return bProcessed;
}

//////////////////////////////////////////////////////////////////////////
bool CTerrainModifyTool::OnKeyUp(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags)
{
    if (nChar == VK_MENU)
    {
        m_isAltPressed = false;
        if (m_prevBrush)
        {
            if (m_isAltPressed && m_prevBrush->type == eBrushRiseLower)
            {
                m_prevBrush->height = -m_prevBrush->height;
            }
            SetCurBrushType(m_prevBrush->type);
        }
        return true;
    }

    if (nChar == VK_CONTROL)
    {
        m_isCtrlPressed = false;
        if (m_pBrush->type == eBrushPickHeight && m_prevBrush && m_prevBrush->type != eBrushPickHeight)
        {
            SetCurBrushType(m_prevBrush->type);
            return true;
        }
        else if (m_pBrush->type == eBrushRiseLower)
        {
            m_pBrush->height = -m_pBrush->height;
            UpdateUI();
            return true;
        }
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
void CTerrainModifyTool::SetCurBrushParams(const CTerrainBrush& brush)
{
    *m_pBrush = brush;
    UpdateUI();
}


//////////////////////////////////////////////////////////////////////////
void CTerrainModifyTool::AdjustBrushValues()
{
    if (m_pBrush->type == eBrushSmooth)
    {
        m_pBrush->radiusInside = 0.0f;
        m_pBrush->height = 0.0f;
    }
    else if (m_pBrush->type == eBrushPickHeight)
    {
        m_pBrush->radius = 0.0f;
        m_pBrush->radiusInside = 0.0f;
        m_pBrush->hardness = 0.0f;
    }
}

//////////////////////////////////////////////////////////////////////////
void CTerrainModifyTool::SyncBrushRadiuses(bool bSync)
{
    m_bSyncBrushRadiuses = bSync;
    if (bSync && m_pBrush && m_pBrush->type != eBrushPickHeight)
    {
        for (int i = eBrushFlatten; i <= eBrushRiseLower; ++i)
        {
            m_brush[i].radius = m_pBrush->radius;
            if (m_brush[i].radiusInside > m_brush[i].radius)
            {
                m_brush[i].radiusInside = m_brush[i].radius;
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CTerrainModifyTool::UpdateUI()
{
    AdjustBrushValues();

    SyncBrushRadiuses(m_bSyncBrushRadiuses);

    if (m_panel)
    {
        m_panel->SetBrush(m_pBrush, m_bSyncBrushRadiuses);
    }
}

//////////////////////////////////////////////////////////////////////////
void CTerrainModifyTool::Paint()
{
    if (!GetIEditor()->Get3DEngine()->GetITerrain())
    {
        return;
    }

    CHeightmap* pHeightmap = GetIEditor()->GetHeightmap();

    CTerrainBrush* pBrush = m_pBrush;
    if (m_bSmoothOverride)
    {
        pBrush = &m_brush[eBrushSmooth];
    }

    int unitSize = pHeightmap->GetUnitSize();

    //dc.renderer->SetMaterialColor( 1,1,0,1 );
    int tx = RoundFloatToInt(m_pointerPos.y / unitSize);
    int ty = RoundFloatToInt(m_pointerPos.x / unitSize);

    float fInsideRadius = (pBrush->radiusInside / unitSize);
    int tsize = (pBrush->radius / unitSize);
    if (tsize == 0)
    {
        tsize = 1;
    }
    int tsize2 = tsize * 2;
    int x1 = tx - tsize;
    int y1 = ty - tsize;

    if (pBrush->type == eBrushFlatten && !m_bSmoothOverride)
    {
        pHeightmap->DrawSpot2(tx, ty, tsize, fInsideRadius, pBrush->height, pBrush->hardness, pBrush->bNoise, pBrush->noiseFreq / 10.0f, pBrush->noiseScale / 1000.0f);
    }
    if (pBrush->type == eBrushRiseLower && !m_bSmoothOverride)
    {
        pHeightmap->RiseLowerSpot(tx, ty, tsize, fInsideRadius, pBrush->height, pBrush->hardness, pBrush->bNoise, pBrush->noiseFreq / 10.0f, pBrush->noiseScale / 1000.0f);
    }
    else if (pBrush->type == eBrushSmooth || m_bSmoothOverride)
    {
        pHeightmap->SmoothSpot(tx, ty, tsize, pBrush->height, pBrush->hardness);//,m_pBrush->noiseFreq/10.0f,m_pBrush->noiseScale/10.0f );
    }
    pHeightmap->UpdateEngineTerrain(x1, y1, tsize2, tsize2, true, false);
    AABB box;
    box.min = m_pointerPos - Vec3(pBrush->radius, pBrush->radius, 0);
    box.max = m_pointerPos + Vec3(pBrush->radius, pBrush->radius, 0);
    box.min.z -= 10000;
    box.max.z += 10000;

    GetIEditor()->UpdateViews(eUpdateHeightmap, &box);

    if (pBrush->bRepositionVegetation && GetIEditor()->GetVegetationMap())
    {
        GetIEditor()->GetVegetationMap()->RepositionArea(box);
    }

    // Make sure objects preserve height.
    if (pBrush->bRepositionObjects)
    {
        GetIEditor()->GetObjectManager()->SendEvent(EVENT_KEEP_HEIGHT, box);
    }
}


//////////////////////////////////////////////////////////////////////////
bool CTerrainModifyTool::OnSetCursor(CViewport* vp)
{
    if (m_pBrush->type == eBrushPickHeight)
    {
        vp->SetCursor(m_hPickCursor);
        return true;
    }
    if ((m_pBrush->type == eBrushFlatten || m_pBrush->type == eBrushRiseLower) && !m_bSmoothOverride)
    {
        vp->SetCursor(m_hFlattenCursor);
        return true;
    }
    else if (m_pBrush->type == eBrushSmooth || m_bSmoothOverride)
    {
        vp->SetCursor(m_hSmoothCursor);
        return true;
    }
    return false;
}


//////////////////////////////////////////////////////////////////////////
void CTerrainModifyTool::SetCurBrushType(BrushType type)
{
    CTerrainBrush* prevPrevBrush = m_prevBrush;
    m_prevBrush = m_pBrush;
    m_currentBrushType = type;
    m_pBrush = &m_brush[type];

    if (m_pBrush->type == eBrushPickHeight && m_prevBrush->type != eBrushPickHeight)
    {
        m_pBrush->height = m_prevBrush->height;
    }

    UpdateUI();
}


//////////////////////////////////////////////////////////////////////////
void CTerrainModifyTool::Command_Activate()
{
    if (GetIEditor()->GetEditMode() == eEditModeSelectArea)
    {
        GetIEditor()->SetEditMode(eEditModeSelect);
    }

    CEditTool* pTool = GetIEditor()->GetEditTool();
    if (pTool && qobject_cast<CTerrainModifyTool*>(pTool))
    {
        // Already active.
        return;
    }

    GetIEditor()->SelectRollUpBar(ROLLUP_TERRAIN);

    // This needs to be done after the terrain tab is selected, because in
    // Cry-Free mode the terrain tool could be closed, whereas in legacy
    // mode the rollupbar is never deleted, it's only hidden
    pTool = new CTerrainModifyTool;
    GetIEditor()->SetEditTool(pTool);

    MainWindow::instance()->update();
}

//////////////////////////////////////////////////////////////////////////
void CTerrainModifyTool::Command_Flatten()
{
    Command_Activate();
    CEditTool* pTool = GetIEditor()->GetEditTool();
    if (pTool && qobject_cast<CTerrainModifyTool*>(pTool))
    {
        CTerrainModifyTool* pModTool = (CTerrainModifyTool*)pTool;
        pModTool->SetCurBrushType(eBrushFlatten);
    }
}

//////////////////////////////////////////////////////////////////////////
void CTerrainModifyTool::Command_Smooth()
{
    Command_Activate();
    CEditTool* pTool = GetIEditor()->GetEditTool();
    if (pTool && qobject_cast<CTerrainModifyTool*>(pTool))
    {
        CTerrainModifyTool* pModTool = (CTerrainModifyTool*)pTool;
        pModTool->SetCurBrushType(eBrushSmooth);
    }
}

//////////////////////////////////////////////////////////////////////////
void CTerrainModifyTool::Command_RiseLower()
{
    Command_Activate();
    CEditTool* pTool = GetIEditor()->GetEditTool();
    if (pTool && qobject_cast<CTerrainModifyTool*>(pTool))
    {
        CTerrainModifyTool* pModTool = (CTerrainModifyTool*)pTool;
        pModTool->SetCurBrushType(eBrushRiseLower);
    }
}

const GUID& CTerrainModifyTool::GetClassID()
{
    return TERRAIN_MODIFY_TOOL_GUID;
}

//////////////////////////////////////////////////////////////////////////
void CTerrainModifyTool::RegisterTool(CRegistrationContext& rc)
{
    rc.pClassFactory->RegisterClass(new CQtViewClass<CTerrainModifyTool>("EditTool.TerrainModify", "Terrain", ESYSTEM_CLASS_EDITTOOL));

    CommandManagerHelper::RegisterCommand(rc.pCommandManager, "edit_tool", "terrain_activate", "", "", functor(CTerrainModifyTool::Command_Activate));
    CommandManagerHelper::RegisterCommand(rc.pCommandManager, "edit_tool", "terrain_flatten", "", "", functor(CTerrainModifyTool::Command_Flatten));
    CommandManagerHelper::RegisterCommand(rc.pCommandManager, "edit_tool", "terrain_smooth", "", "", functor(CTerrainModifyTool::Command_Smooth));
    CommandManagerHelper::RegisterCommand(rc.pCommandManager, "edit_tool", "terrain_rise_lower", "", "", functor(CTerrainModifyTool::Command_RiseLower));
}

#include <TerrainModifyTool.moc>