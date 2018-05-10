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

#include "StackStatusWidget.h"

#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTableView>
#include <QHeaderView>

#include <IAWSResourceManager.h>

#include <DetailWidget/StackStatusWidget.moc>

StackStatusWidget::StackStatusWidget(QSharedPointer<IStackStatusModel> stackStatusModel, QWidget* parent)
    : QFrame(parent)
    , m_stackStatusModel{stackStatusModel}
{
    CreateUI();
}

void StackStatusWidget::CreateUI()
{
    // root

    setObjectName("Status");

    auto rootLayout = new QVBoxLayout {this};
    rootLayout->setSpacing(0);
    rootLayout->setMargin(0);

    // title row

    auto titleRowLayout = new QHBoxLayout {};
    titleRowLayout->setSpacing(0);
    titleRowLayout->setMargin(0);
    rootLayout->addLayout(titleRowLayout);

    // title row - title

    m_titleLabel = new QLabel {
        m_stackStatusModel->GetStatusTitleText(), this
    };
    m_titleLabel->setObjectName("Title");
    m_titleLabel->setWordWrap(false);
    titleRowLayout->addWidget(m_titleLabel);

    // title row - stretch

    titleRowLayout->addStretch();

    // title row - refreshing

    m_refreshingLabel = new QLabel {
        "Refreshing..."
    };
    m_refreshingLabel->setObjectName("Refreshing");
    connect(m_stackStatusModel.data(), &IStackStatusModel::RefreshStatusChanged, this, &StackStatusWidget::OnRefreshStatusChanged);
    titleRowLayout->addWidget(m_refreshingLabel);
    m_refreshingLabel->setVisible(m_stackStatusModel->IsRefreshing());

    // table

    m_statusTable = new QTableView {this};
    m_statusTable->setObjectName("Table");
    m_statusTable->setModel(m_stackStatusModel.data());
    m_statusTable->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeMode::ResizeToContents);
    m_statusTable->verticalHeader()->hide();
    m_statusTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeMode::ResizeToContents);
    m_statusTable->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Fixed);
    m_statusTable->setWordWrap(false);
    m_statusTable->setFrameStyle(QFrame::NoFrame);
    m_statusTable->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_statusTable->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_statusTable->setEditTriggers(QTableView::NoEditTriggers);
    m_statusTable->setSelectionMode(QTableView::NoSelection);
    m_statusTable->setFocusPolicy(Qt::NoFocus);
    for (int column = 0; column < m_stackStatusModel->columnCount(); ++column)
    {
        m_statusTable->hideColumn(column);
    }
    m_statusTable->showColumn(IStackStatusModel::Columns::PendingActionColumn);
    m_statusTable->showColumn(IStackStatusModel::Columns::StackStatusColumn);
    m_statusTable->showColumn(IStackStatusModel::Columns::CreationTimeColumn);
    m_statusTable->showColumn(IStackStatusModel::Columns::LastUpdatedTimeColumn);
    m_statusTable->showColumn(IStackStatusModel::Columns::StackIdColumn);
    rootLayout->addWidget(m_statusTable);

    UpdateStatusTableSize();
}

void StackStatusWidget::UpdateStatusTableSize()
{
    int height = 0;
    auto vh = m_statusTable->verticalHeader();
    for (int i = 0; i < vh->count(); ++i) // there should only be one
    {
        if (!vh->isSectionHidden(i))
        {
            height += vh->sectionSize(i);
        }
    }

    height += m_statusTable->horizontalHeader()->size().height();

    m_statusTable->setMaximumHeight(height);
    m_statusTable->setMinimumHeight(height);
}

void StackStatusWidget::OnRefreshStatusChanged(bool refreshing)
{
    m_refreshingLabel->setVisible(refreshing);

    if (!refreshing)
    {
        UpdateStatusTableSize();
    }
}
