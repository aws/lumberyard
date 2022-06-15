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

#include <StdAfx.h>

#include <Family/ActorTracker.h>
#include <algorithm>

namespace Blast
{
    void ActorTracker::AddActor(BlastActor* actor)
    {
        m_actors.emplace(actor);
        m_entityIdToActor.emplace(actor->GetEntity()->GetId(), actor);
        if (auto* worldBody = actor->GetWorldBody(); worldBody != nullptr)
        {
            m_bodyToActor.emplace(worldBody, actor);
        }
    }

    void ActorTracker::RemoveActor(BlastActor* actor)
    {
        m_bodyToActor.erase(actor->GetWorldBody());
        m_entityIdToActor.erase(actor->GetEntity()->GetId());
        m_actors.erase(actor);
    }

    BlastActor* ActorTracker::GetActorById(AZ::EntityId entityId)
    {
        if (const auto it = m_entityIdToActor.find(entityId); it != m_entityIdToActor.end())
        {
            return it->second;
        }
        return nullptr;
    }

    BlastActor* ActorTracker::GetActorByBody(const Physics::WorldBody* body)
    {
        if (const auto it = m_bodyToActor.find(body); it != m_bodyToActor.end())
        {
            return it->second;
        }
        // It might be the case that actor didn't have rigid body when being added to the tracker, try all actors and
        // see if they match given body. This might happen very rarely, when blast entity is spawned using a dynamic
        // slice and only for the root actor.
        for (auto* actor : m_actors)
        {
            if (actor->GetWorldBody() == body)
            {
                m_bodyToActor.emplace(body, actor);
                return actor;
            }
        }
        return nullptr;
    }

    BlastActor* ActorTracker::FindClosestActor(const AZ::Vector3& position)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Physics);

        const auto candidate = std::min_element(
            m_actors.begin(), m_actors.end(),
            [&position](BlastActor* a, BlastActor* b)
            {
                return a->GetTransform().GetPosition().GetDistanceSq(position) <
                    b->GetTransform().GetPosition().GetDistanceSq(position);
            });
        return candidate == m_actors.end() ? nullptr : *candidate;
    }

    const AZStd::unordered_set<BlastActor*>& ActorTracker::GetActors() const
    {
        return m_actors;
    }
} // namespace Blast
