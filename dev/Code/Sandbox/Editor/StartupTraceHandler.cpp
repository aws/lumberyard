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
#include "StartupTraceHandler.h"
#include <AzCore/Component/TickBus.h>
#include "ErrorDialog.h"

#include <QMessageBox>

namespace SandboxEditor
{
    StartupTraceHandler::StartupTraceHandler()
    {
        ConnectToMessageBus();
    }

    StartupTraceHandler::~StartupTraceHandler()
    {
        EndCollectionAndShowCollectedErrors();
        DisconnectFromMessageBus();
    }

    bool StartupTraceHandler::OnPreAssert(const char* /*fileName*/, int /*line*/, const char* /*func*/, const char* message)
    {
        // Asserts are more fatal than errors, and need to be displayed right away.
        // After the assert occurs, nothing else may be functional enough to collect and display messages.
        OnMessage(message, nullptr, MessageDisplayBehavior::AlwaysShow);
        // Return false so other listeners can handle this. The StartupTraceHandler won't report messages 
        // until the next time the main thread updates the system tick bus function queue. The editor 
        // will probably crash before that occurs, because this is an assert.
        return false;
    }

    bool StartupTraceHandler::OnException(const char* message)
    {
        // Exceptions are more fatal than errors, and need to be displayed right away.
        // After the exception occurs, nothing else may be functional enough to collect and display messages.
        OnMessage(message, nullptr, MessageDisplayBehavior::AlwaysShow);
        // Return false so other listeners can handle this. The StartupTraceHandler won't report messages 
        // until the next time the main thread updates the system tick bus function queue. The editor 
        // will probably crash before that occurs, because this is an exception.
        return false;
    }

    bool StartupTraceHandler::OnPreError(const char* /*window*/, const char* /*fileName*/, int /*line*/, const char* /*func*/, const char* message)
    {
        // If a collection is not active, then show this error. Otherwise, collect it 
        // and show it with other occurs that occured in the operation.
        OnMessage(message, &m_errors, MessageDisplayBehavior::ShowWhenNotCollecting);
        return true;
    }

    bool StartupTraceHandler::OnPreWarning(const char* /*window*/, const char* /*fileName*/, int /*line*/, const char* /*func*/, const char* message)
    {
        // Only track warnings if messages are being collected.
        OnMessage(message, &m_warnings, MessageDisplayBehavior::OnlyCollect);
        return true;
    }

    void StartupTraceHandler::OnMessage(
        const char *message,
        AZStd::list<QString>* messageList,
        MessageDisplayBehavior messageDisplayBehavior)
    {
        AZStd::lock_guard<AZStd::recursive_mutex> lock(m_mutex);

        if (m_isCollecting && messageList)
        {
            messageList->push_back(QString(message));
        }
        else if (
            (!m_isCollecting && messageDisplayBehavior == MessageDisplayBehavior::ShowWhenNotCollecting) ||
            messageDisplayBehavior == MessageDisplayBehavior::AlwaysShow)
        {
            ShowMessageBox(message);
        }
    }

    void StartupTraceHandler::StartCollection()
    {
        AZStd::lock_guard<AZStd::recursive_mutex> lock(m_mutex);
        ConnectToMessageBus();
        if (m_isCollecting)
        {
            EndCollectionAndShowCollectedErrors();
        }
        m_isCollecting = true;
    }

    void StartupTraceHandler::EndCollectionAndShowCollectedErrors()
    {
        AZ::SystemTickBus::QueueFunction([this]() 
        {
            AZStd::lock_guard<AZStd::recursive_mutex> lock(m_mutex);
            m_isCollecting = false;
            ShowMessages(m_warnings, m_errors);
            ClearMessages();
        });
    }

    void StartupTraceHandler::ShowMessageBox(const QString& message)
    {
        AZ::SystemTickBus::QueueFunction([this, message]()
        {
            // Parent to the main window, so that the error dialog doesn't
            // show up as a separate window when alt-tabbing.
            QWidget* mainWindow = nullptr;
            AzToolsFramework::EditorRequests::Bus::BroadcastResult(
                mainWindow,
                &AzToolsFramework::EditorRequests::Bus::Events::GetMainWindow);

            QMessageBox msg(
                QMessageBox::Critical, 
                QObject::tr("Error"), 
                message, 
                QMessageBox::Ok,
                mainWindow);
            msg.exec();
        });
    }

    void StartupTraceHandler::ConnectToMessageBus()
    {
        AZ::Debug::TraceMessageBus::Handler::BusConnect();
    }
    void StartupTraceHandler::DisconnectFromMessageBus()
    {
        AZ::Debug::TraceMessageBus::Handler::BusDisconnect();
    }

    bool StartupTraceHandler::IsConnectedToMessageBus() const
    {
        return AZ::Debug::TraceMessageBus::Handler::BusIsConnected();
    }

    void StartupTraceHandler::ClearMessages()
    {
        AZStd::lock_guard<AZStd::recursive_mutex> lock(m_mutex);
        m_errors.clear();
        m_warnings.clear();
    }

    void StartupTraceHandler::ShowMessages(
        const AZStd::list<QString>& warnings,
        const AZStd::list<QString>& errors
        )
    {
        // If more than one error was found, or any warnings were found, display
        // the errors and warnings in one dialog window.
        if (errors.size() == 0 && warnings.size() == 0)
        {
            return;
        }
        // Parent to the main window, so that the error dialog doesn't
        // show up as a separate window when alt-tabbing.
        QWidget* mainWindow = nullptr;
        AzToolsFramework::EditorRequests::Bus::BroadcastResult(
            mainWindow,
            &AzToolsFramework::EditorRequests::Bus::Events::GetMainWindow);

        ErrorDialog errorDialog(mainWindow);
        errorDialog.AddMessages(
            SandboxEditor::ErrorDialog::MessageType::Error,
            errors);
        errorDialog.AddMessages(
            SandboxEditor::ErrorDialog::MessageType::Warning,
            warnings);
        errorDialog.exec();
    }
}
