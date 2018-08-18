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
#include "Entity.h"
#include "AffineParts.h"
#include "EntitySystem.h"
#include "IEntityClass.h"
#include "ISerialize.h"
#include "PartitionGrid.h"
#include "ProximityTriggerSystem.h"
#include "EntityObject.h"
#include <IAISystem.h>
#include <IAgent.h>
#include <IAIActorProxy.h>
#include "IRenderAuxGeom.h"
#include "ICryAnimation.h"
#include "IAIObjectManager.h"
#include "IAIActor.h"
#include "EntityLayer.h"
#include "GeomCacheAttachmentManager.h"
#include "CharacterBoneAttachmentManager.h"
#include "StringUtils.h"
#include "Components/ComponentEntityAttributes.h"
#include "Components/ComponentClipVolume.h"
#include "Components/ComponentAudio.h"
#include "Components/ComponentArea.h"
#include "Components/ComponentFlowGraph.h"
#include "Components/ComponentTrigger.h"
#include "Components/ComponentEntityNode.h"
#include "Components/ComponentCamera.h"
#include "Components/ComponentRope.h"
#include "Components/ComponentSerialization.h"
#include "Components/IComponentUser.h"
#include "DynamicResponseProxy.h"

// enable this to check nan's on position updates... useful for debugging some weird crashes
#define ENABLE_NAN_CHECK

#ifdef ENABLE_NAN_CHECK
#define CHECKQNAN_FLT(x) \
    assert(*alias_cast<unsigned*>(&x) != 0xffffffffu)
#else
#define CHECKQNAN_FLT(x) (void*)0
#endif

#define CHECKQNAN_VEC(v) \
    CHECKQNAN_FLT(v.x); CHECKQNAN_FLT(v.y); CHECKQNAN_FLT(v.z)

namespace
{
    Matrix34 sIdentityMatrix = Matrix34::CreateIdentity();
}

string CEntity::m_szDescription;

//////////////////////////////////////////////////////////////////////////
CEntity::CEntity(SEntitySpawnParams& params)
{
    m_pClass = params.pClass;
    m_pArchetype = params.pArchetype;
    m_nID = params.id;
    m_flags = params.nFlags;
    m_flagsExtended = params.nFlagsExtended;
    m_guid = params.guid;

    // Set flags.
    m_bActive = 0;
    m_bInActiveList = 0;
    m_bAllowDestroyOrCreateComponent = true;

    m_bBoundsValid = 0;
    m_bInitialized = 0;
    m_bHidden = 0;
    m_bInvisible = 0;
    m_bGarbage = 0;
    m_nUpdateCounter = 0;
    m_bHaveEventListeners = 0;
    m_bTrigger = 0;
    m_bWasRellocated = 0;
    m_bNotInheritXform = 0;
    m_bInShutDown = 0;
    m_bIsFromPool = 0;
    m_bLoadedFromLevelFile = 0;

    m_pGridLocation = 0;
    m_pProximityEntity = 0;

    m_eUpdatePolicy = ENTITY_UPDATE_NEVER;
    m_pBinds = NULL;
    m_aiObjectID = INVALID_AIOBJECTID;

    m_pEntityLinks = 0;

    // Forward dir cache is initially invalid
    m_bDirtyForwardDir = true;

    //////////////////////////////////////////////////////////////////////////
    // Initialize basic parameters.
    //////////////////////////////////////////////////////////////////////////
    if (params.sName && params.sName[0] != '\0')
    {
        SetName(params.sName);
    }
    CHECKQNAN_VEC(params.vPosition);
    m_vPos = params.vPosition;
    m_qRotation = params.qRotation;
    m_vScale = params.vScale;

    CalcLocalTM(m_worldTM);

    ComputeForwardDir();

    //////////////////////////////////////////////////////////////////////////
    // Register the serialization component
    GetEntitySystem()->CreateComponentAndRegister<ComponentSerialization>(IComponent::SComponentInitializer(this));

    //////////////////////////////////////////////////////////////////////////
    // Check if entity needs to create a ComponentUser class.
    IEntityClass::ComponentUserCreateFunc pComponentUserCreateFunc = m_pClass->GetComponentUserCreateFunc();
    if (pComponentUserCreateFunc)
    {
        pComponentUserCreateFunc(this, params, m_pClass->GetComponentUserData());
    }

    if (IEntityPropertyHandler* pPropertyHandler = m_pClass->GetPropertyHandler())
    {
        if (IEntityArchetype* pArchetype = GetArchetype())
        {
            pPropertyHandler->InitArchetypeEntity(this, pArchetype->GetName(), params);
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // Check if entity needs to create a script component.
    IEntityScript* pEntityScript = m_pClass->GetIEntityScript();
    if (pEntityScript)
    {
        // Creates a script component.
        CreateScriptComponent(this, pEntityScript, &params);
    }

    m_nKeepAliveCounter = 0;

    m_cloneLayerId = -1;
}

//////////////////////////////////////////////////////////////////////////
CEntity::~CEntity()
{
    assert(m_nKeepAliveCounter == 0);

    // Components could still be referring to m_szName, so clear them before it gets destroyed
    m_components.clear();
}

IEntitySystem* CEntity::GetEntitySystem() const
{
    return g_pIEntitySystem;
}

//////////////////////////////////////////////////////////////////////////
bool CEntity::ReloadEntity(SEntityLoadParams& loadParams)
{
    bool bResult = false;

    SEntitySpawnParams& params = loadParams.spawnParams;
    XmlNodeRef& entityNode = loadParams.spawnParams.entityNode;

    //////////////////////////////////////////////////////////////////////////
    // Shutdown to clean out old information
    //////////////////////////////////////////////////////////////////////////
    const bool bKeepAI = (params.nFlags & ENTITY_FLAG_HAS_AI) ? true : false;
    ShutDown(!bKeepAI, false);

    // Notify script so it can clean up anything in Lua
    {
        IComponentScriptPtr scriptComponent = GetComponent<IComponentScript>();
        IScriptTable* pScriptTable = scriptComponent ? scriptComponent->GetScriptTable() : NULL;
        if (pScriptTable && pScriptTable->HaveValue("OnBeingReused"))
        {
            Script::CallMethod(pScriptTable, "OnBeingReused");
        }
    }

    // Reset the entity id and guid
    params.prevId = GetId();
    params.prevGuid = GetGuid();
    if (g_pIEntitySystem->ResetEntityId(this, params.id))
    {
        // If entity spawns from archetype take class from archetype
        if (params.pArchetype)
        {
            params.pClass = params.pArchetype->GetClass();
        }
        assert(params.pClass != NULL);   // Class must always be specified

        m_pClass = params.pClass;
        m_pArchetype = params.pArchetype;
        m_nID = params.id;
        m_guid = params.guid;
        m_flags = params.nFlags;
        m_flagsExtended = params.nFlagsExtended;

        IAIObject* pAI = GetAIObject();
        if (pAI)
        {
            m_flags |= ENTITY_FLAG_HAS_AI;
            pAI->SetEntityID(params.id);
        }
        else
        {
            m_flags &= ~ENTITY_FLAG_HAS_AI;
        }

        if (params.sName && params.sName[0] != '\0')
        {
            //renaming level entities at this point is ok, make sure to "fake" non-initialized state
            m_bInitialized = false;
            SetName(params.sName);
            m_bInitialized = true;
        }

        // Set flags.
        m_bActive = 0;
        m_bInActiveList = 0;

        m_bBoundsValid = 0;
        m_bHidden = 0;
        m_bInvisible = 0;
        m_bGarbage = 0;
        m_nUpdateCounter = 0;
        m_bTrigger = 0; // Based on ComponentTrigger
        m_bWasRellocated = 0;
        m_bNotInheritXform = 0;
        m_bDirtyForwardDir = true; // Forward dir cache is initially invalid

        //m_eUpdatePolicy = ENTITY_UPDATE_NEVER;

        //////////////////////////////////////////////////////////////////////////
        // Initialize basic parameters.
        //////////////////////////////////////////////////////////////////////////
        CHECKQNAN_VEC(params.vPosition);
        m_vPos = params.vPosition;
        m_qRotation = params.qRotation;
        m_vScale = params.vScale;

        CalcLocalTM(m_worldTM);

        ComputeForwardDir();

        // Referesh proximity trigger
        if (m_pProximityEntity)
        {
            m_pProximityEntity->id = AZ::EntityId(m_nID);
            OnRellocate(ENTITY_XFORM_POS);
        }

        if (entityNode)
        {
            if (IEntityPropertyHandler* pPropertyHandler = m_pClass->GetPropertyHandler())
            {
                pPropertyHandler->LoadEntityXMLProperties(this, entityNode);
            }

            if (IEntityEventHandler* pEventHandler = m_pClass->GetEventHandler())
            {
                pEventHandler->LoadEntityXMLEvents(this, entityNode);
            }
        }

        //////////////////////////////////////////////////////////////////////////
        // Reload all components
        //////////////////////////////////////////////////////////////////////////

        // Serialize and init the script component first
        IComponentScriptPtr scriptComponent = GetComponent<IComponentScript>();
        if (scriptComponent)
        {
            scriptComponent->Reload(this, params);

            if (entityNode)
            {
                if (IComponentSerializationPtr serializationComponent = GetComponent<IComponentSerialization>())
                {
                    serializationComponent->SerializeXMLOnly(entityNode, true, { IComponentScript::Type() });
                }
            }
        }

        // Reload components (except script)
        ForEachComponent([this, &params](const TComponentPair& pair)
            {
                if (pair.first != IComponentScript::Type() && pair.second)
                {
                    pair.second->Reload(this, params);
                }
            });

        IComponentRenderPtr pRenderComponent = GetComponent<IComponentRender>();
        if (pRenderComponent)
        {
            pRenderComponent->PostInit();
        }

        // Make sure position is registered.
        if (!m_bWasRellocated)
        {
            OnRellocate(ENTITY_XFORM_POS);
        }

        bResult = true;
    }

    return bResult;
}

//////////////////////////////////////////////////////////////////////////
void CEntity::SetPoolControl(bool bSet)
{
    m_bIsFromPool = bSet;

    if (bSet)
    {
        // Initially turned off
        Activate(false);
        Hide(true);
    }
}

//////////////////////////////////////////////////////////////////////////
void CEntity::SetName(const char* sName)
{
    if ((m_flags & ENTITY_FLAG_UNREMOVABLE) && m_bInitialized && !gEnv->IsEditor())
    {
        CryWarning(VALIDATOR_MODULE_ENTITYSYSTEM, VALIDATOR_ERROR, "!Unremovable entities should never change their name! This is non-compatible with C2 savegames.");
    }

    g_pIEntitySystem->ChangeEntityName(this, sName);
    if (IAIObject* pAIObject = GetAIObject())
    {
        pAIObject->SetName(sName);
    }
}

//////////////////////////////////////////////////////////////////////////
void CEntity::SetFlags(uint32 flags)
{
    if (flags != m_flags)
    {
        m_flags = flags;
        IComponentRenderPtr pRenderComponent = GetComponent<IComponentRender>();
        if (pRenderComponent)
        {
            pRenderComponent->UpdateEntityFlags();
        }
        if (m_pGridLocation)
        {
            m_pGridLocation->nEntityFlags = flags;
        }
    }
};

//////////////////////////////////////////////////////////////////////////
bool CEntity::SendEvent(SEntityEvent& event)
{
    FUNCTION_PROFILER_FAST(GetISystem(), PROFILE_ENTITY, g_bProfilerEnabled);

    CTimeValue t0;
    if (CVar::es_DebugEvents)
    {
        t0 = gEnv->pTimer->GetAsyncTime();
    }

    if (CVar::es_DisableTriggers)
    {
        static IEntityClass* pProximityTriggerClass = NULL;
        static IEntityClass* pAreaTriggerClass = NULL;

        if (pProximityTriggerClass == NULL || pAreaTriggerClass == NULL)
        {
            IEntitySystem* pEntitySystem = gEnv->pEntitySystem;

            if (pEntitySystem != NULL)
            {
                IEntityClassRegistry* pClassRegistry = pEntitySystem->GetClassRegistry();

                if (pClassRegistry != NULL)
                {
                    pProximityTriggerClass = pClassRegistry->FindClass("ProximityTrigger");
                    pAreaTriggerClass = pClassRegistry->FindClass("AreaTrigger");
                }
            }
        }

        IEntityClass* pEntityClass = GetClass();
        if (pEntityClass == pProximityTriggerClass || pEntityClass == pAreaTriggerClass)
        {
            if (event.event == ENTITY_EVENT_ENTERAREA || event.event == ENTITY_EVENT_LEAVEAREA)
            {
                return true;
            }
        }
    }

    // AI object position must be updated first so components could eventually use it (as CSmartObjects does)
    switch (event.event)
    {
    case ENTITY_EVENT_XFORM:
        if (!m_bGarbage)
        {
            UpdateAIObject();
        }
        break;
    case ENTITY_EVENT_ENTERAREA:
    case ENTITY_EVENT_HIDE:
    case ENTITY_EVENT_UNHIDE:
    {
        if (!m_bGarbage)
        {
            // Notify audio components before script components!
            IComponentAudioPtr const pAudioComponent = GetComponent<IComponentAudio>();

            if (pAudioComponent != NULL)
            {
                pAudioComponent->ProcessEvent(event);
            }
        }

        break;
    }
    case ENTITY_EVENT_RESET:
    {
        // Activate entity if was deactivated:
        if (m_bGarbage)
        {
            m_bGarbage = false;
            // If entity was deleted in game, ressurect it.
            SEntityEvent entevnt;
            entevnt.event = ENTITY_EVENT_INIT;
            SendEvent(entevnt);
        }

        //CryLogAlways( "%s became visible",GetEntityTextDescription() );
        // [marco] needed when going in and out of game mode in the editor
        IComponentRenderPtr pRenderComponent = GetComponent<IComponentRender>();
        if (pRenderComponent)
        {
            pRenderComponent->SetLastSeenTime(gEnv->pTimer->GetCurrTime());
            ICharacterInstance* pCharacterInstance = pRenderComponent->GetCharacter(0);
            if (pCharacterInstance)
            {
                pCharacterInstance->SetPlaybackScale(1.0f);
            }
        }
        break;
    }
    case ENTITY_EVENT_INIT:
        m_bGarbage = false;
        break;

    case ENTITY_EVENT_ANIM_EVENT:
    {
        // If the event is a sound event, make sure we have a sound proxy.
        const AnimEventInstance* pAnimEvent = reinterpret_cast<const AnimEventInstance*>(event.nParam[0]);
        const char* eventName = (pAnimEvent ? pAnimEvent->m_EventName : 0);
        if (eventName && _stricmp(eventName, "sound") == 0)
        {
            GetOrCreateComponent<IComponentAudio>();
        }
    }
    break;

    case ENTITY_EVENT_DONE:
        // When deleting should detach all children.
    {
        //g_pIEntitySystem->RemoveTimerEvent(GetId(), -1);
        DeallocBindings();
        IPhysicalEntity* pPhysics = GetPhysics();
        if (pPhysics && pPhysics->GetForeignData(PHYS_FOREIGN_ID_ENTITY))
        {
            pe_params_foreign_data pfd;
            pfd.pForeignData = 0;
            pfd.iForeignData = -1;
            pPhysics->SetParams(&pfd, 1);
        }
    }
    break;

    case ENTITY_EVENT_PRE_SERIALIZE:
        //filter out event if not using save/load
        if (!g_pIEntitySystem->ShouldSerializedEntity(this))
        {
            return true;
        }
        break;
    }

    if (!m_bGarbage)
    {
        // Broadcast event to components.
        uint32 nWhyFlags = (uint32)event.nParam[0];

        // Ensure IComponentUser receives events before any IGameObjectExtensions.
        // Not sure if this is necessary, but that was the order before we
        // removed the concept of "IEntityProxy".
        IComponentUserPtr userComponent = GetComponent<IComponentUser>();
        if (userComponent)
        {
            userComponent->ProcessEvent(event);
        }

        if (event.event == ENTITY_EVENT_INIT)
        {
            //[AlexMcC|12.07.10] Follow the same proxy order as CEntity::Init
            ForEachComponent([&event](const TComponentPair& componentPair)
                {
                    if (componentPair.first != IComponentRender::Type()
                        && componentPair.first != IComponentScript::Type()
                        && componentPair.first != IComponentUser::Type())
                    {
                        componentPair.second->ProcessEvent(event);
                    }
                });

            IComponentScriptPtr scriptComponent = GetComponent<IComponentScript>(); // send to scriptComponent later, since it might depend on the state of the other components (for init events)
            if (scriptComponent)
            {
                scriptComponent->ProcessEvent(event);
            }

            IComponentRenderPtr pRenderComponent = GetComponent<IComponentRender>(); // send to render component last to match CEntity::Init()
            if (pRenderComponent)
            {
                pRenderComponent->ProcessEvent(event);
            }
        }
        else
        {
            // Lambda function to process the event on components
            auto pProcessEventFunc = [&event](const TComponentPair& componentPair)
                {
                    if (componentPair.first != IComponentUser::Type())
                    {
                        componentPair.second->ProcessEvent(event);
                    }
                };

            // If it is a timer event then allow to destroy/create entities inside the loop as it might be needed.
            if (event.event == ENTITY_EVENT_TIMER)
            {
                ForEachComponent_AllowDestroyCreate(pProcessEventFunc);
            }
            else
            {
                ForEachComponent(pProcessEventFunc);
            }
        }


        // Give entity system a chance to check the event, and notify other listeners.
        if (event.event != ENTITY_EVENT_XFORM || !(nWhyFlags & ENTITY_XFORM_NO_SEND_TO_ENTITY_SYSTEM))
        {
            g_pIEntitySystem->OnEntityEvent(this, event);
        }

#ifndef _RELEASE
        if (CVar::es_DebugEvents)
        {
            CTimeValue t1 = gEnv->pTimer->GetAsyncTime();
            LogEvent(event, t1 - t0);
        }
#endif

        return true;
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
bool CEntity::Init(SEntitySpawnParams& params)
{
    MEMSTAT_CONTEXT_FMT(EMemStatContextTypes::MSC_Entity, 0, "Init: %s", params.sName ? params.sName : "(noname)");

    // Initialize all currently existing components.
    // Allow component destroy/create since CGameObject creates IGameObjectExtensions during its InitComponent().
    bool initSuccessAll = true;
    ForEachComponent_AllowDestroyCreate([this, &params, &initSuccessAll](const TComponentPair& componentPair)
        {
            if (componentPair.first != IComponentScript::Type())
            {
                if (!componentPair.second->InitComponent(this, params))
                {
                    gEnv->pLog->LogError("Couldn't create entity %s: component %s couldn't be initialized", params.sName, componentPair.first.Name());
                    initSuccessAll = false;
                }
            }
        });

    if (!initSuccessAll)
    {
        return false;
    }

    // Script Component is initialized last.
    IComponentScriptPtr scriptComponent = GetComponent<IComponentScript>();
    if (scriptComponent)
    {
        scriptComponent->InitComponent(this, params);
    }

    // Make sure position is registered.
    if (!m_bWasRellocated)
    {
        OnRellocate(ENTITY_XFORM_POS);
    }

    // Render component has a PostInit.
    IComponentRenderPtr pRenderComponent = GetComponent<IComponentRender>();
    if (pRenderComponent)
    {
        pRenderComponent->PostInit();
    }
    m_bInitialized = true;

    return true;
}

//////////////////////////////////////////////////////////////////////////
void CEntity::Update(SEntityUpdateContext& ctx)
{
    FUNCTION_PROFILER_FAST(GetISystem(), PROFILE_ENTITY, g_bProfilerEnabled);

    if (m_bHidden && !CheckFlags(ENTITY_FLAG_UPDATE_HIDDEN))
    {
        return;
    }

    // Broadcast event to all components (update render component last)
    ForEachComponent_AllowDestroyCreate([&ctx](const TComponentPair& componentPair)
        {
            if (componentPair.first != IComponentRender::Type())
            {
                componentPair.second->UpdateComponent(ctx);
            }
        });

    IComponentRenderPtr pRenderComponent = GetComponent<IComponentRender>();
    if (pRenderComponent)
    {
        pRenderComponent->UpdateComponent(ctx);
    }

    if (m_nUpdateCounter != 0)
    {
        if (--m_nUpdateCounter == 0)
        {
            SetUpdateStatus();
        }
    }
}

void CEntity::PrePhysicsUpdate(float fFrameTime)
{
    FUNCTION_PROFILER_FAST(GetISystem(), PROFILE_ENTITY, g_bProfilerEnabled);

    SEntityEvent evt(ENTITY_EVENT_PREPHYSICSUPDATE);
    evt.fParam[0] = fFrameTime;

    // Broadcast to all components (update render component last)
    ForEachComponent([&evt](const TComponentPair& componentPair)
        {
            if (componentPair.first != IComponentRender::Type())
            {
                componentPair.second->ProcessEvent(const_cast<SEntityEvent&> (evt));
            }
        });

    IComponentRenderPtr pRenderComponent = GetComponent<IComponentRender>();
    if (pRenderComponent)
    {
        pRenderComponent->ProcessEvent(evt);
    }
}

bool CEntity::IsPrePhysicsActive()
{
    return g_pIEntitySystem->IsPrePhysicsActive(this);
}

//////////////////////////////////////////////////////////////////////////
void CEntity::ShutDown(bool bRemoveAI /*= true*/, bool bRemoveComponents /*= true*/)
{
    ENTITY_PROFILER


        m_bInShutDown = true;

    RemoveAllEntityLinks();

    // Remove entity name from the map.
    g_pIEntitySystem->ChangeEntityName(this, "");

    if (m_pGridLocation)
    {
        // Free grid location.
        g_pIEntitySystem->GetPartitionGrid()->FreeLocation(m_pGridLocation);
        m_pGridLocation = 0;
        m_flags |= ENTITY_FLAG_NO_PROXIMITY;
    }

    if (m_pProximityEntity)
    {
        g_pIEntitySystem->GetProximityTriggerSystem()->RemoveEntity(m_pProximityEntity, true);
        m_pProximityEntity = 0;
    }

    if (m_flags & ENTITY_FLAG_TRIGGER_AREAS)
    {
        static_cast<CAreaManager*>(g_pIEntitySystem->GetAreaManager())->ExitAllAreas(this);
    }


    //////////////////////////////////////////////////////////////////////////
    // release AI object and SmartObject
    if (bRemoveAI)
    {
        if (gEnv->pAISystem)
        {
            if (IAIObject* pAIObject = GetAIObject())
            {
                gEnv->pAISystem->GetAIObjectManager()->RemoveObject(m_aiObjectID);
            }
        }

        m_aiObjectID = INVALID_AIOBJECTID;
        m_flags &= ~ENTITY_FLAG_HAS_AI;
    }

    if (gEnv->pAISystem && gEnv->pAISystem->GetSmartObjectManager())
    {
        gEnv->pAISystem->GetSmartObjectManager()->RemoveSmartObject(this);
    }

    //////////////////////////////////////////////////////////////////////////
    // remove timers and listeners
    g_pIEntitySystem->RemoveEntityEventListeners(this);
    g_pIEntitySystem->RemoveTimerEvent(GetId(), -1);
    g_pIEntitySystem->RemoveEntityFromLayers(GetId());

    //////////////////////////////////////////////////////////////////////////
    // Release all components.
    if (bRemoveComponents)
    {
        // Call Done() on each component, but call it for CGameObject first.
        // CGameObject's Done() removes IGameObjectExtensions from the Entity
        // one-at-a-time and does a lot of special-case teardown.
        IComponentUserPtr userComponent = GetComponent<IComponentUser>();
        if (userComponent)
        {
            userComponent->Done();
        }

        ForEachComponent([](const TComponentPair& componentPair)
            {
                if (componentPair.first != IComponentUser::Type())
                {
                    componentPair.second->Done();
                }
            });

        // swap container contents to relinquish all memory
        m_bAllowDestroyOrCreateComponent = false;
        {
            TComponents doneComponents;
            m_components.swap(doneComponents);
        }
        m_bAllowDestroyOrCreateComponent = true;
    }
    //////////////////////////////////////////////////////////////////////////

    DeallocBindings();

    m_bInShutDown = false;
}

//////////////////////////////////////////////////////////////////////////
void CEntity::EnableInheritXForm(bool bEnable)
{
    m_bNotInheritXform = !bEnable;
}

//////////////////////////////////////////////////////////////////////////
void CEntity::AttachChild(IEntity* pChildEntity, const SChildAttachParams& attachParams)
{
    assert(pChildEntity);
    // In debug mode check for attachment recursion.
#ifdef _DEBUG
    assert(this != pChildEntity && "Trying to Attach to itself");
    for (IEntity* pParent = GetParent(); pParent; pParent = pParent->GetParent())
    {
        assert(pParent != pChildEntity && "Recursive Attachment");
    }
#endif
    if (pChildEntity == this)
    {
        EntityWarning("Trying to attaching Entity %s to itself", GetEntityTextDescription());
        return;
    }

    // Child entity must be also CEntity.
    CEntity* pChild = reinterpret_cast<CEntity*>(pChildEntity);
    PREFAST_ASSUME(pChild);

    // If not already attached to this node.
    if (pChild->m_pBinds && pChild->m_pBinds->pParent == this)
    {
        return;
    }

    // Make sure binding structure allocated for entity and child.
    AllocBindings();
    pChild->AllocBindings();

    assert(m_pBinds);
    assert(pChild->m_pBinds);

    Matrix34 childTM;
    if (attachParams.m_nAttachFlags & ATTACHMENT_KEEP_TRANSFORMATION)
    {
        childTM = pChild->GetWorldTM();
    }

    if (attachParams.m_nAttachFlags & ATTACHMENT_GEOMCACHENODE)
    {
#if defined(USE_GEOM_CACHES)
        pChild->m_pBinds->parentBindingType = SBindings::eBT_GeomCacheNode;
        CGeomCacheAttachmentManager* pGeomCacheAttachmentManager = static_cast<CEntitySystem*>(GetEntitySystem())->GetGeomCacheAttachmentManager();
        pGeomCacheAttachmentManager->RegisterAttachment(pChild, this, CryStringUtils::HashString(attachParams.m_target));
#endif
    }
    else if (attachParams.m_nAttachFlags & ATTACHMENT_CHARACTERBONE)
    {
        pChild->m_pBinds->parentBindingType = SBindings::eBT_CharacterBone;
        CCharacterBoneAttachmentManager* pCharacterBoneAttachmentManager = static_cast<CEntitySystem*>(GetEntitySystem())->GetCharacterBoneAttachmentManager();
        const uint32 targetCRC = CCrc32::ComputeLowercase(attachParams.m_target);
        pCharacterBoneAttachmentManager->RegisterAttachment(pChild, this, targetCRC);
    }

    // Add to child list first to make sure node not get deleted while re-attaching.
    m_pBinds->childs.push_back(pChild);
    if (pChild->m_pBinds->pParent)
    {
        pChild->DetachThis();   // Detach node if attached to other parent.
    }
    // Assign this entity as parent to child entity.
    pChild->m_pBinds->pParent = this;

    if (attachParams.m_nAttachFlags & ATTACHMENT_KEEP_TRANSFORMATION)
    {
        // Keep old world space transformation.
        pChild->SetWorldTM(childTM);
    }
    else if (pChild->m_flags & ENTITY_FLAG_TRIGGER_AREAS)
    {
        // If entity should trigger areas, when attaching it make sure local translation is reset to (0,0,0).
        // This prevents invalid position to propogate to area manager and incorrectly leave/enter areas.
        pChild->m_vPos.Set(0, 0, 0);
        pChild->InvalidateTM(ENTITY_XFORM_FROM_PARENT);
    }
    else
    {
        pChild->InvalidateTM(ENTITY_XFORM_FROM_PARENT);
    }

    // Send attach event.
    SEntityEvent event(ENTITY_EVENT_ATTACH);
    event.nParam[0] = pChild->GetId();
    SendEvent(event);

    // Send ATTACH_THIS event to child
    SEntityEvent eventThis(ENTITY_EVENT_ATTACH_THIS);
    eventThis.nParam[0] = GetId();
    pChild->SendEvent(eventThis);
}

//////////////////////////////////////////////////////////////////////////
void CEntity::DetachAll(int nDetachFlags)
{
    if (m_pBinds)
    {
        while (!m_pBinds->childs.empty())
        {
            CEntity* pChild = m_pBinds->childs.front();
            pChild->DetachThis(nDetachFlags);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CEntity::DetachThis(int nDetachFlags, int nWhyFlags)
{
    if (m_pBinds && m_pBinds->pParent)
    {
        Matrix34 worldTM;

        bool bKeepTransform = (nDetachFlags & ATTACHMENT_KEEP_TRANSFORMATION) || (m_flags & ENTITY_FLAG_TRIGGER_AREAS);

        if (bKeepTransform)
        {
            worldTM = GetWorldTM();
        }

        // Copy parent to temp var, erasing child from parent may delete this node if child referenced only from parent.
        CEntity* pParent = m_pBinds->pParent;
        m_pBinds->pParent = NULL;

        // Remove child pointer from parent array of childs.
        SBindings* pParentBinds = pParent->m_pBinds;
        if (pParentBinds)
        {
            stl::find_and_erase(pParentBinds->childs, this);
        }

        if (bKeepTransform)
        {
            // Keep old world space transformation.
            SetLocalTM(worldTM, nWhyFlags | ENTITY_XFORM_FROM_PARENT);
        }
        else
        {
            InvalidateTM(nWhyFlags | ENTITY_XFORM_FROM_PARENT);
        }

        // Send detach event to parent.
        SEntityEvent event(ENTITY_EVENT_DETACH);
        event.nParam[0] = GetId();
        pParent->SendEvent(event);

        // Send DETACH_THIS event to child
        SEntityEvent eventThis(ENTITY_EVENT_DETACH_THIS);
        eventThis.nParam[0] = pParent->GetId();
        SendEvent(eventThis);

        if (m_pBinds->parentBindingType == SBindings::eBT_GeomCacheNode)
        {
#if defined(USE_GEOM_CACHES)
            CGeomCacheAttachmentManager* pGeomCacheAttachmentManager = static_cast<CEntitySystem*>(GetEntitySystem())->GetGeomCacheAttachmentManager();
            pGeomCacheAttachmentManager->UnregisterAttachment(this, pParent);
#endif
        }
        else if (m_pBinds->parentBindingType == SBindings::eBT_CharacterBone)
        {
            CCharacterBoneAttachmentManager* pCharacterBoneAttachmentManager = static_cast<CEntitySystem*>(GetEntitySystem())->GetCharacterBoneAttachmentManager();
            pCharacterBoneAttachmentManager->UnregisterAttachment(this, pParent);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
int CEntity::GetChildCount() const
{
    if (m_pBinds)
    {
        return m_pBinds->childs.size();
    }
    return 0;
}

//////////////////////////////////////////////////////////////////////////
IEntity* CEntity::GetChild(int nIndex) const
{
    if (m_pBinds)
    {
        if (nIndex >= 0 && nIndex < (int)m_pBinds->childs.size())
        {
            return m_pBinds->childs[nIndex];
        }
    }
    return NULL;
}

//////////////////////////////////////////////////////////////////////////
void CEntity::AllocBindings()
{
    if (!m_pBinds)
    {
        m_pBinds = new SBindings;
        m_pBinds->pParent = NULL;
    }
}

//////////////////////////////////////////////////////////////////////////
void CEntity::DeallocBindings()
{
    if (m_pBinds)
    {
        DetachAll();
        if (m_pBinds->pParent)
        {
            DetachThis(0);
        }
        delete m_pBinds;
    }
    m_pBinds = NULL;
}

//////////////////////////////////////////////////////////////////////////
void CEntity::SetWorldTM(const Matrix34& tm, int nWhyFlags)
{
    if (GetParent() && !m_bNotInheritXform)
    {
        Matrix34 localTM = GetParentAttachPointWorldTM().GetInverted() * tm;
        SetLocalTM(localTM, nWhyFlags);
    }
    else
    {
        SetLocalTM(tm, nWhyFlags);
    }
}

//////////////////////////////////////////////////////////////////////////
void CEntity::SetLocalTM(const Matrix34& tm, int nWhyFlags)
{
    if (m_bNotInheritXform && (nWhyFlags & ENTITY_XFORM_FROM_PARENT)) // Ignore parent x-forms.
    {
        return;
    }

    if (tm.IsOrthonormal())
    {
        SetPosRotScale(tm.GetTranslation(), Quat(tm), Vec3(1.0f, 1.0f, 1.0f), nWhyFlags);
    }
    else if (!tm.IsDegenerate())
    {
        AffineParts affineParts;
        affineParts.SpectralDecompose(tm);

        SetPosRotScale(affineParts.pos, affineParts.rot, affineParts.scale, nWhyFlags);
    }
    else
    {
        AZ_Warning("Entity", false, "Trying to set transform to a degenerate matrix for entity %s [%u]", GetName(), GetId());
    }
}

//////////////////////////////////////////////////////////////////////////
void CEntity::SetPosRotScale(const Vec3& vPos, const Quat& qRotation, const Vec3& vScale, int nWhyFlags)
{
    int changed = 0;

    if (m_bNotInheritXform && (nWhyFlags & ENTITY_XFORM_FROM_PARENT)) // Ignore parent x-forms.
    {
        return;
    }

    if (CheckFlags(ENTITY_FLAG_IGNORE_PHYSICS_UPDATE) && (nWhyFlags & ENTITY_XFORM_PHYSICS_STEP)) // Ignore physical based position in editor related entities
    {
        return;
    }

    if (m_vPos != vPos)
    {
        nWhyFlags |= ENTITY_XFORM_POS;
        changed++;
        CHECKQNAN_VEC(vPos);
        m_vPos = vPos;
    }

    if (m_qRotation.v != qRotation.v || m_qRotation.w != qRotation.w)
    {
        nWhyFlags |= ENTITY_XFORM_ROT;
        changed++;
        m_qRotation = qRotation;
    }

    if (m_vScale != vScale)
    {
        nWhyFlags |= ENTITY_XFORM_SCL;
        changed++;
        m_vScale = vScale;
    }

    if (changed != 0)
    {
        InvalidateTM(nWhyFlags);
    }
}

//////////////////////////////////////////////////////////////////////////
void CEntity::CalcLocalTM(Matrix34& tm) const
{
    tm.Set(m_vScale, m_qRotation, m_vPos);
}

//////////////////////////////////////////////////////////////////////////
Matrix34 CEntity::GetLocalTM() const
{
    Matrix34 tm;
    CalcLocalTM(tm);
    return tm;
}

//////////////////////////////////////////////////////////////////////////
void CEntity::OnRellocate(int nWhyFlags)
{
    if (m_flags & ENTITY_FLAG_TRIGGER_AREAS && (nWhyFlags & ENTITY_XFORM_POS))
    {
        static_cast<CAreaManager*>(g_pIEntitySystem->GetAreaManager())->MarkEntityForUpdate(GetId());
    }

    //////////////////////////////////////////////////////////////////////////
    // Reposition entity in the partition grid.
    //////////////////////////////////////////////////////////////////////////
    if ((m_flags & ENTITY_FLAG_NO_PROXIMITY) == 0 &&
        (!m_bHidden || (nWhyFlags & ENTITY_XFORM_EDITOR))
        && !m_bInShutDown)
    {
        if (nWhyFlags & ENTITY_XFORM_POS)
        {
            m_bWasRellocated = true;
            Vec3 wp = GetWorldPos();
            m_pGridLocation = g_pIEntitySystem->GetPartitionGrid()->Rellocate(m_pGridLocation, wp, this);
            if (m_pGridLocation)
            {
                m_pGridLocation->nEntityFlags = m_flags;
            }

            if (!m_bTrigger)
            {
                if (!m_pProximityEntity)
                {
                    m_pProximityEntity = g_pIEntitySystem->GetProximityTriggerSystem()->CreateEntity(AZ::EntityId(m_nID));
                }
                g_pIEntitySystem->GetProximityTriggerSystem()->MoveEntity(m_pProximityEntity, wp);
            }
        }
    }
    else
    {
        if (m_pGridLocation)
        {
            m_bWasRellocated = true;
            g_pIEntitySystem->GetPartitionGrid()->FreeLocation(m_pGridLocation);
            m_pGridLocation = 0;
        }

        if (m_bHidden)
        {
            if (!m_bTrigger)
            {
                //////////////////////////////////////////////////////////////////////////
                if (m_pProximityEntity && !(m_flags & ENTITY_FLAG_LOCAL_PLAYER)) // Hidden local player still should trigger proximity triggers in editor.
                {
                    g_pIEntitySystem->GetProximityTriggerSystem()->RemoveEntity(m_pProximityEntity, true);
                    m_pProximityEntity = 0;
                }
            }
        }
    }
    //////////////////////////////////////////////////////////////////////////
}

//////////////////////////////////////////////////////////////////////////
void CEntity::InvalidateTM(int nWhyFlags, bool bRecalcPhyBounds)
{
    if (nWhyFlags & ENTITY_XFORM_FROM_PARENT)
    {
        nWhyFlags |= ENTITY_XFORM_POS | ENTITY_XFORM_ROT | ENTITY_XFORM_SCL;
    }

    CalcLocalTM(m_worldTM);

    if (m_pBinds)
    {
        if (m_pBinds->pParent && !m_bNotInheritXform)
        {
            m_worldTM = GetParentAttachPointWorldTM() * m_worldTM;
        }

        // Invalidate matrices for all child objects.
        std::vector<CEntity*>::const_iterator it = m_pBinds->childs.begin();
        const std::vector<CEntity*>::const_iterator end = m_pBinds->childs.end();
        CEntity* pChild = NULL;
        for (; it != end; ++it)
        {
            pChild = *it;
            if (pChild)
            {
                pChild->InvalidateTM(nWhyFlags | ENTITY_XFORM_FROM_PARENT);
            }
        }
    }

    // [*DavidR | 23/Sep/2008] Caching world-space forward dir ignoring scaling
    if (nWhyFlags & (ENTITY_XFORM_SCL | ENTITY_XFORM_ROT))
    {
        m_bDirtyForwardDir = true;
    }

    OnRellocate(nWhyFlags);

    // Send transform event.
    if (!(nWhyFlags & ENTITY_XFORM_NO_EVENT))
    {
        SEntityEvent event(ENTITY_EVENT_XFORM);
        event.nParam[0] = nWhyFlags;
        SendEvent(event);
    }
}

//////////////////////////////////////////////////////////////////////////
void CEntity::SetPos(const Vec3& vPos, int nWhyFlags, bool bRecalcPhyBounds, bool bForce)
{
    CHECKQNAN_VEC(vPos);
    if (m_vPos == vPos)
    {
        return;
    }

    m_vPos = vPos;
    InvalidateTM(nWhyFlags | ENTITY_XFORM_POS, bRecalcPhyBounds); // Position change.
}

//////////////////////////////////////////////////////////////////////////
void CEntity::SetRotation(const Quat& qRotation, int nWhyFlags)
{
    if (m_qRotation.v == qRotation.v && m_qRotation.w == qRotation.w)
    {
        return;
    }

    m_qRotation = qRotation;
    InvalidateTM(nWhyFlags | ENTITY_XFORM_ROT); // Rotation change.
}

//////////////////////////////////////////////////////////////////////////
void CEntity::SetScale(const Vec3& vScale, int nWhyFlags)
{
    if (m_vScale == vScale)
    {
        return;
    }

    m_vScale = vScale;
    InvalidateTM(nWhyFlags | ENTITY_XFORM_SCL); // Scale change.
}

//////////////////////////////////////////////////////////////////////////
Ang3 CEntity::GetWorldAngles() const
{
    Matrix34 tm = GetWorldTM();
    tm.OrthonormalizeFast();
    Ang3 angles = Ang3::GetAnglesXYZ(tm);
    return angles;
}

//////////////////////////////////////////////////////////////////////////
Quat CEntity::GetWorldRotation() const
{
    //  if (m_vScale == Vec3(1,1,1))
    if (IsScaled())
    {
        Matrix34 tm = GetWorldTM();
        tm.OrthonormalizeFast();
        return Quat(tm);
    }
    else
    {
        return Quat(GetWorldTM());
    }
}

//////////////////////////////////////////////////////////////////////////
void CEntity::GetWorldBounds(AABB& bbox) const
{
    GetLocalBounds(bbox);
    bbox.SetTransformedAABB(GetWorldTM(), bbox);
}

void CEntity::InvalidateBounds()
{
    IComponentRenderConstPtr pRenderComponent = GetComponent<IComponentRender>();
    if (pRenderComponent)
    {
        const_cast<IComponentRender*>(pRenderComponent.get())->InvalidateLocalBounds();
    }
}
//////////////////////////////////////////////////////////////////////////
void CEntity::GetLocalBounds(AABB& bbox) const
{
    bbox.min.Set(0, 0, 0);
    bbox.max.Set(0, 0, 0);

    IComponentRenderConstPtr pRenderComponent = GetComponent<IComponentRender>();
    if (pRenderComponent)
    {
        const_cast<IComponentRender*>(pRenderComponent.get())->GetLocalBounds(bbox);
    }
    else if (GetComponent<IComponentPhysics>())
    {
        IComponentPhysicsConstPtr pPhysicsComponent = GetComponent<IComponentPhysics>();
        if (pPhysicsComponent)
        {
            const_cast<IComponentPhysics*>(pPhysicsComponent.get())->GetLocalBounds(bbox);
        }
    }
    else
    {
        IComponentTriggerConstPtr triggerComponent = GetComponent<IComponentTrigger>();
        if (triggerComponent)
        {
            triggerComponent->GetTriggerBounds(bbox);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CEntity::SetUpdateStatus()
{
    bool bEnable = GetUpdateStatus();

    g_pIEntitySystem->ActivateEntity(this, bEnable);

    if (IAIObject* pAIObject = GetAIObject())
    {
        if (IAIActorProxy* pProxy = pAIObject->GetProxy())
        {
            pProxy->EnableUpdate(bEnable);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CEntity::Activate(bool bActive)
{
    m_bActive = bActive;
    m_nUpdateCounter = 0;
    SetUpdateStatus();
}

//////////////////////////////////////////////////////////////////////////
void CEntity::PrePhysicsActivate(bool bActive)
{
    g_pIEntitySystem->ActivatePrePhysicsUpdateForEntity(this, bActive);
}

//////////////////////////////////////////////////////////////////////////
void CEntity::SetTimer(int nTimerId, int nMilliSeconds)
{
    KillTimer(nTimerId);
    SEntityTimerEvent timeEvent;
    timeEvent.entityId = m_nID;
    timeEvent.nTimerId = nTimerId;
    timeEvent.nMilliSeconds = nMilliSeconds;
    g_pIEntitySystem->AddTimerEvent(timeEvent);
}

//////////////////////////////////////////////////////////////////////////
void CEntity::KillTimer(int nTimerId)
{
    g_pIEntitySystem->RemoveTimerEvent(m_nID, nTimerId);
}

//////////////////////////////////////////////////////////////////////////
void CEntity::Hide(bool bHide)
{
    if (m_bHidden != bHide)
    {
        m_bHidden = bHide;

        // Update registered locations
        OnRellocate(ENTITY_XFORM_POS);

        if (bHide)
        {
            SEntityEvent e(ENTITY_EVENT_HIDE);
            SendEvent(e);
        }
        else
        {
            SEntityEvent e(ENTITY_EVENT_UNHIDE);
            SendEvent(e);
        }

        // Propogate Hide flag to the child entities.
        if (m_pBinds)
        {
            for (int i = 0; i < (int)m_pBinds->childs.size(); i++)
            {
                if (m_pBinds->childs[i] != NULL)
                {
                    m_pBinds->childs[i]->Hide(bHide);
                }
            }
        }

        SetUpdateStatus();
    }
}


//////////////////////////////////////////////////////////////////////////
void CEntity::Invisible(bool bInvisible)
{
    if (m_bInvisible != bInvisible)
    {
        m_bInvisible = bInvisible;

        if (bInvisible)
        {
            SEntityEvent e(ENTITY_EVENT_INVISIBLE);
            SendEvent(e);
        }
        else
        {
            SEntityEvent e(ENTITY_EVENT_VISIBLE);
            SendEvent(e);
        }

        // Propagate invisible flag to the child entities.
        if (m_pBinds)
        {
            for (int i = 0; i < (int)m_pBinds->childs.size(); i++)
            {
                if (m_pBinds->childs[i] != NULL)
                {
                    m_pBinds->childs[i]->Invisible(bInvisible);
                }
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CEntity::SetUpdatePolicy(EEntityUpdatePolicy eUpdatePolicy)
{
    m_eUpdatePolicy = eUpdatePolicy;
}

//////////////////////////////////////////////////////////////////////////
const char* CEntity::GetEntityTextDescription() const
{
    m_szDescription = m_szName + " (" + m_pClass->GetName() + ")";
    return m_szDescription;
}

//////////////////////////////////////////////////////////////////////////
void CEntity::SerializeXML(XmlNodeRef& node, bool bLoading)
{
    GetComponent<IComponentSerialization>()->SerializeXML(node, bLoading);
}

//////////////////////////////////////////////////////////////////////////
void CEntity::SerializeXML_ExceptScriptComponent(XmlNodeRef& node, bool bLoading)
{
    GetComponent<IComponentSerialization>()->SerializeXML(node, bLoading, { CComponentScript::Type() });
}

//////////////////////////////////////////////////////////////////////////
bool CEntity::GetSignature(TSerialize& signature)
{
    return GetComponent<IComponentSerialization>()->GetSignature(signature);
}

//////////////////////////////////////////////////////////////////////////
bool CEntity::RegisterComponent(IComponentPtr pComponentPtr)
{
    if (!pComponentPtr)
    {
        return false;
    }

    // Adding components to entity at this time is forbidden. Possible solutions:
    // 1) Create component during general initialization, not at this crazy moment.
    // 2) ForEachComponent() is being used when the less restrictive (but less efficient)
    //    ForEachComponent_AllowDestroyCreate() could be used instead.
    CRY_ASSERT_MESSAGE(m_bAllowDestroyOrCreateComponent, "Adding component to entity at this time is forbidden");

    const ComponentType& componentType = pComponentPtr->GetComponentType();
    auto insertResult = m_components.insert(AZStd::make_pair(componentType, pComponentPtr));
    if (!insertResult.second)
    {
        CryWarning(VALIDATOR_MODULE_ENTITYSYSTEM, VALIDATOR_WARNING,
            "Entity (%s) already has a component of type (%s)",
            m_szName.c_str(), componentType.Name());
    }

    static_cast<CEntitySystem*>(gEnv->pEntitySystem)->ComponentRegister(GetId(), pComponentPtr);

    return insertResult.second;
}

bool CEntity::DeregisterComponent(IComponentPtr pComponent)
{
    if (!pComponent)
    {
        return false;
    }

    // Removing component from entity at this time is forbidden. Possible solutions:
    // 1) Remove component during general shutdown, not at this crazy moment.
    // 2) ForEachComponent() is being used when the less restrictive (but less efficient)
    //    ForEachComponent_AllowDestroyCreate() could be used instead.
    CRY_ASSERT_MESSAGE(m_bAllowDestroyOrCreateComponent, "Removing component from entity at this time is forbidden");

    static_cast<CEntitySystem*>(gEnv->pEntitySystem)->ComponentDeregister(GetId(), pComponent);

    size_t numErased = m_components.erase(pComponent->GetComponentType());

    return numErased > 0;
}

//////////////////////////////////////////////////////////////////////////
void CEntity::Serialize(TSerialize ser, int nFlags)
{
    MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_Other, 0, "Entity serialization");
    MEMSTAT_CONTEXT_FMT(EMemStatContextTypes::MSC_Other, 0, "%s", GetClass() ? GetClass()->GetName() : "<UNKNOWN>");
    if (nFlags & ENTITY_SERIALIZE_POSITION)
    {
        ser.Value("position", m_vPos, 'wrld');
    }
    if (nFlags & ENTITY_SERIALIZE_SCALE)
    {
        // TODO: check dimensions
        // TODO: define an AC_WorldPos type
        // TODO: check dimensions
        ser.Value("scale", m_vScale);
    }
    if (nFlags & ENTITY_SERIALIZE_ROTATION)
    {
        ser.Value("rotation", m_qRotation, 'ori3');
    }

    if (nFlags & ENTITY_SERIALIZE_GEOMETRIES)
    {
        int i, nSlots;
        IPhysicalEntity* pPhysics = GetPhysics();
        pe_params_part pp;
        bool bHasIt = false;
        float mass = 0;

        if (!ser.IsReading())
        {
            nSlots = GetSlotCount();
            ser.Value("numslots", nSlots);
            for (i = 0; i < nSlots; i++)
            {
                int nSlotId = i;
                CEntityObject* const pSlot = GetSlot(i);
                if (ser.BeginOptionalGroup("Slot", (pSlot && pSlot->pStatObj)))
                {
                    ser.Value("id", nSlotId);
                    bool bHasXfrom = pSlot->m_pXForm != 0;
                    ser.Value("hasXform", bHasXfrom);
                    if (bHasXfrom)
                    {
                        Vec3 col;
                        col = pSlot->m_pXForm->localTM.GetColumn(0);
                        ser.Value("Xform0", col);
                        col = pSlot->m_pXForm->localTM.GetColumn(1);
                        ser.Value("Xform1", col);
                        col = pSlot->m_pXForm->localTM.GetColumn(2);
                        ser.Value("Xform2", col);
                        col = pSlot->m_pXForm->localTM.GetTranslation();
                        ser.Value("XformT", col);
                    }
                    //ser.Value("hasmat", bHasIt=pSlot->pMaterial!=0);
                    //if (bHasIt)
                    //  ser.Value("matname", pSlot->pMaterial->GetName());
                    ser.Value("flags", pSlot->flags);
                    bool bSlotUpdate = pSlot->bUpdate;
                    ser.Value("update", bSlotUpdate);
                    if (pPhysics)
                    {
                        if (pPhysics->GetType() != PE_STATIC)
                        {
                            if (pSlot->pStatObj->GetFlags() & STATIC_OBJECT_COMPOUND)
                            {
                                MARK_UNUSED pp.partid;
                                for (pp.ipart = 0, mass = 0; pPhysics->GetParams(&pp); pp.ipart++)
                                {
                                    mass += pp.mass;
                                }
                            }
                            else
                            {
                                pp.partid = i;
                                MARK_UNUSED pp.ipart;
                                if (pPhysics->GetParams(&pp))
                                {
                                    mass = pp.mass;
                                }
                            }
                        }
                        ser.Value("mass", mass);
                    }

                    int bHeavySer = 0;
                    if (pPhysics && pPhysics->GetType() == PE_RIGID)
                    {
                        if (pSlot->pStatObj->GetFlags() & STATIC_OBJECT_COMPOUND)
                        {
                            IStatObj::SSubObject* pSubObj;
                            for (i = 0; i < pSlot->pStatObj->GetSubObjectCount(); i++)
                            {
                                if ((pSubObj = pSlot->pStatObj->GetSubObject(i)) && pSubObj->pStatObj)
                                {
                                    bHeavySer |= pSubObj->pStatObj->GetFlags() & STATIC_OBJECT_GENERATED;
                                }
                            }
                        }
                        else
                        {
                            bHeavySer = pSlot->pStatObj->GetFlags() & STATIC_OBJECT_GENERATED;
                        }
                    }

                    if (bHeavySer) // prevent heavy mesh serialization of modified statobjects
                    {
                        ser.Value("noobj", bHasIt = true);
                    }
                    else
                    {
                        ser.Value("noobj", bHasIt = false);
                        gEnv->p3DEngine->SaveStatObj(pSlot->pStatObj, ser);
                    }
                    ser.Value("sshidemask", pSlot->nSubObjHideMask);
                    ser.EndGroup(); //Slot
                }
            }
        }
        else
        {
            bool bClearAfterLoad = false;
            pe_action_remove_all_parts arp;
            if (pPhysics)
            {
                pPhysics->Action(&arp);
            }
            for (i = GetSlotCount() - 1; i >= 0; i--)
            {
                FreeSlot(i);
            }
            ser.Value("numslots", nSlots);
            for (int iter = 0; iter < nSlots; iter++)
            {
                int nSlotId = -1;
                if (ser.BeginOptionalGroup("Slot", true))
                {
                    ser.Value("id", nSlotId);

                    IComponentRenderPtr pRender = GetOrCreateComponent<IComponentRender>();
                    CEntityObject* pSlot = pRender->AllocSlot(nSlotId);

                    ser.Value("hasXform", bHasIt);
                    if (bHasIt)
                    {
                        Vec3 col;
                        Matrix34 mtx;
                        ser.Value("Xform0", col);
                        mtx.SetColumn(0, col);
                        ser.Value("Xform1", col);
                        mtx.SetColumn(1, col);
                        ser.Value("Xform2", col);
                        mtx.SetColumn(2, col);
                        ser.Value("XformT", col);
                        mtx.SetTranslation(col);
                        SetSlotLocalTM(nSlotId, mtx);
                    }

                    ser.Value("flags", pSlot->flags);
                    ser.Value("update", bHasIt);
                    pSlot->bUpdate = bHasIt;
                    if (pPhysics)
                    {
                        ser.Value("mass", mass);
                    }

                    ser.Value("noobj", bHasIt);
                    if (bHasIt)
                    {
                        bClearAfterLoad = true;
                    }
                    else
                    {
                        SetStatObj(gEnv->p3DEngine->LoadStatObj(ser), ENTITY_SLOT_ACTUAL | nSlotId, pPhysics != 0, mass);
                    }

                    ser.Value("sshidemask", pSlot->nSubObjHideMask);
                    ser.EndGroup(); //Slot
                }
            }
            if (bClearAfterLoad)
            {
                if (pPhysics)
                {
                    pPhysics->Action(&arp);
                }
                for (i = GetSlotCount() - 1; i >= 0; i--)
                {
                    FreeSlot(i);
                }
            }
        }
    }

    if (nFlags & ENTITY_SERIALIZE_PROXIES)
    {
        //CryLogAlways("Serializing proxies for %s of class %s", GetName(), GetClass()->GetName());
        bool bSaveComponents = ser.GetSerializationTarget() == eST_Network; // always save for network stream
        if (!bSaveComponents && !ser.IsReading())
        {
            if (IComponentSerializationPtr serializationComponent = GetComponent<IComponentSerialization>())
            {
                if (serializationComponent->NeedSerialize())
                {
                    bSaveComponents = true;
                }
            }
        }

        if (ser.BeginOptionalGroup("Components", bSaveComponents))
        {
            bool bHasSubst;
            if (!ser.IsReading())
            {
                bHasSubst = (GetComponent<IComponentSubstitution>() != nullptr);
                ser.Value("bHasSubst", bHasSubst);
            }
            else
            {
                ser.Value("bHasSubst", bHasSubst);
                if (bHasSubst)
                {
                    GetOrCreateComponent<IComponentSubstitution>();
                }
            }

            GetComponent<IComponentSerialization>()->Serialize(ser);

            ser.EndGroup(); // Components
        }

        //////////////////////////////////////////////////////////////////////////
        // for usable on BasicEntity.
        //////////////////////////////////////////////////////////////////////////
        {
            IScriptTable* pScriptTable = GetScriptTable();
            if (pScriptTable)
            {
                if (ser.IsWriting())
                {
                    if (pScriptTable->HaveValue("__usable"))
                    {
                        bool bUsable = false;
                        pScriptTable->GetValue("__usable", bUsable);
                        int nUsable = (bUsable) ? 1 : -1;
                        ser.Value("__usable", nUsable);
                    }
                }
                else
                {
                    int nUsable = 0;
                    ser.Value("__usable", nUsable);
                    if (nUsable == 1)
                    {
                        pScriptTable->SetValue("__usable", 1);
                    }
                    else if (nUsable == -1)
                    {
                        pScriptTable->SetValue("__usable", 0);
                    }
                    else
                    {
                        pScriptTable->SetToNull("__usable");
                    }
                }
            }
        }
    }

    if (nFlags & ENTITY_SERIALIZE_PROPERTIES)
    {
        IComponentScriptPtr scriptComponent = GetComponent<IComponentScript>();

        if (scriptComponent)
        {
            scriptComponent->SerializeProperties(ser);
        }
    }

    m_bDirtyForwardDir = true;
}

//////////////////////////////////////////////////////////////////////////
void CEntity::Physicalize(SEntityPhysicalizeParams& params)
{
    GetOrCreateComponent<IComponentPhysics>()->Physicalize(params);
}

//////////////////////////////////////////////////////////////////////////
void CEntity::EnablePhysics(bool enable)
{
    IComponentPhysicsPtr pPhysicsComponent = GetComponent<IComponentPhysics>();
    if (pPhysicsComponent)
    {
        pPhysicsComponent->EnablePhysics(enable);
    }
}

//////////////////////////////////////////////////////////////////////////
IPhysicalEntity* CEntity::GetPhysics() const
{
    IComponentPhysicsConstPtr pPhysicsComponent = GetComponent<IComponentPhysics>();
    if (pPhysicsComponent)
    {
        IPhysicalEntity* pPhysicalEntity = pPhysicsComponent->GetPhysicalEntity();
        if (pPhysicalEntity)
        {
            return pPhysicalEntity;
        }
    }

    IComponentRopeConstPtr pRopeComponent = GetComponent<IComponentRope>();
    if (pRopeComponent)
    {
        IRopeRenderNode* pRopeRenderNode = pRopeComponent->GetRopeRenderNode();
        if (pRopeRenderNode)
        {
            return pRopeRenderNode->GetPhysics();
        }
    }

    return NULL;
}

//////////////////////////////////////////////////////////////////////////
// Custom entity material.
//////////////////////////////////////////////////////////////////////////
void CEntity::SetMaterial(_smart_ptr<IMaterial> pMaterial)
{
    m_pMaterial = pMaterial;

    CheckMaterialFlags();

    SEntityEvent event;
    event.event = ENTITY_EVENT_MATERIAL;
    event.nParam[0] = (INT_PTR)pMaterial.get();
    SendEvent(event);
}

//////////////////////////////////////////////////////////////////////////
void CEntity::CheckMaterialFlags()
{
    IComponentRenderPtr renderComponent = GetComponent<IComponentRender>();
    if (!renderComponent)
    {
        return;
    }

    if (IRenderNode* pRenderNode = renderComponent->GetRenderNode())
    {
        bool bCollisionProxy = false;
        bool bRaycastProxy = false;
        bool bWasProxy = (pRenderNode->GetRndFlags() & (ERF_COLLISION_PROXY | ERF_RAYCAST_PROXY)) != 0;

        if (m_pMaterial)
        {
            if (m_pMaterial->GetFlags() & MTL_FLAG_COLLISION_PROXY)
            {
                bCollisionProxy = true;
            }

            if (m_pMaterial->GetFlags() & MTL_FLAG_RAYCAST_PROXY)
            {
                bRaycastProxy = true;
            }
            if (IPhysicalEntity* pPhysics = GetPhysics())
            {
                if (ISurfaceType* pSurf = m_pMaterial->GetSurfaceType())
                {
                    if (pSurf->GetPhyscalParams().collType >= 0)
                    {
                        pe_params_part pp;
                        pp.flagsAND = ~(geom_collides | geom_floats);
                        pp.flagsOR = pSurf->GetPhyscalParams().collType & geom_collides;
                        pPhysics->SetParams(&pp);
                    }
                }
            }
        }

        pRenderNode->SetRndFlags(ERF_COLLISION_PROXY, bCollisionProxy);
        pRenderNode->SetRndFlags(ERF_RAYCAST_PROXY, bRaycastProxy);

        if (bWasProxy || bRaycastProxy || bCollisionProxy)
        {
            if (IPhysicalEntity* pPhysics = GetPhysics())
            {
                pe_params_part pp;
                pp.flagsAND = ~((bRaycastProxy ? geom_colltype_solid : 0) | (bCollisionProxy ? geom_colltype_ray : 0));
                if (bWasProxy)
                {
                    pp.flagsOR = (!bRaycastProxy ? geom_colltype_solid : 0) | (!bCollisionProxy ? geom_colltype_ray : 0);
                }
                pPhysics->SetParams(&pp);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
_smart_ptr<IMaterial> CEntity::GetMaterial()
{
    return m_pMaterial;
}

//////////////////////////////////////////////////////////////////////////
void CEntity::OnCharacterBoundsReset()
{
    m_bBoundsValid = 0;
}

//////////////////////////////////////////////////////////////////////////
int CEntity::PhysicalizeSlot(int slot, SEntityPhysicalizeParams& params)
{
    if (IComponentPhysicsPtr component = GetComponent<IComponentPhysics>())
    {
        return component->AddSlotGeometry(slot, params);
    }
    return -1;
}

//////////////////////////////////////////////////////////////////////////
void CEntity::UnphysicalizeSlot(int slot)
{
    if (IComponentPhysicsPtr component = GetComponent<IComponentPhysics>())
    {
        component->RemoveSlotGeometry(slot);
    }
}

//////////////////////////////////////////////////////////////////////////
void CEntity::UpdateSlotPhysics(int slot)
{
    if (IComponentPhysicsPtr component = GetComponent<IComponentPhysics>())
    {
        component->UpdateSlotGeometry(slot, GetStatObj(slot));
    }
}

//////////////////////////////////////////////////////////////////////////
bool CEntity::IsSlotValid(int nSlot) const
{
    IComponentRenderConstPtr pRenderComponent = GetComponent<IComponentRender>();
    if (pRenderComponent)
    {
        return pRenderComponent->IsSlotValid(nSlot);
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
void CEntity::FreeSlot(int nSlot)
{
    IComponentRenderPtr pRenderComponent = GetComponent<IComponentRender>();
    if (pRenderComponent)
    {
        pRenderComponent->FreeSlot(nSlot);
    }
}

//////////////////////////////////////////////////////////////////////////
int  CEntity::GetSlotCount() const
{
    IComponentRenderConstPtr pRenderComponent = GetComponent<IComponentRender>();
    if (pRenderComponent)
    {
        return pRenderComponent->GetSlotCount();
    }
    return 0;
}

//////////////////////////////////////////////////////////////////////////
CEntityObject* CEntity::GetSlot(int nSlot) const
{
    IComponentRenderConstPtr pRenderComponent = GetComponent<IComponentRender>();
    if (pRenderComponent)
    {
        return pRenderComponent->GetSlot(nSlot);
    }
    return NULL;
}

//////////////////////////////////////////////////////////////////////////
bool CEntity::GetSlotInfo(int nSlot, SEntitySlotInfo& slotInfo) const
{
    IComponentRenderConstPtr pRenderComponent = GetComponent<IComponentRender>();
    if (pRenderComponent)
    {
        return pRenderComponent->GetSlotInfo(nSlot, slotInfo);
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
const Matrix34& CEntity::GetSlotWorldTM(int nSlot) const
{
    IComponentRenderConstPtr pRenderComponent = GetComponent<IComponentRender>();
    if (pRenderComponent)
    {
        return pRenderComponent->GetSlotWorldTM(nSlot);
    }
    return sIdentityMatrix;
}

//////////////////////////////////////////////////////////////////////////
const Matrix34& CEntity::GetSlotLocalTM(int nSlot, bool bRelativeToParent) const
{
    IComponentRenderConstPtr pRenderComponent = GetComponent<IComponentRender>();
    if (pRenderComponent)
    {
        return pRenderComponent->GetSlotLocalTM(nSlot, bRelativeToParent);
    }
    return sIdentityMatrix;
}

//////////////////////////////////////////////////////////////////////////
void CEntity::SetSlotLocalTM(int nSlot, const Matrix34& localTM, int nWhyFlags)
{
    Vec3 col0 = localTM.GetColumn(0);
    Vec3 col1 = localTM.GetColumn(1);
    Vec3 col2 = localTM.GetColumn(2);
    Vec3 col3 = localTM.GetColumn(3);

    CHECKQNAN_VEC(col0);
    CHECKQNAN_VEC(col1);
    CHECKQNAN_VEC(col2);
    CHECKQNAN_VEC(col3);

    IComponentRenderPtr pRenderComponent = GetComponent<IComponentRender>();
    if (pRenderComponent)
    {
        pRenderComponent->SetSlotLocalTM(nSlot, localTM);
    }
}

//////////////////////////////////////////////////////////////////////////
void CEntity::SetSlotCameraSpacePos(int nSlot, const Vec3& cameraSpacePos)
{
    IComponentRenderPtr pRenderComponent = GetComponent<IComponentRender>();
    if (pRenderComponent)
    {
        pRenderComponent->SetSlotCameraSpacePos(nSlot, cameraSpacePos);
    }
}

//////////////////////////////////////////////////////////////////////////
void CEntity::GetSlotCameraSpacePos(int nSlot, Vec3& cameraSpacePos) const
{
    IComponentRenderConstPtr pRenderComponent = GetComponent<IComponentRender>();
    if (pRenderComponent)
    {
        pRenderComponent->GetSlotCameraSpacePos(nSlot, cameraSpacePos);
    }
    else
    {
        cameraSpacePos = ZERO;
    }
}

//////////////////////////////////////////////////////////////////////////
bool CEntity::SetParentSlot(int nParentSlot, int nChildSlot)
{
    IComponentRenderPtr pRenderComponent = GetComponent<IComponentRender>();
    if (pRenderComponent)
    {
        return pRenderComponent->SetParentSlot(nParentSlot, nChildSlot);
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
void CEntity::SetSlotMaterial(int nSlot, _smart_ptr<IMaterial> pMaterial)
{
    IComponentRenderPtr pRenderComponent = GetComponent<IComponentRender>();
    if (pRenderComponent)
    {
        pRenderComponent->SetSlotMaterial(nSlot, pMaterial);
    }
}

//////////////////////////////////////////////////////////////////////////
void CEntity::SetSlotFlags(int nSlot, uint32 nFlags)
{
    IComponentRenderPtr pRenderComponent = GetComponent<IComponentRender>();
    if (pRenderComponent)
    {
        pRenderComponent->SetSlotFlags(nSlot, nFlags);
    }
}

//////////////////////////////////////////////////////////////////////////
uint32 CEntity::GetSlotFlags(int nSlot) const
{
    IComponentRenderConstPtr pRenderComponent = GetComponent<IComponentRender>();
    if (pRenderComponent)
    {
        return pRenderComponent->GetSlotFlags(nSlot);
    }
    return 0;
}

//////////////////////////////////////////////////////////////////////////
bool CEntity::ShouldUpdateCharacter(int nSlot) const
{
    return false;
}

//////////////////////////////////////////////////////////////////////////
ICharacterInstance* CEntity::GetCharacter(int nSlot)
{
    IComponentRenderPtr pRenderComponent = GetComponent<IComponentRender>();
    if (pRenderComponent)
    {
        return pRenderComponent->GetCharacter(nSlot);
    }
    else
    {
        return NULL;
    }
}

//////////////////////////////////////////////////////////////////////////
int CEntity::SetCharacter(ICharacterInstance* pCharacter, int nSlot)
{
    int nUsedSlot = -1;

    if (!AZ::CharacterBoundsNotificationBus::Handler::BusIsConnectedId(pCharacter))
    {
        AZ::CharacterBoundsNotificationBus::Handler::BusDisconnect();
        AZ::CharacterBoundsNotificationBus::Handler::BusConnect(pCharacter);
    }

    nUsedSlot = GetOrCreateComponent<IComponentRender>()->SetSlotCharacter(nSlot, pCharacter);
    if (GetComponent<IComponentPhysics>())
    {
        GetComponent<IComponentPhysics>()->UpdateSlotGeometry(nUsedSlot);
    }

    return nUsedSlot;
}

//////////////////////////////////////////////////////////////////////////
IStatObj* CEntity::GetStatObj(int nSlot)
{
    IComponentRenderPtr pRenderComponent = GetComponent<IComponentRender>();
    if (pRenderComponent)
    {
        return pRenderComponent->GetStatObj(nSlot);
    }
    return NULL;
}

//////////////////////////////////////////////////////////////////////////
int CEntity::SetStatObj(IStatObj* pStatObj, int nSlot, bool bUpdatePhysics, float mass)
{
    int bNoSubslots = nSlot >= 0 || nSlot & ENTITY_SLOT_ACTUAL;   // only use statobj's subslot when appending a new subslot
    nSlot = GetOrCreateComponent<IComponentRender>()->SetSlotGeometry(nSlot, pStatObj);
    if (bUpdatePhysics && GetComponent<IComponentPhysics>())
    {
        GetComponent<IComponentPhysics>()->UpdateSlotGeometry(nSlot, pStatObj, mass, bNoSubslots);
    }

    return nSlot;
}

//////////////////////////////////////////////////////////////////////////
IParticleEmitter* CEntity::GetParticleEmitter(int nSlot)
{
    IComponentRenderPtr pRenderComponent = GetComponent<IComponentRender>();
    if (pRenderComponent)
    {
        return pRenderComponent->GetParticleEmitter(nSlot);
    }
    return NULL;
}

//////////////////////////////////////////////////////////////////////////
IGeomCacheRenderNode* CEntity::GetGeomCacheRenderNode(int nSlot)
{
#if defined(USE_GEOM_CACHES)
    IComponentRenderPtr pRenderComponent = GetComponent<IComponentRender>();
    if (pRenderComponent)
    {
        return pRenderComponent->GetGeomCacheRenderNode(nSlot);
    }
#endif

    return NULL;
}

//////////////////////////////////////////////////////////////////////////
void CEntity::MoveSlot(IEntity* targetIEnt, int nSlot)
{
    CEntity* targetEnt = (CEntity*)targetIEnt;

    CComponentRenderPtr dstRenderComponent = targetEnt->GetOrCreateComponent<CComponentRender>();

    CEntityObject* scrSlot = GetSlot(nSlot);
    CEntityObject* dstSlot = dstRenderComponent->AllocSlot(nSlot);

    ICharacterInstance* pCharacterInstance = scrSlot->pCharacter;

    //--- Flush out any existing slot objects
    dstSlot->ReleaseObjects();

    //--- Shallow copy over slot
    dstSlot->pStatObj = scrSlot->pStatObj;
    dstSlot->pCharacter = pCharacterInstance;
    dstSlot->pLight = scrSlot->pLight;
    dstSlot->pChildRenderNode = scrSlot->pChildRenderNode;
    dstSlot->pFoliage = scrSlot->pFoliage;
    dstSlot->pMaterial = scrSlot->pMaterial;
    dstSlot->bUpdate = scrSlot->bUpdate;
    dstSlot->flags = scrSlot->flags;

    dstRenderComponent->AddFlags(CComponentRender::FLAG_UPDATE);
    dstRenderComponent->InvalidateBounds(true, true);
    dstRenderComponent->UpdateLodDistance(gEnv->p3DEngine->GetFrameLodInfo());

    //--- Ensure that any associated physics is copied over too
    IComponentPhysicsPtr srcPhysicsComponent = GetComponent<IComponentPhysics>();
    if (pCharacterInstance)
    {
        ISkeletonPose* pSkeletonPose = pCharacterInstance->GetISkeletonPose();
        IPhysicalEntity* pCharacterPhysics = pCharacterInstance->GetISkeletonPose()->GetCharacterPhysics();
        pCharacterInstance->GetISkeletonPose()->SetPostProcessCallback(NULL, NULL);
        if (srcPhysicsComponent && (pCharacterPhysics == srcPhysicsComponent->GetPhysicalEntity()))
        {
            IComponentPhysicsPtr dstPhysicsComponent = targetEnt->GetOrCreateComponent<IComponentPhysics>();
            srcPhysicsComponent->MovePhysics(dstPhysicsComponent.get());

            //Make sure auxiliar physics like ropes and attachments are also updated
            pe_params_foreign_data pfd;
            pfd.pForeignData = targetEnt;
            pfd.iForeignData = PHYS_FOREIGN_ID_ENTITY;

            for (int i = 0; pSkeletonPose->GetCharacterPhysics(i); ++i)
            {
                pSkeletonPose->GetCharacterPhysics(i)->SetParams(&pfd);
            }

            IAttachmentManager* pAttachmentManager = pCharacterInstance->GetIAttachmentManager();
            const int attachmentCount = pAttachmentManager->GetAttachmentCount();
            for (int i = 0; i < attachmentCount; ++i)
            {
                IAttachment* pAttachment = pAttachmentManager->GetInterfaceByIndex(i);
                IAttachmentObject* pAttachmentObject = pAttachment->GetIAttachmentObject();
                if ((pAttachment->GetType() == CA_BONE) && (pAttachmentObject != NULL))
                {
                    ICharacterInstance* pAttachedCharacterInstance = pAttachmentObject->GetICharacterInstance();
                    if (pAttachedCharacterInstance != NULL)
                    {
                        ISkeletonPose* pAttachedSkeletonPose = pAttachedCharacterInstance->GetISkeletonPose();
                        IPhysicalEntity* pAttachedCharacterPhysics = pAttachedSkeletonPose->GetCharacterPhysics();
                        if (pAttachedCharacterPhysics != NULL)
                        {
                            pAttachedCharacterPhysics->SetParams(&pfd);
                        }

                        for (int j = 0; pAttachedSkeletonPose->GetCharacterPhysics(j); ++j)
                        {
                            pAttachedSkeletonPose->GetCharacterPhysics(j)->SetParams(&pfd);
                        }
                    }
                }
            }
        }
        // Register ourselves to listen for animation events coming from the character.
        if (ISkeletonAnim* pSkeletonAnim = pCharacterInstance->GetISkeletonAnim())
        {
            pSkeletonAnim->SetEventCallback(CComponentRender::AnimEventCallback, dstRenderComponent.get());
        }
    }

    //--- Clear src slot
    scrSlot->pStatObj = NULL;
    scrSlot->pCharacter = NULL;
    scrSlot->pLight = NULL;
    scrSlot->pChildRenderNode = NULL;
    scrSlot->pFoliage = NULL;
    scrSlot->pMaterial = NULL;
    scrSlot->flags = 0;
    scrSlot->bUpdate = false;
}

//////////////////////////////////////////////////////////////////////////
int CEntity::LoadGeometry(int nSlot, const char* sFilename, const char* sGeomName, int nLoadFlags)
{
    nSlot = GetOrCreateComponent<IComponentRender>()->LoadGeometry(nSlot, sFilename, sGeomName, nLoadFlags);

    if (nLoadFlags & EF_AUTO_PHYSICALIZE)
    {
        // Also physicalize geometry.
        SEntityPhysicalizeParams params;
        params.nSlot = nSlot;
        IStatObj* pStatObj = GetStatObj(ENTITY_SLOT_ACTUAL | nSlot);
        float mass, density;
        if (!pStatObj || pStatObj->GetFlags() & STATIC_OBJECT_COMPOUND ||
            pStatObj->GetPhysicalProperties(mass, density) && (mass >= 0 || density >= 0))
        {
            params.type = PE_RIGID;
        }
        else
        {
            params.type = PE_STATIC;
        }
        Physicalize(params);

        IPhysicalEntity* pe = GetPhysics();
        if (pe)
        {
            pe_action_awake aa;
            aa.bAwake = 0;
            pe->Action(&aa);
            pe_params_foreign_data pfd;
            pfd.iForeignFlagsOR = PFF_UNIMPORTANT;
            pe->SetParams(&pfd);
        }

        // Mark as AI hideable unless the object flags are preventing it.
        if (pStatObj && (pStatObj->GetFlags() & STATIC_OBJECT_NO_AUTO_HIDEPOINTS) == 0)
        {
            AddFlags((uint32)ENTITY_FLAG_AI_HIDEABLE);
        }
    }

    return nSlot;
}

//////////////////////////////////////////////////////////////////////////
int CEntity::LoadCharacter(int nSlot, const char* sFilename, int nLoadFlags)
{
    IComponentRenderPtr pRenderComponent = GetOrCreateComponent<IComponentRender>();

    ICharacterInstance* pChar;
    if ((pChar = pRenderComponent->GetCharacter(nSlot)) && GetComponent<IComponentPhysics>() && pChar->GetISkeletonPose()->GetCharacterPhysics() == GetPhysics())
    {
        GetComponent<IComponentPhysics>()->AttachToPhysicalEntity(0);
    }

    nSlot = pRenderComponent->LoadCharacter(nSlot, sFilename, nLoadFlags);
    if (nLoadFlags & EF_AUTO_PHYSICALIZE)
    {
        ICharacterInstance* pCharacter = GetCharacter(nSlot);
        if (pCharacter)
        {
            pCharacter->SetFlags(pCharacter->GetFlags() | CS_FLAG_UPDATE);
        }

        // Also physicalize geometry.
        SEntityPhysicalizeParams params;
        params.nSlot = nSlot;
        params.type = PE_RIGID;
        Physicalize(params);

        IPhysicalEntity* pe = GetPhysics();
        if (pe)
        {
            pe_action_awake aa;
            aa.bAwake = 0;
            pe->Action(&aa);
            pe_params_foreign_data pfd;
            pfd.iForeignFlagsOR = PFF_UNIMPORTANT;
            pe->SetParams(&pfd);
        }
    }

    ICharacterInstance* pCharacter = GetCharacter(nSlot);
    if (!AZ::CharacterBoundsNotificationBus::Handler::BusIsConnectedId(pCharacter))
    {
        AZ::CharacterBoundsNotificationBus::Handler::BusDisconnect();
        AZ::CharacterBoundsNotificationBus::Handler::BusConnect(pCharacter);
    }

    return nSlot;
}

#if defined(USE_GEOM_CACHES)
int CEntity::LoadGeomCache(int nSlot, const char* sFilename)
{
    IComponentRenderPtr pRenderComponent = GetOrCreateComponent<IComponentRender>();
    int ret = pRenderComponent->LoadGeomCache(nSlot, sFilename);

    SetMaterial(m_pMaterial);

    return ret;
}
#endif

//////////////////////////////////////////////////////////////////////////
int CEntity::SetParticleEmitter(int nSlot, IParticleEmitter* pEmitter, bool bSerialize)
{
    return GetOrCreateComponent<IComponentRender>()->SetParticleEmitter(nSlot, pEmitter, bSerialize);
}

//////////////////////////////////////////////////////////////////////////
int CEntity::LoadParticleEmitter(int nSlot, IParticleEffect* pEffect, SpawnParams const* params, bool bPrime, bool bSerialize)
{
    return GetOrCreateComponent<IComponentRender>()->LoadParticleEmitter(nSlot, pEffect, params, bPrime, bSerialize);
}

//////////////////////////////////////////////////////////////////////////

int CEntity::LoadLight(int nSlot, CDLight* pLight)
{
    uint16 layerId = ~0;
    return GetOrCreateComponent<IComponentRender>()->LoadLight(nSlot, pLight, layerId);
}

//////////////////////////////////////////////////////////////////////////
bool CEntity::UpdateLightClipVolumes(CDLight& light)
{
    for (IEntityLink* pLink = m_pEntityLinks; pLink; pLink = pLink->next)
    {
        IEntity* pLinkedEntity = gEnv->pEntitySystem->GetEntity(pLink->entityId);
        if (pLinkedEntity != nullptr)
        {
            const bool clipVolume = _stricmp(pLinkedEntity->GetClass()->GetName(), "ClipVolume") == 0;
            if (!clipVolume)
            {
                const IComponentClipVolumePtr pVolume = pLinkedEntity->GetComponent<IComponentClipVolume>();
                if (pVolume != nullptr)
                {
                    for (int i = 0; i < 2; ++i)
                    {
                        if (light.m_pClipVolumes[i] == NULL)
                        {
                            light.m_pClipVolumes[i] = pVolume->GetClipVolume();
                            light.m_Flags |= DLF_HAS_CLIP_VOLUME;

                            break;
                        }
                    }
                }
            }
        }
    }

    return (light.m_pClipVolumes[0] || light.m_pClipVolumes[1]);
}

//////////////////////////////////////////////////////////////////////////
int CEntity::LoadCloud(int nSlot, const char* sFilename)
{
    return GetOrCreateComponent<IComponentRender>()->LoadCloud(nSlot, sFilename);
}

//////////////////////////////////////////////////////////////////////////
int CEntity::SetCloudMovementProperties(int nSlot, const SCloudMovementProperties& properties)
{
    return GetOrCreateComponent<IComponentRender>()->SetCloudMovementProperties(nSlot, properties);
}

//////////////////////////////////////////////////////////////////////////
int CEntity::LoadFogVolume(int nSlot, const SFogVolumeProperties& properties)
{
    return GetOrCreateComponent<IComponentRender>()->LoadFogVolume(nSlot, properties);
}

//////////////////////////////////////////////////////////////////////////
int CEntity::FadeGlobalDensity(int nSlot, float fadeTime, float newGlobalDensity)
{
    return GetOrCreateComponent<IComponentRender>()->FadeGlobalDensity(nSlot, fadeTime, newGlobalDensity);
}

//////////////////////////////////////////////////////////////////////////
int CEntity::LoadVolumeObject(int nSlot, const char* sFilename)
{
    return GetOrCreateComponent<IComponentRender>()->LoadVolumeObject(nSlot, sFilename);
}

//////////////////////////////////////////////////////////////////////////
int CEntity::SetVolumeObjectMovementProperties(int nSlot, const SVolumeObjectMovementProperties& properties)
{
    return GetOrCreateComponent<IComponentRender>()->SetVolumeObjectMovementProperties(nSlot, properties);
}

#if !defined(EXCLUDE_DOCUMENTATION_PURPOSE)
int CEntity::LoadPrismObject(int nSlot)
{
    return GetOrCreateComponent<IComponentRender>()->LoadPrismObject(nSlot);
}
#endif // EXCLUDE_DOCUMENTATION_PURPOSE

//////////////////////////////////////////////////////////////////////////
bool CEntity::RegisterInAISystem(const AIObjectParams& params)
{
    m_flags &= ~ENTITY_FLAG_HAS_AI;

    IAISystem* pAISystem = gEnv->pAISystem;
    if (pAISystem)
    {
        IAIObjectManager* pAIObjMgr = pAISystem->GetAIObjectManager();
        if (IAIObject* pAIObject = GetAIObject())
        {
            CRY_ASSERT_TRACE(!m_bIsFromPool, ("Reregistering pool entity \"%s\" AI will break everything! (wierd behavior, dangling pointers, etc.", GetName()));
            if (m_bIsFromPool)
            {
                return false;
            }

            pAIObjMgr->RemoveObject(m_aiObjectID);
            m_aiObjectID = INVALID_AIOBJECTID;
            // The RemoveObject() call triggers immediate complete cleanup. Ideally the system would wait, as it does for internal removals. {2009/04/07}
        }

        if (params.type == 0)
        {
            return true;
        }

        AIObjectParams myparams(params);
        myparams.entityID = m_nID;
        myparams.name = GetName();

        if (IAIObject* pAIObject = pAIObjMgr->CreateAIObject(myparams))
        {
            m_aiObjectID = pAIObject->GetAIObjectID();

            m_flags |= ENTITY_FLAG_HAS_AI;

            UpdateAIObject();

            return true;
        }
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
// reflect changes in position or orientation to the AI object
void CEntity::UpdateAIObject()
{
    IAIObject* pAIObject = GetAIObject();
    if (pAIObject)
    {
        pAIObject->SetPos(GetWorldPos(), GetForwardDir());
    }
}

//////////////////////////////////////////////////////////////////////////
void CEntity::ActivateForNumUpdates(uint32 numUpdates)
{
    if (m_bActive)
    {
        return;
    }

    IAIObject* pAIObject = GetAIObject();
    if (pAIObject && pAIObject->GetProxy())
    {
        return;
    }

    if (m_nUpdateCounter != 0)
    {
        m_nUpdateCounter = numUpdates;
        return;
    }

    m_nUpdateCounter = numUpdates;
    SetUpdateStatus();
}

//////////////////////////////////////////////////////////////////////////
void CEntity::SetPhysicsState(XmlNodeRef& physicsState)
{
    if (physicsState)
    {
        IPhysicalEntity* physic = GetPhysics();
        if (!physic && GetCharacter(0))
        {
            physic = GetCharacter(0)->GetISkeletonPose()->GetCharacterPhysics(0);
        }
        if (physic)
        {
            IXmlSerializer* pSerializer = GetISystem()->GetXmlUtils()->CreateXmlSerializer();
            if (pSerializer)
            {
                ISerialize* ser = pSerializer->GetReader(physicsState);
                physic->SetStateFromSnapshot(ser);
                physic->PostSetStateFromSnapshot();
                pSerializer->Release();
                for (int i = GetSlotCount() - 1; i >= 0; i--)
                {
                    if (GetSlot(i)->pCharacter)
                    {
                        if (GetSlot(i)->pCharacter->GetISkeletonPose()->GetCharacterPhysics() == physic)
                        {
                            GetSlot(i)->pCharacter->GetISkeletonPose()->SynchronizeWithPhysicalEntity(physic);
                        }
                        else if (GetSlot(i)->pCharacter->GetISkeletonPose()->GetCharacterPhysics(0) == physic)
                        {
                            GetSlot(i)->pCharacter->GetISkeletonPose()->SynchronizeWithPhysicalEntity(0, m_vPos, m_qRotation);
                        }
                    }
                }
                ActivateForNumUpdates(5);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
float CEntity::GetObstructionMultiplier() const
{
    // confirm that this is an acceptable way to do this...
    if (const IComponentPhysics* physicsComponent = GetComponent<IComponentPhysics>().get())
    {
        return static_cast<const CComponentPhysics*>(physicsComponent)->GetAudioObstructionMultiplier();
    }
    return 1.f;
}

//////////////////////////////////////////////////////////////////////////
void CEntity::SetObstructionMultiplier(float obstructionMultiplier)
{
    // confirm that this is an acceptable way to do this...
    if (IComponentPhysics* physicsComponent = GetComponent<IComponentPhysics>().get())
    {
        static_cast<CComponentPhysics*>(physicsComponent)->SetAudioObstructionMultiplier(obstructionMultiplier);
    }
}

//////////////////////////////////////////////////////////////////////////
IEntityLink* CEntity::GetEntityLinks()
{
    return m_pEntityLinks;
};

//////////////////////////////////////////////////////////////////////////
IEntityLink* CEntity::AddEntityLink(const char* sLinkName, EntityId entityId, EntityGUID entityGuid)
{
    assert(sLinkName);
    if (sLinkName == NULL)
    {
        return NULL;
    }

    IEntity* pLinkedEntity = GetEntitySystem()->GetEntity(entityId);

    IEntityLink* pNewLink = new IEntityLink;
    assert(strlen(sLinkName) <= ENTITY_LINK_NAME_MAX_LENGTH);
    cry_strcpy(pNewLink->name, sLinkName);
    pNewLink->entityId = entityId;
    pNewLink->entityGuid = entityGuid;
    pNewLink->next = 0;

    if (m_pEntityLinks)
    {
        IEntityLink* pLast = m_pEntityLinks;
        while (pLast->next)
        {
            pLast = pLast->next;
        }
        pLast->next = pNewLink;
    }
    else
    {
        m_pEntityLinks = pNewLink;
    }

    // Send event.
    SEntityEvent event(ENTITY_EVENT_LINK);
    event.nParam[0] = (INT_PTR)pNewLink;
    SendEvent(event);

    return pNewLink;
};

//////////////////////////////////////////////////////////////////////////
void CEntity::RemoveEntityLink(IEntityLink* pLink)
{
    if (!m_pEntityLinks || !pLink)
    {
        return;
    }

    // Send event.
    SEntityEvent event(ENTITY_EVENT_DELINK);
    event.nParam[0] = (INT_PTR)pLink;
    SendEvent(event);

    if (m_pEntityLinks == pLink)
    {
        m_pEntityLinks = m_pEntityLinks->next;
    }
    else
    {
        IEntityLink* pLast = m_pEntityLinks;
        while (pLast->next && pLast->next != pLink)
        {
            pLast = pLast->next;
        }
        if (pLast->next == pLink)
        {
            pLast->next = pLink->next;
        }
    }

    delete pLink;
}

//////////////////////////////////////////////////////////////////////////
void CEntity::RemoveAllEntityLinks()
{
    IEntityLink* pLink = m_pEntityLinks;
    while (pLink)
    {
        IEntityLink* pNext = pLink->next;

        // Send event.
        SEntityEvent event(ENTITY_EVENT_DELINK);
        event.nParam[0] = (INT_PTR)pLink;
        SendEvent(event);

        delete pLink;
        pLink = pNext;
    }
    m_pEntityLinks = 0;
}

//////////////////////////////////////////////////////////////////////////
void CEntity::GetMemoryUsage(ICrySizer* pSizer) const
{
    pSizer->AddObject(this, sizeof(*this));
    ForEachComponent([pSizer](const TComponentPair& componentPair)
        {
            pSizer->AddObject(componentPair.second);
        });
}

//////////////////////////////////////////////////////////////////////////
#define DEF_ENTITY_EVENT_NAME(x) {x,#x}
struct SEventName
{
    EEntityEvent event;
    const char* name;
} s_eventsName[] =
{
    DEF_ENTITY_EVENT_NAME(ENTITY_EVENT_XFORM),
    DEF_ENTITY_EVENT_NAME(ENTITY_EVENT_TIMER),
    DEF_ENTITY_EVENT_NAME(ENTITY_EVENT_INIT),
    DEF_ENTITY_EVENT_NAME(ENTITY_EVENT_DONE),
    DEF_ENTITY_EVENT_NAME(ENTITY_EVENT_VISIBLITY),
    DEF_ENTITY_EVENT_NAME(ENTITY_EVENT_RESET),
    DEF_ENTITY_EVENT_NAME(ENTITY_EVENT_ATTACH),
    DEF_ENTITY_EVENT_NAME(ENTITY_EVENT_ATTACH_THIS),
    DEF_ENTITY_EVENT_NAME(ENTITY_EVENT_DETACH),
    DEF_ENTITY_EVENT_NAME(ENTITY_EVENT_DETACH_THIS),
    DEF_ENTITY_EVENT_NAME(ENTITY_EVENT_LINK),
    DEF_ENTITY_EVENT_NAME(ENTITY_EVENT_DELINK),
    DEF_ENTITY_EVENT_NAME(ENTITY_EVENT_HIDE),
    DEF_ENTITY_EVENT_NAME(ENTITY_EVENT_UNHIDE),
    DEF_ENTITY_EVENT_NAME(ENTITY_EVENT_ENABLE_PHYSICS),
    DEF_ENTITY_EVENT_NAME(ENTITY_EVENT_PHYSICS_CHANGE_STATE),
    DEF_ENTITY_EVENT_NAME(ENTITY_EVENT_SCRIPT_EVENT),
    DEF_ENTITY_EVENT_NAME(ENTITY_EVENT_ENTERAREA),
    DEF_ENTITY_EVENT_NAME(ENTITY_EVENT_LEAVEAREA),
    DEF_ENTITY_EVENT_NAME(ENTITY_EVENT_ENTERNEARAREA),
    DEF_ENTITY_EVENT_NAME(ENTITY_EVENT_LEAVENEARAREA),
    DEF_ENTITY_EVENT_NAME(ENTITY_EVENT_MOVEINSIDEAREA),
    DEF_ENTITY_EVENT_NAME(ENTITY_EVENT_MOVENEARAREA),
    DEF_ENTITY_EVENT_NAME(ENTITY_EVENT_PHYS_POSTSTEP),
    DEF_ENTITY_EVENT_NAME(ENTITY_EVENT_PHYS_BREAK),
    DEF_ENTITY_EVENT_NAME(ENTITY_EVENT_AI_DONE),
    DEF_ENTITY_EVENT_NAME(ENTITY_EVENT_SOUND_DONE),
    DEF_ENTITY_EVENT_NAME(ENTITY_EVENT_NOT_SEEN_TIMEOUT),
    DEF_ENTITY_EVENT_NAME(ENTITY_EVENT_COLLISION),
    DEF_ENTITY_EVENT_NAME(ENTITY_EVENT_RENDER),
    DEF_ENTITY_EVENT_NAME(ENTITY_EVENT_PREPHYSICSUPDATE),
    DEF_ENTITY_EVENT_NAME(ENTITY_EVENT_LEVEL_LOADED),
    DEF_ENTITY_EVENT_NAME(ENTITY_EVENT_START_LEVEL),
    DEF_ENTITY_EVENT_NAME(ENTITY_EVENT_START_GAME),
    DEF_ENTITY_EVENT_NAME(ENTITY_EVENT_ENTER_SCRIPT_STATE),
    DEF_ENTITY_EVENT_NAME(ENTITY_EVENT_LEAVE_SCRIPT_STATE),
    DEF_ENTITY_EVENT_NAME(ENTITY_EVENT_PRE_SERIALIZE),
    DEF_ENTITY_EVENT_NAME(ENTITY_EVENT_POST_SERIALIZE),
    DEF_ENTITY_EVENT_NAME(ENTITY_EVENT_INVISIBLE),
    DEF_ENTITY_EVENT_NAME(ENTITY_EVENT_VISIBLE),
    DEF_ENTITY_EVENT_NAME(ENTITY_EVENT_MATERIAL),
    DEF_ENTITY_EVENT_NAME(ENTITY_EVENT_ONHIT),
    DEF_ENTITY_EVENT_NAME(ENTITY_EVENT_CROSS_AREA),
    DEF_ENTITY_EVENT_NAME(ENTITY_EVENT_ACTIVATED),
    DEF_ENTITY_EVENT_NAME(ENTITY_EVENT_DEACTIVATED),
};

//////////////////////////////////////////////////////////////////////////
void CEntity::LogEvent(SEntityEvent& event, CTimeValue dt)
{
    static int s_LastLoggedFrame = 0;

    int nFrameId = gEnv->pRenderer->GetFrameID(false);
    if (CVar::es_DebugEvents == 2 && s_LastLoggedFrame < nFrameId && s_LastLoggedFrame > nFrameId - 10)
    {
        // On Next frame turn it off.
        CVar::es_DebugEvents = 0;
        return;
    }

    // Find event, quite slow but ok for logging.
    char sNameId[32];
    const char* sName = "";
    for (int i = 0; i < sizeof(s_eventsName) / sizeof(s_eventsName[0]); i++)
    {
        if (s_eventsName[i].event == event.event)
        {
            sName = s_eventsName[i].name;
            break;
        }
    }
    if (!*sName)
    {
        sprintf_s(sNameId, "%d", (int)event.event);
        sName = sNameId;
    }
    s_LastLoggedFrame = nFrameId;

    float fTimeMs = dt.GetMilliSeconds();
    CryLogAlways("<Frame:%d><EntityEvent> [%s](%X)\t[%.2fms]\t%s", nFrameId, sName, (int)event.nParam[0], fTimeMs, GetEntityTextDescription());
}

IAIObject* CEntity::GetAIObject()
{
    return (m_aiObjectID ? gEnv->pAISystem->GetAIObjectManager()->GetAIObject(m_aiObjectID) : NULL);
}

IComponentPtr CEntity::GetComponentImpl(const ComponentType& type)
{
    auto component = m_components.find(type);
    if (component != m_components.end())
    {
        return component->second;
    }
    return nullptr;
}

IComponentConstPtr CEntity::GetComponentImpl(const ComponentType& type) const
{
    auto component = m_components.find(type);
    if (component != m_components.end())
    {
        return component->second;
    }
    return nullptr;
}

//////////////////////////////////////////////////////////////////////////
void CEntity::DebugDraw(const SGeometryDebugDrawInfo& info)
{
    IComponentRenderPtr pRenderComponent = GetComponent<IComponentRender>();
    if (pRenderComponent)
    {
        pRenderComponent->DebugDraw(info);
    }
}

//////////////////////////////////////////////////////////////////////////
Matrix34 CEntity::GetParentAttachPointWorldTM() const
{
    if (!m_pBinds)
    {
        return Matrix34(IDENTITY);
    }

    if (m_pBinds->parentBindingType == SBindings::eBT_GeomCacheNode)
    {
#if defined(USE_GEOM_CACHES)
        CGeomCacheAttachmentManager* pGeomCacheAttachmentManager = static_cast<CEntitySystem*>(GetEntitySystem())->GetGeomCacheAttachmentManager();
        return pGeomCacheAttachmentManager->GetNodeWorldTM(this, m_pBinds->pParent);
#endif
    }
    else if (m_pBinds->parentBindingType == SBindings::eBT_CharacterBone)
    {
        CCharacterBoneAttachmentManager* pCharacterBoneAttachmentManager = static_cast<CEntitySystem*>(GetEntitySystem())->GetCharacterBoneAttachmentManager();
        return pCharacterBoneAttachmentManager->GetNodeWorldTM(this, m_pBinds->pParent);
    }

    return m_pBinds->pParent->m_worldTM;
}

//////////////////////////////////////////////////////////////////////////
bool CEntity::IsParentAttachmentValid() const
{
    if (!m_pBinds || !m_pBinds->pParent)
    {
        return false;
    }

    if (m_pBinds->parentBindingType == SBindings::eBT_GeomCacheNode)
    {
#if defined(USE_GEOM_CACHES)
        CGeomCacheAttachmentManager* pGeomCacheAttachmentManager = static_cast<CEntitySystem*>(GetEntitySystem())->GetGeomCacheAttachmentManager();
        return pGeomCacheAttachmentManager->IsAttachmentValid(this, m_pBinds->pParent);
#endif
    }
    else if (m_pBinds->parentBindingType == SBindings::eBT_CharacterBone)
    {
        CCharacterBoneAttachmentManager* pCharacterBoneAttachmentManager = static_cast<CEntitySystem*>(GetEntitySystem())->GetCharacterBoneAttachmentManager();
        return pCharacterBoneAttachmentManager->IsAttachmentValid(this, m_pBinds->pParent);
    }

    return true;
}

