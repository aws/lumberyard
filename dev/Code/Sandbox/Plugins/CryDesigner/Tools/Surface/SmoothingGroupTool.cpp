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
#include "SmoothingGroupTool.h"
#include "Tools/DesignerTool.h"
#include "Core/Model.h"
#include "Core/SmoothingGroupManager.h"
#include "ToolFactory.h"

namespace
{
    ISmoothingGroupPanel* g_pSmoothingGroupPanel = NULL;
}

void SmoothingGroupTool::Enter()
{
    SelectTool::Enter();

    ElementManager* pSelected = CD::GetDesigner()->GetSelectedElements();
    pSelected->Erase(CD::ePF_Vertex | CD::ePF_Edge);
}

void SmoothingGroupTool::BeginEditParams()
{
    if (!g_pSmoothingGroupPanel)
    {
        g_pSmoothingGroupPanel = (ISmoothingGroupPanel*)uiPanelFactory::the().Create(Tool(), this);
    }
}

void SmoothingGroupTool::EndEditParams()
{
    CD::DestroyPanel(&g_pSmoothingGroupPanel);
}

string SmoothingGroupTool::SetSmoothingGroup(const char* id_name)
{
    ElementManager* pSelected = CD::GetDesigner()->GetSelectedElements();
    if (pSelected->IsEmpty())
    {
        return "";
    }

    std::vector<CD::PolygonPtr> polygons;
    for (int i = 0, iElementCount(pSelected->GetCount()); i < iElementCount; ++i)
    {
        if (!(*pSelected)[i].IsFace() || !(*pSelected)[i].m_pPolygon)
        {
            continue;
        }
        polygons.push_back((*pSelected)[i].m_pPolygon);
    }

    SmoothingGroupManager* pSmoothingGroupMgr = GetModel()->GetSmoothingGroupMgr();
    string newName = id_name ? id_name : pSmoothingGroupMgr->GetEmptyGroupID();
    pSmoothingGroupMgr->RemoveSmoothingGroup(newName);
    pSmoothingGroupMgr->AddSmoothingGroup(newName, new SmoothingGroup(polygons));

    UpdateAll(CD::eUT_Mesh | CD::eUT_SyncPrefab);

    return newName;
}

void SmoothingGroupTool::AddPolygonsToSmoothingGroup(const char* id_name)
{
    ElementManager* pSelected = CD::GetDesigner()->GetSelectedElements();
    if (pSelected->IsEmpty())
    {
        return;
    }

    SmoothingGroupManager* pSmoothingGroupMgr = GetModel()->GetSmoothingGroupMgr();
    for (int i = 0, iElementCount(pSelected->GetCount()); i < iElementCount; ++i)
    {
        if (!(*pSelected)[i].IsFace() || !(*pSelected)[i].m_pPolygon)
        {
            continue;
        }
        pSmoothingGroupMgr->AddPolygon(id_name, (*pSelected)[i].m_pPolygon);
    }

    UpdateAll(CD::eUT_Mesh | CD::eUT_SyncPrefab);
}

bool SmoothingGroupTool::HasSmoothingGroup(const char* id_name) const
{
    SmoothingGroupManager* pSmoothingGroupMgr = GetModel()->GetSmoothingGroupMgr();
    return pSmoothingGroupMgr->GetSmoothingGroup(id_name) != NULL;
}

bool SmoothingGroupTool::IsEmpty(const char* id_name) const
{
    SmoothingGroupManager* pSmoothingGroupMgr = GetModel()->GetSmoothingGroupMgr();
    DesignerSmoothingGroupPtr pSmoothingGroup = pSmoothingGroupMgr->GetSmoothingGroup(id_name);
    if (!pSmoothingGroup)
    {
        return true;
    }
    return pSmoothingGroup->GetPolygonCount() == 0;
}

int SmoothingGroupTool::GetSmoothingGroupCount() const
{
    return GetModel()->GetSmoothingGroupMgr()->GetCount();
}

std::vector<SSyncItem> SmoothingGroupTool::GetAll() const
{
    SmoothingGroupManager* pSmoothingGroupMgr = GetModel()->GetSmoothingGroupMgr();
    std::vector<std::pair<string, DesignerSmoothingGroupPtr> > groupList = pSmoothingGroupMgr->GetSmoothingGroupList();
    std::vector<SSyncItem> items;
    for (int i = 0, iGroupCount(groupList.size()); i < iGroupCount; ++i)
    {
        SSyncItem item;
        item.name = groupList[i].first;
        item.polygons = groupList[i].second->GetAll();
        items.push_back(item);
    }
    return items;
}

void SmoothingGroupTool::SyncAll(std::vector<SSyncItem>& items)
{
    SmoothingGroupManager* pSmoothingGroupMgr = GetModel()->GetSmoothingGroupMgr();

    pSmoothingGroupMgr->Clear();
    for (int i = 0, iCount(items.size()); i < iCount; ++i)
    {
        pSmoothingGroupMgr->AddSmoothingGroup(items[i].name, new SmoothingGroup(items[i].polygons));
    }

    UpdateAll(CD::eUT_Mesh | CD::eUT_SyncPrefab);
}

void SmoothingGroupTool::RenameGroup(const char* id_name, const char* new_id_name)
{
    SmoothingGroupManager* pSmoothingGroupMgr = GetModel()->GetSmoothingGroupMgr();
    DesignerSmoothingGroupPtr pSmoothingGroup = pSmoothingGroupMgr->GetSmoothingGroup(id_name);
    if (pSmoothingGroup)
    {
        pSmoothingGroupMgr->RemoveSmoothingGroup(id_name);
        pSmoothingGroupMgr->AddSmoothingGroup(new_id_name, pSmoothingGroup);
    }
}

void SmoothingGroupTool::RemovePolygonsFromSmoothingGroups()
{
    ElementManager* pSelected = CD::GetDesigner()->GetSelectedElements();
    SmoothingGroupManager* pSmoothingGroupMgr = GetModel()->GetSmoothingGroupMgr();
    for (int i = 0, iElementCount(pSelected->GetCount()); i < iElementCount; ++i)
    {
        if (!(*pSelected)[i].IsFace() || !(*pSelected)[i].m_pPolygon)
        {
            continue;
        }
        pSmoothingGroupMgr->RemovePolygon((*pSelected)[i].m_pPolygon);
    }
    UpdateAll(CD::eUT_Mesh | CD::eUT_SyncPrefab);
}

bool IsLessThanAngleOfSeedPolygons(CD::PolygonPtr pPolygon, const std::set<CD::PolygonPtr>& seedPolygons, BrushFloat fRadian)
{
    std::set<CD::PolygonPtr>::const_iterator ii = seedPolygons.begin();
    for (; ii != seedPolygons.end(); ++ii)
    {
        BrushFloat dot = (*ii)->GetPlane().Normal().Dot(pPolygon->GetPlane().Normal());
        if (std::acos(dot) <= fRadian)
        {
            return true;
        }
    }
    return false;
}

bool IsAdjacentWithSeedPolygons(CD::PolygonPtr pPolygon, const std::set<CD::PolygonPtr>& seedPolygons)
{
    std::set<CD::PolygonPtr>::const_iterator ii = seedPolygons.begin();
    for (; ii != seedPolygons.end(); ++ii)
    {
        bool bHadCommonEdge = false;
        int nEdgeCount = pPolygon->GetEdgeCount();
        for (int k = 0; k < nEdgeCount; ++k)
        {
            BrushEdge3D e = pPolygon->GetEdge(k);
            if ((*ii)->HasEdge(e))
            {
                bHadCommonEdge = true;
                break;
            }
        }
        if (bHadCommonEdge)
        {
            return true;
        }
    }
    return false;
}

void SmoothingGroupTool::ApplyAutoSmooth(int nAngle)
{
    BrushFloat fRadian = ((BrushFloat)nAngle / (BrushFloat)180) * CD::PI;

    ElementManager* pSelected = CD::GetDesigner()->GetSelectedElements();
    std::set<CD::PolygonPtr> usedPolygons;

    SmoothingGroupManager* pSmoothingGroupMgr = GetModel()->GetSmoothingGroupMgr();
    int iSelectedElementCount(pSelected->GetCount());

    std::set<CD::PolygonPtr> seedPolygons;
    std::vector<CD::PolygonPtr> polygonsInGroup;

    while (1)
    {
        CD::PolygonPtr pStartingSeedPolygon = NULL;
        if (seedPolygons.empty())
        {
            for (int i = 0; i < iSelectedElementCount; ++i)
            {
                if (usedPolygons.find((*pSelected)[i].m_pPolygon) == usedPolygons.end())
                {
                    pStartingSeedPolygon = (*pSelected)[i].m_pPolygon;
                    seedPolygons.insert(pStartingSeedPolygon);
                    polygonsInGroup.push_back(pStartingSeedPolygon);
                    usedPolygons.insert(pStartingSeedPolygon);
                    break;
                }
            }
            if (seedPolygons.empty())
            {
                break;
            }
        }

        bool bFinishLoop = false;
        while (!bFinishLoop && !seedPolygons.empty())
        {
            int nOffset = polygonsInGroup.size();
            for (int i = 0; i < iSelectedElementCount; ++i)
            {
                CD::PolygonPtr pPolygon = (*pSelected)[i].m_pPolygon;
                if (usedPolygons.find(pPolygon) != usedPolygons.end())
                {
                    continue;
                }

                if (!IsLessThanAngleOfSeedPolygons(pPolygon, seedPolygons, fRadian))
                {
                    continue;
                }

                if (!IsAdjacentWithSeedPolygons(pPolygon, seedPolygons))
                {
                    continue;
                }

                polygonsInGroup.push_back(pPolygon);
                usedPolygons.insert(pPolygon);
            }

            seedPolygons.clear();
            if (nOffset < polygonsInGroup.size())
            {
                seedPolygons.insert(polygonsInGroup.begin() + nOffset, polygonsInGroup.end());
            }
            else
            {
                if (polygonsInGroup.empty())
                {
                    bFinishLoop = true;
                    break;
                }
                for (int i = 0, iPolygonCount(polygonsInGroup.size()); i < iPolygonCount; ++i)
                {
                    pSmoothingGroupMgr->RemovePolygon(polygonsInGroup[i]);
                }
                string emptyGroupID = pSmoothingGroupMgr->GetEmptyGroupID();
                if (emptyGroupID.empty())
                {
                    bFinishLoop = true;
                    break;
                }
                pSmoothingGroupMgr->AddSmoothingGroup(emptyGroupID, new SmoothingGroup(polygonsInGroup));
                polygonsInGroup.clear();
                break;
            }
        }
        if (bFinishLoop)
        {
            break;
        }
    }

    UpdateAll(CD::eUT_Mesh | CD::eUT_SyncPrefab);
}

void SmoothingGroupTool::SelectPolygonsInSmoothingGroup(const char* id_name)
{
    SmoothingGroupManager* pSmoothingGroupMgr = GetModel()->GetSmoothingGroupMgr();
    DesignerSmoothingGroupPtr pSmoothingGroup = pSmoothingGroupMgr->GetSmoothingGroup(id_name);
    if (pSmoothingGroup == NULL)
    {
        return;
    }

    ElementManager* pSelected = CD::GetDesigner()->GetSelectedElements();
    for (int i = 0, iPolygonCount(pSmoothingGroup->GetPolygonCount()); i < iPolygonCount; ++i)
    {
        CD::PolygonPtr pPolygon = pSmoothingGroup->GetPolygon(i);
        pSelected->Add(SElement(GetBaseObject(), pPolygon));
    }

    CD::GetDesigner()->UpdateSelectionMeshFromSelectedElements(GetMainContext());
}

void SmoothingGroupTool::OnLButtonUp(CViewport* view, UINT nFlags, const QPoint& point)
{
    SelectTool::OnLButtonUp(view, nFlags, point);
    g_pSmoothingGroupPanel->ClearAllSelectionsOfNumbers();
}

bool SmoothingGroupTool::OnKeyDown(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags)
{
    if (nChar == VK_ESCAPE)
    {
        return CD::GetDesigner()->EndCurrentEditSession();
    }
    return true;
}

void SmoothingGroupTool::ClearSelectedElements()
{
    ElementManager* pSelected = CD::GetDesigner()->GetSelectedElements();
    pSelected->Clear();
    CD::GetDesigner()->UpdateSelectionMeshFromSelectedElements(GetMainContext());
}

#include "UIs/SmoothingGroupPanel.h"
REGISTER_DESIGNER_TOOL_WITH_PANEL(CD::eDesigner_SmoothingGroup, CD::eToolGroup_Surface, "Smoothing Group", SmoothingGroupTool, SmoothingGroupPanel)