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
#include "AnimGraphNode.h"


namespace EMotionFX
{
    /**
     *
     *
     *
     */
    class EMFX_API BlendTreeAccumTransformNode
        : public AnimGraphNode
    {
        MCORE_MEMORYOBJECTCATEGORY(BlendTreeAccumTransformNode, EMFX_DEFAULT_ALIGNMENT, EMFX_MEMCATEGORY_ANIMGRAPH_BLENDTREENODES);
    public:
        AZ_RTTI(BlendTreeAccumTransformNode, "{2216B366-F06C-4742-B998-44F4357F45BE}", AnimGraphNode);

        enum
        {
            TYPE_ID = 0x00012346
        };

        // attributes
        enum
        {
            ATTRIB_NODE                 = 0,
            ATTRIB_TRANSLATE_AXIS       = 1,
            ATTRIB_ROTATE_AXIS          = 2,
            ATTRIB_SCALE_AXIS           = 3,
            ATTRIB_TRANSLATE_SPEED      = 4,
            ATTRIB_ROTATE_SPEED         = 5,
            ATTRIB_SCALE_SPEED          = 6,
            ATTRIB_TRANSLATE_INVERT     = 7,
            ATTRIB_ROTATE_INVERT        = 8,
            ATTRIB_SCALE_INVERT         = 9
        };

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
            UniqueData(AnimGraphNode* node, AnimGraphInstance* animGraphInstance)
                : AnimGraphNodeData(node, animGraphInstance)
            {
                mDeltaTime  = 0.0f;
                mNodeIndex  = MCORE_INVALIDINDEX32;
                mMustUpdate = true;
                mIsValid    = false;
                mAdditiveTransform.Identity();
                EMFX_SCALECODE(mAdditiveTransform.mScale.CreateZero();)
            }

            ~UniqueData() {}

            uint32 GetClassSize() const override                                                                                    { return sizeof(UniqueData); }
            AnimGraphObjectData* Clone(void* destMem, AnimGraphObject* object, AnimGraphInstance* animGraphInstance) override        { return new (destMem) UniqueData(static_cast<AnimGraphNode*>(object), animGraphInstance); }

        public:
            Transform   mAdditiveTransform;
            uint32      mNodeIndex;
            float       mDeltaTime;
            bool        mMustUpdate;
            bool        mIsValid;
        };

        static BlendTreeAccumTransformNode* Create(AnimGraph* animGraph);

        void RegisterPorts() override;
        void RegisterAttributes() override;

        void OnUpdateUniqueData(AnimGraphInstance* animGraphInstance) override;

        uint32 GetVisualColor() const override                  { return MCore::RGBA(255, 0, 0); }
        bool GetCanActAsState() const override                  { return false; }
        bool GetSupportsVisualization() const override          { return true; }
        bool GetHasOutputPose() const override                  { return true; }

        AnimGraphPose* GetMainOutputPose(AnimGraphInstance* animGraphInstance) const override     { return GetOutputPose(animGraphInstance, OUTPUTPORT_RESULT)->GetValue(); }

        const char* GetPaletteName() const override;
        AnimGraphObject::ECategory GetPaletteCategory() const override;

        const char* GetTypeString() const override;
        AnimGraphObject* Clone(AnimGraph* animGraph) override;
        AnimGraphObjectData* CreateObjectData() override;

    private:
        int32           mLastTranslateAxis;
        int32           mLastRotateAxis;
        int32           mLastScaleAxis;

        BlendTreeAccumTransformNode(AnimGraph* animGraph);
        ~BlendTreeAccumTransformNode();

        void UpdateUniqueData(AnimGraphInstance* animGraphInstance, UniqueData* uniqueData);
        void Output(AnimGraphInstance* animGraphInstance) override;
        void Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;
    };
}   // namespace EMotionFX
