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

#include "MaximumSizedTableView.h"

#include <QScrollBar>
#include <QHeaderView>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMouseEvent>

#include <DetailWidget/MaximumSizedTableView.moc>

MaximumSizeTableViewObject::MaximumSizeTableViewObject(QWidget* parent)
    : QTableView(parent)
{
}

void MaximumSizeTableViewObject::mouseReleaseEvent(QMouseEvent* event)
{
    QTableView::mouseReleaseEvent(event);

    if (event->button() == Qt::RightButton)
    {
        OnRightClick(event);
    }
}


MaximumSizedTableView::MaximumSizedTableView(QWidget* parent)
    : QFrame(parent)
{
    m_tableView = new MaximumSizeTableViewObject {this};
    m_tableView->setObjectName("Table");
    m_tableView->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeMode::ResizeToContents);
    m_tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeMode::ResizeToContents);
    m_tableView->setSizePolicy(QSizePolicy::Policy::Maximum, QSizePolicy::Policy::Maximum);
    m_tableView->setSizeAdjustPolicy(QAbstractScrollArea::SizeAdjustPolicy::AdjustToContents);

    m_horizontalScrollBarHeight = QScrollBar {
        Qt::Orientation::Horizontal
    }.sizeHint().height();
    m_verticalScrollBarWidth = QScrollBar {
        Qt::Orientation::Vertical
    }.sizeHint().width();

    auto hlayout = new QHBoxLayout {};
    hlayout->setSpacing(0);
    hlayout->setMargin(0);
    hlayout->addWidget(m_tableView, 1);
    hlayout->addStretch();

    auto vlayout = new QVBoxLayout {this};
    vlayout->setSpacing(0);
    vlayout->setMargin(0);
    vlayout->addLayout(hlayout, 1);
    vlayout->addStretch();

    setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);
}

void MaximumSizedTableView::showEvent(QShowEvent* event)
{
    Resize();
    QFrame::showEvent(event);
}

MaximumSizeTableViewObject* MaximumSizedTableView::TableView()
{
    return m_tableView;
}

void MaximumSizedTableView::Resize()
{
    int width = 0;
    auto hh = m_tableView->horizontalHeader();
    for (int i = 0; i < hh->count(); ++i)
    {
        if (!hh->isSectionHidden(i))
        {
            width += hh->sectionSize(i);
        }
    }

    int height = 0;
    auto vh = m_tableView->verticalHeader();
    for (int i = 0; i < vh->count(); ++i)
    {
        if (!vh->isSectionHidden(i))
        {
            height += vh->sectionSize(i);
        }
    }

    height += hh->height();

    if (vh->count() > 0)
    {
        height += m_horizontalScrollBarHeight;
    }
    width += m_verticalScrollBarWidth;

    setMaximumSize(width, height);
    setMinimumSize(0, 0);
}

