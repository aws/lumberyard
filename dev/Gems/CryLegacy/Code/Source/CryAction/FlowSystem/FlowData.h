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

#ifndef CRYINCLUDE_CRYACTION_FLOWSYSTEM_FLOWDATA_H
#define CRYINCLUDE_CRYACTION_FLOWSYSTEM_FLOWDATA_H
#pragma once

#include "IFlowSystem.h"

#if 0
#define FLOWGRAPH_BIT_VAR(type, name, bits) type name
#else
#define FLOWGRAPH_BIT_VAR(type, name, bits) type name: bits
#endif

class CFlowData
    : public IFlowNodeData
{
public:
    static const int TYPE_BITS = 12;
    static const int TYPE_MAX_COUNT = 1 << TYPE_BITS;

    CFlowData(IFlowNodePtr pImpl, const string& name, TFlowNodeTypeId type);
    CFlowData();
    ~CFlowData();
    CFlowData(const CFlowData&);
    void Swap(CFlowData&);
    CFlowData& operator=(const CFlowData& rhs);

    ILINE int GetNumOutputs() const { return m_nOutputs; }
    ILINE void SetOutputFirstEdge(int output, int edge) { m_pOutputFirstEdge[output] = edge; }
    ILINE int GetOutputFirstEdge(int output) const { return m_pOutputFirstEdge[output]; }

    // activation of ports with data
    template <class T>
    ILINE bool ActivateInputPort(TFlowPortId port, const T& value)
    {
        TFlowInputData* pPort = m_pInputData + port;
        pPort->SetUserFlag(true);
        return pPort->SetValueWithConversion(value);
    }

    ILINE bool SetInputPort(TFlowPortId port, const TFlowInputData& value)
    {
        TFlowInputData* pPort = m_pInputData + port;
        return pPort->SetValueWithConversion(value);
    }
    TFlowInputData* GetInputPort(TFlowPortId port)
    {
        TFlowInputData* pPort = m_pInputData + port;
        return pPort;
    }

    void FlagInputPorts();

    // perform a single activation round
    void Activated(IFlowNode::SActivationInfo*, IFlowNode::EFlowEvent);
    // perform a single update round (called if we have called SetRegularlyUpdated(id,true);
    void Update(IFlowNode::SActivationInfo*);
    // sends Suspended/Resumed events
    void SetSuspended(IFlowNode::SActivationInfo*, bool suspended);
    // resolve a port identifier
    bool ResolvePort(const char* name, TFlowPortId& port, bool isOutput);
    bool SerializeXML(IFlowNode::SActivationInfo*, const XmlNodeRef& node, bool reading);
    void Serialize(IFlowNode::SActivationInfo*, TSerialize ser);
    void PostSerialize(IFlowNode::SActivationInfo*);

    void CloneImpl(IFlowNode::SActivationInfo* pActInfo)
    {
        m_pImpl = m_pImpl->Clone(pActInfo);
    }

    bool IsValid() const
    {
        return m_pImpl != NULL;
    }

    bool ValidatePort(TFlowPortId id, bool isOutput) const
    {
        if (isOutput)
        {
            return id < m_nOutputs;
        }
        else
        {
            return id < m_nInputs;
        }
    }

    const IFlowNodePtr& GetImpl() const
    {
        return m_pImpl;
    }

    TFlowNodeTypeId GetTypeId() const { return m_typeId; }
    const string& GetName() const { return m_name; }
    void SetName(const string& name) { m_name = name; }

    const char* GetPortName(TFlowPortId port, bool output) const
    {
        SFlowNodeConfig config;
        DoGetConfiguration(config);
        return output ? config.pOutputPorts[port].name : config.pInputPorts[port].name;
    }

    //////////////////////////////////////////////////////////////////////////
    // IFlowNodeData
    //////////////////////////////////////////////////////////////////////////
    virtual void GetConfiguration(SFlowNodeConfig& config);
    virtual TFlowNodeTypeId GetNodeTypeId() { return m_typeId; }
    virtual const char* GetNodeName() { return m_name.c_str(); }
    virtual IFlowNode* GetNode()
    {
        return &*GetImpl();
    }
    virtual int GetNumInputPorts() const { return m_nInputs; }
    virtual int GetNumOutputPorts() const { return m_nOutputs; }
    virtual EntityId GetCurrentForwardingEntity() const { return m_forwardingEntityID; }
    //////////////////////////////////////////////////////////////////////////
    // ~IFlowNodeData
    //////////////////////////////////////////////////////////////////////////

    virtual bool SetEntityId(const FlowEntityId id);

    FlowEntityId GetEntityId() override
    {
        FlowEntityId id(FlowEntityId::s_invalidFlowEntityID);
        if (m_hasEntity)
        {
            FlowEntityId* pTemp = m_pInputData[0].GetPtr<FlowEntityId>();
            if (pTemp)
            {
                id = *pTemp;
            }
        }
        return id;
    }

    void CompleteActivationInfo(IFlowNode::SActivationInfo*);

    void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject((char*)m_pInputData - 4, (sizeof(*m_pInputData) * m_nInputs) + 4);

        for (int i = 0; i < m_nInputs; i++)
        {
            pSizer->AddObject(m_pInputData[i]);
        }
    }
    bool DoForwardingIfNeed(IFlowNode::SActivationInfo*);


private:
    // REMEMBER: When adding members update CFlowData::Swap(CFlowData&)
    //                                                                                                                                                                32 Bit        64 Bit
    TFlowInputData* m_pInputData;                                                                                       // ptr          4                           8
    int* m_pOutputFirstEdge;                                                                                                // ptr          4                           8
    IFlowNodePtr m_pImpl;                                                                                                       // int+ptr  8 (4+4)             12 (4+8)
    string m_name;                                                                                                                  //                  4                           8
    HSCRIPTFUNCTION m_getFlowgraphForwardingEntity;                                                 // ptr          4                           8
    uint8  m_nInputs;                                                                                                           // uint8        1                           1
    uint8  m_nOutputs;                                                                                                          // uint8        1                           1
    EntityId m_forwardingEntityID;                                                                                  // uint32       4
    FLOWGRAPH_BIT_VAR(uint16, m_typeId, TYPE_BITS);         // 12 Bits                  // uint16       2                           2
    FLOWGRAPH_BIT_VAR(uint16, m_hasEntity, 1);                                                          // ..
    FLOWGRAPH_BIT_VAR(uint16, m_failedGettingFlowgraphForwardingEntity, 1); // ..
    FLOWGRAPH_BIT_VAR(uint16, m_reserved, 1);                                                               // ..
    //                                                                                                                                          ------------------------------
    //                                                                                                                                                                 32 Bytes    48 Bytes

    void DoGetConfiguration(SFlowNodeConfig& config) const;
    void CountConfigurationPorts(const SFlowNodeConfig& config, int& numInputs, int& numOutputs) const;
    bool ForwardingActivated(IFlowNode::SActivationInfo*, IFlowNode::EFlowEvent);
    bool NoForwarding(IFlowNode::SActivationInfo*);
    static ILINE bool HasForwarding(IFlowNode::SActivationInfo*) { return true; }
    void ClearInputActivations();
};

ILINE void CFlowData::ClearInputActivations()
{
    for (int i = 0; i < m_nInputs; i++)
    {
        m_pInputData[i].ClearUserFlag();
    }
}

ILINE void CFlowData::CompleteActivationInfo(IFlowNode::SActivationInfo* pActInfo)
{
    pActInfo->pInputPorts = m_pInputData + m_hasEntity;
}

ILINE void CFlowData::Activated(IFlowNode::SActivationInfo* pActInfo, IFlowNode::EFlowEvent event)
{
    //  FRAME_PROFILER( m_type.c_str(), GetISystem(), PROFILE_ACTION );
    if (m_hasEntity && (m_getFlowgraphForwardingEntity || m_failedGettingFlowgraphForwardingEntity))
    {
        if (ForwardingActivated(pActInfo, event))
        {
            ClearInputActivations();
            return;
        }
    }

    CompleteActivationInfo(pActInfo);
    if (m_hasEntity && m_pInputData[0].IsUserFlagSet())
    {
        m_pImpl->ProcessEvent(IFlowNode::eFE_SetEntityId, pActInfo);
    }

    m_pImpl->ProcessEvent(event, pActInfo);

    // now clear any ports
    ClearInputActivations();
}

ILINE void CFlowData::Update(IFlowNode::SActivationInfo* pActInfo)
{
    //  FRAME_PROFILER( m_type.c_str(), GetISystem(), PROFILE_ACTION );
    CompleteActivationInfo(pActInfo);
    if (m_hasEntity && (m_getFlowgraphForwardingEntity || m_failedGettingFlowgraphForwardingEntity))
    {
        if (ForwardingActivated(pActInfo, IFlowNode::eFE_Update))
        {
            return;
        }
    }
    m_pImpl->ProcessEvent(IFlowNode::eFE_Update, pActInfo);
}

ILINE void CFlowData::SetSuspended(IFlowNode::SActivationInfo* pActInfo, bool suspended)
{
    CompleteActivationInfo(pActInfo);
    m_pImpl->ProcessEvent(suspended ? IFlowNode::eFE_Suspend : IFlowNode::eFE_Resume, pActInfo);
}

#endif // CRYINCLUDE_CRYACTION_FLOWSYSTEM_FLOWDATA_H
