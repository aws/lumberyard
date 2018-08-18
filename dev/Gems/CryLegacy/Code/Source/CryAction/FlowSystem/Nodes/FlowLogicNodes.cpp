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
class CLogicNode
    : public CFlowBaseNode < eNCT_Instanced >
{
    enum InputPorts
    {
        Activate,
        A,
        B,
    };

    enum OutputPorts
    {
        Out,
        OnTrue,
        OnFalse,
    };

public:
    CLogicNode(SActivationInfo* pActInfo)
        : m_bResult(2)
    {
    }

    void Serialize(SActivationInfo*, TSerialize ser) override
    {
        ser.Value("m_bResult", m_bResult);
    }

    // pure virtual!  Use this as a base for 2-operand boolean logic nodes.
    virtual IFlowNodePtr Clone(SActivationInfo* pActInfo) = 0;

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig in_config[] =
        {
            InputPortConfig<bool>("A", _HELP("[Out] = [A] op [B]")),
            InputPortConfig<bool>("B", _HELP("[Out] = [A] op [B]")),
            { 0 }
        };
        static const SOutputPortConfig out_config[] =
        {
            OutputPortConfig<bool>("Out", _HELP("[Out] = [A] op [B]")),
            OutputPortConfig<bool>("OnTrue", _HELP("Activated if [Out] is true.")),
            OutputPortConfig<bool>("OnFalse", _HELP("Activated if [Out] is false.")),
            { 0 }
        };
        config.sDescription = _HELP("Perform logical operation on inputs [A] and [B] and activate result to [Out].\n[OnTrue] port is activated if the result was true, otherwise the [OnFalse] port is activated.");
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.nFlags |= (EFLN_AISEQUENCE_SUPPORTED | EFLN_ACTIVATION_INPUT);
        config.SetCategory(EFLN_APPROVED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        switch (event)
        {
        case eFE_Initialize:
        {
            m_bResult = 2;
            break;
        }

        case eFE_Activate:
        {
            if (IsPortActive(pActInfo, InputPorts::Activate))
            {
                bool a = GetPortBool(pActInfo, InputPorts::A);
                bool b = GetPortBool(pActInfo, InputPorts::B);
                m_bResult = Execute(a, b);
                ActivateOutput(pActInfo, OutputPorts::Out, m_bResult ? 1 : 0);
                ActivateOutput(pActInfo, m_bResult ? OutputPorts::OnTrue : OutputPorts::OnFalse, true);
            }
        }
        }
    }

private:
    virtual bool Execute(bool a, bool b) = 0;

    int8 m_bResult;
};

//////////////////////////////////////////////////////////////////////////
class CFlowNode_AND
    : public CLogicNode
{
public:
    CFlowNode_AND(SActivationInfo* pActInfo)
        : CLogicNode(pActInfo)
    {
    }

    IFlowNodePtr Clone(SActivationInfo* pActInfo) override
    {
        return new CFlowNode_AND(pActInfo);
    }

    void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }

private:
    bool Execute(bool a, bool b) override { return a && b; }
};

//////////////////////////////////////////////////////////////////////////
class CFlowNode_OR
    : public CLogicNode
{
public:
    CFlowNode_OR(SActivationInfo* pActInfo)
        : CLogicNode(pActInfo)
    {
    }

    IFlowNodePtr Clone(SActivationInfo* pActInfo) override
    {
        return new CFlowNode_OR(pActInfo);
    }

    void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }

private:
    bool Execute(bool a, bool b) override { return a || b; }
};

//////////////////////////////////////////////////////////////////////////
class CFlowNode_XOR
    : public CLogicNode
{
public:
    CFlowNode_XOR(SActivationInfo* pActInfo)
        : CLogicNode(pActInfo)
    {
    }

    IFlowNodePtr Clone(SActivationInfo* pActInfo) override
    {
        return new CFlowNode_XOR(pActInfo);
    }

    void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }

private:
    bool Execute(bool a, bool b) override { return a ^ b; }
};

//////////////////////////////////////////////////////////////////////////
class CFlowNode_NOT
    : public CFlowBaseNode < eNCT_Singleton >
{
    enum InputPorts
    {
        In,
    };

    enum OutputPorts
    {
        Out,
    };

public:
    CFlowNode_NOT(SActivationInfo* pActInfo)
    {
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig in_config[] =
        {
            InputPortConfig<bool>("In", _HELP("[Out] = not [In]")),
            { 0 }
        };
        static const SOutputPortConfig out_config[] =
        {
            OutputPortConfig<bool>("Out", _HELP("[Out] = not [In]")),
            { 0 }
        };
        config.sDescription = _HELP("Inverts [In] port, [Out] port will be true when [In] is false and vice versa.");
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.nFlags |= EFLN_AISEQUENCE_SUPPORTED;
        config.SetCategory(EFLN_APPROVED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        switch (event)
        {
        case eFE_Activate:
        case eFE_Initialize:
        {
            bool value = GetPortBool(pActInfo, InputPorts::In);
            ActivateOutput(pActInfo, OutputPorts::Out, !value);
            break;
        }
        }
    }

    void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }
};

//////////////////////////////////////////////////////////////////////////
class CFlowNode_OnChange
    : public CFlowBaseNode < eNCT_Instanced >
{
    enum InputPorts
    {
        In,
    };

    enum OutputPorts
    {
        Out,
    };

public:
    CFlowNode_OnChange(SActivationInfo* pActInfo)
        : m_bCurrent(false)
    {
    }

    IFlowNodePtr Clone(SActivationInfo* pActInfo) override
    {
        return new CFlowNode_OnChange(pActInfo);
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig in_config[] =
        {
            InputPortConfig<bool>("In", _HELP("Input")),
            { 0 }
        };
        static const SOutputPortConfig out_config[] =
        {
            OutputPortConfig<bool>("Out", _HELP("Output")),
            { 0 }
        };
        config.sDescription = _HELP("Activates [In] value to [Out] only when it is different from its previous value.");
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.nFlags |= EFLN_AISEQUENCE_SUPPORTED;
        config.SetCategory(EFLN_APPROVED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        switch (event)
        {
        case eFE_Activate:
        case eFE_Initialize:
        {
            bool value = GetPortBool(pActInfo, InputPorts::In);
            if (value != m_bCurrent)
            {
                ActivateOutput(pActInfo, OutputPorts::Out, value);
                m_bCurrent = value;
            }
            break;
        }
        }
    }

    void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }

private:
    bool m_bCurrent;
};

//////////////////////////////////////////////////////////////////////////
class CFlowNode_Any
    : public CFlowBaseNode < eNCT_Singleton >
{
    enum InputPorts
    {   // instead of enumerating every input (and they aren't referenced explicitly) just define a max.
        InMax = 10, // the maximum number of inputs
    };

    enum OutputPorts
    {
        Out,
    };

public:
    CFlowNode_Any(SActivationInfo* pActInfo)
    {
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig in_config[] =
        {
            InputPortConfig_AnyType("In0", _HELP("Input 0")),
            InputPortConfig_AnyType("In1", _HELP("Input 1")),
            InputPortConfig_AnyType("In2", _HELP("Input 2")),
            InputPortConfig_AnyType("In3", _HELP("Input 3")),
            InputPortConfig_AnyType("In4", _HELP("Input 4")),
            InputPortConfig_AnyType("In5", _HELP("Input 5")),
            InputPortConfig_AnyType("In6", _HELP("Input 6")),
            InputPortConfig_AnyType("In7", _HELP("Input 7")),
            InputPortConfig_AnyType("In8", _HELP("Input 8")),
            InputPortConfig_AnyType("In9", _HELP("Input 9")),
            { 0 }
        };
        static const SOutputPortConfig out_config[] =
        {
            OutputPortConfig_AnyType("Out", _HELP("Output")),
            { 0 }
        };
        config.sDescription = _HELP("Any - Activates [Out] port when any [In#] port is activated.");
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.nFlags |= EFLN_AISEQUENCE_SUPPORTED;
        config.SetCategory(EFLN_APPROVED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        switch (event)
        {
        case eFE_Activate:
        case eFE_Initialize:
        {
            for (int i = 0; i < InputPorts::InMax; i++)
            {
                if (IsPortActive(pActInfo, i))
                {
                    ActivateOutput(pActInfo, OutputPorts::Out, pActInfo->pInputPorts[i]);
                }
            }
            break;
        }
        }
    }

    void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }
};

//////////////////////////////////////////////////////////////////////////
class CFlowNode_Blocker
    : public CFlowBaseNode < eNCT_Singleton >
{
    enum InputPorts
    {
        Block,
        In,
    };

    enum OutputPorts
    {
        Out,
    };

public:
    CFlowNode_Blocker(SActivationInfo* pActInfo)
    {
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig in_config[] =
        {
            InputPortConfig<bool>("Block", false, _HELP("If true, blocks [In] from passing to [Out]. Otherwise passes [In] to [Out].")),
            InputPortConfig_AnyType("In", _HELP("Input")),
            { 0 }
        };
        static const SOutputPortConfig out_config[] =
        {
            OutputPortConfig_AnyType("Out", _HELP("Activates from [In] unless [Block]ed.")),
            { 0 }
        };
        config.sDescription = _HELP("Blocker - If [Block] is true, blocks [In] signals, otherwise passes them to [Out].");
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.nFlags |= EFLN_AISEQUENCE_SUPPORTED;
        config.SetCategory(EFLN_APPROVED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        switch (event)
        {
        case eFE_Activate:
        case eFE_Initialize:
        {
            if (IsPortActive(pActInfo, InputPorts::In) && !GetPortBool(pActInfo, InputPorts::Block))
            {
                ActivateOutput(pActInfo, OutputPorts::Out, GetPortAny(pActInfo, InputPorts::In));
            }
            break;
        }
        }
    }

    void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }
};

//////////////////////////////////////////////////////////////////////////
class CFlowNode_LogicGate
    : public CFlowBaseNode < eNCT_Instanced >
{
    enum InputPorts
    {
        In,
        Closed,
        Open,
        Close,
    };

    enum OutputPorts
    {
        Out,
        OutClosed,
    };

protected:
    bool m_bClosed;

public:
    CFlowNode_LogicGate(SActivationInfo* pActInfo)
        : m_bClosed(false)
    {
    }

    IFlowNodePtr Clone(SActivationInfo* pActInfo) override
    {
        return new CFlowNode_LogicGate(pActInfo);
    }

    void Serialize(SActivationInfo*, TSerialize ser) override
    {
        ser.Value("m_bClosed", m_bClosed);
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig in_config[] =
        {
            InputPortConfig_AnyType("In", _HELP("Input")),
            InputPortConfig<bool>("Closed", false, _HELP("If true, blocks [In] data from passing to [Out]. Otherwise passes [In] to [Out].")),
            InputPortConfig_Void("Open", _HELP("Sets [Closed] to false.")),
            InputPortConfig_Void("Close", _HELP("Sets [Closed] to true.")),
            { 0 }
        };
        static const SOutputPortConfig out_config[] =
        {
            OutputPortConfig_AnyType("Out", _HELP("Output")),
            OutputPortConfig_AnyType("OutClosed", _HELP("Output if the gate is closed")),
            { 0 }
        };
        config.sDescription = _HELP("Gate - If gate is closed, activates the [OutClosed] port, otherwise activates the [Out] port.");
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.nFlags |= EFLN_AISEQUENCE_SUPPORTED;
        config.SetCategory(EFLN_APPROVED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        switch (event)
        {
        case eFE_Initialize:
        {
            m_bClosed = GetPortBool(pActInfo, InputPorts::Closed);
            break;
        }

        case eFE_Activate:
        {
            if (IsPortActive(pActInfo, InputPorts::Closed))
            {
                m_bClosed = GetPortBool(pActInfo, InputPorts::Closed);
            }
            if (IsPortActive(pActInfo, InputPorts::Open))
            {
                m_bClosed = false;
            }
            if (IsPortActive(pActInfo, InputPorts::Close))
            {
                m_bClosed = true;
            }
            if (IsPortActive(pActInfo, InputPorts::In))
            {
                ActivateOutput(pActInfo, m_bClosed ? OutputPorts::OutClosed : OutputPorts::Out, GetPortAny(pActInfo, InputPorts::In));
            }
            break;
        }
        }
    }

    void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }
};

//////////////////////////////////////////////////////////////////////////
class CFlowNode_All
    : public CFlowBaseNode < eNCT_Instanced >
{
    enum InputPorts
    {
        InMax = 8,  // max number of value input ports
        Reset = InMax,
    };

    enum OutputPorts
    {
        Out,
    };

public:
    CFlowNode_All(SActivationInfo* pActInfo)
        : m_inputCount(0)
    {
        ResetState();
    }

    void    ResetState()
    {
        for (unsigned i = 0; i < InputPorts::InMax; i++)
        {
            m_triggered[i] = false;
        }
        m_outputTrig = false;
    }

    IFlowNodePtr Clone(SActivationInfo* pActInfo) override
    {
        CFlowNode_All* clone = new CFlowNode_All(pActInfo);
        clone->m_inputCount = m_inputCount;
        return clone;
    }

    void Serialize(SActivationInfo*, TSerialize ser) override
    {
        char name[32];
        ser.Value("m_inputCount", m_inputCount);
        ser.Value("m_outputTrig", m_outputTrig);
        for (int i = 0; i < InputPorts::InMax; ++i)
        {
            sprintf_s(name, "m_triggered_%d", i);
            ser.Value(name, m_triggered[i]);
        }
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig in_config[] =
        {
            InputPortConfig_Void("In0", _HELP("Input 0")),
            InputPortConfig_Void("In1", _HELP("Input 1")),
            InputPortConfig_Void("In2", _HELP("Input 2")),
            InputPortConfig_Void("In3", _HELP("Input 3")),
            InputPortConfig_Void("In4", _HELP("Input 4")),
            InputPortConfig_Void("In5", _HELP("Input 5")),
            InputPortConfig_Void("In6", _HELP("Input 6")),
            InputPortConfig_Void("In7", _HELP("Input 7")),
            InputPortConfig_Void("Reset", _HELP("Reset the input activation count to 0.")),
            { 0 }
        };
        static const SOutputPortConfig out_config[] =
        {
            OutputPortConfig_Void("Out", _HELP("Output")),
            { 0 }
        };
        config.sDescription = _HELP("All - Activates [Out] when all connected [In#] ports have been activated.");
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.nFlags |= EFLN_AISEQUENCE_SUPPORTED;
        config.SetCategory(EFLN_APPROVED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        switch (event)
        {
        case eFE_ConnectInputPort:
        {
            // Count the number if inputs connected.
            if (pActInfo->connectPort < InputPorts::InMax)
            {
                m_inputCount++;
            }
            break;
        }
        case eFE_DisconnectInputPort:
        {
            // Count the number if inputs connected.
            if (pActInfo->connectPort < InputPorts::InMax)
            {
                m_inputCount--;
            }
            break;
        }
        case eFE_Initialize:
        {
            ResetState();
            // Fall through to check the input.
        }
        case eFE_Activate:
        {
            // Inputs
            int ntrig = 0;
            for (int i = 0; i < InputPorts::InMax; i++)
            {
                if (!m_triggered[i] && IsPortActive(pActInfo, i))
                {
                    m_triggered[i] = (event == eFE_Activate);
                }
                if (m_triggered[i])
                {
                    ntrig++;
                }
            }
            // Reset
            if (IsPortActive(pActInfo, InputPorts::Reset))
            {
                ResetState();
                ntrig = 0;
            }
            // If all inputs have been activated, activate output.
            // make sure we actually have connected inputs!
            if (!m_outputTrig && m_inputCount > 0 && ntrig == m_inputCount)
            {
                ActivateOutput(pActInfo, OutputPorts::Out, true);
                m_outputTrig = (event == eFE_Activate);
            }
            break;
        }
        }
    }

    void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }

private:
    int     m_inputCount;
    bool m_triggered[InputPorts::InMax];
    bool    m_outputTrig;
};

//////////////////////////////////////////////////////////////////////////
class CFlowNode_LogicOnce
    : public CFlowBaseNode < eNCT_Instanced >
{
    enum InputPorts
    {
        InMax = 8,  // max number of value input ports
        Reset = InMax,
    };

    enum OutputPorts
    {
        Out,
    };

public:
    CFlowNode_LogicOnce(SActivationInfo* pActInfo)
        : m_bTriggered(false)
    {
    }

    IFlowNodePtr Clone(SActivationInfo* pActInfo) override
    {
        return new CFlowNode_LogicOnce(pActInfo);
    }

    void Serialize(SActivationInfo*, TSerialize ser) override
    {
        ser.Value("m_bTriggered", m_bTriggered);
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig in_config[] =
        {
            InputPortConfig_AnyType("In0", _HELP("Input 0")),
            InputPortConfig_AnyType("In1", _HELP("Input 1")),
            InputPortConfig_AnyType("In2", _HELP("Input 2")),
            InputPortConfig_AnyType("In3", _HELP("Input 3")),
            InputPortConfig_AnyType("In4", _HELP("Input 4")),
            InputPortConfig_AnyType("In5", _HELP("Input 5")),
            InputPortConfig_AnyType("In6", _HELP("Input 6")),
            InputPortConfig_AnyType("In7", _HELP("Input 7")),
            InputPortConfig_Void("Reset", _HELP("Reset (and allow new activation)")),
            { 0 }
        };
        static const SOutputPortConfig out_config[] =
        {
            OutputPortConfig_AnyType("Out", _HELP("Output")),
            { 0 }
        };
        config.sDescription = _HELP("Once - Passes an activated [In#] port to [Out] port only once.  Will block input after that until [Reset] is activated.");
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.nFlags |= EFLN_AISEQUENCE_SUPPORTED;
        config.SetCategory(EFLN_APPROVED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        switch (event)
        {
        case eFE_Initialize:
        {
            m_bTriggered = false;
            break;
        }
        case eFE_Activate:
        {
            if (m_bTriggered == false)
            {
                for (int i = 0; i < InputPorts::InMax; i++)
                {
                    if (IsPortActive(pActInfo, i))
                    {
                        ActivateOutput(pActInfo, OutputPorts::Out, GetPortAny(pActInfo, i));
                        m_bTriggered = true;
                        break;
                    }
                }
            }
            if (IsPortActive(pActInfo, InputPorts::Reset))
            {
                m_bTriggered = false;
            }
            break;
        }
        }
    }

    void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }

private:
    bool    m_bTriggered;
};


//////////////////////////////////////////////////////////////////////////
class CFlowNode_LogicCountBlocker
    : public CFlowBaseNode < eNCT_Instanced >
{
    enum InputPorts
    {
        In,
        Reset,
        Limit,
    };

    enum OutputPorts
    {
        Out,
    };

public:
    CFlowNode_LogicCountBlocker(SActivationInfo* pActInfo)
        : m_timesTriggered(0)
    {
    }

    IFlowNodePtr Clone(SActivationInfo* pActInfo) override
    {
        return new CFlowNode_LogicCountBlocker(pActInfo);
    }

    void Serialize(SActivationInfo*, TSerialize ser) override
    {
        ser.Value("timesTriggered", m_timesTriggered);
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig in_config[] =
        {
            InputPortConfig_AnyType("In", _HELP("Input")),
            InputPortConfig_Void("Reset", _HELP("Reset counter to 0.")),
            InputPortConfig<int>("Limit", 1, _HELP("How many times [In] is passed to [Out]. After the limit is reached, the output will be blocked until [Reset] is activated."), 0, _UICONFIG("v_min=1,v_max=1000000000")),
            { 0 }
        };
        static const SOutputPortConfig out_config[] =
        {
            OutputPortConfig_AnyType("Out", _HELP("Output (passed from [In]).")),
            { 0 }
        };
        config.sDescription = _HELP("Count Blocker - [In] is sent to [Out] a limited number of times, then blocked until [Reset] is activated.");
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.nFlags |= EFLN_AISEQUENCE_SUPPORTED;
        config.SetCategory(EFLN_APPROVED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        switch (event)
        {
        case eFE_Initialize:
        {
            m_timesTriggered = 0;
            break;
        }
        case eFE_Activate:
        {
            int limit = GetPortInt(pActInfo, InputPorts::Limit);

            if (m_timesTriggered < limit && IsPortActive(pActInfo, InputPorts::In))
            {
                ActivateOutput(pActInfo, OutputPorts::Out, GetPortAny(pActInfo, InputPorts::In));
                m_timesTriggered++;
                break;
            }
            if (IsPortActive(pActInfo, InputPorts::Reset))
            {
                m_timesTriggered = 0;
            }
            break;
        }
        }
    }

    void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }

private:
    int m_timesTriggered;
};



//////////////////////////////////////////////////////////////////////////
class CFlowNode_LogicOnce_NoSerialize
    : public CFlowBaseNode < eNCT_Instanced >
{
    enum InputPorts
    {
        In,
        Reset,
    };

    enum OutputPorts
    {
        Out,
    };

public:
    CFlowNode_LogicOnce_NoSerialize(SActivationInfo* pActInfo)
        : m_bTriggered(false)
    {
    }

    IFlowNodePtr Clone(SActivationInfo* pActInfo) override
    {
        return new CFlowNode_LogicOnce_NoSerialize(pActInfo);
    }

    void Serialize(SActivationInfo*, TSerialize ser) override
    {
        // this node ON PURPOSE does not serialize m_bTriggered. The whole idea of this node is to be
        // used for things that have to be triggered only once in a level, even if a previous savegame
        // is loaded. Its first use is for Tutorial PopUps: after the player see one, that popup should
        // not popup again even if the player loads a previous savegame.  It is not perfect: leaving the
        // game, re-launching, and using the "Resume Game" option, will make the node to be triggered
        // again if an input is activated.
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig in_config[] =
        {
            InputPortConfig_AnyType("In", _HELP("Input")),
            InputPortConfig_Void("Reset", _HELP("Reset (allow new activation).")),
            { 0 }
        };
        static const SOutputPortConfig out_config[] =
        {
            OutputPortConfig_AnyType("Out", _HELP("Output")),
            { 0 }
        };
        config.sDescription = _HELP("Once (No Serialize) - Passes [In] port to [Out] port only once.  Will block input after that until [Reset] is activated.\nWARNING!! The triggered flag is not serialized on savegame!. \nThis means that even if a previous savegame is loaded after the node has been triggered, the node won't be triggered again!");
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.nFlags |= EFLN_AISEQUENCE_SUPPORTED;
        config.SetCategory(EFLN_APPROVED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        switch (event)
        {
        case eFE_Initialize:
        {
            if (gEnv->IsEditor())
            {
                m_bTriggered = false;
            }
            break;
        }
        case eFE_Activate:
        {
            if (m_bTriggered == false)
            {
                if (IsPortActive(pActInfo, InputPorts::In))
                {
                    ActivateOutput(pActInfo, OutputPorts::Out, GetPortAny(pActInfo, InputPorts::In));
                    m_bTriggered = true;
                    break;
                }
            }
            if (IsPortActive(pActInfo, InputPorts::Reset))
            {
                m_bTriggered = false;
            }
            break;
        }
        }
    }

    void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }

private:
    bool    m_bTriggered;
};



//////////////////////////////////////////////////////////////////////////
class CFlowNode_RandomSelect
    : public CFlowBaseNode < eNCT_Singleton >
{
    enum InputPorts
    {
        In,
        MinOut,
        MaxOut,
    };

    enum OutputPorts
    {
        OutMax = 10,    // max number of value output ports
    };

public:
    CFlowNode_RandomSelect(SActivationInfo* pActInfo)
    {
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig in_config[] =
        {
            InputPortConfig_AnyType("In", _HELP("Input")),
            InputPortConfig<int>("MinOut", _HELP("Min number of outputs to activate.")),
            InputPortConfig<int>("MaxOut", _HELP("Max number of outputs to activate.")),
            { 0 }
        };

        static const SOutputPortConfig out_config[] =
        {
            OutputPortConfig_AnyType("Out0", _HELP("Output 0")),
            OutputPortConfig_AnyType("Out1", _HELP("Output1")),
            OutputPortConfig_AnyType("Out2", _HELP("Output2")),
            OutputPortConfig_AnyType("Out3", _HELP("Output3")),
            OutputPortConfig_AnyType("Out4", _HELP("Output4")),
            OutputPortConfig_AnyType("Out5", _HELP("Output5")),
            OutputPortConfig_AnyType("Out6", _HELP("Output6")),
            OutputPortConfig_AnyType("Out7", _HELP("Output7")),
            OutputPortConfig_AnyType("Out8", _HELP("Output8")),
            OutputPortConfig_AnyType("Out9", _HELP("Output9")),
            { 0 }
        };
        config.sDescription = _HELP("Random Select - Passes [In] to a random selection of [Out#] ports: (MinOut <= random <= MaxOut).");
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.nFlags |= EFLN_AISEQUENCE_SUPPORTED;
        config.SetCategory(EFLN_APPROVED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        switch (event)
        {
        case eFE_Initialize:
        case eFE_Activate:
        {
            if (IsPortActive(pActInfo, 0))
            {
                int minOut = GetPortInt(pActInfo, InputPorts::MinOut);
                int maxOut = GetPortInt(pActInfo, InputPorts::MaxOut);

                minOut = CLAMP(minOut, 0, OutputPorts::OutMax);
                maxOut = CLAMP(maxOut, 0, OutputPorts::OutMax);
                if (maxOut < minOut)
                {
                    std::swap(minOut, maxOut);
                }

                int n = cry_random(minOut, maxOut);

                // Collect the outputs to use
                static int out[OutputPorts::OutMax];
                for (unsigned i = 0; i < OutputPorts::OutMax; i++)
                {
                    out[i] = -1;
                }
                int nout = 0;
                for (int i = 0; i < OutputPorts::OutMax; i++)
                {
                    if (IsOutputConnected(pActInfo, i))
                    {
                        out[nout] = i;
                        nout++;
                    }
                }
                if (n > nout)
                {
                    n = nout;
                }

                // Shuffle
                for (int i = 0; i < n; i++)
                {
                    std::swap(out[i], out[cry_random(0, nout - 1)]);
                }

                // Set outputs.
                for (int i = 0; i < n; i++)
                {
                    if (out[i] == -1)
                    {
                        continue;
                    }
                    ActivateOutput(pActInfo, out[i], GetPortAny(pActInfo, InputPorts::In));
                }
            }
            break;
        }
        }
    }

    void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }
};

//////////////////////////////////////////////////////////////////////////
class CFlowNode_RandomTrigger
    : public CFlowBaseNode < eNCT_Instanced >
{
    enum InputPorts
    {
        In,
        Reset,
    };

    enum OutputPorts
    {
        OutMax = 10,    // max number of value output ports
        Done = OutMax,
    };

public:
    CFlowNode_RandomTrigger(SActivationInfo* pActInfo)
        : m_nOutputCount(0)
    {
        Init();
        ResetNode();
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig in_config[] =
        {
            InputPortConfig_AnyType("In", _HELP("Input")),
            InputPortConfig_Void("Reset", _HELP("Resets the activations to 0.")),
            { 0 }
        };
        static const SOutputPortConfig out_config[] =
        {
            OutputPortConfig_AnyType("Out0", _HELP("Output 0")),
            OutputPortConfig_AnyType("Out1", _HELP("Output1")),
            OutputPortConfig_AnyType("Out2", _HELP("Output2")),
            OutputPortConfig_AnyType("Out3", _HELP("Output3")),
            OutputPortConfig_AnyType("Out4", _HELP("Output4")),
            OutputPortConfig_AnyType("Out5", _HELP("Output5")),
            OutputPortConfig_AnyType("Out6", _HELP("Output6")),
            OutputPortConfig_AnyType("Out7", _HELP("Output7")),
            OutputPortConfig_AnyType("Out8", _HELP("Output8")),
            OutputPortConfig_AnyType("Out9", _HELP("Output9")),
            OutputPortConfig_Void("Done", _HELP("Activated after all connected outputs have been activated.")),
            { 0 }
        };
        config.sDescription = _HELP("Random Trigger - On each [In] activation, activates one of the connected [Out#] ports in random order.");
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.nFlags |= EFLN_AISEQUENCE_SUPPORTED;
        config.SetCategory(EFLN_APPROVED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        switch (event)
        {
        case eFE_ConnectOutputPort:
        {
            if (pActInfo->connectPort < OutputPorts::OutMax)
            {
                ++m_nConnectionCounts[pActInfo->connectPort];
                // check if already connected
                for (int i = 0; i < m_nOutputCount; ++i)
                {
                    if (m_nConnectedPorts[i] == pActInfo->connectPort)
                    {
                        return;
                    }
                }
                m_nConnectedPorts[m_nOutputCount++] = pActInfo->connectPort;
                ResetNode();
            }
            break;
        }
        case eFE_DisconnectOutputPort:
        {
            if (pActInfo->connectPort < OutputPorts::OutMax)
            {
                for (int i = 0; i < m_nOutputCount; ++i)
                {
                    // check if really connected
                    if (m_nConnectedPorts[i] == pActInfo->connectPort)
                    {
                        if (m_nConnectionCounts[pActInfo->connectPort] == 1)
                        {
                            m_nConnectedPorts[i] = m_nPorts[m_nOutputCount - 1]; // copy last value to here
                            m_nConnectedPorts[m_nOutputCount - 1] = -1;
                            --m_nOutputCount;

                            ResetNode();
                        }
                        --m_nConnectionCounts[pActInfo->connectPort];
                        return;
                    }
                }
            }
            break;
        }
        case eFE_Initialize:
        {
            ResetNode();
            break;
        }
        case eFE_Activate:
        {
            if (IsPortActive(pActInfo, InputPorts::Reset))
            {
                ResetNode();
            }
            if (IsPortActive(pActInfo, InputPorts::In))
            {
                int numCandidates = m_nOutputCount - m_nTriggered;
                if (numCandidates <= 0)
                {
                    return;
                }

                const int cand = cry_random(0, numCandidates - 1);
                const int whichPort = m_nPorts[cand];
                m_nPorts[cand] = m_nPorts[numCandidates - 1];
                m_nPorts[numCandidates - 1] = -1;
                ++m_nTriggered;
                assert(whichPort >= 0 && whichPort < OutputPorts::OutMax);
                // CryLogAlways("CFlowNode_RandomTrigger: Activating %d", whichPort);
                ActivateOutput(pActInfo, whichPort, GetPortAny(pActInfo, InputPorts::In));
                assert(m_nTriggered > 0 && m_nTriggered <= m_nOutputCount);
                if (m_nTriggered == m_nOutputCount)
                {
                    // CryLogAlways("CFlowNode_RandomTrigger: Done.");
                    // Done
                    ActivateOutput(pActInfo, OutputPorts::Done, true);
                    ResetNode();
                }
            }
            break;
        }
        }
    }

    void Init()
    {
        for (int i = 0; i < OutputPorts::OutMax; ++i)
        {
            m_nConnectedPorts[i] = -1;
            m_nConnectionCounts[i] = 0;
        }
    }

    void ResetNode()
    {
        memcpy(m_nPorts, m_nConnectedPorts, sizeof(m_nConnectedPorts));
        m_nTriggered = 0;
    }

    void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }

    IFlowNodePtr Clone(SActivationInfo* pActInfo) override
    {
        CFlowNode_RandomTrigger* pClone = new CFlowNode_RandomTrigger(pActInfo);
        // copy connected ports to cloned node
        // because atm. we don't send the  eFE_ConnectOutputPort or eFE_DisconnectOutputPort
        // to cloned graphs (see CFlowGraphBase::Clone)
        memcpy(pClone->m_nConnectedPorts, m_nConnectedPorts, sizeof(m_nConnectedPorts));
        memcpy(pClone->m_nConnectionCounts, m_nConnectionCounts, sizeof(m_nConnectionCounts));
        pClone->m_nOutputCount = m_nOutputCount;
        pClone->m_nTriggered = m_nTriggered;
        pClone->ResetNode();
        return pClone;
    }

    void Serialize(SActivationInfo*, TSerialize ser) override
    {
        char name[32];
        ser.Value("m_nOutputCount", m_nOutputCount);
        ser.Value("m_nTriggered", m_nTriggered);
        for (int i = 0; i < OutputPorts::OutMax; ++i)
        {
            sprintf_s(name, "m_nPorts_%d", i);
            ser.Value(name, m_nPorts[i]);
        }
        // the m_nConnectedPorts must not be serialized. it's generated automatically
        // sanity check
        if (ser.IsReading())
        {
            bool bNeedReset = false;
            for (int i = 0; i < OutputPorts::OutMax && !bNeedReset; ++i)
            {
                bool bFound = false;
                for (int j = 0; j < OutputPorts::OutMax && !bFound; ++j)
                {
                    bFound = (m_nPorts[i] == m_nConnectedPorts[j]);
                }
                bNeedReset = !bFound;
            }
            // if some of the serialized port can not be found, reset
            if (bNeedReset)
            {
                ResetNode();
            }
        }
    }

private:
    int m_nConnectedPorts[OutputPorts::OutMax]; // array with port-numbers which are connected.
    int m_nPorts[OutputPorts::OutMax]; // permutation of m_nConnectedPorts array with m_nOutputCount valid entries
    int m_nConnectionCounts[OutputPorts::OutMax]; // number of connections on each port
    int m_nTriggered;
    int m_nOutputCount;
};


//////////////////////////////////////////////////////////////////////////
class CFlowNode_Sequencer
    : public CFlowBaseNode < eNCT_Instanced >
{
    enum InputPorts
    {
        In,
        Closed,
        Open,
        Close,
        Reset,
        Reverse,
    };

    enum OutputPorts
    {
        OutMax = 10,    // max number of value output ports
    };

    static const int InvalidPortIndex = -1;

public:
    CFlowNode_Sequencer(SActivationInfo* pActInfo)
        : m_needToCheckConnectedPorts(true)
        , m_lastTriggeredPort(InvalidPortIndex)
        , m_numConnectedPorts(0)
        , m_closed(false)
    {
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static_assert(InvalidPortIndex + 1 == 0, "Automatic bounds checks for incrementing port numbers won't work!");
        // or else the automatic boundary checks when incrememting the port number would not work

        static const SInputPortConfig in_config[] =
        {
            InputPortConfig_AnyType("In", _HELP("Input")),
            InputPortConfig<bool>("Closed", false, _HELP("If true, blocks all signals.")),
            InputPortConfig_Void("Open", _HELP("Sets [Closed] to false.")),
            InputPortConfig_Void("Close", _HELP("Sets [Closed] to true.")),
            InputPortConfig_Void("Reset", _HELP("Forces next output to be [Out0].")),
            InputPortConfig<bool>("Reverse", false, _HELP("If true, the order of output activation is reversed.")),
            { 0 }
        };
        static const SOutputPortConfig out_config[] =
        {
            OutputPortConfig_AnyType("Out0", _HELP("Output 0")),
            OutputPortConfig_AnyType("Out1", _HELP("Output1")),
            OutputPortConfig_AnyType("Out2", _HELP("Output2")),
            OutputPortConfig_AnyType("Out3", _HELP("Output3")),
            OutputPortConfig_AnyType("Out4", _HELP("Output4")),
            OutputPortConfig_AnyType("Out5", _HELP("Output5")),
            OutputPortConfig_AnyType("Out6", _HELP("Output6")),
            OutputPortConfig_AnyType("Out7", _HELP("Output7")),
            OutputPortConfig_AnyType("Out8", _HELP("Output8")),
            OutputPortConfig_AnyType("Out9", _HELP("Output9")),
            { 0 }
        };
        config.sDescription = _HELP("Sequencer - On each [In] activation, activates one of the connected outputs in sequential order.");
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.nFlags |= EFLN_AISEQUENCE_SUPPORTED;
        config.SetCategory(EFLN_APPROVED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        switch (event)
        {
        case eFE_Initialize:
        {
            m_closed = GetPortBool(pActInfo, InputPorts::Closed);
            // ly: intentional fall through?
        }

        case eFE_ConnectOutputPort:
        case eFE_DisconnectOutputPort:
        {
            m_lastTriggeredPort = InvalidPortIndex;
            m_needToCheckConnectedPorts = true;
            m_numConnectedPorts = 0;
            break;
        }

        case eFE_Activate:
        {
            if (IsPortActive(pActInfo, InputPorts::Closed))
            {
                m_closed = GetPortBool(pActInfo, InputPorts::Closed);
            }
            if (IsPortActive(pActInfo, InputPorts::Open))
            {
                m_closed = false;
            }
            if (IsPortActive(pActInfo, InputPorts::Close))
            {
                m_closed = true;
            }

            if (IsPortActive(pActInfo, InputPorts::Reset))
            {
                m_lastTriggeredPort = InvalidPortIndex;
            }

            if (m_needToCheckConnectedPorts)
            {
                m_needToCheckConnectedPorts = false;
                m_numConnectedPorts = 0;
                for (int port = 0; port < OutputPorts::OutMax; ++port)
                {
                    if (IsOutputConnected(pActInfo, port))
                    {
                        m_connectedPorts[m_numConnectedPorts] = port;
                        m_numConnectedPorts++;
                    }
                }
            }

            if (IsPortActive(pActInfo, InputPorts::In) && m_numConnectedPorts > 0 && !m_closed)
            {
                bool reversed = GetPortBool(pActInfo, InputPorts::Reverse);
                unsigned int port = m_lastTriggeredPort;

                if (reversed)
                {
                    port = min(m_numConnectedPorts - 1, port - 1); // this takes care of both the initial state when it has the PORT_NONE value, and the overflow in the normal decrement situation
                }
                else
                {
                    port = (port + 1) % m_numConnectedPorts;
                }

                ActivateOutput(pActInfo, m_connectedPorts[port], GetPortAny(pActInfo, InputPorts::In));
                m_lastTriggeredPort = port;
            }
            break;
        }
        }
    }

    void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }

    IFlowNodePtr Clone(SActivationInfo* pActInfo) override
    {
        CFlowNode_Sequencer* pClone = new CFlowNode_Sequencer(pActInfo);
        return pClone;
    }

    void Serialize(SActivationInfo*, TSerialize ser) override
    {
        ser.Value("m_lastTriggeredPort", m_lastTriggeredPort);
        ser.Value("m_closed", m_closed);
    }

private:
    bool IsOutputConnected(SActivationInfo* pActInfo, int nPort) const
    {
        SFlowAddress addr(pActInfo->myID, nPort, true);
        return pActInfo->pGraph->IsOutputConnected(addr);
    }

    bool m_needToCheckConnectedPorts;
    bool m_closed;
    unsigned int m_lastTriggeredPort;
    unsigned int m_numConnectedPorts;
    int m_connectedPorts[OutputPorts::OutMax];
};


//////////////////////////////////////////////////////////////////////////
/// Logic Flow Node Registration
//////////////////////////////////////////////////////////////////////////

REGISTER_FLOW_NODE("Logic:AND", CFlowNode_AND);
REGISTER_FLOW_NODE("Logic:OR", CFlowNode_OR);
REGISTER_FLOW_NODE("Logic:XOR", CFlowNode_XOR);
REGISTER_FLOW_NODE("Logic:NOT", CFlowNode_NOT);
REGISTER_FLOW_NODE("Logic:OnChange", CFlowNode_OnChange);
REGISTER_FLOW_NODE("Logic:Any", CFlowNode_Any);
REGISTER_FLOW_NODE("Logic:Blocker", CFlowNode_Blocker);
REGISTER_FLOW_NODE("Logic:All", CFlowNode_All);
REGISTER_FLOW_NODE("Logic:RandomSelect", CFlowNode_RandomSelect);
REGISTER_FLOW_NODE("Logic:RandomTrigger", CFlowNode_RandomTrigger);
REGISTER_FLOW_NODE("Logic:Once", CFlowNode_LogicOnce);
REGISTER_FLOW_NODE("Logic:CountBlocker", CFlowNode_LogicCountBlocker);
REGISTER_FLOW_NODE("Logic:OnceNoSerialize", CFlowNode_LogicOnce_NoSerialize);
REGISTER_FLOW_NODE("Logic:Gate", CFlowNode_LogicGate);
REGISTER_FLOW_NODE("Logic:Sequencer", CFlowNode_Sequencer);
