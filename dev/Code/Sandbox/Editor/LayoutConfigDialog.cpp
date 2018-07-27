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
#include "LayoutConfigDialog.h"

#include <ui_LayoutConfigDialog.h>

class LayoutConfigModel
    : public QAbstractListModel
{
public:
    LayoutConfigModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = {}) const override;
    int columnCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

private:
    static const int kNumLayouts = 9;
};

LayoutConfigModel::LayoutConfigModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

int LayoutConfigModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : kNumLayouts;
}

int LayoutConfigModel::columnCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : 1;
}

QVariant LayoutConfigModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.column() > 0 || index.row() >= kNumLayouts)
    {
        return {};
    }

    switch (role)
    {
    case Qt::SizeHintRole:
        return QSize(42, 36);

    case Qt::DecorationRole:
        return QPixmap(QStringLiteral(":/layouts-%1.png").arg(index.row()));
    }

    return {};
}

// CLayoutConfigDialog dialog

CLayoutConfigDialog::CLayoutConfigDialog(QWidget* pParent /*=NULL*/)
    : QDialog(pParent)
    , m_model(new LayoutConfigModel(this))
    , ui(new Ui::CLayoutConfigDialog)
{
    m_layout = ET_Layout1;

    ui->setupUi(this);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setFixedSize(size());

    ui->m_layouts->setModel(m_model);

    connect(ui->m_buttonBox, &QDialogButtonBox::accepted, this, &CLayoutConfigDialog::OnOK);
    connect(ui->m_buttonBox, &QDialogButtonBox::rejected, this, &CLayoutConfigDialog::reject);
}

CLayoutConfigDialog::~CLayoutConfigDialog()
{
}

void CLayoutConfigDialog::SetLayout(EViewLayout layout)
{
    ui->m_layouts->setCurrentIndex(m_model->index(static_cast<int>(layout), 0));
}

// CLayoutConfigDialog message handlers

//////////////////////////////////////////////////////////////////////////
void CLayoutConfigDialog::OnOK()
{
    auto index = ui->m_layouts->currentIndex();

    if (index.isValid())
    {
        m_layout = static_cast<EViewLayout>(index.row());
    }

    accept();
}

#include <LayoutConfigDialog.moc>
