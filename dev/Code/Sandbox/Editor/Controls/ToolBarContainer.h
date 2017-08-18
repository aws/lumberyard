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

#ifndef CRYINCLUDE_EDITOR_CONTROLS_TOOLBARCONTAINER_H
#define CRYINCLUDE_EDITOR_CONTROLS_TOOLBARCONTAINER_H
#pragma once

// ToolBarContainer.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CToolBarContainer window

class CToolBarContainer
    : public CWnd
{
    // Construction
public:
    CToolBarContainer();

    // Attributes
public:

    // Operations
public:

    // Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CToolBarContainer)
    //}}AFX_VIRTUAL

    // Implementation
public:
    virtual ~CToolBarContainer();

    // Generated message map functions
protected:
    //{{AFX_MSG(CToolBarContainer)
    afx_msg void OnPaint();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // CRYINCLUDE_EDITOR_CONTROLS_TOOLBARCONTAINER_H
