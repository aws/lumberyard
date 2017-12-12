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
#include "LyFlowGraphNotification.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

FlowGraphNotificationDialog::FlowGraphNotificationDialog(QWidget* parent /* = nullptr */)
    : AzQtComponents::StyledDialog(parent, Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowCloseButtonHint | Qt::WindowTitleHint)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setModal(true);
    setWindowModality(Qt::ApplicationModal);
    setWindowTitle(tr("Flow Graph Update"));
    setSizeGripEnabled(false);

    QPoint dialogCenter = mapToGlobal(rect().center());
    QPoint parentWindowCenter = parent->mapToGlobal(window()->rect().center());
    move(parentWindowCenter - dialogCenter);

    QVBoxLayout* layout = new QVBoxLayout();
    
    QLabel* label = new QLabel();
    label->setText(tr(
R"(Amazon Lumberyard has accelerated the development of a new visual scripting solution that will deeply integrate with the new 
Component Entity system and its improved reflection, serialization, messaging and slice features, but we're going to ask that you 
be a little more patient before we show it to you.

If you're using Flow Graph on your project, we'll continue to support you. In the meantime, if you are using the new Component Entity system, 
we encourage you to use Lua for your scripting needs until our new visual scripting solution is available.)"));
    layout->addWidget(label);

    label = new QLabel();
    label->setText(tr("\nHave a great idea for visual scripting ? We'd love to hear about it - let us know on the forums."));
    layout->addWidget(label);
    
    label = new QLabel();
    label->setOpenExternalLinks(true);
    label->setStyleSheet("font-size: 1.5em; font-weight: bold; font-decoration: underline; font-color: #428fF4;");
    label->setText(tr("<a href=\"https://gamedev.amazon.com/forums/spaces/115/scripting.html\">Amazon GameDev Forum - Scripting</a>"));
    layout->addWidget(label);

    label = new QLabel();
    label->setOpenExternalLinks(true);    
    label->setText(tr("\nNeed help getting started with Lua? Check out the Lua Scripting topic in the Lumberyard Developer Guide."));
    layout->addWidget(label);
    
    label = new QLabel();
    label->setOpenExternalLinks(true);
    label->setStyleSheet("font-size: 1.5em; font-weight: bold; font-decoration: underline; font-color: #428fF4;");
    label->setText(tr("<a href=\"http://docs.aws.amazon.com/lumberyard/latest/developerguide/lua-scripting-intro.html\">Help with Lua Scripting</a>"));
    layout->addWidget(label);

    m_dontShowAgainCheckbox = new QCheckBox();
    m_dontShowAgainCheckbox->setText(tr("Don't show this again."));
    layout->addWidget(m_dontShowAgainCheckbox);

    QPushButton* okButton = new QPushButton();
    okButton->setText(tr("Ok"));
    okButton->setDefault(true);
    layout->addWidget(okButton);
    connect(okButton, &QPushButton::clicked, this, &QDialog::close);

    setLayout(layout);
}

void FlowGraphNotificationDialog::closeEvent(QCloseEvent* evt)
{
    gSettings.showFlowgraphNotification = (m_dontShowAgainCheckbox->checkState() != Qt::CheckState::Checked);

    QDialog::closeEvent(evt);
}

void FlowGraphNotificationDialog::Show()
{
    if (gSettings.showFlowgraphNotification)
    {
        raise();
        show();
    }
}

#include <HyperGraph/LyFlowGraphNotification.moc>
