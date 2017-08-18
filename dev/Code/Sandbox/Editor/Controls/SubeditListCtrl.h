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

#ifndef CRYINCLUDE_EDITOR_CONTROLS_SUBEDITLISTCTRL_H
#define CRYINCLUDE_EDITOR_CONTROLS_SUBEDITLISTCTRL_H
#pragma once

// SubeditListView.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CSubeditListCtrl view

class CSubEdit
    : public CEdit
{
    // Construction
public:
    CSubEdit();

    // Attributes
public:

    // Operations
public:

    // Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CSubEdit)
    //}}AFX_VIRTUAL

    // Implementation
public:
    int m_x;
    int m_cx;
    virtual ~CSubEdit();

    // Generated message map functions
protected:
    //{{AFX_MSG(CSubEdit)
    afx_msg void OnWindowPosChanging(WINDOWPOS FAR* lpwndpos);
    //}}AFX_MSG

    DECLARE_MESSAGE_MAP()
};

class CSubeditListCtrl
    : public CListCtrl
{
    class ComboBoxEditSession
        : public CComboBox
    {
    public:
        enum
        {
            ID_COMBO_BOX = 40000
        };

        ComboBoxEditSession();

        bool IsRunning() const;
        void Begin(CSubeditListCtrl* list, int itemIndex, int subItemIndex, const std::vector<string>& options, const string& currentOption);
        void End(bool accept);

        afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
        DECLARE_MESSAGE_MAP()

    private:
        CSubeditListCtrl* m_list;
        int m_itemIndex;
        int m_subItemIndex;
    };

protected:

    CSubeditListCtrl();           // protected constructor used by dynamic creation

    // Attributes
public:

    enum EditStyle
    {
        EDIT_STYLE_NONE,
        EDIT_STYLE_EDIT,
        EDIT_STYLE_COMBO
    };

    // Operations
public:
    CSubEdit m_editWnd;
    int m_item;
    int m_subitem;


    // Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CSubeditListCtrl)
public:
protected:
    virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
    //}}AFX_VIRTUAL

    // Implementation
protected:
    virtual ~CSubeditListCtrl();
#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif

    // Generated message map functions
protected:
    //{{AFX_MSG(CSubeditListCtrl)
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnBeginLabelEdit(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnEndLabelEdit(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnPaint();
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnComboSelChange();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

    virtual void TextChanged(int item, int subitem, const char* szText);
    virtual void GetOptions(int item, int subitem, std::vector<string>& options, string& currentOption) = 0;
    virtual EditStyle GetEditStyle(int item, int subitem) = 0;

    void EditLabelCombo(int itemIndex, int subItemIndex, const std::vector<string>& options, const string& currentOption);

    ComboBoxEditSession m_comboBoxEditSession;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // CRYINCLUDE_EDITOR_CONTROLS_SUBEDITLISTCTRL_H
