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
#include "EventHandler.h"
#include "AnimGraphTransitionCondition.h"

namespace EMotionFX
{
    // forward declarations
    class AnimGraphInstance;
    class AnimGraphNode;

    /**
     *
     *
     *
     */
    class EMFX_API AnimGraphStateCondition
        : public AnimGraphTransitionCondition
    {
        MCORE_MEMORYOBJECTCATEGORY(AnimGraphStateCondition, EMFX_DEFAULT_ALIGNMENT, EMFX_MEMCATEGORY_ANIMGRAPH_CONDITIONS);

        // forward declaration
    private:
        class EventHandler;

    public:
        AZ_RTTI(AnimGraphStateCondition, "{8C955719-5D14-4BB5-BA64-F2A3385CAF7E}", AnimGraphTransitionCondition);

        enum
        {
            TYPE_ID = 0x09502005
        };

        enum
        {
            ATTRIB_STATE                = 0,
            ATTRIB_FUNCTION             = 1,
            ATTRIB_PLAYTIME             = 2
        };

        enum EFunction
        {
            FUNCTION_EXITSTATES         = 0,
            FUNCTION_ENTERING           = 1,
            FUNCTION_ENTER              = 2,
            FUNCTION_EXIT               = 3,
            FUNCTION_END                = 4,
            FUNCTION_PLAYTIME           = 5,
            FUNCTION_NONE               = 6
        };

        // the unique data
        class EMFX_API UniqueData
            : public AnimGraphObjectData
        {
            MCORE_MEMORYOBJECTCATEGORY(AnimGraphStateCondition::UniqueData, EMFX_DEFAULT_ALIGNMENT, EMFX_MEMCATEGORY_ANIMGRAPH_CONDITIONS);
            EMFX_ANIMGRAPHOBJECTDATA_IMPLEMENT_LOADSAVE

        public:
            UniqueData(AnimGraphObject* object, AnimGraphInstance* animGraphInstance);
            virtual ~UniqueData();

            uint32 GetClassSize() const override                    { return sizeof(UniqueData); }
            AnimGraphObjectData* Clone(void* destMem, AnimGraphObject* object, AnimGraphInstance* animGraphInstance) override        { return new (destMem) UniqueData(object, animGraphInstance); }

            void CreateEventHandler();
            void DeleteEventHandler();

        public:
            // The anim graph instance pointer shouldn't change. If it were to
            // change, we'd need to remove an existing event handler and create
            // a new one in the new anim graph instance.
            AnimGraphInstance* const                    mAnimGraphInstance;
            AnimGraphStateCondition::EventHandler*      mEventHandler;
            bool                                        mTriggered;
        };

        static AnimGraphStateCondition* Create(AnimGraph* animGraph);

        void RegisterAttributes() override;
        void OnUpdateUniqueData(AnimGraphInstance* animGraphInstance) override;
        void OnUpdateAttributes() override;
        void OnRenamedNode(AnimGraph* animGraph, AnimGraphNode* node, const AZStd::string& oldName) override;
        void OnRemoveNode(AnimGraph* animGraph, AnimGraphNode* nodeToRemove) override;

        void Reset(AnimGraphInstance* animGraphInstance) override;

        const char* GetTypeString() const override;
        void GetSummary(AZStd::string* outResult) const override;
        void GetTooltip(AZStd::string* outResult) const override;
        const char* GetPaletteName() const override;
        ECategory GetPaletteCategory() const override;

        bool TestCondition(AnimGraphInstance* animGraphInstance) const override;
        const char* GetTestFunctionString() const;
        AnimGraphObject* Clone(AnimGraph* animGraph) override;
        AnimGraphObjectData* CreateObjectData() override;

    private:
        // the event handler
        class EMFX_API EventHandler
            : public AnimGraphInstanceEventHandler
        {
            MCORE_MEMORYOBJECTCATEGORY(EventHandler, EMFX_DEFAULT_ALIGNMENT, EMFX_MEMCATEGORY_ANIMGRAPH_CONDITIONS);

        public:
            EventHandler(AnimGraphStateCondition* condition, UniqueData* uniqueData);
            virtual ~EventHandler();

            // overloaded 
            void OnStateEnter(AnimGraphInstance* animGraphInstance, AnimGraphNode* state) override;
            void OnStateEntering(AnimGraphInstance* animGraphInstance, AnimGraphNode* state) override;
            void OnStateExit(AnimGraphInstance* animGraphInstance, AnimGraphNode* state) override;
            void OnStateEnd(AnimGraphInstance* animGraphInstance, AnimGraphNode* state) override;

        private:
            bool IsTargetState(const AnimGraphNode* state) const
            {
                const AZStd::string& stateName = mCondition->GetAttributeString(ATTRIB_STATE)->GetValue();
                return (stateName.empty() || stateName == state->GetName());
            }

            void OnStateChange(AnimGraphInstance* animGraphInstance, AnimGraphNode* state, EFunction targetFunction)
            {
                // check if the state and the anim graph instance are valid and
                // return directly in case one of them is not
                if (!state || !animGraphInstance)
                {
                    return;
                }

                const int32 functionIndex = mCondition->GetAttributeFloatAsInt32(ATTRIB_FUNCTION);
                if (functionIndex == targetFunction && IsTargetState(state))
                {
                    mUniqueData->mTriggered = true;
                }
            }

            AnimGraphStateCondition*    mCondition;
            UniqueData*                 mUniqueData;
        };

        AnimGraphNode*     mState;

        AnimGraphStateCondition(AnimGraph* animGraph);
        ~AnimGraphStateCondition();
    };
}   // namespace EMotionFX
