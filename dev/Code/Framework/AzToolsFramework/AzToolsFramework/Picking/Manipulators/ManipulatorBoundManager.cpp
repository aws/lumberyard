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

#include "ManipulatorBoundManager.h"

namespace AzToolsFramework
{
    namespace Picking
    {
        ManipulatorBoundManager::ManipulatorBoundManager(ManipulatorManagerId manipulatorManagerId)
            : DefaultContextBoundManager(manipulatorManagerId) {}

        AZStd::shared_ptr<BoundShapeInterface> ManipulatorBoundManager::CreateShape(
            const BoundRequestShapeBase& shapeData, RegisteredBoundId id, AZ::u64 /*userContext*/)
        {
            AZ_Assert(id != InvalidBoundId, "Invalid Bound Id!");

            AZStd::shared_ptr<BoundShapeInterface> shape = shapeData.MakeShapeInterface(id);
            m_bounds.push_back(shape);
            return shape;
        }

        void ManipulatorBoundManager::DeleteShape(AZStd::shared_ptr<BoundShapeInterface> bound)
        {
            const auto found = AZStd::find(m_bounds.begin(), m_bounds.end(), bound);
            if (found != m_bounds.end())
            {
                m_bounds.erase(found);
            }
        }

        void ManipulatorBoundManager::RaySelect(RaySelectInfo& rayInfo)
        {
            using BoundIdHitDistance = AZStd::pair<RegisteredBoundId, float>;

            // create a sorted list of manipulators - sorted based on proximity to ray
            AZStd::vector<BoundIdHitDistance> rayHits;
            rayHits.reserve(m_bounds.size());
            for (const AZStd::shared_ptr<BoundShapeInterface>& bound : m_bounds)
            {
                if (bound->IsValid())
                {
                    float t = 0.0f;
                    if (bound->IntersectRay(rayInfo.m_origin, rayInfo.m_direction, t))
                    {
                        const auto hitItr = AZStd::lower_bound(rayHits.begin(), rayHits.end(), BoundIdHitDistance(0, t),
                            [](const BoundIdHitDistance& lhs, const BoundIdHitDistance& rhs)
                        {
                            return lhs.second < rhs.second;
                        });

                        rayHits.insert(hitItr, AZStd::make_pair(bound->GetBoundId(), t));
                    }
                }
            }

            rayInfo.m_boundIdsHit.reserve(rayHits.size());
            AZStd::copy(rayHits.begin(), rayHits.end(), AZStd::back_inserter(rayInfo.m_boundIdsHit));
        }
    } // namespace Picking
} // namespace AzToolsFramework