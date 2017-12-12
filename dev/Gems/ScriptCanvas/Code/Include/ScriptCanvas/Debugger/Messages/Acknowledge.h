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

namespace ScriptCanvas
{
    namespace Debugger
    {
        namespace Message
        {
            class Acknowledge
                : public AzFramework::TmMsg
            {
            public:
                AZ_CLASS_ALLOCATOR(Acknowledge, AZ::SystemAllocator, 0);
                AZ_RTTI(Acknowledge, "{2FBEC565-7F5F-435E-8BC6-DD17CC1FABE7}", AzFramework::TmMsg);

                Acknowledge(AZ::u32 request = 0, AZ::u32 ackCode = 0)
                    : AzFramework::TmMsg(AZ_CRC("ScriptCanvasDebugger", 0x9d223ef5))
                    , m_request(request)
                    , m_ackCode(ackCode)
                {}

                AZ::u32 m_request;
                AZ::u32 m_ackCode;
            };
        }
    }
}