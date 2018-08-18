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
#include "StdAfx.h"
#include <AzCore/Memory/SystemAllocator.h>

#include "RoadRiverCommon.h"

namespace RoadsAndRivers
{
    float EaseInOut(float t)
    {
        t *= 2.0f;
        if (t < 1)
        {
            return 0.5f * t * t;
        }
        t -= 1.0f;
        return -0.5f * (t * (t - 2.0f) - 1.0f);
    }

    float RoadWidthInterpolator::GetWidth(float distance) const
    {
        if (m_distanceWidthVector.size() == 0)
        {
            return 0.0f;
        }

        auto currentPair = m_distanceWidthVector.begin();
        auto previusPair = currentPair;

        while (currentPair != m_distanceWidthVector.end())
        {
            if (distance < currentPair->distance)
            {
                if (currentPair == m_distanceWidthVector.begin())
                {
                    return currentPair->width;
                }

                float easedInverseLerp = EaseInOut(AZ::LerpInverse(previusPair->distance, currentPair->distance, distance));
                return AZ::Lerp(previusPair->width, currentPair->width, easedInverseLerp);
            }

            previusPair = currentPair;
            ++currentPair;
        }

        return m_distanceWidthVector.rbegin()->width;
    }

    float RoadWidthInterpolator::GetMaximumWidth() const
    {
        auto result = std::max_element(m_distanceWidthVector.begin(), m_distanceWidthVector.end(), [](const DistanceWidth& lhs, const DistanceWidth& rhs)
        {
            return lhs.width < rhs.width;
        });

        if (result == m_distanceWidthVector.end())
        {
            return 0.0f;
        }

        return result->width;
    }

    void RoadWidthInterpolator::InsertDistanceWidthKeyFrame(float distance, float width)
    {
        DistanceWidth keyFrame { distance, width };
        auto placeToInsert = AZStd::upper_bound(m_distanceWidthVector.begin(), m_distanceWidthVector.end(), keyFrame, [](const DistanceWidth& lhs, const DistanceWidth& rhs)
        {
            return lhs.distance < rhs.distance;
        });
        m_distanceWidthVector.insert(placeToInsert, keyFrame);
    }

    void RoadWidthInterpolator::Clear()
    {
        m_distanceWidthVector.clear();
    }

    namespace Utils
    {
        void UpdateSpecs(IRenderNode* node, EngineSpec specs)
        {
            if (node && gEnv && gEnv->p3DEngine)
            {
                const int configSpec = gEnv->pSystem->GetConfigSpec(true);
                AZ::u32 rendFlags = static_cast<AZ::u32>(node->GetRndFlags());
                const bool hidden = static_cast<AZ::u32>(configSpec) < static_cast<AZ::u32>(specs);
                if (hidden)
                {
                    rendFlags |= ERF_HIDDEN;
                }
                else
                {
                    rendFlags &= ~ERF_HIDDEN;
                }
                node->SetRndFlags(rendFlags);
            }
        }
    }

    AZ_CLASS_ALLOCATOR_IMPL(SplineGeometrySector, AZ::SystemAllocator, 0);
    AZ_CLASS_ALLOCATOR_IMPL(RoadWidthInterpolator, AZ::SystemAllocator, 0);
}