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
#include "VolumePanel.h"

/////////////////////////////////////////////////////////////////////////////
// CVolumePanel dialog

#ifdef KDAB_PORT

CVolumePanel::CVolumePanel(CWnd* pParent /*=NULL*/)
    : CXTResizeDialog(pParent)
{
    //{{AFX_DATA_INIT(CVolumePanel)
    // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
}


void CVolumePanel::DoDataExchange(CDataExchange* pDX)
{
    CObjectPanel::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CVolumePanel)
    // NOTE: the ClassWizard will add DDX and DDV calls here
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CVolumePanel, CObjectPanel)
//{{AFX_MSG_MAP(CVolumePanel)
ON_EN_UPDATE(IDC_LENGTH, OnUpdate)
ON_EN_UPDATE(IDC_WIDTH, OnUpdate)
ON_EN_UPDATE(IDC_HEIGHT, OnUpdate)
//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CVolumePanel message handlers

BOOL CVolumePanel::OnInitDialog()
{
    CObjectPanel::OnInitDialog();

    m_size[0].Create(this, IDC_LENGTH);
    m_size[1].Create(this, IDC_WIDTH);
    m_size[2].Create(this, IDC_HEIGHT);

    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

void CVolumePanel::SetSize(Vec3& size)
{
    m_size[0].SetValue(size.x);
    m_size[1].SetValue(size.y);
    m_size[2].SetValue(size.z);
}

Vec3 CVolumePanel::GetSize()
{
    return Vec3(m_size[0].GetValue(), m_size[1].GetValue(), m_size[2].GetValue());
}

#endif // KDAB_PORT