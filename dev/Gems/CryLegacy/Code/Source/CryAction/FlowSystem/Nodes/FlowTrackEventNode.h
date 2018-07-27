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

// Description : Dynamic node for Track Event logic


#ifndef CRYINCLUDE_CRYACTION_FLOWSYSTEM_NODES_FLOWTRACKEVENTNODE_H
#define CRYINCLUDE_CRYACTION_FLOWSYSTEM_NODES_FLOWTRACKEVENTNODE_H
#pragma once

#include <FlowSystem/Nodes/FlowBaseNode.h>
#include <IMovieSystem.h>

class CFlowTrackEventNode
    : public CFlowBaseNode<eNCT_Instanced>
    , public ITrackEventListener
{
public:
    CFlowTrackEventNode(SActivationInfo*);
    CFlowTrackEventNode(CFlowTrackEventNode const& obj);
    CFlowTrackEventNode& operator =(CFlowTrackEventNode const& obj);
    virtual ~CFlowTrackEventNode();

    // IFlowNode
    virtual void AddRef();
    virtual void Release();
    virtual IFlowNodePtr Clone(SActivationInfo* pActInfo);
    virtual void GetConfiguration(SFlowNodeConfig& config);
    virtual void ProcessEvent(EFlowEvent event, SActivationInfo*);
    virtual bool SerializeXML(SActivationInfo* pActInfo, const XmlNodeRef& root, bool reading);
    virtual void Serialize(SActivationInfo*, TSerialize ser);

    // ~ITrackEventListener
    virtual void OnTrackEvent(IAnimSequence* pSequence, int reason, const char* event, void* pUserData);

    void GetMemoryUsage(class ICrySizer* pSizer) const
    {
        pSizer->AddObject(this, sizeof(*this));
        pSizer->AddObject(m_outputStrings);
    }
protected:
    // Add to sequence listener
    void AddListener(SActivationInfo* pActInfo);

private:
    typedef std::vector<string> StrArray;

    int m_refs;
    int m_nOutputs;
    StrArray m_outputStrings;
    SOutputPortConfig* m_outputs;

    SActivationInfo m_actInfo;
    IAnimSequence* m_pSequence;
};

#endif // CRYINCLUDE_CRYACTION_FLOWSYSTEM_NODES_FLOWTRACKEVENTNODE_H
