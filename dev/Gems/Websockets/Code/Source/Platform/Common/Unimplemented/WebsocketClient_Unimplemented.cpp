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

#include "WebsocketClient_Unimplemented.h"

namespace Websockets
{
    void WebsocketClient_Unimplemented::Destroy()
    {
        
    }

    bool WebsocketClient_Unimplemented::ConnectWebsocket(const AZStd::string & websocket, const OnMessage & messageFunc)
    {
        return false;
    }

    bool WebsocketClient_Unimplemented::IsConnectionOpen() const
    {
        return false;
    }

    void WebsocketClient_Unimplemented::SendWebsocketMessage(const AZStd::string & msgBody)
    {
        
    }

    void WebsocketClient_Unimplemented::SendWebsocketMessageBinary(const void * payload, const size_t len)
    {

    }

    void WebsocketClient_Unimplemented::CloseWebsocket()
    {
        
    }

}

