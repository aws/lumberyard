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
#include "stdafx.h"

#include "StackEventsWidget.h"

#include "IEditor.h"

#include <QTableView>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QToolButton>
#include <QTimer>
#include <QScrollArea>
#include <QDebug>
#include <QTextEdit>
#include <QHeaderView>
#include <QMovie>
#include <QMenu>
#include <QAction>
#include <QApplication>
#include <QClipboard>

#include <DetailWidget/StackEventsWidget.moc>

StackEventsWidget::StackEventsWidget(QSharedPointer<IStackEventsModel> stackEventsModel, QWidget* parent)
    : QFrame{parent}
    , m_stackEventsModel{stackEventsModel}
{
    setObjectName("StackEvents");

    auto mainLayout = new QVBoxLayout {this};
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    mainLayout->addWidget(CreateTitleBar(), 0);

    auto content = new QFrame {this};
    content->setObjectName("Content");
    content->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Ignored);
    mainLayout->addWidget(content, 1);

    auto contentLayout = new QVBoxLayout {content};
    contentLayout->setSpacing(0);
    contentLayout->setContentsMargins(0, 0, 0, 0);

    m_logTable = new QTableView {content};
    m_logTable->setObjectName("Table");
    m_logTable->setModel(m_stackEventsModel.data());
    m_logTable->verticalHeader()->hide();
    m_logTable->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);
    m_logTable->setWordWrap(true);
    m_logTable->setEditTriggers(QTableView::NoEditTriggers);
    m_logTable->setSelectionMode(QTableView::SingleSelection);
    m_logTable->setFocusPolicy(Qt::ClickFocus);
    m_logTable->setContextMenuPolicy(Qt::CustomContextMenu);
    QHeaderView* headerView = m_logTable->horizontalHeader();
    headerView->setSectionResizeMode(QHeaderView::ResizeMode::ResizeToContents);
    headerView->setSectionResizeMode(0, QHeaderView::ResizeMode::Stretch);
    contentLayout->addWidget(m_logTable, 1);

    connect(m_stackEventsModel.data(), &IStackEventsModel::rowsInserted, this, &StackEventsWidget::OnRowsInserted);

    connect(m_logTable->horizontalHeader(), &QHeaderView::sectionResized, m_logTable, &QTableView::resizeRowsToContents);
    connect(m_logTable, &QTableView::customContextMenuRequested, this, &StackEventsWidget::OnCustomContextMenuRequested);
}

QWidget* StackEventsWidget::CreateTitleBar()
{
    auto titleBar = new QFrame {};
    titleBar->setObjectName("TitleBar");
    titleBar->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    auto titleBarLayout = new QHBoxLayout {};
    titleBarLayout->setSizeConstraint(QLayout::SizeConstraint::SetDefaultConstraint);
    titleBar->setLayout(titleBarLayout);

    auto titleLabel = new QLabel {
        tr("Progress log")
    };
    titleLabel->setObjectName("Title");
    titleLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    titleBarLayout->addWidget(titleLabel);

    m_statusInProgress = new QWidget {};
    m_statusInProgress->setObjectName("InProgress");
    m_statusInProgress->setToolTip(tr(
            "<span>"
            "This update may take some time to complete. The duration will depend on the kind "
            "of updates you are making and the number of resources you are pushing to the cloud. "
            "You may make additional changes while this update is in progress, but you can only "
            "push one update the cloud at a time."
            "</span>"));
    titleBarLayout->addWidget(m_statusInProgress);

    auto statusInProgressLayout = new QHBoxLayout {};
    statusInProgressLayout->setSizeConstraint(QLayout::SizeConstraint::SetDefaultConstraint);
    m_statusInProgress->setLayout(statusInProgressLayout);

    auto statusInProgressImage = new QLabel {};
    statusInProgressImage->setObjectName("Image");
    m_inProgressMovie = new QMovie {
        "Editor/Icons/CloudCanvas/in_progress.gif"
    };
    m_inProgressMovie->setScaledSize(QSize(16, 16));
    statusInProgressImage->setMovie(m_inProgressMovie);
    statusInProgressImage->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    statusInProgressImage->setFixedSize(16, 16);
    statusInProgressLayout->addWidget(statusInProgressImage);

    auto statusInProgressText = new QLabel {
        tr("Operation in progress")
    };
    statusInProgressText->setObjectName("Text");
    statusInProgressText->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    statusInProgressLayout->addWidget(statusInProgressText);

    m_statusSucceeded = new QLabel {
        tr("Operation succeeded")
    };
    m_statusSucceeded->setObjectName("Succeeded");
    m_statusSucceeded->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    titleBarLayout->addWidget(m_statusSucceeded);

    m_statusFailed = new QLabel {
        tr("Operation failed")
    };
    m_statusFailed->setObjectName("Failed");
    m_statusFailed->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    titleBarLayout->addWidget(m_statusFailed);

    titleBarLayout->addStretch();

    connect(m_stackEventsModel.data(), &IStackEventsModel::CommandStatusChanged, this, &StackEventsWidget::OnCommandStatusChanged);
    OnCommandStatusChanged(m_stackEventsModel->GetCommandStatus());

    return titleBar;
}

void StackEventsWidget::OnCommandStatusChanged(IStackEventsModel::CommandStatus commandStatus)
{
    m_statusInProgress->setVisible(commandStatus == IStackEventsModel::CommandStatus::CommandInProgress);
    m_statusSucceeded->setVisible(commandStatus == IStackEventsModel::CommandStatus::CommandSucceeded);
    m_statusFailed->setVisible(commandStatus == IStackEventsModel::CommandStatus::CommandFailed);

    if (m_statusInProgress->isVisible())
    {
        m_inProgressMovie->start();
    }
    else
    {
        m_inProgressMovie->stop();
    }
}

void StackEventsWidget::OnRowsInserted(const QModelIndex& parent, int first, int last)
{
    for (int row = first; row <= last; ++row)
    {
        m_logTable->resizeRowToContents(row);
    }
    m_logTable->resizeColumnToContents(IStackEventsModel::Columns::StatusColumn);
    m_logTable->resizeColumnToContents(IStackEventsModel::Columns::TimeColumn);
    m_logTable->scrollToBottom();
}

void StackEventsWidget::OnCustomContextMenuRequested(const QPoint& pos)
{
    QModelIndex index = m_logTable->indexAt(pos);
    if (index.isValid())
    {
        auto eventContextMenu = new QMenu {
            this
        };
        auto action = eventContextMenu->addAction(tr("Copy to clipboard"));
        connect(action, &QAction::triggered, this, [this] { QApplication::clipboard()->setText(m_logTable->currentIndex().data().toString());
            });
        eventContextMenu->popup(m_logTable->mapToGlobal(pos));
    }
}

void StackEventsWidget::SetStackEventsModel(QSharedPointer<IStackEventsModel> stackEventsModel)
{
    disconnect(m_stackEventsModel.data(), &IStackEventsModel::CommandStatusChanged, this, &StackEventsWidget::OnCommandStatusChanged);
    disconnect(m_stackEventsModel.data(), &IStackEventsModel::rowsInserted, this, &StackEventsWidget::OnRowsInserted);

    m_stackEventsModel = stackEventsModel;
    m_logTable->setModel(stackEventsModel.data());

    connect(m_stackEventsModel.data(), &IStackEventsModel::CommandStatusChanged, this, &StackEventsWidget::OnCommandStatusChanged);
    connect(m_stackEventsModel.data(), &IStackEventsModel::rowsInserted, this, &StackEventsWidget::OnRowsInserted);

    OnCommandStatusChanged(m_stackEventsModel->GetCommandStatus());
    OnRowsInserted(QModelIndex {}, 0, m_stackEventsModel->rowCount() - 1);
}

