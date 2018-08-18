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

#ifndef CRYINCLUDE_CRYACTION_MANNEQUIN_ACTIONCONTROLLER_H
#define CRYINCLUDE_CRYACTION_MANNEQUIN_ACTIONCONTROLLER_H
#pragma once

#include "ICryMannequin.h"
struct SScopeContext;
class CActionScope;

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/Entity.h>
#include <LmbrCentral/Rendering/MeshComponentBus.h>

#ifdef _RELEASE
#define CRYMANNEQUIN_WARN_ABOUT_VALIDITY() 0
#else
#define CRYMANNEQUIN_WARN_ABOUT_VALIDITY() 1
#endif

enum EFlushMethod
{
    FM_Normal,
    FM_NormalLeaveAnimations,
    FM_Failure,
};

ILINE uint32 GetLeastSignificantBit(ActionScopes scopeMask)
{
    const uint32 mask = (uint32)scopeMask;
    static const uint32 MultiplyDeBruijnBitPosition[32] =
    {
        0, 1, 28, 2, 29, 14, 24, 3, 30, 22, 20, 15, 25, 17, 4, 8,
        31, 27, 13, 23, 21, 19, 16, 7, 26, 12, 18, 6, 11, 5, 10, 9
    };
    return MultiplyDeBruijnBitPosition[((uint32)((mask & ~(mask - 1)) * 0x077CB531U)) >> 27];
}

class CActionController
    : public IActionController
    , private  LmbrCentral::SkinnedMeshInformationBus::Handler
{
public:
    typedef std::vector<CActionController*> TActionControllerList;


    CActionController(IEntity* pEntity, SAnimationContext& context);
    CActionController(const AZ::EntityId& entityId, SAnimationContext& context);

    // -- IActionController implementation --------------------------------------

    ~CActionController();


    //////////////////////////////////////////////////////////////////////////
    // SkinnedMeshInformationBus::Handler Implementation
    void OnSkinnedMeshCreated(ICharacterInstance* characterInstance, const AZ::EntityId& entityId) override;
    void OnSkinnedMeshDestroyed(const AZ::EntityId& entityId) override;
    //////////////////////////////////////////////////////////////////////////

    virtual void OnEvent(const SGameObjectEvent& event);
    virtual void OnAnimationEvent(ICharacterInstance* pCharacter, const AnimEventInstance& event);

    virtual void Reset();
    virtual void Flush();

    virtual uint32 GetTotalScopes() const
    {
        return m_scopeCount;
    }

    void SetScopeContext(uint32 scopeContextID, IEntity& entity, ICharacterInstance* character, const IAnimationDatabase* animDatabase) override;
    void SetScopeContext(uint32 scopeContextID, const AZ::EntityId& entityId, const IAnimationDatabase* animDatabase) override;

    virtual void ClearScopeContext(uint32 scopeContextID, bool flushAnimations = true);

    void ClearAllScopeContexts(bool flushAnimations = true) override;

    virtual bool IsScopeActive(uint32 scopeID) const
    {
        CRY_ASSERT_MESSAGE((scopeID < m_scopeCount), "Invalid scope id");

        return ((m_activeScopes & (1 << scopeID)) != 0);
    }
    virtual ActionScopes GetActiveScopeMask() const
    {
        return m_activeScopes;
    }

    virtual IEntity& GetEntity() const
    {
        CRY_ASSERT(m_cachedEntity);
        return *m_cachedEntity;
    }

    virtual const AZ::EntityId& GetEntityId() const
    {
        return m_entityId;
    }

    virtual IScope* GetScope(uint32 scopeID);
    virtual const IScope* GetScope(uint32 scopeID) const;

    virtual uint32 GetScopeID(const char* name) const
    {
        int scopeID = m_context.controllerDef.m_scopeIDs.Find(name);

        if (scopeID >= 0)
        {
            return scopeID;
        }
        else
        {
            return SCOPE_ID_INVALID;
        }
    }

    virtual TagID GetGlobalTagID(uint32 crc) const
    {
        return GetContext().state.GetDef().Find(crc);
    }

    virtual TagID GetFragTagID(FragmentID fragID, uint32 crc) const
    {
        if (fragID != FRAGMENT_ID_INVALID)
        {
            return m_context.controllerDef.GetFragmentTagDef(fragID)->Find(crc);
        }
        return TAG_ID_INVALID;
    }

    virtual FragmentID GetFragID(uint32 crc) const
    {
        return m_context.controllerDef.m_fragmentIDs.Find(crc);
    }

    virtual const CTagDefinition* GetTagDefinition(FragmentID fragID) const
    {
        if (fragID != FRAGMENT_ID_INVALID)
        {
            return m_context.controllerDef.GetFragmentTagDef(fragID);
        }
        return NULL;
    }

    virtual void Queue(IAction& action, float time = -1.0f);
    virtual void Requeue(IAction& action);

    virtual void Update(float timePassed);

    virtual SAnimationContext& GetContext()
    {
        return m_context;
    }

    virtual const SAnimationContext& GetContext() const
    {
        return m_context;
    }

    virtual void Pause();
    virtual void Resume(uint32 resumeFlags = IActionController::ERF_Default);

    virtual void SetFlag(EActionControllerFlags flag, bool enable)
    {
        if (enable)
        {
            m_flags |= flag;
        }
        else
        {
            m_flags &= ~flag;
        }
    }

    bool GetFlag(EActionControllerFlags flag) const
    {
        return (m_flags & flag) != 0;
    }

    virtual void SetTimeScale(float timeScale);
    virtual float GetTimeScale() const { return m_timeScale; }

    #ifndef _RELEASE

    bool DebugFragments(bool isRootScope)
    {
        return ((s_mnDebugFragments > 0) && GetFlag(AC_DebugDraw) && (isRootScope || (s_mnDebugFragments == 2)));
    }

    void GetStateString(string& state) const;

    #endif //_RELEASE


    // Only needed for animationgraph?
    virtual bool IsActionPending(uint32 userToken) const;

    virtual bool CanInstall(const IAction& action, const ActionScopes& scopeMask, float timeStep, float& timeTillInstall) const;

    virtual bool QueryDuration(IAction& action, float& fragmentDuration, float& transitionDuration) const;


    virtual void SetSlaveController(IActionController& target, uint32 targetContext, bool enslave, const IAnimationDatabase* piOptionTargetDatabase);
    void FlushSlaveController(IActionController& target);

    virtual void RegisterListener(IMannequinListener* listener);
    virtual void UnregisterListener(IMannequinListener* listener);

    virtual IProceduralContext* FindOrCreateProceduralContext(const char* contextName);
    virtual const IProceduralContext* FindProceduralContext(const char* contextName) const;
    virtual IProceduralContext* FindProceduralContext(const char* contextName);
    virtual IProceduralContext* CreateProceduralContext(const char* contextName);

    virtual QuatT ExtractLocalAnimLocation(FragmentID fragID, TagState fragTags, uint32 scopeID, uint32 optionIdx);

    virtual void Release();

    virtual const SMannParameter* GetParam(const char* paramName) const
    {
        return GetParam(MannGenCRC(paramName));
    }
    virtual const SMannParameter* GetParam(uint32 paramNameCRC) const
    {
        const SMannParameter* pRet = m_mannequinParams.FindParam(paramNameCRC);
        for (TOwningControllerList::const_iterator iter = m_owningControllers.begin(); (iter != m_owningControllers.end()) && (pRet == NULL); iter++)
        {
            const CActionController* pActionControllerParent = *iter;
            pRet = pActionControllerParent->GetParam(paramNameCRC);
        }
        return pRet;
    }
    virtual bool RemoveParam(const char* paramName)
    {
        return m_mannequinParams.RemoveParam(paramName);
    }
    virtual bool RemoveParam(uint32 paramNameCRC)
    {
        return m_mannequinParams.RemoveParam(paramNameCRC);
    }
    virtual void SetParam(const char* paramName, const SMannParameter& param)
    {
        m_mannequinParams.SetParam(param.crc, param.value);
    }
    virtual void SetParam(const SMannParameter& param)
    {
        m_mannequinParams.SetParam(param.crc, param.value);
    }
    virtual void ResetParams()
    {
        m_mannequinParams.Reset();
    }

    // -- ~IActionController implementation --------------------------------------

    ActionScopes FlushScopesByScopeContext(uint32 scopeContextID, EFlushMethod flushMethod = FM_Normal); // returns the ActionScopes using this scopeContext

    void ReleaseScopes();
    void ReleaseScopeContexts();

#ifdef _DEBUG
    void ValidateActions();
#endif //_DEBUG

    const char* GetSafeEntityName() const // NOTE: should be safe to build a filename out of!
    {
        if (!m_entityId.IsValid())
        {
            return "<no entity>";
        }

        if (IsLegacyEntityId(m_entityId))
        {
            IEntity* pEntity = gEnv->pEntitySystem->GetEntity(GetLegacyEntityId(m_entityId));
            CRY_ASSERT(pEntity);
            return pEntity ? pEntity->GetName() : "<unknown>";
        }
        else
        {
            AZ::Entity* entity = nullptr;
            EBUS_EVENT_RESULT(entity, AZ::ComponentApplicationBus, FindEntity, m_entityId);
            return entity ? entity->GetName().c_str() : "<unknown>";
        }
    }

    IProceduralContext* FindProceduralContext(uint32 contextNameCRC) const;

    void OnEntityReturnedToPool(EntityId entityId, IEntity* pEntity);

    void StartAction(IAction& action);
    void EndAction(IAction& action, EFlushMethod flushMethod = FM_Normal);

    void RegisterOwner(CActionController& owner)
    {
        m_owningControllers.push_back(&owner);
    }

    void UnregisterOwner(CActionController& owner)
    {
        TOwningControllerList::iterator iter = std::find(m_owningControllers.begin(), m_owningControllers.end(), &owner);

        if (iter != m_owningControllers.end())
        {
            m_owningControllers.erase(iter);
        }
        else
        {
            CryFatalError("Releasing reference to parent that does not exist!");
        }
    }

#ifndef _RELEASE
    static void ChangeDebug(const char* entName);
    static void DumpSequence(const char* entName, float dumpTime);
#endif //!_RELEASE
    static const TActionControllerList& GetGlobalActionControllers() {return s_actionControllers; }
    static IActionController* FindActionController(const IEntity& entity);
    static void OnShutdown();

private:

    static void RegisterCVars();
    static void UnregisterCVars();
    static void Register(CActionController* ac);
    static void Unregister(CActionController* ac);

#ifndef _RELEASE
    void ValidateScopeContext(uint32 scopeContextID) const;
    void ValidateScopeContexts() const;
    void DebugDraw() const;
    void DebugDrawLocation(const QuatT& location, ColorB colorPos, ColorB colorX, ColorB colorY, ColorB colorZ) const;
#endif //!_RELEASE

    void FlushScope(uint32 scopeID, ActionScopes scopeFlag, EFlushMethod flushMethod = FM_Normal);
    void FlushProceduralContexts();

    bool ResolveActionInstallations(float timePassed);
    void ResolveActionStates();
    bool BlendOffActions(float timePassed);
    void ResolveQueue(float timePassed);
    void PruneQueue();
    ActionScopes EndActionsOnScope(ActionScopes scopeMask, IAction* pPendingAction, bool blendOut = false, EFlushMethod flushMethod = FM_Normal);
    bool TryInstalling(IAction& action, float timePassed);
    bool CanInstall(const IAction& action, TagID subContext, const ActionScopes& scopeMask, float timeStep, float& timeTillInstall) const;
    void Install(IAction& action, float timePassed);
    void PushOntoQueue(IAction& action);
    bool IsDifferent(const FragmentID fragID, const TagState& fragmentTags, const ActionScopes& scopeMask) const override;
    void RequestInstall(const IAction& action, const ActionScopes& scopeMask);

    ActionScopes QueryScopeMask(FragmentID fragID, const TagState& fragTags, const TagID tagContext) const;
    ActionScopes ExpandOverlappingScopes(ActionScopes scopeMask) const;
    void InsertEndingAction(IAction& action);
    void SynchTagsToSlave(SScopeContext& scopeContext, bool enable);

    void SetScopeContextInternal(uint32 scopeContextID, IEntity* entity, AZ::EntityId entityId, ICharacterInstance* character, const IAnimationDatabase* animDatabase);

    AZ::EntityId m_entityId;
    IEntity* m_cachedEntity;
    SAnimationContext& m_context;

    SScopeContext* m_scopeContexts;

    const uint32 m_scopeCount;
    CActionScope* const m_scopeArray;
    ActionScopes m_activeScopes;

    typedef std::vector<IActionPtr> TActionList;
    TActionList m_queuedActions;

    struct SProcContext
    {
        uint32                                nameCRC;
        AZStd::shared_ptr<IProceduralContext> pContext;
    };
    std::vector<SProcContext> m_procContexts;

    uint32 m_flags;
    float m_timeScale;

    ActionScopes m_scopeFlushMask;

    typedef std::vector<CActionController*> TOwningControllerList;
    TOwningControllerList m_owningControllers;

    void Record(const SMannHistoryItem& item);
    void RecordTagState();

    void UpdateValidity();
    bool UpdateRootEntityValidity(); // returns true when root entity is present and valid
    bool UpdateScopeContextValidity(uint32 scopeContextId); // returns true when scopecontext is valid
#ifndef _RELEASE
    void DumpHistory(const char* filename, float earliestTime = 0.0f) const;
    static const int TOTAL_HISTORY_SLOTS = 200;
    SMannHistoryItem m_history[TOTAL_HISTORY_SLOTS];
    uint32 m_historySlot;
#endif //!_RELEASE
    TagState m_lastTagStateRecorded;

    typedef std::vector<IMannequinListener*> TListenerList;
    TListenerList m_listeners;

    TActionList m_endedActions;
    std::vector< std::pair<IActionPtr, bool> > m_triggeredActions;

    CMannequinParams m_mannequinParams;

    uint32 m_scopeContextID;
    const IAnimationDatabase* m_animDatabase;
    AZ::EntityId m_ContextEntityId;

    static uint32 s_blendChannelCRCs[MANN_NUMBER_BLEND_CHANNELS];
    static TActionControllerList s_actionControllers;
    static CActionController::TActionList s_actionList;
    static CActionController::TActionList s_tickedActions;

    static bool s_registeredCVars;
#ifndef _RELEASE
    static ICVar* s_cvarMnDebug;
    static CActionController* s_debugAC;
    static int s_mnDebugFragments;
#endif //!_RELEASE
#if CRYMANNEQUIN_WARN_ABOUT_VALIDITY()
    static int s_mnFatalErrorOnInvalidEntity;
    static int s_mnFatalErrorOnInvalidCharInst;
#endif // !CRYMANNEQUIN_WARN_ABOUT_VALIDITY()
};

#endif // CRYINCLUDE_CRYACTION_MANNEQUIN_ACTIONCONTROLLER_H
