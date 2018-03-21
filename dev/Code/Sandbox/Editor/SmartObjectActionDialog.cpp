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
#include <IAISystem.h>
#include <IAgent.h>
#include "AI/AIManager.h"
#include "HyperGraph/FlowGraphManager.h"
#include "HyperGraph/FlowGraph.h"

#include "SmartObjectActionDialog.h"
#include <ui_SmartObjectActionDialog.h>

#include <QStringListModel>

// CSmartObjectActionDialog dialog

CSmartObjectActionDialog::CSmartObjectActionDialog(QWidget* pParent /*=NULL*/)
    : QDialog(pParent)
    , m_ui(new Ui::SmartObjectActionDialog)
{
    m_ui->setupUi(this);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    OnInitDialog();

    connect(m_ui->m_wndActionList, &QAbstractItemView::doubleClicked, this, &CSmartObjectActionDialog::OnLbnDblClk);
    connect(m_ui->m_wndActionList->selectionModel(), &QItemSelectionModel::selectionChanged, this, &CSmartObjectActionDialog::OnLbnSelchangeAction);
    connect(m_ui->m_btnNew, &QPushButton::clicked, this, &CSmartObjectActionDialog::OnNewBtn);
    connect(m_ui->m_btnEdit, &QPushButton::clicked, this, &CSmartObjectActionDialog::OnEditBtn);
    connect(m_ui->m_btnRefresh, &QPushButton::clicked, this, &CSmartObjectActionDialog::OnRefreshBtn);
}

CSmartObjectActionDialog::~CSmartObjectActionDialog()
{
}

void CSmartObjectActionDialog::SetSOAction(const QString& sSOAction)
{
    m_sSOAction = sSOAction;
    auto model = m_ui->m_wndActionList->model();
    auto indexes = model->match(model->index(0, 0), Qt::DisplayRole, m_sSOAction, 1, Qt::MatchExactly);
    m_ui->m_wndActionList->setCurrentIndex(indexes.isEmpty() ? QModelIndex() : indexes.first());
}

// CSmartObjectStateDialog message handlers

void CSmartObjectActionDialog::OnNewBtn()
{
    QString filename;
    if (GetIEditor()->GetAI()->NewAction(filename, this))
    {
        m_sSOAction = PathUtil::GetFileName(filename.toUtf8().data());
        IAIAction* pAction = gEnv->pAISystem->GetAIActionManager()->GetAIAction(m_sSOAction.toUtf8().data());
        if (pAction)
        {
            CFlowGraphManager* pManager = GetIEditor()->GetFlowGraphManager();
            CFlowGraph* pFlowGraph = pManager->FindGraphForAction(pAction);
            assert(pFlowGraph);
            if (pFlowGraph)
            {
                pManager->OpenView(pFlowGraph);
            }

            accept();
        }
    }
}

void CSmartObjectActionDialog::OnEditBtn()
{
    //  reject();

    IAIAction* pAction = gEnv->pAISystem->GetAIActionManager()->GetAIAction(m_sSOAction.toUtf8().data());
    if (pAction)
    {
        CFlowGraphManager* pManager = GetIEditor()->GetFlowGraphManager();
        CFlowGraph* pFlowGraph = pManager->FindGraphForAction(pAction);
        assert(pFlowGraph);
        if (pFlowGraph)
        {
            pManager->OpenView(pFlowGraph);
        }
    }
}

void CSmartObjectActionDialog::OnRefreshBtn()
{
    // add empty string item
    QStringList items;
    items.push_back(QString());

    CAIManager* pAIMgr = GetIEditor()->GetAI();
    assert(pAIMgr);
    typedef QStringList TSOActionsVec;
    TSOActionsVec vecSOActions;
    pAIMgr->GetSmartObjectActions(vecSOActions);
    for (TSOActionsVec::iterator it = vecSOActions.begin(); it != vecSOActions.end(); ++it)
    {
        items.push_back(*it);
    }
    qobject_cast<QStringListModel*>(m_ui->m_wndActionList->model())->setStringList(items);
    m_ui->m_wndActionList->setCurrentIndex(m_ui->m_wndActionList->model()->index(items.indexOf(m_sSOAction), 0));
}

void CSmartObjectActionDialog::OnLbnDblClk()
{
    if (m_ui->m_wndActionList->currentIndex().isValid())
    {
        accept();
    }
}

void CSmartObjectActionDialog::OnLbnSelchangeAction()
{
    m_ui->buttonBox->setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);

    const QModelIndex nSel = m_ui->m_wndActionList->currentIndex();
    if (!nSel.isValid())
    {
        return;
    }
    m_sSOAction = nSel.data().toString();
}


void CSmartObjectActionDialog::OnInitDialog()
{
    setWindowTitle(tr("AI Actions"));
    m_ui->m_labelListCaption->setText(tr("&Choose AI Action:"));

    m_ui->m_btnNew->setEnabled(true);
    m_ui->m_btnEdit->setEnabled(true);
    m_ui->m_btnDelete->setEnabled(false);
    m_ui->m_btnRefresh->setEnabled(true);

    m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

    m_ui->m_wndActionList->setModel(new QStringListModel(this));

    OnRefreshBtn();
}

#include <SmartObjectActionDialog.moc>