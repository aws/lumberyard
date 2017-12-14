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

#include "FrameworkApplicationFixture.h"

#include <atomic>
#include <gtest/gtest.h>

#include <AzCore/Casting/lossy_cast.h>
#include <AzCore/std/parallel/binary_semaphore.h>
#include <AzFramework/Network/AssetProcessorConnection.h>
#include <AzFramework/Asset/AssetProcessorMessages.h>
#include <AzFramework/Asset/AssetSystemComponent.h>

// This is the type and payload sent from A to B
const AZ::u32 ABType = 0x86;
const char* ABPayload = "Hello World";
const AZ::u32 ABPayloadSize = azlossy_caster(strlen(ABPayload));

// This is the type sent from A to B with no payload
const AZ::u32 ABNoPayloadType = 0x69;

// This is the type and payload sent from B to A
const AZ::u32 BAType = 0xffff0000;
const char* BAPayload = "When in the Course of human events it becomes necessary for one people to dissolve the political bands which have connected them with another and to assume among the powers of the earth, the separate and equal station to which the Laws of Nature and of Nature's God entitle them, a decent respect to the opinions of mankind requires that they should declare the causes which impel them to the separation.";
const AZ::u32 BAPayloadSize = azlossy_caster(strlen(BAPayload));

// used to control how many iterations the connection object waits for state change before assuming things have gone wrong
const int numTriesBeforeGiveUp = 100;
const int millisecondsBetweenTries = 25;

// the longest time it should be conceivable for a message to take to send
const int millisecondsForSend = 250;



class APConnectionTest
    : public UnitTest::FrameworkApplicationFixture
{
protected:

    void SetUp() override
    {
        FrameworkApplicationFixture::SetUp();
    }

    void TearDown() override
    {
        FrameworkApplicationFixture::TearDown();
    }

    bool WaitForConnectionStateToBeEqual(AzFramework::AssetSystem::AssetProcessorConnection& connectionObject, AzFramework::SocketConnection::EConnectionState desired)
    {
        int tries = 0;
        while (connectionObject.GetConnectionState() != desired && tries++ < numTriesBeforeGiveUp)
        {
            AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(millisecondsBetweenTries));
        }
        return connectionObject.GetConnectionState() == desired;
    }


    bool WaitForConnectionStateToNotBeEqual(AzFramework::AssetSystem::AssetProcessorConnection& connectionObject, AzFramework::SocketConnection::EConnectionState notDesired)
    {
        int tries = 0;
        while (connectionObject.GetConnectionState() == notDesired && tries++ < numTriesBeforeGiveUp)
        {
            AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(millisecondsBetweenTries));
        }
        return connectionObject.GetConnectionState() != notDesired;
    }

};

TEST_F(APConnectionTest, Sanity)
{
    // Other values here
    EXPECT_TRUE(true);
}

TEST_F(APConnectionTest, TestAddRemoveCallbacks)
{
    using namespace AzFramework;

    // This is connection A
    AssetSystem::AssetProcessorConnection apConnection;
    apConnection.m_unitTesting = true;

    // This is connection B
    AssetSystem::AssetProcessorConnection apListener;
    apListener.m_unitTesting = true;

    std::atomic_uint BAMessageCallbackCount;
    BAMessageCallbackCount = 0;
    std::atomic_uint ABMessageCallbackCount;
    ABMessageCallbackCount = 0;

    AZStd::binary_semaphore messageArrivedSemaphore;

    // Connection A is expecting the above type and payload from B, therefore it is B->A, BA
    auto BACallbackHandle = apConnection.AddMessageHandler(BAType, [&](AZ::u32 typeId, AZ::u32 /*serial*/, const void* data, AZ::u32 dataLength) -> void
    {
        EXPECT_TRUE(typeId == BAType);
        EXPECT_TRUE(BAPayloadSize == dataLength);
        EXPECT_TRUE(!strncmp(reinterpret_cast<const char*>(data), BAPayload, dataLength));
        ++BAMessageCallbackCount;
        messageArrivedSemaphore.release();
    });

    // Connection B is expecting the above type and payload from A, therefore it is A->B, AB
    auto ABCallbackHandle = apListener.AddMessageHandler(ABType, [&](AZ::u32 typeId, AZ::u32 /*serial*/, const void* data, AZ::u32 dataLength) -> void
    {
        EXPECT_TRUE(typeId == ABType);
        EXPECT_TRUE(ABPayloadSize == dataLength);
        EXPECT_TRUE(!strncmp(reinterpret_cast<const char*>(data), ABPayload, dataLength));
        ++ABMessageCallbackCount;
        messageArrivedSemaphore.release();
    });

    // Test listening
    EXPECT_TRUE(apListener.GetConnectionState() == SocketConnection::EConnectionState::Disconnected);
    bool listenResult = apListener.Listen(11112);
    EXPECT_TRUE(listenResult);

    // Wait some time for the connection to start listening, since it doesn't actually call listen() immediately.
    EXPECT_TRUE(WaitForConnectionStateToBeEqual(apListener, SocketConnection::EConnectionState::Listening));
    EXPECT_TRUE(apListener.GetConnectionState() == SocketConnection::EConnectionState::Listening);

    // Test connect success
    EXPECT_TRUE(apConnection.GetConnectionState() == SocketConnection::EConnectionState::Disconnected);
    // This is blocking, should connect
    bool connectResult = apConnection.Connect("127.0.0.1", 11112);
    EXPECT_TRUE(connectResult);

    // Wait some time for the connection to negotiate, only after negotiation succeeds is it actually considered connected,,
    EXPECT_TRUE(WaitForConnectionStateToBeEqual(apConnection, SocketConnection::EConnectionState::Connected));
    
    // Check listener for success - by this time the listener should also be considered connected.
    EXPECT_TRUE(WaitForConnectionStateToBeEqual(apListener, SocketConnection::EConnectionState::Connected));

    //
    // Send first set, ensure we got 1 each
    //

    // Send message from A to B
    apConnection.SendMsg(ABType, ABPayload, ABPayloadSize);
    // Wait some time to allow message to send
    messageArrivedSemaphore.try_acquire_for(AZStd::chrono::milliseconds(millisecondsForSend));
    EXPECT_TRUE(ABMessageCallbackCount == 1);

    // Send message from B to A
    apListener.SendMsg(BAType, BAPayload, BAPayloadSize);
    messageArrivedSemaphore.try_acquire_for(AZStd::chrono::milliseconds(millisecondsForSend));
    EXPECT_TRUE(BAMessageCallbackCount == 1);

    //
    // Send second set, ensure we got 2 each (didn't auto-remove or anything crazy)
    //

    // Send message from A to B
    apConnection.SendMsg(ABType, ABPayload, ABPayloadSize);
    // Wait some time to allow message to send
    messageArrivedSemaphore.try_acquire_for(AZStd::chrono::milliseconds(millisecondsForSend));
    EXPECT_TRUE(ABMessageCallbackCount == 2);

    // Send message from B to A
    apListener.SendMsg(BAType, BAPayload, BAPayloadSize);
    messageArrivedSemaphore.try_acquire_for(AZStd::chrono::milliseconds(millisecondsForSend));
    EXPECT_TRUE(BAMessageCallbackCount == 2);

    // Remove callbacks
    apConnection.RemoveMessageHandler(BAType, BACallbackHandle);
    apListener.RemoveMessageHandler(ABType, ABCallbackHandle);

    // Send message, ensure we still at 2 each

    // Send message from A to B.  note that there is no semaphore to call here - there will be no handler called.
    apConnection.SendMsg(ABType, ABPayload, ABPayloadSize);
    // Wait some time to allow message to send
    AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(millisecondsForSend));
    EXPECT_TRUE(ABMessageCallbackCount == 2);

    // Send message from B to A
    apListener.SendMsg(BAType, BAPayload, BAPayloadSize);
    AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(millisecondsForSend));
    EXPECT_TRUE(BAMessageCallbackCount == 2);

    //
    // Now try adding listeners that remove themselves during callback
    //
    // Connection A is expecting the above type and payload from B, therefore it is B->A, BA
    SocketConnection::TMessageCallbackHandle SelfRemovingBACallbackHandle = SocketConnection::s_invalidCallbackHandle;
    SelfRemovingBACallbackHandle = apConnection.AddMessageHandler(BAType, [&](AZ::u32 typeId, AZ::u32 /*serial*/, const void* data, AZ::u32 dataLength) -> void
    {
        EXPECT_TRUE(typeId == BAType);
        EXPECT_TRUE(BAPayloadSize == dataLength);
        EXPECT_TRUE(!strncmp(reinterpret_cast<const char*>(data), BAPayload, dataLength));
        ++BAMessageCallbackCount;
        apConnection.RemoveMessageHandler(BAType, SelfRemovingBACallbackHandle);
        messageArrivedSemaphore.release();
        
    });

    // Connection B is expecting the above type and payload from A, therefore it is A->B, AB
    SocketConnection::TMessageCallbackHandle SelfRemovingABCallbackHandle = SocketConnection::s_invalidCallbackHandle;
    SelfRemovingABCallbackHandle = apListener.AddMessageHandler(ABType, [&](AZ::u32 typeId, AZ::u32 /*serial*/, const void* data, AZ::u32 dataLength) -> void
    {
        EXPECT_TRUE(typeId == ABType);
        EXPECT_TRUE(ABPayloadSize == dataLength);
        EXPECT_TRUE(!strncmp(reinterpret_cast<const char*>(data), ABPayload, dataLength));
        ++ABMessageCallbackCount;
        apListener.RemoveMessageHandler(ABType, SelfRemovingABCallbackHandle);
        messageArrivedSemaphore.release();
    });
    // Send message, should be at 3 each

    // Send message from A to B
    apConnection.SendMsg(ABType, ABPayload, ABPayloadSize);
    // Wait some time to allow message to send
    messageArrivedSemaphore.try_acquire_for(AZStd::chrono::milliseconds(millisecondsForSend));
    EXPECT_TRUE(ABMessageCallbackCount == 3);

    // Send message from B to A
    apListener.SendMsg(BAType, BAPayload, BAPayloadSize);
    messageArrivedSemaphore.try_acquire_for(AZStd::chrono::milliseconds(millisecondsForSend));
    EXPECT_TRUE(BAMessageCallbackCount == 3);

    // Send message, ensure we still at 3 each

    // Send message from A to B
    apConnection.SendMsg(ABType, ABPayload, ABPayloadSize);
    // Wait some time to allow message to send
    messageArrivedSemaphore.try_acquire_for(AZStd::chrono::milliseconds(millisecondsForSend));
    EXPECT_TRUE(ABMessageCallbackCount == 3);

    // Send message from B to A
    apListener.SendMsg(BAType, BAPayload, BAPayloadSize);
    messageArrivedSemaphore.try_acquire_for(AZStd::chrono::milliseconds(millisecondsForSend));
    EXPECT_TRUE(BAMessageCallbackCount == 3);

    //
    // Now try adding listeners that add more listeners during callback
    //

    // Connection A is expecting the above type and payload from B, therefore it is B->A, BA
    SocketConnection::TMessageCallbackHandle SecondAddedBACallbackHandle = SocketConnection::s_invalidCallbackHandle;
    SocketConnection::TMessageCallbackHandle AddingBACallbackHandle = SocketConnection::s_invalidCallbackHandle;
    AddingBACallbackHandle = apConnection.AddMessageHandler(BAType, [&](AZ::u32 typeId, AZ::u32 /*serial*/, const void* data, AZ::u32 dataLength) -> void
    {
        EXPECT_TRUE(typeId == BAType);
        EXPECT_TRUE(BAPayloadSize == dataLength);
        EXPECT_TRUE(!strncmp(reinterpret_cast<const char*>(data), BAPayload, dataLength));
        ++BAMessageCallbackCount;
        apConnection.RemoveMessageHandler(BAType, AddingBACallbackHandle);
        SecondAddedBACallbackHandle = apConnection.AddMessageHandler(BAType, [&](AZ::u32 typeId, AZ::u32 /*serial*/, const void* data, AZ::u32 dataLength) -> void
        {
            EXPECT_TRUE(typeId == BAType);
            EXPECT_TRUE(BAPayloadSize == dataLength);
            EXPECT_TRUE(!strncmp(reinterpret_cast<const char*>(data), BAPayload, dataLength));
            ++BAMessageCallbackCount;
            apConnection.RemoveMessageHandler(BAType, SecondAddedBACallbackHandle);
            messageArrivedSemaphore.release();
        });
        messageArrivedSemaphore.release();
    });

    // Connection B is expecting the above type and payload from A, therefore it is A->B, AB
    SocketConnection::TMessageCallbackHandle SecondAddedABCallbackHandle = SocketConnection::s_invalidCallbackHandle;
    SocketConnection::TMessageCallbackHandle AddingABCallbackHandle = SocketConnection::s_invalidCallbackHandle;
    AddingABCallbackHandle = apListener.AddMessageHandler(ABType, [&](AZ::u32 typeId, AZ::u32 /*serial*/, const void* data, AZ::u32 dataLength) -> void
    {
        EXPECT_TRUE(typeId == ABType);
        EXPECT_TRUE(ABPayloadSize == dataLength);
        EXPECT_TRUE(!strncmp(reinterpret_cast<const char*>(data), ABPayload, dataLength));
        ++ABMessageCallbackCount;
        apListener.RemoveMessageHandler(ABType, AddingABCallbackHandle);
        messageArrivedSemaphore.release();
        SecondAddedABCallbackHandle = apListener.AddMessageHandler(ABType, [&](AZ::u32 typeId, AZ::u32 /*serial*/, const void* data, AZ::u32 dataLength) -> void
        {
            EXPECT_TRUE(typeId == ABType);
            EXPECT_TRUE(ABPayloadSize == dataLength);
            EXPECT_TRUE(!strncmp(reinterpret_cast<const char*>(data), ABPayload, dataLength));
            ++ABMessageCallbackCount;
            apListener.RemoveMessageHandler(ABType, SecondAddedABCallbackHandle);
            messageArrivedSemaphore.release();
        });
    });
    // Send message, should be at 4 each

    // Send message from A to B
    apConnection.SendMsg(ABType, ABPayload, ABPayloadSize);
    // Wait some time to allow message to send
    messageArrivedSemaphore.try_acquire_for(AZStd::chrono::milliseconds(millisecondsForSend));
    EXPECT_TRUE(ABMessageCallbackCount == 4);

    // Send message from B to A
    apListener.SendMsg(BAType, BAPayload, BAPayloadSize);
    messageArrivedSemaphore.try_acquire_for(AZStd::chrono::milliseconds(millisecondsForSend));
    EXPECT_TRUE(BAMessageCallbackCount == 4);

    // Send message, should be at 5 each

    // Send message from A to B
    apConnection.SendMsg(ABType, ABPayload, ABPayloadSize);
    // Wait some time to allow message to send
    messageArrivedSemaphore.try_acquire_for(AZStd::chrono::milliseconds(millisecondsForSend));
    EXPECT_TRUE(ABMessageCallbackCount == 5);

    // Send message from B to A
    apListener.SendMsg(BAType, BAPayload, BAPayloadSize);
    messageArrivedSemaphore.try_acquire_for(AZStd::chrono::milliseconds(millisecondsForSend));
    EXPECT_TRUE(BAMessageCallbackCount == 5);

    //Send message, should be at 5 still

    apConnection.SendMsg(ABType, ABPayload, ABPayloadSize);
    // Wait some time to allow message to send
    messageArrivedSemaphore.try_acquire_for(AZStd::chrono::milliseconds(millisecondsForSend));
    EXPECT_TRUE(ABMessageCallbackCount == 5);

    // Send message from B to A
    apListener.SendMsg(BAType, BAPayload, BAPayloadSize);
    messageArrivedSemaphore.try_acquire_for(AZStd::chrono::milliseconds(millisecondsForSend));
    EXPECT_TRUE(BAMessageCallbackCount == 5);

    EXPECT_TRUE(apConnection.GetConnectionState() == SocketConnection::EConnectionState::Connected);
    EXPECT_TRUE(apListener.GetConnectionState() == SocketConnection::EConnectionState::Connected);

    // Disconnect A
    bool disconnectResult = apConnection.Disconnect();
    EXPECT_TRUE(disconnectResult);

    // Disconnect B
    disconnectResult = apListener.Disconnect();
    EXPECT_TRUE(disconnectResult);

    // Verify A and B are disconnected
    EXPECT_TRUE(WaitForConnectionStateToBeEqual(apListener, SocketConnection::EConnectionState::Disconnected));
    EXPECT_TRUE(WaitForConnectionStateToBeEqual(apConnection, SocketConnection::EConnectionState::Disconnected));
}

TEST_F(APConnectionTest, TestConnection)
{
    using namespace AzFramework;

    // This is connection A
    AssetSystem::AssetProcessorConnection apConnection;
    apConnection.m_unitTesting = true;

    // This is connection B
    AssetSystem::AssetProcessorConnection apListener;
    apListener.m_unitTesting = true;

    bool ABMessageSuccess = false;
    bool ABNoPayloadMessageSuccess = false;
    bool BAMessageSuccess = false;

    AZStd::binary_semaphore messageArrivedSemaphore;
    // Connection A is expecting the above type and payload from B, therefore it is B->A, BA
    apConnection.AddMessageHandler(BAType, [&](AZ::u32 typeId, AZ::u32 /*serial*/, const void* data, AZ::u32 dataLength) -> void
    {
        EXPECT_TRUE(typeId == BAType);
        EXPECT_TRUE(BAPayloadSize == dataLength);
        EXPECT_TRUE(!strncmp(reinterpret_cast<const char*>(data), BAPayload, dataLength));
        BAMessageSuccess = true;
        messageArrivedSemaphore.release();
    });

    // Connection B is expecting the above type and payload from A, therefore it is A->B, AB
    apListener.AddMessageHandler(ABType, [&](AZ::u32 typeId, AZ::u32 /*serial*/, const void* data, AZ::u32 dataLength) -> void
    {
        EXPECT_TRUE(typeId == ABType);
        EXPECT_TRUE(ABPayloadSize == dataLength);
        EXPECT_TRUE(!strncmp(reinterpret_cast<const char*>(data), ABPayload, dataLength));
        ABMessageSuccess = true;
        messageArrivedSemaphore.release();
    });

    // Connection B is expecting the above type and no payload from A, therefore it is A->B, AB
    apListener.AddMessageHandler(ABNoPayloadType, [&](AZ::u32 typeId, AZ::u32 /*serial*/, const void* /*data*/, AZ::u32 dataLength) -> void
    {
        EXPECT_TRUE(typeId == ABNoPayloadType);
        EXPECT_TRUE(dataLength == 0);
        ABNoPayloadMessageSuccess = true;
        messageArrivedSemaphore.release();
    });

    // Test connection coming online first
    EXPECT_TRUE(apConnection.GetConnectionState() == SocketConnection::EConnectionState::Disconnected);
    bool connectResult = apConnection.Connect("127.0.0.1", 11120);
    EXPECT_TRUE(connectResult);
    
    // wait for it to start the "try to connect / disconnect / reconnect loop"
    EXPECT_TRUE(WaitForConnectionStateToNotBeEqual(apConnection, SocketConnection::EConnectionState::Disconnected));

    // Test listening on separate port
    EXPECT_TRUE(apListener.GetConnectionState() == SocketConnection::EConnectionState::Disconnected);
    bool listenResult = apListener.Listen(54321);
    EXPECT_TRUE(listenResult);
    
    EXPECT_TRUE(WaitForConnectionStateToBeEqual(apListener, SocketConnection::EConnectionState::Listening));

    // Disconnect listener from wrong port
    bool disconnectResult = apListener.Disconnect();
    EXPECT_TRUE(disconnectResult);
    EXPECT_TRUE(WaitForConnectionStateToBeEqual(apListener, SocketConnection::EConnectionState::Disconnected));

    // Listen with correct port
    listenResult = apListener.Listen(11120);
    EXPECT_TRUE(listenResult);
    // Wait some time for apConnection to connect (it has to finish negotiation)
    // Also the listener needs to tick and connect
    EXPECT_TRUE(WaitForConnectionStateToBeEqual(apConnection, SocketConnection::EConnectionState::Connected));
    EXPECT_TRUE(WaitForConnectionStateToBeEqual(apListener, SocketConnection::EConnectionState::Connected));

    // Send message from A to B
    apConnection.SendMsg(ABType, ABPayload, ABPayloadSize);
    // Wait some time to allow message to send
    messageArrivedSemaphore.try_acquire_for(AZStd::chrono::milliseconds(millisecondsForSend));
    EXPECT_TRUE(ABMessageSuccess);

    // Send message from B to A
    apListener.SendMsg(BAType, BAPayload, BAPayloadSize);
    messageArrivedSemaphore.try_acquire_for(AZStd::chrono::milliseconds(millisecondsForSend));
    EXPECT_TRUE(BAMessageSuccess);

    // Send no payload message from A to B
    apConnection.SendMsg(ABNoPayloadType, nullptr, 0);
    messageArrivedSemaphore.try_acquire_for(AZStd::chrono::milliseconds(millisecondsForSend));
    EXPECT_TRUE(ABNoPayloadMessageSuccess);

    // Disconnect A
    disconnectResult = apConnection.Disconnect();
    EXPECT_TRUE(disconnectResult);
    // Disconnect B
    disconnectResult = apListener.Disconnect();
    EXPECT_TRUE(disconnectResult);

    // Verify they've disconnected
    EXPECT_TRUE(WaitForConnectionStateToBeEqual(apConnection, SocketConnection::EConnectionState::Disconnected));
    EXPECT_TRUE(WaitForConnectionStateToBeEqual(apListener, SocketConnection::EConnectionState::Disconnected));
}

TEST_F(APConnectionTest, TestReconnect)
{
    using namespace AzFramework;

    // This is connection A
    AssetSystem::AssetProcessorConnection apConnection;
    apConnection.m_unitTesting = true;

    // This is connection B
    AssetSystem::AssetProcessorConnection apListener;
    apListener.m_unitTesting = true;

    // Test listening - listen takes a moment to actually start listening:
    EXPECT_TRUE(apListener.GetConnectionState() == SocketConnection::EConnectionState::Disconnected);
    bool listenResult = apListener.Listen(11120);
    EXPECT_TRUE(listenResult);
    EXPECT_TRUE(WaitForConnectionStateToBeEqual(apListener, SocketConnection::EConnectionState::Listening));

    // Test connect success
    EXPECT_TRUE(apConnection.GetConnectionState() == SocketConnection::EConnectionState::Disconnected);
    bool connectResult = apConnection.Connect("127.0.0.1", 11120);
    EXPECT_TRUE(connectResult);

    EXPECT_TRUE(WaitForConnectionStateToBeEqual(apConnection, SocketConnection::EConnectionState::Connected));
    EXPECT_TRUE(WaitForConnectionStateToBeEqual(apListener, SocketConnection::EConnectionState::Connected));

    // Disconnect B
    bool disconnectResult = apListener.Disconnect();
    EXPECT_TRUE(disconnectResult);

    EXPECT_TRUE(WaitForConnectionStateToBeEqual(apListener, SocketConnection::EConnectionState::Disconnected));

    // disconncting the listener should kick out the other end:
    // note that the listener was the ONLY one we told to disconnect
    // the other end (the apConnection) is likely to be in a retry state - so it wont be connected, but it also won't necessarily
    // be disconnected, connecting, etc.
    EXPECT_TRUE(WaitForConnectionStateToNotBeEqual(apConnection, SocketConnection::EConnectionState::Connected));

    // start listening again
    listenResult = apListener.Listen(11120);
    EXPECT_TRUE(listenResult);

    // once we start listening, the ap connection should autoconnect very shortly:
    EXPECT_TRUE(WaitForConnectionStateToBeEqual(apConnection, SocketConnection::EConnectionState::Connected));

    // at that point, both sides should consider themselves cyonnected (the listener, too)
    EXPECT_TRUE(WaitForConnectionStateToBeEqual(apListener, SocketConnection::EConnectionState::Connected));

    // now disconnect A
    disconnectResult = apConnection.Disconnect();
    EXPECT_TRUE(disconnectResult);

    EXPECT_TRUE(WaitForConnectionStateToBeEqual(apConnection, SocketConnection::EConnectionState::Disconnected));

    // ensure that B rebinds and starts listening again
    EXPECT_TRUE(WaitForConnectionStateToBeEqual(apListener, SocketConnection::EConnectionState::Listening));

    // reconnect manually from A -> B
    connectResult = apConnection.Connect("127.0.0.1", 11120);
    EXPECT_TRUE(connectResult);

    EXPECT_TRUE(WaitForConnectionStateToBeEqual(apConnection, SocketConnection::EConnectionState::Connected));
    EXPECT_TRUE(WaitForConnectionStateToBeEqual(apListener, SocketConnection::EConnectionState::Connected));

    // disconnect everything, starting with B to ensure that reconnect thread exits on disconnect
    disconnectResult = apListener.Disconnect();
    EXPECT_TRUE(disconnectResult);

    EXPECT_TRUE(WaitForConnectionStateToBeEqual(apListener, SocketConnection::EConnectionState::Disconnected));

    // note that the listener was the ONLY one we told to disconnect!
    // the other end (the apConnection) is likely to be in a retry state (ie, one of the states that is not connected)
    EXPECT_TRUE(WaitForConnectionStateToNotBeEqual(apConnection, SocketConnection::EConnectionState::Connected));
    
    // disconnect A
    disconnectResult = apConnection.Disconnect();
    EXPECT_TRUE(disconnectResult);
    EXPECT_TRUE(WaitForConnectionStateToBeEqual(apConnection, SocketConnection::EConnectionState::Disconnected));
}
