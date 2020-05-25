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
#include "native/utilities/ApplicationServer.h"
#include "native/utilities/assetUtils.h"
#include "native/assetprocessor.h"
#include <QTcpSocket>
#include <QCoreApplication>
#include <AzCore/Debug/Trace.h>


ApplicationServer::ApplicationServer(QObject* parent)
    : QTcpServer(parent)
{
}

ApplicationServer::~ApplicationServer()
{
    ApplicationServerBus::Handler::BusDisconnect();
}

void ApplicationServer::incomingConnection(qintptr socketDescriptor)
{
    if (m_isShuttingDown)
    {
        // deny the connection and return.
        QTcpSocket sock;
        sock.setSocketDescriptor(socketDescriptor, QAbstractSocket::ConnectedState, QIODevice::ReadWrite);
        sock.close();
        return;
    }

    Q_EMIT newIncomingConnection(socketDescriptor);
}

bool ApplicationServer::startListening(unsigned short port)
{
    if (!isListening())
    {
        if (port == 0)
        {
            quint16 listeningPort = AssetUtilities::ReadListeningPortFromBootstrap();
            m_serverListeningPort = static_cast<int>(listeningPort);

#if defined(BATCH_MODE)
            // In batch mode, make sure we use a different port from the GUI
            ++m_serverListeningPort;
#endif
        }
        else
        {
            // override the port
            m_serverListeningPort = port;
        }

#if defined(BATCH_MODE)
        // Since we're starting up builders ourselves and informing them of the port chosen, we can scan for a free port

        while (!listen(QHostAddress::Any, m_serverListeningPort))
        {
            auto error = serverError();
            
            if (error == QAbstractSocket::AddressInUseError)
            {
                ++m_serverListeningPort;
            }
            else
            {
                AZ_Error(AssetProcessor::ConsoleChannel, false, "Failed to start Asset Processor server.  Error: %s", errorString().toStdString().c_str());
                return false;
            }
        }
#else
        if (!listen(QHostAddress::Any, aznumeric_cast<quint16>(m_serverListeningPort)))
        {
            AZ_Error(AssetProcessor::ConsoleChannel, false, "Cannot start Asset Processor server - another instance of the Asset Processor may already be running on port number %d.  If you'd like to run multiple Asset Processors on different branches at the same time, please edit bootstrap.cfg and assign different remote_port values to each branch instance.\n", m_serverListeningPort);
            return false;
        }
#endif

        ApplicationServerBus::Handler::BusConnect();
        AZ_TracePrintf(AssetProcessor::DebugChannel, "Asset Processor server listening on port %d\n", m_serverListeningPort);
    }
    return true;
}

int ApplicationServer::GetServerListeningPort() const
{
    return m_serverListeningPort;
}

void ApplicationServer::QuitRequested()
{
    // stop accepting messages and close the connection immediately.
    pauseAccepting();
    close();
    Q_EMIT ReadyToQuit(this);
}

#include <native/utilities/ApplicationServer.moc>

