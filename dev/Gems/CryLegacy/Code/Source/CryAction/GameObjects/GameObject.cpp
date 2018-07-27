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
#include "GameObjects/GameObject.h"
#include "CryAction.h"
#include "Network/GameContext.h"
#include "Serialization/SerializeScriptTableWriter.h"
#include "Serialization/SerializeScriptTableReader.h"
#include "GameObjectSystem.h"
#include "ITextModeConsole.h"
#include "CryActionCVars.h"
#include "Components/IComponentSerialization.h"

#include <IAIObject.h>
#include <IAIActorProxy.h>

// ugly: for GetMovementController()
#include "IActorSystem.h"
#include "IVehicleSystem.h"

#include "INetwork.h"

#include "Components/IComponentRender.h"
#include "Components/IComponentPhysics.h"
#include "Components/IComponentTrigger.h"
#include "Components/IComponentUser.h"

//#pragma optimize("", off)
//#pragma inline_depth(0)

DECLARE_DEFAULT_COMPONENT_FACTORY(CGameObject, IComponentUser)

#define GET_FLAG_FOR_SLOT(flag, slotbit) (((flag) & (slotbit)) != 0)
#define SET_FLAG_FOR_SLOT(flag, slotbit, value) if (value) { flag |= (slotbit); } else { flag &= ~slotbit; }

static const size_t MAX_REMOVING_EXTENSIONS = 16;
//static const size_t MAX_ADDING_EXTENSIONS = 16;
static const size_t NOT_IN_UPDATE_MODE = size_t(-1);
static EntityId updatingEntity = 0;
static size_t numRemovingExtensions = NOT_IN_UPDATE_MODE;
static IGameObjectSystem::ExtensionID removingExtensions[MAX_REMOVING_EXTENSIONS];
CGameObject::SExtension CGameObject::m_addingExtensions[CGameObject::MAX_ADDING_EXTENSIONS];
int CGameObject::m_nAddingExtension = 0;
CGameObjectSystem* CGameObject::m_pGOS = 0;

static const float UPDATE_TIMEOUT_HUGE = 1e20f;
static const float DISTANCE_CHECKER_TIMEOUT = 1.3f;
static const float FAR_AWAY_DISTANCE = 150.0f;

// MUST BE SYNCHRONIZED WITH EUpdateStates
const float CGameObject::UpdateTimeouts[eUS_COUNT_STATES] =
{
    3.0f, // eUS_Visible_Close
    5.0f, // eUS_Visible_FarAway
    UPDATE_TIMEOUT_HUGE, // eUS_NotVisible_Close
    UPDATE_TIMEOUT_HUGE, // eUS_NotVisible_FarAway
    27.0f, // eUS_CheckVisibility_Close
    13.0f, // eUS_CheckVisibility_FarAway
};
const char* CGameObject::UpdateNames[eUS_COUNT_STATES] =
{
    "Visible_Close", // eUS_Visible_Close
    "Visible_FarAway", // eUS_Visible_FarAway
    "NotVisible_Close", // eUS_NotVisible_Close
    "NotVisible_FarAway", // eUS_NotVisible_FarAway
    "CheckVisibility_Close", // eUS_CheckVisibility_Close
    "CheckVisibility_FarAway", // eUS_CheckVisibility_FarAway
};
const char* CGameObject::EventNames[eUSE_COUNT_EVENTS] =
{
    "BecomeVisible",
    "BecomeClose",
    "BecomeFarAway",
    "Timeout",
};
const CGameObject::EUpdateState CGameObject::UpdateTransitions[eUS_COUNT_STATES][eUSE_COUNT_EVENTS] =
{
    /*
  //eUS_CheckVisibility_FarAway:// {eUS_CheckVisibility_FarAway, eUS_CheckVisibility_FarAway, eUS_CheckVisibility_FarAway, eUS_CheckVisibility_FarAway}
                                     {eUSE_BecomeVisible         , eUSE_BecomeClose           , eUSE_BecomeFarAway         , eUSE_Timeout               }
  */
    /*eUS_Visible_Close:*/
    {eUS_INVALID, eUS_Visible_Close, eUS_Visible_FarAway, eUS_CheckVisibility_Close  },
    /*eUS_Visible_FarAway:*/ {eUS_INVALID, eUS_Visible_Close, eUS_Visible_FarAway, eUS_CheckVisibility_FarAway},
    /*eUS_NotVisible_Close:*/ {eUS_Visible_Close, eUS_NotVisible_Close, eUS_NotVisible_FarAway, eUS_NotVisible_Close       },
    /*eUS_NotVisible_FarAway:*/ {eUS_Visible_FarAway, eUS_NotVisible_Close, eUS_NotVisible_FarAway, eUS_NotVisible_FarAway     },
    /*eUS_CheckVisibility_Close:*/ {eUS_Visible_Close, eUS_CheckVisibility_Close, eUS_CheckVisibility_FarAway, eUS_NotVisible_Close       },
    /*eUS_CheckVisibility_FarAway:*/ {eUS_Visible_FarAway, eUS_CheckVisibility_Close, eUS_CheckVisibility_FarAway, eUS_NotVisible_FarAway     },
};

static int g_TextModeY = 0;
static float g_y;
static CTimeValue g_lastUpdate;
static int g_showUpdateState = 0;

static AABB CreateDistanceAABB(Vec3 center, float radius = FAR_AWAY_DISTANCE)
{
    static const float CHECK_GRANULARITY = 2.0f;
    Vec3 mn, mx;
    for (int i = 0; i < 3; i++)
    {
        center[i] = floor_tpl(center[i] / CHECK_GRANULARITY) * CHECK_GRANULARITY;
        mn[i] = center[i] - radius * 0.5f;
        mx[i] = center[i] + radius * 0.5f;
    }
    return AABB(mn, mx);
}

static int g_forceFastUpdate = 0;
static int g_visibilityTimeout = 0;
static float g_visibilityTimeoutTime = 0.0f;

void CGameObject::CreateCVars()
{
    REGISTER_CVAR(g_forceFastUpdate, 0, VF_CHEAT, "GameObjects IsProbablyVisible->TRUE && IsProbablyDistant()->FALSE");
    REGISTER_CVAR(g_visibilityTimeout, 0, VF_CHEAT, "Adds visibility timeout to IsProbablyVisible() calculations");
    REGISTER_CVAR(g_visibilityTimeoutTime, 30.0f, VF_CHEAT, "Visibility timeout time used by IsProbablyVisible() calculations");
    REGISTER_CVAR(g_showUpdateState, 0, VF_CHEAT, "Show the game object update state of any activated entities; 3-4 -- AI objects only");
}

//------------------------------------------------------------------------
CGameObject::CGameObject()
    : IComponentUser()
    , m_pActionDelegate(0)
    , m_pViewDelegate(0)
    , m_pProfileManager(0)
    ,
#if GAME_OBJECT_SUPPORTS_CUSTOM_USER_DATA
    m_pUserData(0)
    ,
#endif
    m_enabledAspects(NET_ASPECT_ALL)
    , m_delegatableAspects(NET_ASPECT_ALL)
    , m_enabledPhysicsEvents(0)
    , m_forceUpdate(0)
    , m_updateState(eUS_NotVisible_FarAway)
    , m_updateTimer(0.1f)
    , m_isBoundToNetwork(false)
    , m_inRange(false)
    , m_justExchanging(false)
    , m_aiMode(eGOAIAM_VisibleOrInRange)
    , m_physDisableMode(eADPM_Never)
    , m_prePhysicsUpdateRule(ePPU_Never)
    , m_bPrePhysicsEnabled(false)
    , m_predictionHandle(0)
    , m_bPhysicsDisabled(false)
    , m_bNoSyncPhysics(false)
    , m_bNeedsNetworkRebind(false)
    , m_channelId(0)
{
    COMPILE_TIME_ASSERT(eGFE_Last <= 64);

    if (!m_pGOS)
    {
        m_pGOS = (CGameObjectSystem*) CCryAction::GetCryAction()->GetIGameObjectSystem();
    }

    for (int i = 0; i < NUM_ASPECTS; i++)
    {
        m_profiles[i] = 255;
    }

    m_bVisible = true;
}

//------------------------------------------------------------------------
CGameObject::~CGameObject()
{
}

//------------------------------------------------------------------------
ILINE bool CGameObject::TestIsProbablyVisible(uint state)
{
    if (g_forceFastUpdate)
    {
        return false;
    }

    switch (state)
    {
    case eUS_Visible_Close:
    case eUS_Visible_FarAway:
    case eUS_CheckVisibility_Close:
    case eUS_CheckVisibility_FarAway:
        return true;
    case eUS_NotVisible_FarAway:
    case eUS_NotVisible_Close:
        return false;
    }
    CRY_ASSERT(false);
    return false;
}

//------------------------------------------------------------------------
ILINE bool CGameObject::TestIsProbablyDistant(uint state)
{
    if (g_forceFastUpdate)
    {
        return true;
    }

    switch (state)
    {
    case eUS_Visible_FarAway:
    case eUS_CheckVisibility_FarAway:
    case eUS_NotVisible_FarAway:
        return true;
    case eUS_CheckVisibility_Close:
    case eUS_Visible_Close:
    case eUS_NotVisible_Close:
        return false;
    }
    CRY_ASSERT(false);
    return false;
}

//------------------------------------------------------------------------
bool CGameObject::IsProbablyVisible()
{
    // this would be a case of driver in a tank - hidden, but needes to be updated
    if (GetEntity()->IsHidden() && (GetEntity()->GetFlags() & ENTITY_FLAG_UPDATE_HIDDEN))
    {
        return true;
    }

    if (GetEntity()->IsHidden())
    {
        return false;
    }

    if (g_visibilityTimeout)
    {
        IComponentRenderPtr pComponent = GetEntity()->GetComponent<IComponentRender>();
        if (pComponent)
        {
            float fDiff = gEnv->pTimer->GetCurrTime() - pComponent->GetLastSeenTime();
            if (fDiff > g_visibilityTimeoutTime)
            {
                if (m_bVisible)
                {
                    if (g_visibilityTimeout == 2)
                    {
                        CryLogAlways("Entity %s not visible for %f seconds - visibility timeout", GetEntity()->GetName(), fDiff);
                    }
                }
                m_bVisible = false;
                return (false);
            }
        }
    }

    if (!m_bVisible)
    {
        if (g_visibilityTimeout == 2)
        {
            CryLogAlways("Entity %s is now visible again after visibility timeout", GetEntity()->GetName());
        }
    }
    m_bVisible = true;

    return TestIsProbablyVisible(m_updateState);
}

//------------------------------------------------------------------------
bool CGameObject::IsProbablyDistant()
{
    //it's hidden - far away
    if (GetEntity()->IsHidden() && !(GetEntity()->GetFlags() & ENTITY_FLAG_UPDATE_HIDDEN))
    {
        return true;
    }

    if (g_visibilityTimeout)
    {
        if (!m_bVisible)
        {
            return (true);
        }
    }
    return TestIsProbablyDistant(m_updateState);
}

//------------------------------------------------------------------------
void CGameObject::UpdateStateEvent(EUpdateStateEvent evt)
{
    EUpdateState newState = UpdateTransitions[m_updateState][evt];

    if (newState >= eUS_INVALID)
    {
        //GameWarning("%s: visibility activation requested invalid state (%s on event %s)", GetEntity()->GetName(), UpdateNames[m_updateState], EventNames[m_updateState] );
        return;
    }

    if (newState != m_updateState)
    {
        if (TestIsProbablyVisible(newState) && !TestIsProbablyVisible(m_updateState))
        {
            SGameObjectEvent goe(eGFE_OnBecomeVisible, eGOEF_ToExtensions);
            SendEvent(goe);
        }

        m_updateState = newState;
        m_updateTimer = UpdateTimeouts[m_updateState];

        uint32 flags = m_pEntity->GetFlags();
        if (UpdateTransitions[m_updateState][eUSE_BecomeVisible] != eUS_INVALID)
        {
            flags |= ENTITY_FLAG_SEND_RENDER_EVENT;
        }
        else
        {
            flags &= ~ENTITY_FLAG_SEND_RENDER_EVENT;
        }
        m_pEntity->SetFlags(flags);

        EvaluateUpdateActivation();
    }
}

//------------------------------------------------------------------------
bool CGameObject::ShouldUpdateSlot(const SExtension* pExt, uint32 slot, uint32 slotbit, bool checkAIDisable)
{
    if (checkAIDisable)
    {
        if (GET_FLAG_FOR_SLOT(pExt->flagDisableWithAI, slotbit))
        {
            return false;
        }
    }

    if (pExt->forceEnables[slot])
    {
        return true;
    }

    if (GET_FLAG_FOR_SLOT(pExt->flagNeverUpdate, slotbit))
    {
        return false;
    }

    if (!pExt->updateEnables[slot])
    {
        return false;
    }

    bool visibleCheck = true;
    if (GET_FLAG_FOR_SLOT(pExt->flagUpdateWhenVisible, slotbit))
    {
        visibleCheck = TestIsProbablyVisible(m_updateState);
    }

    bool inRangeCheck = true;
    if (GET_FLAG_FOR_SLOT(pExt->flagUpdateWhenInRange, slotbit))
    {
        inRangeCheck = !TestIsProbablyDistant(m_updateState);
    }

    if (GET_FLAG_FOR_SLOT(pExt->flagUpdateCombineOr, slotbit))
    {
        return visibleCheck || inRangeCheck;
    }
    else
    {
        return visibleCheck && inRangeCheck;
    }
}

//------------------------------------------------------------------------
bool CGameObject::InitComponent(IEntity* pEntity, SEntitySpawnParams& spawnParams)
{
    m_pEntity = pEntity;
    m_pEntity->GetComponent<IComponentSerialization>()->Register<CGameObject>(SerializationOrder::User, *this, &CGameObject::Serialize, &CGameObject::SerializeXML, &CGameObject::NeedSerialize, &CGameObject::GetSignature);

    m_entityId = pEntity->GetId();

    IGameObjectSystem* pGOS = m_pGOS;

    if (!m_extensions.empty())
    {
        TExtensions preExtensions;
        m_extensions.swap(preExtensions);
        // this loop repopulates m_extensions
        for (TExtensions::const_iterator iter = preExtensions.begin(); iter != preExtensions.end(); ++iter)
        {
            const char* szExtensionName = pGOS->GetName(iter->id);
            if (!ChangeExtension(szExtensionName, eCE_Activate))
            {
                if (szExtensionName)
                {
                    gEnv->pLog->LogError("[GameObject]: Couldn't activate extension %s", szExtensionName);
                }
                return false;
            }
        }

        // set sticky bits, clear activated bits (extensions activated before initialization are sticky)
        for (TExtensions::iterator iter = m_extensions.begin(); iter != m_extensions.end(); ++iter)
        {
            iter->sticky = iter->activated;
            iter->activated = false;
        }
    }

    GetEntity()->SetFlags(GetEntity()->GetFlags() | ENTITY_FLAG_SEND_RENDER_EVENT);

    return true;
}

//------------------------------------------------------------------------
void CGameObject::Reload(IEntity* pEntity, SEntitySpawnParams& spawnParams)
{
    assert(pEntity);
    assert(m_pEntity == pEntity);
    PREFAST_ASSUME(pEntity);

    m_channelId = 0;
    m_pEntity = pEntity;
    m_entityId = pEntity ? pEntity->GetId() : 0;
    m_pMovementController = NULL;

    m_pActionDelegate = 0;
    m_pViewDelegate = 0;
    m_pProfileManager = 0;
    m_enabledAspects = NET_ASPECT_ALL;
    ;
    m_delegatableAspects = NET_ASPECT_ALL;
    m_enabledPhysicsEvents = 0;
    m_forceUpdate = 0;
    m_updateState = eUS_NotVisible_FarAway;
    m_updateTimer = 0.1f;
    m_isBoundToNetwork = false;
    m_inRange = false;
    m_justExchanging = false;
    m_aiMode = eGOAIAM_VisibleOrInRange;
    m_physDisableMode = eADPM_Never;
    m_prePhysicsUpdateRule = ePPU_Never;
    m_bPrePhysicsEnabled = false;
    m_predictionHandle = 0;
    m_bPhysicsDisabled = false;
    m_bNoSyncPhysics = false;
    m_bNeedsNetworkRebind = false;

    m_pEntity->SetFlags(m_pEntity->GetFlags() | ENTITY_FLAG_SEND_RENDER_EVENT);

    m_distanceChecker.Reset();

    if (!m_extensions.empty())
    {
        TExtensions keepExtensions;
        keepExtensions.reserve(m_extensions.size());
        for (TExtensions::iterator itExtension = m_extensions.begin(), itExtensionsEnd = m_extensions.end(); itExtension != itExtensionsEnd; ++itExtension)
        {
            SExtension& extension = *itExtension;
            assert(extension.pExtension.get());

            if (!extension.pExtension || !extension.pExtension->ReloadExtension(this, spawnParams))
            {
                GetEntity()->DeregisterComponent(extension.pExtension);
                extension.pExtension.reset();
            }
            else
            {
                keepExtensions.push_back(extension);
            }
        }

        m_extensions.resize(keepExtensions.size());
        std::copy(keepExtensions.begin(), keepExtensions.end(), m_extensions.begin());

        bool bHasPostUpdates = false;
        if (!keepExtensions.empty())
        {
            for (TExtensions::iterator itExtension = keepExtensions.begin(), itExtensionsEnd = keepExtensions.end(); itExtension != itExtensionsEnd; ++itExtension)
            {
                SExtension& extension = *itExtension;
                assert(extension.pExtension.get());

                extension.pExtension->PostReloadExtension(this, spawnParams);
                bHasPostUpdates |= extension.postUpdate;
            }
        }

        m_pGOS->SetPostUpdate(this, bHasPostUpdates);
    }

    EvaluateUpdateActivation();
}

bool CGameObject::BindToNetwork(EBindToNetworkMode mode)
{
    return BindToNetworkWithParent(mode, 0);
}

bool CGameObject::BindToNetworkWithParent(EBindToNetworkMode mode, EntityId parentId)
{
    m_isBoundToNetwork = true;
    EvaluateUpdateActivation();

    return true;
}

bool CGameObject::IsAspectDelegatable(NetworkAspectType aspect)
{
    return (m_delegatableAspects & aspect) ? true : false;
}

//------------------------------------------------------------------------
void CGameObject::Done()
{
    // cannot release extensions while Physics is in a CB.
    AcquireMutex();

    // Before deleting the extension, disable receiving any physics events.
    // Mutex prevents
    m_enabledPhysicsEvents = 0;

    FlushExtensions(true);
    m_pGOS->SetPostUpdate(this, false);
    m_distanceChecker.Reset();

    ReleaseMutex();
}

//------------------------------------------------------------------------
void CGameObject::DebugUpdateState()
{
    ITextModeConsole* pTMC = GetISystem()->GetITextModeConsole();

    float white[] = {1, 1, 1, 1};

    char buf[512];
    char* pOut = buf;

    pOut += sprintf_s(pOut, sizeof(buf), "%s: %s", GetEntity()->GetName(), UpdateNames[m_updateState]);
    if (m_updateTimer < (0.5f * UPDATE_TIMEOUT_HUGE))
    {
        pOut += sprintf_s(pOut, sizeof(buf) - (pOut - buf), " timeout:%.2f", m_updateTimer);
    }
    if (ShouldUpdateAI())
    {
        pOut += sprintf_s(pOut, sizeof(buf) - (pOut - buf), m_aiMode == eGOAIAM_Always ? "AI_ALWAYS" : " AIACTIVE");
    }
    if (m_bPrePhysicsEnabled)
    {
        pOut += sprintf_s(pOut, sizeof(buf) - (pOut - buf), " PREPHYSICS");
    }
    if (m_prePhysicsUpdateRule != ePPU_Never)
    {
        pOut += sprintf_s(pOut, sizeof(buf) - (pOut - buf), " pprule=%s", m_prePhysicsUpdateRule == ePPU_Always ? "always" : "whenai");
    }
    if (m_forceUpdate)
    {
        pOut += sprintf_s(pOut, sizeof(buf) - (pOut - buf), " force:%d", m_forceUpdate);
    }
    if (m_physDisableMode != eADPM_Never)
    {
        pOut += sprintf_s(pOut, sizeof(buf) - (pOut - buf), " physDisable:%d", m_physDisableMode);
    }
    gEnv->pRenderer->Draw2dLabel(10, g_y += 10, 1, white, false, "%s", buf);
    if (pTMC)
    {
        pTMC->PutText(0, g_TextModeY++, buf);
    }

    if (g_showUpdateState == 2)
    {
        bool checkAIDisable = !ShouldUpdateAI() && GetEntity()->GetAI();
        for (TExtensions::iterator iter = m_extensions.begin(); iter != m_extensions.end(); ++iter)
        {
            uint slotbit = 1;

            for (int slot = 0; slot < MAX_UPDATE_SLOTS_PER_EXTENSION; slot++)
            {
                pOut = buf;
                pOut += sprintf_s(pOut, sizeof(buf) - (pOut - buf), "%s[%d]: en:%d", m_pGOS->GetName(iter->id), slot, iter->updateEnables[slot]);

                if (iter->forceEnables[slot])
                {
                    pOut += sprintf_s(pOut, sizeof(buf) - (pOut - buf), " force:%d", int(iter->forceEnables[slot]));
                }

                bool done = false;
                if (checkAIDisable)
                {
                    if (done = GET_FLAG_FOR_SLOT(iter->flagDisableWithAI, slotbit))
                    {
                        pOut += sprintf_s(pOut, sizeof(buf) - (pOut - buf), " ai-disable");
                    }
                }

                if (!done && (done = GET_FLAG_FOR_SLOT(iter->flagNeverUpdate, slotbit)))
                {
                    pOut += sprintf_s(pOut, sizeof(buf) - (pOut - buf), " never");
                }

                if (!done)
                {
                    bool visibleCheck = true;
                    if (GET_FLAG_FOR_SLOT(iter->flagUpdateWhenVisible, slotbit))
                    {
                        visibleCheck = TestIsProbablyVisible(m_updateState);
                    }

                    bool inRangeCheck = true;
                    if (GET_FLAG_FOR_SLOT(iter->flagUpdateWhenInRange, slotbit))
                    {
                        inRangeCheck = !TestIsProbablyDistant(m_updateState);
                    }

                    pOut += sprintf_s(pOut, sizeof(buf) - (pOut - buf), " %s", visibleCheck ? "visible" : "!visible");
                    if (GET_FLAG_FOR_SLOT(iter->flagUpdateCombineOr, slotbit))
                    {
                        pOut += sprintf_s(pOut, sizeof(buf) - (pOut - buf), " ||");
                    }
                    else
                    {
                        pOut += sprintf_s(pOut, sizeof(buf) - (pOut - buf), " &&");
                    }
                    pOut += sprintf_s(pOut, sizeof(buf) - (pOut - buf), " %s", inRangeCheck ? "in-range" : "!in-range");
                }
                if (ShouldUpdateSlot(&*iter, slot, slotbit, checkAIDisable))
                {
                    gEnv->pRenderer->Draw2dLabel(20, g_y += 10, 1, white, false, "%s", buf);
                    if (pTMC)
                    {
                        pTMC->PutText(1, g_TextModeY++, buf);
                    }
                }

                slotbit <<= 1;
            }
        }
    }
}

//------------------------------------------------------------------------
void CGameObject::UpdateComponent(SEntityUpdateContext& ctx)
{
    FUNCTION_PROFILER(GetISystem(), PROFILE_ACTION);

    if (gEnv->pTimer->GetFrameStartTime() != g_lastUpdate)
    {
        g_lastUpdate = gEnv->pTimer->GetFrameStartTime();
        g_y = 10;
        g_TextModeY = 0;
    }

    CRY_ASSERT(numRemovingExtensions == NOT_IN_UPDATE_MODE);
    CRY_ASSERT(updatingEntity == 0);
    numRemovingExtensions = 0;
    m_nAddingExtension = 0;
    updatingEntity = GetEntityId();

    m_distanceChecker.Update(*this, ctx.fFrameTime);

    m_updateTimer -= ctx.fFrameTime;
    if (m_updateTimer < 0.0f)
    {
        // be pretty sure that we won't trigger twice...
        m_updateTimer = UPDATE_TIMEOUT_HUGE;
        UpdateStateEvent(eUSE_Timeout);
    }

    if (g_showUpdateState)
    {
        DebugUpdateState();
    }

    /*
     * UPDATE EXTENSIONS
     */
#ifdef _DEBUG
    IGameObjectSystem* pGameObjectSystem = m_pGOS;
#endif
    bool shouldUpdateAI = ShouldUpdateAI();
    bool keepUpdating = shouldUpdateAI;
    bool checkAIDisableOnSlots = !shouldUpdateAI && GetEntity()->GetAI();
    for (TExtensions::iterator iter = m_extensions.begin(); iter != m_extensions.end(); ++iter)
    {
#ifdef _DEBUG
        const char* name = pGameObjectSystem->GetName(iter->id);
#endif
        uint32 slotbit = 1;
        for (uint32 i = 0; i < MAX_UPDATE_SLOTS_PER_EXTENSION; ++i)
        {
            iter->forceEnables[i] -= iter->forceEnables[i] != 0;
            if (ShouldUpdateSlot(&*iter, i, slotbit, checkAIDisableOnSlots))
            {
                iter->pExtension->Update(ctx, i);
                keepUpdating = true;
            }

            slotbit <<= 1;
        }
    }

    if (!keepUpdating && m_forceUpdate <= 0)
    {
        SetActivation(false);
    }

    /*
     * REMOVE EXTENSIONS
     */
    while (numRemovingExtensions)
    {
        for (TExtensions::iterator iter = m_extensions.begin(); iter != m_extensions.end(); ++iter)
        {
            if (iter->id == removingExtensions[numRemovingExtensions - 1])
            {
                RemoveExtension(iter);
                break;
            }
        }

        numRemovingExtensions--;
    }
    /*
     * ADD EXTENSIONS
     */
    for (int i = 0; i < m_nAddingExtension; i++)
    {
        m_extensions.push_back(m_addingExtensions[i]);
    }
    if (0 < m_nAddingExtension)
    {
        std::sort(m_extensions.begin(), m_extensions.end());
        ClearCache();
    }
    for (int i = 0; i < m_nAddingExtension; i++)
    {
        m_addingExtensions[i].pExtension->PostInit(this);
        m_addingExtensions[i] = SExtension();
    }
    m_nAddingExtension = 0;

    CRY_ASSERT(numRemovingExtensions != NOT_IN_UPDATE_MODE);
    CRY_ASSERT(updatingEntity == GetEntityId());
    numRemovingExtensions = NOT_IN_UPDATE_MODE;
    updatingEntity = 0;
}

void CGameObject::UpdateSchedulingProfile()
{
}

void CGameObject::UpdateSchedulingProfiles()
{
}

void CGameObject::ForceUpdateExtension(IGameObjectExtension* pExt, int slot)
{
    SExtension* pInfo = GetExtensionInfo(pExt);
    if (pInfo)
    {
        pInfo->forceEnables[slot] = 255;
        SetActivation(true);
    }
}

//------------------------------------------------------------------------
void CGameObject::ProcessEvent(SEntityEvent& event)
{
    FUNCTION_PROFILER(GetISystem(), PROFILE_ACTION);

    if (m_pEntity)
    {
        switch (event.event)
        {
        case ENTITY_EVENT_RENDER:
        {
            const SRendParams* rParams = reinterpret_cast<const SRendParams*>(event.nParam[0]);
            m_updateState = (rParams->fDistance < FAR_AWAY_DISTANCE) ? eUS_CheckVisibility_Close : eUS_CheckVisibility_FarAway;

            UpdateStateEvent(eUSE_BecomeVisible);
        }
        break;

        case ENTITY_EVENT_INIT:
            if (m_bNeedsNetworkRebind)
            {
                m_bNeedsNetworkRebind = false;
                BindToNetwork(eBTNM_Force);
            }
            break;
        case ENTITY_EVENT_ENTERAREA:
        {
            const EntityId distanceCheckerTrigger = m_distanceChecker.GetDistanceChecker();
            if (distanceCheckerTrigger && event.nParam[2] == distanceCheckerTrigger)
            {
                if (IEntity* pEntity = gEnv->pEntitySystem->GetEntity((EntityId)event.nParam[0]))
                {
                    if (CGameObjectPtr pObj = pEntity->GetComponent<CGameObject>())
                    {
                        if (!pObj->IsInRange())
                        {
                            pObj->SetInRange(true);
                            pObj->UpdateStateEvent(eUSE_BecomeClose);
                        }
                    }
                }
            }
        }
        break;

        case ENTITY_EVENT_LEAVEAREA:
        {
            const EntityId distanceCheckerTrigger = m_distanceChecker.GetDistanceChecker();
            if (distanceCheckerTrigger && event.nParam[2] == distanceCheckerTrigger)
            {
                if (IEntity* pEntity = gEnv->pEntitySystem->GetEntity((EntityId)event.nParam[0]))
                {
                    if (CGameObjectPtr pObj = pEntity->GetComponent<CGameObject>())
                    {
                        if (pObj->IsInRange())
                        {
                            pObj->SetInRange(false);
                            if (!pObj->IsInRange())
                            {
                                pObj->UpdateStateEvent(eUSE_BecomeFarAway);
                            }
                        }
                    }
                }
            }
        }
        break;

        case ENTITY_EVENT_POST_SERIALIZE:
            PostSerialize();
            break;

        case ENTITY_EVENT_DONE:
            break;
        case ENTITY_EVENT_HIDE:
        case ENTITY_EVENT_UNHIDE:
            EvaluateUpdateActivation();
            break;

        case ENTITY_EVENT_PREPHYSICSUPDATE:
        {
            if (event.fParam[0] > 0.0f)
            {
                for (TExtensions::iterator iter = m_extensions.begin(); iter != m_extensions.end(); ++iter)
                {
                    if (iter->preUpdate)
                    {
                        iter->pExtension->PreUpdate(event.fParam[0]);
                    }
                }
            }
        }
        break;
        }

        if (IAIObject* aiObject = m_pEntity->GetAI())
        {
            aiObject->EntityEvent(event);
        }
    }
}

//------------------------------------------------------------------------
IGameObjectExtension* CGameObject::GetExtensionWithRMIBase(const void* pBase)
{
    for (TExtensions::iterator iter = m_extensions.begin(); iter != m_extensions.end(); ++iter)
    {
        if (iter->pExtension->GetRMIBase() == pBase)
        {
            return iter->pExtension.get();
        }
    }
    return NULL;
}

//------------------------------------------------------------------------
void CGameObject::SerializeXML(XmlNodeRef& entityNode, bool loading)
{
}

//------------------------------------------------------------------------
bool CGameObject::CaptureActions(IActionListener* pAL)
{
    if (m_pActionDelegate || !pAL)
    {
        return false;
    }
    m_pActionDelegate = pAL;
    return true;
}

bool CGameObject::CaptureView(IGameObjectView* pGOV)
{
    if (m_pViewDelegate || !pGOV)
    {
        return false;
    }
    m_pViewDelegate = pGOV;
    return true;
}

bool CGameObject::CaptureProfileManager(IGameObjectProfileManager* pPM)
{
    if (m_pProfileManager || !pPM)
    {
        return false;
    }
    m_pProfileManager = pPM;
    return true;
}

void CGameObject::ReleaseActions(IActionListener* pAL)
{
    if (m_pActionDelegate != pAL)
    {
        return;
    }
    m_pActionDelegate = 0;
}

void CGameObject::ReleaseView(IGameObjectView* pGOV)
{
    if (m_pViewDelegate != pGOV)
    {
        return;
    }
    m_pViewDelegate = 0;
}

void CGameObject::ReleaseProfileManager(IGameObjectProfileManager* pPM)
{
    if (m_pProfileManager != pPM)
    {
        return;
    }
    m_pProfileManager = 0;
}

//------------------------------------------------------------------------
void CGameObject::UpdateView(SViewParams& viewParams)
{
    if (m_pViewDelegate)
    {
        m_pViewDelegate->UpdateView(viewParams);
    }
}

//------------------------------------------------------------------------
void CGameObject::PostUpdateView(SViewParams& viewParams)
{
    if (m_pViewDelegate)
    {
        m_pViewDelegate->PostUpdateView(viewParams);
    }
}

//------------------------------------------------------------------------
void CGameObject::OnAction(const ActionId& actionId, int activationMode, float value)
{
    if (m_pActionDelegate)
    {
        m_pActionDelegate->OnAction(actionId, activationMode, value);
    }
    //  else
    //      GameWarning("Action sent to an entity that doesn't implement action-listener; entity class is %s, and action is %s",GetEntity()->GetClass()->GetName(), actionId.c_str());
}

void CGameObject::AfterAction()
{
    if (m_pActionDelegate)
    {
        m_pActionDelegate->AfterAction();
    }
}

//////////////////////////////////////////////////////////////////////////
bool CGameObject::NeedSerialize()
{
    return true;
}

//////////////////////////////////////////////////////////////////////////
bool CGameObject::GetSignature(TSerialize signature)
{
    FUNCTION_PROFILER(GetISystem(), PROFILE_ACTION);

    bool bResult = true;

    signature.BeginGroup("GOExtensions");

    TExtensions::iterator itExtension = m_extensions.begin();
    TExtensions::iterator itExtensionEnd = m_extensions.end();
    for (; bResult && itExtension != itExtensionEnd; ++itExtension)
    {
        struct SExtension& extension = *itExtension;

        // Let extension add to signature
        bResult &= (!extension.pExtension || extension.pExtension->GetEntityPoolSignature(signature));
    }

    signature.EndGroup();

    return bResult;
}

static const char* AspectProfileSerializationName(int i)
{
    static char buffer[11] = {0};

    if (!buffer[0])
    {
        buffer[0] = 'a';
        buffer[1] = 'p';
        buffer[2] = 'r';
        buffer[3] = 'o';
        buffer[4] = 'f';
        buffer[5] = 'i';
        buffer[6] = 'l';
        buffer[7] = 'e';
    }

    assert(i >= 0 && i < 256);
    i = clamp_tpl<int>(i, 0, 255);
    buffer[8] = 0;
    buffer[9] = 0;
    buffer[10] = 0;
    azitoa(i, &(buffer[8]), AZ_ARRAY_SIZE(buffer) - 8, 10);

    return buffer;
}

//------------------------------------------------------------------------
void CGameObject::Serialize(TSerialize ser)
{
    FullSerialize(ser);
}

struct SExtensionSerInfo
{
    uint32 extensionLocalIndex;
    uint32 priority;
    static bool SortingComparison(const SExtensionSerInfo& s0, const SExtensionSerInfo& s1)
    {
        return s0.priority < s1.priority;
    }
};

void CGameObject::FullSerialize(TSerialize ser)
{
    CRY_ASSERT(ser.GetSerializationTarget() != eST_Network);

    if (ser.BeginOptionalGroup("GameObject", true))
    {
        int updateState = eUS_INVALID;
        if (ser.IsWriting())
        {
            updateState = (int) m_updateState;
        }
        ser.Value("updateState", updateState);
        if (ser.IsReading())
        {
            m_updateState = (EUpdateState) updateState;
        }

        const uint8 profileDefault = 255;
        CryFixedStringT<32> name;
        IGameObjectSystem* pGameObjectSystem = m_pGOS;
        std::vector< CryFixedStringT<32> > activatedExtensions;


        int extensionsCount = 0;
        if (ser.IsWriting())
        {
            for (TExtensions::iterator iter = m_extensions.begin(); iter != m_extensions.end(); ++iter)
            {
                if (iter->activated)
                {
                    activatedExtensions.push_back(pGameObjectSystem->GetName(iter->id));
                }
            }
            if (!activatedExtensions.empty())
            {
                ser.Value("activatedExtensions", activatedExtensions);
            }

            extensionsCount = m_extensions.size();
            ser.Value("numExtensions", extensionsCount);

            if (extensionsCount > 1) // when we have many extensions, sort by serialization priority
            {
                typedef std::vector<SExtensionSerInfo> TSortingList;
                TSortingList sortingExtensionsList;

                sortingExtensionsList.resize(m_extensions.size());
                {
                    TExtensions::iterator iterS = m_extensions.begin();
                    TSortingList::iterator iterD = sortingExtensionsList.begin();
                    for (uint32 i = 0; iterS != m_extensions.end(); ++iterS, ++iterD, ++i)
                    {
                        iterD->extensionLocalIndex = i;
                        iterD->priority = pGameObjectSystem->GetExtensionSerializationPriority(iterS->id);
                    }
                    std::sort(sortingExtensionsList.begin(), sortingExtensionsList.end(), &SExtensionSerInfo::SortingComparison);
                }

                // we do this to make sure that the extensions will be in the same update order after reading from savegame. We dont want to change the update order for now, to avoid possible bugs.
                ser.BeginGroup("UpdateOrder");
                for (TExtensions::iterator iter = m_extensions.begin(); iter != m_extensions.end(); ++iter)
                {
                    ser.BeginGroup("Extension");
                    name = pGameObjectSystem->GetName(iter->id);
                    ser.Value("name", name);
                    ser.EndGroup();
                }
                ser.EndGroup();

                // actual serialization call
                for (TSortingList::iterator iter = sortingExtensionsList.begin(); iter != sortingExtensionsList.end(); ++iter)
                {
                    SExtension& extension = m_extensions[ iter->extensionLocalIndex ];
                    ser.BeginGroup("Extension");
                    name = pGameObjectSystem->GetName(extension.id);
                    ser.Value("name", name);
                    extension.pExtension->FullSerialize(ser);
                    ser.EndGroup();
                }
            }
            else
            {
                if (extensionsCount == 1)
                {
                    ser.BeginGroup("Extension");
                    name = pGameObjectSystem->GetName(m_extensions[0].id);
                    ser.Value("name", name);
                    m_extensions[0].pExtension->FullSerialize(ser);
                    ser.EndGroup();
                }
            }

            uint8 profile = profileDefault;
            if (m_pProfileManager)
            {
                for (NetworkAspectID i = 0; i < NUM_ASPECTS; i++)
                {
                    NetworkAspectType aspect = 1 << i;
                    profile = GetAspectProfile((EEntityAspects)aspect);

                    ser.ValueWithDefault(AspectProfileSerializationName(i), profile, profileDefault);
                }
            }
        }
        else //reading
        {
            //          FlushExtensions(false);
            ser.Value("activatedExtensions", activatedExtensions);
            for (std::vector< CryFixedStringT<32> >::iterator iter = activatedExtensions.begin(); iter != activatedExtensions.end(); ++iter)
            {
                ChangeExtension(iter->c_str(), eCE_Activate);
            }

            ser.Value("numExtensions", extensionsCount);

            // we acquire them in the original order before the actual serialization to make sure that the update order will be the same than before saving
            if (extensionsCount > 1)
            {
                uint32 tempCount = extensionsCount;
                ser.BeginGroup("UpdateOrder");
                while (tempCount--)
                {
                    ser.BeginGroup("Extension");
                    ser.Value("name", name);
                    AcquireExtension(name.c_str());
                    ReleaseExtension(name.c_str());
                    ser.EndGroup();
                }
                ser.EndGroup();
            }

            while (extensionsCount--)
            {
                ser.BeginGroup("Extension");
                ser.Value("name", name);
                IGameObjectExtension* pExt = AcquireExtension(name.c_str());
                if (pExt)
                {
                    pExt->FullSerialize(ser);
                    ReleaseExtension(name.c_str());
                }
                ser.EndGroup();
            }

            //physicalize after serialization with the correct parameters
            if (m_pProfileManager)
            {
                for (NetworkAspectID i = 0; i < NUM_ASPECTS; i++)
                {
                    NetworkAspectType aspect = 1 << i;

                    uint8 profile = 0;
                    ser.ValueWithDefault(AspectProfileSerializationName(i), profile, profileDefault);
                    SetAspectProfile((EEntityAspects)aspect, profile, false);
                }
            }
        }

        ser.EndGroup(); //GameObject
    }
}

//------------------------------------------------------------------------
bool CGameObject::NetSerialize(TSerialize ser, EEntityAspects aspect, uint8 profile, int flags)
{
    for (TExtensions::iterator it = m_extensions.begin(); it != m_extensions.end(); ++it)
    {
        if (!it->pExtension->NetSerialize(ser, aspect, profile, flags))
        {
            return false;
        }
    }

    if (aspect == eEA_Physics && !m_pProfileManager)
    {
        if (IComponentSerializationPtr serializationComponent = GetEntity()->GetComponent<IComponentSerialization>())
        {
            serializationComponent->SerializeOnly(ser, { IComponentPhysics::Type() });
        }
        else
        {
            return false;
        }
    }

    if (aspect == eEA_Script)
    {
        CCryAction::GetCryAction()->SerializeScript(ser, GetEntity());
    }

    return true;
}

NetworkAspectType CGameObject::GetNetSerializeAspects()
{
    NetworkAspectType aspects = 0;

    for (TExtensions::iterator iter = m_extensions.begin(); iter != m_extensions.end(); ++iter)
    {
        aspects |= iter->pExtension->GetNetSerializeAspects();
    }

    if (!m_bNoSyncPhysics && (GetEntity()->GetComponent<IComponentPhysics>() || m_pProfileManager))
    {
        aspects |= eEA_Physics;
    }

    if (GetEntity()->GetComponent<IComponentScript>())
    {
        aspects |= eEA_Script;
    }

    return aspects;
}

//------------------------------------------------------------------------
void CGameObject::PostSerialize()
{
    for (TExtensions::iterator iter = m_extensions.begin(); iter != m_extensions.end(); ++iter)
    {
        if (iter->pExtension)
        {
            iter->pExtension->PostSerialize();
        }
    }
    m_distanceChecker.Reset();

    if (m_updateState == eUS_NotVisible_FarAway)
    {
        GetEntity()->SetFlags(GetEntity()->GetFlags() | ENTITY_FLAG_SEND_RENDER_EVENT);
    }
}

void CGameObject::SetChannelId(ChannelId id)
{
    m_channelId = id;

    for (TExtensions::iterator iter = m_extensions.begin(); iter != m_extensions.end(); ++iter)
    {
        if (iter->pExtension)
        {
            iter->pExtension->SetChannelId(id);
        }
    }
}

//------------------------------------------------------------------------
void CGameObject::InitClient(ChannelId channelId)
{
    for (TExtensions::iterator iter = m_extensions.begin(); iter != m_extensions.end(); ++iter)
    {
        iter->pExtension->InitClient(channelId);
    }
}

//------------------------------------------------------------------------
void CGameObject::PostInitClient(ChannelId channelId)
{
    for (TExtensions::iterator iter = m_extensions.begin(); iter != m_extensions.end(); ++iter)
    {
        iter->pExtension->PostInitClient(channelId);
    }
}

//------------------------------------------------------------------------
void CGameObject::SetAuthority(bool auth)
{
    for (TExtensions::iterator iter = m_extensions.begin(); iter != m_extensions.end(); ++iter)
    {
        iter->pExtension->SetAuthority(auth);
    }
}

//------------------------------------------------------------------------
void CGameObject::EnableAspect(NetworkAspectType aspects, bool enable)
{
    (void)aspects;
    (void)enable;
}

//------------------------------------------------------------------------
void CGameObject::ChangedNetworkState(NetworkAspectType aspects)
{
    if (gEnv && gEnv->pNetwork)
    {
        gEnv->pNetwork->ChangedAspects(GetEntityId(), aspects);
    }
}

//------------------------------------------------------------------------
void CGameObject::EnableDelegatableAspect(NetworkAspectType aspects, bool enable)
{
    if (gEnv->pNetwork)
    {
        gEnv->pNetwork->SetObjectDelegatedAspectMask(GetEntityId(), aspects, enable);
    }
}

uint8 CGameObject::GetAspectProfile(EEntityAspects aspect)
{
    return m_profiles[BitIndex(static_cast<NetworkAspectType>(aspect))];
}

bool CGameObject::SetAspectProfile(EEntityAspects aspect, uint8 profile, bool fromNetwork)
{
    CRY_ASSERT_MESSAGE(BitIndex(static_cast<NetworkAspectType>(aspect)) < NUM_ASPECTS, "Invalid aspet index!");

    (void)fromNetwork;
    CRY_ASSERT_MESSAGE(fromNetwork || gEnv->bServer || !gEnv->bMultiplayer, "Aspect profiles can only be set by the server!");

    bool ok = true;
    if (m_pProfileManager)
    {
        ok = m_pProfileManager->SetAspectProfile(aspect, profile);
    }
    if (ok)
    {
        m_profiles[BitIndex(static_cast<NetworkAspectType>(aspect))] = profile;
    }

    return ok;
}

void CGameObject::SetNetworkParent(EntityId id)
{
}

void CGameObject::Pulse(uint32 pulse)
{
}

void CGameObject::RegisterAsPredicted()
{
}

int CGameObject::GetPredictionHandle()
{
    return 0;
}

void CGameObject::RegisterAsValidated(IGameObject* pGO, int predictionHandle)
{
}

void CGameObject::RequestRemoteUpdate(NetworkAspectType aspectMask)
{
}

//------------------------------------------------------------------------
template <class T>
ILINE bool CGameObject::DoGetSetExtensionParams(const char* extension, SmartScriptTable params)
{
    IGameObjectSystem* pGameObjectSystem = m_pGOS;
    SExtension ext;
    ext.id = pGameObjectSystem->GetID(extension);
    if (ext.id != IGameObjectSystem::InvalidExtensionID)
    {
        TExtensions::iterator iter = std::lower_bound(m_extensions.begin(), m_extensions.end(), ext);

        if (iter != m_extensions.end() && ext.id == iter->id)
        {
            T impl(params);
            CSimpleSerialize<T> ser(impl);
            iter->pExtension->FullSerialize(TSerialize(&ser));
            return ser.Ok();
        }
    }
    return false;
}

IGameObjectExtension* CGameObject::QueryExtension(IGameObjectSystem::ExtensionID id) const
{
    FUNCTION_PROFILER(GetISystem(), PROFILE_ACTION);

    SExtension ext;
    ext.id = id;

    if (m_pEntity && (ext.id != IGameObjectSystem::InvalidExtensionID))
    {
        // locate the extension
        TExtensions::const_iterator iter = std::lower_bound(m_extensions.begin(), m_extensions.end(), ext);
        if (iter != m_extensions.end() && iter->id == ext.id)
        {
            return iter->pExtension.get();
        }
    }

    return 0;
}

IGameObjectExtension* CGameObject::QueryExtension(const char* extension) const
{
    FUNCTION_PROFILER(GetISystem(), PROFILE_ACTION);
    return QueryExtension(m_pGOS->GetID(extension));
}

bool CGameObject::GetExtensionParams(const char* extension, SmartScriptTable params)
{
    return DoGetSetExtensionParams<CSerializeScriptTableWriterImpl>(extension, params);
}

bool CGameObject::SetExtensionParams(const char* extension, SmartScriptTable params)
{
    return DoGetSetExtensionParams<CSerializeScriptTableReaderImpl>(extension, params);
}

//------------------------------------------------------------------------
IGameObjectExtension* CGameObject::ChangeExtension(const char* name, EChangeExtension change)
{
    IGameObjectExtension* pRet = NULL;

    IGameObjectSystem* pGameObjectSystem = m_pGOS;
    SExtension ext;
    // fetch extension id
    ext.id = pGameObjectSystem->GetID(name);

    if (m_pEntity) // init has been called, things are safe
    {
        m_justExchanging = true;
        if (ext.id != IGameObjectSystem::InvalidExtensionID)
        {
            // locate the extension
            TExtensions::iterator iter = std::lower_bound(m_extensions.begin(), m_extensions.end(), ext);

            // we're adding an extension
            if (change == eCE_Activate || change == eCE_Acquire)
            {
                if (iter != m_extensions.end() && iter->id == ext.id)
                {
                    iter->refCount += (change == eCE_Acquire);
                    iter->activated |= (change == eCE_Activate);
                    pRet = iter->pExtension.get();
                }
                else
                {
                    ext.refCount += (change == eCE_Acquire);
                    ext.activated |= (change == eCE_Activate);
                    ext.pExtension = pGameObjectSystem->Instantiate(ext.id, this);
                    if (ext.pExtension)
                    {
                        pRet = ext.pExtension.get();
                        if (updatingEntity == GetEntityId())
                        {
                            CRY_ASSERT(m_nAddingExtension < MAX_ADDING_EXTENSIONS);
                            PREFAST_ASSUME(m_nAddingExtension < MAX_ADDING_EXTENSIONS);
                            if (m_nAddingExtension == MAX_ADDING_EXTENSIONS)
                            {
                                CryFatalError("Too many extensions added in a single frame");
                            }
                            m_addingExtensions[m_nAddingExtension++] = ext;
                        }
                        else
                        {
                            ClearCache();
                            m_extensions.push_back(ext);
                            std::sort(m_extensions.begin(), m_extensions.end());
                            ext.pExtension->PostInit(this);
                        }
                    }
                }
            }
            else // change == eCE_Deactivate || change == eCE_Release
            {
                if (iter == m_extensions.end() || iter->id != ext.id)
                {
                    if (change == eCE_Release)
                    {
                        CRY_ASSERT(0);
                        CryFatalError("Attempt to release a non-existant extension %s", name);
                    }
                }
                else
                {
                    iter->refCount -= (change == eCE_Release);
                    if (change == eCE_Deactivate)
                    {
                        iter->activated = false;
                    }

                    if (!iter->refCount && !iter->activated && !iter->sticky)
                    {
                        if (updatingEntity == GetEntityId() && numRemovingExtensions != NOT_IN_UPDATE_MODE)
                        {
                            if (numRemovingExtensions == MAX_REMOVING_EXTENSIONS)
                            {
                                CryFatalError("Trying to removing too many game object extensions in one frame");
                            }
                            PREFAST_ASSUME(numRemovingExtensions < MAX_REMOVING_EXTENSIONS);
                            removingExtensions[numRemovingExtensions++] = iter->id;
                        }
                        else
                        {
                            RemoveExtension(iter);
                        }
                    }
                }
            }
        }
        m_justExchanging = false;
    }
    else if (ext.id != IGameObjectSystem::InvalidExtensionID) // init has not been called... we simply make a temporary extensions array, and fill in the details later
    {
        // only safe to Activate at this stage
        CRY_ASSERT(change == eCE_Activate);
        ClearCache();
        m_extensions.push_back(ext);
        std::sort(m_extensions.begin(), m_extensions.end());
        pRet = static_cast<IGameObjectExtension*>(reinterpret_cast<void*>(0xf0f0f0f0f0f0f0f0));
    }

    return pRet;
}

void CGameObject::RemoveExtension(const TExtensions::iterator& iter)
{
    ClearCache();
    if (iter->preUpdate)
    {
        DisablePreUpdates(iter->pExtension.get());
    }
    if (iter->postUpdate)
    {
        DisablePostUpdates(iter->pExtension.get());
    }
    IGameObjectExtensionPtr pExtension = iter->pExtension;
    std::swap(m_extensions.back(), *iter);
    m_extensions.pop_back();
    std::sort(m_extensions.begin(), m_extensions.end());
    GetEntity()->DeregisterComponent(pExtension);
    pExtension.reset();
}

//------------------------------------------------------------------------
void CGameObject::FlushExtensions(bool includeStickyBits)
{
    ClearCache();
    static bool inFlush = false;

    std::vector<IGameObjectSystem::ExtensionID> activatedExtensions_recurse;
    std::vector<IGameObjectSystem::ExtensionID>* pActivatedExtensions;

    bool wasInFlush = inFlush;
    if (inFlush)
    {
        pActivatedExtensions = &activatedExtensions_recurse;
    }
    else
    {
        pActivatedExtensions = m_pGOS->GetActivatedExtensionsTop();
        inFlush = true;
    }

    CRY_ASSERT(pActivatedExtensions->empty());
    for (TExtensions::iterator iter = m_extensions.begin(); iter != m_extensions.end(); ++iter)
    {
        if (includeStickyBits && iter->sticky)
        {
            iter->sticky = false;
            iter->activated = true;
        }
        if (iter->activated)
        {
            pActivatedExtensions->push_back(iter->id);
        }
    }
    while (!pActivatedExtensions->empty())
    {
        ChangeExtension(m_pGOS->GetName(pActivatedExtensions->back()), eCE_Deactivate);
        pActivatedExtensions->pop_back();
    }
    if (includeStickyBits)
    {
        //CRY_ASSERT( m_extensions.empty() );

        // [10/12/2009 evgeny] The only way to ensure that "0 == m_pProfileManager" in all cases
        // is to release the profile manager here explicitly.
        // It's basically just zeroing the pointer, so not memory leak is expected.
        ReleaseProfileManager(m_pProfileManager);
        CRY_ASSERT(0 == m_pProfileManager);

        CRY_ASSERT(0 == m_pActionDelegate);
        CRY_ASSERT(0 == m_pViewDelegate);
    }

    inFlush = wasInFlush;
}

uint8 CGameObject::GetUpdateSlotEnables(IGameObjectExtension* pExtension, int slot)
{
    SExtension* pExt = GetExtensionInfo(pExtension);
    if (pExt)
    {
        return pExt->updateEnables[slot];
    }
    return 0;
}

void CGameObject::EnableUpdateSlot(IGameObjectExtension* pExtension, int slot)
{
    SExtension* pExt = GetExtensionInfo(pExtension);
    if (pExt)
    {
        CRY_ASSERT_TRACE(255 != pExt->updateEnables[slot], ("Already got 255 reasons for slot %d of '%s' to be enabled", slot, GetEntity()->GetEntityTextDescription()));
        ++pExt->updateEnables[slot];
    }
    EvaluateUpdateActivation();
}

void CGameObject::DisableUpdateSlot(IGameObjectExtension* pExtension, int slot)
{
    SExtension* pExt = GetExtensionInfo(pExtension);
    if (pExt)
    {
        if (pExt->updateEnables[slot])
        {
            --pExt->updateEnables[slot];
        }
    }
    EvaluateUpdateActivation();
}

void CGameObject::EnablePreUpdates(IGameObjectExtension* pExtension)
{
    SExtension* pExt = GetExtensionInfo(pExtension);
    if (pExt)
    {
        pExt->preUpdate = true;
        EnablePrePhysicsUpdate(ePPU_Always);
        RegisterEvent(ENTITY_EVENT_PREPHYSICSUPDATE);
    }
}

void CGameObject::DisablePreUpdates(IGameObjectExtension* pExtension)
{
    bool hasPreUpdates = false;
    for (TExtensions::iterator iter = m_extensions.begin(); iter != m_extensions.end(); ++iter)
    {
        if (iter->pExtension.get() == pExtension)
        {
            iter->preUpdate = false;
        }
        hasPreUpdates |= iter->preUpdate;
    }

    if (!hasPreUpdates)
    {
        EnablePrePhysicsUpdate(ePPU_Never);
    }
}

void CGameObject::EnablePostUpdates(IGameObjectExtension* pExtension)
{
    SExtension* pExt = GetExtensionInfo(pExtension);
    if (pExt)
    {
        pExt->postUpdate = true;
        m_pGOS->SetPostUpdate(this, true);
    }
}

void CGameObject::DisablePostUpdates(IGameObjectExtension* pExtension)
{
    bool hasPostUpdates = false;
    for (TExtensions::iterator iter = m_extensions.begin(); iter != m_extensions.end(); ++iter)
    {
        if (iter->pExtension.get() == pExtension)
        {
            iter->postUpdate = false;
        }
        hasPostUpdates |= iter->postUpdate;
    }
    m_pGOS->SetPostUpdate(this, hasPostUpdates);
}

void CGameObject::HandleEvent(const SGameObjectEvent& evt)
{
    switch (evt.event)
    {
    case eGFE_EnablePhysics:
        break;
    case eGFE_DisablePhysics:
    {
        if (IPhysicalEntity* pEnt = GetEntity()->GetPhysics())
        {
            pe_action_awake awake;
            awake.bAwake = false;
            pEnt->Action(&awake);
        }
    }
    break;
    }
}

//------------------------------------------------------------------------
void CGameObject::SendEvent(const SGameObjectEvent& event)
{
    if (event.flags & eGOEF_ToGameObject)
    {
        CGameObject::HandleEvent(event);
    }
    if (event.flags & eGOEF_ToScriptSystem)
    {
        SmartScriptTable scriptTable = GetEntity()->GetScriptTable();
        if (event.event != IGameObjectSystem::InvalidEventID)
        {
            const char* eventName = m_pGOS->GetEventName(event.event);
            if (!!scriptTable && eventName && scriptTable->HaveValue(eventName))
            {
                Script::CallMethod(scriptTable, eventName);
            }
        }
    }
    if (event.flags & eGOEF_ToExtensions)
    {
        if (event.target == IGameObjectSystem::InvalidExtensionID)
        {
            if (CCryActionCVars::Get().g_handleEvents == 1)
            {
                const uint64 eventBit = 1ULL << (event.event);
                for (TExtensions::iterator iter = m_extensions.begin(); iter != m_extensions.end(); ++iter)
                {
                    const bool bHandleEvent = ((iter->eventReg & eventBit) != 0);
                    if (bHandleEvent)
                    {
                        iter->pExtension->HandleEvent(event);
                    }
                }
            }
            else
            {
                for (TExtensions::iterator iter = m_extensions.begin(); iter != m_extensions.end(); ++iter)
                {
                    iter->pExtension->HandleEvent(event);
                }
            }
        }
        else
        {
            SExtension ext;
            ext.id = event.target;
            TExtensions::iterator iter = std::lower_bound(m_extensions.begin(), m_extensions.end(), ext);
            if (iter != m_extensions.end())
            {
                iter->pExtension->HandleEvent(event);
            }
        }
    }
}

IGameObjectExtension* CGameObject::GetExtensionWithRMIRep(uint32 repId)
{
    for (TExtensions::iterator iter = m_extensions.begin(); iter != m_extensions.end(); ++iter)
    {
        if (iter->pExtension->HasRMIRep(repId))
        {
            return iter->pExtension.get();
        }
    }

    return nullptr;
}

void CGameObject::PostUpdate(float frameTime)
{
    for (TExtensions::iterator iter = m_extensions.begin(); iter != m_extensions.end(); ++iter)
    {
        if (iter->postUpdate)
        {
            iter->pExtension->PostUpdate(frameTime);
        }
    }
}

void CGameObject::SetUpdateSlotEnableCondition(IGameObjectExtension* pExtension, int slot, EUpdateEnableCondition condition)
{
    bool whenVisible = false;
    bool whenInRange = false;
    bool combineOr = false;
    bool disableWithAI = true;
    bool neverUpdate = false;

    switch (condition)
    {
    case eUEC_Never:
        neverUpdate = true;
        break;
    case eUEC_Always:
        whenVisible = false;
        whenInRange = false;
        combineOr = false;
        break;
    case eUEC_Visible:
        whenVisible = true;
        whenInRange = false;
        combineOr = false;
        break;
    case eUEC_VisibleIgnoreAI:
        whenVisible = true;
        whenInRange = false;
        combineOr = false;
        disableWithAI = false;
        break;
    case eUEC_InRange:
        whenVisible = false;
        whenInRange = true;
        combineOr = false;
        break;
    case eUEC_VisibleAndInRange:
        whenVisible = true;
        whenInRange = true;
        combineOr = false;
        break;
    case eUEC_VisibleOrInRange:
        whenVisible = true;
        whenInRange = true;
        combineOr = true;
        break;
    case eUEC_VisibleOrInRangeIgnoreAI:
        whenVisible = true;
        whenInRange = true;
        combineOr = true;
        disableWithAI = false;
        break;
    case eUEC_WithoutAI:
        disableWithAI = false;
        break;
    }

    const uint slotbit = 1 << slot;

    SExtension* pExt = GetExtensionInfo(pExtension);
    if (pExt)
    {
        SET_FLAG_FOR_SLOT(pExt->flagUpdateWhenVisible, slotbit, whenVisible);
        SET_FLAG_FOR_SLOT(pExt->flagUpdateWhenInRange, slotbit, whenInRange);
        SET_FLAG_FOR_SLOT(pExt->flagUpdateCombineOr, slotbit, combineOr);
        SET_FLAG_FOR_SLOT(pExt->flagDisableWithAI, slotbit, disableWithAI);
        SET_FLAG_FOR_SLOT(pExt->flagNeverUpdate, slotbit, neverUpdate);
    }

    EvaluateUpdateActivation();
}


bool CGameObject::ShouldUpdate()
{
    // evaluate main-loop activation
    bool shouldUpdateAI(!GetEntity()->IsHidden() && (IsProbablyVisible() || !IsProbablyDistant()));
    bool shouldBeActivated = shouldUpdateAI;
    bool hasAI = NULL != GetEntity()->GetAI();
    bool checkAIDisableOnSlots = !shouldUpdateAI && hasAI;
    for (TExtensions::iterator iter = m_extensions.begin(); iter != m_extensions.end() && !shouldBeActivated; ++iter)
    {
        uint32 slotbit = 1;
        for (int slot = 0; slot < 4 && !shouldBeActivated; slot++)
        {
            shouldBeActivated |= ShouldUpdateSlot(&*iter, slot, slotbit, checkAIDisableOnSlots);
            slotbit <<= 1;
        }
    }
    return shouldBeActivated;
}

void CGameObject::EvaluateUpdateActivation()
{
    // evaluate main-loop activation
    bool shouldUpdateAI = ShouldUpdateAI();
    bool shouldBeActivated = shouldUpdateAI;
    bool hasAI = NULL != GetEntity()->GetAI();
    bool checkAIDisableOnSlots = !shouldUpdateAI && hasAI;
    for (TExtensions::iterator iter = m_extensions.begin(); iter != m_extensions.end() && !shouldBeActivated; ++iter)
    {
        uint32 slotbit = 1;
        for (uint32 slot = 0; slot < 4 && !shouldBeActivated != 0; slot++)
        {
            shouldBeActivated |= ShouldUpdateSlot(&*iter, slot, slotbit, checkAIDisableOnSlots);
            slotbit <<= 1;
        }
    }

    shouldBeActivated |= (m_forceUpdate > 0);
    SetActivation(shouldBeActivated);

    // evaluate pre-physics activation
    bool shouldActivatePrePhysics = false;
    switch (m_prePhysicsUpdateRule)
    {
    case ePPU_WhenAIActivated:
        if (hasAI)
        {
            shouldActivatePrePhysics = ShouldUpdateAI();
        }
        else
        {
        case ePPU_Always: // yes this is what i intended...
            shouldActivatePrePhysics = true;
        }
        break;
    case ePPU_Never:
        break;
    default:
        CRY_ASSERT(false);
    }

    if (shouldActivatePrePhysics != m_bPrePhysicsEnabled)
    {
        m_pEntity->PrePhysicsActivate(shouldActivatePrePhysics);
        m_bPrePhysicsEnabled = shouldActivatePrePhysics;
    }

    if (TestIsProbablyVisible(m_updateState))
    {
        SetPhysicsDisable(false);
    }
    else
    {
        switch (m_physDisableMode)
        {
        default:
            CRY_ASSERT(false);
        case eADPM_Never:
        case eADPM_WhenAIDeactivated:
            break;
        case eADPM_WhenInvisibleAndFarAway:
            SetPhysicsDisable(!TestIsProbablyVisible(m_updateState) && TestIsProbablyDistant(m_updateState));
            break;
        }
    }
}

void CGameObject::SetActivation(bool activate)
{
    bool wasActivated = m_pEntity->IsActive();

    if (TestIsProbablyVisible(m_updateState))
    {
        SetPhysicsDisable(false);
    }
    else
    {
        switch (m_physDisableMode)
        {
        default:
            CRY_ASSERT(false);
        case eADPM_Never:
            SetPhysicsDisable(false);
        case eADPM_WhenInvisibleAndFarAway:
            break;
        case eADPM_WhenAIDeactivated:
            if (wasActivated && !activate)
            {
                SetPhysicsDisable(true);
            }
            break;
        }
    }

    if (wasActivated != activate)
    {
        m_pEntity->Activate(activate);

        if (!activate)
        {
            IAIObject* aiObject = m_pEntity->GetAI();
            IAIActorProxy* proxy = aiObject ? aiObject->GetProxy() : 0;

            if (proxy)
            {
                proxy->NotifyAutoDeactivated();
            }
        }
    }

    if (activate)
    {
        SetPhysicsDisable(false);
    }
}

void CGameObject::SetPhysicsDisable(bool disablePhysics)
{
    if (disablePhysics != m_bPhysicsDisabled)
    {
        m_bPhysicsDisabled = disablePhysics;
        //CryLogAlways("[gobj] %s physics on %s", disablePhysics? "disable" : "enable", GetEntity()->GetName());
        SGameObjectEvent goe(disablePhysics ? eGFE_DisablePhysics : eGFE_EnablePhysics, eGOEF_ToExtensions | eGOEF_ToGameObject);
        SendEvent(goe);
    }
}

/*
void CGameObject::SetVisibility( bool isVisible )
{
    if (isVisible != m_isVisible)
    {
//      CryLogAlways("%s is now %svisible", m_pEntity->GetName(), isVisible? "" : "in");

        uint32 flags = m_pEntity->GetFlags();
        if (isVisible)
            flags &= ~ENTITY_FLAG_SEND_RENDER_EVENT;
        else
            flags |= ENTITY_FLAG_SEND_RENDER_EVENT;
        m_pEntity->SetFlags(flags);

        m_isVisible = isVisible;
    }
}
*/

IWorldQuery* CGameObject::GetWorldQuery()
{
    if (IWorldQuery* query = (IWorldQuery*)QueryExtension("WorldQuery"))
    {
        return query;
    }
    else
    {
        return NULL;
    }
}

IMovementController* CGameObject::GetMovementController()
{
    IActor* pActor = CCryAction::GetCryAction()->GetIActorSystem()->GetActor(m_pEntity->GetId());
    if (pActor != NULL)
    {
        return pActor->GetMovementController();
    }
    else if (IVehicle* pVehicle = CCryAction::GetCryAction()->GetIVehicleSystem()->GetVehicle(m_pEntity->GetId()))
    {
        return pVehicle->GetMovementController();
    }
    else
    {
        return NULL;
    }
}

void CGameObject::AttachDistanceChecker()
{
    if (CCryAction::GetCryAction()->IsEditing())
    {
        return;
    }

    m_distanceChecker.Init(m_pGOS, GetEntityId());
}

void CGameObject::ForceUpdate(bool force)
{
    if (force)
    {
        ++m_forceUpdate;
    }
    else
    {
        --m_forceUpdate;
    }

    CRY_ASSERT(m_forceUpdate >= 0);

    EvaluateUpdateActivation();
}

struct SContainerSer
    : public ISerializableInfo
{
    void SerializeWith(TSerialize ser)
    {
        for (size_t i = 0; i < m_children.size(); i++)
        {
            m_children[i]->SerializeWith(ser);
        }
    }

    std::vector<ISerializableInfoPtr> m_children;
};

ISerializableInfoPtr CGameObject::GetSpawnInfo()
{
    _smart_ptr<SContainerSer> pC;

    for (TExtensions::iterator iter = m_extensions.begin(); iter != m_extensions.end(); ++iter)
    {
        if (iter->pExtension)
        {
            ISerializableInfoPtr pS = iter->pExtension->GetSpawnInfo();
            if (pS)
            {
                if (!pC)
                {
                    pC = new SContainerSer;
                }
                pC->m_children.push_back(pS);
            }
        }
    }
    return &*pC;
}

uint8 CGameObject::GetDefaultProfile(EEntityAspects aspect)
{
    if (m_pProfileManager)
    {
        return m_pProfileManager->GetDefaultProfile(aspect);
    }
    else
    {
        return 0;
    }
}

bool CGameObject::SetAIActivation(EGameObjectAIActivationMode mode)
{
    // this should be done only for gameobjects with AI. Otherwise breaks weapons (scope update, etc)
    if (!GetEntity()->HasAI())
    {
        return false;
    }

    if (m_aiMode != mode)
    {
        m_aiMode = mode;
        EvaluateUpdateActivation(); // need to recheck any updates on slots
    }

    return GetEntity()->IsActive();
}

bool CGameObject::ShouldUpdateAI()
{
    if (GetEntity()->IsHidden() || !GetEntity()->HasAI())
    {
        return false;
    }

    switch (m_aiMode)
    {
    case eGOAIAM_Never:
        return false;
    case eGOAIAM_Always:
        return true;
    case eGOAIAM_VisibleOrInRange:
        return IsProbablyVisible() || !IsProbablyDistant();
    default:
        CRY_ASSERT(false);
        return false;
    }
}

void CGameObject::EnablePrePhysicsUpdate(EPrePhysicsUpdate updateRule)
{
    m_prePhysicsUpdateRule = updateRule;
    EvaluateUpdateActivation();
}

void CGameObject::PostRemoteSpawn()
{
    for (size_t i = 0; i < m_extensions.size(); ++i)
    {
        m_extensions[i].pExtension->PostRemoteSpawn();
    }
}

void CGameObject::AcquireMutex()
{
    m_mutex.Lock();
}

void CGameObject::ReleaseMutex()
{
    m_mutex.Unlock();
}

void CGameObject::GetMemoryUsage(ICrySizer* s) const
{
    {
        SIZER_SUBCOMPONENT_NAME(s, "GameObject");
        s->AddObject(this, sizeof(*this));
    }
}

void CGameObject::SetAutoDisablePhysicsMode(EAutoDisablePhysicsMode mode)
{
    if (m_physDisableMode != mode)
    {
        m_physDisableMode = mode;
        EvaluateUpdateActivation();
    }
}
#if GAME_OBJECT_SUPPORTS_CUSTOM_USER_DATA
void* CGameObject::GetUserData() const
{
    return m_pUserData;
}

void CGameObject::SetUserData(void* ptr)
{
    m_pUserData = ptr;
}
#endif // GAME_OBJECT_SUPPORTS_CUSTOM_DATA

void CGameObject::RegisterExtForEvents(IGameObjectExtension* piExtension, const int* pEvents, const int numEvents)
{
    SExtension* pExtension = GetExtensionInfo(piExtension);
    if (pExtension)
    {
        if (pEvents)
        {
            uint64 eventReg = 0;
            for (int i = 0; i < numEvents; ++i)
            {
                const int event = pEvents[i];
                eventReg |= 1ULL << (event);
            }
            pExtension->eventReg |= eventReg;
        }
        else
        {
            pExtension->eventReg = ~0;
        }
    }
}

void CGameObject::UnRegisterExtForEvents(IGameObjectExtension* piExtension, const int* pEvents, const int numEvents)
{
    SExtension* pExtension = GetExtensionInfo(piExtension);
    if (pExtension)
    {
        if (pEvents)
        {
            uint64 eventReg = 0;
            for (int i = 0; i < numEvents; ++i)
            {
                const int event = pEvents[i];
                eventReg |= 1ULL << (event);
            }
            pExtension->eventReg &= ~eventReg;
        }
        else
        {
            pExtension->eventReg = 0;
        }
    }
}

void CGameObject::SerializeSpawnInfo(TSerialize ser, IEntity* entity)
{
    for (auto& extension : m_extensions)
    {
        if (extension.pExtension)
        {
            extension.pExtension->SerializeSpawnInfo(ser, entity);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
#if 0
// deprecated and won't compile at all...
void SDistanceChecker::Init(CGameObjectSystem* pGameObjectSystem, EntityId receiverId)
{
    IEntity* pDistanceChecker = pGameObjectSystem->CreatePlayerProximityTrigger();
    if (pDistanceChecker)
    {
        pDistanceChecker->GetComponent<IComponentTrigger>()->ForwardEventsTo(receiverId);
        m_distanceCheckerTimer = -1.0f;
        m_distanceChecker = pDistanceChecker->GetId();
        Update(0.0f);
    }
}

void SDistanceChecker::Reset()
{
    if (m_distanceChecker)
    {
        gEnv->pEntitySystem->RemoveEntity(m_distanceChecker);
        m_distanceChecker = 0;
    }
    m_distanceChecker = 0.0f;
}

void SDistanceChecker::Update(CGameObject& owner, float frameTime)
{
    if (!m_distanceChecker)
    {
        return;
    }

    bool ok = false;
    IEntity* pDistanceChecker = gEnv->pEntitySystem->GetEntity(m_distanceChecker);
    if (pDistanceChecker)
    {
        m_distanceCheckerTimer -= frameTime;
        if (m_distanceCheckerTimer < 0.0f)
        {
            IComponentTriggerPtr triggerComponent = pDistanceChecker->GetComponent<IComponentTrigger>();
            if (triggerComponent)
            {
                triggerComponent->SetTriggerBounds(CreateDistanceAABB(owner.GetEntity()->GetWorldPos()));
                m_distanceCheckerTimer = DISTANCE_CHECKER_TIMEOUT;
                ok = true;
            }
        }
        else
        {
            ok = true;
        }
    }

    if (!ok)
    {
        Reset();

        owner.AttachDistanceChecker();
    }
}

#endif
