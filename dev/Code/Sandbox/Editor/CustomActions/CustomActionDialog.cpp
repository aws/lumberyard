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

#include "CustomActionDialog.h"

#include "HyperGraph/FlowGraphManager.h"
#include "HyperGraph/FlowGraph.h"

#include "CustomActions/CustomActionsEditorManager.h"
#include <ICustomActions.h>
#include <IGameFramework.h>

#include <QStringListModel>

//////////////////////////////////////////////////////////////////////////
// Dialog shown for custom action property
//////////////////////////////////////////////////////////////////////////
CCustomActionDialog::CCustomActionDialog(QWidget* pParent /*=nullptr*/)
    : QDialog(pParent)
    , m_ui(new Ui_CCustomActionDialog)
    , m_model(new QStringListModel(this))
{
    m_ui->setupUi(this);
    m_ui->listView->setModel(m_model);

    connect(m_ui->newButton, &QPushButton::clicked,
        this, &CCustomActionDialog::OnNewBtn);
    connect(m_ui->editButton, &QPushButton::clicked,
        this, &CCustomActionDialog::OnEditBtn);
    connect(m_ui->refreshButton, &QPushButton::clicked,
        this, &CCustomActionDialog::OnRefreshBtn);
    connect(m_ui->listView->selectionModel(), &QItemSelectionModel::currentChanged,
        this, &CCustomActionDialog::OnLbnSelchangeAction);

    setWindowTitle(tr("Custom Actions"));
    m_ui->labelListView->setText(tr("&Choose Custom Action:"));
    m_ui->deleteButton->setEnabled(false);
    m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

    Refresh();
}

CCustomActionDialog::~CCustomActionDialog()
{
}

void CCustomActionDialog::SetCustomAction(const QString& customAction)
{
    QModelIndexList indexes = m_model->match(m_model->index(0, 0), Qt::DisplayRole, customAction, Qt::MatchExactly);
    if (!indexes.isEmpty())
    {
        m_ui->listView->setCurrentIndex(indexes.at(0));
    }
}

QString CCustomActionDialog::GetCustomAction() const
{
    if (m_ui->listView->currentIndex().isValid())
    {
        return m_ui->listView->currentIndex().data().toString();
    }

    return QString();
}

//////////////////////////////////////////////////////////////////////////
bool CCustomActionDialog::OpenViewForCustomAction()
{
    IGame* pGame = GetISystem()->GetIGame();
    if (!pGame)
    {
        return false;
    }

    IGameFramework* pGameFramework = pGame->GetIGameFramework();
    if (!pGameFramework)
    {
        return false;
    }

    ICustomActionManager* pCustomActionManager = pGameFramework->GetICustomActionManager();
    CRY_ASSERT(pCustomActionManager != NULL);
    if (pCustomActionManager)
    {
        ICustomAction* pCustomAction = pCustomActionManager->GetCustomActionFromLibrary(m_customAction.toUtf8().data());
        if (pCustomAction)
        {
            CFlowGraphManager* pManager = GetIEditor()->GetFlowGraphManager();
            CFlowGraph* pFlowGraph = pManager->FindGraphForCustomAction(pCustomAction);
            assert(pFlowGraph);
            if (pFlowGraph)
            {
                pManager->OpenView(pFlowGraph);
                return true;
            }
        }
    }

    return false;
}

// CCustomActionDialog message handlers

//////////////////////////////////////////////////////////////////////////
void CCustomActionDialog::OnNewBtn()
{
    QString filename;

    if (GetIEditor()->GetCustomActionManager()->NewCustomAction(filename))
    {
        m_customAction = PathUtil::GetFileName(filename.toUtf8().data());

        bool bResult = OpenViewForCustomAction();
        if (bResult)
        {
            accept();
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CCustomActionDialog::OnEditBtn()
{
    //  EndDialog( IDCANCEL );

    OpenViewForCustomAction();
}

//////////////////////////////////////////////////////////////////////////
void CCustomActionDialog::OnRefreshBtn()
{
    QString selectedItem = GetCustomAction();
    Refresh();
    SetCustomAction(selectedItem);
}

//////////////////////////////////////////////////////////////////////////
void CCustomActionDialog::Refresh()
{
    QStringList list;

    // add empty string item
    list.append(QString());

    typedef QStringList TCustomActionsVec;
    TCustomActionsVec vecCustomActions;

    CCustomActionsEditorManager* pCustomActionsManager = GetIEditor()->GetCustomActionManager();
    CRY_ASSERT(pCustomActionsManager != NULL);
    if (pCustomActionsManager)
    {
        pCustomActionsManager->GetCustomActions(vecCustomActions);
    }

    for (TCustomActionsVec::iterator it = vecCustomActions.begin(); it != vecCustomActions.end(); ++it)
    {
        list.append(*it);
    }

    m_model->setStringList(list);
}

//////////////////////////////////////////////////////////////////////////
void CCustomActionDialog::OnLbnDblClk()
{
    if (m_ui->listView->currentIndex().isValid())
    {
        accept();
    }
}

//////////////////////////////////////////////////////////////////////////
void CCustomActionDialog::OnLbnSelchangeAction(const QModelIndex& current, const QModelIndex& previous)
{
    m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
	// EXCEPTION: OCX Property Pages should return FALSE
}

#include <CustomActions/CustomActionDialog.moc>