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

// Description : implementation file


#include "stdafx.h"
#include "ToolBarContainer.h"

/////////////////////////////////////////////////////////////////////////////
// CToolBarContainer

CToolBarContainer::CToolBarContainer()
{
}

CToolBarContainer::~CToolBarContainer()
{
}


BEGIN_MESSAGE_MAP(CToolBarContainer, CWnd)
//{{AFX_MSG_MAP(CToolBarContainer)
ON_WM_PAINT()
//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CToolBarContainer message handlers

void CToolBarContainer::OnPaint()
{
    CPaintDC dc(this); // device context for painting
    CRect rect;
    CBrush cFillBrush;

    // Paint the window with a dark gray bursh

    // Get the rect of the client window
    GetClientRect(&rect);

    // Create the brush
    cFillBrush.CreateStockObject(BLACK_BRUSH);

    // Fill the entire client area
    dc.FillRect(&rect, &cFillBrush);

    // Do not call CView::OnPaint() for painting messages
}
