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
    public:
        AZ_RTTI(AnimGraphTagCondition, "{2A786756-80F5-4A55-B00F-5AA876CC4D3A}", AnimGraphTransitionCondition)
        AZ_CLASS_ALLOCATOR_DECL

        enum EFunction : AZ::u8
        {
            FUNCTION_ALL        = 0,
            FUNCTION_NOTALL     = 1,
            FUNCTION_ONEORMORE  = 2,
            FUNCTION_NONE       = 3
        };

        AnimGraphTagCondition();
        AnimGraphTagCondition(AnimGraph* animGraph);
        ~AnimGraphTagCondition() override;

        void Reinit() override;
        bool InitAfterLoading(AnimGraph* animGraph) override;

        void GetSummary(AZStd::string* outResult) const override;
        void GetTooltip(AZStd::string* outResult) const override;
        const char* GetPaletteName() const override;

        bool TestCondition(AnimGraphInstance* animGraphInstance) const override;

        const char* GetTestFunctionString() const;
        void CreateTagString(AZStd::string& outTagString) const;

        void SetFunction(EFunction function);
        void SetTags(const AZStd::vector<AZStd::string>& tags);
        void RenameTag(const AZStd::string& oldTagName, const AZStd::string& newTagName);

        static void Reflect(AZ::ReflectContext* context);

    private:
        static const char* s_functionAllTags;
        static const char* s_functionOneOrMoreInactive;
        static const char* s_functionOneOrMoreActive;
        static const char* s_functionNoTagActive;

        AZStd::vector<AZStd::string>    m_tags;
        AZStd::vector<size_t>           m_tagParameterIndices;
        EFunction                       m_function;
    };
} // namespace EMotionFX