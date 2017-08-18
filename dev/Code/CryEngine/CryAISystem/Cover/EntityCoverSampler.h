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

#ifndef CRYINCLUDE_CRYAISYSTEM_COVER_ENTITYCOVERSAMPLER_H
#define CRYINCLUDE_CRYAISYSTEM_COVER_ENTITYCOVERSAMPLER_H
#pragma once


#include "Cover.h"


class EntityCoverSampler
{
public:
    enum ESide
    {
        Left    = 0,
        Right = 1,
        Front = 2,
        Back    = 3,
        LastSide = Back,
    };

    typedef Functor3<EntityId, EntityCoverSampler::ESide, const ICoverSystem::SurfaceInfo&> Callback;

    EntityCoverSampler();
    ~EntityCoverSampler();

    void Clear();
    void Queue(EntityId entityID, const Callback& callback);
    void Cancel(EntityId entityID);
    void Update();
    void DebugDraw();

private:
    struct QueuedEntity
    {
        enum EState
        {
            Queued = 0,
            SamplingLeft,
            SamplingRight,
            SamplingFront,
            SamplingBack,
        };

        QueuedEntity()
            : distanceSq(FLT_MAX)
            , state(Queued)
        {
        }

        QueuedEntity(EntityId _entityID, Callback _callback)
            : entityID(_entityID)
            , callback(_callback)
            , distanceSq(FLT_MAX)
            , state(Queued)
        {
        }

        EntityId entityID;
        Callback callback;
        float distanceSq;
        EState state;
    };

    struct QueuedEntitySorter
    {
        bool operator()(const QueuedEntity& lhs, const QueuedEntity& rhs) const
        {
            return lhs.distanceSq < rhs.distanceSq;
        }
    };

    typedef std::deque<QueuedEntity> QueuedEntities;
    QueuedEntities m_queue;

    ICoverSampler* m_sampler;
    ICoverSampler::Params m_params;
    uint32 m_currentSide;

    CTimeValue m_lastSort;
    OBB m_obb;
};


#endif // CRYINCLUDE_CRYAISYSTEM_COVER_ENTITYCOVERSAMPLER_H
