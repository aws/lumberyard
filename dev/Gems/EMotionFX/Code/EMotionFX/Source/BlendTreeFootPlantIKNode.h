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
#include "TwoLinkIKSolver.h"


namespace EMotionFX
{
    /*
    class EMFX_API BlendTreeFootPlantIKNode : public AnimGraphNode
    {
        MCORE_MEMORYOBJECTCATEGORY(BlendTreeFootPlantIKNode, EMFX_DEFAULT_ALIGNMENT, EMFX_MEMCATEGORY_ANIMGRAPH_BLENDTREENODES);

        public:
            AZ_RTTI(BlendTreeFootPlantIKNode, "{E97D5B8F-F421-449C-A81C-5F529306E890}", AnimGraphNode);
            enum { TYPE_ID = 0x00002253 };

            enum
            {
                ATTRIB_LEFTFOOTNODE         = 0,    // the left foot node
                ATTRIB_LEFTENDEFFECTORNODE  = 1,    // the left end effector node (optional)
                ATTRIB_LEFTBENDDIRNODE      = 2,    // the left bend direction node (optional)
                ATTRIB_RIGHTFOOTNODE        = 3,    // the right foot node
                ATTRIB_RIGHTENDEFFECTORNODE = 4,    // the right end effector node (optional)
                ATTRIB_RIGHTBENDDIRNODE     = 5,    // the right bend direction node (optional)
                ATTRIB_PELVISNODE           = 6,    // the pelvis node
                ATTRIB_DISPLACEPELVIS       = 7     // allow pelvis displacement?
            };

            enum
            {
                INPUTPORT_POSE              = 0,
                INPUTPORT_FOOTHEIGHTOFFSET  = 1,
                INPUTPORT_WEIGHT            = 2,
                OUTPUTPORT_POSE             = 0
            };

            enum
            {
                PORTID_INPUT_POSE               = 0,
                PORTID_INPUT_WEIGHT             = 1,
                PORTID_INPUT_FOOTHEIGHTOFFSET   = 2,
                PORTID_OUTPUT_POSE              = 0
            };

            enum
            {
                LEG_LEFT    = 0,
                LEG_RIGHT   = 1,
                NUMLEGS     = 2
            };

            class EMFX_API UniqueData : public AnimGraphNodeData
            {
                EMFX_ANIMGRAPHOBJECTDATA_IMPLEMENT_LOADSAVE
                public:
                    UniqueData(AnimGraphNode* node, AnimGraphInstance* animGraphInstance) : AnimGraphNodeData(node, animGraphInstance)     { mIsValid=false; mMustUpdate=true; mPelvisIndex=MCORE_INVALIDINDEX32; }
                    ~UniqueData() {}

                    uint32 GetClassSize() const override                    { return sizeof(UniqueData); }
                    AnimGraphObjectData* Clone(void* destMem, AnimGraphObject* object, AnimGraphInstance* animGraphInstance) override        { return new (destMem) UniqueData(static_cast<AnimGraphNode*>(object), animGraphInstance); }

                public:
                    struct LegInfo
                    {
                        uint32  mNodeIndexA;
                        uint32  mNodeIndexB;
                        uint32  mNodeIndexC;
                        uint32  mEndEffectorNodeIndex;
                        uint32  mBendDirNodeIndex;
                        MCore::Array<uint32>    mRootUpdatePath;        // path from end effector to the root
                        MCore::Array<uint32>    mBendDirUpdatePath;     // path from bend dir node to the root
                        MCore::Array<uint32>    mUpdatePath;            // path from end effector towards end node
                        bool    mIsValid;

                        LegInfo()
                        {
                            mNodeIndexA             = MCORE_INVALIDINDEX32;
                            mNodeIndexB             = MCORE_INVALIDINDEX32;
                            mNodeIndexC             = MCORE_INVALIDINDEX32;
                            mEndEffectorNodeIndex   = MCORE_INVALIDINDEX32;
                            mBendDirNodeIndex       = MCORE_INVALIDINDEX32;
                            mIsValid                = false;
                        }
                    };

                    LegInfo mLegInfos[2];   // index using LEG_LEFT and LEG_RIGHT
                    uint32  mPelvisIndex;
                    bool    mMustUpdate;
                    bool    mIsValid;
            };

            static BlendTreeFootPlantIKNode* Create(AnimGraph* animGraph);

            void Init(AnimGraphInstance* animGraphInstance) override;
            void RegisterPorts() override;
            void RegisterAttributes() override;
            void OnUpdateUniqueData(AnimGraphInstance* animGraphInstance) override;
            bool GetSupportsVisualization() const override  { return true; }
            bool HasOutputPose() const override             { return true; }
            bool GetSupportsDisable() const override        { return true; }
            AnimGraphPose* GetMainOutputPose() const override  { return GetOutputPose(OUTPUTPORT_POSE)->GetValue(); }

            uint32 GetVisualColor() const override  { return MCore::RGBA(255, 0, 0); }

            const char* GetPaletteName() const override;
            AnimGraphObject::ECategory GetPaletteCategory() const override;

            const char* GetTypeString() const override;
            AnimGraphObject* Clone(AnimGraph* animGraph) override;
            AnimGraphObjectData* CreateObjectData() override;

        private:
            BlendTreeFootPlantIKNode(AnimGraph* animGraph);
            ~BlendTreeFootPlantIKNode();

            void UpdateUniqueData(AnimGraphInstance* animGraphInstance, UniqueData* uniqueData);
            bool BuildEndEffectorUpdatePath(AnimGraphInstance* animGraphInstance, UniqueData* uniqueData, uint32 legIndex);
            void Output(AnimGraphInstance* animGraphInstance) override;
            void ExtractMotion(AnimGraphInstance* animGraphInstance) override;
    };
    */
} // namespace EMotionFX
