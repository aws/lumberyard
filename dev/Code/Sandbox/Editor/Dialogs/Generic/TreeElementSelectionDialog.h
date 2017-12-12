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
//               tree element selection  The purpose of this dialog  as one might imagine
//               is to allow the user to select any give tree element from a tree structure
//               for any purpose necessary
//               The recomended way to call this dialog is through DoModal  method  but don t
//               check its return value  Instead  set up callbacks  If you don t set up the
//               callbacks  it may be troublesome to set up the initial data and to get data
//               from the dialog as well
//
// Usage Hint: rename dialog.


#ifndef CRYINCLUDE_EDITOR_DIALOGS_GENERIC_TREEELEMENTSELECTIONDIALOG_H
#define CRYINCLUDE_EDITOR_DIALOGS_GENERIC_TREEELEMENTSELECTIONDIALOG_H
#pragma once


#pragma once;

#include "functor.h"

class CTreeElementSelectionDialog
    : public CDialog
{
    //////////////////////////////////////////////////////////////////////////
    // Types and typedefs.
public:
    typedef Functor1<CTreeElementSelectionDialog*>  TDCallback;
protected:
private:
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // Methods.
public:
    CTreeElementSelectionDialog(CString strCaption);

    // The below 2 functions make a COPY of the elements...
    // ... so don't expect them to be updated if you change them after
    // Setting them.
    void SetImageList(CImageList*   poSourceImageList);
    void SetFont(CFont* poSourceFont);

    void SetInitCallback(TDCallback     fnInitCallback);
    void SetOkCallback(TDCallback           fnOkCallback);
    void SetCancelCallback(TDCallback           fnCancelCallback);

    void DoDataExchange(CDataExchange* pDX);
    BOOL OnInitDialog();
    void OnOK();
    void OnCancel();

    // This functions must be called after either, the create method is called, or better, the do modal is.
    HTREEITEM   InsertItem(const string& rstrItemtext, HTREEITEM hParentItem = TVI_ROOT, DWORD_PTR nItemData = 0, int nItemUnselectedImage = 0, int nItemSelectedImage = 0);
    HTREEITEM   InsertItem(const string& rstrItemtext, HTREEITEM hParentItem = TVI_ROOT, HTREEITEM hAfterWhatItem = TVI_LAST, DWORD_PTR nItemData = 0, int nItemUnselectedImage = 0, int nItemSelectedImage = 0);

    CTreeCtrl& GetTreeControl();
protected:
private:
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // Data.
public:
protected:
    CString             m_strCaption;
    CTreeCtrl           m_oTreeControl;
    bool                    m_boIsInitialized;
    TDCallback      m_fnInitCallback;
    TDCallback      m_fnOkCallback;
    TDCallback      m_fnCancelCallback;
    CFont                   m_oFont;
    CImageList      m_oImageList;
private:
    //////////////////////////////////////////////////////////////////////////
};

#endif // CRYINCLUDE_EDITOR_DIALOGS_GENERIC_TREEELEMENTSELECTIONDIALOG_H
