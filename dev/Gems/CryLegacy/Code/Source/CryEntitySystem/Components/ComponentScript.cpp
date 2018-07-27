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
#include "CryLegacy_precompiled.h"
#include "ComponentScript.h"
#include "Components/IComponentSerialization.h"

#include "EntityScript.h"
#include "Entity.h"
#include "ScriptProperties.h"
#include "ScriptBind_Entity.h"
#include "EntitySystem.h"
#include "ISerialize.h"
#include "EntityCVars.h"

#include <IScriptSystem.h>

DECLARE_DEFAULT_COMPONENT_FACTORY(CComponentScript, IComponentScript)

IComponentPtr CreateScriptComponent(IEntity* pEntity, IEntityScript* pScript, SEntitySpawnParams* pSpawnParams)
{
    // Load script now (Will be ignored if already loaded).
    CEntityScript* pEntityScript = (CEntityScript*)pScript;
    if (!pEntityScript->LoadScript())
    {
        return nullptr;
    }

    return GetIEntitySystem()->CreateComponentAndRegister<CComponentScript>(
        CComponentScript::SComponentInitializerScript(pEntity, pEntityScript, pSpawnParams));
}

//////////////////////////////////////////////////////////////////////////
CComponentScript::CComponentScript()
{
    m_pEntity = NULL;
    m_pScript = NULL;
    m_nCurrStateId = 0;
    m_fScriptUpdateRate = 0;
    m_fScriptUpdateTimer = 0;
    m_bEnableSoundAreaEvents = false;
    m_bUpdateScript = false;
}

//////////////////////////////////////////////////////////////////////////
CComponentScript::~CComponentScript()
{
    SAFE_RELEASE(m_pThis);
}

//////////////////////////////////////////////////////////////////////////
void CComponentScript::Initialize(const SComponentInitializer& init)
{
    assert(init.m_pEntity);

    if (m_pEntity == NULL)
    {
        m_pEntity = init.m_pEntity;

        m_pEntity->GetComponent<IComponentSerialization>()->Register<CComponentScript>(SerializationOrder::Script, *this, &CComponentScript::Serialize, &CComponentScript::SerializeXML, &CComponentScript::NeedSerialize, &CComponentScript::GetSignature);

        m_pScript = static_cast<const SComponentInitializerScript&> (init).m_pScript;

        // New object must be created here.
        CreateScriptTable(static_cast<const SComponentInitializerScript&> (init).m_pSpawnParams);

        m_bUpdateScript = CurrentState()->IsStateFunctionImplemented(ScriptState_OnUpdate);
    }
}

//////////////////////////////////////////////////////////////////////////
bool CComponentScript::InitComponent(IEntity* pEntity, SEntitySpawnParams& params)
{
    FUNCTION_PROFILER(GetISystem(), PROFILE_ENTITY);

    // Call Init.
    CallInitEvent(false);

    return true;
}

//////////////////////////////////////////////////////////////////////////
void CComponentScript::Reload(IEntity* pEntity, SEntitySpawnParams& params)
{
    FUNCTION_PROFILER(GetISystem(), PROFILE_ENTITY);

    IEntityClass* pClass = (pEntity ? pEntity->GetClass() : NULL);
    CEntityScript* pEntityScript = (pClass ? (CEntityScript*)pClass->GetIEntityScript() : NULL);
    if (pEntityScript && pEntityScript->LoadScript())
    {
        // Release current
        SAFE_RELEASE(m_pThis);

        m_pEntity = pEntity;
        m_pScript = pEntityScript;
        m_nCurrStateId = 0;
        m_fScriptUpdateRate = 0;
        m_fScriptUpdateTimer = 0;
        m_bEnableSoundAreaEvents = false;

        m_bUpdateScript = CurrentState()->IsStateFunctionImplemented(ScriptState_OnUpdate);

        // New object must be created here.
        CreateScriptTable(&params);
    }
}

//////////////////////////////////////////////////////////////////////////
void CComponentScript::ChangeScript(IEntityScript* pScript, SEntitySpawnParams* params)
{
    if (pScript)
    {
        // Release current
        SAFE_RELEASE(m_pThis);

        m_pScript = static_cast<CEntityScript*>(pScript);
        m_nCurrStateId = 0;
        m_fScriptUpdateRate = 0;
        m_fScriptUpdateTimer = 0;
        m_bEnableSoundAreaEvents = false;

        m_bUpdateScript = CurrentState()->IsStateFunctionImplemented(ScriptState_OnUpdate);

        // New object must be created here.
        CreateScriptTable(params);
    }
}

//////////////////////////////////////////////////////////////////////////
void CComponentScript::Done()
{
    m_pScript->Call_OnShutDown(m_pThis);
}

//////////////////////////////////////////////////////////////////////////
void CComponentScript::CreateScriptTable(SEntitySpawnParams* pSpawnParams)
{
    m_pThis = m_pScript->GetScriptSystem()->CreateTable();
    m_pThis->AddRef();
    //m_pThis->Clone( m_pScript->GetScriptTable() );

    CEntitySystem* pEntitySystem = GetIEntitySystem();
    if (pEntitySystem)
    {
        //pEntitySystem->GetScriptBindEntity()->DelegateCalls( m_pThis );
        m_pThis->Delegate(m_pScript->GetScriptTable());

        // Started doing this on 20150220, once we found out in the base script table, if there are tables inside there, they won't get instantiated.
        // This instantiates any tables inside the base script table in the case any script doesn't force an instantiate in its OnInit/OnReset functions.
        m_pThis->DeepCopyTables(m_pScript->GetScriptTable());
    }

    // Clone Properties table recursively.
    if (m_pScript->GetPropertiesTable())
    {
        // If entity have an archetype use shared property table.
        IEntityArchetype* pArchetype = m_pEntity->GetArchetype();
        if (!pArchetype)
        {
            // Custom properties table passed
            if (pSpawnParams && pSpawnParams->pPropertiesTable)
            {
                m_pThis->SetValue(SCRIPT_PROPERTIES_TABLE, pSpawnParams->pPropertiesTable);
            }
            else
            {
                SmartScriptTable pProps;
                pProps.Create(m_pScript->GetScriptSystem());
                pProps->Clone(m_pScript->GetPropertiesTable(), true, true);
                m_pThis->SetValue(SCRIPT_PROPERTIES_TABLE, pProps);
            }
        }
        else
        {
            IScriptTable* pPropertiesTable = pArchetype->GetProperties();
            if (pPropertiesTable)
            {
                m_pThis->SetValue(SCRIPT_PROPERTIES_TABLE, pPropertiesTable);
            }
        }

        SmartScriptTable pEntityPropertiesInstance;
        SmartScriptTable pPropsInst;
        if (m_pThis->GetValue("PropertiesInstance", pEntityPropertiesInstance))
        {
            pPropsInst.Create(m_pScript->GetScriptSystem());
            pPropsInst->Clone(pEntityPropertiesInstance, true, true);
            m_pThis->SetValue("PropertiesInstance", pPropsInst);
        }
    }

    // Set self.__this to this pointer of CComponentScript
    ScriptHandle handle;
    handle.ptr = m_pEntity;
    m_pThis->SetValue("__this", handle);
    handle.n = m_pEntity->GetId();
    m_pThis->SetValue("id", handle);
    m_pThis->SetValue("class", m_pEntity->GetClass()->GetName());
}

//////////////////////////////////////////////////////////////////////////
void CComponentScript::UpdateComponent(SEntityUpdateContext& ctx)
{
    // Update`s script function if present.
    if (m_bUpdateScript)
    {
        // Shouldn't be the case, but we must not call Lua with a 0 frametime to avoid potential FPE
        assert(ctx.fFrameTime > FLT_EPSILON);

        if (CVar::pUpdateScript->GetIVal())
        {
            m_fScriptUpdateTimer -= ctx.fFrameTime;
            if (m_fScriptUpdateTimer <= 0)
            {
                ENTITY_PROFILER
                    m_fScriptUpdateTimer = m_fScriptUpdateRate;

                //////////////////////////////////////////////////////////////////////////
                // Script Update.
                m_pScript->CallStateFunction(CurrentState(), m_pThis, ScriptState_OnUpdate, ctx.fFrameTime);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CComponentScript::ProcessEvent(SEntityEvent& event)
{
    switch (event.event)
    {
    case ENTITY_EVENT_RESET:
        // OnReset()
        m_pScript->Call_OnReset(m_pThis, event.nParam[0] == 1);
        break;
    case ENTITY_EVENT_INIT:
    {
        // Call Init.
        CallInitEvent(false);
    }
    break;
    case ENTITY_EVENT_TIMER:
        // OnTimer( nTimerId,nMilliseconds )
        m_pScript->CallStateFunction(CurrentState(), m_pThis, ScriptState_OnTimer, (int)event.nParam[0], (int)event.nParam[1]);
        break;
    case ENTITY_EVENT_XFORM:
        // OnMove()
        m_pScript->CallStateFunction(CurrentState(), m_pThis, ScriptState_OnMove);
        break;
    case ENTITY_EVENT_ATTACH:
        // OnBind( childEntity );
    {
        IEntity* pChildEntity = GetIEntitySystem()->GetEntity((EntityId)event.nParam[0]);
        if (pChildEntity)
        {
            IScriptTable* pChildEntityThis = pChildEntity->GetScriptTable();
            if (pChildEntityThis)
            {
                m_pScript->CallStateFunction(CurrentState(), m_pThis, ScriptState_OnBind, pChildEntityThis);
            }
        }
    }
    break;
    case ENTITY_EVENT_ATTACH_THIS:
        // OnBindThis( ParentEntity );
    {
        IEntity* pParentEntity = GetIEntitySystem()->GetEntity((EntityId)event.nParam[0]);
        if (pParentEntity)
        {
            IScriptTable* pParentEntityThis = pParentEntity->GetScriptTable();
            if (pParentEntityThis)
            {
                m_pScript->CallStateFunction(CurrentState(), m_pThis, ScriptState_OnBindThis, pParentEntityThis);
            }
            else
            {
                m_pScript->CallStateFunction(CurrentState(), m_pThis, ScriptState_OnBindThis);
            }
        }
    }
    break;
    case ENTITY_EVENT_DETACH:
        // OnUnbind( childEntity );
    {
        IEntity* pChildEntity = GetIEntitySystem()->GetEntity((EntityId)event.nParam[0]);
        if (pChildEntity)
        {
            IScriptTable* pChildEntityThis = pChildEntity->GetScriptTable();
            if (pChildEntityThis)
            {
                m_pScript->CallStateFunction(CurrentState(), m_pThis, ScriptState_OnUnBind, pChildEntityThis);
            }
        }
    }
    break;
    case ENTITY_EVENT_DETACH_THIS:
        // OnUnbindThis( ParentEntity );
    {
        IEntity* pParentEntity = GetIEntitySystem()->GetEntity((EntityId)event.nParam[0]);
        if (pParentEntity)
        {
            IScriptTable* pParentEntityThis = pParentEntity->GetScriptTable();
            if (pParentEntityThis)
            {
                m_pScript->CallStateFunction(CurrentState(), m_pThis, ScriptState_OnUnBindThis, pParentEntityThis);
            }
            else
            {
                m_pScript->CallStateFunction(CurrentState(), m_pThis, ScriptState_OnUnBindThis);
            }
        }
    }
    break;
    case ENTITY_EVENT_ENTERAREA:
    {
        if (m_bEnableSoundAreaEvents)
        {
            IEntity const* const pEntity = GetIEntitySystem()->GetEntity(static_cast<EntityId>(event.nParam[0]));

            if (pEntity != NULL)
            {
                IEntity const* const pTriggerEntity = GetIEntitySystem()->GetEntity(static_cast<EntityId>(event.nParam[2]));
                IScriptTable* const pTriggerEntityScript = (pTriggerEntity ? pTriggerEntity->GetScriptTable() : NULL);

                IScriptTable* const pTrgEntityThis = pEntity->GetScriptTable();

                if (pTrgEntityThis != NULL)
                {
                    if (pTriggerEntityScript != NULL)
                    {
                        m_pScript->CallStateFunction(CurrentState(), m_pThis, ScriptState_OnEnterArea, pTrgEntityThis, static_cast<int>(event.nParam[1]), event.fParam[0], pTriggerEntityScript);
                    }
                    else
                    {
                        m_pScript->CallStateFunction(CurrentState(), m_pThis, ScriptState_OnEnterArea, pTrgEntityThis, static_cast<int>(event.nParam[1]), event.fParam[0]);
                    }
                }

                // allow local player/camera source entities to trigger callback even without that entity having a scripttable
                if ((pEntity->GetFlags() & (ENTITY_FLAG_LOCAL_PLAYER | ENTITY_FLAG_CAMERA_SOURCE)) != 0)
                {
                    if (pTriggerEntityScript != NULL)
                    {
                        m_pScript->CallStateFunction(CurrentState(), m_pThis, ScriptState_OnLocalClientEnterArea, pTrgEntityThis, static_cast<int>(event.nParam[1]), event.fParam[0], pTriggerEntityScript);
                    }
                    else
                    {
                        m_pScript->CallStateFunction(CurrentState(), m_pThis, ScriptState_OnLocalClientEnterArea, pTrgEntityThis, static_cast<int>(event.nParam[1]), event.fParam[0]);
                    }
                }

                if ((pEntity->GetFlagsExtended() & (ENTITY_FLAG_EXTENDED_AUDIO_LISTENER)) != 0)
                {
                    m_pScript->CallStateFunction(CurrentState(), m_pThis, ScriptState_OnAudioListenerEnterArea, pTrgEntityThis, static_cast<int>(event.nParam[1]), event.fParam[0], event.fParam[1]);
                }
            }
        }

        break;
    }
    case ENTITY_EVENT_MOVEINSIDEAREA:
    {
        if (m_bEnableSoundAreaEvents)
        {
            IEntity const* const pEntity = GetIEntitySystem()->GetEntity(static_cast<EntityId>(event.nParam[0]));

            if (pEntity != NULL)
            {
                IScriptTable* const pTrgEntityThis = pEntity->GetScriptTable();

                if (pTrgEntityThis != NULL)
                {
                    m_pScript->CallStateFunction(CurrentState(), m_pThis, ScriptState_OnProceedFadeArea, pTrgEntityThis, static_cast<int>(event.nParam[1]), event.fParam[0]);
                }

                // allow local player/camera source entities to trigger callback even without that entity having a scripttable
                if ((pEntity->GetFlags() & (ENTITY_FLAG_LOCAL_PLAYER | ENTITY_FLAG_CAMERA_SOURCE)) != 0)
                {
                    m_pScript->CallStateFunction(CurrentState(), m_pThis, ScriptState_OnLocalClientProceedFadeArea, pTrgEntityThis, static_cast<int>(event.nParam[1]), event.fParam[0]);
                }

                if ((pEntity->GetFlagsExtended() & (ENTITY_FLAG_EXTENDED_AUDIO_LISTENER)) != 0)
                {
                    m_pScript->CallStateFunction(CurrentState(), m_pThis, ScriptState_OnAudioListenerProceedFadeArea, pTrgEntityThis, static_cast<int>(event.nParam[1]), event.fParam[0], event.fParam[1]);
                }
            }
        }

        break;
    }
    case ENTITY_EVENT_LEAVEAREA:
    {
        if (m_bEnableSoundAreaEvents)
        {
            IEntity const* const pEntity = GetIEntitySystem()->GetEntity(static_cast<EntityId>(event.nParam[0]));

            if (pEntity != NULL)
            {
                IEntity const* const pTriggerEntity = GetIEntitySystem()->GetEntity(static_cast<EntityId>(event.nParam[2]));
                IScriptTable* const pTriggerEntityScript = (pTriggerEntity ? pTriggerEntity->GetScriptTable() : NULL);

                IScriptTable* const pTrgEntityThis = pEntity->GetScriptTable();

                if (pTrgEntityThis != NULL)
                {
                    if (pTriggerEntityScript != NULL)
                    {
                        m_pScript->CallStateFunction(CurrentState(), m_pThis, ScriptState_OnLeaveArea, pTrgEntityThis, static_cast<int>(event.nParam[1]), event.fParam[0], pTriggerEntityScript);
                    }
                    else
                    {
                        m_pScript->CallStateFunction(CurrentState(), m_pThis, ScriptState_OnLeaveArea, pTrgEntityThis, static_cast<int>(event.nParam[1]), event.fParam[0]);
                    }
                }

                // allow local player/camera source entities to trigger callback even without that entity having a scripttable
                if ((pEntity->GetFlags() & (ENTITY_FLAG_LOCAL_PLAYER | ENTITY_FLAG_CAMERA_SOURCE)) != 0)
                {
                    if (pTriggerEntityScript != NULL)
                    {
                        m_pScript->CallStateFunction(CurrentState(), m_pThis, ScriptState_OnLocalClientLeaveArea, pTrgEntityThis, static_cast<int>(event.nParam[1]), event.fParam[0], pTriggerEntityScript);
                    }
                    else
                    {
                        m_pScript->CallStateFunction(CurrentState(), m_pThis, ScriptState_OnLocalClientLeaveArea, pTrgEntityThis, static_cast<int>(event.nParam[1]), event.fParam[0]);
                    }
                }

                if ((pEntity->GetFlagsExtended() & (ENTITY_FLAG_EXTENDED_AUDIO_LISTENER)) != 0)
                {
                    m_pScript->CallStateFunction(CurrentState(), m_pThis, ScriptState_OnAudioListenerLeaveArea, pTrgEntityThis, static_cast<int>(event.nParam[1]), event.fParam[0], event.fParam[1]);
                }
            }
        }

        break;
    }
    case ENTITY_EVENT_ENTERNEARAREA:
    {
        if (m_bEnableSoundAreaEvents)
        {
            IEntity const* const pEntity = GetIEntitySystem()->GetEntity(static_cast<EntityId>(event.nParam[0]));

            if (pEntity != NULL)
            {
                IScriptTable* const pTrgEntityThis = pEntity->GetScriptTable();

                if (pTrgEntityThis != NULL)
                {
                    m_pScript->CallStateFunction(CurrentState(), m_pThis, ScriptState_OnEnterNearArea, pTrgEntityThis, static_cast<int>(event.nParam[1]), event.fParam[0]);
                }

                // allow local player/camera source entities to trigger callback even without that entity having a scripttable
                if ((pEntity->GetFlags() & (ENTITY_FLAG_LOCAL_PLAYER | ENTITY_FLAG_CAMERA_SOURCE)) != 0)
                {
                    m_pScript->CallStateFunction(CurrentState(), m_pThis, ScriptState_OnLocalClientEnterNearArea, pTrgEntityThis, static_cast<int>(event.nParam[1]), event.fParam[0]);
                }

                if ((pEntity->GetFlagsExtended() & (ENTITY_FLAG_EXTENDED_AUDIO_LISTENER)) != 0)
                {
                    m_pScript->CallStateFunction(CurrentState(), m_pThis, ScriptState_OnAudioListenerEnterNearArea, pTrgEntityThis, static_cast<int>(event.nParam[1]), event.fParam[0], event.fParam[1]);
                }
            }
        }

        break;
    }
    case ENTITY_EVENT_LEAVENEARAREA:
    {
        if (m_bEnableSoundAreaEvents)
        {
            IEntity const* const pEntity = GetIEntitySystem()->GetEntity(static_cast<EntityId>(event.nParam[0]));

            if (pEntity != NULL)
            {
                IScriptTable* const pTrgEntityThis = pEntity->GetScriptTable();

                if (pTrgEntityThis != NULL)
                {
                    m_pScript->CallStateFunction(CurrentState(), m_pThis, ScriptState_OnLeaveNearArea, pTrgEntityThis, static_cast<int>(event.nParam[1]), event.fParam[0]);
                }

                // allow local player/camera source entities to trigger callback even without that entity having a scripttable
                if ((pEntity->GetFlags() & (ENTITY_FLAG_LOCAL_PLAYER | ENTITY_FLAG_CAMERA_SOURCE)) != 0)
                {
                    m_pScript->CallStateFunction(CurrentState(), m_pThis, ScriptState_OnLocalClientLeaveNearArea, pTrgEntityThis, static_cast<int>(event.nParam[1]), event.fParam[0]);
                }

                if ((pEntity->GetFlagsExtended() & (ENTITY_FLAG_EXTENDED_AUDIO_LISTENER)) != 0)
                {
                    m_pScript->CallStateFunction(CurrentState(), m_pThis, ScriptState_OnAudioListenerLeaveNearArea, pTrgEntityThis, static_cast<int>(event.nParam[1]), event.fParam[0], event.fParam[1]);
                }
            }
        }

        break;
    }
    case ENTITY_EVENT_MOVENEARAREA:
    {
        if (m_bEnableSoundAreaEvents)
        {
            IEntity const* const pEntity = GetIEntitySystem()->GetEntity(static_cast<EntityId>(event.nParam[0]));

            if (pEntity != NULL)
            {
                IScriptTable* const pTrgEntityThis = pEntity->GetScriptTable();

                if (pTrgEntityThis != NULL)
                {
                    m_pScript->CallStateFunction(CurrentState(), m_pThis, ScriptState_OnMoveNearArea, pTrgEntityThis, static_cast<int>(event.nParam[1]), event.fParam[0], event.fParam[1]);
                }

                // allow local player/camera source entities to trigger callback even without that entity having a scripttable
                if ((pEntity->GetFlags() & (ENTITY_FLAG_LOCAL_PLAYER | ENTITY_FLAG_CAMERA_SOURCE)) != 0)
                {
                    m_pScript->CallStateFunction(CurrentState(), m_pThis, ScriptState_OnLocalClientMoveNearArea, pTrgEntityThis, static_cast<int>(event.nParam[1]), event.fParam[0], event.fParam[1]);
                }

                if ((pEntity->GetFlagsExtended() & (ENTITY_FLAG_EXTENDED_AUDIO_LISTENER)) != 0)
                {
                    m_pScript->CallStateFunction(CurrentState(), m_pThis, ScriptState_OnAudioListenerMoveNearArea, pTrgEntityThis, static_cast<int>(event.nParam[1]), event.fParam[0], event.fParam[1]);
                }
            }
        }

        break;
    }
    case ENTITY_EVENT_CROSS_AREA:
    {
        // Implement in case scripts are interested in this event.
        if (m_bEnableSoundAreaEvents)
        {
        }

        break;
    }
    case ENTITY_EVENT_PHYS_BREAK:
    {
        EventPhysJointBroken* pBreakEvent = (EventPhysJointBroken*)event.nParam[0];
        if (pBreakEvent)
        {
            Vec3 pBreakPos = pBreakEvent->pt;
            int nBreakPartId = pBreakEvent->partid[0];
            int nBreakOtherEntityPartId = pBreakEvent->partid[1];
            m_pScript->CallStateFunction(CurrentState(), m_pThis, ScriptState_OnPhysicsBreak, pBreakPos, nBreakPartId, nBreakOtherEntityPartId);
        }
    }
    break;
    case ENTITY_EVENT_SOUND_DONE:
    {
        ScriptHandle oHandle;
        oHandle.n = event.nParam[1];
        m_pScript->CallStateFunction(CurrentState(), m_pThis, ScriptState_OnSoundDone, oHandle);
    }
    break;
    case ENTITY_EVENT_LEVEL_LOADED:
        m_pScript->CallStateFunction(CurrentState(), m_pThis, ScriptState_OnLevelLoaded);
        break;
    case ENTITY_EVENT_START_LEVEL:
        m_pScript->CallStateFunction(CurrentState(), m_pThis, ScriptState_OnStartLevel);
        break;
    case ENTITY_EVENT_START_GAME:
        m_pScript->CallStateFunction(CurrentState(), m_pThis, ScriptState_OnStartGame);
        break;

    case ENTITY_EVENT_PRE_SERIALIZE:
        // Kill all timers.
    {
        // If state changed kill all old timers.
        m_pEntity->KillTimer(-1);
        m_nCurrStateId = 0;
    }
    break;

    case ENTITY_EVENT_POST_SERIALIZE:
        if (m_pThis->HaveValue("OnPostLoad"))
        {
            Script::CallMethod(m_pThis, "OnPostLoad");
        }
        break;
    case ENTITY_EVENT_HIDE:
        m_pScript->CallStateFunction(CurrentState(), m_pThis, ScriptState_OnHidden);
        break;
    case ENTITY_EVENT_UNHIDE:
        m_pScript->CallStateFunction(CurrentState(), m_pThis, ScriptState_OnUnhidden);
        break;
    case ENTITY_EVENT_XFORM_FINISHED_EDITOR:
        m_pScript->Call_OnTransformFromEditorDone(m_pThis);
        break;
    }
    ;
}

//////////////////////////////////////////////////////////////////////////
bool CComponentScript::GotoState(const char* sStateName)
{
    int nStateId = m_pScript->GetStateId(sStateName);
    if (nStateId >= 0)
    {
        GotoState(nStateId);
    }
    else
    {
        EntityWarning("GotoState called with unknown state %s, in entity %s", sStateName, m_pEntity->GetEntityTextDescription());
        return false;
    }
    return true;
}

//////////////////////////////////////////////////////////////////////////
bool CComponentScript::GotoState(int nState)
{
    if (nState == m_nCurrStateId)
    {
        return true; // Already in this state.
    }
    SScriptState* pState = m_pScript->GetState(nState);

    // Call End state event.
    m_pScript->CallStateFunction(CurrentState(), m_pThis, ScriptState_OnEndState);

    // If state changed kill all old timers.
    m_pEntity->KillTimer(-1);

    SEntityEvent levent;
    levent.event = ENTITY_EVENT_LEAVE_SCRIPT_STATE;
    levent.nParam[0] = m_nCurrStateId;
    levent.nParam[1] = 0;
    m_pEntity->SendEvent(levent);

    m_nCurrStateId = nState;

    // Call BeginState event.
    m_pScript->CallStateFunction(CurrentState(), m_pThis, ScriptState_OnBeginState);

    //////////////////////////////////////////////////////////////////////////
    // Repeat check if update script function is implemented.
    m_bUpdateScript = CurrentState()->IsStateFunctionImplemented(ScriptState_OnUpdate);

    /*
    //////////////////////////////////////////////////////////////////////////
    // Check if need ResolveCollisions for OnContact script function.
    m_bUpdateOnContact = IsStateFunctionImplemented(ScriptState_OnContact);
    //////////////////////////////////////////////////////////////////////////
    */

    SEntityEvent eevent;
    eevent.event = ENTITY_EVENT_ENTER_SCRIPT_STATE;
    eevent.nParam[0] = nState;
    eevent.nParam[1] = 0;
    m_pEntity->SendEvent(eevent);

    return true;
}

//////////////////////////////////////////////////////////////////////////
bool CComponentScript::IsInState(const char* sStateName)
{
    int nStateId = m_pScript->GetStateId(sStateName);
    if (nStateId >= 0)
    {
        return nStateId == m_nCurrStateId;
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
bool CComponentScript::IsInState(int nState)
{
    return nState == m_nCurrStateId;
}

//////////////////////////////////////////////////////////////////////////
const char* CComponentScript::GetState()
{
    return m_pScript->GetStateName(m_nCurrStateId);
}

//////////////////////////////////////////////////////////////////////////
int CComponentScript::GetStateId()
{
    return m_nCurrStateId;
}

//////////////////////////////////////////////////////////////////////////
bool CComponentScript::NeedSerialize()
{
    if (!m_pThis)
    {
        return false;
    }

    if (m_fScriptUpdateRate != 0 || m_nCurrStateId != 0)
    {
        return true;
    }

    if (CVar::pEnableFullScriptSave && CVar::pEnableFullScriptSave->GetIVal())
    {
        return true;
    }

    if (m_pThis->HaveValue("OnSave") && m_pThis->HaveValue("OnLoad"))
    {
        return true;
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
void CComponentScript::SerializeProperties(TSerialize ser)
{
    if (ser.GetSerializationTarget() == eST_Network)
    {
        return;
    }

    // Saving.
    if (!(m_pEntity->GetFlags() & ENTITY_FLAG_UNREMOVABLE))
    {
        // Properties never serialized
        /*
        if (!m_pEntity->GetArchetype())
        {
            // If not archetype also serialize properties table of the entity.
            SerializeTable( ser, "Properties" );
        }*/

        // Instance table always initialized for dynamic entities.
        SerializeTable(ser, "PropertiesInstance");
    }
}

//////////////////////////////////////////////////////////////////////////
bool CComponentScript::GetSignature(TSerialize signature)
{
    signature.BeginGroup("ComponentScript");
    {
        if (m_pThis->HaveValue("OnGetPoolSignature"))
        {
            SmartScriptTable persistTable(m_pThis->GetScriptSystem());
            Script::CallMethod(m_pThis, "OnGetPoolSignature", persistTable);
            signature.Value("ScriptData", persistTable.GetPtr());
        }
    }
    signature.EndGroup();
    return true;
}

//////////////////////////////////////////////////////////////////////////
void CComponentScript::Serialize(TSerialize ser)
{
    CHECK_SCRIPT_STACK;

    if (ser.GetSerializationTarget() != eST_Network)
    {
        MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_Other, 0, "Script component serialization");

        if (NeedSerialize())
        {
            if (ser.BeginOptionalGroup("ComponentScript", true))
            {
                ser.Value("scriptUpdateRate", m_fScriptUpdateRate);
                int currStateId = m_nCurrStateId;
                ser.Value("currStateId", currStateId);

                // Simulate state change
                if (m_nCurrStateId != currStateId)
                {
                    // If state changed kill all old timers.
                    m_pEntity->KillTimer(-1);
                    m_nCurrStateId = currStateId;
                }
                if (ser.IsReading())
                {
                    // Repeat check if update script function is implemented.
                    m_bUpdateScript = CurrentState()->IsStateFunctionImplemented(ScriptState_OnUpdate);
                }

                if (CVar::pEnableFullScriptSave && CVar::pEnableFullScriptSave->GetIVal())
                {
                    ser.Value("FullScriptData", m_pThis);
                }
                else if (m_pThis->HaveValue("OnSave") && m_pThis->HaveValue("OnLoad"))
                {
                    //SerializeTable( ser, "Properties" );
                    //SerializeTable( ser, "PropertiesInstance" );
                    //SerializeTable( ser, "Events" );

                    SmartScriptTable persistTable(m_pThis->GetScriptSystem());
                    if (ser.IsWriting())
                    {
                        Script::CallMethod(m_pThis, "OnSave", persistTable);
                    }
                    ser.Value("ScriptData", persistTable.GetPtr());
                    if (ser.IsReading())
                    {
                        Script::CallMethod(m_pThis, "OnLoad", persistTable);
                    }
                }

                ser.EndGroup(); //ComponentScript
            }
            else
            {
                // If we haven't already serialized the script component then reset it back to the default state.

                CRY_ASSERT(m_pScript);

                m_pScript->Call_OnReset(m_pThis, true);
            }
        }
    }
    else
    {
        int stateId = m_nCurrStateId;
        ser.Value("currStateId", stateId, 'sSts');
        if (ser.IsReading())
        {
            GotoState(stateId);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
bool CComponentScript::HaveTable(const char* name)
{
    SmartScriptTable table;
    if (m_pThis && m_pThis->GetValue(name, table))
    {
        return true;
    }
    else
    {
        return false;
    }
}

//////////////////////////////////////////////////////////////////////////
void CComponentScript::SerializeTable(TSerialize ser, const char* name)
{
    CHECK_SCRIPT_STACK;

    SmartScriptTable table;
    if (ser.IsReading())
    {
        if (ser.BeginOptionalGroup(name, true))
        {
            table = SmartScriptTable(m_pThis->GetScriptSystem());
            ser.Value("table", table);
            m_pThis->SetValue(name, table);
            ser.EndGroup();
        }
    }
    else
    {
        if (m_pThis->GetValue(name, table))
        {
            ser.BeginGroup(name);
            ser.Value("table", table);
            ser.EndGroup();
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CComponentScript::SerializeXML(XmlNodeRef& entityNode, bool bLoading)
{
    // Initialize script properties.
    if (bLoading)
    {
        CScriptProperties scriptProps;
        // Initialize properties.
        scriptProps.SetProperties(entityNode, m_pThis);

        XmlNodeRef eventTargets = entityNode->findChild("EventTargets");
        if (eventTargets)
        {
            SetEventTargets(eventTargets);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
struct SEntityEventTarget
{
    string event;
    EntityId target;
    string sourceEvent;
};

//////////////////////////////////////////////////////////////////////////
// Set event targets from the XmlNode exported by Editor.
void CComponentScript::SetEventTargets(XmlNodeRef& eventTargetsNode)
{
    std::set<string> sourceEvents;
    std::vector<SEntityEventTarget> eventTargets;

    IScriptSystem* pSS = GetIScriptSystem();

    for (int i = 0; i < eventTargetsNode->getChildCount(); i++)
    {
        XmlNodeRef eventNode = eventTargetsNode->getChild(i);

        SEntityEventTarget et;
        et.event = eventNode->getAttr("Event");
        if (!eventNode->getAttr("Target", et.target))
        {
            et.target = 0; // failed reading...
        }
        et.sourceEvent = eventNode->getAttr("SourceEvent");

        if (et.event.empty() || !et.target || et.sourceEvent.empty())
        {
            continue;
        }

        eventTargets.push_back(et);
        sourceEvents.insert(et.sourceEvent);
    }
    if (eventTargets.empty())
    {
        return;
    }

    SmartScriptTable pEventsTable;

    if (!m_pThis->GetValue("Events", pEventsTable))
    {
        pEventsTable = pSS->CreateTable();
        // assign events table to the entity self script table.
        m_pThis->SetValue("Events", pEventsTable);
    }

    for (std::set<string>::iterator it = sourceEvents.begin(); it != sourceEvents.end(); ++it)
    {
        SmartScriptTable pTrgEvents(pSS);

        string sourceEvent = *it;

        pEventsTable->SetValue(sourceEvent.c_str(), pTrgEvents);

        // Put target events to table.
        int trgEventIndex = 1;
        for (size_t i = 0; i < eventTargets.size(); i++)
        {
            SEntityEventTarget& et = eventTargets[i];
            if (et.sourceEvent == sourceEvent)
            {
                SmartScriptTable pTrgEvent(pSS);

                pTrgEvents->SetAt(trgEventIndex, pTrgEvent);
                trgEventIndex++;
                ScriptHandle hdl;
                hdl.n = et.target;
                pTrgEvent->SetAt(1, hdl);
                pTrgEvent->SetAt(2, et.event.c_str());
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CComponentScript::CallEvent(const char* sEvent)
{
    m_pScript->CallEvent(m_pThis, sEvent, (bool)true);
}

//////////////////////////////////////////////////////////////////////////
void CComponentScript::CallEvent(const char* sEvent, float fValue)
{
    m_pScript->CallEvent(m_pThis, sEvent, fValue);
}

//////////////////////////////////////////////////////////////////////////
void CComponentScript::CallEvent(const char* sEvent, double fValue)
{
    m_pScript->CallEvent(m_pThis, sEvent, fValue);
}

//////////////////////////////////////////////////////////////////////////
void CComponentScript::CallEvent(const char* sEvent, bool bValue)
{
    m_pScript->CallEvent(m_pThis, sEvent, bValue);
}

//////////////////////////////////////////////////////////////////////////
void CComponentScript::CallEvent(const char* sEvent, const char* sValue)
{
    m_pScript->CallEvent(m_pThis, sEvent, sValue);
}

//////////////////////////////////////////////////////////////////////////
void CComponentScript::CallEvent(const char* sEvent, EntityId nEntityId)
{
    IScriptTable* pTable = 0;
    IEntity* pEntity = GetIEntitySystem()->GetEntity(nEntityId);
    if (pEntity)
    {
        pTable = pEntity->GetScriptTable();
    }
    m_pScript->CallEvent(m_pThis, sEvent, pTable);
}

//////////////////////////////////////////////////////////////////////////
void CComponentScript::CallEvent(const char* sEvent, const Vec3& vValue)
{
    m_pScript->CallEvent(m_pThis, sEvent, vValue);
}

//////////////////////////////////////////////////////////////////////////
void CComponentScript::CallInitEvent(bool bFromReload)
{
    m_pScript->Call_OnInit(m_pThis, bFromReload);
}

//////////////////////////////////////////////////////////////////////////
void CComponentScript::OnPreparedFromPool()
{
    m_pScript->CallStateFunction(CurrentState(), m_pThis, ScriptState_OnPreparedFromPool);
}


//////////////////////////////////////////////////////////////////////////
void CComponentScript::OnCollision(IEntity* pTarget, int matId, const Vec3& pt, const Vec3& n, const Vec3& vel, const Vec3& targetVel, int partId, float mass)
{
    if (!CurrentState()->IsStateFunctionImplemented(ScriptState_OnCollision))
    {
        return;
    }

    FUNCTION_PROFILER(GetISystem(), PROFILE_ENTITY);

    if (!m_hitTable)
    {
        m_hitTable.Create(gEnv->pScriptSystem);
    }
    {
        Vec3 dir(0, 0, 0);
        CScriptSetGetChain chain(m_hitTable);
        chain.SetValue("normal", n);
        chain.SetValue("pos", pt);

        if (vel.GetLengthSquared() > 1e-6f)
        {
            dir = vel.GetNormalized();
            chain.SetValue("dir", dir);
        }

        chain.SetValue("velocity", vel);
        chain.SetValue("target_velocity", targetVel);
        chain.SetValue("target_mass", mass);
        chain.SetValue("partid", partId);
        chain.SetValue("backface", n.Dot(dir) >= 0);
        chain.SetValue("materialId", matId);

        if (pTarget)
        {
            ScriptHandle sh;
            sh.n = pTarget->GetId();

            if (pTarget->GetPhysics())
            {
                chain.SetValue("target_type", (int)pTarget->GetPhysics()->GetType());
            }
            else
            {
                chain.SetToNull("target_type");
            }

            chain.SetValue("target_id", sh);

            if (pTarget->GetScriptTable())
            {
                chain.SetValue("target", pTarget->GetScriptTable());
            }
        }
        else
        {
            chain.SetToNull("target_type");
            chain.SetToNull("target_id");
            chain.SetToNull("target");
        }
    }

    m_pScript->CallStateFunction(CurrentState(), m_pThis, ScriptState_OnCollision, m_hitTable);
}

//////////////////////////////////////////////////////////////////////////
void CComponentScript::SendScriptEvent(int Event, IScriptTable* pParamters, bool* pRet)
{
    SScriptState* pState = CurrentState();
    for (int i = 0; i < NUM_STATES; i++)
    {
        if (m_pScript->ShouldExecuteCall(i) && pState->pStateFuns[i] && pState->pStateFuns[i]->pFunction[ScriptState_OnEvent])
        {
            if (pRet)
            {
                Script::CallReturn(GetIScriptSystem(), pState->pStateFuns[i]->pFunction[ScriptState_OnEvent], m_pThis, Event, pParamters, *pRet);
            }
            else
            {
                Script::Call(GetIScriptSystem(), pState->pStateFuns[i]->pFunction[ScriptState_OnEvent], m_pThis, Event, pParamters);
            }
            pRet = 0;
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CComponentScript::SendScriptEvent(int Event, const char* str, bool* pRet)
{
    SScriptState* pState = CurrentState();
    for (int i = 0; i < NUM_STATES; i++)
    {
        if (m_pScript->ShouldExecuteCall(i) && pState->pStateFuns[i] && pState->pStateFuns[i]->pFunction[ScriptState_OnEvent])
        {
            if (pRet)
            {
                Script::CallReturn(GetIScriptSystem(), pState->pStateFuns[i]->pFunction[ScriptState_OnEvent], m_pThis, Event, str, *pRet);
            }
            else
            {
                Script::Call(GetIScriptSystem(), pState->pStateFuns[i]->pFunction[ScriptState_OnEvent], m_pThis, Event, str);
            }
            pRet = 0;
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CComponentScript::SendScriptEvent(int Event, int nParam, bool* pRet)
{
    SScriptState* pState = CurrentState();
    for (int i = 0; i < NUM_STATES; i++)
    {
        if (m_pScript->ShouldExecuteCall(i) && pState->pStateFuns[i] && pState->pStateFuns[i]->pFunction[ScriptState_OnEvent])
        {
            if (pRet)
            {
                Script::CallReturn(GetIScriptSystem(), pState->pStateFuns[i]->pFunction[ScriptState_OnEvent], m_pThis, Event, nParam, *pRet);
            }
            else
            {
                Script::Call(GetIScriptSystem(), pState->pStateFuns[i]->pFunction[ScriptState_OnEvent], m_pThis, Event, nParam);
            }
            pRet = 0;
        }
    }
}

void CComponentScript::RegisterForAreaEvents(bool bEnable)
{
    m_bEnableSoundAreaEvents = bEnable;
}

bool CComponentScript::IsRegisteredForAreaEvents() const
{
    return m_bEnableSoundAreaEvents;
}

void CComponentScript::GetMemoryUsage(ICrySizer* pSizer) const
{
    pSizer->AddObject(this, sizeof(*this));
}


