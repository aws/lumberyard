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

// Description : Blood splat effect


#ifndef CRYINCLUDE_CRYACTION_EFFECTSYSTEM_BLOODSPLATS_H
#define CRYINCLUDE_CRYACTION_EFFECTSYSTEM_BLOODSPLATS_H
#pragma once

#include "Effect.h"

class CBloodSplats
    : public CEffect
{
public:

    void Init(int type, float maxTime);
    // IEffect overrides
    virtual bool Update(float delta);
    virtual bool OnActivate();
    virtual bool OnDeactivate();
    virtual void GetMemoryUsage(ICrySizer* s) const;
    // ~IEffect overrides

private:
    int         m_type;                 // 0 human, 1 alien
    float       m_maxTime;          // maximum time until effect expires
    float       m_currentTime;  // current time
};

#endif // CRYINCLUDE_CRYACTION_EFFECTSYSTEM_BLOODSPLATS_H
