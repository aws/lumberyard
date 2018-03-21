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


#include "BlendSpaceNode.h"
#include <AzCore/std/containers/vector.h>

namespace EMotionFX
{
    class AnimGraphPose;
    class AnimGraphInstance;
    class MotionInstance;


    class EMFX_API BlendSpace1DNode
        : public BlendSpaceNode
    {
        MCORE_MEMORYOBJECTCATEGORY(BlendSpace1DNode, EMFX_DEFAULT_ALIGNMENT, EMFX_MEMCATEGORY_ANIMGRAPH_BLENDSPACE);
        AZ_RTTI(BlendSpace2DNode, "{E41B443C-8423-4764-97F0-6C57997C2E5B}", BlendSpaceNode);

    public:
        enum
        {
            TYPE_ID = 0x00022100
        };

        enum
        {
            ATTRIB_CALCULATION_METHOD   = 0,
            ATTRIB_EVALUATOR            = 1,
            ATTRIB_SYNC                 = 2,
            ATTRIB_SYNC_MASTERMOTION    = 3,
            ATTRIB_EVENTMODE            = 4,
            ATTRIB_MOTIONS              = 5
        };

        enum
        {
            INPUTPORT_VALUE = 0,
            OUTPUTPORT_POSE = 0
        };

        enum
        {
            PORTID_INPUT_VALUE = 0,
            PORTID_OUTPUT_POSE = 0
        };

        struct CurrentSegmentInfo
        {
            AZ::u32 m_segmentIndex;
            float   m_weightForSegmentEnd; // weight for the segment start will be 1 minus this.
        };

        class EMFX_API UniqueData
            : public AnimGraphNodeData
        {
            EMFX_ANIMGRAPHOBJECTDATA_IMPLEMENT_LOADSAVE
        public:
            UniqueData(AnimGraphNode* node, AnimGraphInstance* animGraphInstance)
                : AnimGraphNodeData(node, animGraphInstance)
                , m_allMotionsHaveSyncTracks(false)
                , m_currentPosition(0)
                , m_masterMotionIdx(0)
                , m_hasOverlappingCoordinates(false)
            {
            }

            ~UniqueData();
            uint32 GetClassSize() const override { return sizeof(UniqueData); }
            AnimGraphObjectData* Clone(void* destMem, AnimGraphObject* object, AnimGraphInstance* animGraphInstance) override
            {
                return new (destMem) UniqueData(static_cast<AnimGraphNode*>(object), animGraphInstance);
            }

            float GetRangeMin() const
            {
                return m_sortedMotions.empty() ? 0.0f : m_motionCoordinates[m_sortedMotions.front()];
            }
            float GetRangeMax() const
            {
                return m_sortedMotions.empty() ? 0.0f : m_motionCoordinates[m_sortedMotions.back()];
            }

        public:
            MotionInfos                     m_motionInfos;
            bool                            m_allMotionsHaveSyncTracks;
            AZStd::vector<float>            m_motionCoordinates;
            AZStd::vector<AZ::u16>          m_sortedMotions; // indices to motions sorted according to their parameter values
            float                           m_currentPosition;
            CurrentSegmentInfo              m_currentSegment;
            BlendInfos                      m_blendInfos;
            AZ::u32                         m_masterMotionIdx; // index of the master motion for syncing
            bool                            m_hasOverlappingCoordinates; // indicates if two coordinates are overlapping, to notify the UI
        };

        static BlendSpace1DNode* Create(AnimGraph* animGraph);

        bool GetValidCalculationMethodAndEvaluator() const;
        const char* GetAxisLabel() const;

        // AnimGraphNode overrides
        bool    GetSupportsVisualization() const override { return true; }
        bool    GetSupportsDisable() const override { return true; }
        bool    GetHasVisualGraph() const override { return true; }
        bool    GetHasOutputPose() const override { return true; }
        uint32  GetVisualColor() const override { return MCore::RGBA(59, 181, 200); }
        void    OnUpdateUniqueData(AnimGraphInstance* animGraphInstance) override;
        void    Init(AnimGraphInstance* animGraphInstance) override;
        void    RegisterPorts() override;
        AnimGraphPose* GetMainOutputPose(AnimGraphInstance* animGraphInstance) const override { return GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue(); }

        // AnimGraphObject overrides
        void        RegisterAttributes() override;
        const char* GetTypeString() const override;
        const char* GetPaletteName() const override;
        AnimGraphObject::ECategory GetPaletteCategory() const override;
        AnimGraphObject* Clone(AnimGraph* animGraph) override;
        AnimGraphObjectData* CreateObjectData() override;
        void OnUpdateAttributes() override;

        //! Update the positions of all motions in the blend space.
        void UpdateMotionPositions(UniqueData& uniqueData);

        //! Called to set the current position from GUI.
        void SetCurrentPosition(float point);

    public:
        //  BlendSpaceNode overrides

        //! Compute the position of the motion in blend space.
        void ComputeMotionPosition(const AZStd::string& motionId, AnimGraphInstance* animGraphInstance, AZ::Vector2& position) override;

        //! Restore the motion coordinates that are set to automatic mode back to the computed values.
        void RestoreMotionCoords(const AZStd::string& motionId, AnimGraphInstance* animGraphInstance) override;

        MCore::AttributeArray* GetMotionAttributeArray() const override;

    protected:
        // AnimGraphNode overrides
        void Output(AnimGraphInstance* animGraphInstance) override;
        void TopDownUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;
        void Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;
        void PostUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;
        void Rewind(AnimGraphInstance* animGraphInstance) override;

    private:
        BlendSpace1DNode(AnimGraph* animGraph);
        ~BlendSpace1DNode();

        bool    UpdateMotionInfos(AnimGraphInstance* animGraphInstance);
        void    SortMotionInstances(UniqueData& uniqueData);

        float   GetCurrentSamplePosition(AnimGraphInstance* animGraphInstance, const UniqueData& uniqueData);
        void    UpdateBlendingInfoForCurrentPoint(UniqueData& uniqueData);
        bool    FindLineSegmentForCurrentPoint(UniqueData& uniqueData);
        void    SetBindPoseAtOutput(AnimGraphInstance* animGraphInstance);

    private:
        float   m_currentPositionSetInteractively;
    };
}   // namespace EMotionFX
