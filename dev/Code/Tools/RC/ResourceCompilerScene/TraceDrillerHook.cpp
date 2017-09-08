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

#include <AzToolsFramework/Debug/TraceContextLogFormatter.h>
#include <TraceDrillerHook.h>

namespace AZ
{
    namespace RC
    {
        TraceDrillerHook::TraceDrillerHook()
        {
            BusConnect();
        }
        
        TraceDrillerHook::~TraceDrillerHook()
        {
            BusDisconnect();
        }

        AZ::Debug::Result TraceDrillerHook::OnPrintf(const AZ::Debug::TraceMessageParameters& parameters)
        {
            AZStd::shared_ptr<const AzToolsFramework::Debug::TraceContextStack> stack = m_stacks.GetCurrentStack();
            if (stack)
            {
                AZStd::string line;
                size_t stackSize = stack->GetStackCount();
                for (size_t i = 0; i < stackSize; ++i)
                {
                    line.clear();
                    if (stack->GetType(i) == AzToolsFramework::Debug::TraceContextStackInterface::ContentType::UuidType)
                    {
                        continue;
                    }

                    AzToolsFramework::Debug::TraceContextLogFormatter::PrintLine(line, *stack, i);
                    RCLogContext(line.c_str());
                }
            }
            
            if (AzFramework::StringFunc::Equal(parameters.window, SceneAPI::Utilities::ErrorWindow))
            {
                RCLogError("%.*s", CalculateLineLength(parameters.message), parameters.message);
            }
            else if (AzFramework::StringFunc::Equal(parameters.window, SceneAPI::Utilities::WarningWindow))
            {
                RCLogWarning("%.*s", CalculateLineLength(parameters.message), parameters.message);
            }
            else
            {
                RCLog("%.*s", CalculateLineLength(parameters.message), parameters.message);
            }
            return AZ::Debug::Result::Handled;
        }

        size_t TraceDrillerHook::CalculateLineLength(const char* message) const
        {
            size_t length = strlen(message);
            while ((message[length - 1] == '\n' || message[length - 1] == '\r' ) && length > 1)
            {
                length--;
            }
            return length;
        }
    } // RC
} // AZ