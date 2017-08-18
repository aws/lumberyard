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

namespace LmbrCentral
{
    size_t PhysicsSystemRequests::RayCastResult::GetHitCount() const
    {
        size_t count = m_piercingHits.size();
        if (m_blockingHit.IsValid())
        {
            ++count;
        }
        return count;
    }

    const PhysicsSystemRequests::RayCastHit* PhysicsSystemRequests::RayCastResult::GetHit(size_t index) const
    {
        if (index < m_piercingHits.size())
        {
            return &m_piercingHits[index];
        }
        else if (index == m_piercingHits.size() && m_blockingHit.IsValid())
        {
            return &m_blockingHit;
        }
        else
        {
            return nullptr;
        }
    }

    bool PhysicsSystemRequests::RayCastResult::HasBlockingHit() const
    {
        return m_blockingHit.IsValid();
    }

    const PhysicsSystemRequests::RayCastHit* PhysicsSystemRequests::RayCastResult::GetBlockingHit() const
    {
        return m_blockingHit.IsValid() ? &m_blockingHit : nullptr;
    }

    size_t PhysicsSystemRequests::RayCastResult::GetPiercingHitCount() const
    {
        return m_piercingHits.size();
    }

    const PhysicsSystemRequests::RayCastHit* PhysicsSystemRequests::RayCastResult::GetPiercingHit(size_t index) const
    {
        return index < m_piercingHits.size() ? &m_piercingHits[index] : nullptr;
    }

    void PhysicsSystemRequests::RayCastResult::SetBlockingHit(const RayCastHit& blockingHit)
    {
        m_blockingHit = blockingHit;
    }

    void PhysicsSystemRequests::RayCastResult::AddPiercingHit(const RayCastHit& piercingHit)
    {
        m_piercingHits.emplace_back(piercingHit);
    }

} // namespace LmbrCentral