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

#include "StdAfx.h"
#include "SoundObjectPanel.h"
#include "Objects/SoundObject.h"

/////////////////////////////////////////////////////////////////////////////
// CSoundObjectPanel dialog


CSoundObjectPanel::CSoundObjectPanel(CWnd* pParent /*=NULL*/)
    : CDialog(CSoundObjectPanel::IDD, pParent)
{
    //{{AFX_DATA_INIT(CSoundObjectPanel)
    //}}AFX_DATA_INIT

    Create(IDD, pParent);
}


void CSoundObjectPanel::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CSoundObjectPanel)
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSoundObjectPanel, CDialog)
//{{AFX_MSG_MAP(CSoundObjectPanel)
//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSoundObjectPanel message handlers

BOOL CSoundObjectPanel::OnInitDialog()
{
    CDialog::OnInitDialog();

    m_innerRadius.Create(this, IDC_INNER_RADIUS);
    m_innerRadius.SetRange(1, 10000);

    m_outerRadius.Create(this, IDC_OUTER_RADIUS);
    m_outerRadius.SetRange(1, 10000);

    m_volume.Create(this, IDC_VOLUME);
    m_volume.SetRange(0, 1);

    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}