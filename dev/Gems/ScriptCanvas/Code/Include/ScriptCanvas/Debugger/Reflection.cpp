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

#include "precompiled.h"

#include <Debugger/Messages/Acknowledge.h>
#include <Debugger/Messages/Request.h>

namespace ScriptCanvas
{
    namespace Debugger
    {
        void ReflectMessages(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (serializeContext)
            {
                // Messages
                serializeContext->Class<Message::Acknowledge, AzFramework::TmMsg>()
                    ->Field("request", &Message::Acknowledge::m_request)
                    ->Field("ackCode", &Message::Acknowledge::m_ackCode);

                serializeContext->Class<Message::Request, AzFramework::TmMsg>()
                    ->Field("m_request", &Message::Request::m_request)
                    ->Field("m_graphId", &Message::Request::m_graphId);

                serializeContext->Class<Message::NodeState, Message::Request>()
                    ->Field("m_node", &Message::NodeState::m_node);
            }

        }
    }
}
