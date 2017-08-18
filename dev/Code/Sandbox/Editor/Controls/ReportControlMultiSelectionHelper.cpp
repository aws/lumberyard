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
#include "ReportControlMultiSelectionHelper.h"

void CReportControlMultiSelectionHelper::OnCreate(CXTPReportControl* pControl) const
{
    assert(pControl);
    pControl->SetMultipleSelection(TRUE);
}


void CReportControlMultiSelectionHelper::OnLButtonDblClk(CXTPReportControl* pControl, UINT nFlags, CPoint ptDblClick) const
{
    SelectFromMouseClick(pControl, nFlags, ptDblClick, ChildSelectMode_Recursive);
}


void CReportControlMultiSelectionHelper::OnLButtonUp(CXTPReportControl* pControl, UINT nFlags, CPoint point) const
{
    // Intentionally doing nothing so that records don't get unselected after double-click.
}


// The following code can be used before calling OnLButtonDown for controls that need to
// keep expand/collapse behaviour.
// {
//  __super::OnLButtonDown( nFlags, point );
//  __super::OnLButtonUp( nFlags, point );
// }
void CReportControlMultiSelectionHelper::OnLButtonDown(CXTPReportControl* pControl, UINT nFlags, CPoint point) const
{
    SelectFromMouseClick(pControl, nFlags, point, ChildSelectMode_Ignore);
}



void CReportControlMultiSelectionHelper::ClearSelection(CXTPReportControl* pControl) const
{
    assert(pControl);
    CXTPReportSelectedRows* pSelectedRows = pControl->GetSelectedRows();
    assert(pSelectedRows != NULL);

    pSelectedRows->Clear();
}


bool CReportControlMultiSelectionHelper::SelectFromMouseClick(CXTPReportControl* pControl, UINT nFlags, CPoint point, ChildSelectMode childSelectMode) const
{
    const bool shiftPressed = (nFlags & MK_SHIFT) != 0;
    if (shiftPressed)
    {
        return false;
    }

    assert(pControl);
    CXTPReportRow* pRow = pControl->HitTest(point);
    if (pRow == NULL)
    {
        return false;
    }

    CXTPReportRecord* pRecord = pRow->GetRecord();
    assert(pRecord != NULL);

    const bool controlPressed = (nFlags & MK_CONTROL) != 0;

    const bool clearSelection = (!controlPressed);
    if (clearSelection)
    {
        ClearSelection(pControl);
    }

    SelectMode selectMode = SelectMode_Add;
    if (controlPressed)
    {
        selectMode = pRow->IsSelected() ? SelectMode_Add : SelectMode_Remove;
    }

    SelectFromRecord(pControl, pRecord, selectMode, childSelectMode);
    return true;
}


void CReportControlMultiSelectionHelper::SelectFromRecord(CXTPReportControl* pControl, CXTPReportRecord* pRecord, SelectMode selectMode, ChildSelectMode childSelectMode) const
{
    assert(pControl);
    if (pRecord == NULL)
    {
        return;
    }

    CXTPReportRows* pRows = pControl->GetRows();
    assert(pRows != NULL);

    CXTPReportRow* pRow = pRows->Find(pRecord);

    SelectFromRow(pControl, pRow, selectMode);

    if (childSelectMode == ChildSelectMode_Recursive)
    {
        CXTPReportRecords* pChildRecords = pRecord->GetChilds();
        if (pChildRecords == NULL)
        {
            return;
        }

        const int childCount = pChildRecords->GetCount();
        for (int i = 0; i < childCount; ++i)
        {
            CXTPReportRecord* pChildRecord = pChildRecords->GetAt(i);
            SelectFromRecord(pControl, pChildRecord, selectMode, childSelectMode);
        }
    }
}


void CReportControlMultiSelectionHelper::SelectFromRow(CXTPReportControl* pControl, CXTPReportRow* pRow, SelectMode selectMode) const
{
    assert(pControl);
    if (pRow == NULL)
    {
        return;
    }

    CXTPReportSelectedRows* pSelectedRows = pControl->GetSelectedRows();
    assert(pSelectedRows != NULL);

    switch (selectMode)
    {
    case SelectMode_Add:
        pSelectedRows->Add(pRow);
        break;

    case SelectMode_Remove:
        pSelectedRows->Remove(pRow);
        break;

    default:
        assert(false);
        break;
    }
}


