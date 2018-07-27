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
#include "ActorSystem.h"
#include "IGameFramework.h"
#include "ScriptBind_ActorSystem.h"

//------------------------------------------------------------------------
CScriptBind_ActorSystem::CScriptBind_ActorSystem(ISystem* pSystem, IGameFramework* pGameFW)
{
    m_pSystem = pSystem;
    //m_pActorSystem = pActorSystem;
    m_pGameFW = pGameFW;

    Init(pSystem->GetIScriptSystem(), m_pSystem);
    SetGlobalName("ActorSystem");

    RegisterGlobals();
    RegisterMethods();
}

//------------------------------------------------------------------------
CScriptBind_ActorSystem::~CScriptBind_ActorSystem()
{
}

//------------------------------------------------------------------------
void CScriptBind_ActorSystem::RegisterGlobals()
{
}

//------------------------------------------------------------------------
void CScriptBind_ActorSystem::RegisterMethods()
{
#undef SCRIPT_REG_CLASSNAME
#define SCRIPT_REG_CLASSNAME &CScriptBind_ActorSystem::

    SCRIPT_REG_TEMPLFUNC(CreateActor, "channelId, actorParams");
    SCRIPT_REG_TEMPLFUNC(IsPlayer, "entityId");
#undef SCRIPT_REG_CLASSNAME
}

//------------------------------------------------------------------------
// Example on how to use this function:
//
// local params =
// {
//   name     = "dude",
//   class    = "CSpectator",
//   position = {x=0, y=0, z=0},
//   rotation = {x=0, y=0, z=0},
//   scale    = {x=1, y=1, z=1}
// }
//
// Actor.CreateActor( channelId, params );
//
int CScriptBind_ActorSystem::CreateActor(IFunctionHandler* pH, ScriptHandle channelId, SmartScriptTable actorParams)
{
    CRY_ASSERT(m_pGameFW->GetIActorSystem() != NULL);
    const char* name;
    const char* className;
    Vec3 position;
    Vec3 scale;
    Vec3 rotation;

#define GET_VALUE_FROM_CHAIN(valName, val, chain)                                                                           \
    if (!chain.GetValue(valName, val))                                                                                      \
    {                                                                                                                       \
        CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_ERROR, "CreateActor failed because <%s> field not specified", valName); \
        bFailed = true;                                                                                                     \
    }

    // The following code had to be enclosed in a bracket because
    // CScriptSetGetChain needs to be declared statically and also needs to
    // be destructed before EndFunction
    bool bFailed = false;
    do
    {
        CScriptSetGetChain actorChain(actorParams);
        GET_VALUE_FROM_CHAIN("name",     name,      actorChain);
        GET_VALUE_FROM_CHAIN("class",    className, actorChain);
        GET_VALUE_FROM_CHAIN("position", position,  actorChain);
        GET_VALUE_FROM_CHAIN("rotation", rotation,  actorChain);
        GET_VALUE_FROM_CHAIN("scale",    scale,     actorChain);
    }

    while (false);

    if (bFailed)
    {
        return pH->EndFunction(false);
    }

    Quat q;
    q.SetRotationXYZ(Ang3(rotation));
    IActor* pActor = m_pGameFW->GetIActorSystem()->CreateActor(HandleToInt<unsigned int>(channelId), name, className, position, q, scale);

    if (pActor == NULL)
    {
        return pH->EndFunction();
    }
    else
    {
        return pH->EndFunction(pActor->GetEntity()->GetScriptTable());
    }
}


//////////////////////////////////////////////////////////////////////////
int CScriptBind_ActorSystem::IsPlayer(IFunctionHandler* pH, ScriptHandle entityId)
{
    EntityId eId = (EntityId)entityId.n;
    if (eId == LOCAL_PLAYER_ENTITY_ID)
    {
        return pH->EndFunction(true);
    }

    IActor* pActor = m_pGameFW->GetIActorSystem()->GetActor(eId);
    return pH->EndFunction(pActor && pActor->IsPlayer());
}
