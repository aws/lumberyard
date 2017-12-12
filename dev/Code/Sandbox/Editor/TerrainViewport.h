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

#ifndef CRYINCLUDE_EDITOR_TERRAINVIEWPORT_H
#define CRYINCLUDE_EDITOR_TERRAINVIEWPORT_H

#pragma once
// TerrainViewport.h : header file
//

#include "Viewport.h"

/////////////////////////////////////////////////////////////////////////////
// CTerrainViewport window

class CTerrainViewport
    : public MFCViewport
{
    // Construction
public:
    CTerrainViewport();

    // Attributes
public:

    // Operations
public:

    virtual void ResetContent();
    virtual void UpdateContent(int flags);

    //virtual void Update();

    //! Map world space position to viewport position.
    virtual POINT     WorldToView(const Vec3& wp) const;
    //! Map viewport position to world space position.
    virtual Vec3        ViewToWorld(CPoint vp, bool* collideWithTerrain = 0, bool onlyTerrain = false, bool bSkipVegetation = false, bool bTestRenderMesh = false) const;
    virtual void        ViewToWorldRay(CPoint vp, Vec3& raySrc, Vec3& rayDir) const;

    //! Map point from view space to heightmap space.
    CPoint ViewToHeightmap(CPoint p);
    //! Map point from heightmap space to view.
    CPoint HeightmapToView(CPoint p);

    //virtual void OnTitleMenu( CMenu &menu );
    //virtual void OnTitleMenuCommand( int id );


    // Scrolling / zooming related
    bool SetScrollOffset(CPoint cNewOffset) { return SetScrollOffset(cNewOffset.x, cNewOffset.y); };
    bool SetScrollOffset(long iX, long iY);
    CPoint GetScrollOffset() const { return m_cScrollOffset; };

    void SetZoomFactor(float fZoomFactor);

    // Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CTerrainViewport)
    //}}AFX_VIRTUAL

    // Implementation
public:
    virtual ~CTerrainViewport();

    // Generated message map functions
protected:
    //{{AFX_MSG(CTerrainViewport)
    afx_msg void OnPaint();
    afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
    afx_msg void OnMouseMove(UINT nFlags, CPoint point);
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

    void DrawStaticObjects(DWORD* pBits, int trgWidth, int trgHeight, CRect rc);


    CPoint m_cMousePos;
    // Defined in Viewoport CPoint m_cMouseDownPos;
    CPoint m_cPrevMousePos;

    float m_prevZoomFactor;
    CSize m_prevScrollOffset;

    CPoint m_cScrollOffset;
    CSize m_heightmapSize;

    CBitmap m_bmpTerrain;
    CDC m_dcTerrain;

    CBitmap m_bmpView;
    CDC m_dcView;

    int m_iBrushSize;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // CRYINCLUDE_EDITOR_TERRAINVIEWPORT_H
