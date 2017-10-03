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
#include "EventHandler.h"
#include "AnimGraphTransitionCondition.h"


namespace EMotionFX
{
    // forward declarations
    class AnimGraphInstance;
    class AnimGraphMotionNode;

    /**
     *
     *
     *
     */
    class EMFX_API AnimGraphMotionCondition
        : public AnimGraphTransitionCondition
    {
        MCORE_MEMORYOBJECTCATEGORY(AnimGraphMotionCondition, EMFX_DEFAULT_ALIGNMENT, EMFX_MEMCATEGORY_ANIMGRAPH_CONDITIONS);

        // Forward declaration.
    private:
        class EventHandler;

    public:
        AZ_RTTI(AnimGraphMotionCondition, "{0E2EDE4E-BDEE-4383-AB18-208CE7F7A784}", AnimGraphTransitionCondition);

        enum
        {
            TYPE_ID = 0x00002001
        };

        enum
        {
            ATTRIB_MOTIONNODE       = 0,
            ATTRIB_FUNCTION         = 1,
            ATTRIB_NUMLOOPS         = 2,
            ATTRIB_PLAYTIME         = 3,
            ATTRIB_EVENTTYPE        = 4,
            ATTRIB_EVENTPARAMETER   = 5
        };

        enum EFunction
        {
            FUNCTION_EVENT                  = 0,
            FUNCTION_HASENDED               = 1,
            FUNCTION_HASREACHEDMAXNUMLOOPS  = 2,
            FUNCTION_PLAYTIME               = 3,
            FUNCTION_PLAYTIMELEFT           = 4,
            FUNCTION_ISMOTIONASSIGNED       = 5,
            FUNCTION_ISMOTIONNOTASSIGNED    = 6,
            FUNCTION_NONE                   = 7
        };

        // the unique data
        class EMFX_API UniqueData
            : public AnimGraphObjectData
        {
            MCORE_MEMORYOBJECTCATEGORY(AnimGraphMotionCondition::UniqueData, EMFX_DEFAULT_ALIGNMENT, EMFX_MEMCATEGORY_ANIMGRAPH_CONDITIONS);
            EMFX_ANIMGRAPHOBJECTDATA_IMPLEMENT_LOADSAVE

        public:
            UniqueData(AnimGraphObject* object, AnimGraphInstance* animGraphInstance, MotionInstance* motionInstance);
            ~UniqueData();

            uint32 GetClassSize() const override                                                                                     { return sizeof(UniqueData); }
            AnimGraphObjectData* Clone(void* destMem, AnimGraphObject* object, AnimGraphInstance* animGraphInstance) override        { return new (destMem) UniqueData(object, animGraphInstance, nullptr); }

        public:
            MotionInstance*                             mMotionInstance;
            AnimGraphMotionCondition::EventHandler*     mEventHandler;
            bool                                        mTriggered;
        };

        static AnimGraphMotionCondition* Create(AnimGraph* animGraph);

        void RegisterAttributes() override;
        void OnUpdateUniqueData(AnimGraphInstance* animGraphInstance) override;
        void OnUpdateAttributes() override;
        void OnRenamedNode(AnimGraph* animGraph, AnimGraphNode* node, const MCore::String& oldName) override;
        void OnRemoveNode(AnimGraph* animGraph, AnimGraphNode* nodeToRemove) override;

        void Reset(AnimGraphInstance* animGraphInstance) override;

        const char* GetTypeString() const override;
        void GetSummary(MCore::String* outResult) const override;
        void GetTooltip(MCore::String* outResult) const override;
        const char* GetPaletteName() const override;
        AnimGraphObjectData* CreateObjectData() override;

        bool TestCondition(AnimGraphInstance* animGraphInstance) const override;
        const char* GetTestFunctionString() const;
        AnimGraphObject* Clone(AnimGraph* animGraph) override;
        EFunction GetFunction() const;

    private:
        // the motion instance event handler
        class EventHandler
            : public MotionInstanceEventHandler
        {
            MCORE_MEMORYOBJECTCATEGORY(AnimGraphMotionCondition::EventHandler, EMFX_DEFAULT_ALIGNMENT, EMFX_MEMCATEGORY_ANIMGRAPH_CONDITIONS);

        public:
            static EventHandler* Create(AnimGraphMotionCondition* condition, UniqueData* uniqueData);

            void OnEvent(const EventInfo& eventInfo) override;
            void OnHasLooped() override;
            void OnIsFrozenAtLastFrame() override;
            void OnDeleteMotionInstance() override;

        private:
            AnimGraphMotionCondition*  mCondition;
            UniqueData*                mUniqueData;

            EventHandler(AnimGraphMotionCondition* condition, UniqueData* uniqueData);
            ~EventHandler();
        };

        AnimGraphMotionNode*   mMotionNode;

        AnimGraphMotionCondition(AnimGraph* animGraph);
        ~AnimGraphMotionCondition();
    };
} // namespace EMotionFX