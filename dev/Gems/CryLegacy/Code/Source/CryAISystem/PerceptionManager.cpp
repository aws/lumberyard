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
#include "PerceptionManager.h"
#include "CAISystem.h"
#include "Puppet.h"
#include "AIPlayer.h"
#include "AIVehicle.h"
#include "DebugDrawContext.h"

// AI Stimulus names for debugging
static const char* g_szAIStimulusType[AISTIM_LAST] =
{
    "SOUND",
    "COLLISION",
    "EXPLOSION",
    "BULLET_WHIZZ",
    "BULLET_HIT",
    "GRENADE"
};
static const char* g_szAISoundStimType[AISOUND_LAST] =
{
    " GENERIC",
    " COLLISION",
    " COLLISION_LOUD",
    " MOVEMENT",
    " MOVEMENT_LOUD",
    " WEAPON",
    " EXPLOSION"
};
static const char* g_szAIGrenadeStimType[AIGRENADE_LAST] =
{
    " THROWN",
    " COLLISION",
    " FLASH_BANG",
    " SMOKE"
};

std::vector<CAIObject*> CPerceptionManager::s_targetEntities;
std::vector<CAIVehicle*> CPerceptionManager::s_playerVehicles;

//===================================================================
// CPerceptionManager
//===================================================================
CPerceptionManager::CPerceptionManager()
{
    Reset(IAISystem::RESET_INTERNAL);
    for (unsigned i = 0; i < AI_MAX_STIMULI; ++i)
    {
        m_stimulusTypes[i].Reset();
    }
}

//===================================================================
// CPerceptionManager
//===================================================================
CPerceptionManager::~CPerceptionManager()
{
}

//===================================================================
// InitCommonTypeDescs
//===================================================================
void CPerceptionManager::InitCommonTypeDescs()
{
    SAIStimulusTypeDesc desc;

    // Sound
    desc.Reset();
    desc.SetName("Sound");
    desc.processDelay = 0.15f;
    desc.duration[AISOUND_GENERIC] = 2.0f;
    desc.duration[AISOUND_COLLISION] = 4.0f;
    desc.duration[AISOUND_COLLISION_LOUD] = 4.0f;
    desc.duration[AISOUND_MOVEMENT] = 2.0f;
    desc.duration[AISOUND_MOVEMENT_LOUD] = 4.0f;
    desc.duration[AISOUND_WEAPON] = 4.0f;
    desc.duration[AISOUND_EXPLOSION] = 6.0f;
    desc.filterTypes = 0;
    desc.nFilters = 0;
    RegisterStimulusDesc(AISTIM_SOUND, desc);

    // Collision
    desc.Reset();
    desc.SetName("Collision");
    desc.processDelay = 0.15f;
    desc.duration[AICOL_SMALL] = 7.0f;
    desc.duration[AICOL_MEDIUM] = 7.0f;
    desc.duration[AICOL_LARGE] = 7.0f;
    desc.filterTypes = (1 << AISTIM_COLLISION) | (1 << AISTIM_EXPLOSION);
    desc.nFilters = 2;
    desc.filters[0].Set(AISTIM_COLLISION, 0, AISTIMFILTER_MERGE_AND_DISCARD, 0.9f); // Merge nearby collisions
    desc.filters[1].Set(AISTIM_EXPLOSION, 0, AISTIMFILTER_DISCARD, 2.5f); // Discard collision near explosions
    RegisterStimulusDesc(AISTIM_COLLISION, desc);

    // Explosion
    desc.Reset();
    desc.SetName("Explosion");
    desc.processDelay = 0.15f;
    desc.duration[0] = 7.0f;
    desc.filterTypes = (1 << AISTIM_EXPLOSION);
    desc.nFilters = 1;
    desc.filters[0].Set(AISTIM_EXPLOSION, 0, AISTIMFILTER_MERGE_AND_DISCARD, 0.5f); // Merge nearby explosions
    RegisterStimulusDesc(AISTIM_EXPLOSION, desc);

    // Bullet Whizz
    desc.Reset();
    desc.SetName("BulletWhizz");
    desc.processDelay = 0.01f;
    desc.duration[0] = 0.5f;
    desc.filterTypes = 0;
    desc.nFilters = 0;
    RegisterStimulusDesc(AISTIM_BULLET_WHIZZ, desc);

    // Bullet Hit
    desc.Reset();
    desc.SetName("BulletHit");
    desc.processDelay = 0.15f;
    desc.duration[0] = 0.5f;
    desc.filterTypes = (1 << AISTIM_BULLET_HIT);
    desc.nFilters = 1;
    desc.filters[0].Set(AISTIM_BULLET_HIT, 0, AISTIMFILTER_MERGE_AND_DISCARD, 0.5f); // Merge nearby hits
    RegisterStimulusDesc(AISTIM_BULLET_HIT, desc);

    // Grenade
    desc.Reset();
    desc.SetName("Grenade");
    desc.processDelay = 0.15f;
    desc.duration[AIGRENADE_THROWN] = 6.0f;
    desc.duration[AIGRENADE_COLLISION] = 6.0f;
    desc.duration[AIGRENADE_FLASH_BANG] = 6.0f;
    desc.duration[AIGRENADE_SMOKE] = 6.0f;
    desc.filterTypes = (1 << AISTIM_GRENADE);
    desc.nFilters = 1;
    desc.filters[0].Set(AISTIM_GRENADE, AIGRENADE_COLLISION, AISTIMFILTER_MERGE_AND_DISCARD, 1.0f); // Merge nearby collisions
    RegisterStimulusDesc(AISTIM_GRENADE, desc);
}

bool CPerceptionManager::RegisterStimulusDesc(EAIStimulusType type, const SAIStimulusTypeDesc& desc)
{
    m_stimulusTypes[type] = desc;
    return true;
}


//===================================================================
// Reset
//===================================================================
void CPerceptionManager::Reset(IAISystem::EResetReason reason)
{
    /*  m_visChecks = 0;
        m_visChecksMax = 0;
        m_visChecksRays = 0;
        m_visChecksRaysMax = 0;
        m_visChecksHistoryHead = 0;
        m_visChecksHistorySize = 0;
        m_nRaysThisUpdateFrame = 0;
        m_maxStimsPerUpdate = 0;*/

    m_visBroadPhaseDt = 0;

    if (reason == IAISystem::RESET_UNLOAD_LEVEL)
    {
        stl::free_container(m_incomingStimuli);
        for (unsigned int i = 0; i < AI_MAX_STIMULI; ++i)
        {
            stl::free_container(m_stimuli[i]);
            m_ignoreStimuliFrom[i].clear();
        }

        if (m_eventListeners.empty())
        {
            stl::free_container(m_eventListeners);
        }

        stl::free_container(s_targetEntities);
        stl::free_container(s_playerVehicles);
    }
    else
    {
        for (unsigned i = 0; i < AI_MAX_STIMULI; ++i)
        {
            m_stimuli[i].reserve(64);
            m_stimuli[i].clear();
            m_ignoreStimuliFrom[i].clear();
        }
        m_incomingStimuli.reserve(64);
        m_incomingStimuli.clear();
        InitCommonTypeDescs();
        s_targetEntities.reserve(10);
        s_playerVehicles.reserve(10);
    }
}

//===================================================================
// UpdatePerception
//===================================================================
bool CPerceptionManager::UpdatePerception(CAIActor* pAIActor, std::vector<CAIObject*>& priorityTargets)
{
    FUNCTION_PROFILER(gEnv->pSystem, PROFILE_AI);

    m_stats.trackers[PERFTRACK_UPDATES].Inc();

    if (pAIActor->IsEnabled() && pAIActor->IsPerceptionEnabled())
    {
        const Vec3& vAIActorPos = pAIActor->GetPos();

        if (!priorityTargets.empty())
        {
            FRAME_PROFILER("AIPlayerVisibilityCheck", gEnv->pSystem, PROFILE_AI);

            // Priority targets.
            for (unsigned i = 0, ni = priorityTargets.size(); i < ni; ++i)
            {
                CAIObject* pTarget = priorityTargets[i];
                if (!pAIActor->IsHostile(pTarget))
                {
                    continue;
                }

                if (pAIActor->IsDevalued(pTarget))
                {
                    continue;
                }

                if (!pAIActor->CanAcquireTarget(pTarget))
                {
                    continue;
                }

                IAIObject::EFieldOfViewResult viewResult = pAIActor->IsObjectInFOV(pTarget);
                if (IAIObject::eFOV_Outside == viewResult)
                {
                    continue;
                }

                pTarget->SetObservable(true);

                m_stats.trackers[PERFTRACK_VIS_CHECKS].Inc();

                const Vec3& vTargetPos = pTarget->GetPos();

                // TODO(Márcio): Implement
                // To make it generic, can have the vision map store 2 collision flag fields and alternate them in case of failure.
                /*
                bool skipSoftCover = false;
                // See grenades through vegetation.
                if (pTarget->GetType() == AIOBJECT_GRENADE || pTarget->GetType() == AIOBJECT_RPG)
                    skipSoftCover = true;

                // See live and memory target through vegetation.
                if ((pAIActor->GetAttentionTargetType() == AITARGET_VISUAL ||
                    pAIActor->GetAttentionTargetType() == AITARGET_MEMORY) &&
                    pAIActor->GetAttentionTargetThreat() == AITHREAT_AGGRESSIVE)
                {
                    const float distSq = Distance::Point_PointSq(vAIActorPos, vTargetPos);
                    skipSoftCover = (distSq < sqr(pPuppet->GetParameters().m_fMeleeRange * 1.2f));
                }*/

                bool visible = false;
                if (!RayOcclusionPlaneIntersection(vAIActorPos, vTargetPos))
                {
                    visible = pAIActor->CanSee(pTarget->GetVisionID());
                }

                if (visible)
                {
                    // Notify visual perception.
                    SAIEVENT event;
                    event.sourceId = pTarget->GetPerceivedEntityID();
                    event.bFuzzySight = (viewResult == IAIObject::eFOV_Secondary);
                    event.vPosition = vTargetPos;
                    pAIActor->Event(AIEVENT_ONVISUALSTIMULUS, &event);
                }

                switch (pTarget->GetType())
                {
                case AIOBJECT_PLAYER:
                case AIOBJECT_ACTOR:
                    if (pTarget->IsEnabled())
                    {
                        if (pAIActor->GetAttentionTarget() == pTarget)
                        {
                            const float distSq = Distance::Point_PointSq(vAIActorPos, vTargetPos);
                            if (distSq < sqr(pAIActor->GetParameters().m_fMeleeRange))
                            {
                                if (pAIActor->CloseContactEnabled())
                                {
                                    pAIActor->SetSignal(1, "OnCloseContact", pTarget->GetEntity(), 0, gAIEnv.SignalCRCs.m_nOnCloseContact);
                                    pAIActor->SetCloseContact(true);
                                }
                            }
                        }
                    }
                }
            }
        }

        {
            FRAME_PROFILER("ProbableTargets", gEnv->pSystem, PROFILE_AI);

            // Probable targets.
            for (unsigned i = 0, ni = pAIActor->m_probableTargets.size(); i < ni; ++i)
            {
                CAIObject* pProbTarget = pAIActor->m_probableTargets[i];

                CPuppet* pTargetPuppet = pProbTarget->CastToCPuppet();
                if (pTargetPuppet)
                {
                    if (pTargetPuppet->m_Parameters.m_bInvisible || pAIActor->IsDevalued(pProbTarget))
                    {
                        continue;
                    }
                }

                if (!pAIActor->CanAcquireTarget(pProbTarget))
                {
                    continue;
                }

                if (!pAIActor->IsHostile(pProbTarget))
                {
                    continue;
                }

                IAIObject::EFieldOfViewResult viewResult = pAIActor->IsObjectInFOV(pProbTarget);
                if (IAIObject::eFOV_Outside == viewResult)
                {
                    continue;
                }

                pProbTarget->SetObservable(true);

                const Vec3& vProbTargetPos = pProbTarget->GetPos();

                bool visible = false;
                if (!RayOcclusionPlaneIntersection(vAIActorPos, vProbTargetPos))
                {
                    visible = pAIActor->CanSee(pProbTarget->GetVisionID());
                }

                if (visible)
                {
                    SAIEVENT event;
                    event.sourceId = pProbTarget->GetPerceivedEntityID();
                    event.bFuzzySight = (viewResult == IAIObject::eFOV_Secondary);
                    event.fThreat = 1.f;
                    event.vPosition = vProbTargetPos;
                    pAIActor->Event(AIEVENT_ONVISUALSTIMULUS, &event);

                    const float distSq = Distance::Point_PointSq(vAIActorPos, vProbTargetPos);
                    pAIActor->CheckCloseContact(pProbTarget, distSq);
                }
            }
        }
    }

    return false;
}

bool CPerceptionManager::FilterStimulus(SAIStimulus* stim)
{
    const SAIStimulusTypeDesc* desc = &m_stimulusTypes[stim->type];
    const SAIStimulusFilter* filters[AI_MAX_FILTERS];

    // Merge and filter with current active stimuli.
    for (unsigned int i = 0; i < AI_MAX_STIMULI; ++i)
    {
        unsigned int mask = (1 << i);
        if ((mask & desc->filterTypes) == 0)
        {
            continue;
        }

        // Collect filters for this stimuli type
        unsigned int nFilters = 0;
        for (unsigned j = 0; j < desc->nFilters; ++j)
        {
            const SAIStimulusFilter* filter = &desc->filters[j];
            if ((unsigned int)filter->type != stim->type)
            {
                continue;
            }
            if (filter->subType && (unsigned int)filter->subType != stim->subType)
            {
                continue;
            }
            filters[nFilters++] = filter;
        }
        if (nFilters == 0)
        {
            continue;
        }

        std::vector<SStimulusRecord>& stimuli = m_stimuli[i];
        for (unsigned j = 0, nj = stimuli.size(); j < nj; ++j)
        {
            SStimulusRecord& s = stimuli[j];
            Vec3 diff = stim->pos - s.pos;
            float distSq = diff.GetLengthSquared();

            // Apply filters
            for (unsigned int k = 0; k < nFilters; ++k)
            {
                const SAIStimulusFilter* filter = filters[k];
                if (filter->subType && (filter->subType & (1 << s.subType)))
                {
                    continue;
                }

                if (filter->merge == AISTIMFILTER_MERGE_AND_DISCARD)
                {
                    // Merge stimuli
                    // Allow to merge stimuli before they are processed.
                    const float duration = desc->duration[stim->subType];
                    if (distSq < sqr(s.radius * filter->scale))
                    {
                        if ((duration - s.t) < desc->processDelay)
                        {
                            float dist = sqrtf(distSq);
                            // Merge spheres into a larger one.
                            if (dist > 0.00001f)
                            {
                                diff *= 1.0f / dist;
                                // Calc new radius
                                float r = s.radius;
                                s.radius = (dist + r + stim->radius) / 2;
                                // Calc new location
                                s.pos += diff * (s.radius - r);
                            }
                            else
                            {
                                // Spheres are at same location, merge radii.
                                s.radius = max(s.radius, stim->radius);
                            }
                        }
                        return true;
                    }
                }
                else
                {
                    // Discard stimuli
                    if (distSq < sqr(s.radius * filter->scale))
                    {
                        return true;
                    }
                }
            }
        }
    }

    // Not filtered nor merged, should create new stimulus.
    return false;
}

//===================================================================
// Update
//===================================================================
void CPerceptionManager::Update(float dt)
{
    // Update stats about the incoming stimulus.
    m_stats.trackers[PERFTRACK_INCOMING_STIMS].Inc(m_incomingStimuli.size());

    // Process incoming stimuli
    bool previousDiscarded = false;
    for (unsigned i = 0, ni = m_incomingStimuli.size(); i < ni; ++i)
    {
        SAIStimulus& is = m_incomingStimuli[i];
        // Check if stimulus should be discarded because it is set linked to the discard rules
        // of the previous stimulus.
        bool discardWithPrevious = previousDiscarded && (is.flags & AISTIMPROC_FILTER_LINK_WITH_PREVIOUS);
        if (!discardWithPrevious && !FilterStimulus(&is))
        {
            const SAIStimulusTypeDesc* desc = &m_stimulusTypes[is.type];
            std::vector<SStimulusRecord>& stimuli = m_stimuli[is.type];

            // Create new Stimulus
            stimuli.resize(stimuli.size() + 1);
            SStimulusRecord& stim = stimuli.back();
            stim.sourceId = is.sourceId;
            stim.targetId = is.targetId;
            stim.pos = is.pos;
            stim.dir = is.dir;
            stim.radius = is.radius;
            stim.t = desc->duration[is.subType];
            stim.type = is.type;
            stim.subType = is.subType;
            stim.flags = is.flags;
            stim.dispatched = 0;

            previousDiscarded = false;
        }
        else
        {
            previousDiscarded = true;
        }
    }
    m_incomingStimuli.clear();

    // Update stimuli
    // Merge and filter with current active stimuli.
    for (unsigned int i = 0; i < AI_MAX_STIMULI; ++i)
    {
        std::vector<SStimulusRecord>& stims = m_stimuli[i];
        for (unsigned j = 0; j < stims.size(); )
        {
            SStimulusRecord& stim = stims[j];
            const SAIStimulusTypeDesc* desc = &m_stimulusTypes[stim.type];

            // Dispatch
            if (!stim.dispatched && stim.t < (desc->duration[stim.subType] - desc->processDelay))
            {
                float threat = 1.0f;
                switch (stim.type)
                {
                case AISTIM_SOUND:
                    HandleSound(stim);
                    switch (stim.subType)
                    {
                    case AISOUND_GENERIC:
                        threat = 0.3f;
                        break;
                    case AISOUND_COLLISION:
                        threat = 0.3f;
                        break;
                    case AISOUND_COLLISION_LOUD:
                        threat = 0.3f;
                        break;
                    case AISOUND_MOVEMENT:
                        threat = 0.5f;
                        break;
                    case AISOUND_MOVEMENT_LOUD:
                        threat = 0.5f;
                        break;
                    case AISOUND_WEAPON:
                        threat = 1.0f;
                        break;
                    case AISOUND_EXPLOSION:
                        threat = 1.0f;
                        break;
                    }
                    ;
                    break;
                case AISTIM_COLLISION:
                    HandleCollision(stim);
                    threat = 0.2f;
                    break;
                case AISTIM_EXPLOSION:
                    HandleExplosion(stim);
                    threat = 1.0f;
                    break;
                case AISTIM_BULLET_WHIZZ:
                    HandleBulletWhizz(stim);
                    threat = 0.7f;
                    break;
                case AISTIM_BULLET_HIT:
                    HandleBulletHit(stim);
                    threat = 1.0f;
                    break;
                case AISTIM_GRENADE:
                    HandleGrenade(stim);
                    threat = 0.7f;
                    break;
                default:
                    // Invalid type
                    AIAssert(0);
                    break;
                }

                NotifyAIEventListeners(stim, threat);

                stim.dispatched = 1;
            }

            // Update stimulus time.
            stim.t -= dt;
            if (stim.t < 0.0f)
            {
                // The stimuli has timed out, delete it.
                if (&stims[j] != &stims.back())
                {
                    stims[j] = stims.back();
                }
                stims.pop_back();
            }
            else
            {
                // Advance
                ++j;
            }
        }

        // Update stats about the stimulus.
        m_stats.trackers[PERFTRACK_STIMS].Inc(stims.size());

        // Update the ignores.
        if (!m_ignoreStimuliFrom[i].empty())
        {
            StimulusIgnoreMap::iterator it = m_ignoreStimuliFrom[i].begin();
            while (it != m_ignoreStimuliFrom[i].end())
            {
                it->second -= dt;
                if (it->second <= 0.0f)
                {
                    StimulusIgnoreMap::iterator del = it;
                    ++it;
                    m_ignoreStimuliFrom[i].erase(del);
                }
                else
                {
                    ++it;
                }
            }
        }
    }

    VisCheckBroadPhase(dt);

    // Update stats
    m_stats.Update();
}

//===================================================================
// VisCheckBroadPhase
//===================================================================
void CPerceptionManager::VisCheckBroadPhase(float dt)
{
    FUNCTION_PROFILER(gEnv->pSystem, PROFILE_AI);

    ActorLookUp& lookUp = *gAIEnv.pActorLookUp;
    lookUp.Prepare(ActorLookUp::Position | ActorLookUp::EntityID);

    const float UPDATE_DELTA_TIME = 0.2f;

    m_visBroadPhaseDt += dt;
    if (m_visBroadPhaseDt < UPDATE_DELTA_TIME)
    {
        return;
    }
    m_visBroadPhaseDt -= UPDATE_DELTA_TIME;
    if (m_visBroadPhaseDt > UPDATE_DELTA_TIME)
    {
        m_visBroadPhaseDt = 0.0f;
    }

    size_t activeActorCount = lookUp.GetActiveCount();

    if (!activeActorCount)
    {
        return;
    }

    // Find player vehicles (driver inside but disabled).
    s_playerVehicles.clear();

    const AIObjectOwners::iterator vehicleBegin = gAIEnv.pAIObjectManager->m_Objects.find(AIOBJECT_VEHICLE);
    const AIObjectOwners::iterator aiEnd = gAIEnv.pAIObjectManager->m_Objects.end();
    for (AIObjectOwners::iterator ai = vehicleBegin; ai != aiEnd && ai->first == AIOBJECT_VEHICLE; ++ai)
    {
        CAIVehicle* pVehicle = (CAIVehicle*)ai->second.GetAIObject();
        if (!pVehicle->IsEnabled() && pVehicle->IsDriverInside())
        {
            s_playerVehicles.push_back(pVehicle);
        }
    }

    // Find target entities
    s_targetEntities.clear();

    const AIObjectOwners::iterator targetsIt = gAIEnv.pAIObjectManager->m_Objects.find(AIOBJECT_TARGET);
    const AIObjectOwners::iterator targetsEnd = gAIEnv.pAIObjectManager->m_Objects.end();

    for (AIObjectOwners::iterator ai = targetsIt; ai != targetsEnd && ai->first == AIOBJECT_TARGET; ++ai)
    {
        CAIObject* pTarget = ai->second.GetAIObject();
        if (pTarget->IsEnabled())
        {
            s_targetEntities.push_back(pTarget);
        }
    }

    // Clear potential targets.
    for (size_t actorIndex = 0; actorIndex < activeActorCount; ++actorIndex)
    {
        CAIActor* pAIActor = lookUp.GetActor<CAIActor>(actorIndex);
        pAIActor->ClearProbableTargets();
    }

    // Find potential targets.
    for (size_t actorIndex = 0; actorIndex < activeActorCount; ++actorIndex)
    {
        CAIActor* pFirst = lookUp.GetActor<CAIActor>(actorIndex);

        const Vec3 firstPos = lookUp.GetPosition(actorIndex);

        // Check against other AIs
        for (size_t actorIndex2 = actorIndex + 1; actorIndex2 < activeActorCount; ++actorIndex2)
        {
            CAIActor* pSecond = lookUp.GetActor<CAIActor>(actorIndex2);

            const Vec3 secondPos = lookUp.GetPosition(actorIndex2);

            float distSq = Distance::Point_PointSq(firstPos, secondPos);
            if ((distSq < sqr(pFirst->GetMaxTargetVisibleRange(pSecond))) && pFirst->IsHostile(pSecond))
            {
                pFirst->AddProbableTarget(pSecond);
            }

            if ((distSq < sqr(pSecond->GetMaxTargetVisibleRange(pFirst)) && pSecond->IsHostile(pFirst)))
            {
                pSecond->AddProbableTarget(pFirst);
            }
        }

        // Check against player vehicles.
        for (unsigned j = 0, nj = s_playerVehicles.size(); j < nj; ++j)
        {
            CAIVehicle* pSecond = s_playerVehicles[j];

            const Vec3 secondPos = pSecond->GetPos();

            float distSq = Distance::Point_PointSq(firstPos, secondPos);
            if ((distSq < sqr(pFirst->GetMaxTargetVisibleRange(pSecond))) && pFirst->IsHostile(pSecond))
            {
                pFirst->AddProbableTarget(pSecond);
            }
        }

        // Check against target entities.
        for (unsigned j = 0, nj = s_targetEntities.size(); j < nj; ++j)
        {
            CAIObject* pSecond = s_targetEntities[j];

            const Vec3 secondPos = pSecond->GetPos();

            float distSq = Distance::Point_PointSq(firstPos, secondPos);
            if ((distSq < sqr(pFirst->GetMaxTargetVisibleRange(pSecond))) && pFirst->IsHostile(pSecond))
            {
                pFirst->AddProbableTarget(pSecond);
            }
        }
    }
}


inline int bit(unsigned b, unsigned x)
{
    return (x >> b) & 1;
}

inline ColorB GetColorFromId(unsigned x)
{
    unsigned r = (bit(0, x) << 7) | (bit(4, x) << 5) | (bit(7, x) << 3);
    unsigned g = (bit(1, x) << 7) | (bit(5, x) << 5) | (bit(8, x) << 3);
    unsigned b = (bit(2, x) << 7) | (bit(6, x) << 5) | (bit(9, x) << 3);
    return ColorB(255 - r, 255 - g, 255 - b);
}


//===================================================================
// DebugDraw
//===================================================================
void CPerceptionManager::DebugDraw(int mode)
{
    CAIPlayer* pPlayer = CastToCAIPlayerSafe(GetAISystem()->GetPlayer());

    typedef std::map<unsigned, unsigned> OccMap;
    OccMap occupied;

    CDebugDrawContext dc;

    for (unsigned i = 0; i < AI_MAX_STIMULI; ++i)
    {
        switch (i)
        {
        case AISTIM_SOUND:
            if (gAIEnv.CVars.DrawSoundEvents == 0)
            {
                continue;
            }
            break;
        case AISTIM_COLLISION:
            if (gAIEnv.CVars.DrawCollisionEvents == 0)
            {
                continue;
            }
            break;
        case AISTIM_EXPLOSION:
            if (gAIEnv.CVars.DrawExplosions == 0)
            {
                continue;
            }
            break;
        case AISTIM_BULLET_WHIZZ:
        case AISTIM_BULLET_HIT:
            if (gAIEnv.CVars.DrawBulletEvents == 0)
            {
                continue;
            }
            break;
        case AISTIM_GRENADE:
            if (gAIEnv.CVars.DrawGrenadeEvents == 0)
            {
                continue;
            }
            break;
        }
        ;

        const SAIStimulusTypeDesc* desc = &m_stimulusTypes[i];
        std::vector<SStimulusRecord>& stimuli = m_stimuli[i];
        for (unsigned j = 0, nj = stimuli.size(); j < nj; ++j)
        {
            SStimulusRecord& s = stimuli[j];

            assert(s.subType < AI_MAX_SUBTYPES);
            float tmax = desc->duration[s.subType];
            float a = clamp_tpl((s.t - tmax / 2) / (tmax / 2), 0.0f, 1.0f);

            int row = 0;
            unsigned hash = HashFromVec3(s.pos, 0.1f, 1.0f / 0.1f);
            OccMap::iterator it = occupied.find(hash);
            if (it != occupied.end())
            {
                row = it->second;
                it->second++;
            }
            else
            {
                occupied[hash] = 1;
            }
            if (row > 5)
            {
                row = 5;
            }

            bool thrownByPlayer = pPlayer ? pPlayer->IsThrownByPlayer(s.sourceId) : false;

            ColorB color = thrownByPlayer ? ColorB(240, 16, 0) : GetColorFromId(i);
            ColorB colorTrans(color);
            color.a = 10 + (uint8)(240 * a);
            colorTrans.a = 10 + (uint8)(128 * a);

            char rowTxt[] = "\n\n\n\n\n\n\n";
            rowTxt[row] = '\0';

            char subTypeTxt[128] = "";
            switch (s.type)
            {
            case AISTIM_SOUND:
                switch (s.subType)
                {
                case AISOUND_GENERIC:
                    azsnprintf(subTypeTxt, 128, "GENERIC  R=%.1f", s.radius);
                    break;
                case AISOUND_COLLISION:
                    azsnprintf(subTypeTxt, 128, "COLLISION  R=%.1f", s.radius);
                    break;
                case AISOUND_COLLISION_LOUD:
                    azsnprintf(subTypeTxt, 128, "COLLISION LOUD  R=%.1f", s.radius);
                    break;
                case AISOUND_MOVEMENT:
                    azsnprintf(subTypeTxt, 128, "MOVEMENT  R=%.1f", s.radius);
                    break;
                case AISOUND_MOVEMENT_LOUD:
                    azsnprintf(subTypeTxt, 128, "MOVEMENT LOUD  R=%.1f", s.radius);
                    break;
                case AISOUND_WEAPON:
                    azsnprintf(subTypeTxt, 128, "WEAPON\nR=%.1f", s.radius);
                    break;
                case AISOUND_EXPLOSION:
                    azsnprintf(subTypeTxt, 128, "EXPLOSION  R=%.1f", s.radius);
                    break;
                }
                break;
            case AISTIM_COLLISION:
                switch (s.subType)
                {
                case AICOL_SMALL:
                    azsnprintf(subTypeTxt, 128, "SMALL  R=%.1f", s.radius);
                    break;
                case AICOL_MEDIUM:
                    azsnprintf(subTypeTxt, 128, "MEDIUM  R=%.1f", s.radius);
                    break;
                case AICOL_LARGE:
                    azsnprintf(subTypeTxt, 128, "LARGE  R=%.1f", s.radius);
                    break;
                }
                ;
                break;
            case AISTIM_EXPLOSION:
                azsnprintf(subTypeTxt, 128, "R=%.1f", s.radius);
                break;
            case AISTIM_BULLET_WHIZZ:
                azsnprintf(subTypeTxt, 128, "R=%.1f", s.radius);
                break;
            case AISTIM_BULLET_HIT:
                azsnprintf(subTypeTxt, 128, "R=%.1f", s.radius);
                break;
            case AISTIM_GRENADE:
                azsnprintf(subTypeTxt, 128, "R=%.1f", s.radius);
                break;
            }
            ;

            Vec3 ext(0, 0, s.radius);
            if (s.dir.GetLengthSquared() > 0.1f)
            {
                ext = s.dir * s.radius;
            }

            dc->DrawSphere(s.pos, 0.15f, colorTrans);
            dc->DrawLine(s.pos, color, s.pos + ext, color);
            dc->DrawWireSphere(s.pos, s.radius, color);
            dc->Draw3dLabel(s.pos, 1.1f, "%s%s  %s", rowTxt, desc->name, subTypeTxt);
        }
    }
}

//===================================================================
// DebugDrawPerformance
//===================================================================
void CPerceptionManager::DebugDrawPerformance(int mode)
{
    CDebugDrawContext dc;
    ColorB  white(255, 255, 255);

    static std::vector<Vec3> points;

    if (mode == 1)
    {
        // Draw visibility performance

        const int visChecks = m_stats.trackers[PERFTRACK_VIS_CHECKS].GetCount();
        const int visChecksMax = m_stats.trackers[PERFTRACK_VIS_CHECKS].GetCountMax();
        const int updates = m_stats.trackers[PERFTRACK_UPDATES].GetCount();
        const int updatesMax = m_stats.trackers[PERFTRACK_UPDATES].GetCountMax();

        dc->Draw2dLabel(50, 200, 2, white, false, "Updates:");
        dc->Draw2dLabel(175, 200, 2, white, false, "%d", updates);
        dc->Draw2dLabel(215, 200 + 5, 1, white, false, "max:%d", updatesMax);

        dc->Draw2dLabel(50, 225, 2, white, false, "Vis checks:");
        dc->Draw2dLabel(175, 225, 2, white, false, "%d", visChecks);
        dc->Draw2dLabel(215, 225 + 5, 1, white, false, "max:%d", visChecksMax);

        {
            dc->Init2DMode();
            dc->SetAlphaBlended(true);
            dc->SetBackFaceCulling(false);
            dc->SetDepthTest(false);

            float   rw = (float) dc->GetWidth();
            float   rh = (float) dc->GetHeight();
            //      float   as = rh/rw;
            float scale = 1.0f / rh;

            // Divider lines every 10 units.
            for (unsigned i = 0; i <= 10; ++i)
            {
                int v = i * 10;
                dc->DrawLine(Vec3(0.1f, 0.9f - v * scale, 0), ColorB(255, 255, 255, 128), Vec3(0.9f, 0.9f - v * scale, 0), ColorB(255, 255, 255, 128));
                dc->Draw2dLabel(0.1f * rw - 20, rh * 0.9f - v * scale * rh - 6, 1.0f, white, false, "%d", i * 10);
            }

            int idx[2] = {PERFTRACK_UPDATES, PERFTRACK_VIS_CHECKS};
            ColorB colors[3] = { ColorB(255, 0, 0), ColorB(0, 196, 255), ColorB(255, 255, 255) };

            for (int i = 0; i < 2; ++i)
            {
                const CValueHistory<int>& hist = m_stats.trackers[idx[i]].GetHist();
                unsigned n = hist.GetSampleCount();
                if (!n)
                {
                    continue;
                }

                points.resize(n);

                for (unsigned j = 0; j < n; ++j)
                {
                    float t = (float)j / (float)hist.GetMaxSampleCount();
                    points[j].x = 0.1f + t * 0.8f;
                    points[j].y = 0.9f - hist.GetSample(j) * scale;
                    points[j].z = 0;
                }
                dc->DrawPolyline(&points[0], n, false, colors[i]);
            }
        }

        ActorLookUp& lookUp = *gAIEnv.pActorLookUp;
        size_t activeActorCount = lookUp.GetActiveCount();

        for (size_t actorIndex = 0; actorIndex < activeActorCount; ++actorIndex)
        {
            CAIActor* pAIActor = lookUp.GetActor<CAIActor>(actorIndex);
            dc->DrawSphere(lookUp.GetPosition(actorIndex), 1.0f, ColorB(255, 0, 0));
        }
    }
    else if (mode == 2)
    {
        // Draw stims performance

        const int incoming = m_stats.trackers[PERFTRACK_INCOMING_STIMS].GetCount();
        const int incomingMax = m_stats.trackers[PERFTRACK_INCOMING_STIMS].GetCountMax();
        const int stims = m_stats.trackers[PERFTRACK_STIMS].GetCount();
        const int stimsMax = m_stats.trackers[PERFTRACK_STIMS].GetCountMax();

        dc->Draw2dLabel(50, 200, 2, white, false, "Incoming:");
        dc->Draw2dLabel(175, 200, 2, white, false, "%d", incoming);
        dc->Draw2dLabel(215, 200 + 5, 1, white, false, "max:%d", incomingMax);

        dc->Draw2dLabel(50, 225, 2, white, false, "Stims:");
        dc->Draw2dLabel(175, 225, 2, white, false, "%d", stims);
        dc->Draw2dLabel(215, 225 + 5, 1, white, false, "max:%d", stimsMax);

        dc->Init2DMode();
        dc->SetAlphaBlended(true);
        dc->SetBackFaceCulling(false);
        dc->SetDepthTest(false);

        float   rw = (float) dc->GetWidth();
        float   rh = (float) dc->GetHeight();
        //      float   as = rh/rw;
        float scale = 1.0f / rh;

        // Divider lines every 10 units.
        for (unsigned i = 0; i <= 10; ++i)
        {
            int v = i * 10;
            dc->DrawLine(Vec3(0.1f, 0.9f - v * scale, 0), ColorB(255, 255, 255, 128), Vec3(0.9f, 0.9f - v * scale, 0), ColorB(255, 255, 255, 128));
            dc->Draw2dLabel(0.1f * rw - 20, rh * 0.9f - v * scale * rh - 6, 1.0f, white, false, "%d", i * 10);
        }

        int idx[2] = {PERFTRACK_INCOMING_STIMS, PERFTRACK_STIMS};
        ColorB colors[2] = { ColorB(0, 196, 255), ColorB(255, 255, 255) };

        for (int i = 0; i < 2; ++i)
        {
            const CValueHistory<int>& hist = m_stats.trackers[idx[i]].GetHist();
            unsigned n = hist.GetSampleCount();
            if (!n)
            {
                continue;
            }

            points.resize(n);

            for (unsigned j = 0; j < n; ++j)
            {
                float t = (float)j / (float)hist.GetMaxSampleCount();
                points[j].x = 0.1f + t * 0.8f;
                points[j].y = 0.9f - hist.GetSample(j) * scale;
                points[j].z = 0;
            }
            dc->DrawPolyline(&points[0], n, false, colors[i]);
        }
    }
}


void CPerceptionManager::RegisterStimulus(const SAIStimulus& stim)
{
    // Check if we should ignore stimulus from the source.
    StimulusIgnoreMap& ignore = m_ignoreStimuliFrom[stim.type];
    if (ignore.find(stim.sourceId) != ignore.end())
    {
        return;
    }

    //  const SAIStimulusTypeDesc* desc = &m_stimulusTypes[stim.type];
    m_incomingStimuli.resize(m_incomingStimuli.size() + 1);
    SAIStimulus& is = m_incomingStimuli.back();
    is = stim;

    //  m_maxIncomingPerUpdate = max(m_maxIncomingPerUpdate, (int)m_incomingStimuli.size());
}

//===================================================================
// IgnoreStimulusFrom
//===================================================================
void CPerceptionManager::IgnoreStimulusFrom(EntityId sourceId, EAIStimulusType type, float time)
{
    StimulusIgnoreMap& ignore = m_ignoreStimuliFrom[(int)type];
    StimulusIgnoreMap::iterator it = ignore.find(sourceId);
    if (it != ignore.end())
    {
        it->second = max(it->second, time);
    }
    else
    {
        ignore[sourceId] = time;
    }
}


//===================================================================
// HandleSound
//===================================================================
void CPerceptionManager::HandleSound(const SStimulusRecord& stim)
{
    FUNCTION_PROFILER(gEnv->pSystem, PROFILE_AI);

    ActorLookUp& lookUp = *gAIEnv.pActorLookUp;
    lookUp.Prepare(ActorLookUp::EntityID);

    IEntity* pSourceEntity = gEnv->pEntitySystem->GetEntity(stim.sourceId);
    IEntity* pTargetEntity = gEnv->pEntitySystem->GetEntity(stim.targetId);

    CAIActor* pSourceAI = pSourceEntity ? CastToCAIActorSafe(pSourceEntity->GetAI()) : NULL;

    // If this is a collision sound
    if (stim.type == AISOUND_COLLISION || stim.type == AISOUND_COLLISION_LOUD)
    {
        CAIActor* pTargetAI = pTargetEntity ? CastToCAIActorSafe(pTargetEntity->GetAI()) : NULL;

        // Do not report collision sounds between two AI
        if (pSourceAI || pTargetAI)
        {
            return;
        }
    }

    // perform suppression of the sound from any nearby suppressors
    float rad = SupressSound(stim.pos, stim.radius);

    EntityId throwingEntityID = 0;

    // If the sound events comes from an object thrown by the player
    if (CAIPlayer* pPlayer = CastToCAIPlayerSafe(GetAISystem()->GetPlayer()))
    {
        if (pPlayer->IsThrownByPlayer(stim.sourceId))
        {
            // Record the thrower (to use as targetID - required for target tracking)
            throwingEntityID = pPlayer->GetEntityID();
        }
    }

    size_t activeCount = lookUp.GetActiveCount();

    for (size_t actorIndex = 0; actorIndex < activeCount; ++actorIndex)
    {
        CAIActor* pReceiverAIActor = lookUp.GetActor<CAIActor>(actorIndex);

        // Do not report sounds to the sound source.
        if (lookUp.GetEntityID(actorIndex) == stim.sourceId)
        {
            continue;
        }

        if (!pReceiverAIActor->IsPerceptionEnabled())
        {
            continue;
        }

        if (pSourceAI)
        {
            const bool isHostile = pSourceAI->IsHostile(pReceiverAIActor);

            // Ignore impact or explosion sounds from the same species and group
            switch (stim.subType)
            {
            case AISOUND_COLLISION:
            case AISOUND_COLLISION_LOUD:
            case AISOUND_EXPLOSION:

                if (!isHostile && pReceiverAIActor->GetGroupId() == pSourceAI->GetGroupId())
                {
                    continue;
                }
            }

            // No vehicle sounds for same species - destructs convoys
            if (!isHostile && pSourceAI->GetType() == AIOBJECT_VEHICLE)
            {
                continue;
            }
        }

        // The sound is occluded because of the buildings.
        if (IsSoundOccluded(pReceiverAIActor, stim.pos))
        {
            continue;
        }

        // Send event.
        SAIEVENT event;
        event.vPosition = stim.pos;
        event.fThreat = rad;
        event.nType = (int)stim.subType;
        event.nFlags = stim.flags;
        event.sourceId = throwingEntityID ? throwingEntityID : (pSourceAI ? pSourceAI->GetPerceivedEntityID() : stim.sourceId);
        pReceiverAIActor->Event(AIEVENT_ONSOUNDEVENT, &event);

        RecordStimulusEvent(stim, event.fThreat, *pReceiverAIActor);
    }
}

//===================================================================
// HandleCollisionEvent
//===================================================================
void CPerceptionManager::HandleCollision(const SStimulusRecord& stim)
{
    FUNCTION_PROFILER(gEnv->pSystem, PROFILE_AI);

    ActorLookUp& lookUp = *gAIEnv.pActorLookUp;
    lookUp.Prepare(ActorLookUp::Position);

    IEntity* pCollider = gEnv->pEntitySystem->GetEntity(stim.sourceId);
    IEntity* pTarget = gEnv->pEntitySystem->GetEntity(stim.targetId);

    CAIActor* pColliderAI = pCollider ? CastToCAIActorSafe(pCollider->GetAI()) : 0;
    CAIActor* pTargetAI = pTarget ? CastToCAIActorSafe(pTarget->GetAI()) : 0;

    // Do not report AI collisions
    if (pColliderAI || pTargetAI)
    {
        return;
    }

    bool thrownByPlayer = false;
    if (pCollider)
    {
        CAIPlayer* pPlayer = CastToCAIPlayerSafe(GetAISystem()->GetPlayer());
        if (pPlayer)
        {
            thrownByPlayer = pPlayer->IsThrownByPlayer(pCollider->GetId());
        }
    }

    // Allow to react to larger objects which collide nearby.
    size_t activeCount = lookUp.GetActiveCount();

    for (size_t actorIndex = 0; actorIndex < activeCount; ++actorIndex)
    {
        CAIActor* pReceiverAIActor = lookUp.GetActor<CAIActor>(actorIndex);

        const float scale = pReceiverAIActor->GetParameters().m_PerceptionParams.collisionReactionScale;
        const float rangeSq = sqr(stim.radius * scale);

        float distSq = Distance::Point_PointSq(stim.pos, lookUp.GetPosition(actorIndex));
        if (distSq > rangeSq)
        {
            continue;
        }

        if (!pReceiverAIActor->IsPerceptionEnabled())
        {
            continue;
        }

        IAISignalExtraData* pData = GetAISystem()->CreateSignalExtraData();
        pData->iValue = thrownByPlayer ? 1 : 0;
        pData->fValue = sqrtf(distSq);
        pData->point = stim.pos;
        pReceiverAIActor->SetSignal(0, "OnCloseCollision", 0, pData);

        if (thrownByPlayer)
        {
            if (CPuppet* pReceiverPuppet = pReceiverAIActor->CastToCPuppet())
            {
                pReceiverPuppet->SetAlarmed();
            }
        }

        RecordStimulusEvent(stim, 0.0f, *pReceiverAIActor);
    }
}

//===================================================================
// HandleExplosion
//===================================================================
void CPerceptionManager::HandleExplosion(const SStimulusRecord& stim)
{
    FUNCTION_PROFILER(gEnv->pSystem, PROFILE_AI);

    ActorLookUp& lookUp = *gAIEnv.pActorLookUp;
    lookUp.Prepare(ActorLookUp::Position);

    // React to explosions.
    size_t activeCount = lookUp.GetActiveCount();

    for (size_t actorIndex = 0; actorIndex < activeCount; ++actorIndex)
    {
        CAIActor* pReceiverAIActor = lookUp.GetActor<CAIActor>(actorIndex);

        const float scale = pReceiverAIActor->GetParameters().m_PerceptionParams.collisionReactionScale;
        const float rangeSq = sqr(stim.radius * scale);

        float distSq = Distance::Point_PointSq(stim.pos, lookUp.GetPosition(actorIndex));
        if (distSq > rangeSq)
        {
            continue;
        }

        if (!pReceiverAIActor->IsPerceptionEnabled())
        {
            continue;
        }

        if (stim.flags & AISTIMPROC_ONLY_IF_VISIBLE)
        {
            if (!IsStimulusVisible(stim, pReceiverAIActor))
            {
                continue;
            }
        }

        IAISignalExtraData* pData = GetAISystem()->CreateSignalExtraData();
        pData->point = stim.pos;

        SetLastExplosionPosition(stim.pos, pReceiverAIActor);
        pReceiverAIActor->SetSignal(0, "OnExposedToExplosion", 0, pData);

        if (CPuppet* pReceiverPuppet = pReceiverAIActor->CastToCPuppet())
        {
            pReceiverPuppet->SetAlarmed();
        }

        RecordStimulusEvent(stim, 0.0f, *pReceiverAIActor);
    }
}

//===================================================================
// HandleBulletHit
//===================================================================
void CPerceptionManager::HandleBulletHit(const SStimulusRecord& stim)
{
    FUNCTION_PROFILER(gEnv->pSystem, PROFILE_AI);

    ActorLookUp& lookUp = *gAIEnv.pActorLookUp;
    lookUp.Prepare(ActorLookUp::Position | ActorLookUp::EntityID);

    IEntity* pShooterEnt = gEnv->pEntitySystem->GetEntity(stim.sourceId);
    CAIActor* pShooterActor = NULL;
    if (pShooterEnt)
    {
        pShooterActor = CastToCAIActorSafe(pShooterEnt->GetAI());
    }

    // Send bullet events
    size_t activeCount = lookUp.GetActiveCount();

    for (size_t actorIndex = 0; actorIndex < activeCount; ++actorIndex)
    {
        CAIActor* pReceiverAIActor = lookUp.GetActor<CAIActor>(actorIndex);

        // Skip own fire.
        if (stim.sourceId == lookUp.GetEntityID(actorIndex))
        {
            continue;
        }

        if (!pReceiverAIActor->IsPerceptionEnabled())
        {
            continue;
        }

        // Skip non-hostile bullets.
        if (!pReceiverAIActor->IsHostile(pShooterActor))
        {
            continue;
        }

        AABB bounds;
        pReceiverAIActor->GetEntity()->GetWorldBounds(bounds);
        const float d = Distance::Point_AABBSq(stim.pos, bounds);
        const float r = max(stim.radius, pReceiverAIActor->m_Parameters.m_PerceptionParams.bulletHitRadius);

        if (d < sqr(r))
        {
            EntityId sourceId = 0;
            if (pShooterActor)
            {
                sourceId = pShooterActor->GetPerceivedEntityID();
            }
            else if (pShooterEnt)
            {
                sourceId = pShooterEnt->GetId();
            }

            // Send event.
            SAIEVENT event;
            event.sourceId = sourceId;
            if (pShooterActor)
            {
                event.vPosition = pShooterActor->GetFirePos();
            }
            else if (pShooterEnt)
            {
                event.vPosition = pShooterEnt->GetWorldPos();
            }
            else
            {
                event.vPosition = stim.pos;
            }
            event.vStimPos = stim.pos;
            event.nFlags = stim.flags;
            event.fThreat = 1.0f; // pressureMultiplier

            CPuppet* pActorPuppet = CastToCPuppetSafe(pShooterActor);
            if (pActorPuppet)
            {
                event.fThreat = pActorPuppet->GetCurrentWeaponDescriptor().pressureMultiplier;
            }

            pReceiverAIActor->Event(AIEVENT_ONBULLETRAIN, &event);

            RecordStimulusEvent(stim, 0.0f, *pReceiverAIActor);
        }
    }
}

//===================================================================
// HandleBulletWhizz
//===================================================================
void CPerceptionManager::HandleBulletWhizz(const SStimulusRecord& stim)
{
    FUNCTION_PROFILER(gEnv->pSystem, PROFILE_AI);

    ActorLookUp& lookUp = *gAIEnv.pActorLookUp;
    lookUp.Prepare(ActorLookUp::Position | ActorLookUp::EntityID);

    IEntity* pShooterEnt = gEnv->pEntitySystem->GetEntity(stim.sourceId);
    IAIObject* pShooterAI = pShooterEnt ? pShooterEnt->GetAI() : 0;

    Lineseg lof(stim.pos, stim.pos + stim.dir * stim.radius);
    float t;

    size_t activeCount = lookUp.GetActiveCount();

    for (size_t actorIndex = 0; actorIndex < activeCount; ++actorIndex)
    {
        CAIActor* pReceiverAIActor = lookUp.GetActor<CAIActor>(actorIndex);

        // Skip own fire.
        if (lookUp.GetEntityID(actorIndex) == stim.sourceId)
        {
            continue;
        }

        const float r = 5.0f;

        float d = Distance::Point_LinesegSq(lookUp.GetPosition(actorIndex), lof, t);
        if (d < sqr(r))
        {
            if (!pReceiverAIActor->IsPerceptionEnabled())
            {
                continue;
            }

            // Skip non-hostile bullets.
            if (!pReceiverAIActor->IsHostile(pShooterAI))
            {
                continue;
            }

            const Vec3 hitPos = lof.GetPoint(t);
            SAIStimulus hitStim(AISTIM_BULLET_HIT, 0, stim.sourceId, 0, hitPos, ZERO, r);
            RegisterStimulus(hitStim);
        }

        RecordStimulusEvent(stim, 0.0f, *pReceiverAIActor);
    }
}

//===================================================================
// HandleGrenade
//===================================================================
void CPerceptionManager::HandleGrenade(const SStimulusRecord& stim)
{
    FUNCTION_PROFILER(gEnv->pSystem, PROFILE_AI);

    EntityId shooterId = stim.sourceId;

    IEntity* pShooter = gEnv->pEntitySystem->GetEntity(shooterId);
    if (!pShooter)
    {
        return;
    }

    ActorLookUp& lookUp = *gAIEnv.pActorLookUp;
    lookUp.Prepare(ActorLookUp::Position | ActorLookUp::EntityID);

    CAIActor* pShooterActor = 0;
    if (pShooter)
    {
        pShooterActor = CastToCAIActorSafe(pShooter->GetAI());
    }

    switch (stim.subType)
    {
    case AIGRENADE_THROWN:
    {
        float radSq = sqr(stim.radius);
        Vec3 throwPos = pShooterActor ? pShooterActor->GetFirePos() : stim.pos;     // Grenade position
        // Inform the AI that sees the throw
        size_t activeCount = lookUp.GetActiveCount();

        for (size_t actorIndex = 0; actorIndex < activeCount; ++actorIndex)
        {
            CAIActor* pAIActor = lookUp.GetActor<CAIActor>(actorIndex);
            if (lookUp.GetEntityID(actorIndex) == shooterId)
            {
                continue;
            }

            const Vec3 vAIActorPos = lookUp.GetPosition(actorIndex);

            // If the puppet is not close to the predicted position, skip.
            if (Distance::Point_PointSq(stim.pos, vAIActorPos) > radSq)
            {
                continue;
            }

            if (!pAIActor->IsPerceptionEnabled())
            {
                continue;
            }

            // Inform enemies only.
            const bool isShootherFriendly = pShooterActor ? !pShooterActor->IsHostile(pAIActor) : false;
            if (isShootherFriendly)
            {
                continue;
            }

            // Only sense grenades that are on front of the AI and visible when thrown.
            // Another signal is sent when the grenade hits the ground.
            Vec3 delta = throwPos - vAIActorPos;        // grenade to AI
            float dist = delta.NormalizeSafe();
            const float thr = cosf(DEG2RAD(160.0f));
            if (delta.Dot(pAIActor->GetViewDir()) > thr)
            {
                static const int objTypes = ent_static | ent_terrain | ent_rigid | ent_sleeping_rigid;
                static const unsigned int flags = rwi_stop_at_pierceable | rwi_colltype_any;
                const RayCastResult& result = gAIEnv.pRayCaster->Cast(RayCastRequest(vAIActorPos, delta * dist, objTypes, flags));

                if (result)
                {
                    throwPos = result[0].pt;
                }

                GetAISystem()->AddDebugLine(vAIActorPos, throwPos, 255, 0, 0, 15.0f);

                if (!result || result[0].dist > dist * 0.9f)
                {
                    IAISignalExtraData* pEData = GetAISystem()->CreateSignalExtraData();        // no leak - this will be deleted inside SendAnonymousSignal
                    pEData->point = stim.pos;     // avoid predicted pos
                    pEData->nID = shooterId;
                    pEData->iValue = 1;

                    SetLastExplosionPosition(stim.pos, pAIActor);
                    GetAISystem()->SendSignal(SIGNALFILTER_SENDER, 1, "OnGrenadeDanger", pAIActor, pEData);

                    RecordStimulusEvent(stim, 0.0f, *pAIActor);
                }
            }
        }
    }
    break;
    case AIGRENADE_COLLISION:
    {
        float radSq = sqr(stim.radius);

        size_t activeCount = lookUp.GetActiveCount();

        for (size_t actorIndex = 0; actorIndex < activeCount; ++actorIndex)
        {
            CAIActor* pAIActor = lookUp.GetActor<CAIActor>(actorIndex);

            // If the puppet is not close to the grenade, skip.
            if (Distance::Point_PointSq(stim.pos, lookUp.GetPosition(actorIndex)) > radSq)
            {
                continue;
            }

            IAISignalExtraData* pEData = GetAISystem()->CreateSignalExtraData();        // no leak - this will be deleted inside SendAnonymousSignal
            pEData->point = stim.pos;
            pEData->nID = shooterId;
            pEData->iValue = 2;

            SetLastExplosionPosition(stim.pos, pAIActor);
            GetAISystem()->SendSignal(SIGNALFILTER_SENDER, 1, "OnGrenadeDanger", pAIActor, pEData);

            RecordStimulusEvent(stim, 0.0f, *pAIActor);
        }
    }
    break;
    case AIGRENADE_FLASH_BANG:
    {
        float radSq = sqr(stim.radius);

        size_t activeCount = lookUp.GetActiveCount();

        for (size_t actorIndex = 0; actorIndex < activeCount; ++actorIndex)
        {
            CAIActor* pAIActor = lookUp.GetActor<CAIActor>(actorIndex);
            if (lookUp.GetEntityID(actorIndex) == shooterId)
            {
                continue;
            }

            const Vec3 vAIActorPos = lookUp.GetPosition(actorIndex);

            // If AI Actor is not close to the flash, skip.
            if (Distance::Point_PointSq(stim.pos, vAIActorPos) > radSq)
            {
                continue;
            }

            // Only sense grenades that are on front of the AI and visible when thrown.
            // Another signal is sent when the grenade hits the ground.
            Vec3 delta = stim.pos - vAIActorPos;        // grenade to AI
            float dist = delta.NormalizeSafe();
            const float thr = cosf(DEG2RAD(160.0f));
            if (delta.Dot(pAIActor->GetViewDir()) > thr)
            {
                static const int objTypes = ent_static | ent_terrain | ent_rigid | ent_sleeping_rigid;
                static const unsigned int flags = rwi_stop_at_pierceable | rwi_colltype_any;

                if (!gAIEnv.pRayCaster->Cast(RayCastRequest(vAIActorPos, delta * dist, objTypes, flags)))
                {
                    IAISignalExtraData* pExtraData = GetAISystem()->CreateSignalExtraData();
                    pExtraData->iValue = 0;
                    GetAISystem()->SendSignal(SIGNALFILTER_SENDER, 1, "OnExposedToFlashBang", pAIActor, pExtraData);

                    RecordStimulusEvent(stim, 0.0f, *pAIActor);
                }
            }
        }
    }
    break;
    case AIGRENADE_SMOKE:
    {
        float radSq = sqr(stim.radius);
        size_t activeCount = lookUp.GetActiveCount();

        for (size_t actorIndex = 0; actorIndex < activeCount; ++actorIndex)
        {
            CAIActor* pAIActor = lookUp.GetActor<CAIActor>(actorIndex);

            // If the puppet is not close to the smoke, skip.
            if (Distance::Point_PointSq(stim.pos, lookUp.GetPosition(actorIndex)) > radSq)
            {
                continue;
            }

            GetAISystem()->SendSignal(SIGNALFILTER_SENDER, 1, "OnExposedToSmoke", pAIActor);

            RecordStimulusEvent(stim, 0.0f, *pAIActor);
        }
    }
    break;
    default:
        break;
    }
}

void CPerceptionManager::SetLastExplosionPosition(const Vec3& position, CAIActor* pAIActor) const
{
    assert(pAIActor);
    if (pAIActor)
    {
        IScriptTable* pActorScriptTable = pAIActor->GetEntity()->GetScriptTable();
        Vec3 tempPos(ZERO);
        if (pActorScriptTable->GetValue("lastExplosiveThreatPos", tempPos))
        {
            pActorScriptTable->SetValue("lastExplosiveThreatPos", position);
        }
    }
}

//===================================================================
// RegisterAIEventListener
//===================================================================
void CPerceptionManager::RegisterAIEventListener(IAIEventListener* pListener, const Vec3& pos, float rad, int flags)
{
    if (!pListener)
    {
        return;
    }

    // Check if the listener exists
    for (unsigned i = 0, ni = m_eventListeners.size(); i < ni; ++i)
    {
        SAIEventListener& listener = m_eventListeners[i];
        if (listener.pListener == pListener)
        {
            listener.pos = pos;
            listener.radius = rad;
            listener.flags = flags;
            return;
        }
    }

    m_eventListeners.resize(m_eventListeners.size() + 1);
    SAIEventListener& listener = m_eventListeners.back();
    listener.pListener = pListener;
    listener.pos = pos;
    listener.radius = rad;
    listener.flags = flags;
}

//===================================================================
// UnregisterAIEventListener
//===================================================================
void CPerceptionManager::UnregisterAIEventListener(IAIEventListener* pListener)
{
    if (!pListener)
    {
        return;
    }

    // Check if the listener exists
    for (unsigned i = 0, ni = m_eventListeners.size(); i < ni; ++i)
    {
        SAIEventListener& listener = m_eventListeners[i];
        if (listener.pListener == pListener)
        {
            m_eventListeners[i] = m_eventListeners.back();
            m_eventListeners.pop_back();
            return;
        }
    }
}

//===================================================================
// IsPointInRadiusOfStimulus
//===================================================================
bool CPerceptionManager::IsPointInRadiusOfStimulus(EAIStimulusType type, const Vec3& pos) const
{
    const std::vector<SStimulusRecord>& stims = m_stimuli[(int)type];
    for (unsigned i = 0, ni = stims.size(); i < ni; ++i)
    {
        const SStimulusRecord& s = stims[i];
        if (Distance::Point_PointSq(s.pos, pos) < sqr(s.radius))
        {
            return true;
        }
    }
    return false;
}

//===================================================================
// NotifyAIEventListeners
//===================================================================
void CPerceptionManager::NotifyAIEventListeners(const SStimulusRecord& stim, float threat)
{
    FUNCTION_PROFILER(gEnv->pSystem, PROFILE_AI);

    int flags = 1 << (int)stim.type;

    for (unsigned i = 0, ni = m_eventListeners.size(); i < ni; ++i)
    {
        SAIEventListener& listener = m_eventListeners[i];
        if ((listener.flags & flags) != 0 && Distance::Point_PointSq(stim.pos, listener.pos) < sqr(listener.radius + stim.radius))
        {
            listener.pListener->OnAIEvent((EAIStimulusType)stim.type, stim.pos, stim.radius, threat, stim.sourceId);
        }
    }
}

//===================================================================
// SupressSound
//===================================================================
float CPerceptionManager::SupressSound(const Vec3& pos, float radius)
{
    float minScale = 1.0f;
    AIObjectOwners::iterator ai = gAIEnv.pAIObjectManager->m_Objects.find(AIOBJECT_SNDSUPRESSOR), aiend = gAIEnv.pAIObjectManager->m_Objects.end();
    for (; ai != aiend; ++ai)
    {
        if (ai->first != AIOBJECT_SNDSUPRESSOR)
        {
            break;
        }

        CAIObject* obj = ai->second.GetAIObject();
        if (!obj->IsEnabled())
        {
            continue;
        }

        const float r = obj->GetRadius();
        const float silenceRadius = r * 0.3f;
        const float distSqr = Distance::Point_PointSq(pos, obj->GetPos());

        if (distSqr > sqr(r))
        {
            continue;
        }

        if (distSqr < sqr(silenceRadius))
        {
            return 0.0f;
        }

        const float dist = sqrtf(distSqr);
        const float scale = (dist - silenceRadius) / (r - silenceRadius);
        minScale = min(minScale, scale);
    }

    return radius * minScale;
}

//===================================================================
// IsSoundOccluded
//===================================================================
bool CPerceptionManager::IsSoundOccluded(CAIActor* pAIActor, const Vec3& vSoundPos)
{
    FUNCTION_PROFILER(gEnv->pSystem, PROFILE_AI);

    // TODO: Check if this function is necessary at all!

    int nBuildingIDPuppet = -1, nBuildingIDSound = -1;

    Vec3 actorPos(pAIActor->GetPos());

    gAIEnv.pNavigation->CheckNavigationType(actorPos, nBuildingIDPuppet, pAIActor->m_movementAbility.pathfindingProperties.navCapMask);
    gAIEnv.pNavigation->CheckNavigationType(vSoundPos, nBuildingIDSound, pAIActor->m_movementAbility.pathfindingProperties.navCapMask);

    // Sound and puppet both are outdoor, not occluded.
    if (nBuildingIDPuppet < 0 && nBuildingIDSound < 0)
    {
        return false;
    }

    if (nBuildingIDPuppet != nBuildingIDSound)
    {
        if (nBuildingIDPuppet < 0)
        {
            // Puppet is outdoors, sound indoors
            if (IVisArea* pAreaSound = gEnv->p3DEngine->GetVisAreaFromPos(vSoundPos))
            {
                // Occluded if the area is not connected to outdoors.
                return !pAreaSound->IsConnectedToOutdoor();
            }
        }
        else if (nBuildingIDSound < 0)
        {
            // Sound is outdoors, puppet indoors
            if (IVisArea* pAreaPuppet = gEnv->p3DEngine->GetVisAreaFromPos(actorPos))
            {
                // Occluded if the area is not connected to outdoors.
                return !pAreaPuppet->IsConnectedToOutdoor();
            }
        }
        else
        {
            // If in two different buildings we cannot hear the sound for sure.
            // Require one building to have visarea, though.
            if (gEnv->p3DEngine->GetVisAreaFromPos(actorPos) || gEnv->p3DEngine->GetVisAreaFromPos(vSoundPos))
            {
                return true;
            }
        }
    }

    // Not occluded
    return false;
}

//===================================================================
// RayOcclusionPlaneIntersection
//===================================================================
int CPerceptionManager::RayOcclusionPlaneIntersection(const Vec3& start, const Vec3& end)
{
    const ShapeMap& occPlanes = GetAISystem()->GetOcclusionPlanes();

    if (occPlanes.empty())
    {
        return 0;
    }

    ShapeMap::const_iterator di = occPlanes.begin(), diend = occPlanes.end();
    for (; di != diend; ++di)
    {
        if (!di->second.shape.empty())
        {
            float fShapeHeight = ((di->second.shape).front()).z;
            if ((start.z < fShapeHeight) && (end.z < fShapeHeight))
            {
                continue;
            }
            if ((start.z > fShapeHeight) && (end.z > fShapeHeight))
            {
                continue;
            }

            // find out where ray hits horizontal plane fShapeHeigh (with a nasty hack)
            Vec3 vIntersection;
            float t = (start.z - fShapeHeight) / (start.z - end.z);
            vIntersection = start + t * (end - start);


            // is it inside the polygon?
            if (Overlap::Point_Polygon2D(vIntersection, di->second.shape, &di->second.aabb))
            {
                return 1;
            }
        }
    }

    return 0;
}

//===================================================================
// Serialize
//===================================================================
void CPerceptionManager::Serialize(TSerialize ser)
{
    char name[64];
    ser.BeginGroup("PerceptionManager");
    for (unsigned i = 0; i < AI_MAX_STIMULI; ++i)
    {
        azsnprintf(name, 64, "m_stimuli%02d", i);
        ser.Value(name, m_stimuli[i]);
        azsnprintf(name, 64, "m_ignoreStimuliFrom%02d", i);
        ser.Value(name, m_ignoreStimuliFrom[i]);
    }
    ser.EndGroup();
}

//===================================================================
// RecordStimulusEvent
//===================================================================
void CPerceptionManager::RecordStimulusEvent(const SStimulusRecord& stim, float fRadius, IAIObject& receiver) const
{
#ifdef CRYAISYSTEM_DEBUG
    stack_string sType = g_szAIStimulusType[stim.type];
    if (stim.type == AISTIM_SOUND)
    {
        sType += g_szAISoundStimType[stim.subType];
    }
    else if (stim.type == AISTIM_GRENADE)
    {
        sType += g_szAIGrenadeStimType[stim.subType];
    }

    IEntity* pSource = gEnv->pEntitySystem->GetEntity(stim.sourceId);
    IEntity* pTarget = gEnv->pEntitySystem->GetEntity(stim.targetId);
    stack_string sDebugLine;
    sDebugLine.Format("%s from %s to %s", sType.c_str(), pSource ? pSource->GetName() : "Unknown", pTarget ? pTarget->GetName() : "All");

    // Record event
    if (gEnv->pAISystem->IsRecording(&receiver, IAIRecordable::E_REGISTERSTIMULUS))
    {
        gEnv->pAISystem->Record(&receiver, IAIRecordable::E_REGISTERSTIMULUS, sDebugLine.c_str());
    }

    IAIRecordable::RecorderEventData recorderEventData(sDebugLine.c_str());
    receiver.RecordEvent(IAIRecordable::E_REGISTERSTIMULUS, &recorderEventData);
#endif
}

bool CPerceptionManager::IsStimulusVisible(const SStimulusRecord& stim, const CAIActor* pAIActor)
{
    Vec3 stimPos = stim.pos;
    const Vec3 vAIActorPos = pAIActor->GetPos();
    Vec3 delta = stimPos - vAIActorPos;
    float dist = delta.NormalizeSafe();
    const float thr = cosf(DEG2RAD(110.0f));
    if (delta.dot(pAIActor->GetViewDir()) > thr)
    {
        static const int objTypes = ent_static | ent_terrain | ent_rigid | ent_sleeping_rigid;
        static const unsigned int flags = rwi_stop_at_pierceable | rwi_colltype_any;
        const RayCastResult& result = gAIEnv.pRayCaster->Cast(RayCastRequest(vAIActorPos, delta * dist, objTypes, flags));

        if (result)
        {
            stimPos = result[0].pt;
        }

        GetAISystem()->AddDebugLine(vAIActorPos, stimPos, 255, 0, 0, 15.0f);

        if (!result || result[0].dist > dist * 0.9f)
        {
            return true;
        }
    }

    return false;
}
