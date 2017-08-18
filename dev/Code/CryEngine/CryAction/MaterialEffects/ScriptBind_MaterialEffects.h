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

// Description : MaterialEffects ScriptBind

#ifndef CRYINCLUDE_CRYACTION_MATERIALEFFECTS_SCRIPTBIND_MATERIALEFFECTS_H
#define CRYINCLUDE_CRYACTION_MATERIALEFFECTS_SCRIPTBIND_MATERIALEFFECTS_H
#pragma once


#include <IScriptSystem.h>
#include <ScriptHelpers.h>

class CMaterialEffects;

class CScriptBind_MaterialEffects
    : public CScriptableBase
{
public:
    CScriptBind_MaterialEffects(ISystem* pSystem, CMaterialEffects* pDS);
    virtual ~CScriptBind_MaterialEffects();

    void Release() { delete this; };

    virtual void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(this, sizeof(*this));
    }
private:
    void RegisterGlobals();
    void RegisterMethods();

    int GetEffectId(IFunctionHandler* pH, const char* customName, int surfaceIndex2);
    int GetEffectIdByLibName(IFunctionHandler* pH, const char* LibName, const char* FGFXName);
    int PrintEffectIdByMatIndex(IFunctionHandler* pH, int materialIndex1, int materialIndex2);
    int ExecuteEffect(IFunctionHandler* pH, int effectId, SmartScriptTable paramsTable);

private:
    ISystem*       m_pSystem;
    CMaterialEffects* m_pMFX;
};

#endif // CRYINCLUDE_CRYACTION_MATERIALEFFECTS_SCRIPTBIND_MATERIALEFFECTS_H
