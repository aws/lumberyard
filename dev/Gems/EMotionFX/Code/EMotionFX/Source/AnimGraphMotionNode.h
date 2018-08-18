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
#include "AnimGraphNodeData.h"
#include "PlayBackInfo.h"

namespace EMotionFX
{
    // forward declarations
    class Motion;
    class ActorInstance;
    class MotionSet;

    /**
     *
     *
     */
    class EMFX_API AnimGraphMotionNode
        : public AnimGraphNode
    {
    public:
        AZ_RTTI(AnimGraphMotionNode, "{B8B8AAE6-E532-4BF8-898F-3D40AA41BC82}", AnimGraphNode)
        AZ_CLASS_ALLOCATOR_DECL

        enum
        {
            INPUTPORT_PLAYSPEED                 = 0,
            OUTPUTPORT_POSE                     = 0,
            OUTPUTPORT_MOTION                   = 1
        };

        enum
        {
            PORTID_INPUT_PLAYSPEED              = 0,
            PORTID_OUTPUT_POSE                  = 0,
            PORTID_OUTPUT_MOTION                = 1
        };

        enum EIndexMode : AZ::u8
        {
            INDEXMODE_RANDOMIZE                 = 0,
            INDEXMODE_RANDOMIZE_NOREPEAT        = 1,
            INDEXMODE_SEQUENTIAL                = 2,
            INDEXMODE_NUMMETHODS
        };

        class EMFX_API UniqueData
            : public AnimGraphNodeData
        {
            EMFX_ANIMGRAPHOBJECTDATA_IMPLEMENT_LOADSAVE
        public:
            AZ_CLASS_ALLOCATOR_DECL

            UniqueData(AnimGraphNode* node, AnimGraphInstance* animGraphInstance, uint32 motionSetID, MotionInstance* instance);
            ~UniqueData() override;

            void Reset() override;

        public:
            uint32              mMotionSetID;
            uint32              mEventTrackIndex;
            uint32              mActiveMotionIndex;
            MotionInstance*     mMotionInstance;
            bool                mReload;
        };

        AnimGraphMotionNode();
        ~AnimGraphMotionNode();

        void Reinit() override;
        bool InitAfterLoading(AnimGraph* animGraph) override;

        bool GetHasOutputPose() const override              { return true; }
        bool GetCanActAsState() const override              { return true; }
        bool GetSupportsDisable() const override            { return true; }
        bool GetSupportsVisualization() const override      { return true; }
        uint32 GetVisualColor() const override              { return MCore::RGBA(96, 61, 231); }

        void OnUpdateUniqueData(AnimGraphInstance* animGraphInstance) override;
        void OnActorMotionExtractionNodeChanged() override;
        void RecursiveOnChangeMotionSet(AnimGraphInstance* animGraphInstance, MotionSet* newMotionSet) override;

        const char* GetPaletteName() const override;
        AnimGraphObject::ECategory GetPaletteCategory() const override;

        MotionInstance* FindMotionInstance(AnimGraphInstance* animGraphInstance) const;

        AnimGraphPose* GetMainOutputPose(AnimGraphInstance* animGraphInstance) const override             { return GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue(); }

        void SetCurrentPlayTime(AnimGraphInstance* animGraphInstance, float timeInSeconds) override;
        void Rewind(AnimGraphInstance* animGraphInstance) override;

        void UpdatePlayBackInfo(AnimGraphInstance* animGraphInstance);

        float ExtractCustomPlaySpeed(AnimGraphInstance* animGraphInstance) const;
        void PickNewActiveMotion(UniqueData* uniqueData);

        size_t GetNumMotions() const;
        const char* GetMotionId(size_t index) const;
        void ReplaceMotionId(const char* what, const char* replaceWith);
        void AddMotionId(const AZStd::string& name);

        bool GetIsLooping() const                   { return m_loop; }
        bool GetIsRetargeting() const               { return m_retarget; }
        bool GetIsReversed() const                  { return m_reverse; }
        bool GetEmitEvents() const                  { return m_emitEvents; }
        bool GetMirrorMotion() const                { return m_mirrorMotion; }
        bool GetIsMotionExtraction() const          { return m_motionExtraction; }

        void SetMotionIds(const AZStd::vector<AZStd::string>& motionIds);
        void SetLoop(bool loop);
        void SetRetarget(bool retarget);
        void SetReverse(bool reverse);
        void SetEmitEvents(bool emitEvents);
        void SetMirrorMotion(bool mirrorMotion);
        void SetMotionExtraction(bool motionExtraction);
        void SetMotionPlaySpeed(float playSpeed);
        void SetIndexMode(EIndexMode eIndexMode);
        void SetNextMotionAfterLooop(bool nextMotionAfterLoop);
        void SetRewindOnZeroWeight(bool rewindOnZeroWeight);

        static void Reflect(AZ::ReflectContext* context);

    private:
        PlayBackInfo                    m_playInfo;
        AZStd::vector<AZStd::string>    m_motionIds;
        float                           m_playSpeed;
        EIndexMode                      m_indexMode;
        bool                            m_loop;
        bool                            m_retarget;
        bool                            m_reverse;
        bool                            m_emitEvents;
        bool                            m_mirrorMotion;
        bool                            m_motionExtraction;
        bool                            m_nextMotionAfterLoop;
        bool                            m_rewindOnZeroWeight;

        MotionInstance* CreateMotionInstance(ActorInstance* actorInstance, AnimGraphInstance* animGraphInstance);
        void TopDownUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;
        void Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;
        void Output(AnimGraphInstance* animGraphInstance) override;
        void PostUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;

        void OnMotionIdsChanged();
        void OnMirrorMotionChanged();

        AZ::Crc32 GetRewindOnZeroWeightVisibility() const;
        AZ::Crc32 GetMultiMotionWidgetsVisibility() const;
    };
} // namespace EMotionFX