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

#ifndef CRYINCLUDE_CRYACTION_FLOWSYSTEM_NODES_FLOWDELAYNODE_H
#define CRYINCLUDE_CRYACTION_FLOWSYSTEM_NODES_FLOWDELAYNODE_H
#pragma once

#include <queue>
#include <FlowSystem/Nodes/FlowBaseNode.h>

class CFlowDelayNode
    : public CFlowBaseNode<eNCT_Instanced>
{
public:
    CFlowDelayNode(SActivationInfo*);
    virtual ~CFlowDelayNode();

    // IFlowNode
    virtual IFlowNodePtr Clone(SActivationInfo*);
    virtual void GetConfiguration(SFlowNodeConfig&);
    virtual void ProcessEvent(EFlowEvent event, SActivationInfo*);
    virtual void Serialize(SActivationInfo*, TSerialize ser);

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
        s->AddObject(m_activations);
    }
    // ~IFlowNode

protected:
    // let derived classes decide how long to delay
    virtual float GetDelayTime(SActivationInfo*) const;
    virtual bool GetShouldReset(SActivationInfo* pActInfo);

private:
    void RemovePendingTimers();

    enum
    {
        INP_IN = 0,
        INP_DELAY,
        INP_RESET_ON_EACH_INPUT
    };

    SActivationInfo m_actInfo;
    struct SDelayData
    {
        SDelayData() {}
        SDelayData(const CTimeValue& timeout, const TFlowInputData& data)
            : m_timeout(timeout)
            , m_data(data) {}

        CTimeValue m_timeout;
        TFlowInputData m_data;
        bool operator<(const SDelayData& rhs) const
        {
            return m_timeout < rhs.m_timeout;
        }

        void Serialize(TSerialize ser)
        {
            ser.Value("m_timeout", m_timeout);
            ser.Value("m_data", m_data);
        }

        void GetMemoryUsage(ICrySizer* pSizer) const { /*nothing*/}
    };

    typedef std::map<IGameFramework::TimerID, SDelayData> Activations;
    Activations m_activations;
    static void OnTimer(void* pUserData, IGameFramework::TimerID ref);
};

#endif // CRYINCLUDE_CRYACTION_FLOWSYSTEM_NODES_FLOWDELAYNODE_H
