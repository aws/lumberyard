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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

// Description : Dialog for python script terminal


#include "StdAfx.h"
#include "ScriptTermDialog.h"
#include "ScriptHelpDialog.h"
#include <ui_ScriptTermDialog.h>
#include "QtViewPaneManager.h"

#include <QtUtil.h>

#include <QFont>
#include <QCompleter>
#include <QStringListModel>
#include <QStringBuilder>

#include <AzToolsFramework/API/EditorPythonRunnerRequestsBus.h>

CScriptTermDialog::CScriptTermDialog(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::CScriptTermDialog)
{
    ui->setupUi(this);
    AzToolsFramework::EditorPythonConsoleNotificationBus::Handler::BusConnect();

    ui->SCRIPT_INPUT->installEventFilter(this);

    connect(ui->SCRIPT_HELP, &QToolButton::clicked, this, &CScriptTermDialog::OnScriptHelp);
    connect(ui->SCRIPT_INPUT, &QLineEdit::returnPressed, this, &CScriptTermDialog::OnOK);
    connect(ui->SCRIPT_INPUT, &QLineEdit::textChanged, this, &CScriptTermDialog::OnScriptInputTextChanged);

    InitCompleter();
}

CScriptTermDialog::~CScriptTermDialog()
{
    AzToolsFramework::EditorPythonConsoleNotificationBus::Handler::BusDisconnect();
}

void CScriptTermDialog::RegisterViewClass()
{
    AzToolsFramework::ViewPaneOptions options;
    options.canHaveMultipleInstances = true;
    options.sendViewPaneNameBackToAmazonAnalyticsServers = true;
    AzToolsFramework::RegisterViewPane<CScriptTermDialog>(SCRIPT_TERM_WINDOW_NAME, LyViewPane::CategoryOther, options);
}

void CScriptTermDialog::InitCompleter()
{
    QStringList inputs;

    AzToolsFramework::EditorPythonConsoleInterface* editorPythonConsoleInterface = AZ::Interface<AzToolsFramework::EditorPythonConsoleInterface>::Get();
    if (editorPythonConsoleInterface)
    {
        AzToolsFramework::EditorPythonConsoleInterface::GlobalFunctionCollection globalFunctionCollection;
        editorPythonConsoleInterface->GetGlobalFunctionList(globalFunctionCollection);
        for (const AzToolsFramework::EditorPythonConsoleInterface::GlobalFunction& globalFunction : globalFunctionCollection)
        {
            inputs.append(QString("%1.%2()").arg(globalFunction.m_moduleName.data()).arg(globalFunction.m_functionName.data()));
        }
    }

    m_lastCommandModel = new QStringListModel(ui->SCRIPT_INPUT);
    m_completionModel = new QStringListModel(inputs, ui->SCRIPT_INPUT);
    auto completer = new QCompleter(m_completionModel, ui->SCRIPT_INPUT);
    ui->SCRIPT_INPUT->setCompleter(completer);
}

void CScriptTermDialog::OnOK()
{
    const QString command = ui->SCRIPT_INPUT->text();
    QString command2 = QLatin1String("] ")
        % ui->SCRIPT_INPUT->text()
        % QLatin1String("\r\n");
    AppendToConsole(command2);

    // Add the command to the history.
    m_lastCommands.removeOne(command);
    if (!command.isEmpty())
    {
        m_lastCommands.prepend(command);
    }

    ExecuteAndPrint(command.toLocal8Bit().data());
    //clear script input via a QueuedConnection because completer sets text when it's done, undoing our clear.
    QMetaObject::invokeMethod(ui->SCRIPT_INPUT, "setText", Qt::QueuedConnection, Q_ARG(QString, ""));
}

void CScriptTermDialog::OnScriptInputTextChanged(const QString& text)
{
    if (text.isEmpty())
    {
        ui->SCRIPT_INPUT->completer()->setModel(m_completionModel);
    }
}


void CScriptTermDialog::OnScriptHelp()
{
    CScriptHelpDialog::GetInstance()->show();
}

void CScriptTermDialog::ExecuteAndPrint(const char* cmd)
{
    if (AzToolsFramework::EditorPythonRunnerRequestBus::HasHandlers())
    {
        AzToolsFramework::EditorPythonRunnerRequestBus::Broadcast(&AzToolsFramework::EditorPythonRunnerRequestBus::Events::ExecuteByString, cmd);
    }
    else
    {
        AZ_Warning("python", false, "EditorPythonRunnerRequestBus has no handlers");
    }
}

void CScriptTermDialog::AppendText(const char* pText)
{
    AppendToConsole(QtUtil::ToQString(pText));
}

void CScriptTermDialog::OnTraceMessage(AZStd::string_view message)
{
    AppendToConsole(QtUtil::ToQString(message.data()));
}

void CScriptTermDialog::OnErrorMessage(AZStd::string_view message)
{
    AppendToConsole(QtUtil::ToQString(message.data()), QColor(255, 64, 64));
}

void CScriptTermDialog::OnExceptionMessage(AZStd::string_view message)
{
    AppendToConsole(QtUtil::ToQString(message.data()), QColor(128, 64, 64));
}

void CScriptTermDialog::AppendToConsole(const QString& string, const QColor& color)
{
    QTextCharFormat format;
    format.setForeground(color);

    QTextCursor cursor(ui->SCRIPT_OUTPUT->document());
    cursor.movePosition(QTextCursor::End);
    cursor.insertText(string, format);
}

bool CScriptTermDialog::eventFilter(QObject* obj, QEvent* e)
{
    if (e->type() == QEvent::KeyPress)
    {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(e);
        if (keyEvent->key() == Qt::Key_Down)
        {
            m_lastCommandModel->setStringList(m_lastCommands);
            ui->SCRIPT_INPUT->completer()->setModel(m_lastCommandModel);
            ui->SCRIPT_INPUT->completer()->setCompletionPrefix("");
            ui->SCRIPT_INPUT->completer()->complete();
            m_upArrowLastCommandIndex = -1;
            return true;
        }
        else if (keyEvent->key() == Qt::Key_Up)
        {
            if (!m_lastCommands.isEmpty())
            {
                m_upArrowLastCommandIndex++;
                if (m_upArrowLastCommandIndex < 0)
                {
                    m_upArrowLastCommandIndex = 0;
                }
                else if (m_upArrowLastCommandIndex >= m_lastCommands.size())
                {
                    // Already at the last item, nothing to do
                    return true;
                }

                ui->SCRIPT_INPUT->setText(m_lastCommands.at(m_upArrowLastCommandIndex));
            }
        }
        else
        {
            // Reset cycling, we only want for sequential up arrow presses
            m_upArrowLastCommandIndex = -1;
        }
    }
    return QObject::eventFilter(obj, e);
}



#include <ScriptTermDialog.moc>
