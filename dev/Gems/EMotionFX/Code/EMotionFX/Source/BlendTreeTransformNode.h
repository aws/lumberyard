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
    class EMFX_API BlendTreeTransformNode
        : public AnimGraphNode
    {
        MCORE_MEMORYOBJECTCATEGORY(BlendTreeTransformNode, EMFX_DEFAULT_ALIGNMENT, EMFX_MEMCATEGORY_ANIMGRAPH_BLENDTREENODES);
    public:
        AZ_RTTI(BlendTreeTransformNode, "{348DB122-ABA7-4ED8-BB20-0F9560F7FA6B}", AnimGraphNode);

        enum
        {
            TYPE_ID = 0x00012345
        };

        // attributes
        enum
        {
            ATTRIB_NODE             = 0,
            ATTRIB_MINTRANSLATE     = 1,
            ATTRIB_MAXTRANSLATE     = 2,
            ATTRIB_MINROTATE        = 3,
            ATTRIB_MAXROTATE        = 4,
            ATTRIB_MINSCALE         = 5,
            ATTRIB_MAXSCALE         = 6
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
                : AnimGraphNodeData(node, animGraphInstance)     { mNodeIndex = MCORE_INVALIDINDEX32; mMustUpdate = true; mIsValid = false; }
            ~UniqueData() {}

            uint32 GetClassSize() const override                                                                                        { return sizeof(UniqueData); }
            AnimGraphObjectData* Clone(void* destMem, AnimGraphObject* object, AnimGraphInstance* animGraphInstance) override            { return new (destMem) UniqueData(static_cast<AnimGraphNode*>(object), animGraphInstance); }

        public:
            uint32          mNodeIndex;
            bool            mMustUpdate;
            bool            mIsValid;
        };

        static BlendTreeTransformNode* Create(AnimGraph* animGraph);

        void Init(AnimGraphInstance* animGraphInstance) override;
        void RegisterPorts() override;
        void RegisterAttributes() override;
        void OnUpdateUniqueData(AnimGraphInstance* animGraphInstance) override;

        uint32 GetVisualColor() const override                  { return MCore::RGBA(255, 0, 0); }
        bool GetCanActAsState() const override                  { return false; }
        bool GetSupportsVisualization() const override          { return true; }
        bool GetHasOutputPose() const override                  { return true; }
        AnimGraphObjectData* CreateObjectData() override;
        AnimGraphPose* GetMainOutputPose(AnimGraphInstance* animGraphInstance) const override     { return GetOutputPose(animGraphInstance, OUTPUTPORT_RESULT)->GetValue(); }

        const char* GetPaletteName() const override;
        AnimGraphObject::ECategory GetPaletteCategory() const override;

        const char* GetTypeString() const override;
        AnimGraphObject* Clone(AnimGraph* animGraph) override;

    private:
        BlendTreeTransformNode(AnimGraph* animGraph);
        ~BlendTreeTransformNode();

        void UpdateUniqueData(AnimGraphInstance* animGraphInstance, UniqueData* uniqueData);
        void Output(AnimGraphInstance* animGraphInstance) override;
    };
}   // namespace EMotionFX
