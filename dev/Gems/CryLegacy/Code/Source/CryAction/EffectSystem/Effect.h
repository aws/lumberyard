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

// Description : Base class for effects managed by the effect system


#ifndef CRYINCLUDE_CRYACTION_EFFECTSYSTEM_EFFECT_H
#define CRYINCLUDE_CRYACTION_EFFECTSYSTEM_EFFECT_H
#pragma once

#include "IEffectSystem.h"


class CEffect
    : public IEffect
{
public:
    // IEffect
    virtual bool Activating(float delta);
    virtual bool Update(float delta);
    virtual bool Deactivating(float delta);
    virtual bool OnActivate();
    virtual bool OnDeactivate();
    virtual void SetState(EEffectState state);
    virtual EEffectState GetState();
    // ~IEffect
private:
    EEffectState        m_state;
};

#endif // CRYINCLUDE_CRYACTION_EFFECTSYSTEM_EFFECT_H
