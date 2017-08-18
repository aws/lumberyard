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

#ifndef CRYINCLUDE_EDITOR_VOLUMEPANEL_H
#define CRYINCLUDE_EDITOR_VOLUMEPANEL_H

#pragma once

#ifdef KDAB_PORT

// VolumePanel.h : header file
//
#include "ObjectPanel.h"

/////////////////////////////////////////////////////////////////////////////
// CVolumePanel dialog

class CVolumePanel
    : public CObjectPanel
{
    // Construction
public:
    CVolumePanel(CWnd* pParent = NULL);   // standard constructor

    // Dialog Data
    //{{AFX_DATA(CVolumePanel)
    enum
    {
        IDD = IDD_PANEL_VOLUME
    };
    // NOTE: the ClassWizard will add data members here
    //}}AFX_DATA

    void SetSize(Vec3& size);
    Vec3 GetSize();

    // Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CVolumePanel)
protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

    // Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(CVolumePanel)
    virtual BOOL OnInitDialog();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

    CNumberCtrl m_size[3];
};

#endif // KDAB_PORT

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // CRYINCLUDE_EDITOR_VOLUMEPANEL_H
