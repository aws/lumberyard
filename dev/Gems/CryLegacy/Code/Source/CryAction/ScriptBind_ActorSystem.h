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

// Description : Exposes basic Actor System to the Script System.


#ifndef CRYINCLUDE_CRYACTION_SCRIPTBIND_ACTORSYSTEM_H
#define CRYINCLUDE_CRYACTION_SCRIPTBIND_ACTORSYSTEM_H
#pragma once

#include <IScriptSystem.h>
#include <ScriptHelpers.h>

struct IActorSystem;
struct IGameFramework;


class CScriptBind_ActorSystem
    : public CScriptableBase
{
public:
    CScriptBind_ActorSystem(ISystem* pSystem, IGameFramework* pGameFW);
    virtual ~CScriptBind_ActorSystem();

    void Release() { delete this; };

    virtual void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(this, sizeof(*this));
    }

    //! <code>ActorSystem.CreateActor( channelId, actorParams )</code>
    //!     <param name="channelId">Identifier for the network channel.</param>
    //!     <param name="actorParams">Parameters for the actor.</param>
    //! <description>Creates an actor.</description>
    int CreateActor(IFunctionHandler* pH, ScriptHandle channelId, SmartScriptTable actorParams);

    //! <code>System.IsAPlayer()</code>
    //! <description>Checks if the entityId is the same as the Id associated with the player</description>
    int IsPlayer(IFunctionHandler* pH, ScriptHandle entityId);

private:
    void RegisterGlobals();
    void RegisterMethods();

    ISystem* m_pSystem;
    IGameFramework* m_pGameFW;
};

#endif // CRYINCLUDE_CRYACTION_SCRIPTBIND_ACTORSYSTEM_H
