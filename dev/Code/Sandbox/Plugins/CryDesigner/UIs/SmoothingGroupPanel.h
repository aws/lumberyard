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

#pragma once

#include "UICommon.h"
#include "Core/Polygon.h"

class SmoothingGroupTool;
class QListWidget;

class SmoothingGroupPanel
    : public CD::QPropertyTreeWidget
    , public ISmoothingGroupPanel
{
public:

    SmoothingGroupPanel(SmoothingGroupTool* pSmoothingGroupTool);

    void Done() override;

    void ClearAllSelectionsOfNumbers(int nExcludedID = -1) override {}
    void ShowAllNumbers() override {}
    void HideNumber(int nNumber) override {}

    void Serialize(Serialization::IArchive& ar) override;

protected:

    static const char* EmptySlotName;
    struct SSmoothingGroupItem
    {
        CryStringT<char> groupName;
        SSmoothingGroupItem()
            : groupName(EmptySlotName)
        {
        }
        SSmoothingGroupItem(const string& name)
            : groupName(name)
        {
        }
        void Serialize(Serialization::IArchive& ar)
        {
            ar(groupName, "groupname", "^");
        }
    };

    void OnAssignSG();
    void OnAddFacesToSG();
    void OnAutoSmooth();
    void OnSelectBySG();
    void OnGroupItemSelected();
    void OnClearEmpty();
    void OnChange(bool continuous);

    std::vector<CD::PolygonPtr> GetSelectedPolygons();

    void RenameSmoothingGroup(std::vector<SSmoothingGroupItem>& groupItemsBefore);
    void UpdateFromSmoothingGroupManager();
    void UpdateToSmoothingGroupManager();

    void RemoveEmptySlots();
    bool HasEmptySlot() const;
    void SelectEmptySlot();
    string GetEmptyGroupID() const;
    int GetSelectedGroupIndex() const;

    void RemoveDuplicatedPolygons(std::vector<CD::PolygonPtr>& polygons);
    int GetDifferentMemberIndex(const std::vector<SSmoothingGroupItem>& items);

    bool HasName(const char* name) const;
    bool HasPolygon(CD::PolygonPtr pPolygon, std::vector<CD::PolygonPtr>& polygons) const;

    float m_fThresholdAngle;
    std::vector<SSmoothingGroupItem> m_SmoothingGroupList;
    std::map<string, std::vector<CD::PolygonPtr> > m_PolygonStorage;
    SmoothingGroupTool* m_pSmoothingGroupTool;
    const void* m_pSelectedItemAddress;
    string m_SelectedItemName;
};