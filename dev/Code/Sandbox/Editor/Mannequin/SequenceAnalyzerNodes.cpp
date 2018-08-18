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

#include "StdAfx.h"
#include "SequenceAnalyzerNodes.h"
#include "FragmentTrack.h"
#include "MannequinDialog.h"
#include "Controls/PreviewerPage.h"

#include "QtUtil.h"

const int NUM_SCOPE_PARAM_IDS = 4;
CSequencerNode::SParamInfo g_scopeParamInfo[NUM_SCOPE_PARAM_IDS] =
{
    CSequencerNode::SParamInfo("TransitionProperties", SEQUENCER_PARAM_TRANSITIONPROPS, 0),
    CSequencerNode::SParamInfo("FragmentId", SEQUENCER_PARAM_FRAGMENTID, 0),
    CSequencerNode::SParamInfo("AnimLayer", SEQUENCER_PARAM_ANIMLAYER, CSequencerNode::PARAM_MULTIPLE_TRACKS),
    CSequencerNode::SParamInfo("ProcLayer", SEQUENCER_PARAM_PROCLAYER, CSequencerNode::PARAM_MULTIPLE_TRACKS)
};

const int NUM_GLOBAL_PARAM_IDS = 2;
CSequencerNode::SParamInfo g_scopeParamInfoGlobal[NUM_GLOBAL_PARAM_IDS] =
{
    CSequencerNode::SParamInfo("TagState", SEQUENCER_PARAM_TAGS, 0),
    CSequencerNode::SParamInfo("Params", SEQUENCER_PARAM_PARAMS, 0)
};

const int MI_SET_CONTEXT_NONE = 10000;
const int MI_SET_CONTEXT_BASE = 10001;



//////////////////////////////////////////////////////////////////////////
CRootNode::CRootNode(CSequencerSequence* sequence, const SControllerDef& controllerDef)
    : CSequencerNode(sequence, controllerDef)
{
}

CRootNode::~CRootNode()
{
}


int CRootNode::GetParamCount() const
{
    return NUM_GLOBAL_PARAM_IDS;
}

bool CRootNode::GetParamInfo(int nIndex, SParamInfo& info) const
{
    if (nIndex < NUM_GLOBAL_PARAM_IDS)
    {
        info = g_scopeParamInfoGlobal[nIndex];
        return true;
    }
    return false;
}


CSequencerTrack* CRootNode::CreateTrack(ESequencerParamType nParamId)
{
    CSequencerTrack* newTrack = NULL;
    switch (nParamId)
    {
    case SEQUENCER_PARAM_TAGS:
        newTrack = new CTagTrack(m_controllerDef.m_tags);
        break;
    case SEQUENCER_PARAM_PARAMS:
        newTrack = new CParamTrack();
        break;
    }

    if (newTrack)
    {
        AddTrack(nParamId, newTrack);
    }

    return newTrack;
}


//////////////////////////////////////////////////////////////////////////



CScopeNode::CScopeNode(CSequencerSequence* sequence, SScopeData* pScopeData, EMannequinEditorMode mode)
    : CSequencerNode(sequence, *pScopeData->mannContexts->m_controllerDef)
    , m_pScopeData(pScopeData)
    , m_mode(mode)
{
    assert(m_mode == eMEM_Previewer || m_mode == eMEM_TransitionEditor);
}

void CScopeNode::InsertMenuOptions(QMenu* menu)
{
    menu->addSeparator();

    uint32 contextID = m_pScopeData->contextID;
    uint32 numContextDatas = m_pScopeData->mannContexts->m_contextData.size();
    uint32 numMatchingContexts = 0;
    for (uint32 i = 0; i < numContextDatas; i++)
    {
        if (m_pScopeData->mannContexts->m_contextData[i].contextID == contextID)
        {
            menuSetContext.addAction(m_pScopeData->mannContexts->m_contextData[i].name)->setData(MI_SET_CONTEXT_BASE + i);
            numMatchingContexts++;
        }
    }
    if ((numMatchingContexts > 0) && (contextID > 0))
    {
        menuSetContext.addAction(QObject::tr("None"))->setData(MI_SET_CONTEXT_NONE);
        numMatchingContexts++;
    }
    menuSetContext.setTitle(QObject::tr("Change Context"));
    menu->addMenu(&menuSetContext)->setEnabled(numMatchingContexts > 1);
}

void CScopeNode::ClearMenuOptions(QMenu* menu)
{
    menuSetContext.clear();
}

void CScopeNode::OnMenuOption(int menuOption)
{
    if (menuOption == MI_SET_CONTEXT_NONE)
    {
        CMannequinDialog::GetCurrentInstance()->ClearContextData(m_mode, m_pScopeData->contextID);
        if (m_mode == eMEM_Previewer)
        {
            CMannequinDialog::GetCurrentInstance()->Previewer()->SetUIFromHistory();
        }
        else if (m_mode == eMEM_TransitionEditor)
        {
            CMannequinDialog::GetCurrentInstance()->TransitionEditor()->SetUIFromHistory();
        }
    }
    else if ((menuOption >= MI_SET_CONTEXT_BASE) && ((menuOption < MI_SET_CONTEXT_BASE + 100)))
    {
        int scopeContext = menuOption - MI_SET_CONTEXT_BASE;
        CMannequinDialog::GetCurrentInstance()->EnableContextData(m_mode, scopeContext);
        if (m_mode == eMEM_Previewer)
        {
            CMannequinDialog::GetCurrentInstance()->Previewer()->SetUIFromHistory();
        }
        else if (m_mode == eMEM_TransitionEditor)
        {
            CMannequinDialog::GetCurrentInstance()->TransitionEditor()->SetUIFromHistory();
        }
    }
}


IEntity* CScopeNode::GetEntity()
{
    return m_pScopeData->context[m_mode] ? m_pScopeData->context[m_mode]->viewData[m_mode].entity : NULL;
}

int CScopeNode::GetParamCount() const
{
    return NUM_SCOPE_PARAM_IDS;
}

bool CScopeNode::GetParamInfo(int nIndex, SParamInfo& info) const
{
    if (nIndex < NUM_SCOPE_PARAM_IDS)
    {
        info = g_scopeParamInfo[nIndex];
        return true;
    }
    return false;
}


CSequencerTrack* CScopeNode::CreateTrack(ESequencerParamType nParamId)
{
    CSequencerTrack* newTrack = NULL;
    switch (nParamId)
    {
    case SEQUENCER_PARAM_ANIMLAYER:
        newTrack = new CClipTrack(m_pScopeData->context[m_mode], m_mode);
        break;
    case SEQUENCER_PARAM_PROCLAYER:
        newTrack = new CProcClipTrack(m_pScopeData->context[m_mode], m_mode);
        break;
    case SEQUENCER_PARAM_FRAGMENTID:
        newTrack = new CFragmentTrack(*m_pScopeData, m_mode);
        break;
    case SEQUENCER_PARAM_TRANSITIONPROPS:
        if (m_mode == eMEM_TransitionEditor)
        {
            newTrack = new CTransitionPropertyTrack(*m_pScopeData);
        }
        break;
    }

    if (newTrack)
    {
        AddTrack(nParamId, newTrack);
    }

    return newTrack;
}


void CScopeNode::UpdateMutedLayerMasks(uint32 mutedAnimLayerMask, uint32 mutedProcLayerMask)
{
    if (m_pScopeData && m_pScopeData->mannContexts && m_pScopeData->mannContexts->viewData[m_mode].m_pActionController)
    {
        IScope* pScope = m_pScopeData->mannContexts->viewData[m_mode].m_pActionController->GetScope(m_pScopeData->scopeID);
        if (pScope)
        {
            pScope->MuteLayers(mutedAnimLayerMask, mutedProcLayerMask);
        }
    }
}

