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


#include "StdAfx.h"
#include "MissionSelectDialog.h"
#include "ui_MissionSelectDialog.h"
#include "Mission.h"
#include "CryEditDoc.h"
#include "QtUtil.h"

/////////////////////////////////////////////////////////////////////////////
// CMissionSelectDialog dialog


CMissionSelectDialog::CMissionSelectDialog(QWidget* pParent /*=nullptr*/)
    : QDialog(pParent)
    , ui(new Ui::CMissionSelectDialog)
{
    ui->setupUi(this);
    m_missions = ui->MISSIONS;

    connect(ui->BTNOK, &QPushButton::clicked, this, &CMissionSelectDialog::accept);
    connect(ui->BTNCANCEL, &QPushButton::clicked, this, &CMissionSelectDialog::reject);
    connect(ui->MISSIONS, &QListWidget::itemSelectionChanged, this, &CMissionSelectDialog::OnSelectMission);
    connect(ui->MISSIONS, &QListWidget::itemDoubleClicked, this, &CMissionSelectDialog::OnDblclkMissions);
    connect(ui->MISSIONS, &QListWidget::itemSelectionChanged, this, &CMissionSelectDialog::OnSelectMission);

    CCryEditDoc* doc = GetIEditor()->GetDocument();

    for (int i = 0; i < doc->GetMissionCount(); i++)
    {
        CMission* m = doc->GetMission(i);
        m_missions->addItem(m->GetName());
        m_descriptions << m->GetDescription();
    }

    if (doc->GetMissionCount() > 0)
    {
        // Select first mission.
        m_missions->item(0)->setSelected(true);
    }
}

CMissionSelectDialog::~CMissionSelectDialog()
{
}

/////////////////////////////////////////////////////////////////////////////
// CMissionSelectDialog message handlers

void CMissionSelectDialog::accept()
{
    CCryEditDoc* doc = GetIEditor()->GetDocument();
    for (int i = 0; i < doc->GetMissionCount(); i++)
    {
        CMission* m = doc->GetMission(i);
        m->SetDescription(m_descriptions[i]);
    }

    QDialog::accept();
}

void CMissionSelectDialog::reject()
{
    QDialog::reject();
}

void CMissionSelectDialog::OnSelectMission()
{
    // TODO: Add your control notification handler code here
    QModelIndexList selection = m_missions->selectionModel()->selectedRows();
    if (!selection.isEmpty())
    {
        m_description = m_descriptions[selection.front().row()];
    }
}

void CMissionSelectDialog::OnDblclkMissions()
{
    accept();
}

void CMissionSelectDialog::OnUpdateDescription()
{
    QModelIndexList selection = m_missions->selectionModel()->selectedRows();
    if (!selection.isEmpty())
    {
        m_descriptions[selection.front().row()] = m_description;
    }
}
