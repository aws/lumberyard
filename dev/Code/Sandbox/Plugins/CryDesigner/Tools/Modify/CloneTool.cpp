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
#include "CloneTool.h"
#include "SurfaceInfoPicker.h"
#include "Prefabs/PrefabItem.h"
#include "Prefabs/PrefabManager.h"
#include "Objects/PrefabObject.h"
#include "ViewManager.h"
#include "Tools/DesignerTool.h"
#include "ToolFactory.h"
#include "Serialization/Enum.h"
#include "Serialization/Decorators/EditorActionButton.h"

using namespace CD;

SERIALIZATION_ENUM_BEGIN(EPlacementType, "PlacementType")
SERIALIZATION_ENUM_VALUE(ePlacementType_Divide, "Divide")
SERIALIZATION_ENUM_VALUE(ePlacementType_Multiply, "Multiply")
SERIALIZATION_ENUM_END()

namespace CD
{
    void SCloneParameter::Serialize(Serialization::IArchive& ar)
    {
        ar(Serialization::Range(m_NumberOfClones, 1, 64), "CloneCount", "Clone Count");
        if (m_PlacementType != CD::ePlacementType_None)
        {
            ar(m_PlacementType, "PlacementType", "Placement Type");
        }
    }
}

CloneTool::CloneTool(CD::EDesignerTool tool)
    : BaseTool(tool)
    , m_bSuspendedUndo(false)
{
    if (Tool() == CD::eDesigner_CircleClone)
    {
        m_CloneParameter.m_PlacementType = CD::ePlacementType_None;
    }
    else
    {
        m_CloneParameter.m_PlacementType = CD::ePlacementType_Divide;
    }
}

const char* CloneTool::ToolClass() const
{
    return Serialization::getEnumDescription<CD::EDesignerTool>().label(CD::eDesigner_CircleClone);
}

void CloneTool::Enter()
{
    BaseTool::Enter();

    m_vStartPos = Vec3(0, 0, 0);
    m_SelectedClone = (CD::Clone*)GetBaseObject();
    m_InitObjectWorldPos = m_SelectedClone->GetWorldPos();

    AABB boundbox;
    m_SelectedClone->GetBoundBox(boundbox);

    m_vStartPos = GetCenterBottom(boundbox);
    m_vPickedPos = m_vStartPos;

    m_fRadius = 0.1f;

    if (!m_bSuspendedUndo)
    {
        GetIEditor()->SuspendUndo();
        m_bSuspendedUndo = true;
    }

    if (GetModel()->IsEmpty(0))
    {
        CD::GetDesigner()->SwitchTool(CD::eDesigner_Object);
    }
}

void CloneTool::Leave()
{
    BaseTool::Leave();

    if (m_bSuspendedUndo)
    {
        GetIEditor()->ResumeUndo();
        m_bSuspendedUndo = false;
    }

    if (m_SelectedClone)
    {
        m_SelectedClone->SetWorldPos(m_InitObjectWorldPos);
    }

    m_Clones.clear();

    CD::GetDesigner()->GetDesignerPanel()->UpdateCloneArrayButtons();
}

void CloneTool::Serialize(Serialization::IArchive& ar)
{
    ar(Serialization::ActionButton([this] {Confirm();
            }), "Confirm", "Confirm");
    m_CloneParameter.Serialize(ar);
}

void CloneTool::OnLButtonDown(CViewport* view, UINT nFlags, const QPoint& point)
{
    UpdateCloneList();
    m_vPickedPos = m_vStartPos;
    m_Plane = BrushPlane(m_vStartPos, m_vStartPos + BrushVec3(0, 1, 0), m_vStartPos + BrushVec3(1, 0, 0));
}

void CloneTool::OnMouseMove(CViewport* view, UINT nFlags, const QPoint& point)
{
    if (nFlags & MK_LBUTTON)
    {
        if (Tool() == CD::eDesigner_ArrayClone)
        {
            CSurfaceInfoPicker picker;
            SRayHitInfo hitInfo;
            int nCloneCount = m_Clones.size();
            DESIGNER_ASSERT(nCloneCount > 0);
            if (nCloneCount > 0)
            {
                CSurfaceInfoPicker::CExcludedObjects excludedObjList;
                excludedObjList.Add(m_SelectedClone);
                for (int i = 0; i < nCloneCount; ++i)
                {
                    excludedObjList.Add(m_Clones[i]);
                }

                if (picker.Pick(point, hitInfo, &excludedObjList))
                {
                    m_vPickedPos = hitInfo.vHitPos;
                    UpdateClonePositions();
                }
            }
        }
        else if (Tool() == CD::eDesigner_CircleClone)
        {
            Vec3 vWorldRaySrc;
            Vec3 vWorldRayDir;
            GetIEditor()->GetActiveView()->ViewToWorldRay(point, vWorldRaySrc, vWorldRayDir);
            BrushVec3 vOut;
            if (m_Plane.HitTest(vWorldRaySrc, vWorldRaySrc + vWorldRayDir, NULL, &vOut))
            {
                m_vPickedPos = ToVec3(vOut);
                UpdateClonePositions();
            }
        }
    }
}

void CloneTool::Display(struct DisplayContext& dc)
{
    dc.SetColor(ColorB(100, 200, 100, 255));

    Matrix34 tm = dc.GetMatrix();
    dc.PopMatrix();

    if (Tool() == CD::eDesigner_CircleClone)
    {
        dc.DrawCircle(m_vStartPos, m_fRadius);
        dc.DrawLine(m_vStartPos, Vec3(m_vPickedPos.x, m_vPickedPos.y, m_vStartPos.z));
    }
    else
    {
        dc.DrawLine(m_vStartPos, m_vPickedPos);
    }

    dc.PushMatrix(tm);
}

void CloneTool::SetPivotToObject(CBaseObject* pObj, const Vec3& pos)
{
    AABB localBB;
    pObj->GetLocalBounds(localBB);
    Vec3 vLocalCenterBottom = GetCenterBottom(localBB);
    pObj->SetPos(pos - vLocalCenterBottom);
}

Vec3 CloneTool::GetCenterBottom(const AABB& aabb) const
{
    Vec3 vCenterBottom;
    vCenterBottom.x = (aabb.min.x + aabb.max.x) * 0.5f;
    vCenterBottom.y = (aabb.min.y + aabb.max.y) * 0.5f;
    vCenterBottom.z = aabb.min.z;
    return vCenterBottom;
}

void CloneTool::OnChangeParameter(bool continuous)
{
    UpdateCloneList();
    UpdateClonePositions();
}

void CloneTool::UpdateCloneList()
{
    if (!m_SelectedClone || m_Clones.size() == m_CloneParameter.m_NumberOfClones)
    {
        return;
    }

    m_Clones.clear();
    m_Clones.reserve(m_CloneParameter.m_NumberOfClones);

    for (int i = 0; i < m_CloneParameter.m_NumberOfClones; ++i)
    {
        CD::Clone* pClone = new CD::Clone;
        pClone->SetModel(new CD::Model(*m_SelectedClone->GetModel()));
        pClone->SetMaterial(m_SelectedClone->GetMaterial());
        pClone->GetCompiler()->Compile(pClone, pClone->GetModel());
        m_Clones.push_back(pClone);
    }

    GetIEditor()->GetViewManager()->UpdateViews();
}

void CloneTool::UpdateClonePositions()
{
    if (Tool() == CD::eDesigner_ArrayClone)
    {
        UpdateClonePositionsAlongLine();
    }
    else
    {
        UpdateClonePositionsAlongCircle();
    }
}

void CloneTool::UpdateClonePositionsAlongLine()
{
    int nCloneCount = m_Clones.size();
    DESIGNER_ASSERT(nCloneCount > 0);
    if (nCloneCount <= 0)
    {
        return;
    }
    Vec3 vDelta = m_CloneParameter.m_PlacementType == ePlacementType_Divide ? (m_vPickedPos - m_vStartPos) / nCloneCount : (m_vPickedPos - m_vStartPos);
    for (int i = 1; i <= nCloneCount; ++i)
    {
        SetPivotToObject(m_Clones[i - 1], m_vStartPos + vDelta * i);
    }
    SetPivotToObject(m_SelectedClone, m_vStartPos);
}

void CloneTool::UpdateClonePositionsAlongCircle()
{
    int nCloneCount = m_Clones.size();
    DESIGNER_ASSERT(nCloneCount > 0);
    if (nCloneCount <= 0)
    {
        return;
    }
    m_fRadius = (Vec2(m_vStartPos) - Vec2(m_vPickedPos)).GetLength();
    if (m_fRadius < 0.1f)
    {
        m_fRadius = 0.1f;
    }

    BrushVec2 vCenterOnPlane = m_Plane.W2P(ToBrushVec3(m_vStartPos));
    BrushVec2 vPickedPosOnPlane = m_Plane.W2P(ToBrushVec3(m_vPickedPos));
    float fAngle = CD::ComputeAnglePointedByPos(vCenterOnPlane, vPickedPosOnPlane);

    int nCloneCountInPanel = m_CloneParameter.m_NumberOfClones;
    std::vector<BrushVec2> vertices2D;
    CD::MakeSectorOfCircle(m_fRadius, m_Plane.W2P(ToBrushVec3(m_vStartPos)), fAngle, CD::PI2, nCloneCountInPanel + 1, vertices2D);

    for (int i = 0; i < nCloneCountInPanel; ++i)
    {
        if (i == 0)
        {
            SetPivotToObject(m_SelectedClone, ToVec3(m_Plane.P2W(vertices2D[i])));
        }
        else
        {
            SetPivotToObject(m_Clones[i - 1], ToVec3(m_Plane.P2W(vertices2D[i])));
        }
    }
}

void CloneTool::Confirm()
{
    if (m_bSuspendedUndo)
    {
        GetIEditor()->ResumeUndo();
        m_bSuspendedUndo = false;
    }
    FreezeClones();
    m_SelectedClone = NULL;
    CD::GetDesigner()->SwitchTool(CD::eDesigner_Object);
}

void CloneTool::FreezeClones()
{
    if (!m_SelectedClone || m_Clones.empty())
    {
        return;
    }

    CPrefabManager* pPrefabManager = GetIEditor()->GetPrefabManager();
    if (!pPrefabManager)
    {
        return;
    }

    IDataBaseLibrary* pLibrary = pPrefabManager->FindLibrary("Level");
    if (!pLibrary)
    {
        return;
    }

    CUndo undo("Clone Designer Object");

    CSelectionGroup selectionGroup;
    selectionGroup.AddObject(m_SelectedClone);

    CD::GetDesigner()->SetBaseObject(NULL);

    CPrefabItem* pPrefabItem = (CPrefabItem*)GetIEditor()->GetPrefabManager()->CreateItem(pLibrary);
    pPrefabItem->MakeFromSelection(selectionGroup);

    CSelectionGroup* pSelection = GetIEditor()->GetObjectManager()->GetSelection();
    if (pSelection && !pSelection->IsEmpty())
    {
        CBaseObject* pSelected = pSelection->GetObject(0);
        if (qobject_cast<CPrefabObject*>(pSelected))
        {
            CPrefabObject* pPrefabObj = ((CPrefabObject*)pSelected);
            pPrefabObj->Open();
            if (pPrefabObj->GetChildCount() > 0)
            {
                CD::GetDesigner()->SetBaseObject(pPrefabObj->GetChild(0));
            }
        }
    }

    for (int i = 0, iCloneCount(m_Clones.size()); i < iCloneCount; ++i)
    {
        CPrefabObject* pDesignerPrefabObj = (CPrefabObject*)GetIEditor()->GetObjectManager()->NewObject(PREFAB_OBJECT_CLASS_NAME);
        pDesignerPrefabObj->SetPrefab(pPrefabItem, true);

        AABB aabb;
        m_Clones[i]->GetLocalBounds(aabb);

        pDesignerPrefabObj->SetWorldPos(m_Clones[i]->GetWorldPos() + aabb.min);
        pDesignerPrefabObj->Open();
    }

    m_Clones.clear();
}

#include "UIs/PropertyTreePanel.h"
REGISTER_DESIGNER_TOOL_WITH_PANEL(CD::eDesigner_CircleClone, CD::eToolGroup_Modify, "Circle Clone", CloneTool, PropertyTreePanel<CloneTool>)
REGISTER_DESIGNER_TOOL_WITH_PANEL(CD::eDesigner_ArrayClone, CD::eToolGroup_Modify, "Array Clone", ArrayCloneTool, PropertyTreePanel<ArrayCloneTool>)