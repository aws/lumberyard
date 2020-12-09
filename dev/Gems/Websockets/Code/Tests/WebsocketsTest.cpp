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

#include <Tests/WebsocketsTest.h>

namespace WebsocketsTesting
{
    void OnMessage(WebsocketLibDefs::server* serv, WebsocketLibDefs::hdl hdl, WebsocketLibDefs::message_ptr msg)
    {
        serv->send(hdl, msg->get_payload(), msg->get_opcode());  //echo to client
    }
}


class WebsocketsTest
    : public UnitTest::AllocatorsTestFixture
{
public:

    WebsocketsTest() {};
    virtual ~WebsocketsTest() { };

    //client functions
    AZStd::unique_ptr<Websockets::IWebsocketClient> CreateClient(const AZStd::string_view& websocket, Websockets::OnMessage messageFunc)
    {
        AZStd::unique_ptr<Websockets::WebsocketClient> socket = AZStd::make_unique<Websockets::WebsocketClient>();

        if (!socket)
        {
            AZ_Error("Websocket Gem", socket, "Socket not created!");
            return nullptr;
        }

        if (!socket->ConnectWebsocket(websocket, messageFunc))
        {
            return nullptr;
        }

        return socket;
    }

    //server functions
    void CreateEchoServer()
    {
        // Set logging settings
        m_echoServer.set_access_channels(websocketpp::log::alevel::all);
        m_echoServer.clear_access_channels(websocketpp::log::alevel::frame_payload);

        // Initialize Asio
        m_echoServer.init_asio();
        
        // Register our message handler
        m_echoServer.set_message_handler(WebsocketLibDefs::bind(&WebsocketsTesting::OnMessage, &m_echoServer, WebsocketLibDefs::_1, WebsocketLibDefs::_2));
        // Listen on port
        int port = strtoul(WebsocketsTesting::testPort, nullptr, 0);
        m_echoServer.listen(port);

        // Start the server accept loop
        m_echoServer.start_accept();

        // Start the ASIO io_service run loop in another thread
        m_thread = AZStd::make_unique<AZStd::thread>([this]() { m_echoServer.run(); });
    }

    void DestroyEchoServer()
    {
        m_echoServer.stop_listening();

        websocketpp::connection_hdl hdl;
        websocketpp::lib::error_code ec;
        m_echoServer.close(hdl, websocketpp::close::status::going_away, "Websocket server closing", ec);

        if (m_thread && m_thread->joinable())
        {
            m_thread->join();
            m_thread.reset();
        }
    }

    //timeout functions
    void SetTime()
    {
        m_timeOut = AZStd::chrono::seconds(2);
        m_time = AZStd::chrono::high_resolution_clock::now();
    }

    bool CheckTime()
    {
        AZStd::chrono::time_point<> time = AZStd::chrono::high_resolution_clock::now();
        if ((time - m_time) > m_timeOut)
        {
            return true;
        }

        return false;
    }

protected:

    void TestBody() override {}

    void SetUp() override
    {
        UnitTest::AllocatorsTestFixture::SetUp();
        CreateEchoServer();
        WebsocketsTesting::proceed = false;
    }

    void TearDown() override
    {
        DestroyEchoServer();
        UnitTest::AllocatorsTestFixture::TearDown();
    }

    WebsocketLibDefs::server m_echoServer;
    AZStd::unique_ptr<AZStd::thread> m_thread;

    //timeout functionality
    AZStd::chrono::time_point<> m_time;
    AZStd::chrono::microseconds m_timeOut;
};

TEST_F(WebsocketsTest, WebsocketCreateClientTest_FT)
{
    WebsocketsTest test;
    Websockets::OnMessage messageFunc = [](const AZStd::string_view message) {};
    AZStd::string testWebsocket = AZStd::string(WebsocketsTesting::localSocket) + AZStd::string(WebsocketsTesting::testPort);
    AZStd::unique_ptr<Websockets::IWebsocketClient> testClient = test.CreateClient(testWebsocket, messageFunc);

    EXPECT_NE(testClient.get(), nullptr);
    testClient->CloseWebsocket();
}

TEST_F(WebsocketsTest, WebsocketFailToCloseTest_FT)
{
    WebsocketsTest test;
    test.SetTime();

    Websockets::OnMessage messageFunc = [](const AZStd::string_view message)
    {
    };

    AZStd::string testWebsocket = AZStd::string(WebsocketsTesting::localSocket) + AZStd::string(WebsocketsTesting::testPort);
    AZStd::unique_ptr<Websockets::IWebsocketClient> testClient = test.CreateClient(testWebsocket, messageFunc);

    EXPECT_NE(testClient.get(), nullptr);

    //wait for thread to open connection - allow for timeout if threads fail at any point
    while (!testClient->IsConnectionOpen()
        && !test.CheckTime())
    {
        AZStd::this_thread::sleep_for(WebsocketsTesting::milliSecondDelay);
    }

    ASSERT_TRUE(testClient->IsConnectionOpen());
}

TEST_F(WebsocketsTest, WebsocketSendMessageTest_FT)
{
    WebsocketsTest test;
    test.SetTime();
    EXPECT_FALSE(WebsocketsTesting::proceed);

    Websockets::OnMessage messageFunc = [](const AZStd::string_view message)
    {
        EXPECT_EQ(message, WebsocketsTesting::testMessage);
        WebsocketsTesting::proceed = true;
    };

    AZStd::string testWebsocket = AZStd::string(WebsocketsTesting::localSocket) + AZStd::string(WebsocketsTesting::testPort);
    AZStd::unique_ptr<Websockets::IWebsocketClient> testClient = test.CreateClient(testWebsocket, messageFunc);

    EXPECT_NE(testClient.get(), nullptr);

    //wait for thread to open connection - allow for timeout if threads fail at any point
    while (!testClient->IsConnectionOpen()
        && !test.CheckTime())
    {
        AZStd::this_thread::sleep_for(WebsocketsTesting::milliSecondDelay);
    }

    ASSERT_TRUE(testClient->IsConnectionOpen());
    testClient->SendWebsocketMessage(WebsocketsTesting::testMessage);

    //wait for message to send and be received via threads
    while (!WebsocketsTesting::proceed
        && !test.CheckTime())
    {
        AZStd::this_thread::sleep_for(WebsocketsTesting::milliSecondDelay);
    }

    //verify whether test passed or failed
    EXPECT_TRUE(WebsocketsTesting::proceed);

    testClient->CloseWebsocket();
}

TEST_F(WebsocketsTest, WebsocketSendBinaryMessageTest_FT)
{
    WebsocketsTest test;
    test.SetTime();
    EXPECT_FALSE(WebsocketsTesting::proceed);

    Websockets::OnMessage messageFunc = [](const AZStd::string_view message)
    {
        EXPECT_EQ(message, WebsocketsTesting::testMessage);
        WebsocketsTesting::proceed = true;
    };

    AZStd::string testWebsocket = AZStd::string(WebsocketsTesting::localSocket) + AZStd::string(WebsocketsTesting::testPort);
    AZStd::unique_ptr<Websockets::IWebsocketClient> testClient = test.CreateClient(testWebsocket, messageFunc);

    EXPECT_NE(testClient.get(), nullptr);

    //wait for thread to open connection - allow for timeout if threads fail at any point
    while (!testClient->IsConnectionOpen()
        && !test.CheckTime())
    {
        AZStd::this_thread::sleep_for(WebsocketsTesting::milliSecondDelay);
    }

    ASSERT_TRUE(testClient->IsConnectionOpen());
    testClient->SendWebsocketMessageBinary(WebsocketsTesting::testMessage, WebsocketsTesting::testMessageSize);

    //wait for message to send and be received via threads
    while (!WebsocketsTesting::proceed
        && !test.CheckTime())
    {
        AZStd::this_thread::sleep_for(WebsocketsTesting::milliSecondDelay);
    }

    //verify whether test passed or failed
    EXPECT_TRUE(WebsocketsTesting::proceed);

    testClient->CloseWebsocket();
}


TEST_F(WebsocketsTest, WebsocketBadAddressTest_FT)
{
    WebsocketsTest test;
    Websockets::OnMessage messageFunc = [](const AZStd::string_view message)
    {
    };

    constexpr const char* badSocket = "ws://lcalhost:";
    AZStd::string testWebsocket = AZStd::string(badSocket) + AZStd::string(WebsocketsTesting::testPort);

    AZStd::unique_ptr<Websockets::IWebsocketClient> testClient = test.CreateClient(testWebsocket, messageFunc);

    EXPECT_NE(testClient.get(), nullptr);

    EXPECT_FALSE(testClient->IsConnectionOpen());
}

TEST_F(WebsocketsTest, WebsocketBadPortTest_FT)
{
    WebsocketsTest test;
    Websockets::OnMessage messageFunc = [](const AZStd::string_view message)
    {
    };

    constexpr const char* badPort = "10095";
    AZStd::string testWebsocket = AZStd::string(WebsocketsTesting::localSocket) + AZStd::string(badPort);

    AZStd::unique_ptr<Websockets::IWebsocketClient> testClient = test.CreateClient(testWebsocket, messageFunc);

    EXPECT_NE(testClient.get(), nullptr);

    EXPECT_FALSE(testClient->IsConnectionOpen());
}

TEST_F(WebsocketsTest, WebsocketNoAddressTest_FT)
{
    WebsocketsTest test;
    Websockets::OnMessage messageFunc = [](const AZStd::string_view message)
    {
    };

    AZStd::string testWebsocket = "";
    AZ_TEST_START_TRACE_SUPPRESSION;
    AZStd::unique_ptr<Websockets::IWebsocketClient> testClient = test.CreateClient(testWebsocket, messageFunc);
    AZ_TEST_STOP_TRACE_SUPPRESSION(2);
    EXPECT_EQ(testClient.get(), nullptr);
}

TEST_F(WebsocketsTest, WebsocketSendSpecialCharsTest_FT)
{
    WebsocketsTest test;
    test.SetTime();
    EXPECT_FALSE(WebsocketsTesting::proceed);

    Websockets::OnMessage messageFunc = [](const AZStd::string_view message)
    {
        WebsocketsTesting::proceed = true;
    };

    AZStd::string testWebsocket = AZStd::string(WebsocketsTesting::localSocket) + AZStd::string(WebsocketsTesting::testPort);
    AZStd::unique_ptr<Websockets::IWebsocketClient> testClient = test.CreateClient(testWebsocket, messageFunc);

    EXPECT_NE(testClient.get(), nullptr);

    //wait for thread to open connection - allow for timeout if threads fail at any point
    while (!testClient->IsConnectionOpen()
        && !test.CheckTime())
    {
        AZStd::this_thread::sleep_for(WebsocketsTesting::milliSecondDelay);
    }

    ASSERT_TRUE(testClient->IsConnectionOpen());

    //send special chars
    constexpr const char* specialChars = "# $ % &' ( ) * + , - ぁあぃいぅうぇえぉおかがきぎくぐけげこごさざしじす";
    testClient->SendWebsocketMessage(specialChars);

    //wait for message to send and be received via threads
    while (!WebsocketsTesting::proceed
        && !test.CheckTime())
    {
        AZStd::this_thread::sleep_for(WebsocketsTesting::milliSecondDelay);
    }

    //verify whether test passed or failed
    EXPECT_TRUE(WebsocketsTesting::proceed);

    testClient->CloseWebsocket();
}

TEST_F(WebsocketsTest, WebsocketSendLargeMessageTest_FT)
{
    WebsocketsTest test;
    test.SetTime();
    EXPECT_FALSE(WebsocketsTesting::proceed);

    Websockets::OnMessage messageFunc = [](const AZStd::string_view message)
    {
        WebsocketsTesting::proceed = true;
    };

    AZStd::string testWebsocket = AZStd::string(WebsocketsTesting::localSocket) + AZStd::string(WebsocketsTesting::testPort);
    AZStd::unique_ptr<Websockets::IWebsocketClient> testClient = test.CreateClient(testWebsocket, messageFunc);

    EXPECT_NE(testClient.get(), nullptr);

    //wait for thread to open connection - allow for timeout if threads fail at any point
    while (!testClient->IsConnectionOpen()
        && !test.CheckTime())
    {
        AZStd::this_thread::sleep_for(WebsocketsTesting::milliSecondDelay);
    }

    ASSERT_TRUE(testClient->IsConnectionOpen());

    const int stringSize = 1024;
    AZStd::string largeMessage(stringSize, 'a');

    testClient->SendWebsocketMessage(largeMessage);

    //wait for message to send and be received via threads
    while (!WebsocketsTesting::proceed
        && !test.CheckTime())
    {
        AZStd::this_thread::sleep_for(WebsocketsTesting::milliSecondDelay);
    }

    //verify whether test passed or failed
    EXPECT_TRUE(WebsocketsTesting::proceed);

    testClient->CloseWebsocket();
}
