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

// Description : FlowGraph Node that sends an output upon leaving the game (either in Editor or in Pure Game mode).

#include "CryLegacy_precompiled.h"
#include <FlowSystem/Nodes/FlowBaseNode.h>

class CFlowNode_Stop
    : public CFlowBaseNode<eNCT_Instanced>
{
public:

    CFlowNode_Stop(SActivationInfo* pActInfo)
        : m_bActive(false)
    {}

    //////////////////////////////////////////////////////////////////////////
    ~CFlowNode_Stop() {}

    //////////////////////////////////////////////////////////////////////////
    virtual IFlowNodePtr Clone(SActivationInfo* pActInfo)
    {
        return new CFlowNode_Stop(pActInfo);
    }

    //////////////////////////////////////////////////////////////////////////
    enum INPUTS
    {
        eIn_TriggerInGame,
        eIn_TriggerInEditor,
    };

    enum OUTPUTS
    {
        eOut_Output,
    };

    //////////////////////////////////////////////////////////////////////////
    virtual void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig inputs[] =
        {
            InputPortConfig<bool>("TriggerInGame", true, _HELP("Determines if the output should be triggered in the PureGameMode"), "InGame"),
            InputPortConfig<bool>("TriggerInEditor", true, _HELP("Determines if the output should be triggered in the EditorGameMode"), "InEditor"),
            {0}
        };

        static const SOutputPortConfig outputs[] =
        {
            OutputPortConfig_Void("Output", _HELP("Produces output on game stop"), "output"),
            {0}
        };

        config.pInputPorts = inputs;
        config.pOutputPorts = outputs;
        config.sDescription = _HELP("This node sends output when leaving the game.");
        config.SetCategory(EFLN_APPROVED);
    }

    //////////////////////////////////////////////////////////////////////////
    virtual void Serialize(SActivationInfo* pActInfo, TSerialize ser)
    {
        bool bActive = m_bActive;
        ser.Value("active", bActive);

        if (ser.IsReading())
        {
            m_bActive = false;
            SetState(pActInfo, bActive);
        }
    }

    //////////////////////////////////////////////////////////////////////////
    virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        SFlowAddress addr(pActInfo->myID, eOut_Output, true);

        switch (event)
        {
        case eFE_Update:
        {
            if (gEnv->IsEditor())
            {
                if (gEnv->IsEditing())
                {
                    pActInfo->pGraph->ActivatePort(addr, true);
                    SetState(pActInfo, false);
                }
            }
            else
            {
                // when we're in pure game mode
                if (gEnv->pSystem->IsQuitting())     //Is this too late?
                {
                    pActInfo->pGraph->ActivatePort(addr, true);
                    SetState(pActInfo, false);
                }
            }

            break;
        }

        case eFE_Initialize:
        {
            SetState(pActInfo, true);
            break;
        }
        }
    }

    //////////////////////////////////////////////////////////////////////////
    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }

private:

    //////////////////////////////////////////////////////////////////////////
    bool CanTriggerInGame(SActivationInfo* const pActInfo) const
    {
        return *(pActInfo->pInputPorts[eIn_TriggerInGame].GetPtr<bool>());
    }

    //////////////////////////////////////////////////////////////////////////
    bool CanTriggerInEditor(SActivationInfo* const pActInfo) const
    {
        return *(pActInfo->pInputPorts[eIn_TriggerInEditor].GetPtr<bool>());
    }

    //////////////////////////////////////////////////////////////////////////
    void SetState(SActivationInfo* const pActInfo, bool const bActive)
    {
        if (m_bActive != bActive)
        {
            m_bActive = bActive;
            pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, m_bActive);
        }
    }

    bool m_bActive;
};

REGISTER_FLOW_NODE("Game:Stop", CFlowNode_Stop);
