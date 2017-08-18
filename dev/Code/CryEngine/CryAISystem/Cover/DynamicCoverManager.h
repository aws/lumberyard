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

#ifndef CRYINCLUDE_CRYAISYSTEM_COVER_DYNAMICCOVERMANAGER_H
#define CRYINCLUDE_CRYAISYSTEM_COVER_DYNAMICCOVERMANAGER_H
#pragma once


#include "Cover.h"
#include "EntityCoverSampler.h"

#include <HashGrid.h>


class DynamicCoverManager
    : public IEntityEventListener
{
    struct EntityCoverState;
public:
    struct ValidationSegment
    {
        ValidationSegment()
            : center(ZERO)
            , normal(ZERO)
            , height(.0f)
            , length(.0f)
            , surfaceID(0)
            , segmentIdx(0)
            , flags(0)
        {
        }

        enum Flags
        {
            Disabled   = 1 << 0,
            Validating = 1 << 1,
        };

        Vec3 center;
        Vec3 normal;

        float height;
        float length;

        CoverSurfaceID surfaceID;
        uint16 segmentIdx;
        uint16 flags;
    };

    DynamicCoverManager();

    // IEntityEventListener
    virtual void OnEntityEvent(IEntity* entity, SEntityEvent& event);
    //~IEntityEventListener

    void AddValidationSegment(const ValidationSegment& segment);
    void RemoveSurfaceValidationSegments(const CoverSurfaceID& surfaceID);
    void AddEntity(EntityId entityID);
    void RemoveEntity(EntityId entityID);

    void Reset();
    void Clear();
    void ClearValidationSegments();
    void Update(float updateTime);

    void BreakEvent(const Vec3& position, float radius);
    void MoveEvent(EntityId entityID, const Matrix34& worldTM);

private:
    void RemoveEntity(EntityId entityID, EntityCoverState& state);
    void EntityCoverSampled(EntityId entityID, EntityCoverSampler::ESide side, const ICoverSystem::SurfaceInfo& surfaceInfo);
    void RemoveEntityCoverSurfaces(EntityCoverState& state);

    void QueueValidation(int index);
    void ValidateOne();
    void IntersectionTestComplete(const QueuedIntersectionID& intID, const IntersectionTestResult& result);

    typedef std::vector<ValidationSegment> Segments;
    Segments m_segments;

    typedef std::vector<uint16> FreeSegments;
    FreeSegments m_freeSegments;

    struct segment_position
    {
        segment_position(const Segments& segments)
            : m_segments(segments)
        {
        }

        Vec3 operator()(int segmentIdx) const
        {
            return m_segments[segmentIdx].center;
        }
    private:
        const Segments& m_segments;
    };

    typedef hash_grid<256, uint32, hash_grid_2d<Vec3, Vec3i>, segment_position> SegmentsGrid;
    SegmentsGrid m_segmentsGrid;

    typedef std::vector<int> DirtySegments;
    DirtySegments m_dirtyBuf;

    struct QueuedValidation
    {
        QueuedValidation()
            : validationSegmentIdx(-1)
            , waitingCount(0)
            , negativeCount(0)
        {
        }

        QueuedValidation(int index)
            : validationSegmentIdx(index)
            , waitingCount(0)
            , negativeCount(0)
        {
        }

        int validationSegmentIdx;

        uint8 waitingCount;
        uint8 negativeCount;
    };

    typedef std::deque<QueuedValidation> ValidationQueue;
    ValidationQueue m_validationQueue;


    struct EntityCoverState
    {
        enum EState
        {
            Moving,
            Sampling,
            Sampled,
        };

        EntityCoverState()
            : lastMovement((int64)0ll)
            , state(Moving)
        {
            surfaceID[EntityCoverSampler::Left] = CoverSurfaceID(CoverSurfaceID(0));
            surfaceID[EntityCoverSampler::Right] = CoverSurfaceID(CoverSurfaceID(0));
            surfaceID[EntityCoverSampler::Front] = CoverSurfaceID(CoverSurfaceID(0));
            surfaceID[EntityCoverSampler::Back] = CoverSurfaceID(CoverSurfaceID(0));

            memset(&lastWorldTM, 0, sizeof(Matrix34));
        }

        EntityCoverState(const CTimeValue& now)
            : lastMovement(now)
            , state(Moving)
        {
            surfaceID[EntityCoverSampler::Left] = CoverSurfaceID(CoverSurfaceID(0));
            surfaceID[EntityCoverSampler::Right] = CoverSurfaceID(CoverSurfaceID(0));
            surfaceID[EntityCoverSampler::Front] = CoverSurfaceID(CoverSurfaceID(0));
            surfaceID[EntityCoverSampler::Back] = CoverSurfaceID(CoverSurfaceID(0));

            memset(&lastWorldTM, 0, sizeof(Matrix34));
        }

        CTimeValue lastMovement;
        CoverSurfaceID surfaceID[4];
        Matrix34 lastWorldTM;
        EState state;
    };

    typedef VectorMap<EntityId, EntityCoverState> EntityCover;
    EntityCover m_entityCover;

    EntityCoverSampler m_entityCoverSampler;
};

#endif // CRYINCLUDE_CRYAISYSTEM_COVER_DYNAMICCOVERMANAGER_H
