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
#include "BlendTreeRetargetPoseNode.h"
#include "AnimGraphInstance.h"
#include "Actor.h"
#include "ActorInstance.h"
#include "AnimGraphAttributeTypes.h"
#include "EventManager.h"
#include "RetargetSetup.h"


namespace EMotionFX
{
    // constructor
    BlendTreeRetargetPoseNode::BlendTreeRetargetPoseNode(AnimGraph* animGraph)
        : AnimGraphNode(animGraph, nullptr, TYPE_ID)
    {
        // allocate space for the variables
        CreateAttributeValues();
        RegisterPorts();
        InitInternalAttributesForAllInstances();
    }


    // destructor
    BlendTreeRetargetPoseNode::~BlendTreeRetargetPoseNode()
    {
    }


    // create
    BlendTreeRetargetPoseNode* BlendTreeRetargetPoseNode::Create(AnimGraph* animGraph)
    {
        return new BlendTreeRetargetPoseNode(animGraph);
    }


    // create unique data
    AnimGraphObjectData* BlendTreeRetargetPoseNode::CreateObjectData()
    {
        return AnimGraphNodeData::Create(this, nullptr);
    }


    // register the ports
    void BlendTreeRetargetPoseNode::RegisterPorts()
    {
        // setup the output ports
        InitInputPorts(2);
        SetupInputPort("Input Pose", INPUTPORT_POSE, AttributePose::TYPE_ID, PORTID_INPUT_POSE);
        SetupInputPortAsNumber("Enabled", INPUTPORT_ENABLED, PORTID_INPUT_ENABLED);

        InitOutputPorts(1);
        SetupOutputPortAsPose("Output Pose", OUTPUTPORT_RESULT, PORTID_OUTPUT_POSE);
    }


    // register the parameters
    void BlendTreeRetargetPoseNode::RegisterAttributes()
    {
    }


    // get the palette name
    const char* BlendTreeRetargetPoseNode::GetPaletteName() const
    {
        return "Retarget Pose";
    }


    // get the category
    AnimGraphObject::ECategory BlendTreeRetargetPoseNode::GetPaletteCategory() const
    {
        return AnimGraphObject::CATEGORY_MISC;
    }


    // create a clone of this node
    AnimGraphObject* BlendTreeRetargetPoseNode::Clone(AnimGraph* animGraph)
    {
        // create the clone
        BlendTreeRetargetPoseNode* clone = new BlendTreeRetargetPoseNode(animGraph);

        // copy base class settings such as parameter values to the new clone
        CopyBaseObjectTo(clone);

        // return a pointer to the clone
        return clone;
    }


    // pre-create unique data object
    void BlendTreeRetargetPoseNode::Init(AnimGraphInstance* animGraphInstance)
    {
        MCORE_UNUSED(animGraphInstance);
    }


    // check if retargeting is enabled
    bool BlendTreeRetargetPoseNode::GetIsRetargetEnabled(AnimGraphInstance* animGraphInstance) const
    {
        // check the enabled state
        bool isEnabled = true;
        if (mInputPorts[INPUTPORT_ENABLED].mConnection)
        {
            isEnabled = GetInputNumberAsBool(animGraphInstance, INPUTPORT_ENABLED);
        }

        return isEnabled;
    }


    // perform the update
    void BlendTreeRetargetPoseNode::Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        AnimGraphNode* sourceNode = GetInputNode(INPUTPORT_POSE);
        if (sourceNode == nullptr)
        {
            AnimGraphNodeData* uniqueData = FindUniqueNodeData(animGraphInstance);
            uniqueData->Clear();
            return;
        }

        // update the source node
        UpdateAllIncomingNodes(animGraphInstance, timePassedInSeconds);

        // init the unique data
        AnimGraphNodeData* uniqueData = FindUniqueNodeData(animGraphInstance);
        uniqueData->Init(animGraphInstance, sourceNode);
    }


    // perform the calculations / actions
    void BlendTreeRetargetPoseNode::Output(AnimGraphInstance* animGraphInstance)
    {
        if (mInputPorts[INPUTPORT_POSE].mConnection == nullptr)
        {
            // get the output pose
            RequestPoses(animGraphInstance);
            AnimGraphPose* outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_RESULT)->GetValue();
            outputPose->InitFromBindPose(animGraphInstance->GetActorInstance());
            return;
        }

        // if we're disabled just forward the input pose
        if (mDisabled)
        {
            OutputIncomingNode(animGraphInstance, GetInputNode(INPUTPORT_POSE));
            const AnimGraphPose* inputPose = GetInputPose(animGraphInstance, INPUTPORT_POSE)->GetValue();
            // output the pose
            RequestPoses(animGraphInstance);
            AnimGraphPose* outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_RESULT)->GetValue();
            *outputPose = *inputPose;
            return;
        }

        // get the input pose
        OutputIncomingNode(animGraphInstance, GetInputNode(INPUTPORT_POSE));
        OutputIncomingNode(animGraphInstance, GetInputNode(INPUTPORT_ENABLED));

        const AnimGraphPose* inputPose = GetInputPose(animGraphInstance, INPUTPORT_POSE)->GetValue();
        const Actor* targetActor = animGraphInstance->GetActorInstance()->GetActor();

        // output the pose
        RequestPoses(animGraphInstance);
        AnimGraphPose* outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_RESULT)->GetValue();
        *outputPose = *inputPose; // TODO: is this really needed?

        // check the enabled state
        if (GetIsRetargetEnabled(animGraphInstance) /* && actor->HasRetargetInfo()*/)
        {
            RetargetSetup* retargetSetup = targetActor->GetRetargetSetup();
            retargetSetup->RetargetPoseAuto(&inputPose->GetPose(), &outputPose->GetPose());
        }

        // visualize it
    #ifdef EMFX_EMSTUDIOBUILD
        if (GetCanVisualize(animGraphInstance))
        {
            animGraphInstance->GetActorInstance()->DrawSkeleton(outputPose->GetPose(), mVisualizeColor);
        }
    #endif
    }


    // get the blend node type string
    const char* BlendTreeRetargetPoseNode::GetTypeString() const
    {
        return "BlendTreeRetargetPoseNode";
    }
}   // namespace EMotionFX

