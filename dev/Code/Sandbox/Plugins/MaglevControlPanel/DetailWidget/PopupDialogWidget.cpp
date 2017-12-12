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

#include "PopupDialogWidget.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QStyle>
#include <QVariant>

#include "DetailWidget/PopupDialogWidget.moc"

PopupDialogWidget::PopupDialogWidget(QWidget* parent, Qt::WindowFlags f)
    : QDialog(parent, f)
    , m_primaryButton {tr("OK")}
    , m_cancelButton {tr("Cancel")}
    , m_currentRowNum {0}
{
    setWindowModality(Qt::WindowModal);

    QVBoxLayout* outerLayout = new QVBoxLayout {
        this
    };
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->setSpacing(0);
    setLayout(outerLayout);

    QFrame* marginFrame = new QFrame {};
    marginFrame->setObjectName("MarginFrame");
    outerLayout->addWidget(marginFrame, 1);

    QVBoxLayout* mainLayout = new QVBoxLayout {
        this
    };
    mainLayout->setContentsMargins(0, 0, 0, 0);
    marginFrame->setLayout(mainLayout);

    m_gridLayout.setColumnStretch(1, 1);
    mainLayout->addLayout(&m_gridLayout, 1);

    m_bottomLabel.setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    QHBoxLayout* buttonLayout = new QHBoxLayout;
    buttonLayout->addWidget(&m_primaryButton, 0, Qt::AlignLeft | Qt::AlignHCenter);
    buttonLayout->addWidget(&m_bottomLabel, 1, Qt::AlignLeft | Qt::AlignHCenter);
    buttonLayout->addWidget(&m_cancelButton, 0, Qt::AlignRight | Qt::AlignHCenter);
    mainLayout->addLayout(buttonLayout);
    m_primaryButton.setObjectName("DialogButton");
    m_primaryButton.setProperty("class", "Primary");
    m_cancelButton.setObjectName("DialogButton");
    QObject::connect(&m_primaryButton, &QPushButton::clicked, this, &QDialog::accept);
    QObject::connect(&m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);
}

void PopupDialogWidget::AddWidgetPairRow(QWidget* leftWidget, QWidget* rightWidget, int stretch)
{
    m_gridLayout.addWidget(leftWidget, m_currentRowNum, 0, Qt::AlignLeft);
    m_gridLayout.addWidget(rightWidget, m_currentRowNum, 1);
    if (stretch)
    {
        m_gridLayout.setRowStretch(m_currentRowNum, stretch);
    }
    ++m_currentRowNum;
}

void PopupDialogWidget::AddStretchRow(int stretch)
{
    m_gridLayout.addLayout(new QVBoxLayout{}, m_currentRowNum, 0, 1, -1);
    m_gridLayout.setRowStretch(m_currentRowNum, stretch);
    ++m_currentRowNum;
}

void PopupDialogWidget::AddSpanningWidgetRow(QWidget* widget, int stretch)
{
    m_gridLayout.addWidget(widget, m_currentRowNum, 0, 1, -1);
    if (stretch)
    {
        m_gridLayout.setRowStretch(m_currentRowNum, stretch);
    }
    ++m_currentRowNum;
}

void PopupDialogWidget::AddLeftOnlyWidgetRow(QWidget* widget, int stretch)
{
    m_gridLayout.addWidget(widget, m_currentRowNum, 0, Qt::AlignLeft);
    if (stretch)
    {
        m_gridLayout.setRowStretch(m_currentRowNum, stretch);
    }
    ++m_currentRowNum;
}

void PopupDialogWidget::AddRightOnlyWidgetRow(QWidget* widget, int stretch)
{
    m_gridLayout.addWidget(widget, m_currentRowNum, 1);
    if (stretch)
    {
        m_gridLayout.setRowStretch(m_currentRowNum, stretch);
    }
    ++m_currentRowNum;
}

QPushButton* PopupDialogWidget::GetPrimaryButton()
{
    return &m_primaryButton;
}

QPushButton* PopupDialogWidget::GetCancelButton()
{
    return &m_cancelButton;
}

QLabel* PopupDialogWidget::GetBottomLabel()
{
    return &m_bottomLabel;
}

void PopupDialogWidget::moveEvent(QMoveEvent* event)
{
    DialogMoved();
}

void PopupDialogWidget::resizeEvent(QResizeEvent* event)
{
    DialogResized();
}