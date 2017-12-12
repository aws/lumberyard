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

#include <AzFramework/TargetManagement/TargetManagementAPI.h>
#include <ScriptCanvas/Core/Node.h>

namespace ScriptCanvas
{
    namespace Debugger
    {
        namespace Message
        {
            class Request
                : public AzFramework::TmMsg
            {
            public:
                AZ_CLASS_ALLOCATOR(Request, AZ::SystemAllocator, 0);
                AZ_RTTI(Request, "{0283335F-E3FF-4292-99BA-36A289DFED87}", AzFramework::TmMsg);

                Request(AZ::u32 request = 0, const AZ::EntityId& graphId = AZ::EntityId())
                    : AzFramework::TmMsg(AZ_CRC("ScriptCanvasDebugRequest", 0xa4c45706))
                    , m_request(request)
                    , m_graphId(graphId)
                {}

                AZ::u32 m_request;
                AZ::EntityId m_graphId;
            };

            class NodeState
                : public Request
            {
            public:
                AZ_CLASS_ALLOCATOR(NodeState, AZ::SystemAllocator, 0);
                AZ_RTTI(NodeState, "{6F06FB58-C608-4126-8F81-D303BAB6C02D}", Request);

                NodeState(ScriptCanvas::Node* node = nullptr, const AZ::EntityId& graphId = AZ::EntityId())
                    : Request(AZ_CRC("ScriptCanvas_OnEntry", 0x07ee76d4), graphId)
                    , m_node(node)
                {}

                ScriptCanvas::Node* m_node;
            };

        }


    }
}