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

#include "stdafx.h"
#include "Resource.h"
#include "TelemetryDialog.h"
#include "TelemetryViewClass.h"

//////////////////////////////////////////////////////////////////////////

#define TELEM_DIALOGFRAME_CLASSNAME "TelemetryDialog"

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE(CTelemetryDialog, CWnd)

BEGIN_MESSAGE_MAP(CTelemetryDialog, CWnd)
ON_WM_SIZE()
END_MESSAGE_MAP()

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

CTelemetryDialog::CTelemetryDialog()
    : m_connectionPanel(m_repository)
{
    Create(NULL, NULL, WS_CHILD | WS_VISIBLE, CRect(0, 0, 1, 1), AfxGetMainWnd(), 0);

    m_rollupCtrl.Create(WS_VISIBLE | WS_CHILD, CRect(0, 0, 1, 1), this, NULL);

    m_connectionPanel.Create(CConnectionPanel::IDD, &m_rollupCtrl);
    m_rollupCtrl.InsertPage("StatsTool", &m_connectionPanel);
}

//////////////////////////////////////////////////////////////////////////

CTelemetryDialog::~CTelemetryDialog()
{
}

//////////////////////////////////////////////////////////////////////////

void CTelemetryDialog::OnSize(UINT nType, int cx, int cy)
{
    CWnd::OnSize(nType, cx, cy);

    RECT rcRollUp;
    GetClientRect(&rcRollUp);

    if (m_rollupCtrl.m_hWnd)
    {
        m_rollupCtrl.MoveWindow(rcRollUp.left, rcRollUp.top, rcRollUp.right, rcRollUp.bottom, FALSE);
    }
}

//////////////////////////////////////////////////////////////////////////

void CTelemetryDialog::RegisterViewClass()
{
    GetIEditor()->GetClassFactory()->RegisterClass(new CTelemetryViewClass);
}

//////////////////////////////////////////////////////////////////////////


