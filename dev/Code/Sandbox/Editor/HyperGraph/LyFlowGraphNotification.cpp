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

#include "StdAfx.h"
#include "LyFlowGraphNotification.h"

#include <AzToolsFramework/API/ToolsApplicationAPI.h>

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
    setWindowTitle(tr("Lumberyard Visual Scripting Announcement"));
    setSizeGripEnabled(false);

    QPoint dialogCenter = mapToGlobal(rect().center());
    QPoint parentWindowCenter = parent->mapToGlobal(window()->rect().center());
    move(parentWindowCenter - dialogCenter);

    QVBoxLayout* layout = new QVBoxLayout();
    
    QLabel* label = new QLabel();
    label->setText(tr(
R"(Amazon Lumberyard is pleased to announce the PREVIEW release of Script Canvas. 
This new visual scripting solution is deeply integrated with the component entity 
system's improved reflection, serialization, messaging, and slice features.)"));
    layout->addWidget(label);

    label = new QLabel();
    label->setText(tr("Learn more about Script Canvas:\r\n"));
    layout->addWidget(label);

    label = new QLabel();
    label->setOpenExternalLinks(true);
    label->setText(tr("<a href=\"http://docs.aws.amazon.com/console/lumberyard/userguide/script-canvas\">Getting Started with Script Canvas</a>."));
    layout->addWidget(label);

    label = new QLabel();
    layout->addWidget(label);

    m_dontShowAgainCheckbox = new QCheckBox();
    m_dontShowAgainCheckbox->setText(tr("Don't show this again."));
    layout->addWidget(m_dontShowAgainCheckbox);

    QHBoxLayout* buttonLayout = new QHBoxLayout();

    QPushButton* okButton = new QPushButton();
    okButton->setText(tr("Ok"));
    okButton->setDefault(true);
    buttonLayout->addWidget(okButton);
    connect(okButton, &QPushButton::clicked, this, &QDialog::close);

    QPushButton* scButton = new QPushButton();
    scButton->setText(tr("Try Script Canvas"));
    scButton->setDefault(false);
    buttonLayout->addWidget(scButton);
    connect(scButton, &QPushButton::clicked, this, &FlowGraphNotificationDialog::OnTryScriptCanvas);

    layout->addLayout(buttonLayout);

    setLayout(layout);
}

void FlowGraphNotificationDialog::OnTryScriptCanvas()
{
    close();

    AzToolsFramework::EditorRequests::Bus::Broadcast(&AzToolsFramework::EditorRequests::CloseViewPane, LyViewPane::LegacyFlowGraph);

    AzToolsFramework::EditorRequests::Bus::Broadcast(&AzToolsFramework::EditorRequests::OpenViewPane, LyViewPane::ScriptCanvas);
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
