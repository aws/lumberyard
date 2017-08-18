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

#ifndef CRYINCLUDE_EDITOR_TERRAINFORMULADLG_H
#define CRYINCLUDE_EDITOR_TERRAINFORMULADLG_H

#pragma once
// TerrainFormulaDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CTerrainFormulaDlg dialog

class CTerrainFormulaDlg
    : public CDialog
{
    // Construction
public:
    CTerrainFormulaDlg(CWnd* pParent = NULL);   // standard constructor

    // Dialog Data
    //{{AFX_DATA(CTerrainFormulaDlg)
    enum
    {
        IDD = IDD_TERRAIN_FORMULA
    };
    double  m_dParam1;
    double  m_dParam2;
    double  m_dParam3;
    //}}AFX_DATA


    // Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CTerrainFormulaDlg)
protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

    // Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(CTerrainFormulaDlg)
    // NOTE: the ClassWizard will add member functions here
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // CRYINCLUDE_EDITOR_TERRAINFORMULADLG_H
