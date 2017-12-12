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
#include "AnimGraphTransitionCondition.h"


namespace EMotionFX
{
    // forward declarations
    class AnimGraphInstance;

    /**
     *
     *
     *
     */
    class EMFX_API AnimGraphTimeCondition
        : public AnimGraphTransitionCondition
    {
        MCORE_MEMORYOBJECTCATEGORY(AnimGraphTimeCondition, EMFX_DEFAULT_ALIGNMENT, EMFX_MEMCATEGORY_ANIMGRAPH_CONDITIONS);

    public:
        AZ_RTTI(AnimGraphTimeCondition, "{9CFC3B92-0D9B-4EC8-9999-625EF21A9993}", AnimGraphTransitionCondition);

        enum
        {
            TYPE_ID = 0x00005210
        };

        enum
        {
            ATTRIB_COUNTDOWNTIME    = 0,
            ATTRIB_USERANDOMIZATION = 1,
            ATTRIB_MINRANDOMTIME    = 2,
            ATTRIB_MAXRANDOMTIME    = 3
        };

        // the unique data
        class EMFX_API UniqueData
            : public AnimGraphObjectData
        {
            MCORE_MEMORYOBJECTCATEGORY(AnimGraphTimeCondition::UniqueData, EMFX_DEFAULT_ALIGNMENT, EMFX_MEMCATEGORY_ANIMGRAPH_CONDITIONS);
            EMFX_ANIMGRAPHOBJECTDATA_IMPLEMENT_LOADSAVE

        public:
            UniqueData(AnimGraphObject* object, AnimGraphInstance* animGraphInstance);
            ~UniqueData();

            uint32 GetClassSize() const override                                                                                    { return sizeof(UniqueData); }
            AnimGraphObjectData* Clone(void* destMem, AnimGraphObject* object, AnimGraphInstance* animGraphInstance) override        { return new (destMem) UniqueData(object, animGraphInstance); }

        public:
            float   mElapsedTime;       /**< The elapsed time in seconds for the given anim graph instance. */
            float   mCountDownTime;     /**< The count down time in seconds for the given anim graph instance. */
        };

        static AnimGraphTimeCondition* Create(AnimGraph* animGraph);

        void RegisterAttributes() override;

        const char* GetTypeString() const override;
        void GetSummary(MCore::String* outResult) const override;
        void GetTooltip(MCore::String* outResult) const override;
        const char* GetPaletteName() const override;
        AnimGraphObjectData* CreateObjectData() override;

        void Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;
        bool TestCondition(AnimGraphInstance* animGraphInstance) const override;
        AnimGraphObject* Clone(AnimGraph* animGraph) override;
        void Reset(AnimGraphInstance* animGraphInstance) override;
        void OnUpdateUniqueData(AnimGraphInstance* animGraphInstance) override;

    private:
        AnimGraphTimeCondition(AnimGraph* animGraph);
        ~AnimGraphTimeCondition();

        void OnUpdateAttributes() override;
    };
}   // namespace EMotionFX
