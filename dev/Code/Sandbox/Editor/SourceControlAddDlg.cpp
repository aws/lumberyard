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
#include "SourceControlAddDlg.h"
#include <ui_SourceControlAddDlg.h>

/////////////////////////////////////////////////////////////////////////////
// CSourceControlAddDlg dialog


CSourceControlAddDlg::CSourceControlAddDlg(const QString& sFilename, QWidget* pParent /*=NULL*/)
    : QDialog(pParent), m_scFlags(NONE), m_sFilename(sFilename), ui(new Ui::CSourceControlAddDlg)
{
    ui->setupUi(this);

    ui->m_labelFilename->setText(m_sFilename);

    connect(ui->m_buttonCancel, &QPushButton::clicked, this, &QDialog::reject);
    connect(ui->m_buttonAddDefault, &QPushButton::clicked, this, &CSourceControlAddDlg::OnBnClickedAddDefault);
    connect(ui->m_buttonOk, &QPushButton::clicked, this, &CSourceControlAddDlg::OnBnClickedOk);
}

CSourceControlAddDlg::~CSourceControlAddDlg()
{
}


/////////////////////////////////////////////////////////////////////////////
// CSourceControlAddDlg message handlers

void CSourceControlAddDlg::OnBnClickedOk()
{
    m_sDesc = ui->m_editDesc->text();
    if (!m_sDesc.isEmpty())
    {
        accept();
    }
}

void CSourceControlAddDlg::OnBnClickedAddDefault()
{
    m_scFlags = ADD_WITHOUT_SUBMIT;
    accept();
}

#include <SourceControlAddDlg.moc>
