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

#include "StackEventsSplitter.h"
#include "StackEventsWidget.h"

#include <QLabel>

#include "DetailWidget/StackEventsSplitter.moc"

StackEventsSplitter::StackEventsSplitter(QSharedPointer<IStackEventsModel> model, QWidget* parent)
    : QSplitter{parent}
    , m_model{model}
{
    // orientation

    setOrientation(Qt::Orientation::Vertical);

    // add placeholder top widget

    auto placeHolderLabel = new QLabel {
        "(SetTopWidget)"
    };
    addWidget(placeHolderLabel);
    setCollapsible(0, true);
    setStretchFactor(0, 1);

    // add events bottom widget

    m_eventsWidget = new StackEventsWidget {
        model, this
    };
    addWidget(m_eventsWidget);
    setCollapsible(1, false);
    setStretchFactor(1, 0);

    // connect model

    connect(m_model.data(), &IStackEventsModel::CommandStatusChanged, this, &StackEventsSplitter::OnCommandStatusChanged);

    // connect timers

    m_initialRefreshTimer.setInterval(INITIAL_REFRESH_TIMEOUT_MS);
    connect(&m_initialRefreshTimer, &QTimer::timeout, this, &StackEventsSplitter::OnInitialRefreshTimeout);

    m_refreshTimer.setInterval(REFRESH_TIMEOUT_MS);
    connect(&m_refreshTimer, &QTimer::timeout, this, &StackEventsSplitter::OnRefreshTimeout);
}

QWidget* StackEventsSplitter::GetTopWidget() const
{
    return widget(0);
}

void StackEventsSplitter::SetTopWidget(QWidget* widget)
{
    delete GetTopWidget();

    insertWidget(0, widget);

    setCollapsible(0, true);
    setStretchFactor(0, 1);
    setCollapsible(1, false);
    setStretchFactor(1, 0);
}

void StackEventsSplitter::SetStackEventsModel(QSharedPointer<IStackEventsModel> eventsModel)
{
    disconnect(m_model.data(), &IStackEventsModel::CommandStatusChanged, this, &StackEventsSplitter::OnCommandStatusChanged);

    m_model = eventsModel;

    connect(m_model.data(), &IStackEventsModel::CommandStatusChanged, this, &StackEventsSplitter::OnCommandStatusChanged);

    m_eventsWidget->SetStackEventsModel(eventsModel);
}

void StackEventsSplitter::MaximizeEventLog()
{
    moveSplitter(0, 1);
}

void StackEventsSplitter::ShowEventLog()
{
    int min, max;
    getRange(1, &min, &max);
    if (sizes()[0] >= max - 20)
    {
        int mid = size().height() / 2;
        moveSplitter(mid, 1);
    }
}

void StackEventsSplitter::MinimizeEventLog()
{
    int max = size().height();
    moveSplitter(max, 1);
}


void StackEventsSplitter::OnCommandStatusChanged(IStackEventsModel::CommandStatus commandStatus)
{
    switch (commandStatus)
    {
    case IStackEventsModel::CommandStatus::CommandInProgress:
        m_initialRefreshTimer.start();
        m_refreshTimer.stop();
        ShowEventLog();
        break;

    case IStackEventsModel::CommandStatus::CommandFailed:
    case IStackEventsModel::CommandStatus::CommandSucceeded:
        m_initialRefreshTimer.stop();
        m_refreshTimer.stop();
        RefreshTime();
        break;

    case IStackEventsModel::CommandStatus::NoCommandStatus:
        m_initialRefreshTimer.stop();
        m_refreshTimer.stop();
        break;

    default:
        assert(false);
        break;
    }
}

void StackEventsSplitter::OnInitialRefreshTimeout()
{
    m_initialRefreshTimer.stop();
    m_refreshTimer.start();
    RefreshTime();
}

void StackEventsSplitter::OnRefreshTimeout()
{
    RefreshTime();
}

