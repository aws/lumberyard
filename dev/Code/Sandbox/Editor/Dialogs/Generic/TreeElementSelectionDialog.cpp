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

// Description : This is the header file for the general utility dialog for
//               tree element selection. The purpose of this dialog, as one might imagine,
//               is to allow the user to select any give tree element from a tree structure
//               for any purpose necessary.
//               The recomended way to call this dialog is through DoModal()
//               method.


#include "stdafx.h"
#include "TreeElementSelectionDialog.h"

//////////////////////////////////////////////////////////////////////////
CTreeElementSelectionDialog::CTreeElementSelectionDialog(CString strCaption)
    : CDialog(IDD_GENERIC_TREE_SELECTION_DIALOG)
    , m_strCaption(strCaption)
    , m_boIsInitialized(false)
{
    // Defaults
    m_oFont.CreateFont(14, 0, 0, 0,
        FW_NORMAL, FALSE, FALSE, FALSE,
        ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
        DEFAULT_PITCH, "Arial");

    CMFCUtils::LoadTrueColorImageList(m_oImageList, IDB_ANIMATIONS_TREE, 16, RGB(255, 0, 255));
    m_oImageList.SetOverlayImage(1, 1);
}
//////////////////////////////////////////////////////////////////////////
void CTreeElementSelectionDialog::SetImageList(CImageList*  poSourceImageList)
{
    m_oImageList.DeleteImageList();
    m_oImageList.Create(poSourceImageList);
    if (m_boIsInitialized)
    {
        m_oTreeControl.SetImageList(&m_oImageList, TVSIL_NORMAL);
    }
}
//////////////////////////////////////////////////////////////////////////
void CTreeElementSelectionDialog::SetFont(CFont*    poSourceFont)
{
    LOGFONT stLogFont;
    memset(&stLogFont, 0, sizeof(LOGFONT));

    poSourceFont->GetLogFont(&stLogFont);
    m_oFont.DeleteObject();
    m_oFont.CreateFontIndirect(&stLogFont);

    if (m_boIsInitialized)
    {
        m_oTreeControl.SetFont(&m_oFont);
    }
}
//////////////////////////////////////////////////////////////////////////
void CTreeElementSelectionDialog::SetInitCallback(TDCallback        fnInitCallback)
{
    m_fnInitCallback = fnInitCallback;
}
//////////////////////////////////////////////////////////////////////////
void CTreeElementSelectionDialog::SetOkCallback(TDCallback          fnOkCallback)
{
    m_fnOkCallback = fnOkCallback;
}
//////////////////////////////////////////////////////////////////////////
void CTreeElementSelectionDialog::SetCancelCallback(TDCallback          fnCancelCallback)
{
    m_fnCancelCallback = fnCancelCallback;
}
//////////////////////////////////////////////////////////////////////////
void CTreeElementSelectionDialog::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_TREE, m_oTreeControl);
}
//////////////////////////////////////////////////////////////////////////
BOOL CTreeElementSelectionDialog::OnInitDialog()
{
    if (CDialog::OnInitDialog() == FALSE)
    {
        return FALSE;
    }

    SetWindowText(m_strCaption);

    m_oTreeControl.SetFont(&m_oFont);
    m_oTreeControl.SetImageList(&m_oImageList, TVSIL_NORMAL);

    m_boIsInitialized = true;

    if (m_fnInitCallback)
    {
        m_fnInitCallback(this);
    }

    return TRUE;
}
//////////////////////////////////////////////////////////////////////////
void CTreeElementSelectionDialog::OnOK()
{
    // We call this BEFORE the OnOk as we want the window to be still open.
    if (m_fnOkCallback)
    {
        m_fnOkCallback(this);
    }

    CDialog::OnOK();
}
//////////////////////////////////////////////////////////////////////////
void CTreeElementSelectionDialog::OnCancel()
{
    if (m_fnCancelCallback)
    {
        m_fnCancelCallback(this);
    }

    CDialog::OnCancel();
}
//////////////////////////////////////////////////////////////////////////
HTREEITEM   CTreeElementSelectionDialog::InsertItem(const string& rstrItemtext, HTREEITEM hParentItem, DWORD_PTR nItemData, int nItemUnselectedImage, int nItemSelectedImage)
{
    return InsertItem(rstrItemtext, hParentItem, TVI_LAST, nItemData, nItemUnselectedImage, nItemSelectedImage);
}
//////////////////////////////////////////////////////////////////////////
HTREEITEM   CTreeElementSelectionDialog::InsertItem(const string& rstrItemtext, HTREEITEM hParentItem, HTREEITEM hAfterWhatItem, DWORD_PTR nItemData, int nItemUnselectedImage, int nItemSelectedImage)
{
    // We can only successfully add items after
    HTREEITEM       hCurrentItem(NULL);

    if (!m_boIsInitialized)
    {
        return NULL;
    }

    hCurrentItem = m_oTreeControl.InsertItem(rstrItemtext.c_str(), nItemUnselectedImage, nItemSelectedImage, hParentItem, hAfterWhatItem);
    m_oTreeControl.SetItemData(hCurrentItem, nItemData);

    return hCurrentItem;
}
//////////////////////////////////////////////////////////////////////////
CTreeCtrl& CTreeElementSelectionDialog::GetTreeControl()
{
    return m_oTreeControl;
}
//////////////////////////////////////////////////////////////////////////
