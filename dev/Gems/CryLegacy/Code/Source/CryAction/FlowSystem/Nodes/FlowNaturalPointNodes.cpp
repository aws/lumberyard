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

class CFlowNode_TrackIR
    : public CFlowBaseNode<eNCT_Instanced>
{
public:

    CFlowNode_TrackIR(SActivationInfo* pActivationInfo)
        : m_frequency(0.0f)
    {
    }

    virtual IFlowNodePtr Clone(SActivationInfo* pActInfo) { return new CFlowNode_TrackIR(pActInfo); }

    enum EInputPorts
    {
        eIP_Sync = 0,
        eIP_Auto,
        eIP_Freq,
    };

    enum EOutputPorts
    {
        eOP_Pitch = 0,
        eOP_Yaw,
        eOP_Roll,
        eOP_X,
        eOP_Y,
        eOP_Z
    };

    void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig inputConfig[] =
        {
            InputPortConfig_Void        ("Sync", _HELP("Synchronize")),
            InputPortConfig<bool>       ("Auto", false, _HELP("Auto update")),
            InputPortConfig<float>  ("Freq", 0.0f, _HELP("Auto update frequency (0 to update every frame)")),
            { 0 }
        };

        static const SOutputPortConfig outputConfig[] =
        {
            OutputPortConfig<float> ("Pitch", _HELP("Pitch")),
            OutputPortConfig<float> ("Yaw", _HELP("Yaw")),
            OutputPortConfig<float> ("Roll", _HELP("Roll")),
            OutputPortConfig<float> ("X", _HELP("Position X")),
            OutputPortConfig<float> ("Y", _HELP("Position Y")),
            OutputPortConfig<float> ("Z", _HELP("Position Z")),
            { 0 }
        };

        config.sDescription = _HELP("Get status of joint in Kinect skeleton");
        config.pInputPorts  = inputConfig;
        config.pOutputPorts = outputConfig;

        config.SetCategory(EFLN_APPROVED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActivationInfo)
    {
        switch (event)
        {
        case eFE_Initialize:
        {
            SetAutoUpdate(pActivationInfo, GetPortBool(pActivationInfo, eIP_Auto));

            break;
        }

        case eFE_Activate:
        {
            if (IsPortActive(pActivationInfo, eIP_Auto))
            {
                SetAutoUpdate(pActivationInfo, GetPortBool(pActivationInfo, eIP_Auto));
            }
            else if (IsPortActive(pActivationInfo, eIP_Freq))
            {
                m_frequency = GetPortFloat(pActivationInfo, eIP_Freq);
            }
            else if (IsPortActive(pActivationInfo, eIP_Sync))
            {
                Sync(pActivationInfo);
            }

            break;
        }

        case eFE_Update:
        {
            CTimeValue  delta = gEnv->pTimer->GetFrameStartTime() - m_prevTime;

            if (delta.GetSeconds() >= m_frequency)
            {
                Sync(pActivationInfo);
            }

            break;
        }
        }
    }

    virtual void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->Add(*this);
    }

private:

    void SetAutoUpdate(SActivationInfo* pActivationInfo, bool enable)
    {
        pActivationInfo->pGraph->SetRegularlyUpdated(pActivationInfo->myID, enable);

        if (enable)
        {
            m_prevTime = gEnv->pTimer->GetFrameStartTime();
        }
    }

    void Sync(SActivationInfo* pActivationInfo)
    {
        if (INaturalPointInput* pNPInput = gEnv->pSystem->GetIInput()->GetNaturalPointInput())
        {
            NP_RawData currentNPData;

            if (pNPInput->GetNaturalPointData(currentNPData))
            {
                ActivateOutput(pActivationInfo, eOP_Pitch, double(currentNPData.fNPPitch));
                ActivateOutput(pActivationInfo, eOP_Yaw, double(currentNPData.fNPYaw));
                ActivateOutput(pActivationInfo, eOP_Roll, double(currentNPData.fNPRoll));
                ActivateOutput(pActivationInfo, eOP_X, double(currentNPData.fNPX));
                ActivateOutput(pActivationInfo, eOP_Y, double(currentNPData.fNPY));
                ActivateOutput(pActivationInfo, eOP_Z, double(currentNPData.fNPZ));
            }
        }

        m_prevTime = gEnv->pTimer->GetFrameStartTime();
    }

    float               m_frequency;

    CTimeValue  m_prevTime;
};

FLOW_NODE_BLACKLISTED("NaturalPoint:GetNPData", CFlowNode_TrackIR);
