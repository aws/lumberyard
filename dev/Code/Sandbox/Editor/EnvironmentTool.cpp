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

// Description : Places Environment on terrain.


#include "StdAfx.h"
#include "EnvironmentTool.h"
#include "EnvironmentPanel.h"
#include "MainWindow.h"

//////////////////////////////////////////////////////////////////////////
CEnvironmentTool::CEnvironmentTool()
{
    SetStatusText(tr("Click Apply to accept changes"));
    m_panelId = 0;
    m_panel = 0;
}

//////////////////////////////////////////////////////////////////////////
CEnvironmentTool::~CEnvironmentTool()
{
}

//////////////////////////////////////////////////////////////////////////
void CEnvironmentTool::BeginEditParams(IEditor* ie, int flags)
{
    m_ie = ie;
    if (!m_panelId)
    {
        m_panel = new CEnvironmentPanel;
        m_panelId = m_ie->AddRollUpPage(ROLLUP_TERRAIN, "Environment", m_panel);
        MainWindow::instance()->setFocus();
    }
}

//////////////////////////////////////////////////////////////////////////
void CEnvironmentTool::EndEditParams()
{
    if (m_panelId)
    {
        m_ie->RemoveRollUpPage(ROLLUP_TERRAIN, m_panelId);
        m_panel = 0;
    }
}

#include <EnvironmentTool.moc>