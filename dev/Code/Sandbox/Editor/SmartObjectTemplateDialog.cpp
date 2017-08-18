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
#include <IAISystem.h>
#include "ItemDescriptionDlg.h"
#include "AI/AIManager.h"

#include <QAbstractListModel>

#include "SmartObjectTemplateDialog.h"

#include "ui_SmartObjectTemplateDialog.h"

// CSmartObjectTemplateDialog dialog

class SmartObjectTemplateModel
    : public QAbstractListModel
{
public:
    enum CustomRoles
    {
        IdRole = Qt::UserRole,
        DescriptionRole
    };

    SmartObjectTemplateModel(QObject* parent = nullptr)
        : QAbstractListModel(parent)
    {
    }

    virtual ~SmartObjectTemplateModel()
    {
    }

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    void UpdateFrom(const MapTemplates& templates);

    QModelIndex IndexOf(int id);
    QModelIndex IndexOf(const QString& name);

private:
    struct TemplateDetails
    {
        const int id;
        const QString name;
        const QString description;
    };

    std::vector<TemplateDetails> m_templates;
};

int SmartObjectTemplateModel::rowCount(const QModelIndex& parent /* = {} */) const
{
    if (parent == QModelIndex())
    {
        return m_templates.size();
    }

    return {};
}

QVariant SmartObjectTemplateModel::data(const QModelIndex& index, int role /* = Qt::DisplayRole */) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_templates.size() || index.column() != 0)
    {
        return {};
    }

    switch (role)
    {
    case Qt::DisplayRole:
        return m_templates[index.row()].name;
    case IdRole:
        return m_templates[index.row()].id;
    case DescriptionRole:
        return m_templates[index.row()].description;
    default:
        return {};
    }
}

QVariant SmartObjectTemplateModel::headerData(int section, Qt::Orientation orientation, int role /* = Qt::DisplayRole */) const
{
    if (section == 1 && role == Qt::DisplayRole)
    {
        return tr("Template");
    }

    return {};
}

void SmartObjectTemplateModel::UpdateFrom(const MapTemplates& templates)
{
    emit beginResetModel();
    m_templates.clear();

    for (auto& entry : templates)
    {
        m_templates.push_back(TemplateDetails { entry.first, entry.second->name, entry.second->description });
    }
    emit endResetModel();
}

QModelIndex SmartObjectTemplateModel::IndexOf(int id)
{
    for (auto entry = m_templates.cbegin(); entry != m_templates.cend(); ++entry)
    {
        if (entry->id == id)
        {
            return createIndex(std::distance(m_templates.cbegin(), entry), 0);
        }
    }

    return {};
}

QModelIndex SmartObjectTemplateModel::IndexOf(const QString& name)
{
    for (auto entry = m_templates.cbegin(); entry != m_templates.cend(); ++entry)
    {
        if (entry->name == name)
        {
            return createIndex(std::distance(m_templates.cbegin(), entry), 0);
        }
    }

    return {};
}


CSmartObjectTemplateDialog::CSmartObjectTemplateDialog(QWidget* parent /* = nullptr */)
    : QDialog(parent)
    , m_ui(new Ui::SmartObjectTemplateDialog)
    , m_model(new SmartObjectTemplateModel(this))
{
    m_ui->setupUi(this);
    OnInitDialog();
}

CSmartObjectTemplateDialog::~CSmartObjectTemplateDialog()
{
}

// CSmartObjectTemplateDialog message handlers

void CSmartObjectTemplateDialog::OnDeleteBtn()
{
}

void CSmartObjectTemplateDialog::OnNewBtn()
{
}

void CSmartObjectTemplateDialog::OnEditBtn()
{
}

void CSmartObjectTemplateDialog::OnRefreshBtn()
{
    CAIManager* pAIMgr = GetIEditor()->GetAI();
    assert(pAIMgr);

    m_model->UpdateFrom(pAIMgr->GetMapTemplates());

    if (m_idSOTemplate >= 0)
    {
        QModelIndex index = m_model->IndexOf(m_idSOTemplate);
        m_ui->templateListView->selectionModel()->select(index, QItemSelectionModel::ClearAndSelect);
    }
}

void CSmartObjectTemplateDialog::OnLbnSelchangeTemplate(const QItemSelection& selected, const QItemSelection& deselected)
{
    if (selected.isEmpty())
    {
        m_idSOTemplate = -1;
        m_ui->templateDescriptionLabel->clear();
        m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
        return;
    }

    QModelIndex index = selected.indexes().first();

    m_idSOTemplate = index.data(SmartObjectTemplateModel::IdRole).toInt();
    m_ui->templateDescriptionLabel->setText(index.data(SmartObjectTemplateModel::DescriptionRole).toString());
    m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
}

void CSmartObjectTemplateDialog::OnInitDialog()
{
    m_ui->templateListView->setModel(m_model);

    OnRefreshBtn();

    m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

    connect(m_ui->templateListView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &CSmartObjectTemplateDialog::OnLbnSelchangeTemplate);
    connect(m_ui->templateListView, &QAbstractItemView::doubleClicked, this, &QDialog::accept);
}

void CSmartObjectTemplateDialog::SetSOTemplate(const QString& sSOTemplate)
{
    QModelIndex index = m_model->IndexOf(sSOTemplate);
    m_ui->templateListView->setCurrentIndex(index);
    m_ui->templateListView->selectionModel()->select(index, QItemSelectionModel::ClearAndSelect);
}

QString CSmartObjectTemplateDialog::GetSOTemplate() const
{
    auto selected = m_ui->templateListView->selectionModel()->selectedIndexes();
    if (selected.isEmpty())
    {
        return {};
    }

    return selected.first().data(Qt::DisplayRole).toString();
};

#include <SmartObjectTemplateDialog.moc>
