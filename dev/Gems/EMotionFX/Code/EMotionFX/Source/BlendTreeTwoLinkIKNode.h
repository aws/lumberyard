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
     */
    class EMFX_API BlendTreeTwoLinkIKNode
        : public AnimGraphNode
    {
        MCORE_MEMORYOBJECTCATEGORY(BlendTreeTwoLinkIKNode, EMFX_DEFAULT_ALIGNMENT, EMFX_MEMCATEGORY_ANIMGRAPH_BLENDTREENODES);

    public:
        AZ_RTTI(BlendTreeTwoLinkIKNode, "{0C3E8B7F-F810-47A6-B1A9-27BD4E4B5500}", AnimGraphNode);

        enum
        {
            TYPE_ID = 0x00001286
        };

        enum
        {
            ATTRIB_ENDNODE          = 0,    // the end node, for example the foot or hand
            ATTRIB_ENDEFFECTORNODE  = 1,    // the end effector node, for example the foot or hand, or a helper child of it
            ATTRIB_ALIGNNODE        = 2,    // the node to align the end effector to (optional)
            ATTRIB_BENDDIRNODE      = 3,    // the bend direction node
            ATTRIB_ENABLEROTATION   = 4,    // enable the goal rotation?
            ATTRIB_RELBENDDIR       = 5,    // use relative bend direction?
            ATTRIB_EXTRACTBENDDIR   = 6     // extract bend dir from the input pose?
        };

        enum
        {
            INPUTPORT_POSE      = 0,
            INPUTPORT_GOALPOS   = 1,
            INPUTPORT_GOALROT   = 2,
            INPUTPORT_BENDDIR   = 3,
            INPUTPORT_WEIGHT    = 4,
            OUTPUTPORT_POSE     = 0
        };

        enum
        {
            PORTID_INPUT_POSE       = 0,
            PORTID_INPUT_GOALPOS    = 1,
            PORTID_INPUT_GOALROT    = 2,
            PORTID_INPUT_BENDDIR    = 3,
            PORTID_INPUT_WEIGHT     = 4,
            PORTID_OUTPUT_POSE      = 0
        };

        class EMFX_API UniqueData
            : public AnimGraphNodeData
        {
            EMFX_ANIMGRAPHOBJECTDATA_IMPLEMENT_LOADSAVE
        public:
            UniqueData(AnimGraphNode* node, AnimGraphInstance* animGraphInstance)
                : AnimGraphNodeData(node, animGraphInstance)
            {
                mNodeIndexA             = MCORE_INVALIDINDEX32;
                mNodeIndexB             = MCORE_INVALIDINDEX32;
                mNodeIndexC             = MCORE_INVALIDINDEX32;
                mEndEffectorNodeIndex   = MCORE_INVALIDINDEX32;
                mAlignNodeIndex         = MCORE_INVALIDINDEX32;
                mBendDirNodeIndex       = MCORE_INVALIDINDEX32;
                mMustUpdate             = true;
                mIsValid                = false;
            }
            ~UniqueData() {}

            uint32 GetClassSize() const override                                                                                    { return sizeof(UniqueData); }
            AnimGraphObjectData* Clone(void* destMem, AnimGraphObject* object, AnimGraphInstance* animGraphInstance) override        { return new (destMem) UniqueData(static_cast<AnimGraphNode*>(object), animGraphInstance); }

        public:
            uint32  mNodeIndexA;
            uint32  mNodeIndexB;
            uint32  mNodeIndexC;
            uint32  mEndEffectorNodeIndex;
            uint32  mAlignNodeIndex;
            uint32  mBendDirNodeIndex;
            bool    mMustUpdate;
            bool    mIsValid;
        };

        static BlendTreeTwoLinkIKNode* Create(AnimGraph* animGraph);

        void Init(AnimGraphInstance* animGraphInstance) override;
        void RegisterPorts() override;
        void RegisterAttributes() override;
        void OnUpdateUniqueData(AnimGraphInstance* animGraphInstance) override;
        void OnUpdateAttributes() override;
        bool GetSupportsVisualization() const override          { return true; }
        bool GetHasOutputPose() const override                  { return true; }
        bool GetSupportsDisable() const override                { return true; }
        uint32 GetVisualColor() const override                  { return MCore::RGBA(255, 0, 0); }
        AnimGraphPose* GetMainOutputPose(AnimGraphInstance* animGraphInstance) const override     { return GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue(); }

        const char* GetPaletteName() const override;
        AnimGraphObject::ECategory GetPaletteCategory() const override;

        const char* GetTypeString() const override;
        AnimGraphObject* Clone(AnimGraph* animGraph) override;
        AnimGraphObjectData* CreateObjectData() override;

    private:
        BlendTreeTwoLinkIKNode(AnimGraph* animGraph);
        ~BlendTreeTwoLinkIKNode();

        void UpdateUniqueData(AnimGraphInstance* animGraphInstance, UniqueData* uniqueData);
        void Output(AnimGraphInstance* animGraphInstance) override;
    };
} // namespace EMotionFX
