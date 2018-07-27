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

// Description : IMFXEffect implementation


#include "CryLegacy_precompiled.h"
#include "MFXFlowGraphEffect.h"
#include "MaterialEffects.h"
#include "MaterialFGManager.h"
#include "MaterialEffectsCVars.h"
#include "CryAction.h"

namespace
{
    CMaterialFGManager* GetFGManager()
    {
        CMaterialEffects* pMFX = static_cast<CMaterialEffects*> (CCryAction::GetCryAction()->GetIMaterialEffects());
        if (pMFX == 0)
        {
            return 0;
        }
        CMaterialFGManager* pMFXFGMgr = pMFX->GetFGManager();
        assert (pMFXFGMgr != 0);
        return pMFXFGMgr;
    }
};

CMFXFlowGraphEffect::CMFXFlowGraphEffect()
    : CMFXEffectBase(eMFXPF_Flowgraph)
{
}

CMFXFlowGraphEffect::~CMFXFlowGraphEffect()
{
    CMaterialFGManager* pMFXFGMgr = GetFGManager();
    if (pMFXFGMgr && m_flowGraphParams.fgName.empty() == false)
    {
        pMFXFGMgr->EndFGEffect(m_flowGraphParams.fgName);
    }
}

void CMFXFlowGraphEffect::LoadParamsFromXml(const XmlNodeRef& paramsNode)
{
    // Xml data format
    /*
    <FlowGraph name="..." maxdist="..." param1="..." param2="..." />
    */

    m_flowGraphParams.fgName = paramsNode->getAttr("name");
    if (paramsNode->haveAttr("maxdist"))
    {
        paramsNode->getAttr("maxdist", m_flowGraphParams.maxdistSq);
        m_flowGraphParams.maxdistSq *= m_flowGraphParams.maxdistSq;
    }
    paramsNode->getAttr("param1", m_flowGraphParams.params[0]); //MFX custom param 1
    paramsNode->getAttr("param2", m_flowGraphParams.params[1]); //MFX custom param 2
    m_flowGraphParams.params[2] = 1.0f; //Intensity (set dynamically from game code)
    m_flowGraphParams.params[3] = 0.0f; //Blend out time
}

void CMFXFlowGraphEffect::Execute(const SMFXRunTimeEffectParams& params)
{
    FUNCTION_PROFILER(gEnv->pSystem, PROFILE_ACTION);

    if (CMaterialEffectsCVars::Get().mfx_EnableFGEffects == 0)
    {
        return;
    }

    float distToPlayerSq = 0.f;
    IActor* pAct = gEnv->pGame->GetIGameFramework()->GetClientActor();
    if (pAct)
    {
        distToPlayerSq = (pAct->GetEntity()->GetWorldPos() - params.pos).GetLengthSquared();
    }

    //Check max distance
    if (m_flowGraphParams.maxdistSq == 0.f || distToPlayerSq <= m_flowGraphParams.maxdistSq)
    {
        CMaterialFGManager* pMFXFGMgr = GetFGManager();
        pMFXFGMgr->StartFGEffect(m_flowGraphParams, sqrt_tpl(distToPlayerSq));
        //if(pMFXFGMgr->StartFGEffect(m_flowGraphParams.fgName))
        //  CryLogAlways("Starting FG HUD effect %s (player distance %f)", m_flowGraphParams.fgName.c_str(),distToPlayer);
    }
}

void CMFXFlowGraphEffect::SetCustomParameter(const char* customParameter, const SMFXCustomParamValue& customParameterValue)
{
    FUNCTION_PROFILER(gEnv->pSystem, PROFILE_ACTION);

    if (CMaterialEffectsCVars::Get().mfx_EnableFGEffects == 0)
    {
        return;
    }

    CMaterialFGManager* pMFXFGMgr = GetFGManager();
    pMFXFGMgr->SetFGCustomParameter(m_flowGraphParams, customParameter, customParameterValue);
}

void CMFXFlowGraphEffect::GetResources(SMFXResourceList& resourceList) const
{
    SMFXFlowGraphListNode* listNode = SMFXFlowGraphListNode::Create();
    listNode->m_flowGraphParams.name = m_flowGraphParams.fgName;

    SMFXFlowGraphListNode* next = resourceList.m_flowGraphList;

    if (!next)
    {
        resourceList.m_flowGraphList = listNode;
    }
    else
    {
        while (next->pNext)
        {
            next = next->pNext;
        }

        next->pNext = listNode;
    }
}

void CMFXFlowGraphEffect::GetMemoryUsage(ICrySizer* pSizer) const
{
    pSizer->AddObject(this, sizeof(*this));
    pSizer->AddObject(m_flowGraphParams.fgName);
}
