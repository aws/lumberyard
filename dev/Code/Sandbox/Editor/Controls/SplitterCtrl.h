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

// Description : CSplitterCtrl (former CSplitterWndEx) class.


#ifndef CRYINCLUDE_EDITOR_CONTROLS_SPLITTERCTRL_H
#define CRYINCLUDE_EDITOR_CONTROLS_SPLITTERCTRL_H
#pragma once

class CSplitterCtrl
    : public CSplitterWnd
{
public:
    DECLARE_DYNAMIC(CSplitterCtrl)

    CSplitterCtrl();
    ~CSplitterCtrl();

    virtual CWnd* GetActivePane(int* pRow = NULL, int* pCol = NULL);
    //! Assign any Window to splitter window pane.
    void SetPane(int row, int col, CWnd* pWnd, SIZE sizeInit);
    // Override this for flat look.
    void OnDrawSplitter(CDC* pDC, ESplitType nType, const CRect& rectArg);
    void SetNoBorders();
    void SetTrackable(bool bTrackable);

protected:
    DECLARE_MESSAGE_MAP()
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);

    // override CSplitterWnd special command routing to frame
    virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
    virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
    bool m_bTrackable;
};
#endif // CRYINCLUDE_EDITOR_CONTROLS_SPLITTERCTRL_H
