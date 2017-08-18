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

#ifndef CRYINCLUDE_EDITOR_DRAWWND_H
#define CRYINCLUDE_EDITOR_DRAWWND_H

#pragma once

// DrawWnd.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDrawWnd window

class CTerrainDialog;

class CDrawWnd
    : public CWnd
{
    // Construction
public:
    CDrawWnd();

    // Attributes
public:

    // Operations
public:

    // Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CDrawWnd)
    //}}AFX_VIRTUAL

    // Implementation
public:
    CPoint SelectPoint();
    bool Create(CWnd* pwndParent);
    virtual ~CDrawWnd();

    void SetCurrentBrush(UINT iBrush) { m_iCurBrush = iBrush; };
    UINT GetCurBrush() { return m_iCurBrush; };

    void SetCoordinateWindow(CWnd* pNewCoordWnd) { m_pCoordWnd = pNewCoordWnd; };

    void SetShowWater(bool bShow) { m_bShowWater = bShow; };
    bool GetShowWater() { return m_bShowWater; };

    void SetShowMapObj(bool bShow) { m_bShowMapObj = bShow; };
    bool GetShowMapObj() { return m_bShowMapObj; };

    CPoint GetMarkerPos() { return m_ptMarker; };
    void SetMarkerPos(CPoint ptMarker) { m_ptMarker =  ptMarker; };

    // Set to -1.0f to disable the set to height feature and use a normal brush instead
    void SetSetToHeight(float fHeight) { m_fSetToHeight = fHeight; };
    float GetSetToHeight() { return m_fSetToHeight; };

    // Generated message map functions
protected:
    // Needs access to the message map
    friend CTerrainDialog;

    //{{AFX_MSG(CDrawWnd)
    afx_msg void OnPaint();
    afx_msg void OnMouseMove(UINT nFlags, CPoint point);
    afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
    afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnMButtonDown(UINT nFlags, CPoint point);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

private:
    // Scrolling / zooming related
    bool SetScrollOffset(CPoint cNewOffset) { return SetScrollOffset(cNewOffset.x, cNewOffset.y); };
    bool SetScrollOffset(long iX, long iY);
    bool SetZoomFactor(float fZoomFactor);
    CPoint GetScrollOffset() { return m_cScrollOffset; };
    float GetZoomFactor() { return m_fZoomFactor; };
    void WndCoordToHMCoord(CPoint* pWndPt);
    void HMCoordToWndCoord(CPoint* pWndPt);
    void SetUseNoiseBrush(bool bUse) { m_bUseNoiseBrush = bUse; };

    // Misc
    void MakeAlpha(CBitmap& ioBM);
    void SetOpacity(float fOpacity) { m_fOpacity = fOpacity; };

    // Scrolling / zooming related
    CPoint m_cMouseDownPos;
    CPoint m_cScrollOffset;
    float m_fZoomFactor;

    // Misc
    CBitmap m_bmpBrushes;
    CDC m_dcBrushes;
    UINT m_iCurBrush;
    DWORD* m_pWaterTexData;
    float m_fOpacity;
    CWnd* m_pCoordWnd;
    bool m_bShowWater;
    bool m_bShowMapObj;
    float m_fSetToHeight;
    bool m_bUseNoiseBrush;
    CPoint m_ptMarker;
    CHeightmap* m_heightmap;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // CRYINCLUDE_EDITOR_DRAWWND_H

