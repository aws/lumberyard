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

#include <EMotionFX/Source/AnimGraphNode.h>


namespace EMotionFX
{
    class EMFX_API BlendTreeRagdollStrenghModifierNode
        : public AnimGraphNode
    {
    public:
        AZ_RTTI(BlendTreeRagdollStrenghModifierNode, "{9A31E551-CC6D-4FBE-BBFA-4291EAF62A86}", AnimGraphNode)
        AZ_CLASS_ALLOCATOR_DECL

        enum
        {
            INPUTPORT_POSE = 0,
            INPUTPORT_STRENGTH = 1,
            INPUTPORT_DAMPINGRATIO = 2,
            OUTPUTPORT_POSE = 0
        };

        enum
        {
            PORTID_POSE = 0,
            PORTID_STRENGTH = 1,
            PORTID_DAMPINGRATIO = 2,
            PORTID_OUTPUT_POSE = 0
        };

        enum StrengthInputType : AZ::u8
        {
            STRENGTHINPUTTYPE_NONE = 0,
            STRENGTHINPUTTYPE_OVERWRITE = 1,
            STRENGTHINPUTTYPE_MULTIPLY = 2
        };

        enum DampingRatioInputType : AZ::u8
        {
            DAMPINGRATIOINPUTTYPE_NONE = 0,
            DAMPINGRATIOINPUTTYPE_OVERWRITE = 1
        };

        class EMFX_API UniqueData
            : public AnimGraphNodeData
        {
            EMFX_ANIMGRAPHOBJECTDATA_IMPLEMENT_LOADSAVE
        public:
            AZ_CLASS_ALLOCATOR_DECL

            UniqueData(AnimGraphNode* node, AnimGraphInstance* animGraphInstance);
            ~UniqueData() override = default;

        public:
            AZStd::vector<AZ::u32> m_modifiedJointIndices;
            bool m_mustUpdate;
        };

        BlendTreeRagdollStrenghModifierNode();
        ~BlendTreeRagdollStrenghModifierNode() override = default;

        void Reinit() override;
        bool InitAfterLoading(AnimGraph* animGraph) override;

        uint32 GetVisualColor() const override                          { return MCore::RGBA(206, 175, 148); }
        const char* GetPaletteName() const override                     { return "Ragdoll Strength Modifier"; }
        AnimGraphObject::ECategory GetPaletteCategory() const override  { return AnimGraphObject::CATEGORY_PHYSICS; }

        void OnUpdateUniqueData(AnimGraphInstance* animGraphInstance) override;

        void Output(AnimGraphInstance* animGraphInstance) override;
        AnimGraphPose* GetMainOutputPose(AnimGraphInstance* animGraphInstance) const override     { return GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue(); }
        bool GetHasOutputPose() const override                          { return true; }

        static void Reflect(AZ::ReflectContext* context);

    private:
        bool IsStrengthReadOnly() const;
        bool IsDampingRatioReadOnly() const;

        AZStd::vector<AZStd::string>    m_modifiedJointNames;
        float                           m_strength;
        float                           m_dampingRatio;
        StrengthInputType               m_strengthInputType;
        DampingRatioInputType           m_dampingRatioInputType;
    };
} // namespace EMotionFX