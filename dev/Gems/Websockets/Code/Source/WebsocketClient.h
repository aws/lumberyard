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

#pragma once
#include <AzCore/Component/Component.h>
#include <WebsocketClient_Platform.h>

namespace Websockets
{
    
    class WebsocketClient
        : public IWebsocketClient
        , public WebsocketClient_Platform
    {
    private:
        using Platform = WebsocketClient_Platform;

    public:
        AZ_CLASS_ALLOCATOR(WebsocketClient, AZ::SystemAllocator, 0);
        AZ_RTTI(WebsocketClient, "{17711615-3BC1-40BA-BF0E-D238B1810D2D}");

        WebsocketClient() { };
        WebsocketClient(const WebsocketClient& socket) { };  //avoids copying because websocketpp deletes these functions and values will be constructed anyway

        ~WebsocketClient() override;

        WebsocketClient& operator=(const WebsocketClient& socket) { return *this; };  //again, operator= is deleted for clients, etc. in websocketpp

        static void Reflect(AZ::ReflectContext* context);

        bool ConnectWebsocket(const AZStd::string& websocket, const OnMessage& messageFunc);

        //interface functions
        bool IsConnectionOpen() const override;
        void SendWebsocketMessage(const AZStd::string& msgBody) override;
        void SendWebsocketMessageBinary(const void* payload, const size_t len) override;
        void CloseWebsocket() override;
    };
}
