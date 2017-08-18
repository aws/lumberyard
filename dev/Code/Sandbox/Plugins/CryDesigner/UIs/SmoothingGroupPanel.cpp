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
#include "SmoothingGroupPanel.h"
#include "DesignerPanel.h"
#include "Tools/Surface/SmoothingGroupTool.h"
#include "Tools/DesignerTool.h"
#include "Core/Model.h"
#include <QListWidget>
#include <QGridLayout>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QMessageBox>
#include "Serialization/Decorators/EditorActionButton.h"

const char* SmoothingGroupPanel::EmptySlotName = "Empty Slot";

SmoothingGroupPanel::SmoothingGroupPanel(SmoothingGroupTool* pSmoothingGroupTool)
    : m_pSmoothingGroupTool(pSmoothingGroupTool)
{
    m_pSelectedItemAddress = NULL;
    m_fThresholdAngle = 45.0f;
    CreatePropertyTree(this,
        [=](bool continuous)
        {
            OnChange(continuous);
        });
    m_pPropertyTree->setUndoEnabled(false);
    QObject::connect(m_pPropertyTree, &QPropertyTree::signalSelected, this, [ = ] {OnGroupItemSelected();
        });
    UpdateFromSmoothingGroupManager();

    CD::SetAttributeWidget(m_pSmoothingGroupTool, this, CD::MaximumAttributePanelHeight);
}

void SmoothingGroupPanel::Done()
{
    CD::RemoveAttributeWidget(m_pSmoothingGroupTool);
}

void SmoothingGroupPanel::UpdateFromSmoothingGroupManager()
{
    m_SmoothingGroupList.clear();
    m_PolygonStorage.clear();

    std::vector<SSyncItem> items = m_pSmoothingGroupTool->GetAll();
    for (int i = 0, iCount(items.size()); i < iCount; ++i)
    {
        SSmoothingGroupItem item(items[i].name);
        m_PolygonStorage[items[i].name] = items[i].polygons;
        m_SmoothingGroupList.push_back(item);
    }

    m_pPropertyTree->revert();
}

bool SmoothingGroupPanel::HasName(const char* name) const
{
    for (int i = 0, iCount(m_SmoothingGroupList.size()); i < iCount; ++i)
    {
        if (m_SmoothingGroupList[i].groupName == name)
        {
            return true;
        }
    }
    return false;
}

void SmoothingGroupPanel::UpdateToSmoothingGroupManager()
{
    std::map<string, std::vector<CD::PolygonPtr> >::iterator ii = m_PolygonStorage.begin();
    for (; ii != m_PolygonStorage.end(); )
    {
        if (!HasName(ii->first))
        {
            ii = m_PolygonStorage.erase(ii);
        }
        else
        {
            ++ii;
        }
    }

    std::vector<CD::SSyncItem> items;
    for (int i = 0, iCount(m_SmoothingGroupList.size()); i < iCount; ++i)
    {
        std::map<string, std::vector<CD::PolygonPtr> >::iterator ii = m_PolygonStorage.find(m_SmoothingGroupList[i].groupName);
        if (ii == m_PolygonStorage.end())
        {
            continue;
        }
        if (ii->second.empty())
        {
            return;
        }
        CD::SSyncItem item;
        item.name = m_SmoothingGroupList[i].groupName;
        item.polygons = m_PolygonStorage[m_SmoothingGroupList[i].groupName];
        items.push_back(item);
    }
    m_pSmoothingGroupTool->SyncAll(items);
}

void SmoothingGroupPanel::Serialize(Serialization::IArchive& ar)
{
    std::vector<SSmoothingGroupItem> groupItemsBefore = m_SmoothingGroupList;

    ar(m_SmoothingGroupList, "SmoothingGroups", "Smoothing Groups");
    ar(Serialization::ActionButton(std::bind(&SmoothingGroupPanel::OnAddFacesToSG, this)), "Add", "Add Faces To SG");
    ar(Serialization::ActionButton(std::bind(&SmoothingGroupPanel::OnSelectBySG, this)), "Select", "Select Faces By SG");
    ar(Serialization::ActionButton(std::bind(&SmoothingGroupPanel::OnClearEmpty, this)), "ClearEmpty", "Clear Empty SGs");
    ar(Serialization::ActionButton(std::bind(&SmoothingGroupPanel::OnAutoSmooth, this)), "AutoSmooth", "Auto Smooth with Threshold Angle");
    ar(m_fThresholdAngle, "ThresholdAngle", "Threshold Angle");

    if (m_SmoothingGroupList.size() == groupItemsBefore.size())
    {
        RenameSmoothingGroup(groupItemsBefore);
    }
    else if (m_SmoothingGroupList.size() < groupItemsBefore.size())
    {
        UpdateToSmoothingGroupManager();
    }
    else if (m_SmoothingGroupList.size() == groupItemsBefore.size() + 1)
    {
        SelectEmptySlot();
    }
}

void SmoothingGroupPanel::RenameSmoothingGroup(std::vector<SSmoothingGroupItem>& groupItemsBefore)
{
    int nIndex = GetDifferentMemberIndex(groupItemsBefore);
    if (nIndex != -1)
    {
        std::map<string, std::vector<CD::PolygonPtr> >::iterator iter = m_PolygonStorage.find(groupItemsBefore[nIndex].groupName);
        if (iter != m_PolygonStorage.end())
        {
            m_PolygonStorage[m_SmoothingGroupList[nIndex].groupName] = iter->second;
            m_PolygonStorage.erase(iter);
        }
    }
}

int SmoothingGroupPanel::GetDifferentMemberIndex(const std::vector<SSmoothingGroupItem>& items)
{
    if (items.size() != m_SmoothingGroupList.size())
    {
        return -1;
    }

    bool bDifferent = false;
    for (int i = 0, iCount(items.size()); i < iCount; ++i)
    {
        if (!HasName(items[i].groupName))
        {
            bDifferent = true;
            break;
        }
    }
    if (!bDifferent)
    {
        return -1;
    }

    for (int i = 0, iCount(items.size()); i < iCount; ++i)
    {
        if (items[i].groupName != m_SmoothingGroupList[i].groupName)
        {
            return i;
        }
    }

    return -1;
}

void SmoothingGroupPanel::RemoveEmptySlots()
{
    std::vector<SSmoothingGroupItem>::iterator ii = m_SmoothingGroupList.begin();
    for (; ii != m_SmoothingGroupList.end(); )
    {
        if ((*ii).groupName == EmptySlotName)
        {
            ii = m_SmoothingGroupList.erase(ii);
        }
        else
        {
            ++ii;
        }
    }
    m_pPropertyTree->revert();
}

bool SmoothingGroupPanel::HasEmptySlot() const
{
    for (int i = 0, iCount(m_SmoothingGroupList.size()); i < iCount; ++i)
    {
        if (m_SmoothingGroupList[i].groupName == EmptySlotName)
        {
            return true;
        }
    }
    return false;
}

void SmoothingGroupPanel::SelectEmptySlot()
{
    for (int i = 0, iCount(m_SmoothingGroupList.size()); i < iCount; ++i)
    {
        if (m_SmoothingGroupList[i].groupName == EmptySlotName)
        {
            m_pSelectedItemAddress = (const void*)&m_SmoothingGroupList[i];
            m_SelectedItemName = EmptySlotName;
            break;
        }
    }
}

int SmoothingGroupPanel::GetSelectedGroupIndex() const
{
    for (int i = 0, iCount(m_SmoothingGroupList.size()); i < iCount; ++i)
    {
        if (m_pSelectedItemAddress == &m_SmoothingGroupList[i])
        {
            return i;
        }
    }
    return -1;
}

std::vector<CD::PolygonPtr> SmoothingGroupPanel::GetSelectedPolygons()
{
    ElementManager* pSelected = CD::GetDesigner()->GetSelectedElements();
    std::vector<CD::PolygonPtr> polygons;
    for (int i = 0, iCount(pSelected->GetCount()); i < iCount; ++i)
    {
        const SElement& e = pSelected->Get(i);
        if (e.IsFace() && e.m_pPolygon)
        {
            polygons.push_back(e.m_pPolygon);
        }
    }
    return polygons;
}

string SmoothingGroupPanel::GetEmptyGroupID() const
{
    string basicName = "SG";
    int count = 0;
    do
    {
        char buff[1024];
        sprintf(buff, "%d", count);
        string combinedName = basicName + string(buff);
        if (!HasName(combinedName))
        {
            return combinedName;
        }
    }
    while ((++count) != 0);
    return "";
}

bool SmoothingGroupPanel::HasPolygon(CD::PolygonPtr pPolygon, std::vector<CD::PolygonPtr>& polygons) const
{
    for (int i = 0, iCount(polygons.size()); i < iCount; ++i)
    {
        if (polygons[i] == pPolygon)
        {
            return true;
        }
    }
    return false;
}

void SmoothingGroupPanel::RemoveDuplicatedPolygons(std::vector<CD::PolygonPtr>& polygons)
{
    std::map<string, std::vector<CD::PolygonPtr> >::iterator i1 = m_PolygonStorage.begin();
    for (; i1 != m_PolygonStorage.end(); )
    {
        std::vector<CD::PolygonPtr>& polygonsIn = i1->second;
        std::vector<CD::PolygonPtr>::iterator i2 = polygonsIn.begin();
        for (; i2 != polygonsIn.end(); )
        {
            if (HasPolygon(*i2, polygons))
            {
                i2 = polygonsIn.erase(i2);
            }
            else
            {
                ++i2;
            }
        }
        if (polygonsIn.empty())
        {
            for (int i = 0, iCount(m_SmoothingGroupList.size()); i < iCount; ++i)
            {
                if (m_SmoothingGroupList[i].groupName == i1->first)
                {
                    m_SmoothingGroupList[i].groupName = EmptySlotName;
                }
            }
            i1 = m_PolygonStorage.erase(i1);
        }
        else
        {
            ++i1;
        }
    }
}

void SmoothingGroupPanel::OnAssignSG()
{
    int nItemIndex = GetSelectedGroupIndex();
    if (nItemIndex == -1)
    {
        CD::MessageBox("Warning", "One slot needs to be chosen.");
        return;
    }

    std::vector<CD::PolygonPtr> polygons = GetSelectedPolygons();
    if (polygons.empty())
    {
        CD::MessageBox("Warning", "At least one face should be chosen.");
        return;
    }

    RemoveDuplicatedPolygons(polygons);

    m_SmoothingGroupList[nItemIndex].groupName = GetEmptyGroupID();
    m_PolygonStorage[m_SmoothingGroupList[nItemIndex].groupName] = polygons;

    UpdateToSmoothingGroupManager();

    m_pPropertyTree->revert();
}

void SmoothingGroupPanel::OnAddFacesToSG()
{
    int nItemIndex = GetSelectedGroupIndex();
    if (nItemIndex == -1)
    {
        CD::MessageBox("Warning", "One smoothing group should be chosen before adding faces.");
        return;
    }
    if (m_PolygonStorage.find(m_SmoothingGroupList[nItemIndex].groupName) != m_PolygonStorage.end())
    {
        std::vector<CD::PolygonPtr> polygons = GetSelectedPolygons();
        RemoveDuplicatedPolygons(polygons);
        string groupName = m_SmoothingGroupList[nItemIndex].groupName;
        m_PolygonStorage[groupName].insert(m_PolygonStorage[groupName].end(), polygons.begin(), polygons.end());
        UpdateToSmoothingGroupManager();
        m_pPropertyTree->revert();
    }
    else
    {
        OnAssignSG();
    }
}

void SmoothingGroupPanel::OnSelectBySG()
{
    int nIndex = GetSelectedGroupIndex();
    if (nIndex == -1)
    {
        return;
    }

    m_pSmoothingGroupTool->ClearSelectedElements();
    if (m_SmoothingGroupList[nIndex].groupName != EmptySlotName)
    {
        m_pSmoothingGroupTool->SelectPolygonsInSmoothingGroup(m_SmoothingGroupList[nIndex].groupName);
    }
}

void SmoothingGroupPanel::OnAutoSmooth()
{
    CUndo undo("Designer : Smoothing Group");
    m_pSmoothingGroupTool->GetModel()->RecordUndo("Designer : Smoothing Group", m_pSmoothingGroupTool->GetBaseObject());
    m_pSmoothingGroupTool->ApplyAutoSmooth((int)m_fThresholdAngle);
    UpdateFromSmoothingGroupManager();
}

void SmoothingGroupPanel::OnGroupItemSelected()
{
    bool bSmoothingGroupSelected = false;
    const void* pAddress = m_pPropertyTree->selectedRow() ? m_pPropertyTree->selectedRow()->searchHandle() : NULL;
    if (pAddress == NULL)
    {
        return;
    }

    for (int i = 0, iCount(m_SmoothingGroupList.size()); i < iCount; ++i)
    {
        if (pAddress == &m_SmoothingGroupList[i])
        {
            m_pSelectedItemAddress = pAddress;
            m_SelectedItemName = m_SmoothingGroupList[i].groupName;
            bSmoothingGroupSelected = true;
            break;
        }
    }
}

void SmoothingGroupPanel::OnClearEmpty()
{
    std::vector<SSmoothingGroupItem>::iterator ii = m_SmoothingGroupList.begin();
    for (; ii != m_SmoothingGroupList.end(); )
    {
        if (m_PolygonStorage.find((*ii).groupName) == m_PolygonStorage.end() || m_PolygonStorage[(*ii).groupName].empty())
        {
            m_PolygonStorage.erase((*ii).groupName);
            ii = m_SmoothingGroupList.erase(ii);
        }
        else
        {
            ++ii;
        }
    }
    UpdateToSmoothingGroupManager();
    m_pPropertyTree->revert();
}

void SmoothingGroupPanel::OnChange(bool continuous)
{
    if (continuous)
    {
        return;
    }

    UpdateToSmoothingGroupManager();
}