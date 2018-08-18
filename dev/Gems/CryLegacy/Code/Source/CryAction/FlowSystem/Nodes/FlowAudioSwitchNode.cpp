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

// Description : FlowGraph Node that sets AudioSwitches.


#include "CryLegacy_precompiled.h"
#include <FlowSystem/Nodes/FlowBaseNode.h>
#include "Components/IComponentAudio.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////
class CFlowNode_AudioSwitch
    : public CFlowBaseNode<eNCT_Instanced>
{
public:
    //////////////////////////////////////////////////////////////////////////////////////////////////
    CFlowNode_AudioSwitch(SActivationInfo* pActInfo)
        : m_nCurrentState(0)
        , m_nSwitchID(INVALID_AUDIO_CONTROL_ID)
    {
        // sanity checks
        AZ_Assert((CFlowNode_AudioSwitch::eIn_SwitchStateNameLast - CFlowNode_AudioSwitch::eIn_SwitchStateNameFirst) == (NUM_STATES - 1),
            "CFlowNode_AudioSwitch - Number of SwitchState ports is different than expected!");
        AZ_Assert((CFlowNode_AudioSwitch::eIn_SetStateLast - CFlowNode_AudioSwitch::eIn_SetStateFirst) == (NUM_STATES - 1),
            "CFlowNode_AudioSwitch - Number of SetState ports is different than expected!");

        for (uint32 i = 0; i < NUM_STATES; ++i)
        {
            m_aSwitchStates[i] = INVALID_AUDIO_SWITCH_STATE_ID;
        }
    }

    //////////////////////////////////////////////////////////////////////////////////////////////////
    ~CFlowNode_AudioSwitch() override
    {}

    //////////////////////////////////////////////////////////////////////////////////////////////////
    IFlowNodePtr Clone(SActivationInfo* pActInfo) override
    {
        return new CFlowNode_AudioSwitch(pActInfo);
    }

    //////////////////////////////////////////////////////////////////////////////////////////////////
    enum INPUTS
    {
        eIn_SwitchName,

        eIn_SwitchStateNameFirst,
        eIn_SwitchStateName2,
        eIn_SwitchStateName3,
        eIn_SwitchStateNameLast,

        eIn_SetStateFirst,
        eIn_SetState2,
        eIn_SetState3,
        eIn_SetStateLast,
    };

    //////////////////////////////////////////////////////////////////////////////////////////////////
    enum OUTPUTS
    {
    };

    //////////////////////////////////////////////////////////////////////////////////////////////////
    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig inputs[] =
        {
            InputPortConfig<string>("audioSwitch_SwitchName", _HELP("name of the Audio Switch"), "Switch"),
            InputPortConfig<string>("audioSwitchState_SwitchStateName1", _HELP("name of a Switch State"), "State1"),
            InputPortConfig<string>("audioSwitchState_SwitchStateName2", _HELP("name of a Switch State"), "State2"),
            InputPortConfig<string>("audioSwitchState_SwitchStateName3", _HELP("name of a Switch State"), "State3"),
            InputPortConfig<string>("audioSwitchState_SwitchStateName4", _HELP("name of a Switch State"), "State4"),

            InputPortConfig_Void("audioSwitchState_SetState1", _HELP("Sets the switch to the corresponding state"), "SetState1"),
            InputPortConfig_Void("audioSwitchState_SetState2", _HELP("Sets the switch to the corresponding state"), "SetState2"),
            InputPortConfig_Void("audioSwitchState_SetState3", _HELP("Sets the switch to the corresponding state"), "SetState3"),
            InputPortConfig_Void("audioSwitchState_SetState4", _HELP("Sets the switch to the corresponding state"), "SetState4"),
            {0}
        };

        static const SOutputPortConfig outputs[] =
        {
            {0}
        };

        config.pInputPorts = inputs;
        config.pOutputPorts = outputs;
        config.sDescription = _HELP("This node allows one to set Audio Switches.");
        config.nFlags |= EFLN_TARGET_ENTITY;
        config.SetCategory(EFLN_APPROVED);
    }

    //////////////////////////////////////////////////////////////////////////////////////////////////
    void Serialize(SActivationInfo* pActInfo, TSerialize ser) override
    {
        uint32 nCurrentState = m_nCurrentState;
        ser.BeginGroup("FlowAudioSwitchNode");
        ser.Value("current_state", nCurrentState);
        ser.EndGroup();

        if (ser.IsReading())
        {
            m_nCurrentState = 0;
            Init(pActInfo, nCurrentState);
        }
    }

    //////////////////////////////////////////////////////////////////////////////////////////////////
    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        switch (event)
        {
        case eFE_Initialize:
        {
            Init(pActInfo, 0);
            break;
        }
        case eFE_Activate:
        {
            if (IsPortActive(pActInfo, CFlowNode_AudioSwitch::eIn_SwitchName))
            {
                GetSwitchID(pActInfo);
            }

            for (uint32 iStateName = CFlowNode_AudioSwitch::eIn_SwitchStateNameFirst; iStateName <= CFlowNode_AudioSwitch::eIn_SwitchStateNameLast; ++iStateName)
            {
                if (IsPortActive(pActInfo, iStateName))
                {
                    GetSwitchStateID(pActInfo, iStateName);
                }
            }

            for (uint32 iStatePort = CFlowNode_AudioSwitch::eIn_SetStateFirst; iStatePort <= CFlowNode_AudioSwitch::eIn_SetStateLast; ++iStatePort)
            {
                if (IsPortActive(pActInfo, iStatePort))
                {
                    SetState(pActInfo->pEntity, iStatePort - CFlowNode_AudioSwitch::eIn_SetStateFirst + 1);
                    break;    // short-circuit behaviour: set the first state activated and stop
                }
            }

            break;
        }
        }
    }

    //////////////////////////////////////////////////////////////////////////////////////////////////
    void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }

private:

    static const int NUM_STATES = 4;

    //////////////////////////////////////////////////////////////////////////////////////////////////
    void GetSwitchID(SActivationInfo* const pActInfo)
    {
        m_nSwitchID = INVALID_AUDIO_CONTROL_ID;
        const string& rSwitchName = GetPortString(pActInfo, eIn_SwitchName);
        if (!rSwitchName.empty())
        {
            Audio::AudioSystemRequestBus::BroadcastResult(m_nSwitchID, &Audio::AudioSystemRequestBus::Events::GetAudioSwitchID, rSwitchName.c_str());
        }
    }

    //////////////////////////////////////////////////////////////////////////////////////////////////
    void GetSwitchStateID(SActivationInfo* const pActInfo, const uint32 nStateInputIdx)
    {
        m_aSwitchStates[nStateInputIdx - eIn_SwitchStateNameFirst] = INVALID_AUDIO_CONTROL_ID;
        const string& rStateName = GetPortString(pActInfo, nStateInputIdx);
        if (!rStateName.empty() && m_nSwitchID != INVALID_AUDIO_CONTROL_ID)
        {
            Audio::AudioSystemRequestBus::BroadcastResult(m_aSwitchStates[nStateInputIdx - eIn_SwitchStateNameFirst],
                &Audio::AudioSystemRequestBus::Events::GetAudioSwitchStateID,
                m_nSwitchID,
                rStateName.c_str());
        }
    }

    //////////////////////////////////////////////////////////////////////////////////////////////////
    void Init(SActivationInfo* const pActInfo, const uint32 nCurrentState)
    {
        GetSwitchID(pActInfo);

        if (m_nSwitchID != INVALID_AUDIO_CONTROL_ID)
        {
            for (uint32 iStateName = CFlowNode_AudioSwitch::eIn_SwitchStateNameFirst; iStateName <= CFlowNode_AudioSwitch::eIn_SwitchStateNameLast; ++iStateName)
            {
                GetSwitchStateID(pActInfo, iStateName);
            }
        }

        m_oRequest.pData = &m_oRequestData;
        m_oRequestData.nSwitchID = m_nSwitchID;

        m_nCurrentState = 0;

        if (nCurrentState != 0)
        {
            SetState(pActInfo->pEntity, nCurrentState);
        }
    }

    //////////////////////////////////////////////////////////////////////////////////////////////////
    void SetStateOnProxy(IEntity* const pEntity, const uint32 nStateIdx)
    {
        IComponentAudioPtr pAudioComponent = pEntity->GetOrCreateComponent<IComponentAudio>();
        if (pAudioComponent)
        {
            pAudioComponent->SetSwitchState(m_nSwitchID, m_aSwitchStates[nStateIdx]);
        }
    }

    //////////////////////////////////////////////////////////////////////////////////////////////////
    void SetStateOnGlobalObject(const uint32 nStateIdx)
    {
        m_oRequestData.nStateID = m_aSwitchStates[nStateIdx];
        Audio::AudioSystemRequestBus::Broadcast(&Audio::AudioSystemRequestBus::Events::PushRequest, m_oRequest);
    }

    //////////////////////////////////////////////////////////////////////////////////////////////////
    void SetState(IEntity* const pEntity, const int nNewState)
    {
        AZ_Assert((0 < nNewState) && (nNewState <= NUM_STATES), "SetState - Audio State index is out of range!");

        // Cannot check for m_nCurrentState != nNewState, because there can be several flowgraph nodes
        // setting the states on the same switch. This particular node might need to set the same state again,
        // if another node has set a different one in between the calls to set the state on this node.
        m_nCurrentState = static_cast<uint32>(nNewState);

        if (pEntity)
        {
            SetStateOnProxy(pEntity, m_nCurrentState - 1);
        }
        else
        {
            SetStateOnGlobalObject(m_nCurrentState - 1);
        }
    }

    //////////////////////////////////////////////////////////////////////////////////////////////////
    uint32 m_nCurrentState;

    Audio::TAudioControlID m_nSwitchID;
    Audio::TAudioSwitchStateID m_aSwitchStates[NUM_STATES];

    Audio::SAudioRequest m_oRequest;
    Audio::SAudioObjectRequestData<Audio::eAORT_SET_SWITCH_STATE> m_oRequestData;
};

REGISTER_FLOW_NODE("Audio:Switch", CFlowNode_AudioSwitch);
