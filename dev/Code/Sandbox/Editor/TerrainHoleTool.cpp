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
#include "TerrainHoleTool.h"
#include "Viewport.h"
#include "TerrainHolePanel.h"
#include "Terrain/Heightmap.h"
#include "Objects/DisplayContext.h"
#include "MainWindow.h"

#include "I3DEngine.h"
#include "ITerrain.h"

float CTerrainHoleTool::m_brushRadius = 0.5;
bool CTerrainHoleTool::m_bMakeHole = true;

//////////////////////////////////////////////////////////////////////////
CTerrainHoleTool::CTerrainHoleTool()
{
    m_panelId = 0;
    m_panel = 0;

    m_pointerPos(0, 0, 0);

    GetIEditor()->ClearSelection();
}

//////////////////////////////////////////////////////////////////////////
CTerrainHoleTool::~CTerrainHoleTool()
{
    m_pointerPos(0, 0, 0);
    if (GetIEditor()->IsUndoRecording())
    {
        GetIEditor()->CancelUndo();
    }
}

//////////////////////////////////////////////////////////////////////////
void CTerrainHoleTool::BeginEditParams(IEditor* ie, int flags)
{
    if (!m_panelId)
    {
        m_panel = new CTerrainHolePanel(this);
        m_panelId = GetIEditor()->AddRollUpPage(ROLLUP_TERRAIN, "Modify Terrain", m_panel);
        MainWindow::instance()->setFocus();
    }
}

//////////////////////////////////////////////////////////////////////////
void CTerrainHoleTool::EndEditParams()
{
    if (m_panelId)
    {
        GetIEditor()->RemoveRollUpPage(ROLLUP_TERRAIN, m_panelId);
        m_panelId = 0;
        m_panel = 0;
    }
}

//////////////////////////////////////////////////////////////////////////
bool CTerrainHoleTool::MouseCallback(CViewport* view, EMouseEvent event, QPoint& point, int flags)
{
    bool bHitTerrain(false);
    Vec3 stPointerPos;

    stPointerPos = view->ViewToWorld(point, &bHitTerrain, true);

    // When there is no terrain, the returned position would be 0,0,0 causing
    // users to often delete the terrain under this position unintentionally.
    if (bHitTerrain)
    {
        m_pointerPos = stPointerPos;
    }

    if (event == eMouseLDown || (event == eMouseMove && (flags & MK_LBUTTON)))
    {
        AzToolsFramework::UndoSystem::URSequencePoint* undoOperation = nullptr;
        AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(
            undoOperation, &AzToolsFramework::ToolsApplicationRequests::GetCurrentUndoBatch);

        if (!undoOperation)
        {
            AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(
                undoOperation, &AzToolsFramework::ToolsApplicationRequests::BeginUndoBatch, "Modify Terrain");
        }
        Modify();
    }
    else
    {
        AzToolsFramework::UndoSystem::URSequencePoint* undoOperation = nullptr;
        AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(
            undoOperation, &AzToolsFramework::ToolsApplicationRequests::GetCurrentUndoBatch);
        if (undoOperation)
        {
            AzToolsFramework::ToolsApplicationRequests::Bus::Broadcast(&AzToolsFramework::ToolsApplicationRequests::EndUndoBatch);
        }
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////
void CTerrainHoleTool::Display(DisplayContext& dc)
{
    if (dc.flags & DISPLAY_2D)
    {
        return;
    }

    CHeightmap* pHeightmap = GetIEditor()->GetHeightmap();
    if (!pHeightmap)
    {
        return;
    }

    int unitSize = pHeightmap->GetUnitSize();

    dc.SetColor(0, 1, 0, 1);
    dc.DrawTerrainCircle(m_pointerPos, m_brushRadius, 0.2f);

    Vec2i min;
    Vec2i max;

    CalculateHoleArea(min, max);

    if (m_bMakeHole)
    {
        dc.SetColor(1, 0, 0, 1);
    }
    else
    {
        dc.SetColor(0, 0, 1, 1);
    }

    dc.DrawTerrainRect(min.y * unitSize, min.x * unitSize, max.y * unitSize + unitSize, max.x * unitSize + unitSize, 0.2f);
}

//////////////////////////////////////////////////////////////////////////
bool CTerrainHoleTool::OnKeyDown(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags)
{
    bool bProcessed = false;
    if (nChar == VK_OEM_6)
    {
        if (m_brushRadius < 100)
        {
            m_brushRadius += 0.5f;
        }
        bProcessed = true;
    }
    if (nChar == VK_OEM_4)
    {
        if (m_brushRadius > 0.5f)
        {
            m_brushRadius -= 0.5f;
        }
        bProcessed = true;
    }

    if (nChar == VK_CONTROL && !(nFlags & (1 << 14))) // only once (no repeat).
    {
        m_bMakeHole = !m_bMakeHole;
        m_panel->SetMakeHole(m_bMakeHole);
    }

    if (bProcessed && m_panel)
    {
        m_panel->SetRadius();
    }
    return bProcessed;
}

bool CTerrainHoleTool::OnKeyUp(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags)
{
    if (nChar == VK_CONTROL)
    {
        m_bMakeHole = !m_bMakeHole;
        m_panel->SetMakeHole(m_bMakeHole);
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
void CTerrainHoleTool::Modify()
{
    if (!GetIEditor()->Get3DEngine()->GetITerrain())
    {
        return;
    }

    CHeightmap* pHeightmap = GetIEditor()->GetHeightmap();
    if (!pHeightmap)
    {
        return;
    }

    Vec2i min;
    Vec2i max;

    CalculateHoleArea(min, max);

    if (
        (max.x - min.x) >= 0
        &&
        (max.y - min.y) >= 0
        )
    {
        pHeightmap->MakeHole(min.x, min.y, max.x - min.x, max.y - min.y, m_bMakeHole);
    }
}
//////////////////////////////////////////////////////////////////////////
bool CTerrainHoleTool::CalculateHoleArea(Vec2i& min, Vec2i& max) const
{
    CHeightmap* pHeightmap = GetIEditor()->GetHeightmap();
    if (!pHeightmap)
    {
        return false;
    }

    int unitSize = pHeightmap->GetUnitSize();

    float fx1 = (m_pointerPos.y - (float)m_brushRadius) / (float)unitSize;
    float fy1 = (m_pointerPos.x - (float)m_brushRadius) / (float)unitSize;
    float fx2 = (m_pointerPos.y + (float)m_brushRadius) / (float)unitSize;
    float fy2 = (m_pointerPos.x + (float)m_brushRadius) / (float)unitSize;

    // As the engine interfaces take ints, this loss of precision if expected
    // and needed to correctly predict.
    min.x = MAX(fx1, 0);
    min.y = MAX(fy1, 0);
    max.x = MIN(fx2, pHeightmap->GetWidth() - 1.0f);
    max.y = MIN(fy2, pHeightmap->GetHeight() - 1.0f);

    return true;
}
//////////////////////////////////////////////////////////////////////////

#include <TerrainHoleTool.moc>