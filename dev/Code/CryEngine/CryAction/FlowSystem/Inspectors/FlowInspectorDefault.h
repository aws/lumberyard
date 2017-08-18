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

// Description : A FlowGraph inspector which draws information of all
//               flowgraphs as 2D overlay


#ifndef CRYINCLUDE_CRYACTION_FLOWSYSTEM_INSPECTORS_FLOWINSPECTORDEFAULT_H
#define CRYINCLUDE_CRYACTION_FLOWSYSTEM_INSPECTORS_FLOWINSPECTORDEFAULT_H
#pragma once

#include "IFlowSystem.h"

class CFlowInspectorDefault
    : public IFlowGraphInspector
{
public:
    CFlowInspectorDefault(IFlowSystem* pFlowSystem);
    virtual ~CFlowInspectorDefault();

    // IFlowGraphInspector interface
    virtual void AddRef();
    virtual void Release();
    virtual void PreUpdate(IFlowGraph* pGraph);    // called once per frame before IFlowSystem::Update
    virtual void PostUpdate(IFlowGraph* pGraph);   // called once per frame after  IFlowSystem::Update
    virtual void NotifyFlow(IFlowGraph* pGraph, const SFlowAddress from, const SFlowAddress to);
    virtual void NotifyProcessEvent(IFlowNode::EFlowEvent event, IFlowNode::SActivationInfo* pActInfo, IFlowNode* pImpl);
    virtual void AddFilter(IFlowGraphInspector::IFilterPtr pFilter);
    virtual void RemoveFilter(IFlowGraphInspector::IFilterPtr pFilter);
    virtual void GetMemoryUsage(ICrySizer* s) const;
    // ~IFlowGraphInspector interface

    enum ERecType
    {
        eRTUnknown,
        eRTEntity,
        eRTAction,
        eRTLast
    };

    struct TFlowRecord
    {
        IFlowGraph*    m_pGraph;         // only valid during NotifyFlow, afterwards DON'T de-reference!
        SFlowAddress   m_from;
        SFlowAddress   m_to;
        TFlowInputData m_data;
        CTimeValue     m_tstamp;
        ERecType       m_type;
        string         m_message;        // complete message with graph, action, nodes, ports, value

        bool operator == (const TFlowRecord& lhs) const { const TFlowRecord& rhs = *this; return lhs.m_from == rhs.m_from && lhs.m_to == rhs.m_to && lhs.m_pGraph == rhs.m_pGraph; }
        bool operator !=(const TFlowRecord& rc) const { return !(*this == rc); }

        void GetMemoryUsage(ICrySizer* pSizer) const{}
    };

protected:
    virtual void UpdateRecords();
    virtual void DrawRecords(const std::deque<TFlowRecord>& inRecords, int inBaseRow, float inMaxAge) const;
    void DrawLabel(float col, float row, const ColorF& color, float glow, const char* szText, float fScale = 1.0f) const;
    // return true if flow should be recorded, false otherwise
    bool RunFilters(IFlowGraph* pGraph, const SFlowAddress from, const SFlowAddress to);

    IRenderer*   m_pRenderer;
    IFlowSystem* m_pFlowSystem;
    int          m_refs;
    int          m_newOneTime;
    int          m_newCont;
    std::vector<TFlowRecord> m_curRecords;
    std::deque<TFlowRecord>  m_oneTimeRecords;  // all records which occurred once updated every frame
    std::deque<TFlowRecord>  m_contRecords; // records which already occurred are put into here
    IFilter_AutoArray m_filters;
    CTimeValue   m_currentTime;
    bool         m_bProcessing;
    bool         m_bPaused;
};

#endif // CRYINCLUDE_CRYACTION_FLOWSYSTEM_INSPECTORS_FLOWINSPECTORDEFAULT_H
