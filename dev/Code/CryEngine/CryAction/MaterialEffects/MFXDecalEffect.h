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

// Description : Decal effect

#ifndef CRYINCLUDE_CRYACTION_MATERIALEFFECTS_MFXDECALEFFECT_H
#define CRYINCLUDE_CRYACTION_MATERIALEFFECTS_MFXDECALEFFECT_H
#pragma once

#include "MFXEffectBase.h"

struct SMFXDecalParams
{
    string material;
    float  minscale;
    float  maxscale;
    float  rotation;
    float  lifetime;
    float  growTime;
    bool   assemble;
    bool   forceedge;
};

class CMFXDecalEffect
    : public CMFXEffectBase
{
public:
    CMFXDecalEffect();
    virtual ~CMFXDecalEffect();

    // IMFXEffect
    virtual void Execute(const SMFXRunTimeEffectParams& params) override;
    virtual void LoadParamsFromXml(const XmlNodeRef& paramsNode) override;
    virtual void GetResources(SMFXResourceList& resourceList) const override;
    virtual void PreLoadAssets() override;
    virtual void ReleasePreLoadAssets() override;
    virtual void GetMemoryUsage(ICrySizer* pSizer) const override;
    //~IMFXEffect

private:

    void ReleaseMaterial();

    SMFXDecalParams       m_decalParams;
    _smart_ptr<IMaterial> m_material;
};

#endif // CRYINCLUDE_CRYACTION_MATERIALEFFECTS_MFXDECALEFFECT_H
