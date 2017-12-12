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

// Description : Class to assist with setting easy to use scroll bars for
//               any CWnd inherited control


#ifndef CRYINCLUDE_EDITOR_UTIL_SCROLLHELPER_H
#define CRYINCLUDE_EDITOR_UTIL_SCROLLHELPER_H
#pragma once



// Usage example:
// Suppose the following CMyWnd. CMyWndBase inherits in some way from CWnd.
// We want to add _only_ a vertical slider to CMyWnd:
//
// class CMyWnd : public CMyWndBase
// {
//  ..
//  CScrollHelper m_scrollHelper;
// };
//
// -Disable horizontal scrolling on m_scrollHelper:
//    m_scrollHelper.SetAllowed( CScrollHelper::HORIZONTAL, false );
//
// -Register the instance of CMyWnd with m_scrollHelper:
//    Call m_scrollHelper.SetWnd( this ) from CMyWnd::OnInitDialog()
//
// -Notify CScrollHelper of the size that CMyWnd would need to be to display the full area:
//    Call m_scrollHelper.SetDesiredClientSize(..)
//
// -Notify m_scrollHelper of changes to the controlled window size:
//    Add ON_WM_SIZE() to the message map for CMyWnd, and call m_scrollHelper.OnSize(..)
//    from CMyWnd::OnSize(..)
//
// -Notify m_scrollHelper of scroll events (in this example, only VScroll):
//    Add ON_WM_VSCROLL() to the message map for CMyWnd, and call m_scrollHelper.OnVScroll(..)
//    from CMyWnd::OnVScroll(..)


class CScrollHelper
{
public:
    enum BarId
    {
        HORIZONTAL,
        VERTICAL
    };

    CScrollHelper();
    ~CScrollHelper();

    void SetWnd(CWnd* pWnd);

    void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
    void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
    void OnSize(UINT nType, int cx, int cy);

    void SetDesiredClientSize(const CRect& desiredClientSize);
    void SetAutoScrollWindow(bool autoScrollWindow);
    void ResetScroll();
    void SetAllowed(BarId barId, bool allowed);

    int GetScrollAmount(BarId barId) const;

private:
    void OnClientSizeUpdated();
    void UpdateScrollBars();

    void UpdateScrollBar(BarId barId, int desiredSize, int viewSize);

    void OnScroll(BarId barId, UINT nSBCode, UINT nPos);

    void Update();

private:
    CRect m_desiredClientSize;

    bool m_autoScrollWindow;

    CWnd* m_pWnd;

    bool m_visible[ 2 ];
    int m_totalScroll[ 2 ];
    int m_winBarId[ 2 ];
    bool m_allowed[ 2 ];
};

#endif // CRYINCLUDE_EDITOR_UTIL_SCROLLHELPER_H
