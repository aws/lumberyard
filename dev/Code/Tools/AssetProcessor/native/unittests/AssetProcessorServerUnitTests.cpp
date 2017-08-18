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
#include "AssetProcessorServerUnitTests.h"
#include "native/connection/connection.h"
#include "native/connection/connectionManager.h"

#define FEATURE_TEST_LISTEN_PORT 12125

AssetProcessorServerUnitTest::AssetProcessorServerUnitTest()
{
    m_connectionManager = ConnectionManager::Get();
    m_iniConfiguration.readINIConfigFile();
    m_iniConfiguration.SetListeningPort(FEATURE_TEST_LISTEN_PORT);
    m_iniConfiguration.SetProxyInformation("localhost:45643");

    m_applicationServer.startListening(FEATURE_TEST_LISTEN_PORT); // a port that is not the normal port
    m_connectionManager->ReadProxyServerInformation();
    connect(&m_applicationServer, SIGNAL(newIncomingConnection(qintptr)), m_connectionManager, SLOT(NewConnection(qintptr)));
}

void AssetProcessorServerUnitTest::AssetProcessorServerTest()
{
}

void AssetProcessorServerUnitTest::StartTest()
{
    RunFirstPartOfUnitTestsForAssetProcessorServer();
}

int AssetProcessorServerUnitTest::UnitTestPriority() const
{
    return -5;
}

void AssetProcessorServerUnitTest::RunFirstPartOfUnitTestsForAssetProcessorServer()
{
    m_numberOfDisconnectionReceived = 0;
    m_connection = connect(m_connectionManager, SIGNAL(ConnectionError(unsigned int, QString)), this, SLOT(ConnectionErrorForNonProxyMode(unsigned int, QString)));
    m_connectionManager->SetProxyConnect(false);
    m_connectionId = m_connectionManager->addConnection();
    Connection* connection = m_connectionManager->getConnection(m_connectionId);
    connection->SetPort(FEATURE_TEST_LISTEN_PORT);
    connection->SetIpAddress("127.0.0.1");
    connection->SetAutoConnect(true);
}

void AssetProcessorServerUnitTest::ConnectionErrorForNonProxyMode(unsigned int connId, QString error)
{
    if ((connId == 10 || connId == 11))
    {
        if (QString::compare(error, "Attempted to negotiate with self") == 0)
        {
            m_gotNegotiationWithSelfError = true;
        }
        ++m_numberOfDisconnectionReceived;
    }

    if (m_numberOfDisconnectionReceived == 2)
    {
        m_connectionManager->removeConnection(m_connectionId);
        disconnect(m_connection);
        if (!m_gotNegotiationWithSelfError)
        {
            Q_EMIT UnitTestFailed("Unexpected Error Received");
        }
        else
        {
            Q_EMIT UnitTestPassed();
        }
    }
}

#include <native/unittests/AssetProcessorServerUnitTests.moc>

REGISTER_UNIT_TEST(AssetProcessorServerUnitTest)

