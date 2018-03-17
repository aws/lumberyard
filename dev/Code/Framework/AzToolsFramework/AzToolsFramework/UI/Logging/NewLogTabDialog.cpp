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

namespace AzToolsFramework
{
    namespace LogPanel
    {
        NewLogTabDialog::NewLogTabDialog(QWidget* pParent)
            : QDialog(pParent)
        {
            uiManager.setupUi(this);

            AZStd::vector<AZStd::string> knownWindows;
            EBUS_EVENT(LegacyFramework::LogComponentAPI::Bus, EnumWindowTypes, knownWindows);

            // hardwired into the incoming message filter, "All" will take any source's message
            uiManager.windowNameBox->addItem("All");

            for (AZStd::vector<AZStd::string>::iterator it = knownWindows.begin(); it != knownWindows.end(); ++it)
            {
                uiManager.windowNameBox->addItem(it->c_str());
            }
        }

        void NewLogTabDialog::attemptAccept()
        {
            m_windowName = uiManager.windowNameBox->currentText();
            m_tabName = uiManager.tabNameEdit->text();
            m_textFilter = uiManager.filterEdit->text();
            m_checkNormal = uiManager.checkNormal->isChecked();
            m_checkWarning = uiManager.checkWarning->isChecked();
            m_checkError = uiManager.checkError->isChecked();
            m_checkDebug = uiManager.checkDebug->isChecked();

            if (m_tabName.isEmpty())
            {
                QApplication::beep();
                return;
            }

            emit accept();
        }
    } // namespace LogPanel
} // namespace AzToolsFramework