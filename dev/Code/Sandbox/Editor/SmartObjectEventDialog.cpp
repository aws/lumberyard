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
//#include "AI/AIManager.h"
#include "SmartObjects/SmartObjectsEditorDialog.h"

#include "SmartObjectEventDialog.h"
#include <ui_SmartObjectEventDialog.h>

#include <QStringListModel>
#include <QMessageBox>

// CSmartObjectEventDialog dialog

CSmartObjectEventDialog::CSmartObjectEventDialog(QWidget* pParent /*=NULL*/)
    : QDialog(pParent)
    , m_ui(new Ui::SmartObjectEventDialog)
{
    m_ui->setupUi(this);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    m_ui->m_wndEventList->setModel(new QStringListModel(this));

    OnInitDialog();

    connect(m_ui->m_wndEventList, &QAbstractItemView::doubleClicked, this, &CSmartObjectEventDialog::OnLbnDblClk);
    connect(m_ui->m_wndEventList->selectionModel(), &QItemSelectionModel::currentChanged, this, &CSmartObjectEventDialog::OnLbnSelchangeEvent);
    connect(m_ui->m_btnNew, &QPushButton::clicked, this, &CSmartObjectEventDialog::OnNewBtn);
    connect(m_ui->m_btnEdit, &QPushButton::clicked, this, &CSmartObjectEventDialog::OnEditBtn);
    connect(m_ui->m_btnDelete, &QPushButton::clicked, this, &CSmartObjectEventDialog::OnDeleteBtn);
    connect(m_ui->m_btnRefresh, &QPushButton::clicked, this, &CSmartObjectEventDialog::OnRefreshBtn);
}

CSmartObjectEventDialog::~CSmartObjectEventDialog()
{
}

// CSmartObjectEventDialog message handlers

void CSmartObjectEventDialog::OnDeleteBtn()
{
}

void CSmartObjectEventDialog::OnNewBtn()
{
    CItemDescriptionDlg dlg(this);
    dlg.setWindowTitle(tr("New Event"));
    dlg.setItemCaption(tr("Event name:"));
    if (dlg.exec())
    {
        const QModelIndexList index = m_ui->m_wndEventList->model()->match(m_ui->m_wndEventList->model()->index(0, 0), Qt::DisplayRole, dlg.item(), 1, Qt::MatchExactly);
        if (!index.isEmpty())
        {
            m_sSOEvent = dlg.item();
            m_ui->m_wndEventList->setCurrentIndex(index.first());

            QMessageBox::critical(this, QString(), tr("Entered event name already exists and might be used for something else...\n\nThe event will be not created!"));
        }
        else if (CSOLibrary::StartEditing())
        {
            m_sSOEvent = dlg.item();
            auto model = m_ui->m_wndEventList->model();
            const int row = model->rowCount();
            model->insertRow(row);
            model->setData(model->index(row, 0), m_sSOEvent);
            m_ui->m_wndEventList->setCurrentIndex(model->index(row, 0));
            m_ui->m_description->setText(dlg.description());
            CSOLibrary::AddEvent(m_sSOEvent.toUtf8().data(), dlg.description().toUtf8().data());
        }

        m_ui->buttonBox->setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
    }
}

void CSmartObjectEventDialog::OnEditBtn()
{
    if (m_sSOEvent.isEmpty())
    {
        return;
    }

    CSOLibrary::VectorEventData::iterator it = CSOLibrary::FindEvent(m_sSOEvent.toUtf8().data());
    if (it == CSOLibrary::GetEvents().end())
    {
        return;
    }

    CItemDescriptionDlg dlg(this, false);
    dlg.setWindowTitle(tr("Edit Event"));
    dlg.setItemCaption(("Event name:"));
    dlg.setItem(m_sSOEvent);
    dlg.setDescription(it->description);
    if (dlg.exec())
    {
        if (CSOLibrary::StartEditing())
        {
            it->description = dlg.description();
            m_ui->m_description->setPlainText(dlg.description());
        }
    }
}

void CSmartObjectEventDialog::OnRefreshBtn()
{
    QStringList items;

    // add empty string item
    items.push_back(QString());

    //CAIManager* pAIMgr = GetIEditor()->GetAI();
    //ASSERT( pAIMgr );

    std::for_each(CSOLibrary::GetEvents().begin(), CSOLibrary::GetEvents().end(), [&](const CSOLibrary::CEventData& data){ items.push_back(data.name); });

    qobject_cast<QStringListModel*>(m_ui->m_wndEventList->model())->setStringList(items);
    m_ui->m_wndEventList->setCurrentIndex(m_ui->m_wndEventList->model()->index(items.indexOf(m_sSOEvent), 0));
    UpdateDescription();
}

void CSmartObjectEventDialog::OnLbnDblClk()
{
    if (m_ui->m_wndEventList->currentIndex().isValid())
    {
        accept();
    }
}

void CSmartObjectEventDialog::OnLbnSelchangeEvent()
{
    m_ui->buttonBox->setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);

    const QModelIndex nSel = m_ui->m_wndEventList->currentIndex();
    if (!nSel.isValid())
    {
        m_sSOEvent.clear();
        return;
    }
    m_sSOEvent = nSel.data().toString();
    UpdateDescription();
}

void CSmartObjectEventDialog::UpdateDescription()
{
    if (m_sSOEvent.isEmpty())
    {
        m_ui->m_description->clear();
    }
    else
    {
        CSOLibrary::VectorEventData::iterator it = CSOLibrary::FindEvent(m_sSOEvent.toUtf8().data());
        if (it != CSOLibrary::GetEvents().end())
        {
            m_ui->m_description->setPlainText(it->description);
        }
        else
        {
            m_ui->m_description->clear();
        }
    }
}

void CSmartObjectEventDialog::OnInitDialog()
{
    setWindowTitle(tr("Smart Object Events"));
    m_ui->m_labelListCaption->setText(tr("&Choose Smart Object Event:"));

    m_ui->m_btnNew->setEnabled(true);
    m_ui->m_btnEdit->setEnabled(true);
    m_ui->m_btnDelete->setEnabled(false);
    m_ui->m_btnRefresh->setEnabled(false);

    m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

    OnRefreshBtn();
}

#include <SmartObjectEventDialog.moc>
