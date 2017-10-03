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

// include the required headers
#include "EMotionFXConfig.h"
#include "AnimGraphBindPoseNode.h"
#include "AnimGraphInstance.h"
#include "Actor.h"
#include "ActorInstance.h"
#include "AnimGraphAttributeTypes.h"

namespace EMotionFX
{
    // constructor
    AnimGraphBindPoseNode::AnimGraphBindPoseNode(AnimGraph* animGraph)
        : AnimGraphNode(animGraph, nullptr, TYPE_ID)
    {
        // allocate space for the variables
        CreateAttributeValues();
        RegisterPorts();
        InitInternalAttributesForAllInstances();
    }


    // destructor
    AnimGraphBindPoseNode::~AnimGraphBindPoseNode()
    {
    }


    // create
    AnimGraphBindPoseNode* AnimGraphBindPoseNode::Create(AnimGraph* animGraph)
    {
        return new AnimGraphBindPoseNode(animGraph);
    }


    // create unique data
    AnimGraphObjectData* AnimGraphBindPoseNode::CreateObjectData()
    {
        return AnimGraphNodeData::Create(this, nullptr);
    }


    // register the ports
    void AnimGraphBindPoseNode::RegisterPorts()
    {
        // setup the output ports
        InitOutputPorts(1);
        SetupOutputPortAsPose("Output Pose", OUTPUTPORT_RESULT, PORTID_OUTPUT_POSE);
    }


    // register the parameters
    void AnimGraphBindPoseNode::RegisterAttributes()
    {
    }


    // get the palette name
    const char* AnimGraphBindPoseNode::GetPaletteName() const
    {
        return "Bind Pose";
    }


    // get the category
    AnimGraphObject::ECategory AnimGraphBindPoseNode::GetPaletteCategory() const
    {
        return AnimGraphObject::CATEGORY_SOURCES;
    }


    // create a clone of this node
    AnimGraphObject* AnimGraphBindPoseNode::Clone(AnimGraph* animGraph)
    {
        // create the clone
        AnimGraphBindPoseNode* clone = new AnimGraphBindPoseNode(animGraph);

        // copy base class settings such as parameter values to the new clone
        CopyBaseObjectTo(clone);

        // return a pointer to the clone
        return clone;
    }


    // pre-create unique data object
    void AnimGraphBindPoseNode::Init(AnimGraphInstance* animGraphInstance)
    {
        MCORE_UNUSED(animGraphInstance);
        //mOutputPose.Init( animGraphInstance->GetActorInstance() );
    }


    // perform the calculations / actions
    void AnimGraphBindPoseNode::Output(AnimGraphInstance* animGraphInstance)
    {
        // get the output pose
        RequestPoses(animGraphInstance);
        AnimGraphPose* outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_RESULT)->GetValue();

        outputPose->InitFromBindPose(animGraphInstance->GetActorInstance());

        // visualize it
    #ifdef EMFX_EMSTUDIOBUILD
        if (GetCanVisualize(animGraphInstance))
        {
            animGraphInstance->GetActorInstance()->DrawSkeleton(outputPose->GetPose(), mVisualizeColor);
        }
    #endif
    }


    // get the blend node type string
    const char* AnimGraphBindPoseNode::GetTypeString() const
    {
        return "AnimGraphBindPoseNode";
    }
}   // namespace EMotionFX

