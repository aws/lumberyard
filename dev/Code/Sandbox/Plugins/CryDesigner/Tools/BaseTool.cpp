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
#include "Tools/BaseTool.h"
#include "Tools/DesignerTool.h"
#include "ViewManager.h"
#include "Core/Model.h"
#include "Objects/DesignerObject.h"
#include "Core/ModelCompiler.h"
#include "Tools/Select/SelectTool.h"
#include "Core/BrushHelper.h"
#include "Core/SmoothingGroupManager.h"
#include "ToolFactory.h"
#include "Serialization/Enum.h"

using namespace CD;

const char* BaseTool::ToolClass() const
{
    return Serialization::getEnumDescription<CD::EDesignerTool>().label(Tool());
}

const char* BaseTool::ToolName() const
{
    return Serialization::getEnumDescription<CD::EDesignerTool>().name(Tool());
}

BrushMatrix34 BaseTool::GetWorldTM() const
{
    CBaseObject* pBaseObj = GetBaseObject();
    DESIGNER_ASSERT(pBaseObj);
    return pBaseObj->GetWorldTM();
}

void BaseTool::Enter()
{
    CD::GetDesigner()->ReleaseObjectGizmo();
    BeginEditParams();
    if (GetBaseObject())
    {
        GetBaseObject()->SetHighlight(false);
    }
}

void BaseTool::Leave()
{
    if (GetModel())
    {
        GetModel()->ClearExcludedEdgesInDrawing();
    }
    CD::GetDesigner()->ReleaseSelectionMesh();
    EndEditParams();
}

void BaseTool::BeginEditParams()
{
    if (!m_pPanel)
    {
        m_pPanel = (IBasePanel*)uiPanelFactory::the().Create(Tool(), this);
    }
}

void BaseTool::EndEditParams()
{
    DestroyPanel(&m_pPanel);
}

void BaseTool::OnMouseMove(CViewport* view, UINT nFlags, const QPoint& point)
{
    BrushVec3 localRaySrc, localRayDir;
    GetLocalViewRay(GetBaseObject()->GetWorldTM(), view, point, localRaySrc, localRayDir);

    int nPolygonIndex(0);
    if (GetModel()->QueryPolygon(localRaySrc, localRayDir, nPolygonIndex))
    {
        PolygonPtr pPolygon = GetModel()->GetPolygon(nPolygonIndex);
        CD::GetDesigner()->UpdateSelectionMesh(pPolygon, GetCompiler(), GetBaseObject());
        m_Plane = pPolygon->GetPlane();
    }
    else
    {
        CD::GetDesigner()->UpdateSelectionMesh(NULL, GetCompiler(), GetBaseObject());
    }
}

CD::ModelCompiler* BaseTool::GetCompiler() const
{
    DesignerTool* pEditTool = CD::GetDesigner();
    if (pEditTool == NULL)
    {
        return NULL;
    }
    return pEditTool->GetCompiler();
}

CBaseObject* BaseTool::GetBaseObject() const
{
    DesignerTool* pEditTool = CD::GetDesigner();
    if (pEditTool == NULL)
    {
        return NULL;
    }
    return pEditTool->GetBaseObject();
}

SMainContext BaseTool::GetMainContext() const
{
    SMainContext mc;
    mc.pObject = GetBaseObject();
    mc.pCompiler = GetCompiler();
    mc.pModel = GetModel();
    mc.pSelected = CD::GetDesigner()->GetSelectedElements();
    return mc;
}

void BaseTool::UpdateAll(int updateType)
{
    CD::UpdateAll(GetMainContext(), updateType);
}

void BaseTool::UpdateShelf(int nShelf)
{
    if (!GetCompiler())
    {
        return;
    }
    GetCompiler()->Compile(GetBaseObject(), GetModel(), nShelf);
}

Model* BaseTool::GetModel() const
{
    DesignerTool* pEditTool = CD::GetDesigner();
    if (pEditTool == NULL)
    {
        return NULL;
    }
    return pEditTool->GetModel();
}

void BaseTool::Display(DisplayContext& dc)
{
    if (dc.flags & DISPLAY_2D)
    {
        return;
    }
    DisplayDimensionHelper(dc);
}

bool BaseTool::OnKeyDown(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags)
{
    if (nChar == VK_ESCAPE)
    {
        return CD::GetDesigner()->EndCurrentEditSession();
    }
    return true;
}

void BaseTool::DisplayDimensionHelper(DisplayContext& dc, const AABB& aabb)
{
    if (gDesignerSettings.bDisplayDimensionHelper && GetBaseObject())
    {
        Matrix34 poppedTM = dc.GetMatrix();
        dc.PopMatrix();
        GetBaseObject()->DrawDimensionsImpl(dc, aabb);
        dc.PushMatrix(poppedTM);
    }
}

void BaseTool::DisplayDimensionHelper(DisplayContext& dc, int nShelf)
{
    AABB aabb = GetModel()->GetBoundBox(nShelf);
    if (!aabb.IsReset())
    {
        if (gDesignerSettings.bDisplayDimensionHelper)
        {
            DisplayDimensionHelper(dc, aabb);
        }
    }
}

bool BaseTool::IsModelEmpty() const
{
    return GetModel() && GetModel()->IsEmpty(0) ? true : false;
}

QString BaseTool::GetStatusText() const
{
    int nCount = GetIEditor()->GetSelection()->GetCount();
    if (nCount > 0)
    {
        return QObject::tr("%1 Object(s) Selected").arg(nCount);
    }
    else
    {
        return QObject::tr("No Selection");
    }
}
