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

#ifndef CRYINCLUDE_CRYACTION_MATERIALEFFECTS_MFXFLOWGRAPHEFFECT_H
#define CRYINCLUDE_CRYACTION_MATERIALEFFECTS_MFXFLOWGRAPHEFFECT_H
#pragma once

#include "MFXEffectBase.h"

struct SMFXFlowGraphParams
{
    static const int MAX_CUSTOM_PARAMS = 4;
    string fgName;
    float  maxdistSq; // max distance (squared) for spawning this effect
    float  params[MAX_CUSTOM_PARAMS];

    SMFXFlowGraphParams()
    {
        maxdistSq = 0.0f;
        memset(&params, 0, sizeof(params));
    }
};

class CMFXFlowGraphEffect
    : public CMFXEffectBase
{
public:
    CMFXFlowGraphEffect();
    virtual ~CMFXFlowGraphEffect();

    //IMFXEffect
    virtual void Execute(const SMFXRunTimeEffectParams& params) override;
    virtual void LoadParamsFromXml(const XmlNodeRef& paramsNode) override;
    virtual void SetCustomParameter(const char* customParameter, const SMFXCustomParamValue& customParameterValue) override;
    virtual void GetResources(SMFXResourceList& resourceList) const override;
    virtual void GetMemoryUsage(ICrySizer* pSizer) const override;
    //~IMFXEffect

private:
    SMFXFlowGraphParams m_flowGraphParams;
};

#endif // CRYINCLUDE_CRYACTION_MATERIALEFFECTS_MFXFLOWGRAPHEFFECT_H
