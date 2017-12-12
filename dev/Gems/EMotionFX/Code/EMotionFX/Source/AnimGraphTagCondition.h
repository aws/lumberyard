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

#include <AzCore/std/containers/vector.h>
#include <EMotionFX/Source/EMotionFXConfig.h>
#include <EMotionFX/Source/AnimGraphTransitionCondition.h>


namespace EMotionFX
{
    // forward declarations
    class AnimGraphInstance;

    class EMFX_API AnimGraphTagCondition
        : public AnimGraphTransitionCondition
    {
        MCORE_MEMORYOBJECTCATEGORY(AnimGraphTagCondition, EMFX_DEFAULT_ALIGNMENT, EMFX_MEMCATEGORY_ANIMGRAPH_CONDITIONS);

    public:
        AZ_RTTI(AnimGraphTagCondition, "{2A786756-80F5-4A55-B00F-5AA876CC4D3A}", AnimGraphTransitionCondition);

        enum
        {
            TYPE_ID = 0x00005321
        };

        enum EFunction
        {
            FUNCTION_ALL        = 0,
            FUNCTION_NOTALL     = 1,
            FUNCTION_ONEORMORE  = 2,
            FUNCTION_NONE       = 3

        };

        enum
        {
            ATTRIB_FUNCTION     = 0,
            ATTRIB_TAGS         = 1
        };

        static AnimGraphTagCondition* Create(AnimGraph* animGraph);

        void RegisterAttributes() override;
        void OnUpdateAttributes() override;

        const char* GetTypeString() const override;
        void GetSummary(MCore::String* outResult) const override;
        void GetTooltip(MCore::String* outResult) const override;
        const char* GetPaletteName() const override;

        bool TestCondition(AnimGraphInstance* animGraphInstance) const override;
        AnimGraphObject* Clone(AnimGraph* animGraph) override;
        AnimGraphObjectData* CreateObjectData() override;

        const char* GetTestFunctionString() const;
        void CreateTagString(AZStd::string& outTagString) const;

    private:
        AnimGraphTagCondition(AnimGraph* animGraph);
        ~AnimGraphTagCondition() override;

        AZStd::vector<AZ::u32> mTagParameterIndices;
    };
} // namespace EMotionFX