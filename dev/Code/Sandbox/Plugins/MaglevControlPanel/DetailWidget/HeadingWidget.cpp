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

#include "HeadingWidget.h"

#include <QVariant>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>

#include <DetailWidget/HeadingWidget.moc>

HeadingWidget::HeadingWidget(QWidget* parent)
    : QFrame(parent)
{
    CreateUI();
}

void HeadingWidget::CreateUI()
{
    setObjectName("Heading");

    auto rootLayout = new QHBoxLayout {};
    rootLayout->setMargin(0);
    rootLayout->setSpacing(0);
    setLayout(rootLayout);

    auto textLayout = new QVBoxLayout {};
    textLayout->setMargin(0);
    textLayout->setSpacing(0);
    rootLayout->addLayout(textLayout);

    m_refreshButton = new QPushButton {};
    m_refreshButton->setObjectName("RefreshButton");
    m_refreshButton->setIcon(QIcon("Editor/Icons/CloudCanvas/refresh.png"));
    m_refreshButton->setIconSize(QSize(16, 16));
    m_refreshButton->setToolTip(tr("Refresh window contents"));
    m_refreshButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    connect(m_refreshButton, &QPushButton::clicked, this, &HeadingWidget::RefreshClicked);
    rootLayout->addWidget(m_refreshButton, 0, Qt::AlignTop);

    m_titleLabel = new QLabel {
        "(SetTitleText|HideTitle)"
    };
    m_titleLabel->setObjectName("Title");
    m_titleLabel->setWordWrap(true);
    textLayout->addWidget(m_titleLabel);

    m_messageLabel = new QLabel {
        "(SetMessageText)"
    };
    m_messageLabel->setObjectName("Message");
    m_messageLabel->setWordWrap(true);
    textLayout->addWidget(m_messageLabel);
}

void HeadingWidget::SetTitleText(const QString& text)
{
    m_titleLabel->setText(text);
    m_titleLabel->setVisible(true);
}

void HeadingWidget::HideTitle()
{
    m_titleLabel->setVisible(false);
}

void HeadingWidget::SetMessageText(const QString& text)
{
    m_messageLabel->setText(text);
    m_messageLabel->setVisible(true);
}

void HeadingWidget::HideMessage()
{
    m_messageLabel->setVisible(false);
}

void HeadingWidget::HideRefresh()
{
    m_refreshButton->setVisible(false);
}
