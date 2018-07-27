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

#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/std/string/string.h>
#include <EMotionFX/Source/EMotionFXConfig.h>
#include <EMotionFX/Source/AnimGraphNode.h>


namespace EMotionFX
{
    /**
     *
     *
     *
     */
    class EMFX_API BlendTreeTransformNode
        : public AnimGraphNode
    {
    public:
        AZ_RTTI(BlendTreeTransformNode, "{348DB122-ABA7-4ED8-BB20-0F9560F7FA6B}", AnimGraphNode)
        AZ_CLASS_ALLOCATOR_DECL

        // input ports
        enum
        {
            INPUTPORT_POSE              = 0,
            INPUTPORT_TRANSLATE_AMOUNT  = 1,
            INPUTPORT_ROTATE_AMOUNT     = 2,
            INPUTPORT_SCALE_AMOUNT      = 3
        };

        enum
        {
            PORTID_INPUT_POSE               = 0,
            PORTID_INPUT_TRANSLATE_AMOUNT   = 1,
            PORTID_INPUT_ROTATE_AMOUNT      = 2,
            PORTID_INPUT_SCALE_AMOUNT       = 3
        };

        // output ports
        enum
        {
            OUTPUTPORT_RESULT   = 0
        };

        enum
        {
            PORTID_OUTPUT_POSE = 0
        };

        class EMFX_API UniqueData
            : public AnimGraphNodeData
        {
            EMFX_ANIMGRAPHOBJECTDATA_IMPLEMENT_LOADSAVE
        public:
            AZ_CLASS_ALLOCATOR_DECL

            UniqueData(AnimGraphNode* node, AnimGraphInstance* animGraphInstance)
                : AnimGraphNodeData(node, animGraphInstance)     { mNodeIndex = MCORE_INVALIDINDEX32; mMustUpdate = true; mIsValid = false; }
            ~UniqueData() {}

        public:
            uint32          mNodeIndex;
            bool            mMustUpdate;
            bool            mIsValid;
        };

        BlendTreeTransformNode();
        ~BlendTreeTransformNode();

        void Reinit() override;
        bool InitAfterLoading(AnimGraph* animGraph) override;

        void OnUpdateUniqueData(AnimGraphInstance* animGraphInstance) override;

        uint32 GetVisualColor() const override                  { return MCore::RGBA(255, 0, 0); }
        bool GetCanActAsState() const override                  { return false; }
        bool GetSupportsVisualization() const override          { return true; }
        bool GetHasOutputPose() const override                  { return true; }
        AnimGraphPose* GetMainOutputPose(AnimGraphInstance* animGraphInstance) const override     { return GetOutputPose(animGraphInstance, OUTPUTPORT_RESULT)->GetValue(); }

        const char* GetPaletteName() const override;
        AnimGraphObject::ECategory GetPaletteCategory() const override;

        void SetTargetNodeName(const AZStd::string& targetNodeName);
        void SetMinTranslation(const AZ::Vector3& minTranslation);
        void SetMaxTranslation(const AZ::Vector3& maxTranslation);
        void SetMinRotation(const AZ::Vector3& minRotation);
        void SetMaxRotation(const AZ::Vector3& maxRotation);
        void SetMinScale(const AZ::Vector3& minScale);
        void SetMaxScale(const AZ::Vector3& maxScale);

        static void Reflect(AZ::ReflectContext* context);

    private:
        void UpdateUniqueData(AnimGraphInstance* animGraphInstance, UniqueData* uniqueData);
        void Output(AnimGraphInstance* animGraphInstance) override;

        AZStd::string       m_targetNodeName;
        AZ::Vector3         m_minTranslation;
        AZ::Vector3         m_maxTranslation;
        AZ::Vector3         m_minRotation;
        AZ::Vector3         m_maxRotation;
        AZ::Vector3         m_minScale;
        AZ::Vector3         m_maxScale;
    };
}   // namespace EMotionFX
