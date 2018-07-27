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

// Description : Basic and partial implemenation of IMFXEffect which serves as a base for concrete implementations


#ifndef _MFX_EFFECT_BASE_H_
#define _MFX_EFFECT_BASE_H_

#pragma once

#include <CryFlags.h>
#include "IMFXEffect.h"

class CMFXEffectBase
    : public IMFXEffect
{
public:

    CMFXEffectBase(const uint16 _typeFilterFlag);

    //IMFXEffect (partial implementation)
    virtual void SetCustomParameter(const char* customParameter, const SMFXCustomParamValue& customParameterValue) override;
    virtual void PreLoadAssets() override;
    virtual void ReleasePreLoadAssets() override;
    //~IMFXEffect

    bool CanExecute(const SMFXRunTimeEffectParams& params) const;

private:
    CCryFlags<uint16> m_runtimeExecutionFilter;
};

typedef _smart_ptr<CMFXEffectBase> TMFXEffectBasePtr;

#endif // _MFX_EFFECT_BASE_H_

