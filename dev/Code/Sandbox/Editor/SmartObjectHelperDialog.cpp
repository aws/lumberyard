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
#include <IAISystem.h>
#include "ItemDescriptionDlg.h"
#include "AI/AIManager.h"

#include "SmartObjects/SmartObjectsEditorDialog.h"
#include "SmartObjectHelperDialog.h"
#include <ui_SmartObjectHelperDialog.h>

#include <QStringListModel>

// CSmartObjectHelperDialog dialog

CSmartObjectHelperDialog::CSmartObjectHelperDialog(QWidget* pParent /*=NULL*/, bool bAllowEmpty /*=true*/, bool bFromTemplate /*=false*/, bool bShowAuto /*=false*/)
    : QDialog(pParent)
    , m_bAllowEmpty(bAllowEmpty)
    , m_bFromTemplate(bFromTemplate)
    , m_bShowAuto(bShowAuto)
    , m_ui(new Ui::SmartObjectHelperDialog)
{
    m_ui->setupUi(this);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    m_ui->m_wndHelperList->setModel(new QStringListModel(this));

    OnInitDialog();

    connect(m_ui->m_wndHelperList, &QAbstractItemView::doubleClicked, this, &CSmartObjectHelperDialog::OnLbnDblClk);
    connect(m_ui->m_wndHelperList->selectionModel(), &QItemSelectionModel::currentChanged, this, &CSmartObjectHelperDialog::OnLbnSelchangeHelper);
    connect(m_ui->m_btnNew, &QPushButton::clicked, this, &CSmartObjectHelperDialog::OnNewBtn);
    connect(m_ui->m_btnEdit, &QPushButton::clicked, this, &CSmartObjectHelperDialog::OnEditBtn);
    connect(m_ui->m_btnDelete, &QPushButton::clicked, this, &CSmartObjectHelperDialog::OnDeleteBtn);
    connect(m_ui->m_btnRefresh, &QPushButton::clicked, this, &CSmartObjectHelperDialog::OnRefreshBtn);
}

CSmartObjectHelperDialog::~CSmartObjectHelperDialog()
{
}

// CSmartObjectHelperDialog message handlers

void CSmartObjectHelperDialog::OnDeleteBtn()
{
}

void CSmartObjectHelperDialog::OnNewBtn()
{
    /*
        CString filename;
        if ( GetIEditor()->GetAI()->NewAction(filename, this) )
        {
            m_sSOAction = PathUtil::GetFileName((const char*)filename);
            IAIAction* pAction = gEnv->pAISystem->GetAIAction( m_sSOAction );
            if ( pAction )
            {
                CFlowGraphManager* pManager = GetIEditor()->GetFlowGraphManager();
                CFlowGraph* pFlowGraph = pManager->FindGraphForAction( pAction );
                assert( pFlowGraph );
                if ( pFlowGraph )
                    pManager->OpenView( pFlowGraph );

                accept();
            }
        }
    */
}

void CSmartObjectHelperDialog::OnEditBtn()
{
    if (m_sSOHelper.isEmpty())
    {
        return;
    }

    CSOLibrary::VectorHelperData::iterator itHelper = CSOLibrary::FindHelper(m_sClassName, m_sSOHelper);
    if (itHelper != CSOLibrary::GetHelpers().end())
    {
        CItemDescriptionDlg dlg(this, false);
        dlg.setWindowTitle(tr("Edit Helper"));
        dlg.setItemCaption(tr("Helper name:"));
        dlg.setItem(itHelper->name);
        dlg.setDescription(itHelper->description);
        if (dlg.exec())
        {
            if (CSOLibrary::StartEditing())
            {
                itHelper->description = dlg.description();
                m_ui->m_description->setPlainText(itHelper->description);
            }
        }
    }
}

void CSmartObjectHelperDialog::OnRefreshBtn()
{
    QStringList items;
    // add empty string item
    if (m_bAllowEmpty)
    {
        items.push_back(QString());
    }

    if (m_bShowAuto)
    {
        items.push_back(tr("<Auto>"));
        items.push_back(tr("<AutoInverse>"));
    }

    //CAIManager* pAIMgr = GetIEditor()->GetAI();
    //ASSERT( pAIMgr );

    std::for_each(CSOLibrary::HelpersLowerBound(m_sClassName), CSOLibrary::HelpersUpperBound(m_sClassName), [&](const CSOLibrary::CHelperData& data) { items.push_back(data.name); });

    if (m_bFromTemplate)
    {
        CSOLibrary::VectorClassData::iterator itClass = CSOLibrary::FindClass(m_sClassName.toUtf8().data());
        if (itClass != CSOLibrary::GetClasses().end() && itClass->pClassTemplateData)
        {
            const CSOLibrary::CClassTemplateData::TTemplateHelpers& templateHelpers = itClass->pClassTemplateData->helpers;
            std::for_each(templateHelpers.begin(), templateHelpers.end(), [&](const CSOLibrary::CClassTemplateData::CTemplateHelper& data) { items.push_back(data.name); });
        }
    }

    qobject_cast<QStringListModel*>(m_ui->m_wndHelperList->model())->setStringList(items);

    m_ui->m_wndHelperList->setCurrentIndex(m_ui->m_wndHelperList->model()->index(items.indexOf(m_sSOHelper), 0));
    m_ui->m_btnEdit->setEnabled(CSOLibrary::FindHelper(m_sClassName, m_sSOHelper) != CSOLibrary::GetHelpers().end());
    UpdateDescription();
}

void CSmartObjectHelperDialog::OnLbnDblClk()
{
    if (m_ui->m_wndHelperList->currentIndex().isValid())
    {
        accept();
    }
}

void CSmartObjectHelperDialog::OnLbnSelchangeHelper()
{
    m_ui->buttonBox->setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);

    const QModelIndex nSel = m_ui->m_wndHelperList->currentIndex();
    if (!nSel.isValid())
    {
        m_sSOHelper.clear();
        return;
    }
    m_sSOHelper = nSel.data().toString();
    m_ui->m_btnEdit->setEnabled(CSOLibrary::FindHelper(m_sClassName, m_sSOHelper) != CSOLibrary::GetHelpers().end());
    UpdateDescription();
}

void CSmartObjectHelperDialog::UpdateDescription()
{
    if (m_sSOHelper.isEmpty())
    {
        m_ui->m_description->clear();
    }
    else if (m_sSOHelper == tr("<Auto>"))
    {
        m_ui->m_description->setPlainText(tr("The helper gets extracted automatically from the chosen animation."));
    }
    else if (m_sSOHelper == tr("<AutoInverse>"))
    {
        m_ui->m_description->setPlainText(tr("The helper gets extracted automatically from the chosen animation, then rotated by 180 degrees."));
    }
    else
    {
        CSOLibrary::VectorHelperData::iterator itHelper = CSOLibrary::FindHelper(m_sClassName, m_sSOHelper);
        if (itHelper != CSOLibrary::GetHelpers().end())
        {
            m_ui->m_description->setPlainText(itHelper->description);
        }
        else
        {
            m_ui->m_description->clear();
        }
    }
}

void CSmartObjectHelperDialog::OnInitDialog()
{
    setWindowTitle(tr("Smart Object Helpers"));
    m_ui->m_labelListCaption->setText(tr("&Choose Smart Object Helper:"));

    m_ui->m_btnNew->setEnabled(false);
    m_ui->m_btnEdit->setEnabled(true);
    m_ui->m_btnDelete->setEnabled(false);
    m_ui->m_btnRefresh->setEnabled(false);

    m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

    OnRefreshBtn();
}

#include <SmartObjectHelperDialog.moc>