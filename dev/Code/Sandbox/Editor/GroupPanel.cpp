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
#include "GroupPanel.h"
#include <ui_GroupPanel.h>
#include "CryEdit.h"

#include "Objects/Group.h"

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// CGroupPanel dialog
//////////////////////////////////////////////////////////////////////////
CGroupPanel::CGroupPanel(CGroup* obj, QWidget* pParent /* = nullptr */)
    : QWidget(pParent)
    , m_obj(obj)
    , ui(new Ui::CGroupPanel)
{
    ui->setupUi(this);
    GetIEditor()->RegisterNotifyListener(this);
    m_GroupBn = ui->GROUP;
    connect(m_GroupBn, &QPushButton::clicked, this, &CGroupPanel::OnGroup);
    m_OpenBn = ui->OPEN;
    connect(m_OpenBn, &QPushButton::clicked, this, &CGroupPanel::OnOpen);
    m_AttachBn = ui->ATTACH;
    connect(m_AttachBn, &QPushButton::clicked, this, &CGroupPanel::OnAttach);
    m_UngroupBn = ui->UNGROUP;
    connect(m_UngroupBn, &QPushButton::clicked, this, &CGroupPanel::OnUngroup);
    m_CloseBn = ui->CLOSE;
    connect(m_CloseBn, &QPushButton::clicked, this, &CGroupPanel::OnCloseGroup);
    m_DetachBn = ui->DETACH;
    connect(m_DetachBn, &QPushButton::clicked, this, &CGroupPanel::OnDetach);

    UpdateButtons();
}

CGroupPanel::~CGroupPanel()
{
    GetIEditor()->UnregisterNotifyListener(this);
}

//////////////////////////////////////////////////////////////////////////
void CGroupPanel::OnGroup()
{
    CCryEditApp::instance()->OnGroupMake();
}

//////////////////////////////////////////////////////////////////////////
void CGroupPanel::OnOpen()
{
    CCryEditApp::instance()->OnGroupOpen();
    UpdateButtons();
}

//////////////////////////////////////////////////////////////////////////
void CGroupPanel::OnAttach()
{
    CCryEditApp::instance()->OnGroupAttach();
    //UpdateButtons();
}

//////////////////////////////////////////////////////////////////////////
void CGroupPanel::OnUngroup()
{
    CCryEditApp::instance()->OnGroupUngroup();
}

//////////////////////////////////////////////////////////////////////////
void CGroupPanel::OnCloseGroup()
{
    CCryEditApp::instance()->OnGroupClose();
    UpdateButtons();
}

//////////////////////////////////////////////////////////////////////////
void CGroupPanel::OnDetach()
{
    CCryEditApp::instance()->OnGroupDetach();
    UpdateButtons();
}

//////////////////////////////////////////////////////////////////////////
void CGroupPanel::UpdateButtons()
{
    if (!m_obj)
    {
        return;
    }

    m_OpenBn->setEnabled(m_obj->IsOpen() ? FALSE : TRUE);
    m_CloseBn->setEnabled(m_obj->IsOpen() ? TRUE : FALSE);
    m_DetachBn->setEnabled(m_obj->GetParent() ? TRUE : FALSE);
}

//////////////////////////////////////////////////////////////////////////
void CGroupPanel::OnEditorNotifyEvent(EEditorNotifyEvent event)
{
    switch (event)
    {
    case eNotify_OnEditToolChange:
        UpdateButtons();
        break;
    }
}