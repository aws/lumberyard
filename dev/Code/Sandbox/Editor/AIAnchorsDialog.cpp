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
#include "AIAnchorsDialog.h"
#include "AI/AIManager.h"

#include <ui_AIAnchorsDialog.h>

#include <QMessageBox>
#include <QStringListModel>

// CAIAnchorsDialog dialog

CAIAnchorsDialog::CAIAnchorsDialog(QWidget* pParent /*=NULL*/)
    : QDialog(pParent)
    , m_anchorsModel(new QStringListModel(this))
    , ui(new Ui::CAIAnchorsDialog)
{
    m_sAnchor = "";

    ui->setupUi(this);
    ui->m_wndAnchorsList->setModel(m_anchorsModel);

    OnInitDialog();

    connect(ui->m_newButton, &QPushButton::clicked, this, &CAIAnchorsDialog::OnNewBtn);
    connect(ui->m_editButton, &QPushButton::clicked, this, &CAIAnchorsDialog::OnEditBtn);
    connect(ui->m_deleteButton, &QPushButton::clicked, this, &CAIAnchorsDialog::OnDeleteBtn);
    connect(ui->m_refreshButton, &QPushButton::clicked, this, &CAIAnchorsDialog::OnRefreshBtn);
    connect(ui->m_wndAnchorsList, &QListView::doubleClicked, this, &CAIAnchorsDialog::OnLbnDblClk);
    connect(ui->m_wndAnchorsList->selectionModel(), &QItemSelectionModel::selectionChanged, this, &CAIAnchorsDialog::OnLbnSelchangeAnchors);
    connect(ui->m_buttonBox, &QDialogButtonBox::accepted, this, &CAIAnchorsDialog::accept);
    connect(ui->m_buttonBox, &QDialogButtonBox::rejected, this, &CAIAnchorsDialog::reject);
}

CAIAnchorsDialog::~CAIAnchorsDialog()
{
}

// CAIAnchorsDialog message handlers

void CAIAnchorsDialog::OnNewBtn()
{
    CFileUtil::EditTextFile("Scripts\\AI\\Anchor.lua");
}

void CAIAnchorsDialog::OnEditBtn()
{
    CFileUtil::EditTextFile("Scripts\\AI\\Anchor.lua");
}

void CAIAnchorsDialog::OnDeleteBtn()
{
    QString msg = tr("Do you really want to delete AI Anchor Type %1?").arg(m_sAnchor);
    if (QMessageBox::question(this, tr("Editor"), msg) == QMessageBox::Yes)
    {
        CAIManager* pAIMgr = GetIEditor()->GetAI();
        //pAIMgr->DeleteAnchor(m_sAnchor);

        auto index = ui->m_wndAnchorsList->currentIndex();
        if (index.isValid())
        {
            m_anchorsModel->removeRow(index.row());
        }
    }
}

void CAIAnchorsDialog::OnRefreshBtn()
{
    CAIManager* pAIMgr = GetIEditor()->GetAI();
    pAIMgr->ReloadScripts();
    typedef QStringList TAnchorActionsVec;
    TAnchorActionsVec vecAnchorActions;
    pAIMgr->GetAnchorActions(vecAnchorActions);

    m_anchorsModel->setStringList(vecAnchorActions);

    auto indexes = m_anchorsModel->match(m_anchorsModel->index(0, 0), Qt::DisplayRole, m_sAnchor, 1, Qt::MatchExactly);
    if (!indexes.isEmpty())
    {
        ui->m_wndAnchorsList->setCurrentIndex(indexes.first());
    }
}

void CAIAnchorsDialog::OnLbnDblClk()
{
    if (ui->m_wndAnchorsList->selectionModel()->hasSelection())
    {
        accept();
    }
}

void CAIAnchorsDialog::OnLbnSelchangeAnchors()
{
    auto index = ui->m_wndAnchorsList->currentIndex();
    if (!index.isValid())
    {
        m_sAnchor.clear();
        return;
    }

    ui->m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);

    m_sAnchor = index.data().toString();
}

void CAIAnchorsDialog::OnInitDialog()
{
    ui->m_newButton->setEnabled(false);
    ui->m_deleteButton->setEnabled(false);

    CAIManager* pAIMgr = GetIEditor()->GetAI();
    assert(pAIMgr);
    typedef QStringList TAnchorActionsVec;
    TAnchorActionsVec vecAnchorActions;
    pAIMgr->GetAnchorActions(vecAnchorActions);

    m_anchorsModel->setStringList(vecAnchorActions);
}

void CAIAnchorsDialog::SetAIAnchor(const QString& sAnchor)
{
    m_sAnchor = sAnchor;

    auto indexes = m_anchorsModel->match(m_anchorsModel->index(0, 0), Qt::DisplayRole, m_sAnchor, 1, Qt::MatchExactly);
    if (!indexes.isEmpty())
    {
        ui->m_wndAnchorsList->setCurrentIndex(indexes.first());
    }
}

#include <AIAnchorsDialog.moc>
