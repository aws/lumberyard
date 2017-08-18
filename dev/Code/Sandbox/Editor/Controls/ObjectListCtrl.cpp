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

#include "stdafx.h"
#include "ObjectListCtrl.h"
#include <algorithm>
#include <functional>


// CObjectListCtrl

IMPLEMENT_DYNAMIC(CObjectListCtrl, CListCtrl)
CObjectListCtrl::CObjectListCtrl()
{
}

CObjectListCtrl::~CObjectListCtrl()
{
    DeleteAllItems();
}

void CObjectListCtrl::AddListener(IObjectListCtrlListener* pListener)
{
    this->listeners.insert(pListener);
}

void CObjectListCtrl::RemoveListener(IObjectListCtrlListener* pListener)
{
    this->listeners.erase(pListener);
}

int CObjectListCtrl::AddObject(IListCtrlObject* pObject)
{
    // Add the object to the array.
    int nObjectIndex = int(this->objects.size());
    if (pObject->GetText(1).IsEmpty())
    {
        this->objects.push_back(pObject);
        return nObjectIndex;
    }

    int ind = nObjectIndex;
    if (nObjectIndex == 0)
    {
        this->objects.push_back(pObject);
    }
    else
    {
        f32 timeCurrEvent = atof(pObject->GetText(1).GetBuffer());
        for (int i = 0; i < nObjectIndex; ++i)
        {
            f32 time = atof(this->objects[i]->GetText(1).GetBuffer());
            if (time > timeCurrEvent)
            {
                ind = i;
                break;
            }
        }
        this->objects.insert(this->objects.begin() + ind, pObject);
    }

    SetItemCount(this->objects.size());
    pObject->SetIndex(ind);

    return ind;
}

void CObjectListCtrl::DeleteAllItems()
{
    if (!::IsWindow(m_hWnd))
    {
        return;
    }

    // Override this method to clean up the objects.
    this->objects.clear();
    SetItemCount(0);

    // Perform usual removal of list control items.
    CListCtrl::DeleteAllItems();
}

BEGIN_MESSAGE_MAP(CObjectListCtrl, CSubeditListCtrl)
ON_NOTIFY_REFLECT(LVN_GETDISPINFO, OnLVNGetDispInfo)
ON_NOTIFY_REFLECT(LVN_SETDISPINFO, OnLVNSetDispInfo)
ON_NOTIFY_REFLECT(LVN_ODSTATECHANGED, OnLVNODStateChanged)
ON_NOTIFY_REFLECT(LVN_ITEMCHANGED, OnLVNItemChanged)
END_MESSAGE_MAP()



// CObjectListCtrl message handlers

void CObjectListCtrl::OnLVNGetDispInfo(NMHDR* pHDR, LRESULT* result)
{
    NMLVDISPINFO* pInfo = (NMLVDISPINFO*)pHDR;

    // Get the object that the item is displaying.
    IListCtrlObject* pObject = this->objects[pInfo->item.iItem];

    // Get the text for the item.
    if (pInfo->item.mask & LVIF_TEXT)
    {
        CString sText = pObject->GetText(pInfo->item.iSubItem);
        if (pInfo->item.cchTextMax > 0)
        {
            cry_strcpy(pInfo->item.pszText, pInfo->item.cchTextMax, sText);
        }
    }

    // Get the state for the item.
    if (pInfo->item.mask & LVIF_STATE)
    {
        pInfo->item.state = pObject->IsSelected() ? LVIS_SELECTED : 0;
    }

    // Update the image index.
    if (pInfo->item.mask & LVIF_IMAGE)
    {
        pInfo->item.iImage = 0;
    }
}

void CObjectListCtrl::OnLVNSetDispInfo(NMHDR* pHDR, LRESULT* result)
{
    NMLVDISPINFO* pInfo = (NMLVDISPINFO*)pHDR;

    // Get the object that the item is displaying.
    IListCtrlObject* pObject = this->objects[pInfo->item.lParam];

    *result = 0;
}

void CObjectListCtrl::OnLVNODStateChanged(NMHDR* pHDR, LRESULT* result)
{
    NMLVODSTATECHANGE* pInfo = (NMLVODSTATECHANGE*)pHDR;

    for (std::set<IObjectListCtrlListener*>::iterator itListener = this->listeners.begin(); itListener != this->listeners.end(); ++itListener)
    {
        (*itListener)->OnObjectListCtrlSelectionRangeChanged(this, pInfo->iFrom, pInfo->iTo, (pInfo->uNewState & LVIS_SELECTED) != 0);
    }

    *result = 0;
}

void CObjectListCtrl::OnLVNItemChanged(NMHDR* pHDR, LRESULT* result)
{
    NMLISTVIEW* pInfo = (NMLISTVIEW*)pHDR;

    // Check whether selections have changed.
    if (pInfo->uChanged & LVIF_STATE)
    {
        // Choose the range of items that have changed state - note a value of -1 indicates that every object was changed.
        int nFirst = pInfo->iItem;
        int nLast = pInfo->iItem;
        if (pInfo->iItem == -1)
        {
            nFirst = 0;
            nLast = this->GetItemCount() - 1;
        }

        // Report the change to listeners.
        for (std::set<IObjectListCtrlListener*>::iterator itListener = this->listeners.begin(); itListener != this->listeners.end(); ++itListener)
        {
            (*itListener)->OnObjectListCtrlSelectionRangeChanged(this, nFirst, nLast, (pInfo->uNewState & LVIS_SELECTED) != 0);
        }
    }
}

void CObjectListCtrl::TextChanged(int item, int subitem, const char* szText)
{
    // Report the change to listeners.
    if (szText != 0)
    {
        for (std::set<IObjectListCtrlListener*>::iterator itListener = this->listeners.begin(); itListener != this->listeners.end(); ++itListener)
        {
            (*itListener)->OnObjectListCtrlLabelChanged(this, item, subitem, szText);
        }
    }
}

void CObjectListCtrl::GetOptions(int item, int subitem, std::vector<string>& options, string& currentOption)
{
    objects[item]->GetOptions(subitem, options, currentOption);
}

CObjectListCtrl::EditStyle CObjectListCtrl::GetEditStyle(int item, int subitem)
{
    return objects[item]->GetEditStyle(subitem);
}
