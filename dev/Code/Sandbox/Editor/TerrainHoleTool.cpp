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

#include "ViewManager.h"
#include "Viewport.h"
#include "TerrainHolePanel.h"
#include "Terrain/Heightmap.h"
#include "Objects/DisplayContext.h"
#include "MainWindow.h"

#include "I3DEngine.h"
#include "ITerrain.h"

#include "Util/BoostPythonHelpers.h"

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
    UpdatePointerPos(view, point);

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
AZStd::pair<Vec2i, Vec2i> CTerrainHoleTool::Modify()
{
    if (!GetIEditor()->Get3DEngine()->GetITerrain())
    {
        return {};
    }

    CHeightmap* pHeightmap = GetIEditor()->GetHeightmap();
    if (!pHeightmap)
    {
        return {};
    }

    Vec2i min;
    Vec2i max;

    CalculateHoleArea(min, max);

    if ((max.x - min.x) >= 0 && (max.y - min.y) >= 0)
    {
        pHeightmap->MakeHole(min.x, min.y, max.x - min.x, max.y - min.y, m_bMakeHole);
        return AZStd::make_pair(min, max);
    }
    else
    {
        return {};
    }
}

//////////////////////////////////////////////////////////////////////////
bool CTerrainHoleTool::UpdatePointerPos(CViewport* view, const QPoint& pos)
{
    bool bHitTerrain(false);
    Vec3 stPointerPos;

    stPointerPos = view->ViewToWorld(pos, &bHitTerrain, true);

    // When there is no terrain, the returned position would be 0,0,0 causing
    // users to often delete the terrain under this position unintentionally.
    if (bHitTerrain)
    {
        m_pointerPos = stPointerPos;
    }

    return bHitTerrain;
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
class ScriptingBindings
{
public:
    //////////////////////////////////////////////////////////////////////////
    static void PySetBrushRadius(float radius)
    {
        if (CTerrainHoleTool* holeTool = GetTool())
        {
            holeTool->SetBrushRadius(radius);
            RefreshUI(*holeTool);
        }
    }

    //////////////////////////////////////////////////////////////////////////
    static float PyGetBrushRadius()
    {
        return CTerrainHoleTool::m_brushRadius;
    }

    //////////////////////////////////////////////////////////////////////////
    static void PySetMakeHole(bool bEnable)
    {
        CTerrainHoleTool::m_bMakeHole = bEnable;

        if (CTerrainHoleTool* holeTool = GetTool())
        {
            RefreshUI(*holeTool);
        }
    }

    //////////////////////////////////////////////////////////////////////////
    static bool PyGetMakeHole()
    {
        return CTerrainHoleTool::m_bMakeHole;
    }

    //////////////////////////////////////////////////////////////////////////
    static boost::python::tuple PyPaintHole(float x, float y)
    {
        if (CTerrainHoleTool* holeTool = GetTool())
        {
            if (holeTool->UpdatePointerPos(GetIEditor()->GetViewManager()->GetGameViewport(), QPoint(x, y)))
            {
                AZStd::pair<Vec2i, Vec2i> modifiedArea = holeTool->Modify();

                if (!modifiedArea.first.IsZero() || !modifiedArea.second.IsZero())
                {
                    return boost::python::make_tuple(modifiedArea.first.x, modifiedArea.first.y, modifiedArea.second.x, modifiedArea.second.y);
                }
            }
        }

        return {};
    }
    
    //////////////////////////////////////////////////////////////////////////
    static void PyPaintHoleDirect(int x1, int y1, int x2, int y2)
    {
        IEditor* editor = GetIEditor();
        if (!editor)
        {
            return;
        }

        CHeightmap* heightmap = editor->GetHeightmap();
        if (!heightmap)
        {
            return;
        }

        heightmap->MakeHole(x1, y1, x2, y2, CTerrainHoleTool::m_bMakeHole);
    }

    //////////////////////////////////////////////////////////////////////////
    static bool PyIsHoleAt(int x, int y)
    {
        IEditor* editor = GetIEditor();
        if (!editor)
        {
            return false;
        }

        CHeightmap* heightmap = editor->GetHeightmap();
        if (!heightmap)
        {
            return false;
        }

        return heightmap->IsHoleAt(x, y);
    }

private:
    //////////////////////////////////////////////////////////////////////////
    static void RefreshUI(CTerrainHoleTool& tool)
    {
        if (tool.m_panelId)
        {
            tool.m_panel->SetMakeHole(tool.m_bMakeHole);
            tool.m_panel->SetRadius();

            IEditor* editor = GetIEditor();
            AZ_Assert(editor, "Editor instance doesn't exist!");
            if (editor)
            {
                editor->Notify(eNotify_OnInvalidateControls);
            }
        }
    }

    //////////////////////////////////////////////////////////////////////////
    static CTerrainHoleTool* GetTool()
    {
        IEditor* editor = GetIEditor();
        AZ_Assert(editor, "Editor instance doesn't exist!");

        if (!editor)
        {
            return nullptr;
        }

        CEditTool* pTool = editor->GetEditTool();

        if (!pTool || !qobject_cast<CTerrainHoleTool*>(pTool))
        {
            editor->SelectRollUpBar(ROLLUP_TERRAIN);

            // This needs to be done after the terrain tab is selected, because in
            // Cry-Free mode the terrain tool could be closed, whereas in legacy
            // mode the rollupbar is never deleted, it's only hidden
            pTool = new CTerrainHoleTool();
            editor->SetEditTool(pTool);

            MainWindow::instance()->update();
        }

        return reinterpret_cast<CTerrainHoleTool*>(pTool);
    }
};

//////////////////////////////////////////////////////////////////////////

REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(ScriptingBindings::PySetBrushRadius, terrain, set_brush_radius,
    "Sets the brush radius of the hole tool.", "terrain.set_brush_radius(float radius)");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(ScriptingBindings::PyGetBrushRadius, terrain, get_brush_radius,
    "Gets the brush radius of the hole tool.", "float terrain.get_brush_radius()");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(ScriptingBindings::PySetMakeHole, terrain, set_make_hole,
    "Sets making holes to enabled or disabled. Disabling will remove holes.", "void terrain.set_make_hole(bool make_hole)");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(ScriptingBindings::PyGetMakeHole, terrain, get_make_hole,
    "Returns whether or not making holes is enabled.", "bool terrain.get_make_hole()");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(ScriptingBindings::PyPaintHole, terrain, paint_hole,
    "Paints or removes holes from the terrain based on flags set. X and Y are viewport coordinates.",
    "tuple terrain.paint_hole(float x, float y)");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(ScriptingBindings::PyPaintHoleDirect, terrain, paint_hole_direct,
    "Paints or removes holes directly on the heightmap based on flags set.",
    "void terrain.paint_hole_direct(int x1, int y1, int x2, int y2)");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(ScriptingBindings::PyIsHoleAt, terrain, is_hole_at,
    "Returns whether or not a hole exists at point X, Y. X and Y are heightmap coordinates.",
    "bool terrain.is_hole_at(int x, int y)");

#include <TerrainHoleTool.moc>
