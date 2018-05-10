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

#include "StackListWidget.h"
#include <IAWSResourceManager.h>

#include <QPushButton>
#include <QVBoxLayout>
#include <QLabel>
#include <QHeaderView>
#include <QScrollBar>
#include <QDebug>

#include "MaximumSizedTableView.h"

#include "HeadingWidget.h"
#include "ButtonBarWidget.h"
#include "StackStatusWidget.h"

StackListWidget::StackListWidget(QSharedPointer<IStackStatusListModel> stackStatusListModel, QWidget* parent)
    : QFrame(parent)
    , m_stackStatusListModel{stackStatusListModel}
{
    CreateUI();
}

void StackListWidget::CreateUI()
{
    // root layout

    setObjectName("StackList");

    auto rootLayout = new QVBoxLayout {this};
    rootLayout->setSpacing(0);
    rootLayout->setMargin(0);

    // top

    auto topWidget = new QFrame {this};
    topWidget->setObjectName("Top");
    rootLayout->addWidget(topWidget);

    auto topLayout = new QVBoxLayout {topWidget};
    topLayout->setSpacing(0);
    topLayout->setMargin(0);

    // top - heading

    auto headingWidget = new HeadingWidget {topWidget};
    headingWidget->SetTitleText(m_stackStatusListModel->GetMainTitleText());
    headingWidget->SetMessageText(m_stackStatusListModel->GetMainMessageText());
    connect(headingWidget, &HeadingWidget::RefreshClicked, this, [this](){ m_stackStatusListModel->Refresh(); });
    topLayout->addWidget(headingWidget);

    // top - buttons

    m_buttonBarWidget = new ButtonBarWidget {};
    topLayout->addWidget(m_buttonBarWidget);

    // bottom

    auto bottomWidget = new QFrame {this};
    bottomWidget->setObjectName("Bottom");
    rootLayout->addWidget(bottomWidget, 1);

    auto bottomLayout = new QVBoxLayout {bottomWidget};
    bottomLayout->setMargin(0);
    bottomLayout->setSpacing(0);

    // bottom - title

    auto titleRowLayout = new QHBoxLayout {};
    titleRowLayout->setMargin(0);
    titleRowLayout->setSpacing(0);
    bottomLayout->addLayout(titleRowLayout);

    auto listTitleLabel = new QLabel {
        m_stackStatusListModel->GetListTitleText(), bottomWidget
    };
    listTitleLabel->setObjectName("Title");
    titleRowLayout->addWidget(listTitleLabel);

    titleRowLayout->addStretch();

    m_refreshingLabel = new QLabel {
        tr("Refreshing..."), bottomWidget
    };
    m_refreshingLabel->setObjectName("Refreshing");
    titleRowLayout->addWidget(m_refreshingLabel);
    connect(m_stackStatusListModel.data(), &IStackStatusListModel::RefreshStatusChanged, this, &StackListWidget::OnRefreshStatusChanged);
    m_refreshingLabel->setVisible(m_stackStatusListModel->IsRefreshing());

    // bottom - message

    auto listMessageLabel = new QLabel {
        m_stackStatusListModel->GetListMessageText()
    };
    listMessageLabel->setObjectName("Message");
    listMessageLabel->setWordWrap(true);
    bottomLayout->addWidget(listMessageLabel);

    // bottom - table

    m_listTable = new MaximumSizedTableView {bottomWidget};
    m_listTable->setObjectName("Table");
    m_listTable->TableView()->setModel(m_stackStatusListModel.data());
    m_listTable->TableView()->verticalHeader()->hide();
    m_listTable->TableView()->setEditTriggers(QTableView::NoEditTriggers);
    m_listTable->TableView()->setSelectionMode(QTableView::NoSelection);
    m_listTable->TableView()->setFocusPolicy(Qt::NoFocus);
    m_listTable->TableView()->setWordWrap(false);
    for (int column = 0; column < m_stackStatusListModel->columnCount(); ++column)
    {
        m_listTable->TableView()->hideColumn(column);
    }
    m_listTable->TableView()->showColumn(IStackStatusListModel::Columns::PendingActionColumn);
    m_listTable->TableView()->showColumn(IStackStatusListModel::Columns::NameColumn);
    m_listTable->TableView()->showColumn(IStackStatusListModel::Columns::PhysicalResourceIdColumn);
    m_listTable->TableView()->showColumn(IStackStatusListModel::Columns::ResourceStatusColumn);
    m_listTable->TableView()->showColumn(IStackStatusListModel::Columns::TimestampColumn);

    m_listTable->Resize();

    bottomLayout->addWidget(m_listTable, 1);

    // bottom - stretch

    bottomLayout->addStretch();
}

void StackListWidget::AddButton(QPushButton* button)
{
    m_buttonBarWidget->AddButton(button);
}

void StackListWidget::OnRefreshStatusChanged(bool refreshing)
{
    m_refreshingLabel->setVisible(refreshing);
    if (!refreshing)
    {
        m_listTable->Resize();
    }
}

