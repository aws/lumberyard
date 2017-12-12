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
#include "CheckOutDialog.h"
#include "ui_CheckOutDialog.h"


// CCheckOutDialog dialog

CCheckOutDialog::CCheckOutDialog(const QString& file, QWidget* pParent)
    : QDialog(pParent)
    , m_ui(new Ui::CheckOutDialog)
{
    m_ui->setupUi(this);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    m_file = file;

    m_ui->icon->setPixmap(style()->standardIcon(QStyle::SP_MessageBoxQuestion).pixmap(m_ui->icon->width()));

    OnInitDialog();

    connect(m_ui->buttonCheckout, &QPushButton::clicked, this, &CCheckOutDialog::OnBnClickedCheckout);
    connect(m_ui->buttonOverwriteAll, &QPushButton::clicked, this, &CCheckOutDialog::OnBnClickedOverwriteAll);
    connect(m_ui->buttonOk, &QPushButton::clicked, this, &CCheckOutDialog::OnBnClickedOk);
}

CCheckOutDialog::~CCheckOutDialog()
{
}

//////////////////////////////////////////////////////////////////////////
// CCheckOutDialog message handlers
void CCheckOutDialog::OnBnClickedCheckout()
{
    // Check out this file.
    done(CHECKOUT);
}

//////////////////////////////////////////////////////////////////////////
void CCheckOutDialog::OnBnClickedOk()
{
    // Overwrite this file.
    done(OVERWRITE);
}

//////////////////////////////////////////////////////////////////////////
void CCheckOutDialog::OnBnClickedOverwriteAll()
{
    done(OVERWRITE_ALL);
    InstanceIsForAll() = true;
}

//////////////////////////////////////////////////////////////////////////
void CCheckOutDialog::OnInitDialog()
{
    setWindowTitle(tr("%1 is read-only").arg(Path::GetFile(m_file)));

    m_ui->m_text->setText(tr("%1 is read-only file, can be under Source Control.").arg(m_file));

    m_ui->buttonOverwriteAll->setEnabled(InstanceEnableForAll());

    m_ui->buttonCheckout->setEnabled(GetIEditor()->IsSourceControlAvailable());

    adjustSize();
}


//static ////////////////////////////////////////////////////////////////
bool& CCheckOutDialog::InstanceEnableForAll()
{
    static bool isEnableForAll = false;
    return isEnableForAll;
}

//static ////////////////////////////////////////////////////////////////
bool& CCheckOutDialog::InstanceIsForAll()
{
    static bool isForAll = false;
    return isForAll;
}

//static ////////////////////////////////////////////////////////////////
bool CCheckOutDialog::EnableForAll(bool isEnable)
{
    bool bPrevEnable = InstanceEnableForAll();
    InstanceEnableForAll() = isEnable;
    if (!bPrevEnable || !isEnable)
    {
        InstanceIsForAll() = false;
    }
    return bPrevEnable;
}

#include <CheckoutDialog.moc>