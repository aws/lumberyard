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

#include <ScriptCanvas/Core/Graph.h>
#include <ScriptCanvas/Core/Node.h>
#include <AzCore/RTTI/BehaviorContext.h>

namespace ScriptCanvas
{
    struct BehaviorContextMethodHelper
    {
        enum BehaviorContextInputOutput : size_t
        {
            MaxCount = 40,
        };

        AZ_INLINE static bool Call(Node& node, AZ::BehaviorMethod* method, AZ::BehaviorValueParameter* paramBegin, AZ::BehaviorValueParameter* paramEnd, SlotId resultSlotId)
        {
            const auto numExpectedArgs(static_cast<unsigned int>(method->GetNumArguments()));
            if ((paramEnd - paramBegin) == numExpectedArgs)
            {
                AZ::Outcome<bool, AZStd::string> withResultsOutcome = AttemptCallWithResults(node, method, paramBegin, numExpectedArgs, resultSlotId);
                if (!withResultsOutcome.IsSuccess())
                {
                    SCRIPTCANVAS_REPORT_ERROR((node), "Script Canvas attempt to call %s with a result failed", method->m_name.data());
                    return false;
                }
                else if (!withResultsOutcome.GetValue() && !method->Call(paramBegin, numExpectedArgs))
                {
                    SCRIPTCANVAS_REPORT_ERROR((node), "Script Canvas attempt to call %s failed", method->m_name.data());
                    return false;
                }
            }
            else
            {
                SCRIPTCANVAS_REPORT_ERROR((node), "Script Canvas attempt to call %s failed, it expects %d args but called with %d", method->m_name.c_str(), numExpectedArgs, paramEnd - paramBegin);
                return false;
            }

            return true;
        }

        AZ_INLINE static AZ::Outcome<bool, AZStd::string> AttemptCallWithResults(Node& node, AZ::BehaviorMethod* method, AZ::BehaviorValueParameter* params, unsigned int numExpectedArgs, SlotId resultSlotId)
        {
            auto resultSlot = resultSlotId.IsValid() ? node.GetSlot(resultSlotId) : nullptr;
            const AZ::BehaviorParameter* resultType = method->GetResult();
            if (method->HasResult() && resultType && resultSlot)
            {
                AZStd::vector<AZStd::pair<Node*, const SlotId>> outputNodes = node.ModConnectedNodes(*resultSlot);
                if (!outputNodes.empty())
                {
                    const AZ::Outcome<Datum, AZStd::string> result = Datum::CallBehaviorContextMethodResult(method, resultType, params, numExpectedArgs);

                    if (result.IsSuccess())
                    {
                        for (auto& nodePtrSlot : outputNodes)
                        {
                            if (nodePtrSlot.first)
                            {
                                node.SetInput(*nodePtrSlot.first, nodePtrSlot.second, result.GetValue());
                            }
                        }
                    }
                    else
                    {
                       return AZ::Failure(result.GetError());
                    }

                    // no error and call was attempted
                    return AZ::Success(true);
                }
                // it is fine to have no output nodes
            }
            else if (resultSlotId.IsValid())
            {
                return AZ::Failure(AZStd::string::format("Script Canvas attempt to call %s failed, valid slot ID passed in, but no slot found for it", method->m_name.c_str()));
            }
            // it is fine for the method to have no result, under any other condition

            // no error, no attempt to call was made
            return AZ::Success(false);
        }
    }; // struct BehaviorContextMethodHelper    

} // namespace ScriptCanvas