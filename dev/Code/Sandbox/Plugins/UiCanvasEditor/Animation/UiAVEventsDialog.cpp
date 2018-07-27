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
#include "EditorDefs.h"
#include "UiEditorAnimationBus.h"
#include "UiAVEventsDialog.h"
#include "StringDlg.h"
#include "UiAnimViewSequence.h"
#include "AnimationContext.h"
#include <limits>


// CUiAVEventsDialog dialog

namespace
{
    const int kCountSubItemIndex = 1;
    const int kTimeSubItemIndex = 2;
}

IMPLEMENT_DYNAMIC(CUiAVEventsDialog, CDialog)

CUiAVEventsDialog::CUiAVEventsDialog(CWnd* pParent /*=NULL*/)
    : CDialog(CUiAVEventsDialog::IDD, pParent)
{
}

CUiAVEventsDialog::~CUiAVEventsDialog()
{
}

void CUiAVEventsDialog::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_EVENTS_LIST, m_List);
}


BEGIN_MESSAGE_MAP(CUiAVEventsDialog, CDialog)
ON_BN_CLICKED(IDC_BUTTON_ADDEVENT, &CUiAVEventsDialog::OnBnClickedButtonAddEvent)
ON_BN_CLICKED(IDC_BUTTON_REMOVEEVENT, &CUiAVEventsDialog::OnBnClickedButtonRemoveEvent)
ON_BN_CLICKED(IDC_BUTTON_RENAMEEVENT, &CUiAVEventsDialog::OnBnClickedButtonRenameEvent)
ON_BN_CLICKED(IDC_BUTTON_UPEVENT, &CUiAVEventsDialog::OnBnClickedButtonUpEvent)
ON_BN_CLICKED(IDC_BUTTON_DOWNEVENT, &CUiAVEventsDialog::OnBnClickedButtonDownEvent)
ON_NOTIFY(LVN_ITEMCHANGED, IDC_EVENTS_LIST, OnListItemChanged)
END_MESSAGE_MAP()


// CUiAVEventsDialog message handlers

void CUiAVEventsDialog::OnBnClickedButtonAddEvent()
{
#if UI_ANIMATION_REMOVED    // UI_ANIMATION_REVISIT do we need this file at all?
    CUiAnimViewSequence* pSequence = nullptr;
    EBUS_EVENT_RESULT(pSequence, UiEditorAnimationBus, GetCurrentSequence);

    CStringDlg dlg(_T("Track Event Name"));
    if (dlg.DoModal() == IDOK && !dlg.GetString().IsEmpty())
    {
        CString add = dlg.GetString();

        // Make sure it doesn't already exist
        LVFINDINFO find;
        find.flags = LVFI_STRING | LVFI_WRAP;
        find.psz = add;
        if (-1 == m_List.FindItem(&find))
        {
            for (int k = 0; k < m_List.GetItemCount(); ++k)
            {
                m_List.SetItemState(k, 0, LVIS_SELECTED);
            }
            m_List.InsertItem(m_List.GetItemCount(), add);
            m_List.SetItemText(m_List.GetItemCount() - 1, kCountSubItemIndex, "0");
            m_List.SetItemText(m_List.GetItemCount() - 1, kTimeSubItemIndex, "");
            m_List.SetItemState(m_List.GetItemCount() - 1, LVIS_SELECTED, LVIS_SELECTED);
            pSequence->AddTrackEvent(add);
        }
    }
    m_List.SetFocus();
#endif
}

void CUiAVEventsDialog::OnBnClickedButtonRemoveEvent()
{
#if UI_ANIMATION_REMOVED
    CUiAnimViewSequence* pSequence = nullptr;
    EBUS_EVENT_RESULT(pSequence, UiEditorAnimationBus, GetCurrentSequence);

    POSITION pos = m_List.GetFirstSelectedItemPosition();
    while (pos)
    {
        int index = m_List.GetNextSelectedItem(pos);
        if (MessageBox("This removal might cause some link breakages in Flow Graph.\nStill continue?", "Remove Event", MB_YESNO | MB_ICONWARNING) == IDYES)
        {
            CString eventName = m_List.GetItemText(index, 0);
            m_List.DeleteItem(index);
            pSequence->RemoveTrackEvent(eventName);
            pos = m_List.GetFirstSelectedItemPosition();
        }
    }
    m_List.SetFocus();
#endif
}

void CUiAVEventsDialog::OnBnClickedButtonRenameEvent()
{
#if UI_ANIMATION_REMOVED
    CUiAnimViewSequence* pSequence = nullptr;
    EBUS_EVENT_RESULT(pSequence, UiEditorAnimationBus, GetCurrentSequence);

    POSITION pos = m_List.GetFirstSelectedItemPosition();
    if (pos)
    {
        int index = m_List.GetNextSelectedItem(pos);
        CStringDlg dlg(_T("Track Event Name"));
        if (dlg.DoModal() == IDOK && !dlg.GetString().IsEmpty())
        {
            CString oldName = m_List.GetItemText(index, 0);
            CString newName = dlg.GetString();
            if (oldName != newName)
            {
                // Make sure it doesn't already exist
                LVFINDINFO find;
                find.flags = LVFI_STRING | LVFI_WRAP;
                find.psz = newName;
                if (-1 == m_List.FindItem(&find))
                {
                    m_List.SetItemText(index, 0, newName);
                    pSequence->RenameTrackEvent(oldName, newName);
                }
            }
        }
    }
    m_List.SetFocus();
#endif
}

void CUiAVEventsDialog::OnBnClickedButtonUpEvent()
{
#if UI_ANIMATION_REMOVED
    CUiAnimViewSequence* pSequence = nullptr;
    EBUS_EVENT_RESULT(pSequence, UiEditorAnimationBus, GetCurrentSequence);

    POSITION pos = m_List.GetFirstSelectedItemPosition();
    if (pos)
    {
        int index = m_List.GetNextSelectedItem(pos);
        if (index > 0)
        {
            CString up = m_List.GetItemText(index - 1, 0);
            CString down = m_List.GetItemText(index, 0);
            m_List.SetItemText(index - 1, 0, down);
            m_List.SetItemText(index, 0, up);
            m_List.SetItemState(index - 1, LVIS_SELECTED, LVIS_SELECTED);
            m_List.SetItemState(index, 0, LVIS_SELECTED);
            pSequence->MoveUpTrackEvent(down);
        }
    }
    m_List.SetFocus();
#endif
}

void CUiAVEventsDialog::OnBnClickedButtonDownEvent()
{
#if UI_ANIMATION_REMOVED
    CUiAnimViewSequence* pSequence = nullptr;
    EBUS_EVENT_RESULT(pSequence, UiEditorAnimationBus, GetCurrentSequence);

    POSITION pos = m_List.GetFirstSelectedItemPosition();
    if (pos)
    {
        int index = m_List.GetNextSelectedItem(pos);
        if (index < m_List.GetItemCount() - 1)
        {
            CString up = m_List.GetItemText(index, 0);
            CString down = m_List.GetItemText(index + 1, 0);
            m_List.SetItemText(index, 0, down);
            m_List.SetItemText(index + 1, 0, up);
            m_List.SetItemState(index, 0, LVIS_SELECTED);
            m_List.SetItemState(index + 1, LVIS_SELECTED, LVIS_SELECTED);
            pSequence->MoveDownTrackEvent(up);
        }
    }
    m_List.SetFocus();
#endif
}

BOOL CUiAVEventsDialog::OnInitDialog()
{
#if UI_ANIMATION_REMOVED
    CDialog::OnInitDialog();

    m_List.InsertColumn(0, "Event", LVCFMT_LEFT, 100);
    m_List.InsertColumn(1, "# of use", LVCFMT_RIGHT, 30, kCountSubItemIndex);
    m_List.InsertColumn(2, "Time of first usage", LVCFMT_RIGHT, 50, kTimeSubItemIndex);

    CUiAnimViewSequence* pSequence = nullptr;
    EBUS_EVENT_RESULT(pSequence, UiEditorAnimationBus, GetCurrentSequence);
    assert(pSequence);

    // Push existing items into list
    const int iCount = pSequence->GetTrackEventsCount();
    for (int i = 0; i < iCount; ++i)
    {
        m_List.InsertItem(i, pSequence->GetTrackEvent(i));
        float timeFirstUsed = 0;
        int usageCount = GetNumberOfUsageAndFirstTimeUsed(pSequence->GetTrackEvent(i), timeFirstUsed);
        CString countText, timeText;
        countText.Format("%d", usageCount);
        timeText.Format("%.3f", timeFirstUsed);
        m_List.SetItemText(i, kCountSubItemIndex, countText);
        m_List.SetItemText(i, kTimeSubItemIndex, usageCount > 0 ? timeText : "");
    }

    UpdateButtons();
#endif

    return TRUE;  // return TRUE unless you set the focus to a control
    // EXCEPTION: OCX Property Pages should return FALSE
}

void CUiAVEventsDialog::OnListItemChanged(NMHDR* pNMHDR, LRESULT* pResult)
{
    NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

    // if( pNMListView->uNewState & LVIS_SELECTED )
    {
        // int selectedItem = pNMListView->iItem;
        UpdateButtons();
    }

    *pResult = 0;
}
void CUiAVEventsDialog::UpdateButtons()
{
    BOOL bRemove = FALSE, bRename = FALSE, bUp = FALSE, bDown = FALSE;

    UINT nSelected = m_List.GetSelectedCount();
    if (nSelected > 1)
    {
        bRemove = TRUE;
        bRename = FALSE;
    }
    else if (nSelected > 0)
    {
        bRemove = bRename = TRUE;

        POSITION pos = m_List.GetFirstSelectedItemPosition();
        int index = m_List.GetNextSelectedItem(pos);
        if (index > 0)
        {
            bUp = TRUE;
        }
        if (index < m_List.GetItemCount() - 1)
        {
            bDown = TRUE;
        }
    }

    GetDlgItem(IDC_BUTTON_REMOVEEVENT)->EnableWindow(bRemove);
    GetDlgItem(IDC_BUTTON_RENAMEEVENT)->EnableWindow(bRename);
    GetDlgItem(IDC_BUTTON_UPEVENT)->EnableWindow(bUp);
    GetDlgItem(IDC_BUTTON_DOWNEVENT)->EnableWindow(bDown);
}

int CUiAVEventsDialog::GetNumberOfUsageAndFirstTimeUsed(const char* eventName, float& timeFirstUsed) const
{
    CUiAnimViewSequence* pSequence = nullptr;
    EBUS_EVENT_RESULT(pSequence, UiEditorAnimationBus, GetCurrentSequence);

    int usageCount = 0;
    float firstTime = std::numeric_limits<float>::max();

    CUiAnimViewAnimNodeBundle nodeBundle = pSequence->GetAnimNodesByType(eUiAnimNodeType_Event);
    const unsigned int numNodes = nodeBundle.GetCount();

    for (unsigned int currentNode = 0; currentNode < numNodes; ++currentNode)
    {
        CUiAnimViewAnimNode* pCurrentNode = nodeBundle.GetNode(currentNode);

        CUiAnimViewTrackBundle tracks = pCurrentNode->GetTracksByParam(eUiAnimParamType_TrackEvent);
        const unsigned int numTracks = tracks.GetCount();

        for (unsigned int currentTrack = 0; currentTrack < numTracks; ++currentTrack)
        {
            CUiAnimViewTrack* pTrack = tracks.GetTrack(currentTrack);

            for (int currentKey = 0; currentKey < pTrack->GetKeyCount(); ++currentKey)
            {
                CUiAnimViewKeyHandle keyHandle = pTrack->GetKey(currentKey);

                IEventKey key;
                keyHandle.GetKey(&key);

                if (strcmp(key.event, eventName) == 0) // If it has a key with the specified event set
                {
                    ++usageCount;
                    if (key.time < firstTime)
                    {
                        firstTime = key.time;
                    }
                }
            }
        }
    }

    if (usageCount > 0)
    {
        timeFirstUsed = firstTime;
    }
    return usageCount;
}
