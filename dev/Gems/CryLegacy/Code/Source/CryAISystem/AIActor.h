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

// Description : Header for the CAIActor class


#ifndef CRYINCLUDE_CRYAISYSTEM_AIACTOR_H
#define CRYINCLUDE_CRYAISYSTEM_AIACTOR_H
#pragma once

#include "IAIActor.h"
#include "IAgent.h"
#include "AIObject.h"
#include "BlackBoard.h"

#include "ValueHistory.h"
#include <INavigationSystem.h>

#include "PersonalLog.h"

struct IPerceptionHandlerModifier;
struct SShape;

class SelectionTree;

namespace BehaviorTree
{
    struct INode;
    struct BehaviorTreeInstance;
    class TimestampCollection;
}

enum EObjectUpdate
{
    AIUPDATE_FULL,
    AIUPDATE_DRY,
};

// Structure reflecting the physical entity parts.
// When choosing the target location to hit, the AI chooses amongst one of these.
struct SAIDamagePart
{
    SAIDamagePart()
        : pos(ZERO)
        , damageMult(0.f)
        , volume(0.f)
        , surfaceIdx(0) {}
    void GetMemoryUsage(ICrySizer* pSizer) const {}
    Vec3    pos;                // Position of the part.
    float   damageMult; // Damage multiplier of the part, can be used to choose the most damage causing part.
    float   volume;         // The volume of the part, can be used to choose the largest part to hit.
    int     surfaceIdx; // The index of the surface material.
};

typedef std::vector<SAIDamagePart>  DamagePartVector;
typedef std::vector<IPerceptionHandlerModifier*> TPerceptionHandlerModifiersVector;


/*! Basic ai object class. Defines a framework that all puppets and points of interest
    later follow.
*/
class CAIActor
    : public CAIObject
    , public IAIActor
{
    friend class CAISystem;

public:
    CAIActor();
    virtual ~CAIActor();

    virtual const IAIActor* CastToIAIActor() const { return this; }
    virtual IAIActor* CastToIAIActor() { return this; }

    virtual bool CanDamageTarget(IAIObject* target = 0) const;
    virtual bool CanDamageTargetWithMelee() const;

    virtual IPhysicalEntity* GetPhysics(bool bWantCharacterPhysics = false) const;

    void SetBehaviorVariable(const char* variableName, bool value);
    bool GetBehaviorVariable(const char* variableName) const;

    class SelectionTree* GetBehaviorSelectionTree() const;
    class SelectionVariables* GetBehaviorSelectionVariables() const;

    void ResetBehaviorSelectionTree(EObjectResetType type);
    bool ProcessBehaviorSelectionTreeSignal(const char* signalName, uint32 signalCRC);
    bool UpdateBehaviorSelectionTree();

    void ResetModularBehaviorTree(EObjectResetType type);

#if defined(CRYAISYSTEM_DEBUG)

    void DebugDrawBehaviorSelectionTree();

#endif

    const SAIBodyInfo& QueryBodyInfo();
    const SAIBodyInfo& GetBodyInfo() const;

    ////////////////////////////////////////////////////////////////////////////////////////
    //IAIPathAgent//////////////////////////////////////////////////////////////////////////
    virtual IEntity* GetPathAgentEntity() const;
    virtual const char* GetPathAgentName() const;
    virtual unsigned short GetPathAgentType() const;
    virtual float GetPathAgentPassRadius() const;
    virtual Vec3 GetPathAgentPos() const;
    virtual Vec3 GetPathAgentVelocity() const;
    virtual void GetPathAgentNavigationBlockers(NavigationBlockers& navigationBlockers, const struct PathfindRequest* pRequest);

    virtual size_t GetNavNodeIndex() const;

    virtual const AgentMovementAbility& GetPathAgentMovementAbility() const;
    virtual IPathFollower* GetPathFollower() const;

    virtual unsigned int GetPathAgentLastNavNode() const;
    virtual void SetPathAgentLastNavNode(unsigned int lastNavNode);

    virtual void SetPathToFollow(const char* pathName);
    virtual void SetPathAttributeToFollow(bool bSpline);

    //Path finding avoids blocker type by radius.
    virtual void SetPFBlockerRadius(int blockerType, float radius);

    //Can path be modified to use request.targetPoint?  Results are cacheded in request.
    virtual ETriState CanTargetPointBeReached(CTargetPointRequest& request);

    //Is request still valid/use able
    virtual bool UseTargetPointRequest(const CTargetPointRequest& request);//??

    virtual bool GetValidPositionNearby(const Vec3& proposedPosition, Vec3& adjustedPosition) const;
    virtual bool GetTeleportPosition(Vec3& teleportPos) const;

    virtual bool IsPointValidForAgent(const Vec3& pos, uint32 flags) const { return true; }
    //IAIPathAgent//////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////

    //===================================================================
    // inherited virtual interface functions
    //===================================================================
    virtual void SetPos(const Vec3& pos, const Vec3& dirFwrd = Vec3_OneX);
    virtual void Reset(EObjectResetType type);
    virtual void OnObjectRemoved(CAIObject* pObject);
    virtual SOBJECTSTATE& GetState(void) {return m_State; }
    virtual const SOBJECTSTATE& GetState(void) const {return m_State; }
    virtual void SetSignal(int nSignalID, const char* szText, IEntity* pSender = 0, IAISignalExtraData* pData = NULL, uint32 crcCode = 0);
    virtual void OnAIHandlerSentSignal(const char* szText, uint32 crcCode);
    virtual void Serialize(TSerialize ser);
    virtual void Update(EObjectUpdate type);
    virtual void UpdateProxy(EObjectUpdate type);
    virtual void UpdateDisabled(EObjectUpdate type);    // when AI object is disabled still may need to send some signals
    virtual void SetProxy(IAIActorProxy* proxy);
    virtual IAIActorProxy* GetProxy() const;
    virtual bool CanAcquireTarget(IAIObject* pOther) const;
    virtual void ResetPerception();
    virtual bool IsHostile(const IAIObject* pOther, bool bUsingAIIgnorePlayer = true) const;
    virtual void ParseParameters(const AIObjectParams& params, bool bParseMovementParams = true);
    virtual void Event(unsigned short eType, SAIEVENT* pAIEvent);
    virtual void EntityEvent(const SEntityEvent& event);
    virtual void SetGroupId(int id);
    virtual void SetFactionID(uint8 factionID);

    virtual void SetObserver(bool observer);
    virtual uint32 GetObserverTypeMask() const;
    virtual uint32 GetObservableTypeMask() const;

    virtual void ReactionChanged(uint8 factionID, IFactionMap::ReactionType reaction);
    virtual void VisionChanged(float sightRange, float primaryFOVCos, float secondaryFOVCos);
    virtual bool IsObserver() const;
    virtual bool CanSee(const VisionID& otherVisionID) const;

    virtual void RegisterBehaviorListener(IActorBehaviorListener* listener);
    virtual void UnregisterBehaviorListener(IActorBehaviorListener* listener);
    virtual void BehaviorEvent(EBehaviorEvent event);
    virtual void BehaviorChanged(const char* current, const char* previous);

    //===================================================================
    // virtual functions rooted here
    //===================================================================

    // Returns a list containing the information about the parts that can be shot at.
    virtual DamagePartVector*   GetDamageParts() { return 0; }

    const AgentParameters& GetParameters() const { return m_Parameters; }
    virtual void SetParameters(const AgentParameters& params);
    virtual const AgentMovementAbility& GetMovementAbility() const { return m_movementAbility; }
    virtual void SetMovementAbility(AgentMovementAbility& params) { m_movementAbility = params; }
    // (MATT) There is now a method to serialise these {2009/04/23}

    virtual bool IsLowHealthPauseActive() const { return false; }
    virtual IEntity* GetGrabbedEntity() const { return 0; } // consider only player grabbing things, don't care about NPC
    virtual bool IsGrabbedEntityInView(const Vec3& pos) const { return false; }

    void GetLocalBounds(AABB& bbox) const;

    virtual bool IsDevalued(IAIObject* pAIObject) { return false; }

    virtual void ResetLookAt();
    virtual bool SetLookAtPointPos(const Vec3& vPoint, bool bPriority = false);
    virtual bool SetLookAtDir(const Vec3& vDir, bool bPriority = false);

    virtual void ResetBodyTargetDir();
    virtual void SetBodyTargetDir(const Vec3& vDir);
    virtual const Vec3& GetBodyTargetDir() const;

    virtual void SetMoveTarget(const Vec3& vMoveTarget);
    virtual void GoTo(const Vec3& vTargetPos);
    virtual void SetSpeed(float fSpeed);

    virtual bool IsInvisibleFrom(const Vec3& pos, bool bCheckCloak = true, bool bCheckCloakDistance = true, const CloakObservability& cloakObservability = CloakObservability()) const;

    //===================================================================
    // non-virtual functions
    //===================================================================

    void NotifyDeath();
    // Returns true if the cloak is effective (includes check for cloak and
    bool IsCloakEffective(const Vec3& pos) const;

    // Get the sight FOV cosine values for the actor
    void GetSightFOVCos(float& primaryFOVCos, float& secondaryFOVCos) const;
    void CacheFOVCos(float primaryFOV, float secondaryFOV);

    // Used to adjust the visibility range for an object checking if they can see me
    virtual float AdjustTargetVisibleRange(const CAIActor& observer, float fVisibleRange) const;

    // Returns the maximum visible range to the target
    virtual float GetMaxTargetVisibleRange(const IAIObject* pTarget, bool bCheckCloak = true) const;

    // Gets the light level at actors position.
    inline EAILightLevel GetLightLevel() const { return m_lightLevel; }
    virtual bool IsAffectedByLight() const { return m_Parameters.m_PerceptionParams.isAffectedByLight; }

    // Populates list of physics entities to skip for raycasting.
    virtual void GetPhysicalSkipEntities(PhysSkipList& skipList) const;
    virtual void UpdateObserverSkipList();

    virtual NavigationAgentTypeID GetNavigationTypeID() const override
    {
        return m_navigationTypeID;
    }

    // Coordination state change functions
    void CoordinationEntered(const char* signalName);
    void CoordinationExited(const char* signalName);

    bool m_bCheckedBody;

    SOBJECTSTATE        m_State;
    AgentParameters m_Parameters;
    AgentMovementAbility    m_movementAbility;

#ifdef CRYAISYSTEM_DEBUG
    CValueHistory<float>* m_healthHistory;
#endif

    std::vector<CAIObject*> m_probableTargets;

    void AddProbableTarget(CAIObject* pTarget);
    void ClearProbableTargets();

    virtual void EnablePerception(bool enable);
    virtual bool IsPerceptionEnabled() const;

    virtual bool IsActive() const { return m_bEnabled; }
    virtual bool IsAgent() const { return true; }

    inline bool IsUsingCombatLight() const { return m_usingCombatLight; }

    inline float GetCachedWaterOcclusionValue() const { return m_cachedWaterOcclusionValue; }

    virtual IBlackBoard* GetBlackBoard() { return &m_blackBoard; }
    virtual IBlackBoard* GetBehaviorBlackBoard() { return &m_behaviorBlackBoard; }

    virtual IAIObject* GetAttentionTarget() const { return m_refAttentionTarget.GetAIObject(); }
    virtual void SetAttentionTarget(CWeakRef<CAIObject> refAttTarget);

    inline virtual EAITargetThreat GetAttentionTargetThreat() const { return m_State.eTargetThreat; }
    inline virtual EAITargetType GetAttentionTargetType() const { return m_State.eTargetType; }

    inline virtual EAITargetThreat GetPeakThreatLevel() const { return m_State.ePeakTargetThreat; }
    inline virtual EAITargetType GetPeakThreatType() const { return m_State.ePeakTargetType; }
    inline virtual tAIObjectID GetPeakTargetID() const { return m_State.ePeakTargetID; }

    inline virtual EAITargetThreat GetPreviousPeakThreatLevel() const { return m_State.ePreviousPeakTargetThreat; }
    inline virtual EAITargetType GetPreviousPeakThreatType() const { return m_State.ePreviousPeakTargetType; }
    inline virtual tAIObjectID GetPreviousPeakTargetID() const { return m_State.ePreviousPeakTargetID; }

    virtual Vec3 GetFloorPosition(const Vec3& pos);

    // check if enemy is close enough and send OnCloseContact if so
    virtual void CheckCloseContact(IAIObject* pTarget, float distSq);
    inline bool CloseContactEnabled() { return !m_bCloseContact; }
    void SetCloseContact(bool bCloseContact);

    EFieldOfViewResult IsObjectInFOV(CAIObject* pTarget, float fDistanceScale = 1.f) const;

    void AddPersonallyHostile(tAIObjectID hostileID);
    void RemovePersonallyHostile(tAIObjectID hostileID);
    void ResetPersonallyHostiles();
    bool IsPersonallyHostile(tAIObjectID hostileID) const;

#ifdef CRYAISYSTEM_DEBUG
    void UpdateHealthHistory();
#endif

    virtual void SetTerritoryShapeName(const char* szName);
    virtual const char* GetTerritoryShapeName() const;
    virtual const char* GetWaveName() const { return m_Parameters.m_sWaveName.c_str(); }
    virtual bool IsPointInsideTerritoryShape(const Vec3& vPos, bool bCheckHeight) const;
    virtual bool ConstrainInsideTerritoryShape(Vec3& vPos, bool bCheckHeight) const;

    const SShape* GetTerritoryShape() const { return m_territoryShape; }
    SShape* GetTerritoryShape() { return m_territoryShape; }

    void GetMovementSpeedRange(float fUrgency, bool bSlowForStrafe, float& normalSpeed, float& minSpeed, float& maxSpeed) const;

    virtual EFieldOfViewResult IsPointInFOV(const Vec3& pos, float distanceScale = 1.f) const;

    enum ENavInteraction
    {
        NI_IGNORE, NI_STEER, NI_SLOW
    };
    // indicates the way that two objects should negotiate each other
    static ENavInteraction GetNavInteraction(const CAIObject* navigator, const CAIObject* obstacle);

    virtual void CancelRequestedPath(bool actorRemoved);

    enum BehaviorTreeEvaluationMode
    {
        EvaluateWhenVariablesChange,
        EvaluationBlockedUntilBehaviorUnlocks,

        BehaviorTreeEvaluationModeCount,
        FirstBehaviorTreeEvaluationMode = 0
    };

    void SetBehaviorTreeEvaluationMode(const BehaviorTreeEvaluationMode mode) { m_behaviorTreeEvaluationMode = mode; }

    #ifdef AI_COMPILE_WITH_PERSONAL_LOG
    PersonalLog& GetPersonalLog() { return m_personalLog; }
    #endif

    // - returns the initial position at the time the Puppet was constructed
    // - this is the position of where the level designer placed the Puppet
    // - if false is returned, then the Puppet was probably not created through the level editor
    bool GetInitialPosition(Vec3& initialPosition) const;

    // Returns the attention target if it is an agent, or the attention target association, or null if there is no association.
    static CWeakRef<CAIActor> GetLiveTarget(const CWeakRef<CAIObject>& refTarget);

    // Returns the attention target if it is an agent, or the attention target association, or null if there is no association.
    static const CAIObject* GetLiveTarget(const CAIObject* pTarget);

protected:

    uint32 GetFactionVisionMask(uint8 factionID) const;

    void SerializeMovementAbility(TSerialize ser);

    //process cloak_scale fading
    void UpdateCloakScale();

    // Updates the properties of the damage parts.
    void    UpdateDamageParts(DamagePartVector& parts);

    EFieldOfViewResult CheckPointInFOV(const Vec3& vPoint, float fSightRange) const;

    virtual void HandlePathDecision(MNMPathRequestResult& result);
    virtual void HandleVisualStimulus(SAIEVENT* pAIEvent);
    virtual void HandleSoundEvent(SAIEVENT* pAIEvent);
    virtual void HandleBulletRain(SAIEVENT* pAIEvent);

    // AIOT_PLAYER is to be treated specially, as it's the player, but otherwise is the same as
    // AIOT_AGENTSMALL
    enum EAIObjectType
    {
        AIOT_UNKNOWN, AIOT_PLAYER, AIOT_AGENTSMALL, AIOT_AGENTMED, AIOT_AGENTBIG, AIOT_MAXTYPES
    };
    // indicates the sort of object the agent is with regard to navigation
    static EAIObjectType GetObjectType(const CAIObject* ai, unsigned short type);

    _smart_ptr<IAIActorProxy> m_proxy;

    EAILightLevel m_lightLevel;
    bool m_usingCombatLight;
    int8 m_perceptionDisabled;

    float m_cachedWaterOcclusionValue;

    Vec3 m_vLastFullUpdatePos;
    EStance m_lastFullUpdateStance;

    CBlackBoard m_blackBoard;
    CBlackBoard m_behaviorBlackBoard;

    TPerceptionHandlerModifiersVector m_perceptionHandlerModifiers;

    typedef std::set<IActorBehaviorListener*> BehaviorListeners;
    BehaviorListeners m_behaviorListeners;

    BehaviorTreeEvaluationMode m_behaviorTreeEvaluationMode;
    std::unique_ptr<SelectionTree> m_behaviorSelectionTree;
    std::unique_ptr<SelectionVariables> m_behaviorSelectionVariables;
    std::unique_ptr<BehaviorTree::BehaviorTreeInstance> m_behaviorTreeInstance;

    // (MATT) Note: this is a different attention target to the classic. Nasty OO hack {2009/02/03}
    // [4/20/2010 evgeny] Moved here from CPipeUser and CAIPlayer
    CWeakRef<CAIObject> m_refAttentionTarget;

    CTimeValue  m_CloseContactTime; // timeout for close contact
    bool    m_bCloseContact;        // used to prevent the signal OnCloseContact being sent repeatedly to same object

    string  m_territoryShapeName;
    SShape* m_territoryShape;

    float m_bodyTurningSpeed;
    Vec3 m_lastBodyDir;

    float m_stimulusStartTime;

    unsigned int m_activeCoordinationCount;

    bool m_observer;

private:
    float m_FOVPrimaryCos;
    float m_FOVSecondaryCos;

    float m_currentCollisionAvoidanceRadiusIncrement;

    typedef VectorSet<tAIObjectID> PersonallyHostiles;
    PersonallyHostiles m_forcefullyHostiles;

    SAIBodyInfo m_bodyInfo;

    NavigationAgentTypeID m_navigationTypeID;

    #ifdef AI_COMPILE_WITH_PERSONAL_LOG
    PersonalLog m_personalLog;
    #endif

    // position of the Puppet placed by the level designer
    struct SInitialPosition
    {
        Vec3    pos;
        bool    isValid;    // becomes valid in Reset()

        SInitialPosition() { isValid = false; }
    };

    SInitialPosition m_initialPosition;

private:
    void StartBehaviorTree(const char* behaviorName);
    void StopBehaviorTree();
    bool IsRunningBehaviorTree() const;

    bool m_runningBehaviorTree;
};

ILINE const CAIActor* CastToCAIActorSafe(const IAIObject* pAI) { return pAI ? pAI->CastToCAIActor() : 0; }
ILINE CAIActor* CastToCAIActorSafe(IAIObject* pAI) { return pAI ? pAI->CastToCAIActor() : 0; }

#endif // CRYINCLUDE_CRYAISYSTEM_AIACTOR_H
