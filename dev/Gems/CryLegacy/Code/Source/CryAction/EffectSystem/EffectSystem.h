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

// Description : Implements a system to manage various effects (bloodsplashes,
//               viewmodes, etc...)

#ifndef CRYINCLUDE_CRYACTION_EFFECTSYSTEM_EFFECTSYSTEM_H
#define CRYINCLUDE_CRYACTION_EFFECTSYSTEM_EFFECTSYSTEM_H
#pragma once

#include "IEffectSystem.h"

class CEffectSystem
    : public IEffectSystem
{
public:
    CEffectSystem();
    virtual ~CEffectSystem();

    // IEffectSystem
    virtual bool Init();
    virtual void Update(float delta);
    virtual void Shutdown();

    virtual EffectId GetEffectId(const char* name);

    virtual void Activate(const EffectId& eid);
    virtual bool BindEffect(const char* name, IEffect* pEffect);
    virtual IGroundEffect* CreateGroundEffect(IEntity* pEntity);

    virtual void RegisterFactory(const char* name, IEffect * (*)(), bool isAI);

    virtual void GetMemoryStatistics(ICrySizer* s);
    // ~IEffectSystem

private:
    typedef std::map<string, EffectId>  TNameToId;
    typedef std::vector<IEffect*>               TEffectVec;
    typedef std::map<string, IEffect*(*)()>    TEffectClassMap;

    TNameToId               m_nameToId;             // map from a string name to an effect id
    TEffectVec          m_effects;              // effect instances (array index == effect id)
    TEffectClassMap m_effectClasses;    // effect factories
};

#endif // CRYINCLUDE_CRYACTION_EFFECTSYSTEM_EFFECTSYSTEM_H
