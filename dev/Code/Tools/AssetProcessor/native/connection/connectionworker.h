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
#ifndef ASSETPROCESSOR_CONNECTIONWORKER_H
#define ASSETPROCESSOR_CONNECTIONWORKER_H

#include <QTcpSocket>
#include "native/connection/connectionMessages.h"

#include <AzCore/Memory/Memory.h>
#include <AzCore/Memory/SystemAllocator.h>

/** This Class is responsible for connecting to the client
 * And also proxying connections to an upstream Asset Processor
 */

#undef SendMessage

namespace AssetProcessor
{
    class ConnectionWorker
        : public QObject
    {
        Q_OBJECT

    public:
        explicit ConnectionWorker(qintptr socketDescriptor = -1, bool m_inProxyMode = false, QObject* parent = 0);
        virtual ~ConnectionWorker();
        QTcpSocket& GetSocket();
        QTcpSocket& GetProxySocket();
        void Reset();
        bool Terminate();
        bool IsProxyMode() const;

        bool ReadMessage(QTcpSocket& socket, AssetProcessor::Message& message);
        bool ReadData(QTcpSocket& socket, char* buffer, qint64 size);
        bool WriteMessage(QTcpSocket& socket, const AssetProcessor::Message& message);
        bool WriteData(QTcpSocket& socket, const char* buffer, qint64 size);

        //! True if we initiated the connection, false if someone connected to us.
        bool InitiatedConnection() const;

Q_SIGNALS:
        void ReceiveMessage(unsigned int type, unsigned int serial, QByteArray payload);
        void SocketIPAddress(QString ipAddress);
        void SocketPort(int port);
        void Identifier(QString identifier);
        void AssetPlatform(QString platform);
        void ConnectionDisconnected();
        void ConnectionEstablished(QString ipAddress, quint16 port);
        void ErrorMessage(QString msg);

        // the token identifies the unique connection instance, since multiple may have the same address
        void IsAddressWhiteListed(QHostAddress hostAddress, void* token); 

    public Q_SLOTS:
        void ConnectSocket(qintptr socketDescriptor);
        void ConnectToEngine(QString ipAddress, quint16 port);
        void ConnectProxySocket(QString proxyAddr, quint16 proxyPort);
        void EngineSocketHasData();
        void UpstreamSocketHasData();
        void UpstreamSocketStateChanged(QAbstractSocket::SocketState socketState);
        void EngineSocketStateChanged(QAbstractSocket::SocketState socketState);
        void SendMessage(unsigned int type, unsigned int serial, QByteArray payload);
        void DisconnectSockets();
        void RequestTerminate();
        void SetProxyMode(bool IsProxyMode);
        bool NegotiateDirect(bool initiate);
        bool NegotiateProxy(bool initiate);

        // the token will be the same token sent in the whitelisting request.
        void AddressIsWhiteListed(void* token, bool result);

    private Q_SLOTS:
        void TerminateConnection();

    protected:
        void OnProxyNegotiated();
        

    private:
        QTcpSocket m_engineSocket;
        QTcpSocket m_upstreamSocket;
        bool m_inProxyMode;
        volatile bool m_terminate;
        volatile bool m_alreadySentTermination = false;
        bool m_initiatedConnection = false;
        bool m_upstreamSocketIsConnected = false;
        bool m_engineSocketIsConnected = false;
        int m_waitDelay = 500; // we expect more-or-less instantaneous connection
    };
} // namespace AssetProcessor

#endif // ASSETPROCESSOR_CONNECTIONWORKER_H
