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

// Description : Virtual base class for all derived effects


#ifndef CRYINCLUDE_CRYACTION_MATERIALEFFECTS_IMFXEFFECT_H
#define CRYINCLUDE_CRYACTION_MATERIALEFFECTS_IMFXEFFECT_H
#pragma once

#include <IMaterialEffects.h>

typedef string TMFXNameId; // we use strings, and with clever assigning we minimize duplicates

struct IMFXEffect
    : public _reference_target_t
{
    enum MFXEffectType
    {
        TYPE_DEFAULT = 0,
        TYPE_PLAYER_FOOTSTEP,
        TYPE_IMPACT,
        TYPE_COLLISION,
    };
    virtual ~IMFXEffect() {};

    virtual void Execute(const SMFXRunTimeEffectParams& params) = 0;
    virtual void LoadParamsFromXml(const XmlNodeRef& paramsNode) = 0;
    virtual void SetCustomParameter(const char* customParameter, const SMFXCustomParamValue& customParameterValue) = 0;
    virtual void GetResources(SMFXResourceList& resourceList) const = 0;
    virtual void PreLoadAssets() = 0;
    virtual void ReleasePreLoadAssets() = 0;
    virtual void GetMemoryUsage(ICrySizer* pSizer) const = 0;
};

#endif // CRYINCLUDE_CRYACTION_MATERIALEFFECTS_IMFXEFFECT_H
