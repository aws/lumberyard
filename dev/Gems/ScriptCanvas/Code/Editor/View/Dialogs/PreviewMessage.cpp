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

#include "precompiled.h"
#include "PreviewMessage.h"
#include <Editor/Settings.h>
#include <AzCore/UserSettings/UserSettingsProvider.h>

#include <ScriptCanvas/Bus/ScriptCanvasBus.h>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

namespace ScriptCanvasEditor
{


    PreviewMessageDialog::PreviewMessageDialog(QWidget* parent /* = nullptr */)
        : AzQtComponents::StyledDialog(parent, Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowCloseButtonHint | Qt::WindowTitleHint)
    {
        setAttribute(Qt::WA_DeleteOnClose);
        setModal(true);
        setWindowModality(Qt::ApplicationModal);
        setWindowTitle(tr("Welcome to Script Canvas"));
        setSizeGripEnabled(false);
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Minimum);
        const int maximumWidth = 420;

        QPoint dialogCenter = mapToGlobal(rect().center());
        QPoint parentWindowCenter = parent->mapToGlobal(window()->rect().center());
        move(parentWindowCenter - dialogCenter);

        QVBoxLayout* layout = new QVBoxLayout();
        layout->setSpacing(8);
        layout->setSizeConstraint(QLayout::SetFixedSize); // Prevent the user from resizing the dialog

        QLabel* label = new QLabel();
        label->setText(tr("Welcome to Script Canvas <i>preview</i>, a new visual scripting system that unlocks Lumberyard functionality previously available only to programmers."));
        label->setWordWrap(true);
        label->setFixedWidth(maximumWidth);
        layout->addWidget(label);

        label = new QLabel();
        label->setOpenExternalLinks(true);
        label->setWordWrap(true);
        label->setFixedWidth(maximumWidth);
        label->setText(tr("Get started with the <a href=\"http://docs.aws.amazon.com/console/lumberyard/userguide/script-canvas\">Script Canvas</a> topic in the Lumberyard User Guide and the <a href=\"http://docs.aws.amazon.com/console/lumberyard/userguide/script-canvas-sample\">Script Canvas Basic Sample</a>."));
        layout->addWidget(label);

        label = new QLabel();
        label->setOpenExternalLinks(true);
        label->setWordWrap(true);
        label->setFixedWidth(maximumWidth);
        label->setText(tr("We've only just begun and Script Canvas will change, improve, and grow in future releases based on your feedback. Tell us what you like or what's missing, report bugs, or join the discussion on our <a href=\"https://gamedev.amazon.com/forums/spaces/115/scripting.html\">forums</a>."));
        layout->addWidget(label);

        m_dontShowAgainCheckbox = new QCheckBox();
        m_dontShowAgainCheckbox->setText(tr("Don't show this again."));
        m_dontShowAgainCheckbox->setChecked(false);
        layout->addWidget(m_dontShowAgainCheckbox);

        QPushButton* okButton = new QPushButton();
        okButton->setText(tr("Get Started!"));
        okButton->setDefault(true);
        layout->addWidget(okButton);
        connect(okButton, &QPushButton::clicked, this, &QDialog::close);

        setLayout(layout);
    }

    void PreviewMessageDialog::closeEvent(QCloseEvent* evt)
    {
        AZStd::intrusive_ptr<EditorSettings::ScriptCanvasEditorSettings> settings = AZ::UserSettings::CreateFind<EditorSettings::ScriptCanvasEditorSettings>(AZ_CRC("ScriptCanvasPreviewSettings", 0x1c5a2965), AZ::UserSettings::CT_LOCAL);
        if (settings)
        {
            settings->m_showPreviewMessage = (m_dontShowAgainCheckbox->checkState() != Qt::CheckState::Checked);
            AZ::UserSettingsOwnerRequestBus::Event(AZ::UserSettings::CT_LOCAL, &AZ::UserSettingsOwnerRequests::SaveSettings);
        }
        
        QDialog::closeEvent(evt);
    }

    void PreviewMessageDialog::Show()
    {
        raise();

        Qt::WindowFlags flags = windowFlags();
        setWindowFlags(flags | Qt::Tool);
        setModal(false);
        show();

    }


}
#include <Editor/View/Dialogs/PreviewMessage.moc>
