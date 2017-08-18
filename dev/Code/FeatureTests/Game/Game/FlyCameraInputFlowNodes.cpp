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
#include "StdAfx.h"

#include <FlowSystem/Nodes/FlowBaseNode.h>

#include "FlyCameraInputBus.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace LYGame
{
    const string g_flyCameraInputSetEnabledNodePath = "Game:FlyCameraInput:SetEnabled";

    ////////////////////////////////////////////////////////////////////////////////////////////////
    class FlyCameraInputSetEnabledFlowNode
        : public CFlowBaseNode<eNCT_Instanced>
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        FlyCameraInputSetEnabledFlowNode(SActivationInfo* activationInfo)
            : CFlowBaseNode()
        {
        }

        ////////////////////////////////////////////////////////////////////////////////////////////
        ~FlyCameraInputSetEnabledFlowNode() override
        {
        }

        // IFlowNode
        IFlowNodePtr Clone(SActivationInfo* pActInfo) override
        {
            return new FlyCameraInputSetEnabledFlowNode(pActInfo);
        }
        // ~IFlowNode

    protected:
        enum InputPorts
        {
            InputPortActivate = 0,
            InputPortEnable,
        };

        enum OutputPorts
        {
            OutputPortDone = 0
        };

        ////////////////////////////////////////////////////////////////////////////////////////////
        void GetConfiguration(SFlowNodeConfig& config) override
        {
            static const SInputPortConfig inputs[] =
            {
                InputPortConfig_Void("Activate", _HELP("Set the enabled flag")),
                InputPortConfig<bool>("Enabled", false, _HELP("Whether the fly camera input should be enabled.")),
                { 0 }
            };

            static const SOutputPortConfig outputs[] =
            {
                OutputPortConfig_Void("Done", _HELP("Sends a signal when the node has executed.")),
                { 0 }
            };

            config.pInputPorts = inputs;
            config.pOutputPorts = outputs;
            config.sDescription = _HELP("Enables or disables the fly camera input on a camera entity.");
            config.nFlags |= EFLN_TARGET_ENTITY;
            config.SetCategory(EFLN_APPROVED);
        }


        ////////////////////////////////////////////////////////////////////////////////////////////
        void ProcessEvent(EFlowEvent event, SActivationInfo* activationInfo) override
        {
            if (eFE_Activate == event && IsPortActive(activationInfo, InputPortActivate))
            {
                FlowEntityId id = activationInfo->entityId;
                AZ::EntityId entityId = activationInfo->entityId;
                if (id == FlowEntityId::s_invalidFlowEntityID || IsLegacyEntityId(entityId))
                {
                    CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING,
                        "FlowGraph: %s Node: must have a component entity specified\n",
                        g_flyCameraInputSetEnabledNodePath.c_str());
                    return;
                }

                bool enabled = GetPortBool(activationInfo, InputPortEnable);

                EBUS_EVENT_ID(entityId, FlyCameraInputBus, SetIsEnabled, enabled);

                // Trigger output
                ActivateOutput(activationInfo, OutputPortDone, 0);
            }
        }

        ////////////////////////////////////////////////////////////////////////////////////////////
        void GetMemoryUsage(ICrySizer* sizer) const override
        {
            sizer->Add(*this);
        }

    };

    REGISTER_FLOW_NODE(g_flyCameraInputSetEnabledNodePath, FlyCameraInputSetEnabledFlowNode)
}
