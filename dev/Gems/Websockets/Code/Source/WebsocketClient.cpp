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

#include <WebsocketClient.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>

namespace Websockets
{
    WebsocketClient::~WebsocketClient()
    {
        Platform::Destroy();
    }

    void WebsocketClient::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<WebsocketClient>()
                ->Version(1)
                ;
        }
    }

    bool WebsocketClient::ConnectWebsocket(const AZStd::string& websocket, const OnMessage& messageFunc)
    {
        return Platform::ConnectWebsocket(websocket, messageFunc);
    }

    bool WebsocketClient::IsConnectionOpen() const
    {
        return Platform::IsConnectionOpen();
    }

    void WebsocketClient::SendWebsocketMessage(const AZStd::string& msgBody)
    {
        Platform::SendWebsocketMessage(msgBody);
    }

    void WebsocketClient::SendWebsocketMessageBinary(const void* payload, const size_t len)
    {
        Platform::SendWebsocketMessageBinary(payload, len);
    }

    void WebsocketClient::CloseWebsocket()
    {
        Platform::CloseWebsocket();
    }
}
