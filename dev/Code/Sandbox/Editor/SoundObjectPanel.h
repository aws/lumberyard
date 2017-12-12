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

#ifndef CRYINCLUDE_EDITOR_SOUNDOBJECTPANEL_H
#define CRYINCLUDE_EDITOR_SOUNDOBJECTPANEL_H

#pragma once
// SoundObjectPanel.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CSoundObjectPanel dialog

class CSoundObjectPanel
    : public CDialog
{
    // Construction
public:
    CSoundObjectPanel(CWnd* pParent = NULL);   // standard constructor

    // Dialog Data
    //{{AFX_DATA(CSoundObjectPanel)
    enum
    {
        IDD = IDD_PANEL_SOUNDOBJECT
    };
    //}}AFX_DATA

    void SetSound(class CSoundObject* sound);

    // Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CSoundObjectPanel)
protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

    CNumberCtrl m_outerRadius;
    CNumberCtrl m_innerRadius;
    CNumberCtrl m_volume;

    // Implementation
protected:
    virtual void OnOK() {};
    virtual void OnCancel() {};

    // Generated message map functions
    //{{AFX_MSG(CSoundObjectPanel)
    virtual BOOL OnInitDialog();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

    CSoundObject * m_sound;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // CRYINCLUDE_EDITOR_SOUNDOBJECTPANEL_H
