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
#include "MFXForceFeedbackFX.h"
#include "CryAction.h"
#include "IForceFeedbackSystem.h"


CMFXForceFeedbackEffect::CMFXForceFeedbackEffect()
    : CMFXEffectBase(eMFXPF_ForceFeedback)
{
}

CMFXForceFeedbackEffect::~CMFXForceFeedbackEffect()
{
}

void CMFXForceFeedbackEffect::LoadParamsFromXml(const XmlNodeRef& paramsNode)
{
    // Xml data format
    /*
    <ForceFeedback name="..." minFallOffDistance="..." maxFallOffDistance="..." />
    */
    m_forceFeedbackParams.forceFeedbackEventName = paramsNode->getAttr("name");

    float minFallOffDistance = 0.0f;
    paramsNode->getAttr("minFallOffDistance", minFallOffDistance);
    float maxFallOffDistance = 5.0f;
    paramsNode->getAttr("maxFallOffDistance", maxFallOffDistance);

    m_forceFeedbackParams.intensityFallOffMinDistanceSqr = minFallOffDistance * minFallOffDistance;
    m_forceFeedbackParams.intensityFallOffMaxDistanceSqr = maxFallOffDistance * maxFallOffDistance;
}

void CMFXForceFeedbackEffect::Execute(const SMFXRunTimeEffectParams& params)
{
    FUNCTION_PROFILER(gEnv->pSystem, PROFILE_ACTION);

    float distanceToPlayerSqr = FLT_MAX;
    IActor* pClientActor = gEnv->pGame->GetIGameFramework()->GetClientActor();
    if (pClientActor)
    {
        distanceToPlayerSqr = (pClientActor->GetEntity()->GetWorldPos() - params.pos).GetLengthSquared();
    }

    const float testDistanceSqr = clamp_tpl(distanceToPlayerSqr, m_forceFeedbackParams.intensityFallOffMinDistanceSqr, m_forceFeedbackParams.intensityFallOffMaxDistanceSqr);
    const float minMaxDiffSqr = m_forceFeedbackParams.intensityFallOffMaxDistanceSqr - m_forceFeedbackParams.intensityFallOffMinDistanceSqr;

    float effectIntensity = (float)fsel(-minMaxDiffSqr, 0.0f, 1.0f - (testDistanceSqr - m_forceFeedbackParams.intensityFallOffMinDistanceSqr) / (minMaxDiffSqr + FLT_EPSILON));
    effectIntensity *= effectIntensity;
    if (effectIntensity > 0.01f)
    {
        IForceFeedbackSystem* pForceFeedback = CCryAction::GetCryAction()->GetIForceFeedbackSystem();
        assert(pForceFeedback);
        ForceFeedbackFxId fxId = pForceFeedback->GetEffectIdByName(m_forceFeedbackParams.forceFeedbackEventName.c_str());
        pForceFeedback->PlayForceFeedbackEffect(fxId, SForceFeedbackRuntimeParams(effectIntensity, 0.0f), eIDT_Gamepad);
    }
}

void CMFXForceFeedbackEffect::GetResources(SMFXResourceList& resourceList) const
{
    SMFXForceFeedbackListNode* listNode = SMFXForceFeedbackListNode::Create();
    listNode->m_forceFeedbackParams.forceFeedbackEventName = m_forceFeedbackParams.forceFeedbackEventName.c_str();
    listNode->m_forceFeedbackParams.intensityFallOffMinDistanceSqr = m_forceFeedbackParams.intensityFallOffMinDistanceSqr;
    listNode->m_forceFeedbackParams.intensityFallOffMaxDistanceSqr = m_forceFeedbackParams.intensityFallOffMaxDistanceSqr;

    SMFXForceFeedbackListNode* next = resourceList.m_forceFeedbackList;

    if (!next)
    {
        resourceList.m_forceFeedbackList = listNode;
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
void CMFXForceFeedbackEffect::GetMemoryUsage(ICrySizer* pSizer) const
{
    pSizer->Add(*this);
    pSizer->Add(m_forceFeedbackParams.forceFeedbackEventName);
}
