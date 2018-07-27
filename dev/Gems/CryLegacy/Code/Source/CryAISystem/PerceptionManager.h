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

#ifndef CRYINCLUDE_CRYAISYSTEM_PERCEPTIONMANAGER_H
#define CRYINCLUDE_CRYAISYSTEM_PERCEPTIONMANAGER_H
#pragma once

#include "ValueHistory.h"

struct SAIStimulusFilter
{
    inline void Set(unsigned char t, unsigned char st, EAIStimulusFilterMerge m, float s)
    {
        scale = s;
        merge = m;
        type = t;
        subType = st;
    }

    inline void Reset()
    {
        scale = 0.0f;
        merge = 0;
        type = 0;
        subType = 0;
    }

    float scale;                            // How much the existing stimulus radius is scaled before checking.
    unsigned char merge;            // Filter merge type, see EAIStimulusFilterMerge.
    unsigned char type;             // Stimulus type to match against.
    unsigned char subType;      // Stimulus subtype _mask_ to match against, or 0 if not applied.
};

// Stimulus descriptor.
// Defines a stimulus type and how the incoming stimuli of the specified type should be handled.
//
// Common stimuli defined in EAIStimulusType are already registered in the perception manager.
// For game specific stimuli, the first stim type should be LAST_AISTIM.
//
// Example:
//      desc.Reset();
//      desc.SetName("Collision");
//      desc.processDelay = 0.15f;
//      desc.duration[AICOL_SMALL] = 7.0f;
//      desc.duration[AICOL_MEDIUM] = 7.0f;
//      desc.duration[AICOL_LARGE] = 7.0f;
//      desc.filterTypes = (1<<AISTIM_COLLISION) | (1<<AISTIM_EXPLOSION);
//      desc.nFilters = 2;
//      desc.filters[0].Set(AISTIM_COLLISION, 0, AISTIMFILTER_MERGE_AND_DISCARD, 0.9f); // Merge nearby collisions
//      desc.filters[1].Set(AISTIM_EXPLOSION, 0, AISTIMFILTER_DISCARD, 2.5f); // Discard collision near explosions
//      pPerceptionMgr->RegisterStimulusDesc(AISTIM_COLLISION, desc);
//
struct SAIStimulusTypeDesc
{
    inline void SetName(const char* n)
    {
        assert(strlen(n) < sizeof(name));
        cry_strcpy(name, n);
    }

    inline void Reset()
    {
        processDelay = 0.0f;
        filterTypes = 0;
        for (int i = 0; i < AI_MAX_SUBTYPES; i++)
        {
            duration[i] = 0.0f;
        }
        for (int i = 0; i < AI_MAX_FILTERS; i++)
        {
            filters[i].Reset();
        }
        name[0] = '\0';
        nFilters = 0;
    }

    float processDelay;                             // Delay before the stimulus is actually sent.
    unsigned int filterTypes;                   // Mask of all types of filters contained in the filter list.
    float duration[AI_MAX_SUBTYPES];    // Duration of the stimulus, accessed by the subType of SAIStimulus.
    SAIStimulusFilter filters[AI_MAX_FILTERS];  // The filter list.
    char name[32];                                      // Name of the stimulus.
    unsigned char nFilters;                     // Number of filters in the filter list.
};

class CPerceptionManager
{
public:
    CPerceptionManager();
    ~CPerceptionManager();

    void Reset(IAISystem::EResetReason reason);
    void Update(float dt);
    void DebugDraw(int mode);
    void DebugDrawPerformance(int mode);
    void Serialize(TSerialize ser);

    bool UpdatePerception(CAIActor* pAIActor, std::vector<CAIObject*>& priorityTargets);

    void RegisterStimulus(const SAIStimulus& stim);
    void IgnoreStimulusFrom(EntityId sourceId, EAIStimulusType type, float time);
    bool IsPointInRadiusOfStimulus(EAIStimulusType type, const Vec3& pos) const;

    bool RegisterStimulusDesc(EAIStimulusType type, const SAIStimulusTypeDesc& desc);

    void RegisterAIEventListener(IAIEventListener* pListener, const Vec3& pos, float rad, int flags);
    void UnregisterAIEventListener(IAIEventListener* pListener);

private:
    struct SStimulusRecord
    {
        EntityId sourceId;
        EntityId targetId;
        Vec3 pos;
        Vec3 dir;
        float radius;
        float t;
        unsigned char type;
        unsigned char subType;
        unsigned char flags;
        unsigned char dispatched;

        void Serialize(TSerialize ser)
        {
            ser.BeginGroup("Stim");
            ser.Value("sourceId", sourceId);
            ser.Value("targetId", targetId);
            ser.Value("pos", pos);
            ser.Value("dir", dir);
            ser.Value("radius", radius);
            ser.Value("t", t);
            ser.Value("type", type);
            ser.Value("subType", subType);
            ser.Value("flags", flags);
            ser.Value("dispatched", dispatched);
            ser.EndGroup();
        }
    };

    struct SAIEventListener
    {
        IAIEventListener* pListener;
        Vec3 pos;
        float radius;
        int flags;
    };

    void HandleSound(const SStimulusRecord& stim);
    void HandleCollision(const SStimulusRecord& stim);
    void HandleExplosion(const SStimulusRecord& stim);
    void HandleBulletHit(const SStimulusRecord& stim);
    void HandleBulletWhizz(const SStimulusRecord& stim);
    void HandleGrenade(const SStimulusRecord& stim);
    void VisCheckBroadPhase(float dt);
    /// Checks if the sound is occluded.
    bool IsSoundOccluded(CAIActor* pAIActor, const Vec3& vSoundPos);
    /// Suppresses the sound radius based on sound suppressors.
    float SupressSound(const Vec3& pos, float radius);
    int RayOcclusionPlaneIntersection(const Vec3& start, const Vec3& end);
    void NotifyAIEventListeners(const SStimulusRecord& stim, float threat);
    void InitCommonTypeDescs();
    bool FilterStimulus(SAIStimulus* stim);

    /// Records a stimulus event to the AI recorder
    void RecordStimulusEvent(const SStimulusRecord& stim, float fRadius, IAIObject& receiver) const;
    void SetLastExplosionPosition(const Vec3& position, CAIActor* pAIActor) const;

    // We currently consider a stimulus visible if it's inbetween 160 degrees
    // left or right of the view direction (Similar to the grenades check)
    // and if the raycast to the stimulus position is successful
    bool IsStimulusVisible(const SStimulusRecord& stim, const CAIActor* pAIActor);


    SAIStimulusTypeDesc m_stimulusTypes[AI_MAX_STIMULI];
    std::vector<SStimulusRecord> m_stimuli[AI_MAX_STIMULI];
    std::vector<SAIStimulus> m_incomingStimuli;
    typedef std::map<EntityId, float> StimulusIgnoreMap;
    StimulusIgnoreMap m_ignoreStimuliFrom[AI_MAX_STIMULI];
    std::vector<SAIEventListener>   m_eventListeners;
    float m_visBroadPhaseDt;

    static std::vector<CAIObject*> s_targetEntities;
    static std::vector<CAIVehicle*> s_playerVehicles;

    static const int PERF_TRACKER_SAMPLE_COUNT = 200;

    class CPerfTracker
    {
        int m_count, m_countMax;
        CValueHistory<int> m_hist;
    public:
        inline CPerfTracker()
            : m_count(0)
            , m_countMax(0)
            , m_hist(PERF_TRACKER_SAMPLE_COUNT, 0) {}
        inline ~CPerfTracker() {}

        inline void Inc(int n = 1)
        {
            m_count += n;
        }

        inline void Update()
        {
            m_countMax = max(m_count, m_countMax);
            m_hist.Sample(m_count);
            m_count = 0;
        }

        inline void Reset()
        {
            m_count = m_countMax = 0;
            m_hist.Reset();
        }

        inline int GetCount() const { return m_count; }
        inline int GetCountMax() const { return m_countMax; }
        inline const CValueHistory<int>& GetHist() const { return m_hist; }
    };

    enum Trackers
    {
        PERFTRACK_VIS_CHECKS,
        PERFTRACK_UPDATES,
        PERFTRACK_INCOMING_STIMS,
        PERFTRACK_STIMS,
        COUNT_PERFTRACK // must be last
    };

    struct PerfStats
    {
        inline void Update()
        {
            for (unsigned i = 0; i < COUNT_PERFTRACK; ++i)
            {
                trackers[i].Update();
            }
        }

        CPerfTracker trackers[COUNT_PERFTRACK];
    };

    PerfStats m_stats;
};

#endif // CRYINCLUDE_CRYAISYSTEM_PERCEPTIONMANAGER_H
