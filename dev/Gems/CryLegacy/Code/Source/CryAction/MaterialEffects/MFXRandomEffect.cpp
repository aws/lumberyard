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
#include "MFXRandomEffect.h"


void CMFXRandomizerContainer::ExecuteRandomizedEffects(const SMFXRunTimeEffectParams& params)
{
    TMFXEffectBasePtr pEffect = ChooseCandidate(params);
    if (pEffect)
    {
        pEffect->Execute(params);
    }
}

TMFXEffectBasePtr CMFXRandomizerContainer::ChooseCandidate(const SMFXRunTimeEffectParams& params) const
{
    const size_t nEffects = m_effects.size();
    if (nEffects == 0)
    {
        return TMFXEffectBasePtr(NULL);
    }

    CryFixedArray<TMFXEffectBasePtr, 16> candidatesArray;

    TMFXEffects::const_iterator it    = m_effects.begin();
    TMFXEffects::const_iterator itEnd = m_effects.end();
    while ((it != itEnd) && !candidatesArray.isfull())
    {
        const TMFXEffectBasePtr& pEffect = *it;
        if (pEffect->CanExecute(params))
        {
            candidatesArray.push_back(pEffect);
        }
        ++it;
    }

    TMFXEffectBasePtr pChosenEffect = NULL;
    if (!candidatesArray.empty())
    {
        const uint32 randChoice = cry_random(0U, candidatesArray.size() - 1);
        pChosenEffect = candidatesArray[randChoice];
    }

    return pChosenEffect;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

CMFXRandomEffect::CMFXRandomEffect()
    : CMFXEffectBase(eMFXPF_All)
{
}

CMFXRandomEffect::~CMFXRandomEffect()
{
}

void CMFXRandomEffect::GetMemoryUsage(ICrySizer* pSizer) const
{
    pSizer->AddObject(this, sizeof(*this));
}

void CMFXRandomEffect::Execute(const SMFXRunTimeEffectParams& params)
{
    FUNCTION_PROFILER(gEnv->pSystem, PROFILE_ACTION);

    m_container.ExecuteRandomizedEffects(params);
}

void CMFXRandomEffect::LoadParamsFromXml(const XmlNodeRef& paramsNode)
{
    // Xml data format
    /*
    <RandEffect>
        <Particle />
        <Audio />
        <ForceFeedback />
        ...
    </Particle>
    */

    m_container.BuildFromXML(paramsNode);
}

void CMFXRandomEffect::GetResources(SMFXResourceList& resourceList) const
{
    m_container.GetResources(resourceList);
}

void CMFXRandomEffect::PreLoadAssets()
{
    m_container.PreLoadAssets();
}

void CMFXRandomEffect::ReleasePreLoadAssets()
{
    m_container.ReleasePreLoadAssets();
}

void CMFXRandomEffect::SetCustomParameter(const char* customParameter, const SMFXCustomParamValue& customParameterValue)
{
    m_container.SetCustomParameter(customParameter, customParameterValue);
}
