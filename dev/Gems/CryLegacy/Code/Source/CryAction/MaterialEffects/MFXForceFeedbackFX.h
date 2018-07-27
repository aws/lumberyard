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

#ifndef CRYINCLUDE_CRYACTION_MATERIALEFFECTS_MFXFORCEFEEDBACKFX_H
#define CRYINCLUDE_CRYACTION_MATERIALEFFECTS_MFXFORCEFEEDBACKFX_H
#pragma once

#include "MFXEffectBase.h"

struct SMFXForceFeedbackParams
{
    string forceFeedbackEventName;
    float intensityFallOffMinDistanceSqr;
    float intensityFallOffMaxDistanceSqr;

    SMFXForceFeedbackParams()
        : intensityFallOffMinDistanceSqr(0.0f)
        , intensityFallOffMaxDistanceSqr(25.0f)
    {
    }
};

class CMFXForceFeedbackEffect
    : public CMFXEffectBase
{
public:
    CMFXForceFeedbackEffect();
    virtual ~CMFXForceFeedbackEffect();

    //IMFXEffect
    virtual void Execute(const SMFXRunTimeEffectParams& params) override;
    virtual void LoadParamsFromXml(const XmlNodeRef& paramsNode) override;
    virtual void GetResources(SMFXResourceList& resourceList) const override;
    virtual void GetMemoryUsage(ICrySizer* pSizer) const override;
    //~IMFXEffect

protected:
    SMFXForceFeedbackParams m_forceFeedbackParams;
};

#endif // CRYINCLUDE_CRYACTION_MATERIALEFFECTS_MFXFORCEFEEDBACKFX_H
