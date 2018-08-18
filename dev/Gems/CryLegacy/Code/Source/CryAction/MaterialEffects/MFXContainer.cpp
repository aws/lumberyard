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
#include "MFXContainer.h"

#include "MFXRandomEffect.h"
#include "MFXParticleEffect.h"
#include "MFXAudioEffect.h"
#include "MFXDecalEffect.h"
#include "MFXFlowGraphEffect.h"
#include "MFXForceFeedbackFX.h"

#include "GameXmlParamReader.h"

namespace MaterialEffectsUtils
{
    CMFXEffectBase* CreateEffectByName(const char* effectType)
    {
        if (!azstricmp("RandEffect", effectType))
        {
            return new CMFXRandomEffect();
        }
        else if (!azstricmp("Particle", effectType))
        {
            return new CMFXParticleEffect();
        }
        else if (!azstricmp("Audio", effectType))
        {
            return new CMFXAudioEffect();
        }
        else if (!azstricmp("Decal", effectType))
        {
            return new CMFXDecalEffect();
        }
        else if (!azstricmp("FlowGraph", effectType))
        {
            return new CMFXFlowGraphEffect();
        }
        else if (!azstricmp("ForceFeedback", effectType))
        {
            return new CMFXForceFeedbackEffect();
        }

        return NULL;
    }
}

CMFXContainer::~CMFXContainer()
{
}

void CMFXContainer::BuildFromXML(const XmlNodeRef& paramsNode)
{
    CRY_ASSERT(paramsNode != NULL);

    LoadParamsFromXml(paramsNode);

    BuildChildEffects(paramsNode);
}

void CMFXContainer::Execute(const SMFXRunTimeEffectParams& params)
{
    TMFXEffects::iterator it = m_effects.begin();
    TMFXEffects::iterator itEnd = m_effects.end();

    while (it != itEnd)
    {
        TMFXEffectBasePtr& pEffect = *it;
        if (pEffect->CanExecute(params))
        {
            pEffect->Execute(params);
        }

        ++it;
    }
}

void CMFXContainer::SetCustomParameter(const char* customParameter, const SMFXCustomParamValue& customParameterValue)
{
    FUNCTION_PROFILER(gEnv->pSystem, PROFILE_ACTION);

    TMFXEffects::iterator it = m_effects.begin();
    TMFXEffects::iterator itEnd = m_effects.end();
    while (it != itEnd)
    {
        TMFXEffectBasePtr& pEffect = *it;
        pEffect->SetCustomParameter(customParameter, customParameterValue);

        ++it;
    }
}

void CMFXContainer::GetResources(SMFXResourceList& resourceList) const
{
    TMFXEffects::const_iterator it = m_effects.begin();
    TMFXEffects::const_iterator itEnd = m_effects.end();
    while (it != itEnd)
    {
        TMFXEffectBasePtr pEffect = *it;
        pEffect->GetResources(resourceList);

        ++it;
    }
}

void CMFXContainer::PreLoadAssets()
{
    TMFXEffects::const_iterator it = m_effects.begin();
    TMFXEffects::const_iterator itEnd = m_effects.end();
    while (it != itEnd)
    {
        TMFXEffectBasePtr pEffect = *it;
        pEffect->PreLoadAssets();

        ++it;
    }
}

void CMFXContainer::ReleasePreLoadAssets()
{
    TMFXEffects::const_iterator it = m_effects.begin();
    TMFXEffects::const_iterator itEnd = m_effects.end();
    while (it != itEnd)
    {
        TMFXEffectBasePtr pEffect = *it;
        pEffect->ReleasePreLoadAssets();

        ++it;
    }
}

void CMFXContainer::GetMemoryUsage(ICrySizer* pSizer) const
{
    pSizer->AddObject(m_params.name);
    pSizer->AddObject(m_params.libraryName);
    pSizer->AddObject(m_effects);
}

void CMFXContainer::BuildChildEffects(const XmlNodeRef& paramsNode)
{
    const CGameXmlParamReader reader(paramsNode);
    const int totalChildCount = reader.GetUnfilteredChildCount();
    const int filteredChildCount = reader.GetFilteredChildCount();

    m_effects.reserve(filteredChildCount);

    for (int i = 0; i < totalChildCount; i++)
    {
        XmlNodeRef currentEffectNode = reader.GetFilteredChildAt(i);
        if (currentEffectNode == NULL)
        {
            continue;
        }

        const char* nodeName = currentEffectNode->getTag();
        if (nodeName == 0 || *nodeName == 0)
        {
            continue;
        }

        TMFXEffectBasePtr pEffect = MaterialEffectsUtils::CreateEffectByName(nodeName);
        if (pEffect != NULL)
        {
            pEffect->LoadParamsFromXml(currentEffectNode);
            m_effects.push_back(pEffect);
        }
    }
}

void CMFXContainer::LoadParamsFromXml(const XmlNodeRef& paramsNode)
{
    m_params.name = paramsNode->getAttr("name");

    paramsNode->getAttr("delay", m_params.delay);
}

//////////////////////////////////////////////////////////////////////////

void CMFXDummyContainer::BuildChildEffects(const XmlNodeRef& paramsNode)
{
}