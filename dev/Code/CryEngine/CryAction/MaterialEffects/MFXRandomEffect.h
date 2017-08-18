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

// Description : Random effect (randomly plays one of its child effects)

#ifndef CRYINCLUDE_CRYACTION_MATERIALEFFECTS_MFXRANDOMEFFECT_H
#define CRYINCLUDE_CRYACTION_MATERIALEFFECTS_MFXRANDOMEFFECT_H
#pragma once

#include "MFXEffectBase.h"
#include "MFXContainer.h"


class CMFXRandomizerContainer
    : public CMFXContainer
{
public:
    void ExecuteRandomizedEffects(const SMFXRunTimeEffectParams& params);

private:
    TMFXEffectBasePtr ChooseCandidate(const SMFXRunTimeEffectParams& params) const;
};

class CMFXRandomEffect
    : public CMFXEffectBase
{
public:
    CMFXRandomEffect();
    virtual ~CMFXRandomEffect();

    //IMFXEffect
    virtual void Execute(const SMFXRunTimeEffectParams& params) override;
    virtual void LoadParamsFromXml(const XmlNodeRef& paramsNode) override;
    virtual void SetCustomParameter(const char* customParameter, const SMFXCustomParamValue& customParameterValue) override;
    virtual void GetResources(SMFXResourceList& resourceList) const override;
    virtual void PreLoadAssets() override;
    virtual void ReleasePreLoadAssets() override;
    virtual void GetMemoryUsage(ICrySizer* pSizer) const override;
    //~IMFXEffect

private:
    CMFXRandomizerContainer m_container;
};

#endif // CRYINCLUDE_CRYACTION_MATERIALEFFECTS_MFXRANDOMEFFECT_H
