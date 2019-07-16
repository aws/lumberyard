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

#include <AzToolsFramework/UI/LegacyFramework/Core/EditorFrameworkAPI.h>

#include "NewLogTabDialog.h"
#include <UI/Logging/NewLogTabDialog.moc>
#include <UI/Logging/ui_NewLogTabDialog.h>
#include <QPushButton>
#include <QLineEdit>
#include <QDialogButtonBox>

namespace AzToolsFramework
{
    namespace LogPanel
    {
        NewLogTabDialog::NewLogTabDialog(QWidget* pParent)
            : QDialog(pParent)
            , uiManager(new Ui::newLogDialog)
        {
            uiManager->setupUi(this);

            // hardwired into the incoming message filter, "All" will take any source's message
            uiManager->windowNameBox->addItem("All");

            AZStd::vector<AZStd::string> knownWindows;
            EBUS_EVENT(LegacyFramework::LogComponentAPI::Bus, EnumWindowTypes, knownWindows);

            if (knownWindows.size() > 0)
            {
                for (AZStd::vector<AZStd::string>::iterator it = knownWindows.begin(); it != knownWindows.end(); ++it)
                {
                    uiManager->windowNameBox->addItem(it->c_str());
                }
            }
            else
            {
                uiManager->windowNameBox->hide();
                uiManager->windowNameBoxLabel->hide();
            }

            QPushButton* ok = uiManager->buttonBox->button(QDialogButtonBox::Ok);
            ok->setEnabled(false);
            connect(uiManager->tabNameEdit, &QLineEdit::textChanged, this, [this, ok]() {
                QString text = uiManager->tabNameEdit->text();
                bool okEnabled = text.length() > 0;
                ok->setEnabled(okEnabled);

                if (!okEnabled)
                {
                    ok->setToolTip("Please enter a valid tab name before saving");
                }
                else
                {
                    ok->setToolTip("");
                }
            });
        }

        NewLogTabDialog::~NewLogTabDialog()
        {
            // defined in here so that ui_NewLogTabDialog.h doesn't have to be included in the header file
        }

        void NewLogTabDialog::attemptAccept()
        {
            m_windowName = uiManager->windowNameBox->currentText();
            m_tabName = uiManager->tabNameEdit->text();
            m_textFilter = uiManager->filterEdit->text();
            m_checkNormal = uiManager->checkNormal->isChecked();
            m_checkWarning = uiManager->checkWarning->isChecked();
            m_checkError = uiManager->checkError->isChecked();
            m_checkDebug = uiManager->checkDebug->isChecked();

            if (m_tabName.isEmpty())
            {
                uiManager->tabNameEdit->setFocus();
                QApplication::beep();
                return;
            }

            emit accept();
        }
    } // namespace LogPanel
} // namespace AzToolsFramework