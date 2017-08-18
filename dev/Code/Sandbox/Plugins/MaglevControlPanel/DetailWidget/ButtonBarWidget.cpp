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

#include "ButtonBarWidget.h"

#include <QVariant>
#include <QVBoxLayout>
#include <QLabel>
#include <QHBoxLayout>
#include <QPushButton>
#include <QCheckBox>
#include <QDesktopServices>
#include <QUrl>

#include <DetailWidget/ButtonBarWidget.moc>

ButtonBarWidget::ButtonBarWidget()
{
    CreateUI();
}

void ButtonBarWidget::CreateUI()
{
    setObjectName("ButtonBar");

    m_layout = new QHBoxLayout();
    m_layout->setMargin(0);
    m_layout->setSpacing(0);
    setLayout(m_layout);

    m_layout->addStretch();
}

void ButtonBarWidget::AddButton(QPushButton* button)
{
    // insert before stretch
    m_layout->insertWidget(m_layout->count() - 1, button);
}

void ButtonBarWidget::AddCheckBox(QCheckBox* checkBox)
{
    m_layout->insertWidget(m_layout->count() - 1, checkBox);
}
