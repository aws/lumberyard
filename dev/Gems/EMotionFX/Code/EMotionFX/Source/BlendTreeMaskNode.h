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

#include "EMotionFXConfig.h"
#include "AnimGraphNode.h"


namespace EMotionFX
{
    /**
     *
     */
    class EMFX_API BlendTreeMaskNode
        : public AnimGraphNode
    {
    public:
        AZ_RTTI(BlendTreeMaskNode, "{24647B8B-05B4-4D5D-9161-F0AD0B456B09}", AnimGraphNode)
        AZ_CLASS_ALLOCATOR_DECL

        //
        enum
        {
            INPUTPORT_POSE_0    = 0,
            INPUTPORT_POSE_1    = 1,
            INPUTPORT_POSE_2    = 2,
            INPUTPORT_POSE_3    = 3,
            OUTPUTPORT_RESULT   = 0
        };

        enum
        {
            PORTID_INPUT_POSE_0     = 0,
            PORTID_INPUT_POSE_1     = 1,
            PORTID_INPUT_POSE_2     = 2,
            PORTID_INPUT_POSE_3     = 3,
            PORTID_OUTPUT_RESULT    = 0
        };

        class EMFX_API UniqueData
            : public AnimGraphNodeData
        {
            EMFX_ANIMGRAPHOBJECTDATA_IMPLEMENT_LOADSAVE
        public:
            AZ_CLASS_ALLOCATOR_DECL

            UniqueData(AnimGraphNode* node, AnimGraphInstance* animGraphInstance)
                : AnimGraphNodeData(node, animGraphInstance)     { mMustUpdate = true; }
            ~UniqueData()   {}

        public:
            AZStd::vector< AZStd::vector<AZ::u32> > mMasks;
            bool                                    mMustUpdate;
        };

        BlendTreeMaskNode();
        ~BlendTreeMaskNode();

        void Reinit() override;
        bool InitAfterLoading(AnimGraph* animGraph) override;

        void OnUpdateUniqueData(AnimGraphInstance* animGraphInstance) override;
        bool GetHasOutputPose() const override                      { return true; }
        bool GetSupportsVisualization() const override              { return true; }
        uint32 GetVisualColor() const override                      { return MCore::RGBA(50, 200, 50); }
        AnimGraphPose* GetMainOutputPose(AnimGraphInstance* animGraphInstance) const override         { return GetOutputPose(animGraphInstance, OUTPUTPORT_RESULT)->GetValue(); }

        const char* GetPaletteName() const override;
        AnimGraphObject::ECategory GetPaletteCategory() const override;

        bool GetOutputEvents(size_t index) const;

        void SetMask0(const AZStd::vector<AZStd::string>& mask0);
        void SetMask1(const AZStd::vector<AZStd::string>& mask1);
        void SetMask2(const AZStd::vector<AZStd::string>& mask2);
        void SetMask3(const AZStd::vector<AZStd::string>& mask3);
        void SetOutputEvents0(bool outputEvents0);
        void SetOutputEvents1(bool outputEvents1);
        void SetOutputEvents2(bool outputEvents2);
        void SetOutputEvents3(bool outputEvents3);

        static void Reflect(AZ::ReflectContext* context);

    private:
        void UpdateUniqueData(AnimGraphInstance* animGraphInstance, UniqueData* uniqueData);
        void Output(AnimGraphInstance* animGraphInstance) override;
        void Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;
        void PostUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;

        void UpdateUniqueMask(Actor* actor, const AZStd::vector<AZStd::string>& nodeMask, AZStd::vector<AZ::u32>& outNodeIndices) const;

        AZStd::vector<AZStd::string>        m_mask0;
        AZStd::vector<AZStd::string>        m_mask1;
        AZStd::vector<AZStd::string>        m_mask2;
        AZStd::vector<AZStd::string>        m_mask3;
        bool                                m_outputEvents0;
        bool                                m_outputEvents1;
        bool                                m_outputEvents2;
        bool                                m_outputEvents3;

        static size_t                       m_numMasks;
    };
}   // namespace EMotionFX
