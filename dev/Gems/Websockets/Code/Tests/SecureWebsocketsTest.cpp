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
    enum tls_mode
    {
        intermediate = 1,
        modern = 2
    };

    void OnSecureMessage(WebsocketLibDefs::secureServer* serv, WebsocketLibDefs::hdl hdl, WebsocketLibDefs::message_ptr msg)
    {
        serv->send(hdl, msg->get_payload(), msg->get_opcode());  //echo to client
    }

    std::string GetPassword() {
        return "test";
    }

    WebsocketLibDefs::ContextPtr OnTlsInit(tls_mode mode, websocketpp::connection_hdl hdl)
    {
        namespace asio = websocketpp::lib::asio;

        WebsocketLibDefs::ContextPtr contextPtr = websocketpp::lib::make_shared<asio::ssl::context>(asio::ssl::context::sslv23);

        contextPtr->set_options(asio::ssl::context::default_workarounds |
            asio::ssl::context::no_sslv2 |
            asio::ssl::context::no_sslv3 |
            asio::ssl::context::single_dh_use);

        contextPtr->set_password_callback(bind(&GetPassword));

        //directly from official asio test case - not a specialized key!
        const char pem[] = R"(-----BEGIN PRIVATE KEY-----
MIIEvAIBADANBgkqhkiG9w0BAQEFAASCBKYwggSiAgEAAoIBAQDMYjHlTUeUGSys
Fz5PZcvgS3gojBlEAOu2gXFQDcJ7kq6dJ4jKsPaH1Q5jAtEDkU/el8otzfacOgyp
2ZxioRimpmcORWGU0bKJcenh4ZQ1oK1CQObjeYk1YgE7H8/sBetSdtL4n1rB8LIz
AV/k6kwSZFu3/lSmc6g09H4efSKGKVBcVOjBamcvFGVH4KhM2NyL+ffeV5H2Ucxk
ipyhpr4uxEoC3EV60sQxloqZb+upBM0LS4kVvaYMcn39XkUx3Z8FwN5+hFGwsWda
tU8zDxEuRMrZxG7mwDXLBGehtQvoJIVMQbOuwBQcgAbsVyy1dxV3aczbLX0iGEuG
eBhdFE+BAgMBAAECggEAQaPn0nUXYAEVz89HO8i9ybNzS9Jy6txA18SK1+MTawyY
9/AShsZ+5vEORc5JwpOQyzSEwmE7qsEaABLbnvGOMTeQMY0m4dzXMj1bmCgSqYaJ
HpYpkTUfU/2913dIF81u3nU7HI5RX6gmEyuF2MdG10FUE6ujFDJg+2DqgHA//kYD
hkXFinVS2PuZs8d5xdzpF0aCIWTuOc+Fgsyhdm/lZRIzFdID45YUVuPIN2uh+GkM
ENp/r1x7dPlDRqiL1ufP0mTQGs26S5kQSF8W0BClkOIOgmrhSON4+Vqhqx+ki/7w
RY+7mmgdvt0uzYT+Lk2cDw4f89Rsh7rR1EieBpQ2YQKBgQDq6zAHWfweJmkugT0w
HzI0UKfcOdzlJBwMu6tSgSHU99dnXlTwQY8sG7vtfRekokoo7XY4JsSk1n6E9OVy
4UKuEvU1+llDGxtvHxEEGOAgwB8wxMuY4uNYgDVhTlUzr2ERcet7FOIGzxEWzSsg
5vgnTQfyMzAh5/6k8CsHVI4u2wKBgQDeuYVCgg555lcc5rvTFxfU15d3fweSd78+
akgIBaXAlFbxI+5znGPmKG/ii4N2XObC8B568fA2nIxw6M1xgbKyvvmN3ECYiqWv
bx8x6Vg5Slg0vJr+DrPgvIKbOWEEKF/cfpTeeVLP0gUBT63mA3qezuRx1r0JJr7A
k9a4Td9j0wKBgDmRQMfMaVgKGaRnz1LHkkn3qerx0wvj+Wu1YZpqQpwp0ANovm/R
4P/yG+9qxCx4CKxW5K2F8pJibcavLLsmMGzwAF8l5lHnhqWIe2cBoYrlCb+tuibR
Et1RLcOWqpJr2+GmhQo4Z9s7SvjHdlYtw4n9+oCDwrvMWj6ZDDJTqjQZAoGAEhRt
RODZ2/texvHT/Wa6gISfvwuIydL+q0plXoFW2zMve5O3H5tqYJyXuIQqv8j60og7
cS+CmGxM2j2Lr9MfdnMaPvHKLJfUq1ER7zNJ/hyS3HUS/9yhrXSgBYm63mOIpJWB
8C1ZE5Ww4lJdg3Z01b9lu/f6kGucwHU/0OZBZBECgYAQ+dl2kKKd+lQ9O/LVz7oD
goQMPYF+QZcEhY4vlYKkWVtR2A0CiY6XeTi6vO/qVUt/ht+UO3XIJFOjGV1VyORQ
Bhibfstxl5s59jGlns5y5QqcRKzCiX74BKG0xQUtHgga7Od6L+GJKbJAPBfncYwW
U7Tfwwi0WbbgQoy5Xr/5gg==
-----END PRIVATE KEY-----
-----BEGIN CERTIFICATE-----
MIIFBTCCAu2gAwIBAgICEAEwDQYJKoZIhvcNAQELBQAwgYIxCzAJBgNVBAYTAlVT
MQswCQYDVQQIDAJJTDEUMBIGA1UECgwLV2ViU29ja2V0KysxKjAoBgNVBAsMIVdl
YlNvY2tldCsrIENlcnRpZmljYXRlIEF1dGhvcml0eTEkMCIGA1UEAwwbV2ViU29j
a2V0KysgSW50ZXJtZWRpYXRlIENBMB4XDTE2MDYwODEyNDUxMloXDTI2MDYwNjEy
NDUxMlowfjELMAkGA1UEBhMCVVMxCzAJBgNVBAgMAklMMRAwDgYDVQQHDAdDaGlj
YWdvMRQwEgYDVQQKDAtXZWJTb2NrZXQrKzEgMB4GA1UECwwXV2ViU29ja2V0Kysg
VExTIEV4YW1wbGUxGDAWBgNVBAMMD3dlYnNvY2tldHBwLm9yZzCCASIwDQYJKoZI
hvcNAQEBBQADggEPADCCAQoCggEBAMxiMeVNR5QZLKwXPk9ly+BLeCiMGUQA67aB
cVANwnuSrp0niMqw9ofVDmMC0QORT96Xyi3N9pw6DKnZnGKhGKamZw5FYZTRsolx
6eHhlDWgrUJA5uN5iTViATsfz+wF61J20vifWsHwsjMBX+TqTBJkW7f+VKZzqDT0
fh59IoYpUFxU6MFqZy8UZUfgqEzY3Iv5995XkfZRzGSKnKGmvi7ESgLcRXrSxDGW
iplv66kEzQtLiRW9pgxyff1eRTHdnwXA3n6EUbCxZ1q1TzMPES5EytnEbubANcsE
Z6G1C+gkhUxBs67AFByABuxXLLV3FXdpzNstfSIYS4Z4GF0UT4ECAwEAAaOBhzCB
hDALBgNVHQ8EBAMCBDAwEwYDVR0lBAwwCgYIKwYBBQUHAwEwYAYDVR0RBFkwV4IP
d2Vic29ja2V0cHAub3JnghN3d3cud2Vic29ja2V0cHAub3Jnghl1dGlsaXRpZXMu
d2Vic29ja2V0cHAub3JnghRkb2NzLndlYnNvY2tldHBwLm9yZzANBgkqhkiG9w0B
AQsFAAOCAgEAelJvIWFikBU3HVoP0icuoezTHGqABPLCeooTC/GELq7lHCFEjiqW
p96Zc3vrk+0Z0tkYy3E0fpuzPtlTUhBzO3fMF41FpB5ix3W/tH9YJvrozlIuDD1I
IEusxomeeiMRbyYpX/gkSOO74ylCzMEQVzleMNdpzpeXOg0Kp5z2JNShdEoT7eMR
qkJQJjMdL6QeXUqWNvX1Zqb8v6VeWGWjuu/cl374P8D8bjn89VwZQ5HFqoLOhI5v
XEYsMViZWwLSMcfWTU2Rdi0RxUZQVciLP/3GQROR1/0/e1J1kd7GsRWQMZcU20Vy
jXBVAiWhW1bgd0XOrrFILsAmnBtinEJiE+h5UC4ksZtwWf9x1IhXGlpb9bmD4+Ud
93wmqytPXBFL6wwlj4IYjjy0gU6xP6h7nwhHXnBlwFWGDpe8Cco9qgyJxJxBTtj9
MbBv+BSLXJoniDASdk6RIqCjPWZtWbQ7j5mIKV0bdJQZpBX553QOy8AoIpJE32An
FzR0SSCHOCgSAbqtM8CvLO6mquEJunmwKQx6xfos5N6ee+D+JtUFTw04TrjZUzFs
Z7v3SN/N4Hd13iTBDSu4XY/tJYICvTRLYNrzQRh/XEVbEEVxXhL8rxNn5aL1pqrV
yEnvHXrnSXWxTif1K+hS2HfTkQ6d1GjglvmwkoBqBHuRH0OJ1VguTqM=
-----END CERTIFICATE-----)";

        asio::error_code ec;
        contextPtr->use_certificate_chain(asio::buffer(pem), ec);
        contextPtr->use_private_key(asio::buffer(pem), asio::ssl::context::pem);

        const char dh[] = R"(-----BEGIN DH PARAMETERS-----
MIIBCAKCAQEAmFgi7m4IsMY6mo17EZUDT/twxsKS0Fy1uFfOXE+wYUn7LHRn+0cG
krEMXRPY2DbdRJTcTZoboSe13miAeydTVxemZGI3DKMuouHOJ5NMbPBJLoLXSARZ
oLHKr/dW9w5axK31Ux3VgFILZ6BvLu3HTVxnqXrDZ0km+XET8MnOxpp2hHrMD9CV
uVawq9fKMuOD2TmAZN46JBFJ6gt3BkycTXJMcuPqOTc3G7udAaH3VEymLcRh8QdJ
ISsYk3oB3jO3SKwpew5N6ThWmR066CAo4ncJ715lEqmt1u/554pnpi//+loKBJUp
xNMJuWF60+7OucdEjOs7Fb2aiNSA2rr9SwIBAg==
-----END DH PARAMETERS-----)";

        contextPtr->use_tmp_dh(asio::buffer(dh), ec);

        std::string ciphers = "ECDHE-RSA-AES128-GCM-SHA256:ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE\
-RSA-AES256-GCM-SHA384:ECDHE-ECDSA-AES256-GCM-SHA384:DHE-RSA-AES128-GCM-SHA256:DHE-DSS-AES128-GCM\
-SHA256:kEDH+AESGCM:ECDHE-RSA-AES128-SHA256:ECDHE-ECDSA-AES128-SHA256:ECDHE-RSA-AES128-SHA:ECDHE\
-ECDSA-AES128-SHA:ECDHE-RSA-AES256-SHA384:ECDHE-ECDSA-AES256-SHA384:ECDHE-RSA-AES256-SHA:ECDHE\
-ECDSA-AES256-SHA:DHE-RSA-AES128-SHA256:DHE-RSA-AES128-SHA:DHE-DSS-AES128-SHA256:DHE-RSA-AES256\
-SHA256:DHE-DSS-AES256-SHA:DHE-RSA-AES256-SHA:AES128-GCM-SHA256:AES256-GCM-SHA384:AES128-SHA256\
:AES256-SHA256:AES128-SHA:AES256-SHA:AES:CAMELLIA:DES-CBC3-SHA:!aNULL:!eNULL:!EXPORT:!DES:!RC4:\
!MD5:!PSK:!aECDH:!EDH-DSS-DES-CBC3-SHA:!EDH-RSA-DES-CBC3-SHA:!KRB5-DES-CBC3-SHA";

        if (SSL_CTX_set_cipher_list(contextPtr->native_handle(), ciphers.c_str()) != 1)
        {
            EXPECT_TRUE(false);
        }

        return contextPtr;
    }
}

class SecureWebsocketsTest
    : public UnitTest::AllocatorsTestFixture
{
public:

    SecureWebsocketsTest() {};
    virtual ~SecureWebsocketsTest() { };

    //client functions
    AZStd::unique_ptr<Websockets::IWebsocketClient> CreateClient(const AZStd::string_view& websocket, Websockets::OnMessage messageFunc)
    {
        AZStd::unique_ptr<Websockets::SecureWebsocketClient> socket = AZStd::make_unique<Websockets::SecureWebsocketClient>();

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
        m_echoServer.set_message_handler(WebsocketLibDefs::bind(&WebsocketsTesting::OnSecureMessage, &m_echoServer, WebsocketLibDefs::_1, WebsocketLibDefs::_2));
        m_echoServer.set_tls_init_handler(bind(&WebsocketsTesting::OnTlsInit, WebsocketsTesting::tls_mode::modern, WebsocketLibDefs::_1));
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

    WebsocketLibDefs::secureServer m_echoServer;
    AZStd::unique_ptr<AZStd::thread> m_thread;

    //timeout functionality
    AZStd::chrono::time_point<> m_time;
    AZStd::chrono::microseconds m_timeOut;
};

TEST_F(SecureWebsocketsTest, WebsocketCreateSecureClientTest_FT)
{
    SecureWebsocketsTest test;
    Websockets::OnMessage messageFunc = [](const AZStd::string_view message) {};
    AZStd::string testWebsocket = AZStd::string(WebsocketsTesting::secureSocket) + AZStd::string(WebsocketsTesting::testPort);
    AZStd::unique_ptr<Websockets::IWebsocketClient> testClient = test.CreateClient(testWebsocket, messageFunc);

    EXPECT_NE(testClient.get(), nullptr);
    testClient->CloseWebsocket();
}

TEST_F(SecureWebsocketsTest, SecureWebsocketFailToCloseTest_FT)
{
    SecureWebsocketsTest test;
    test.SetTime();

    Websockets::OnMessage messageFunc = [](const AZStd::string_view message)
    {
    };

    AZStd::string testWebsocket = AZStd::string(WebsocketsTesting::secureSocket) + AZStd::string(WebsocketsTesting::testPort);
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

TEST_F(SecureWebsocketsTest, SecureWebsocketSendMessageTest_FT)
{
    SecureWebsocketsTest test;
    test.SetTime();
    EXPECT_FALSE(WebsocketsTesting::proceed);

    Websockets::OnMessage messageFunc = [](const AZStd::string_view message)
    {
        EXPECT_EQ(message, WebsocketsTesting::testMessage);
        WebsocketsTesting::proceed = true;
    };

    AZStd::string testWebsocket = AZStd::string(WebsocketsTesting::secureSocket) + AZStd::string(WebsocketsTesting::testPort);
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

TEST_F(SecureWebsocketsTest, SecureWebsocketSendMessageBinaryTest_FT)
{
    SecureWebsocketsTest test;
    test.SetTime();
    EXPECT_FALSE(WebsocketsTesting::proceed);

    Websockets::OnMessage messageFunc = [](const AZStd::string_view message)
    {
        EXPECT_EQ(message, WebsocketsTesting::testMessage);
        WebsocketsTesting::proceed = true;
    };

    AZStd::string testWebsocket = AZStd::string(WebsocketsTesting::secureSocket) + AZStd::string(WebsocketsTesting::testPort);
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

TEST_F(SecureWebsocketsTest, SecureWebsocketBadAddressTest_FT)
{
    SecureWebsocketsTest test;
    Websockets::OnMessage messageFunc = [](const AZStd::string_view message)
    {
    };

    constexpr const char* badSocket = "wss://lcalhost:";
    AZStd::string testWebsocket = AZStd::string(badSocket) + AZStd::string(WebsocketsTesting::testPort);

    AZStd::unique_ptr<Websockets::IWebsocketClient> testClient = test.CreateClient(testWebsocket, messageFunc);

    EXPECT_NE(testClient.get(), nullptr);

    EXPECT_FALSE(testClient->IsConnectionOpen());
}

TEST_F(SecureWebsocketsTest, SecureWebsocketBadPortTest_FT)
{
    SecureWebsocketsTest test;
    Websockets::OnMessage messageFunc = [](const AZStd::string_view message)
    {
    };

    constexpr const char* badPort = "10095";
    AZStd::string testWebsocket = AZStd::string(WebsocketsTesting::secureSocket) + AZStd::string(badPort);

    AZStd::unique_ptr<Websockets::IWebsocketClient> testClient = test.CreateClient(testWebsocket, messageFunc);

    EXPECT_NE(testClient.get(), nullptr);

    EXPECT_FALSE(testClient->IsConnectionOpen());
}

TEST_F(SecureWebsocketsTest, SecureWebsocketNoAddressTest_FT)
{
    SecureWebsocketsTest test;
    Websockets::OnMessage messageFunc = [](const AZStd::string_view message)
    {
    };

    AZStd::string testWebsocket = "";
    AZ_TEST_START_TRACE_SUPPRESSION;
    AZStd::unique_ptr<Websockets::IWebsocketClient> testClient = test.CreateClient(testWebsocket, messageFunc);
    AZ_TEST_STOP_TRACE_SUPPRESSION(2);
    EXPECT_EQ(testClient.get(), nullptr);
}

TEST_F(SecureWebsocketsTest, SecureWebsocketSendSpecialCharsTest_FT)
{
    SecureWebsocketsTest test;
    test.SetTime();
    EXPECT_FALSE(WebsocketsTesting::proceed);

    Websockets::OnMessage messageFunc = [](const AZStd::string_view message)
    {
        WebsocketsTesting::proceed = true;
    };

    AZStd::string testWebsocket = AZStd::string(WebsocketsTesting::secureSocket) + AZStd::string(WebsocketsTesting::testPort);
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

TEST_F(SecureWebsocketsTest, SecureWebsocketSendLargeMessageTest_FT)
{
    SecureWebsocketsTest test;
    test.SetTime();
    EXPECT_FALSE(WebsocketsTesting::proceed);

    Websockets::OnMessage messageFunc = [](const AZStd::string_view message)
    {
        WebsocketsTesting::proceed = true;
    };

    AZStd::string testWebsocket = AZStd::string(WebsocketsTesting::secureSocket) + AZStd::string(WebsocketsTesting::testPort);
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

AZ_UNIT_TEST_HOOK();
