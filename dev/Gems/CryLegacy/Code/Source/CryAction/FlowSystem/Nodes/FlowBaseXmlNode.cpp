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

// Description : Flowgraph nodes to read/write Xml files


#include "CryLegacy_precompiled.h"
#include "FlowBaseXmlNode.h"

CGraphDocManager* CGraphDocManager::m_instance = NULL;
CGraphDocManager* GDM = NULL;

////////////////////////////////////////////////////
CFlowXmlNode_Base::CFlowXmlNode_Base(SActivationInfo* pActInfo)
    : m_initialized(false)
{
}

////////////////////////////////////////////////////
CFlowXmlNode_Base::~CFlowXmlNode_Base(void)
{
    if (GDM == NULL)
    {
        return;
    }

    if (m_actInfo.pGraph == NULL)
    {
        return;
    }

    GDM->DeleteXmlDocument(m_actInfo.pGraph);
}

////////////////////////////////////////////////////
void CFlowXmlNode_Base::ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
{
    m_actInfo = *pActInfo;
    if (eFE_Initialize == event && !m_initialized)
    {
        if (GDM == NULL)
        {
            CGraphDocManager::Create();
        }

        m_initialized = true;
        PREFAST_SUPPRESS_WARNING(6011); // gets initialized above if NULL
        GDM->MakeXmlDocument(pActInfo->pGraph);
    }
    else if (eFE_Activate == event && IsPortActive(pActInfo, EIP_Execute))
    {
        const bool bResult = Execute(pActInfo);
        if (bResult)
        {
            ActivateOutput(pActInfo, EOP_Success, true);
        }
        else
        {
            ActivateOutput(pActInfo, EOP_Fail, true);
        }
        ActivateOutput(pActInfo, EOP_Done, bResult);
    }
}

////////////////////////////////////////////////////
////////////////////////////////////////////////////

////////////////////////////////////////////////////
CGraphDocManager::CGraphDocManager()
{
}

////////////////////////////////////////////////////
CGraphDocManager::~CGraphDocManager()
{
}

////////////////////////////////////////////////////
void CGraphDocManager::Create()
{
    if (!m_instance)
    {
        m_instance = new CGraphDocManager;
        GDM = m_instance;
    }
}

////////////////////////////////////////////////////
CGraphDocManager* CGraphDocManager::Get()
{
    return m_instance;
}

////////////////////////////////////////////////////
void CGraphDocManager::MakeXmlDocument(IFlowGraph* pGraph)
{
    GraphDocMap::iterator graph = m_GraphDocMap.find(pGraph);
    if (graph == m_GraphDocMap.end())
    {
        SXmlDocument doc;
        doc.root = doc.active = NULL;
        doc.refCount = 1;
        m_GraphDocMap[pGraph] = doc;
    }
    else
    {
        ++graph->second.refCount;
    }
}

////////////////////////////////////////////////////
void CGraphDocManager::DeleteXmlDocument(IFlowGraph* pGraph)
{
    GraphDocMap::iterator graph = m_GraphDocMap.find(pGraph);
    if (graph != m_GraphDocMap.end() && --graph->second.refCount <= 0)
    {
        m_GraphDocMap.erase(graph);
    }
}

////////////////////////////////////////////////////
bool CGraphDocManager::GetXmlDocument(IFlowGraph* pGraph, SXmlDocument** document)
{
    bool bResult = false;

    GraphDocMap::iterator graph = m_GraphDocMap.find(pGraph);
    if (document && graph != m_GraphDocMap.end())
    {
        *document = &(graph->second);
        bResult = true;
    }

    return bResult;
}
