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

#include "AnimGraphNode.h"
#include <AzCore/Math/Vector2.h>


namespace EMotionFX
{
    class BlendSpaceParamEvaluator;

    class EMFX_API BlendSpaceNode
        : public AnimGraphNode
    {
        MCORE_MEMORYOBJECTCATEGORY(BlendSpaceNode, EMFX_DEFAULT_ALIGNMENT, EMFX_MEMCATEGORY_ANIMGRAPH_BLENDSPACE);
        AZ_RTTI(BlendSpaceNode, "{11EC99C4-6A25-4610-86FD-B01F2E53007E}", AnimGraphNode);

    public:
        enum EBlendSpaceEventMode
        {
            BSEVENTMODE_ALL_ACTIVE_MOTIONS = 0,
            BSEVENTMODE_MOST_ACTIVE_MOTION = 1
        };

        enum class ECalculationMethod
        {
            AUTO,
            MANUAL
        };

    public:
        BlendSpaceNode(AnimGraph* animGraph, const char* name, uint32 typeID);

        //! Compute the position of the motion in blend space.
        virtual void ComputeMotionPosition(const AZStd::string& motionId, AnimGraphInstance* animGraphInstance, AZ::Vector2& position) = 0;

        //! Restore the motion coordinates that are set to automatic mode back to the computed values.
        virtual void RestoreMotionCoords(const AZStd::string& motionId, AnimGraphInstance* animGraphInstance) = 0;

        //! Common interface to access the blend space motions regardless of the blend space dimension.
        virtual MCore::AttributeArray* GetMotionAttributeArray() const = 0;

        //! The node is in interactive mode when the user is interactively changing the current point.
        void SetInteractiveMode(bool enable) { mInteractiveMode = enable; }
        bool IsInInteractiveMode() const { return mInteractiveMode; }

    protected:
        struct MotionInfo
        {
            MotionInstance*     m_motionInstance;
            AnimGraphSyncTrack  m_syncTrack;
            uint32              m_syncIndex;
            float               m_playSpeed;
            float               m_currentTime; // current play time (NOT normalized)
            float               m_preSyncTime;

            MotionInfo();
        };
        using MotionInfos = AZStd::vector<MotionInfo>;

        struct BlendInfo
        {
            AZ::u32 m_motionIndex;
            float   m_weight;

            bool operator<(const BlendInfo& rhs)
            {
                // We want to sort in decreasing order of weight.
                return m_weight > rhs.m_weight;
            }
        };
        using BlendInfos = AZStd::vector<BlendInfo>;

    protected:
        void RegisterCalculationMethodAttribute(const char* name, const char* internalName, const char* description);
        void RegisterBlendSpaceEvaluatorAttribute(const char* name, const char* internalName, const char* description);
        ECalculationMethod GetBlendSpaceCalculationMethod(uint32 attribIndex) const;
        BlendSpaceParamEvaluator* GetBlendSpaceParamEvaluator(uint32 attribIndex) const;

        void RegisterBlendSpaceEventFilterAttribute();
        void RegisterMasterMotionAttribute();

        uint32 FindBlendSpaceMotionAttributeIndexByMotionId(uint32 motionsAttributeIndex, const AZStd::string& motionId) const;

        void DoUpdate(float timePassedInSeconds, const BlendInfos& blendInfos, ESyncMode syncMode, AZ::u32 masterIdx, MotionInfos& motionInfos);
        void DoTopDownUpdate(AnimGraphInstance* animGraphInstance, ESyncMode syncMode,
            AZ::u32 masterIdx, MotionInfos& motionInfos, bool motionsHaveSyncTracks);
        void DoPostUpdate(AnimGraphInstance* animGraphInstance, AZ::u32 masterIdx, BlendInfos& blendInfos, MotionInfos& motionInfos,
            EBlendSpaceEventMode eventFilterMode, AnimGraphRefCountedData* data);

        void RewindMotions(MotionInfos& motionInfos);

        static AZ::u32 GetIndexOfMotionInBlendInfos(const BlendInfos& blendInfos, AZ::u32 motionIndex);

        static void ClearMotionInfos(MotionInfos& motionInfos);
        static void AddMotionInfo(MotionInfos& motionInfos, MotionInstance* motionInstance);
        
        static bool DoAllMotionsHaveSyncTracks(const MotionInfos& motionInfos);

        static void DoClipBasedSyncOfMotionsToMaster(AZ::u32 masterIdx, MotionInfos& motionInfos);
        static void DoEventBasedSyncOfMotionsToMaster(AZ::u32 masterIdx, MotionInfos& motionInfos);

        static void SyncMotionToNode(AnimGraphInstance* animGraphInstance, ESyncMode syncMode, MotionInfo& motionInfo, AnimGraphNode* srcNode);

    private:
        bool mInteractiveMode;// true when the user is changing the current point by dragging in GUI
    };

    //================  Inline implementations ====================================

    MCORE_INLINE AZ::u32 BlendSpaceNode::GetIndexOfMotionInBlendInfos(const BlendInfos& blendInfos, AZ::u32 motionIndex)
    {
        const AZ::u32 numBlendInfos = (AZ::u32)blendInfos.size();
        for (AZ::u32 i=0; i < numBlendInfos; ++i)
        {
            if (blendInfos[i].m_motionIndex == motionIndex)
            {
                return i;
            }
        }
        return MCORE_INVALIDINDEX32;
    }

} // namespace EMotionFX
