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
#include "ItemDescriptionDlg.h"
#include "SmartObjectStateDialog.h"
#include "SmartObjectPatternDialog.h"
#include "ui_SmartObjectPatternDialog.h"
#include "AI/AIManager.h"

#include <QStringListModel>

// CSmartObjectPatternDialog dialog


CSmartObjectPatternDialog::CSmartObjectPatternDialog(QWidget* pParent /*=NULL*/)
    : QDialog(pParent)
    , m_ui(new Ui::SmartObjectPatternDialog)
{
    m_ui->setupUi(this);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    m_ui->m_wndList->setModel(new QStringListModel(this));
    OnInitDialog();
    connect(m_ui->m_wndList, &QAbstractItemView::doubleClicked, this, &CSmartObjectPatternDialog::OnLbnDblClk);
    connect(m_ui->m_wndList->selectionModel(), &QItemSelectionModel::selectionChanged, this, &CSmartObjectPatternDialog::OnLbnSelchange);
    connect(m_ui->m_btnNew, &QPushButton::clicked, this, &CSmartObjectPatternDialog::OnNewBtn);
    connect(m_ui->m_btnEdit, &QPushButton::clicked, this, &CSmartObjectPatternDialog::OnLbnDblClk);
    connect(m_ui->m_btnDelete, &QPushButton::clicked, this, &CSmartObjectPatternDialog::OnDeleteBtn);
}

CSmartObjectPatternDialog::~CSmartObjectPatternDialog()
{
}

void CSmartObjectPatternDialog::SetPattern(const QString& sPattern)
{
    qobject_cast<QStringListModel*>(m_ui->m_wndList->model())->setStringList(sPattern.split(QStringLiteral("|"), QString::SkipEmptyParts));
}

QString CSmartObjectPatternDialog::GetPattern() const
{
    return qobject_cast<QStringListModel*>(m_ui->m_wndList->model())->stringList().join(QStringLiteral(" | "));
}

// CSmartObjectStateDialog message handlers

void CSmartObjectPatternDialog::OnNewBtn()
{
    CSmartObjectStateDialog dlg({}, this, true);
    if (dlg.exec())
    {
        QString item = dlg.GetSOState();
        if (!item.isEmpty())
        {
            auto model = m_ui->m_wndList->model();
            const int row = model->rowCount();
            model->insertRow(row);
            model->setData(model->index(row, 0), item);
            m_ui->m_wndList->setCurrentIndex(model->index(row, 0));

            m_ui->buttonBox->setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
            m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
        }
    }
}

void CSmartObjectPatternDialog::OnDeleteBtn()
{
    const QModelIndex sel = m_ui->m_wndList->currentIndex();
    if (sel.isValid())
    {
        m_ui->m_wndList->model()->removeRow(sel.row());

        m_ui->buttonBox->setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
    }
}

void CSmartObjectPatternDialog::OnLbnDblClk()
{
    const QModelIndex nSel = m_ui->m_wndList->currentIndex();
    if (nSel.isValid())
    {
        QString item = nSel.data().toString();
        CSmartObjectStateDialog dlg(item, this, true);
        if (dlg.exec())
        {
            item = dlg.GetSOState();
            if (!item.isEmpty())
            {
                m_ui->m_wndList->model()->setData(nSel, item);

                m_ui->buttonBox->setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
                m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
            }
        }
    }
}

void CSmartObjectPatternDialog::OnLbnSelchange()
{
    const bool nSel = m_ui->m_wndList->selectionModel()->hasSelection();
    m_ui->m_btnEdit->setEnabled(nSel);
    m_ui->m_btnDelete->setEnabled(nSel);
}

void CSmartObjectPatternDialog::OnInitDialog()
{
    m_ui->m_btnEdit->setEnabled(false);
    m_ui->m_btnDelete->setEnabled(false);
    m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
}

#include <SmartObjectPatternDialog.moc>
