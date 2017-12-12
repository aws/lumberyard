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

// include the required headers
#include "EMotionFXConfig.h"
#include "AnimGraphObject.h"

namespace EMotionFX
{
    // forward declarations
    class AnimGraphInstance;
    class AnimGraphTransitionCondition;
    class Transform;
    class Pose;

    /**
     *
     *
     *
     */
    class EMFX_API AnimGraphStateTransition
        : public AnimGraphObject
    {
        MCORE_MEMORYOBJECTCATEGORY(AnimGraphStateTransition, EMFX_DEFAULT_ALIGNMENT, EMFX_MEMCATEGORY_ANIMGRAPH_TRANSITIONS);

    public:
        AZ_RTTI(AnimGraphStateTransition, "{E69C8C6E-7066-43DD-B1BF-0D2FFBDDF457}", AnimGraphObject);

        enum
        {
            BASETYPE_ID = 0x00000002
        };
        enum
        {
            TYPE_ID = 0x00001000
        };

        enum
        {
            INTERPOLATIONFUNCTION_LINEAR        = 0,
            INTERPOLATIONFUNCTION_EASECURVE     = 1
        };

        enum
        {
            ATTRIB_DISABLED                     = 0,
            ATTRIB_PRIORITY                     = 1,
            ATTRIB_CANBEINTERRUPTED             = 2,
            ATTRIB_CANINTERRUPTOTHERTRANSITIONS = 3,
            ATTRIB_ALLOWSELFINTERRUPTION        = 4,
            ATTRIB_ALLOWEDSTATES                = 5,
            ATTRIB_BLENDTIME                    = 6,
            ATTRIB_SYNC                         = 7,
            ATTRIB_EVENTMODE                    = 8,
            ATTRIB_INTERPOLATIONTYPE            = 9,
            ATTRIB_EASEIN_SMOOTH                = 10,
            ATTRIB_EASEOUT_SMOOTH               = 11
        };

        class EMFX_API UniqueData
            : public AnimGraphObjectData
        {
            EMFX_ANIMGRAPHOBJECTDATA_IMPLEMENT_LOADSAVE
        public:
            UniqueData(AnimGraphObject* object, AnimGraphInstance* animGraphInstance, AnimGraphNode* sourceNode);
            ~UniqueData() {}

            uint32 GetClassSize() const override                                                                                    { return sizeof(UniqueData); }
            AnimGraphObjectData* Clone(void* destMem, AnimGraphObject* object, AnimGraphInstance* animGraphInstance) override        { return new (destMem) UniqueData(object, animGraphInstance, nullptr); }

        public:
            AnimGraphNode* mSourceNode;
            float           mBlendWeight;
            float           mBlendProgress;
            float           mTotalSeconds;
            bool            mIsDone;
        };

        static AnimGraphStateTransition* Create(AnimGraph* animGraph);

        void Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;
        virtual void Init(AnimGraphInstance* animGraphInstance) override;
        virtual void OnUpdateAttributes() override;
        virtual void OnRenamedNode(AnimGraph* animGraph, AnimGraphNode* node, const MCore::String& oldName) override;
        virtual void OnCreatedNode(AnimGraph* animGraph, AnimGraphNode* node) override;
        virtual void OnRemoveNode(AnimGraph* animGraph, AnimGraphNode* nodeToRemove) override;
        virtual void OnUpdateUniqueData(AnimGraphInstance* animGraphInstance) override;
        virtual void RegisterAttributes() override;
        virtual void RecursiveCollectObjects(MCore::Array<AnimGraphObject*>& outObjects) const override;
        void ExtractMotion(AnimGraphInstance* animGraphInstance, Transform* outTransform, Transform* outTransformMirrored) const;
        AnimGraphObject* RecursiveClone(AnimGraph* animGraph, AnimGraphObject* parentObject) override;

        void OnStartTransition(AnimGraphInstance* animGraphInstance);
        void OnEndTransition(AnimGraphInstance* animGraphInstance);
        bool GetIsDone(AnimGraphInstance* animGraphInstance) const;
        float GetBlendWeight(AnimGraphInstance* animGraphInstance) const;

        void CalcTransitionOutput(AnimGraphInstance* animGraphInstance, AnimGraphPose& from, AnimGraphPose& to, AnimGraphPose* outputPose) const;

        bool CheckIfIsReady(AnimGraphInstance* animGraphInstance) const;
        float GetBlendTime(AnimGraphInstance* animGraphInstance) const;

        void RegisterPorts() {}
        virtual bool ConvertAttribute(uint32 attributeIndex, const MCore::Attribute* attributeToConvert, const MCore::String& attributeName) override;

        uint32 GetBaseType() const override;
        const char* GetTypeString() const override;
        const char* GetPaletteName() const override;
        AnimGraphObject* Clone(AnimGraph* animGraph) override;
        AnimGraphObject::ECategory GetPaletteCategory() const override;
        AnimGraphObjectData* CreateObjectData() override;

        uint32 GetVisualColor() const;
        bool GetIsStateTransitionNode() const;

        int32 GetPriority() const;

        bool GetCanBeInterrupted() const;
        bool GetCanInterruptOtherTransitions() const;
        bool GetCanInterruptItself() const;

        void SetIsDisabled(bool isDisabled);
        bool GetIsDisabled() const;

        ESyncMode GetSyncMode() const;
        EEventMode GetEventFilterMode() const;

        /**
         * Get the unique identification number for the transition.
         * @return The unique identification number.
         */
        MCORE_INLINE uint32 GetID() const                                                       { return mID; }

        /**
         * Set the unique identification number for the transition.
         * @param[in] id The unique identification number.
         */
        void SetID(uint32 id);

        /**
         * Set if the transition is a wildcard transition or not. A wildcard transition is a transition that will be used in case there is no other path from the current
         * to the destination state. It is basically a transition from all nodes to the destination node of the wildcard transition. A wildcard transition does not have a fixed source node.
         * @param[in] isWildcardTransition Pass true in case the transition is a wildcard transition, false if not.
         */
        void SetIsWildcardTransition(bool isWildcardTransition);

        void SetSourceNode(AnimGraphInstance* animGraphInstance, AnimGraphNode* sourceNode);
        void SetSourceNode(AnimGraphNode* node);
        AnimGraphNode* GetSourceNode(AnimGraphInstance* animGraphInstance) const;
        MCORE_INLINE AnimGraphNode* GetSourceNode() const                                      { return mSourceNode; }

        void SetTargetNode(AnimGraphNode* node);
        MCORE_INLINE AnimGraphNode* GetTargetNode() const                                      { return mTargetNode; }

        void SetVisualOffsets(int32 startX, int32 startY, int32 endX, int32 endY);

        /**
         * Check if the transition is a wildcard transition. A wildcard transition is a transition that will be used in case there is no other path from the current
         * to the destination state. It is basically a transition from all nodes to the destination node of the wildcard transition. A wildcard transition does not have a fixed source node.
         * @result True in case the transition is a wildcard transition, false if not.
         */
        MCORE_INLINE bool GetIsWildcardTransition() const                                       { return mIsWildcardTransition; }

        int32 GetVisualStartOffsetX() const;
        int32 GetVisualStartOffsetY() const;
        int32 GetVisualEndOffsetX() const;
        int32 GetVisualEndOffsetY() const;

        MCORE_INLINE uint32 GetNumConditions() const                                            { return mConditions.GetLength(); }
        MCORE_INLINE AnimGraphTransitionCondition* GetCondition(uint32 index) const            { return mConditions[index]; }
        uint32 FindConditionIndex(AnimGraphTransitionCondition* condition) const;

        void AddCondition(AnimGraphTransitionCondition* condition);
        void InsertCondition(AnimGraphTransitionCondition* condition, uint32 index);
        void ReserveConditions(uint32 numConditions);
        void RemoveCondition(uint32 index, bool delFromMem = true);
        void RemoveAllConditions(bool delFromMem = true);
        void ResetConditions(AnimGraphInstance* animGraphInstance);

    protected:
        AnimGraphStateTransition(AnimGraph* animGraph);
        virtual ~AnimGraphStateTransition();

        float CalculateWeight(float linearWeight) const;

        MCore::Array<AnimGraphTransitionCondition*>    mConditions;
        AnimGraphNode*                                 mSourceNode;
        AnimGraphNode*                                 mTargetNode;
        int32                                           mStartOffsetX;
        int32                                           mStartOffsetY;
        int32                                           mEndOffsetX;
        int32                                           mEndOffsetY;
        uint32                                          mID;                        /**< The unique identification number. */
        bool                                            mIsWildcardTransition;      /**< Flag which indicates if the state transition is a wildcard transition or not. */
    };
}   // namespace EMotionFX
