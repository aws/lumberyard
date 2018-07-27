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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#include "CryLegacy_precompiled.h"
#include <FlowSystem/Nodes/FlowBaseNode.h>

//////////////////////////////////////////////////////////////////////////
class CFlowNode_IfCondition
    : public CFlowBaseNode<eNCT_Instanced>
{
    enum InputPorts
    {
        Activate,
        Condition,
        In,
    };

    enum OutputPorts
    {
        OnFalse,
        OnTrue,
    };

public:
    CFlowNode_IfCondition(SActivationInfo*)
        : m_condition(false)
    {
    }

    IFlowNodePtr Clone(SActivationInfo* pActInfo) override
    {
        return new CFlowNode_IfCondition(pActInfo);
    }

    void Serialize(SActivationInfo*, TSerialize ser) override
    {
        ser.Value("condition", m_condition);
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig in_config [] =
        {
            InputPortConfig<bool>("Condition", _HELP("If [Condition] is false, input [In] will be routed to [OnFalse], otherwise to [OnTrue].")),
            InputPortConfig_AnyType("In", _HELP("Value to route to [OnFalse] or [OnTrue], based on [Condition].")),
            {0}
        };
        static const SOutputPortConfig out_config [] =
        {
            OutputPortConfig_AnyType("OnFalse", _HELP("Activated if [Condition] is false.")),
            OutputPortConfig_AnyType("OnTrue", _HELP("Activated if [Condition] is true.")),
            {0}
        };
        config.sDescription = _HELP("Routes data flow based on a boolean condition.\nSetting input [In] will route it to either [OnFalse] or [OnTrue], based on [Condition].");
        config.nFlags |= EFLN_ACTIVATION_INPUT;
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.SetCategory(EFLN_APPROVED);
    }

    void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        switch (event)
        {
        case eFE_Initialize:
        {
            m_condition = GetPortBool(pActInfo, InputPorts::Condition);
            break;
        }

        case eFE_Activate:
        {
            if (IsPortActive(pActInfo, InputPorts::Condition))
            {
                m_condition = GetPortBool(pActInfo, InputPorts::Condition);
            }

            // only port [Activate] triggers an output
            if (IsPortActive(pActInfo, InputPorts::Activate))
            {
                ActivateOutput(pActInfo,
                    m_condition ? OutputPorts::OnTrue : OutputPorts::OnFalse,
                    GetPortAny(pActInfo, InputPorts::In));
            }
            break;
        }
        }
    }

protected:
    bool m_condition;
};

//////////////////////////////////////////////////////////////////////////
class CFlowNode_SelectCondition
    : public CFlowBaseNode<eNCT_Instanced>
{
    enum InputPorts
    {
        Activate,
        Condition,
        InTrue,
        InFalse,
    };

    enum OutputPorts
    {
        Out,
    };

public:
    CFlowNode_SelectCondition(SActivationInfo*)
        : m_condition(false)
    {
    }

    IFlowNodePtr Clone(SActivationInfo* pActInfo) override
    {
        return new CFlowNode_SelectCondition(pActInfo);
    }

    void Serialize(SActivationInfo*, TSerialize ser) override
    {
        ser.Value("condition", m_condition);
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig in_config [] =
        {
            InputPortConfig<bool>("Condition", _HELP("If [Condition] is false, input [InFalse] will be routed to [Out], otherwise input [InTrue] will be routed to [Out].")),
            InputPortConfig_AnyType("InTrue", _HELP("Value sent to [Out] when [Condition] is true.")),
            InputPortConfig_AnyType("InFalse", _HELP("Value sent to [Out] when [Condition] is false.")),
            {0}
        };
        static const SOutputPortConfig out_config [] =
        {
            OutputPortConfig_AnyType("Out", _HELP("Activated from [InFalse] or [InTrue], based on [Condition]")),
            {0}
        };
        config.sDescription = _HELP("Routes data flow based on a boolean condition.\nDepending on [Condition], either input [InTrue] or [InFalse] will be routed to [Out].");
        config.nFlags |= EFLN_ACTIVATION_INPUT;
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.SetCategory(EFLN_APPROVED);
    }

    void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        switch (event)
        {
        case eFE_Initialize:
        {
            m_condition = GetPortBool(pActInfo, InputPorts::Condition);
            break;
        }

        case eFE_Activate:
        {
            if (IsPortActive(pActInfo, InputPorts::Condition))
            {
                m_condition = GetPortBool(pActInfo, InputPorts::Condition);
            }

            if (IsPortActive(pActInfo, InputPorts::Activate))
            {
                ActivateOutput(pActInfo,
                    OutputPorts::Out,
                    GetPortAny(pActInfo, m_condition ? InputPorts::InTrue : InputPorts::InFalse));
            }
            break;
        }
        }
    }

protected:
    bool m_condition;
};


//////////////////////////////////////////////////////////////////////////
/// Logic Condition Flow Node Registration
//////////////////////////////////////////////////////////////////////////

REGISTER_FLOW_NODE("Logic:IfCondition", CFlowNode_IfCondition)
REGISTER_FLOW_NODE("Logic:SelectCondition", CFlowNode_SelectCondition)
