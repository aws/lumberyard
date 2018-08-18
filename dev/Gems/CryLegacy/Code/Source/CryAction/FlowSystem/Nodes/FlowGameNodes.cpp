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
#include <FlowSystem/Nodes/FlowBaseNode.h>
#include "CryAction.h"
#include "IActorSystem.h"
#include "GameObjects/GameObject.h"
#include "IGameRulesSystem.h"
#include "ILevelSystem.h"

inline IActor* GetAIActor(IFlowNode::SActivationInfo* pActInfo)
{
    if (!pActInfo->pEntity)
    {
        return 0;
    }
    return CCryAction::GetCryAction()->GetIActorSystem()->GetActor(pActInfo->pEntity->GetId());
}

class CFlowPlayer
    : public CFlowBaseNode<eNCT_Singleton>
{
public:
    CFlowPlayer(SActivationInfo* pActInfo)
    {
    }

    virtual void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig inputs[] = {
            InputPortConfig_Void   ("update", _HELP("Retriggers the entity id. Required for multiplayer")),
            {0}
        };
        static const SOutputPortConfig outputs[] = {
            OutputPortConfig<FlowEntityId>("entityId", _HELP("Player entity id")),
            {0}
        };
        config.pInputPorts = inputs;
        config.pOutputPorts = outputs;
        config.sDescription = _HELP("Outputs the local players entity id - NOT USABLE FOR MULTIPLAYER WITHOUT UPDATING BY HAND AFTER GAMESTART");
        config.SetCategory(EFLN_APPROVED);
    }

    virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        switch (event)
        {
        case eFE_Initialize:
        {
            bool done = UpdateEntityIdOutput(pActInfo);
            if (!done)
            {
                pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, true);
            }
            break;
        }

        case eFE_Activate:
            UpdateEntityIdOutput(pActInfo);
            break;

        case eFE_Update:
            pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, false);
            UpdateEntityIdOutput(pActInfo);
            break;
        }
    }

    bool UpdateEntityIdOutput(SActivationInfo* pActInfo)
    {
        IActor* pActor = CCryAction::GetCryAction()->GetClientActor();
        if (pActor)
        {
            ActivateOutput(pActInfo, 0, pActor->GetEntityId());
            return true;
        }
        else
        {
            return false;
        }
    }


    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }
};

class CFlowAllPlayers
    : public CFlowBaseNode<eNCT_Singleton>
{
public:
    CFlowAllPlayers(SActivationInfo* pActInfo)
    {
    }

    virtual void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig inputs[] = {
            InputPortConfig_Void   ("update", _HELP("Retriggers the entity id. Required for multiplayer")),
            {0}
        };
        static const SOutputPortConfig outputs[] = {
            OutputPortConfig<FlowEntityId>("entityId1", _HELP("Player 1")),
            OutputPortConfig<FlowEntityId>("entityId2", _HELP("Player 2")),
            OutputPortConfig<FlowEntityId>("entityId3", _HELP("Player 3")),
            OutputPortConfig<FlowEntityId>("entityId4", _HELP("Player 4")),
            {0}
        };
        config.pInputPorts = inputs;
        config.pOutputPorts = outputs;
        config.sDescription = _HELP("Outputs the players entity id..");
        config.SetCategory(EFLN_APPROVED);
    }

    virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        switch (event)
        {
        case eFE_Initialize:
        case eFE_Activate:
        {
            IActorSystem* pActorSystem = gEnv->pGame->GetIGameFramework()->GetIActorSystem();
            IActorIteratorPtr actorIt = pActorSystem->CreateActorIterator();
            int iNumPlayers = 0;
            IActor* pActor = actorIt->Next();
            while (iNumPlayers < 4 && pActor)
            {
                if (pActor->GetChannelId())
                {
                    ActivateOutput(pActInfo, iNumPlayers, pActor->GetEntityId());
                    ++iNumPlayers;
                }

                pActor = actorIt->Next();
            }
        }
        break;
        }
    }

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }
};

class CFlowIsPlayer
    : public CFlowBaseNode<eNCT_Singleton>
{
public:
    CFlowIsPlayer(SActivationInfo* pActInfo)
    {
    }

    virtual void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig inputs[] = {
            InputPortConfig_Void   ("update", _HELP("Retriggers the output.")),
            {0}
        };
        static const SOutputPortConfig outputs[] = {
            OutputPortConfig<bool>("isPlayer", _HELP("Entity is a player (local or multiplayer client).")),
            {0}
        };
        config.pInputPorts = inputs;
        config.pOutputPorts = outputs;
        config.nFlags |= EFLN_TARGET_ENTITY;
        config.sDescription = _HELP("Outputs whether an entity is a player.");
        config.SetCategory(EFLN_OBSOLETE);
    }

    virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        switch (event)
        {
        case eFE_Initialize:
        case eFE_Activate:
        {
            if (!pActInfo->pEntity)
            {
                ActivateOutput(pActInfo, 0, false);
            }
            else
            {
                CRY_ASSERT(gEnv->pGame);
                IActor* pActor = gEnv->pGame->GetIGameFramework()->GetIActorSystem()->GetActor(pActInfo->pEntity->GetId());
                if (pActor && pActor->GetChannelId() != 0)     //is this a client ?
                {
                    ActivateOutput(pActInfo, 0, true);
                }
                else
                {
                    ActivateOutput(pActInfo, 0, false);
                }
            }
        }
        break;
        }
    }

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }
};


//////////////////////////////////////////////////////////////////////////

void SendFlowHitToEntity(EntityId targetId, EntityId shooterId, int damage, const Vec3& pos)
{
    if (IEntity* pGameRules = CCryAction::GetCryAction()->GetIGameRulesSystem()->GetCurrentGameRulesEntity())
    {
        if (IScriptTable* pGameRulesScript = pGameRules->GetScriptTable())
        {
            IScriptSystem* pSS = pGameRulesScript->GetScriptSystem();
            if (pGameRulesScript->GetValueType("CreateHit") == svtFunction &&
                pSS->BeginCall(pGameRulesScript, "CreateHit"))
            {
                pSS->PushFuncParam(pGameRulesScript);
                pSS->PushFuncParam(ScriptHandle(targetId));
                pSS->PushFuncParam(ScriptHandle(shooterId));
                pSS->PushFuncParam(ScriptHandle(0)); // weapon
                pSS->PushFuncParam(damage);
                pSS->PushFuncParam(0);  // radius
                pSS->PushFuncParam(""); // material
                pSS->PushFuncParam(0); // partID
                pSS->PushFuncParam("normal");  // type
                pSS->PushFuncParam(pos);
                pSS->PushFuncParam(FORWARD_DIRECTION);  // dir
                pSS->PushFuncParam(FORWARD_DIRECTION); // normal
                pSS->EndCall();
            }
        }
    }
}
//////////////////////////////////////////////////////////////////////////

class CFlowDamageActor
    : public CFlowBaseNode<eNCT_Singleton>
{
    enum
    {
        INP_TRIGGER = 0,
        INP_DAMAGE,
        INP_RELATIVEDAMAGE,
        INP_POSITION
    };


public:
    CFlowDamageActor(SActivationInfo* pActInfo)
    {
    }

    virtual void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig inputs[] = {
            InputPortConfig_Void   ("Trigger", _HELP("Trigger this port to actually damage the actor")),
            InputPortConfig<int>   ("damage", 0, _HELP("Amount of damage to exert when [Trigger] is activated"), _HELP("Damage")),
            InputPortConfig<int>   ("damageRelative", 0, _HELP("Amount of damage to exert when [Trigger] is activated. Is relative to the maximun health of the actor. (100 = full max damage)"), _HELP("DamageRelative")),
            InputPortConfig<Vec3>  ("Position", Vec3(ZERO), _HELP("Position of damage")),
            {0}
        };
        config.pInputPorts = inputs;
        config.nFlags |= EFLN_TARGET_ENTITY;
        config.sDescription = _HELP("Damages attached entity by [Damage] when [Trigger] is activated");
        config.SetCategory(EFLN_APPROVED);
    }

    virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        switch (event)
        {
        case eFE_Activate:
            if (IsPortActive(pActInfo, INP_TRIGGER))
            {
                IActor* pActor = GetAIActor(pActInfo);
                if (pActor)
                {
                    int damage = GetPortInt(pActInfo, INP_DAMAGE);
                    if (damage == 0)
                    {
                        damage = (GetPortInt(pActInfo, INP_RELATIVEDAMAGE) * int(pActor->GetMaxHealth())) / 100;
                    }

                    SendFlowHitToEntity(pActor->GetEntityId(), pActor->GetEntityId(), damage, GetPortVec3(pActInfo, INP_POSITION));
                }
            }
            break;
        }
    }

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }
};


class CFlowDamageEntity
    : public CFlowBaseNode<eNCT_Singleton>
{
    enum
    {
        INP_TRIGGER = 0,
        INP_DAMAGE,
        INP_RELATIVEDAMAGE,
        INP_POSITION
    };

public:
    CFlowDamageEntity(SActivationInfo* pActInfo)
    {
    }

    virtual void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig inputs[] = {
            InputPortConfig_Void   ("Trigger", _HELP("Trigger this port to actually damage the actor")),
            InputPortConfig<int>   ("damage", 0, _HELP("Amount of damage to exert when [Trigger] is activated"), _HELP("Damage")),
            InputPortConfig<int>   ("damageRelative", 0, _HELP("Amount of damage to exert when [Trigger] is activated. Is relative to the maximun health of the entity. (100 = full max damage). /nwarning: This will only works for entities with a GetMaxHealth() lua function"), _HELP("DamageRelative")),
            InputPortConfig<Vec3>  ("Position", Vec3(ZERO), _HELP("Position of damage")),
            {0}
        };
        config.pInputPorts = inputs;
        config.nFlags |= EFLN_TARGET_ENTITY;
        config.sDescription = _HELP("Damages attached entity by [Damage] when [Trigger] is activated");
        config.SetCategory(EFLN_APPROVED);
    }

    virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        switch (event)
        {
        case eFE_Activate:
            if (IsPortActive(pActInfo, INP_TRIGGER))
            {
                IEntity* pEntity = pActInfo->pEntity;
                if (pEntity)
                {
                    int damage = GetPortInt(pActInfo, INP_DAMAGE);
                    if (damage == 0)
                    {
                        int damageRelative = GetPortInt(pActInfo, INP_RELATIVEDAMAGE);
                        if (IScriptTable* pScriptTable = pEntity->GetScriptTable())
                        {
                            IScriptSystem* pSS = pScriptTable->GetScriptSystem();
                            if (pScriptTable->GetValueType("GetMaxHealth") == svtFunction &&    pSS->BeginCall(pScriptTable, "GetMaxHealth"))
                            {
                                pSS->PushFuncParam(pScriptTable);
                                ScriptAnyValue result;
                                if (pSS->EndCallAny(result))
                                {
                                    int maxHealth = 0;
                                    if (result.CopyTo(maxHealth))
                                    {
                                        damage = (damageRelative * maxHealth) / 100;
                                    }
                                }
                            }
                        }
                    }

                    SendFlowHitToEntity(pEntity->GetId(), pEntity->GetId(), damage, GetPortVec3(pActInfo, INP_POSITION));
                }
            }
            break;
        }
    }

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }
};



class CFlowActorGrabObject
    : public CFlowBaseNode<eNCT_Singleton>
{
public:
    CFlowActorGrabObject(SActivationInfo* pActInfo)
    {
    }

    virtual void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig inputs[] = {
            InputPortConfig<FlowEntityId>("objectId", _HELP("Entity to grab")),
            InputPortConfig_Void("grab", _HELP("Pulse this to grab object.")),
            InputPortConfig_Void("drop", _HELP("Pulse this to drop currently grabbed object.")),
            InputPortConfig<bool>("throw", _HELP("If true, object is thrown forward.")),
            {0}
        };
        static const SOutputPortConfig outputs[] = {
            OutputPortConfig<bool>("success", _HELP("true if object was grabbed/dropped successfully")),
            OutputPortConfig<FlowEntityId>("grabbedObjId", _HELP("Currently grabbed object, or 0")),
            {0}
        };
        config.pInputPorts = inputs;
        config.pOutputPorts = outputs;
        config.nFlags |= EFLN_TARGET_ENTITY;
        config.sDescription = _HELP("Target entity will try to grab the input object, respectively drop/throw its currently grabbed object.");
        config.SetCategory(EFLN_APPROVED);
    }

    virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        switch (event)
        {
        case eFE_Activate:
        {
            IActor* pActor = GetAIActor(pActInfo);
            if (!pActor)
            {
                break;
            }

            IEntity* pEnt = pActor->GetEntity();
            if (!pEnt)
            {
                break;
            }

            HSCRIPTFUNCTION func = 0;
            int ret = 0;
            IScriptTable* pTable = pEnt->GetScriptTable();
            if (!pTable)
            {
                break;
            }

            if (IsPortActive(pActInfo, 1) && pTable->GetValue("GrabObject", func))
            {
                IEntity* pObj = gEnv->pEntitySystem->GetEntity(FlowEntityId(GetPortEntityId(pActInfo, 0)));
                if (pObj)
                {
                    IScriptTable* pObjTable = pObj->GetScriptTable();
                    Script::CallReturn(gEnv->pScriptSystem, func, pTable, pObjTable, ret);
                }
                ActivateOutput(pActInfo, 0, ret);
            }
            else if (IsPortActive(pActInfo, 2) && pTable->GetValue("DropObject", func))
            {
                bool bThrow = GetPortBool(pActInfo, 3);
                Script::CallReturn(gEnv->pScriptSystem, func, pTable, bThrow, ret);
                ActivateOutput(pActInfo, 0, ret);
            }

            if (pTable->GetValue("GetGrabbedObject", func))
            {
                ScriptHandle sH(0);
                Script::CallReturn(gEnv->pScriptSystem, func, pTable, sH);
                ActivateOutput(pActInfo, 1, EntityId(sH.n));
            }

            if (func)
            {
                gEnv->pScriptSystem->ReleaseFunc(func);
            }

            break;
        }
        }
    }

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }
};


class CFlowActorGetHealth
    : public CFlowBaseNode<eNCT_Singleton>
{
public:
    CFlowActorGetHealth(SActivationInfo* pActInfo)
    {
    }

    virtual void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig inputs[] = {
            InputPortConfig_Void("Trigger", _HELP("Trigger this port to get the current health")),
            {0}
        };
        static const SOutputPortConfig outputs[] = {
            OutputPortConfig<int>("Health", _HELP("Current health of entity")),
            {0}
        };
        config.pInputPorts = inputs;
        config.pOutputPorts = outputs;
        config.nFlags |= EFLN_TARGET_ENTITY;
        config.sDescription = _HELP("Get health of an entity (actor)");
        config.SetCategory(EFLN_APPROVED);
    }

    virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        switch (event)
        {
        case eFE_Activate:
            if (IsPortActive(pActInfo, 0))
            {
                IActor* pActor = GetAIActor(pActInfo);
                if (pActor)
                {
                    ActivateOutput(pActInfo, 0, pActor->GetHealth());
                }
                else
                {
                    GameWarning("CFlowActorGetHealth - No Entity or Entity not an Actor!");
                }
            }
        }
    }

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }
};


class CFlowActorSetHealth
    : public CFlowBaseNode<eNCT_Singleton>
{
public:
    CFlowActorSetHealth(SActivationInfo* pActInfo)
    {
    }

    virtual void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig inputs[] = {
            InputPortConfig_Void("Trigger", _HELP("Trigger this port to set health")),
            InputPortConfig<float>("Value", _HELP("Health value to be set on entity when Trigger is activated")),
            {0}
        };
        static const SOutputPortConfig outputs[] = {
            OutputPortConfig<float>("Health", _HELP("Current health of entity (activated when Trigger is activated)")),
            {0}
        };
        config.pInputPorts = inputs;
        config.pOutputPorts = outputs;
        config.nFlags |= EFLN_TARGET_ENTITY;
        config.sDescription = _HELP("Set health of an entity (actor)");
        config.SetCategory(EFLN_APPROVED);
    }

    virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        switch (event)
        {
        case eFE_Activate:
            if (IsPortActive(pActInfo, 0))
            {
                IActor* pActor = GetAIActor(pActInfo);
                if (pActor)
                {
                    if (!pActor->IsDead())
                    {
                        pActor->SetHealth(GetPortFloat(pActInfo, 1));
                        ActivateOutput(pActInfo, 0, pActor->GetHealth()); // use pActor->GetHealth (might have been clamped to maxhealth]
                    }
                }
                else
                {
                    GameWarning("CFlowActorSetHealth - No Entity or Entity not an actor!");
                }
            }
        }
    }

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }
};


class CFlowActorCheckHealth
    : public CFlowBaseNode<eNCT_Singleton>
{
public:
    CFlowActorCheckHealth(SActivationInfo* pActInfo)
    {
    }

    virtual void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig inputs[] = {
            InputPortConfig_Void("Trigger", _HELP("Trigger this port to check health")),
            InputPortConfig<float>("MinHealth", 0.0f, _HELP("Lower limit of range")),
            InputPortConfig<float>("MaxHealth", 100.0f, _HELP("Upper limit of range")),
            {0}
        };
        static const SOutputPortConfig outputs[] = {
            OutputPortConfig<bool>("InRange", _HELP("True if Health is in range, False otherwise")),
            {0}
        };
        config.pInputPorts = inputs;
        config.pOutputPorts = outputs;
        config.nFlags |= EFLN_TARGET_ENTITY;
        config.sDescription = _HELP("Check if health of entity (actor) is in range [MinHealth, MaxHealth]");
        config.SetCategory(EFLN_APPROVED);
    }

    virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        switch (event)
        {
        case eFE_Activate:
            if (IsPortActive(pActInfo, 0))
            {
                IActor* pActor = GetAIActor(pActInfo);
                if (pActor)
                {
                    float health = pActor->GetHealth();
                    float minH = GetPortFloat(pActInfo, 1);
                    float maxH = GetPortFloat(pActInfo, 2);
                    ActivateOutput(pActInfo, 0, minH <= health && health <= maxH);
                }
                else
                {
                    CryLogAlways("CFlowActorCheckHealth - No Entity or Entity not an Actor!");
                }
            }
        }
    }

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }
};

class CFlowGameObjectEvent
    : public CFlowBaseNode<eNCT_Singleton>
{
public:
    CFlowGameObjectEvent(SActivationInfo* pActInfo)
    {
    }

    void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig in_ports[] =
        {
            InputPortConfig_Void("Trigger", _HELP("Trigger this port to send the event")),
            InputPortConfig<string>("EventName", _HELP("Name of event to send")),
            InputPortConfig<string>("EventParam", _HELP("Parameter of the event [event-specific]")),
            {0}
        };
        config.nFlags |= EFLN_TARGET_ENTITY;
        config.pInputPorts = in_ports;
        config.pOutputPorts = 0;
        config.sDescription = _HELP("Broadcast a game object event or send to a specific entity. EventParam is an event specific string");
        config.SetCategory(EFLN_ADVANCED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        if (eFE_Activate == event && IsPortActive(pActInfo, 0))
        {
            const string& eventName = GetPortString(pActInfo, 1);
            uint32 eventId = CCryAction::GetCryAction()->GetIGameObjectSystem()->GetEventID(eventName.c_str());
            SGameObjectEvent evt(eventId, eGOEF_ToAll, IGameObjectSystem::InvalidExtensionID, (void*) (GetPortString(pActInfo, 2).c_str()));
            if (pActInfo->pEntity == 0)
            {
                // broadcast to all gameobject events
                CCryAction::GetCryAction()->GetIGameObjectSystem()->BroadcastEvent(evt);
            }
            else
            {
                // send to a specific entity only
                if (CGameObjectPtr pGameObject = pActInfo->pEntity->GetComponent<CGameObject>())
                {
                    pGameObject->SendEvent(evt);
                }
            }
        }
    }

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }
};


class CFlowGetSupportedGameRulesForMap
    : public CFlowBaseNode<eNCT_Instanced>
{
public:
    enum EInputs
    {
        IN_GET,
        IN_MAPNAME
    };
    enum EOutputs
    {
        OUT_DONE
    };

    CFlowGetSupportedGameRulesForMap(SActivationInfo* pActInfo)
    {
    }

    IFlowNodePtr Clone(SActivationInfo* pActInfo)
    {
        return new CFlowGetSupportedGameRulesForMap(pActInfo);
    }

    virtual void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig inputs[] = {
            InputPortConfig_Void("Get", _HELP("Fetch supported gamerules")),
            InputPortConfig<string>("Mapname", _HELP("map name")),
            {0}
        };
        static const SOutputPortConfig outputs[] = {
            OutputPortConfig<string>("GameRules", _HELP("Supported gamerules pipe-separated string")),
            {0}
        };
        config.pInputPorts = inputs;
        config.pOutputPorts = outputs;
        config.sDescription = _HELP("Get supported gamerules for a map");
        config.SetCategory(EFLN_APPROVED);
    }

    virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        switch (event)
        {
        case eFE_Activate:
            if (IsPortActive(pActInfo, 0))
            {
                ILevelInfo* pLevelInfo = NULL;
                string mapname = GetPortString(pActInfo, IN_MAPNAME);

                //If mapname is provided, try and fetch those rules
                if (strcmp(mapname, "") != 0)
                {
                    pLevelInfo = CCryAction::GetCryAction()->GetILevelSystem()->GetLevelInfo(mapname);
                }


                //if mapname wasnt provided or is invalid grab current map
                if (!pLevelInfo && CCryAction::GetCryAction()->GetILevelSystem()->GetCurrentLevel())
                {
                    pLevelInfo = CCryAction::GetCryAction()->GetILevelSystem()->GetCurrentLevel()->GetLevelInfo();
                }

                if (pLevelInfo)
                {
                    ILevelInfo::TStringVec gamerules = pLevelInfo->GetGameRules();
                    string outString = "";

                    for (int i = 0; i < gamerules.size(); i++)
                    {
                        if (i > 0)
                        {
                            outString.append("|");
                        }
                        outString.append(gamerules[i]);
                    }
                    ActivateOutput(pActInfo, OUT_DONE, outString);
                }
            }
        }
    }

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }
};

class CFlowGetStateOfEntity
    : public CFlowBaseNode<eNCT_Instanced>
{
public:
    enum EInputs
    {
        IN_GET,
        IN_MAPNAME
    };
    enum EOutputs
    {
        OUT_STATE
    };

    CFlowGetStateOfEntity(SActivationInfo* pActInfo)
    {
    }

    IFlowNodePtr Clone(SActivationInfo* pActInfo)
    {
        return new CFlowGetStateOfEntity(pActInfo);
    }

    virtual void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig inputs[] = {
            InputPortConfig_Void("Get", _HELP("Get state")),
            {0}
        };
        static const SOutputPortConfig outputs[] = {
            OutputPortConfig<string>("State", _HELP("Current state as string")),
            {0}
        };
        config.nFlags |= EFLN_TARGET_ENTITY;
        config.pInputPorts = inputs;
        config.pOutputPorts = outputs;
        config.sDescription = _HELP("Get current state of an entity");
        config.SetCategory(EFLN_APPROVED);
    }

    virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        switch (event)
        {
        case eFE_Activate:
            if (IsPortActive(pActInfo, 0))
            {
                string state = "invalid entity";
                IEntity* ent = pActInfo->pEntity;
                if (ent)
                {
                    IComponentScriptPtr scriptComponent = ent->GetComponent<IComponentScript>();
                    state = "invalid script";
                    if (scriptComponent)
                    {
                        state = scriptComponent->GetState();
                    }
                }

                ActivateOutput(pActInfo, OUT_STATE, state);
            }
        }
    }

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }
};

//////////////////////////////////////////////////////////////////////////
class CFlowIsLevelOfType
    : public CFlowBaseNode<eNCT_Instanced>
{
public:
    enum EInputs
    {
        IN_CHECK,
        IN_TYPE
    };
    enum EOutputs
    {
        OUT_RESULT
    };

    CFlowIsLevelOfType(SActivationInfo* pActInfo)
    {
    }

    IFlowNodePtr Clone(SActivationInfo* pActInfo)
    {
        return new CFlowIsLevelOfType(pActInfo);
    }

    virtual void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig inputs[] = {
            InputPortConfig_Void("Check", _HELP("Check if level is of given type")),
            InputPortConfig<string>("Type", "", _HELP("type you want to check against"), 0, _UICONFIG("enum_global:level_types")),
            {0}
        };
        static const SOutputPortConfig outputs[] = {
            OutputPortConfig<bool>("Result", _HELP("Result")),
            {0}
        };

        config.pInputPorts = inputs;
        config.pOutputPorts = outputs;
        config.sDescription = _HELP("Check if a level is of given type");
        config.SetCategory(EFLN_APPROVED);
    }

    virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        switch (event)
        {
        case eFE_Activate:
            if (IsPortActive(pActInfo, 0))
            {
                bool bResult = false;
                const char* levelType;

                levelType = GetPortString(pActInfo, IN_TYPE);
                bResult = CCryAction::GetCryAction()->GetILevelSystem()->GetCurrentLevel()->GetLevelInfo()->IsOfType(levelType);

                ActivateOutput(pActInfo, OUT_RESULT, bResult);
            }
        }
    }

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }
};


//////////////////////////////////////////////////////////////////////////
class CGameGetClientActorId
    : public CFlowBaseNode<eNCT_Instanced>
{
public:
    enum EInputs
    {
        IN_SOCKET
    };
    enum EOutputs
    {
        CLIENTID
    };

    CGameGetClientActorId(SActivationInfo* pActInfo)
    {
    }

    IFlowNodePtr Clone(SActivationInfo* pActInfo)
    {
        return new CGameGetClientActorId(pActInfo);
    }

    virtual void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig inputs[] = {
            InputPortConfig_AnyType("In", _HELP("When triggered will fetch and pass on the client actor ID"), _HELP("In")),
            {0}
        };
        static const SOutputPortConfig outputs[] = {
            OutputPortConfig<FlowEntityId>("id", _HELP("Client ID")),
            {0}
        };

        config.pInputPorts = inputs;
        config.pOutputPorts = outputs;
        config.sDescription = _HELP("Fetches and passes on the client actor ID");
        config.SetCategory(EFLN_APPROVED);
    }

    virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        switch (event)
        {
        case eFE_Activate:
            if (IsPortActive(pActInfo, 0))
            {
                assert(gEnv);
                assert(gEnv->pGame);
                assert(gEnv->pGame->GetIGameFramework());

                auto clientActorId = gEnv->pGame->GetIGameFramework()->GetClientActorId();
                ActivateOutput(pActInfo, EOutputs::CLIENTID, clientActorId);
            }
        }
    }

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }
};

REGISTER_FLOW_NODE("Actor:LocalPlayer", CFlowPlayer);
REGISTER_FLOW_NODE("Game:IsPlayer", CFlowIsPlayer);
REGISTER_FLOW_NODE("Actor:Damage", CFlowDamageActor);
REGISTER_FLOW_NODE("Entity:Damage", CFlowDamageEntity)
REGISTER_FLOW_NODE("Actor:GrabObject", CFlowActorGrabObject);
REGISTER_FLOW_NODE("Actor:HealthGet", CFlowActorGetHealth);
REGISTER_FLOW_NODE("Actor:HealthSet", CFlowActorSetHealth);
REGISTER_FLOW_NODE("Actor:HealthCheck", CFlowActorCheckHealth);
REGISTER_FLOW_NODE("Game:ObjectEvent", CFlowGameObjectEvent);
REGISTER_FLOW_NODE("Game:GetSupportedGameRulesForMap", CFlowGetSupportedGameRulesForMap);
REGISTER_FLOW_NODE("Game:GetEntityState", CFlowGetStateOfEntity);
REGISTER_FLOW_NODE("Game:IsLevelOfType", CFlowIsLevelOfType);
REGISTER_FLOW_NODE("Game:GetClientActorId", CGameGetClientActorId);

FLOW_NODE_BLACKLISTED("Game:AllPlayers", CFlowAllPlayers);
