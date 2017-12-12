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
    class EMFX_API AnimGraphPlayTimeCondition
        : public AnimGraphTransitionCondition
    {
        MCORE_MEMORYOBJECTCATEGORY(AnimGraphPlayTimeCondition, EMFX_DEFAULT_ALIGNMENT, EMFX_MEMCATEGORY_ANIMGRAPH_CONDITIONS);

    public:
        AZ_RTTI(AnimGraphPlayTimeCondition, "{5368D058-9552-4282-A273-AA9344E65D2E}", AnimGraphTransitionCondition);

        enum
        {
            TYPE_ID = 0x00029610
        };

        enum
        {
            ATTRIB_NODE         = 0,
            ATTRIB_PLAYTIME     = 1,
            ATTRIB_MODE         = 2
        };

        enum
        {
            MODE_REACHEDTIME    = 0,
            MODE_REACHEDEND     = 1,
            MODE_HASLESSTHAN    = 2,
            MODE_NUM            = 3
        };

        static AnimGraphPlayTimeCondition* Create(AnimGraph* animGraph);

        void RegisterAttributes() override;

        const char* GetTypeString() const override;
        void GetSummary(MCore::String* outResult) const override;
        void GetTooltip(MCore::String* outResult) const override;
        const char* GetPaletteName() const override;
        AnimGraphObjectData* CreateObjectData() override;

        void OnUpdateAttributes() override;
        bool TestCondition(AnimGraphInstance* animGraphInstance) const override;
        AnimGraphObject* Clone(AnimGraph* animGraph) override;

        void OnRenamedNode(AnimGraph* animGraph, AnimGraphNode* node, const MCore::String& oldName) override;
        void OnRemoveNode(AnimGraph* animGraph, AnimGraphNode* nodeToRemove) override;

    private:
        AnimGraphNode*     mNode;

        AnimGraphPlayTimeCondition(AnimGraph* animGraph);
        ~AnimGraphPlayTimeCondition();
    };
}   // namespace EMotionFX
