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
#include "native/utilities/assetUtils.h"
#include <AzFramework/Network/AssetProcessorConnection.h>
#include <AzCore/std/parallel/thread.h>
#include <AzCore/std/chrono/clocks.h>
#include <AzCore/std/bind/bind.h>

#include <AzFramework/StringFunc/StringFunc.h>

#define FEATURE_TEST_LISTEN_PORT 12125
#define WAIT_TIME_IN_MSECS 20
// Enable this define only if you are debugging a deadlock/timing issue etc in the AssetProcessorConnection, which you are unable to reproduce otherwise.
// enabling this define will result in a lot more number of connections that connect/disconnect with AP and therefore will result in the unit tests taking a lot more time to complete
//#define DEBUG_ASSETPROCESSORCONNECTION

#if defined(DEBUG_ASSETPROCESSORCONNECTION)
#define NUMBER_OF_CONNECTION 5
#define NUMBER_OF_ITERATION  100
#define NUMBER_OF_TRIES 10
#else
#define NUMBER_OF_CONNECTION 1
#define NUMBER_OF_ITERATION  10
#define NUMBER_OF_TRIES 3
#endif

AssetProcessorServerUnitTest::AssetProcessorServerUnitTest()
{
    m_connectionManager = ConnectionManager::Get();
    m_iniConfiguration.readINIConfigFile();
    m_iniConfiguration.SetListeningPort(FEATURE_TEST_LISTEN_PORT);

    m_applicationServer.startListening(FEATURE_TEST_LISTEN_PORT); // a port that is not the normal port
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
    m_connectionId = m_connectionManager->addConnection();
    Connection* connection = m_connectionManager->getConnection(m_connectionId);
    connection->SetPort(FEATURE_TEST_LISTEN_PORT);
    connection->SetIpAddress("127.0.0.1");
    connection->SetAutoConnect(true);
}

void AssetProcessorServerUnitTest::RunAssetProcessorConnectionStressTest(bool failNegotiation)
{

    AZStd::string azAppRoot = AZStd::string(QDir::current().absolutePath().toUtf8().constData());
    AZStd::string azBranchToken;
    AzFramework::StringFunc::AssetPath::CalculateBranchToken(azAppRoot, azBranchToken);

    QString branchToken(azBranchToken.c_str());

    if (failNegotiation)
    {
        branchToken = branchToken.append("invalid"); //invalid branch token will result in negotiation to fail
    }

    AZStd::atomic_int numberOfConnection(0);

    enum : int { totalConnections = NUMBER_OF_CONNECTION * NUMBER_OF_TRIES };

    AZStd::function<void(int)> StartConnection = [&branchToken, &numberOfConnection](int numTimeWait)
    {
        for (int idx = 0; idx < NUMBER_OF_TRIES; ++idx)
        {
            AzFramework::AssetSystem::AssetProcessorConnection connection;
            connection.Configure(branchToken.toUtf8().data(), "pc", "UNITTEST"); // UNITTEST identifier will skip the processID validation during negotiation
            connection.Connect("127.0.0.1", FEATURE_TEST_LISTEN_PORT);
            while (!connection.IsConnected() && !connection.NegotiationFailed())
            {
                AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(WAIT_TIME_IN_MSECS));
            }

            AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(numTimeWait + idx));
            numberOfConnection.fetch_sub(1);
        }
    };

    AZStd::vector<AZStd::thread> assetProcessorConnectionList;
    for (int iteration = 0; iteration < NUMBER_OF_ITERATION; ++iteration)
    {
        numberOfConnection = totalConnections;

        for (int idx = 0; idx < NUMBER_OF_CONNECTION; ++idx)
        {
            assetProcessorConnectionList.push_back(AZStd::thread(AZStd::bind(StartConnection, iteration)));
        };

        // We need to process all events, since AssetProcessorServer is also on the same thread
        while (numberOfConnection.load())
        {
            QCoreApplication::processEvents(QEventLoop::WaitForMoreEvents, WAIT_TIME_IN_MSECS);
        }

        for (int idx = 0; idx < NUMBER_OF_CONNECTION; ++idx)
        {
            if (assetProcessorConnectionList[idx].joinable())
            {
                assetProcessorConnectionList[idx].join();
            }
        }

        assetProcessorConnectionList.clear();
    }
}

void AssetProcessorServerUnitTest::AssetProcessorConnectionStressTest()
{
    // UnitTest for testing the AssetProcessorConnection by creating lot of connections that connects to AP and then disconnecting them at different times
    // This test should detect any deadlocks that can arise due to rapidly connecting/disconnecting connections
    UnitTestUtils::AssertAbsorber assertAbsorber;

    // Testing the case when negotiation succeeds
    RunAssetProcessorConnectionStressTest(false);

    UNIT_TEST_CHECK(assertAbsorber.m_numErrorsAbsorbed == 0);
    UNIT_TEST_CHECK(assertAbsorber.m_numAssertsAbsorbed == 0);

    // Testing the case when negotiation fails
    RunAssetProcessorConnectionStressTest(true);

    UNIT_TEST_CHECK(assertAbsorber.m_numErrorsAbsorbed == 0);
    UNIT_TEST_CHECK(assertAbsorber.m_numAssertsAbsorbed == 0);

    Q_EMIT UnitTestPassed();
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
            AssetProcessorConnectionStressTest();
        }
    }
}

#include <native/unittests/AssetProcessorServerUnitTests.moc>

REGISTER_UNIT_TEST(AssetProcessorServerUnitTest)

