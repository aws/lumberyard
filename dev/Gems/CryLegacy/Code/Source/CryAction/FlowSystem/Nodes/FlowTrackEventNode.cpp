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

#include "CryLegacy_precompiled.h"
#include "FlowTrackEventNode.h"

CFlowTrackEventNode::CFlowTrackEventNode(SActivationInfo* pActInfo)
    : m_refs(0)
    , m_pSequence(NULL)
    , m_nOutputs(1)
{
    m_outputs = new SOutputPortConfig[1];
    m_outputs[0] = SOutputPortConfig();
}

CFlowTrackEventNode::~CFlowTrackEventNode()
{
    SAFE_DELETE_ARRAY(m_outputs);
    if (NULL != m_pSequence && false == gEnv->IsEditor())
    {
        m_pSequence->RemoveTrackEventListener(this);
        m_pSequence = NULL;
    }
}

CFlowTrackEventNode::CFlowTrackEventNode(CFlowTrackEventNode const& obj)
    : m_refs(0)
    , m_pSequence(NULL)
    , m_outputs(NULL)
{
    *this = obj;
}

CFlowTrackEventNode& CFlowTrackEventNode::operator =(CFlowTrackEventNode const& obj)
{
    if (this != &obj)
    {
        m_refs = 0; // New reference count
        m_pSequence = obj.m_pSequence;

        // Copy outputs
        m_nOutputs = obj.m_nOutputs;
        SAFE_DELETE_ARRAY(m_outputs);
        m_outputs = new SOutputPortConfig[m_nOutputs];
        for (int i = 0; i < m_nOutputs; ++i)
        {
            m_outputs[i] = obj.m_outputs[i];
        }

        m_outputStrings = obj.m_outputStrings;
    }
    return *this;
}

void CFlowTrackEventNode::AddRef()
{
    ++m_refs;
}

void CFlowTrackEventNode::Release()
{
    if (0 == --m_refs)
    {
        delete this;
    }
}

IFlowNodePtr CFlowTrackEventNode::Clone(SActivationInfo* pActInfo)
{
    CFlowTrackEventNode* pClone = new CFlowTrackEventNode(*this);
    return pClone;
}

void CFlowTrackEventNode::GetConfiguration(SFlowNodeConfig& config)
{
    static const SInputPortConfig inputs[] = {
        // Note: Must be first!
        InputPortConfig<string>("seq_Sequence", "", _HELP("Working animation sequence"), _HELP("Sequence"), 0),
        InputPortConfig<int>("seqid_SequenceId", 0, _HELP("Working animation sequence"), _HELP("SequenceId"), 0),
        {0}
    };

    config.pInputPorts = inputs;
    config.pOutputPorts = m_outputs;
    config.SetCategory(EFLN_APPROVED);
    config.nFlags |= EFLN_DYNAMIC_OUTPUT;
    config.nFlags |= EFLN_HIDE_UI;
}

void CFlowTrackEventNode::ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
{
    if (event == eFE_Initialize && false == gEnv->IsEditor())
    {
        AddListener(pActInfo);
    }
}

bool CFlowTrackEventNode::SerializeXML(SActivationInfo* pActInfo, const XmlNodeRef& root, bool reading)
{
    if (true == reading)
    {
        int count = root->getChildCount();

        // Resize
        if (m_outputs)
        {
            SAFE_DELETE_ARRAY(m_outputs);
        }
        m_outputs = new SOutputPortConfig[count + 1];
        m_nOutputs = count + 1;

        for (int i = 0; i < count; ++i)
        {
            XmlNodeRef child = root->getChild(i);
            m_outputStrings.push_back(child->getAttr("Name"));
            m_outputs[i] = OutputPortConfig<string>(m_outputStrings[i]);
        }
        m_outputs[count] = SOutputPortConfig();
    }
    return true;
}

void CFlowTrackEventNode::Serialize(SActivationInfo* pActInfo, TSerialize ser)
{
    if (ser.IsReading() && false == gEnv->IsEditor())
    {
        AddListener(pActInfo);
    }
}

void CFlowTrackEventNode::AddListener(SActivationInfo* pActInfo)
{
    CRY_ASSERT(pActInfo);
    m_actInfo = *pActInfo;

    // Remove from old
    if (NULL != m_pSequence)
    {
        m_pSequence->RemoveTrackEventListener(this);
        m_pSequence = NULL;
    }

    // Look up sequence
    const int kSequenceName = 0;
    const int kSequenceId = 1;
    m_pSequence = gEnv->pMovieSystem->FindSequenceById((uint32)GetPortInt(pActInfo, kSequenceId));
    if (NULL == m_pSequence)
    {
        string name = GetPortString(pActInfo, kSequenceName);
        m_pSequence = gEnv->pMovieSystem->FindLegacySequenceByName(name.c_str());
    }
    if (NULL != m_pSequence)
    {
        m_pSequence->AddTrackEventListener(this);
    }
}

void CFlowTrackEventNode::OnTrackEvent(IAnimSequence* pSequence, int reason, const char* event, void* pUserData)
{
    if (reason != ITrackEventListener::eTrackEventReason_Triggered)
    {
        return;
    }

    // Find output port and call it
    for (int i = 0; i < m_nOutputs; ++i)
    {
        if (m_outputs[i].name && strcmp(m_outputs[i].name, event) == 0)
        {
            // Call it
            TFlowInputData value;
            const char* param = (const char*)pUserData;
            value.Set(string(param));
            ActivateOutput(&m_actInfo, i, value);
            return;
        }
    }
}
