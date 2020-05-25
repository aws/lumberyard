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

#include "StdAfx.h"

#include "LegacyTerrainBase.h"

ISystem* LegacyTerrainBase::m_pSystem = nullptr;
I3DEngine* LegacyTerrainBase::m_p3DEngine = nullptr;
IObjManager* LegacyTerrainBase::m_pObjManager = nullptr;
ITimer* LegacyTerrainBase::m_pTimer = nullptr;
IRenderer* LegacyTerrainBase::m_pRenderer = nullptr;
IPhysicalWorld* LegacyTerrainBase::m_pPhysicalWorld = nullptr;
ICryPak* LegacyTerrainBase::m_pCryPak = nullptr;
ILog* LegacyTerrainBase::m_pLog = nullptr;
IConsole* LegacyTerrainBase::m_pConsole = nullptr;
bool LegacyTerrainBase::m_bEditor = false;
LegacyTerrainCVars LegacyTerrainBase::m_cvars;


void LegacyTerrainBase::SetSystem(ISystem* pSystem)
{
    AZ_Assert(pSystem, "Invalid ISystem");
    m_pSystem = pSystem;

    SSystemGlobalEnvironment* env = pSystem->GetGlobalEnvironment();
    m_p3DEngine = env->p3DEngine;
    m_pObjManager = m_p3DEngine->GetObjManager();
    m_pTimer = env->pTimer;
    m_pRenderer = env->pRenderer;
    m_pPhysicalWorld = env->pPhysicalWorld;
    m_pCryPak = env->pCryPak;
    m_pLog = env->pLog;
    m_pConsole = env->pConsole;
    m_bEditor = env->IsEditor();
}


int LegacyTerrainBase::GetCVarAsInteger(const char* cvarName)
{
    AZ_Assert(m_pConsole, "Uninitialized IConsole");
    AZ_Assert(cvarName, "Null cvarName");
    ICVar* pvar = m_pConsole->GetCVar(cvarName);
    if (!pvar)
    {
        return 0;
    }
    return pvar->GetIVal();
}

float LegacyTerrainBase::GetCVarAsFloat(const char* cvarName)
{
    AZ_Assert(m_pConsole, "Uninitialized IConsole");
    AZ_Assert(cvarName, "Null cvarName");
    ICVar* pvar = m_pConsole->GetCVar(cvarName);
    if (!pvar)
    {
        return 0.0f;
    }
    return pvar->GetFVal();
}
