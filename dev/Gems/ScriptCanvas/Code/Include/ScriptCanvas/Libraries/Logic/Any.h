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

#include <AzCore/std/containers/vector.h>

#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Core/GraphBus.h>

#include <ScriptCanvas/CodeGen/CodeGen.h>

#include <Include/ScriptCanvas/Libraries/Logic/Any.generated.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Logic
        {
            class Any
                : public Node
                , public EndpointNotificationBus::MultiHandler
            {
                ScriptCanvas_Node(Any,
                    ScriptCanvas_Node::Uuid("{6359C34F-3C34-4784-9FFD-18F1C8E3482D}")
                    ScriptCanvas_Node::Description("Will trigger the Out pin whenever any of the In pins get triggered")
                );
                
            public:
                Any();

                // Node
                void OnInit() override;

                void OnInputSignal(const SlotId& slot) override;
                ////

                // EndpointNotificationBus
                void OnEndpointConnected(const Endpoint& endpoint) override;
                void OnEndpointDisconnected(const Endpoint& endpoint) override;
                ////
    
            protected:

                // Outputs
                ScriptCanvas_Out(ScriptCanvas_Out::Name("Out", "Signaled when the node receives a signal from the selected index"));
                
            private:

                AZStd::string GenerateInputName(int counter);

                bool AllInputSlotsFilled() const;
                void AddInputSlot();
                void CleanUpInputSlots();
                void FixupStateNames();

                ScriptCanvas_SerializeProperty(AZStd::vector< SlotId >, m_inputSlots);
            };
        }
    }
}