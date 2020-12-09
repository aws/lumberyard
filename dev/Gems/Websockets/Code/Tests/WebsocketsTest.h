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
#include <AzTest/AzTest.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <WebsocketsSystemComponent.h>

 //websocketpp functionality
#define BOOST_ALL_NO_LIB
#define ASIO_STANDALONE
#define _WEBSOCKETPP_CPP11_RANDOM_DEVICE_
#define _WEBSOCKETPP_CPP11_TYPE_TRAITS_
AZ_PUSH_DISABLE_WARNING(4267 4996, "-Wunknown-warning-option")
#include <websocketpp/server.hpp>
#include <websocketpp/config/asio.hpp>
AZ_POP_DISABLE_WARNING
#undef _WEBSOCKETPP_CPP11_TYPE_TRAITS_
#undef _WEBSOCKETPP_CPP11_RANDOM_DEVICE_
#undef ASIO_STANDALONE
#undef BOOST_ALL_NO_LIB

namespace WebsocketLibDefs
{
    //general server
    typedef websocketpp::server<websocketpp::config::asio> server;
    typedef server::message_ptr message_ptr;
    typedef websocketpp::connection_hdl hdl;

    //secure server
    typedef websocketpp::server<websocketpp::config::asio_tls> secureServer;
    typedef websocketpp::lib::shared_ptr<websocketpp::lib::asio::ssl::context> ContextPtr;

    using websocketpp::lib::bind;
    using websocketpp::lib::placeholders::_1;
    using websocketpp::lib::placeholders::_2;
}

//websocketpp
namespace WebsocketsTesting
{
    static bool proceed = false;

    constexpr const char* localSocket = "ws://localhost:";
    constexpr const char* secureSocket = "wss://localhost:";
    constexpr const char* testPort = "10090";
    constexpr const char* testMessage = "TestMessage";

    const int testMessageSize = 11;

    static const AZStd::chrono::milliseconds milliSecondDelay(1000);
}
