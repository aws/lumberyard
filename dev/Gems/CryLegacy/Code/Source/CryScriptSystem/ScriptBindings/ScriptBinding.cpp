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

#include "CryLegacy_precompiled.h"
#include "ScriptBinding.h"

#include "ScriptBind_System.h"
#include "ScriptBind_Particle.h"
#include "ScriptBind_Sound.h"
#include "ScriptBind_Movie.h"
#include "ScriptBind_Script.h"
#include "ScriptBind_Physics.h"

#include "SurfaceTypes.h"

#include <IScriptSystem.h>

#if defined(AZ_RESTRICTED_PLATFORM)
#include AZ_RESTRICTED_FILE(ScriptBinding_cpp, AZ_RESTRICTED_PLATFORM)
#endif

CScriptBindings::CScriptBindings()
{
    m_pScriptSurfaceTypes = 0;
}

CScriptBindings::~CScriptBindings()
{
    Done();
}

//////////////////////////////////////////////////////////////////////////
void CScriptBindings::Init(ISystem* pSystem, IScriptSystem* pSS)
{
    m_binds.push_back(std::make_unique<CScriptBind_System>(pSS, pSystem));
    m_binds.push_back(std::make_unique<CScriptBind_Particle>(pSS, pSystem));
    m_binds.push_back(std::make_unique<CScriptBind_Sound>(pSS, pSystem));
    m_binds.push_back(std::make_unique<CScriptBind_Movie>(pSS, pSystem));
    m_binds.push_back(std::make_unique<CScriptBind_Script>(pSS, pSystem));
    m_binds.push_back(std::make_unique<CScriptBind_Physics>(pSS, pSystem));

    //////////////////////////////////////////////////////////////////////////
    // Enumerate script surface types.
    //////////////////////////////////////////////////////////////////////////
    m_pScriptSurfaceTypes = new CScriptSurfaceTypesLoader;
}

//////////////////////////////////////////////////////////////////////////
void CScriptBindings::Done()
{
    if (m_pScriptSurfaceTypes)
    {
        delete m_pScriptSurfaceTypes;
    }
    m_pScriptSurfaceTypes = NULL;

    // Done script bindings.
    m_binds.clear();
}

//////////////////////////////////////////////////////////////////////////
void CScriptBindings::LoadScriptedSurfaceTypes(const char* sFolder, bool bReload)
{
    m_pScriptSurfaceTypes->LoadSurfaceTypes(sFolder, bReload);
}

//////////////////////////////////////////////////////////////////////////
void CScriptBindings::GetMemoryStatistics(ICrySizer* pSizer) const
{
    //pSizer->AddObject(m_binds);
}
