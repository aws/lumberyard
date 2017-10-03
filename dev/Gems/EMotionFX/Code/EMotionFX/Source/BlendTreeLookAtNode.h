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

#include "EMotionFXConfig.h"
#include "AnimGraphNode.h"

#include <AzCore/Math/Vector2.h>


namespace EMotionFX
{
    /**
     *
     */
    class EMFX_API BlendTreeLookAtNode
        : public AnimGraphNode
    {
        MCORE_MEMORYOBJECTCATEGORY(BlendTreeLookAtNode, EMFX_DEFAULT_ALIGNMENT, EMFX_MEMCATEGORY_ANIMGRAPH_BLENDTREENODES);

    public:
        AZ_RTTI(BlendTreeLookAtNode, "{7FBBFD4A-3B17-47D6-8419-8F8F5B89C1B3}", AnimGraphNode);

        enum
        {
            TYPE_ID = 0x00040360
        };

        enum
        {
            ATTRIB_NODE,                // the node to apply the lookat on, for example the head
            ATTRIB_YAWPITCHROLL_MIN,    // minimum angles in degrees
            ATTRIB_YAWPITCHROLL_MAX,    // maximum angles in degrees
            ATTRIB_CONSTRAINTROTATION,  // the rotation to move into constraint space
            ATTRIB_POSTROTATION,        // a relative post rotation
            ATTRIB_SPEED,               // follow speed
            ATTRIB_TWISTAXIS,           // twist axis
            ATTRIB_CONSTRAINTS          // enable constraints?
        };

        enum
        {
            INPUTPORT_POSE      = 0,
            INPUTPORT_GOALPOS   = 1,
            INPUTPORT_WEIGHT    = 2,
            OUTPUTPORT_POSE     = 0
        };

        enum
        {
            PORTID_INPUT_POSE       = 0,
            PORTID_INPUT_GOALPOS    = 1,
            PORTID_INPUT_WEIGHT     = 2,
            PORTID_OUTPUT_POSE      = 0
        };

        class EMFX_API UniqueData
            : public AnimGraphNodeData
        {
            EMFX_ANIMGRAPHOBJECTDATA_IMPLEMENT_LOADSAVE
        public:
            UniqueData(AnimGraphNode* node, AnimGraphInstance* animGraphInstance)
                : AnimGraphNodeData(node, animGraphInstance)     { mNodeIndex = MCORE_INVALIDINDEX32; mMustUpdate = true; mIsValid = false; mFirstUpdate = true; mTimeDelta = 0.0f; }
            ~UniqueData() {}

            uint32 GetClassSize() const override                                                                                        { return sizeof(UniqueData); }
            AnimGraphObjectData* Clone(void* destMem, AnimGraphObject* object, AnimGraphInstance* animGraphInstance) override            { return new (destMem) UniqueData(static_cast<AnimGraphNode*>(object), animGraphInstance); }

        public:
            MCore::Quaternion mRotationQuat;
            float       mTimeDelta;
            uint32      mNodeIndex;
            bool        mMustUpdate;
            bool        mIsValid;
            bool        mFirstUpdate;
        };

        static BlendTreeLookAtNode* Create(AnimGraph* animGraph);

        void Init(AnimGraphInstance* animGraphInstance) override;
        void RegisterPorts() override;
        void RegisterAttributes() override;
        void OnUpdateUniqueData(AnimGraphInstance* animGraphInstance) override;
        bool GetSupportsVisualization() const override              { return true; }
        bool GetHasOutputPose() const override                      { return true; }
        bool GetSupportsDisable() const override                    { return true; }
        uint32 GetVisualColor() const override                      { return MCore::RGBA(255, 0, 0); }
        AnimGraphPose* GetMainOutputPose(AnimGraphInstance* animGraphInstance) const override     { return GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue(); }

        const char* GetPaletteName() const override;
        AnimGraphObject::ECategory GetPaletteCategory() const override;

        const char* GetTypeString() const override;
        AnimGraphObject* Clone(AnimGraph* animGraph) override;
        AnimGraphObjectData* CreateObjectData() override;

        void OnUpdateAttributes() override;

    private:
        BlendTreeLookAtNode(AnimGraph* animGraph);
        ~BlendTreeLookAtNode();

        void UpdateUniqueData(AnimGraphInstance* animGraphInstance, UniqueData* uniqueData);
        void Output(AnimGraphInstance* animGraphInstance) override;
        void Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;
    };
}   // namespace EMotionFX
