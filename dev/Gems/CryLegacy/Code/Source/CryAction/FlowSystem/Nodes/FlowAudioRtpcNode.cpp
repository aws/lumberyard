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

// Description: FlowGraph Node that sets RTPC values.


#include "CryLegacy_precompiled.h"
#include <FlowSystem/Nodes/FlowBaseNode.h>
#include "Components/IComponentAudio.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
class CFlowNode_AudioRtpc
    : public CFlowBaseNode<eNCT_Instanced>
{
public:
    ///////////////////////////////////////////////////////////////////////////////////////////////
    CFlowNode_AudioRtpc(SActivationInfo* pActInfo)
        : m_fValue(0.0f)
        , m_nRtpcID(INVALID_AUDIO_CONTROL_ID)
    {}

    ///////////////////////////////////////////////////////////////////////////////////////////////
    ~CFlowNode_AudioRtpc() override
    {}

    ///////////////////////////////////////////////////////////////////////////////////////////////
    IFlowNodePtr Clone(SActivationInfo* pActInfo) override
    {
        return new CFlowNode_AudioRtpc(pActInfo);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    enum INPUTS
    {
        eIn_RtpcName,
        eIn_RtpcValue,
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////
    enum OUTPUTS
    {
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////
    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig inputs[] =
        {
            InputPortConfig<string>("audioRTPC_Name", _HELP("RTPC name"), "Name"),
            InputPortConfig<float>("value", _HELP("RTPC value"), "Value"),
            {0}
        };

        static const SOutputPortConfig outputs[] =
        {
            {0}
        };

        config.pInputPorts = inputs;
        config.pOutputPorts = outputs;
        config.sDescription = _HELP("This node sets RTPC values.");
        config.nFlags |= EFLN_TARGET_ENTITY;
        config.SetCategory(EFLN_APPROVED);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    void Serialize(SActivationInfo* pActInfo, TSerialize ser) override
    {
        float fValue = m_fValue;
        ser.BeginGroup("FlowAudioRtpcNode");
        ser.Value("value", fValue);
        ser.EndGroup();

        if (ser.IsReading())
        {
            SetValue(pActInfo->pEntity, fValue);
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        switch (event)
        {
        case eFE_Initialize:
        {
            Init(pActInfo);
            break;
        }
        case eFE_Activate:
        {
            if (IsPortActive(pActInfo, CFlowNode_AudioRtpc::eIn_RtpcValue))
            {
                SetValue(pActInfo->pEntity, GetPortFloat(pActInfo, CFlowNode_AudioRtpc::eIn_RtpcValue));
            }

            if (IsPortActive(pActInfo, CFlowNode_AudioRtpc::eIn_RtpcName))
            {
                GetRtpcID(pActInfo);
            }

            break;
        }
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }

private:

    ///////////////////////////////////////////////////////////////////////////////////////////////
    void GetRtpcID(SActivationInfo* const pActInfo)
    {
        m_nRtpcID = INVALID_AUDIO_CONTROL_ID;
        const string& rRtpcName = GetPortString(pActInfo, CFlowNode_AudioRtpc::eIn_RtpcName);
        if (!rRtpcName.empty())
        {
            Audio::AudioSystemRequestBus::BroadcastResult(m_nRtpcID, &Audio::AudioSystemRequestBus::Events::GetAudioRtpcID, rRtpcName.c_str());
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    void SetValue(IEntity* const pEntity, const float fValue)
    {
        // Don't optimize here!
        // We always need to set the value as it could have been manipulated by another entity.
        m_fValue = fValue;

        if (pEntity)
        {
            SetOnProxy(pEntity, m_fValue);
        }
        else
        {
            SetOnGlobalObject(m_fValue);
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    void Init(SActivationInfo* const pActInfo)
    {
        GetRtpcID(pActInfo);

        m_oRequest.pData = &m_oRequestData;
        m_oRequestData.nControlID = m_nRtpcID;

        float fValue = GetPortFloat(pActInfo, eIn_RtpcValue);
        SetValue(pActInfo->pEntity, fValue);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    void SetOnProxy(IEntity* const pEntity, const float fValue)
    {
        IComponentAudioPtr pAudioComponent = pEntity->GetOrCreateComponent<IComponentAudio>();

        if (pAudioComponent)
        {
            pAudioComponent->SetRtpcValue(m_nRtpcID, fValue);
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    void SetOnGlobalObject(float const fValue)
    {
        m_oRequestData.fValue = fValue;
        Audio::AudioSystemRequestBus::Broadcast(&Audio::AudioSystemRequestBus::Events::PushRequest, m_oRequest);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    float m_fValue;
    Audio::TAudioControlID m_nRtpcID;

    Audio::SAudioRequest m_oRequest;
    Audio::SAudioObjectRequestData<Audio::eAORT_SET_RTPC_VALUE> m_oRequestData;
};

REGISTER_FLOW_NODE("Audio:Rtpc", CFlowNode_AudioRtpc);
