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

#include "WebsocketClient_Default.h"

namespace Websockets
{
    void WebsocketClient_Default::Destroy()
    {
        if (m_connection && m_connection->get_state() != websocketpp::session::state::closed)
        {
            m_client.stop();  //hard stop client because it hasn't been properly closed previously
        }
        if (m_thread && m_thread->joinable())
        {
            m_thread->join();
            m_thread.reset();
        }
    }

    bool WebsocketClient_Default::ConnectWebsocket(const AZStd::string & websocket, const OnMessage & messageFunc)
    {
        m_messageFunction = messageFunc;

        //websocket setup
        m_client.set_access_channels(websocketpp::log::alevel::all);
        m_client.clear_access_channels(websocketpp::log::alevel::frame_payload);

        // Initialize ASIO
        m_client.init_asio();

        //use lambda to set handler
        m_client.set_message_handler([this](websocketpp::connection_hdl hdl, websocketpp::config::asio_client::message_type::ptr pMessage)
        {
            const std::string& messageString = pMessage->get_payload();
            m_messageFunction(AZStd::string_view(messageString.c_str(), messageString.size()));
        }
        );

        websocketpp::lib::error_code errorCode;
        m_connection = m_client.get_connection(std::string(websocket.c_str(), websocket.size()), errorCode);

        AZ_Error("Websocket Gem", !(errorCode.value()), "Could not create connection because: %s", errorCode.message().c_str());
        AZ_Error("Websocket Gem", m_connection, "Connection is null");

        if (!m_connection)
        {
            return false;
        }

        // Connect only requests a connection. No network messages are
        // exchanged until run().
        m_client.connect(m_connection);

        //run() is blocking, so we start it up on another thread  
        m_thread = AZStd::make_unique<AZStd::thread>([this]() { m_client.run(); });

        return true;
    }

    bool WebsocketClient_Default::IsConnectionOpen() const
    {
        return m_connection->get_state() == websocketpp::session::state::open;
    }

    void WebsocketClient_Default::SendWebsocketMessage(const AZStd::string & msgBody)
    {
        if (!m_connection)
        {
            AZ_Warning("Websocket Gem", m_connection, "Cannot send message because we are not connected to a websocket.");
            return;
        }

        websocketpp::lib::error_code errorCode;

        AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::AzCore, "Send Websocket Message");

        m_client.send(m_connection->get_handle(), msgBody.c_str(), websocketpp::frame::opcode::text, errorCode);

        AZ_Warning("Websocket Gem", !(errorCode.value()), "Message failed to send because: %s", errorCode.message().c_str());
    }

    void WebsocketClient_Default::SendWebsocketMessageBinary(const void* payload, size_t len)
    {
        if (!m_connection)
        {
            AZ_Warning("Websocket Gem", m_connection, "Cannot send message because we are not connected to a websocket.");
            return;
        }

        websocketpp::lib::error_code errorCode;

        AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::AzCore, "Send Websocket Message");

        m_client.send(m_connection->get_handle(), payload, len, websocketpp::frame::opcode::binary, errorCode);

        AZ_Warning("Websocket Gem", !(errorCode.value()), "Message failed to send because: %s", errorCode.message().c_str());
    }

    void WebsocketClient_Default::CloseWebsocket()
    {
        websocketpp::lib::error_code ec;
        m_client.close(m_connection->get_handle(), websocketpp::close::status::going_away, "Websocket client closing", ec);
    }

}

