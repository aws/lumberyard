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

#include "Print.h"

#include <ScriptCanvas/Execution/RuntimeBus.h>
#include <ScriptCanvas/Execution/ExecutionBus.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace String
        {
            void Print::OnInputSignal(const SlotId& slotId)
            {
#if !defined(PERFORMANCE_BUILD) && !defined(_RELEASE) 
                AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::ScriptCanvas, "ScriptCanvas::Print::OnInputSignal");

                AZStd::string text = ProcessFormat();

                AZ_TracePrintf("Script Canvas", "%s\n", text.c_str());
                LogNotificationBus::Event(GetGraphId(), &LogNotifications::LogMessage, text);

                AZ::EntityId assetNodeId{};
                ScriptCanvas::RuntimeRequestBus::EventResult(assetNodeId, m_executionUniqueId, &ScriptCanvas::RuntimeRequests::FindAssetNodeIdByRuntimeNodeId, GetEntityId());

                SC_EXECUTION_TRACE_ANNOTATE_NODE((*this), (AnnotateNodeSignal(CreateGraphInfo(m_executionUniqueId, GetGraphIdentifier()), AnnotateNodeSignal::AnnotationLevel::Info, text, AZ::NamedEntityId(assetNodeId, GetNodeName()))));
#endif
                SignalOutput(GetSlotId("Out"));

            }
        }
    }
}

#include <Include/ScriptCanvas/Libraries/String/Print.generated.cpp>