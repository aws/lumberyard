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
#pragma once

#include <I3DEngine.h>

#include <AzCore/Math/Vector3.h>
#include <AzCore/std/containers/map.h>

namespace RoadsAndRivers
{
    /**
     * Wrapper around rendering node reference to keep track of it's lifetime
     */
    template<typename RenderNodeType>
    class SectorRenderNode
    {
    public:
        bool HasRenderNode() const
        {
            return mSectorRenderNode != nullptr;
        }

        void SetRenderNode(RenderNodeType* node)
        {
            mSectorRenderNode = AZStd::shared_ptr<RenderNodeType>(node, [](RenderNodeType *node)
            {
                gEnv->p3DEngine->DeleteRenderNode(node);
            });
        }

        AZStd::shared_ptr<RenderNodeType> GetRenderNode() const
        {
            return mSectorRenderNode;
        }

    private:
        AZStd::shared_ptr<RenderNodeType> mSectorRenderNode;
    };

    /**
     * A single chunk of spline geometry made of 4 points
     */
    class SplineGeometrySector
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL;
        AZStd::vector<AZ::Vector3> points;

        float t0 = 0.0f;
        float t1 = 0.0f;
    };

    /**
     * A keyframe to hold a width value at specific spline distance;
     * Used by RoadWidthInterpolator
     */
    struct DistanceWidth
    {
        float distance;
        float width;
    };

    /**
     * Provides an interpolated value among set of keys and values
     */
    class RoadWidthInterpolator
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL;
		
        float GetWidth(float distance) const;
        float GetMaximumWidth() const;
        void InsertDistanceWidthKeyFrame(float distance, float width);
        void Clear();

    private:
        AZStd::vector<DistanceWidth> m_distanceWidthVector;
    };

    namespace Utils
    {
        void UpdateSpecs(IRenderNode* node, EngineSpec specs);
    }
}