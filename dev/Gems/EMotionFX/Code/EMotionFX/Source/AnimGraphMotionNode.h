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
#include "AnimGraphNodeData.h"
#include "PlayBackInfo.h"

namespace EMotionFX
{
    // forward declarations
    class Motion;
    class ActorInstance;
    class MotionSet;

    /**
     *
     *
     */
    class EMFX_API AnimGraphMotionNode
        : public AnimGraphNode
    {
        MCORE_MEMORYOBJECTCATEGORY(AnimGraphMotionNode, EMFX_DEFAULT_ALIGNMENT, EMFX_MEMCATEGORY_ANIMGRAPH_BLENDTREENODES);

    public:
        AZ_RTTI(AnimGraphMotionNode, "{B8B8AAE6-E532-4BF8-898F-3D40AA41BC82}", AnimGraphNode);

        enum
        {
            TYPE_ID = 0x00000002
        };

        enum
        {
            ATTRIB_MOTION                       = 0,
            ATTRIB_LOOP                         = 1,
            ATTRIB_RETARGET                     = 2,
            ATTRIB_REVERSE                      = 3,
            ATTRIB_EVENTS                       = 4,
            ATTRIB_MIRROR                       = 5,
            ATTRIB_MOTIONEXTRACTION             = 6,
            ATTRIB_PLAYSPEED                    = 7,
            ATTRIB_INDEXMODE                    = 8,
            ATTRIB_NEXTMOTIONAFTEREND           = 9,
            ATTRIB_REWINDONZEROWEIGHT           = 10
        };

        enum
        {
            INPUTPORT_PLAYSPEED                 = 0,
            OUTPUTPORT_POSE                     = 0,
            OUTPUTPORT_MOTION                   = 1
        };

        enum
        {
            PORTID_INPUT_PLAYSPEED              = 0,
            PORTID_OUTPUT_POSE                  = 0,
            PORTID_OUTPUT_MOTION                = 1
        };

        enum
        {
            INDEXMODE_RANDOMIZE                 = 0,
            INDEXMODE_RANDOMIZE_NOREPEAT        = 1,
            INDEXMODE_SEQUENTIAL                = 2,
            INDEXMODE_NUMMETHODS
        };

        class EMFX_API UniqueData
            : public AnimGraphNodeData
        {
            EMFX_ANIMGRAPHOBJECTDATA_IMPLEMENT_LOADSAVE
        public:
            UniqueData(AnimGraphNode* node, AnimGraphInstance* animGraphInstance, uint32 motionSetID, MotionInstance* instance);
            ~UniqueData();

            uint32 GetClassSize() const override                                                                                    { return sizeof(UniqueData); }
            AnimGraphObjectData* Clone(void* destMem, AnimGraphObject* object, AnimGraphInstance* animGraphInstance) override        { return new (destMem) UniqueData(static_cast<AnimGraphNode*>(object), animGraphInstance, MCORE_INVALIDINDEX32, nullptr); }

            void Reset() override;

        public:
            uint32              mMotionSetID;
            uint32              mEventTrackIndex;
            uint32              mActiveMotionIndex;
            MotionInstance*     mMotionInstance;
            bool                mReload;
        };

        static AnimGraphMotionNode* Create(AnimGraph* animGraph);

        void Init(AnimGraphInstance* animGraphInstance) override;
        void Prepare(AnimGraphInstance* animGraphInstance) override;

        bool GetHasOutputPose() const override              { return true; }
        bool GetCanActAsState() const override              { return true; }
        bool GetSupportsDisable() const override            { return true; }
        bool GetSupportsVisualization() const override      { return true; }
        uint32 GetVisualColor() const override              { return MCore::RGBA(96, 61, 231); }

        void RegisterPorts() override;
        void RegisterAttributes() override;
        void OnUpdateAttributes() override;
        void OnUpdateUniqueData(AnimGraphInstance* animGraphInstance) override;
        void OnActorMotionExtractionNodeChanged() override;
        bool ConvertAttribute(uint32 attributeIndex, const MCore::Attribute* attributeToConvert, const MCore::String& attributeName) override;

        const char* GetTypeString() const override;
        AnimGraphNode* Clone(AnimGraph* animGraph) override;
        AnimGraphObjectData* CreateObjectData() override;

        const char* GetPaletteName() const override;
        AnimGraphObject::ECategory GetPaletteCategory() const override;

        MotionInstance* FindMotionInstance(AnimGraphInstance* animGraphInstance) const;

        AnimGraphPose* GetMainOutputPose(AnimGraphInstance* animGraphInstance) const override             { return GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue(); }

        void SetCurrentPlayTime(AnimGraphInstance* animGraphInstance, float timeInSeconds) override;
        void Rewind(AnimGraphInstance* animGraphInstance) override;

        void UpdatePlayBackInfo(AnimGraphInstance* animGraphInstance);

        float ExtractCustomPlaySpeed(AnimGraphInstance* animGraphInstance) const;
        void PickNewActiveMotion(UniqueData* uniqueData);

        AZ::u32 GetNumMotions() const;
        const char* GetMotionID(uint32 index) const;

    private:
        PlayBackInfo                    mPlayInfo;
        MCore::String                   mCurMotionArrayString;
        MCore::String                   mLastMotionArrayString;
        bool                            mLastLoop;
        bool                            mLastRetarget;
        bool                            mLastReverse;
        bool                            mLastEvents;
        bool                            mLastMirror;
        bool                            mLastMotionExtraction;

        AnimGraphMotionNode(AnimGraph* animGraph);
        ~AnimGraphMotionNode();

        MotionInstance* CreateMotionInstance(ActorInstance* actorInstance, AnimGraphInstance* animGraphInstance);
        void TopDownUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;
        void Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;
        void Output(AnimGraphInstance* animGraphInstance) override;
        void PostUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;
    };
}   // namespace EMotionFX
