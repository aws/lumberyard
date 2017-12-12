#pragma once

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

#include <cstdint>
#include <IRCLog.h>
#include <AzCore/Debug/TraceMessageBus.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/Debug/TraceContextMultiStackHandler.h>

namespace AZ
{
    namespace RC
    {
        class TraceDrillerHook
            : public AZ::Debug::TraceMessageBus::Handler
        {
        public:
            TraceDrillerHook();
            ~TraceDrillerHook() override;

            bool OnPrintf(const char* window, const char* message) override;

        private:
            size_t CalculateLineLength(const char* message) const;

            AzToolsFramework::Debug::TraceContextMultiStackHandler m_stacks;
        };
    } // RC
} // AZ