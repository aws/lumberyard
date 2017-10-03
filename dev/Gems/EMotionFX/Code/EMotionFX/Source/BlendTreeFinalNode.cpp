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

// include required headers
#include "EMotionFXConfig.h"
#include "BlendTreeFinalNode.h"
#include "BlendTree.h"
#include "AnimGraphNode.h"
#include "AnimGraphAttributeTypes.h"


namespace EMotionFX
{
    // constructor
    BlendTreeFinalNode::BlendTreeFinalNode(AnimGraph* animGraph)
        : AnimGraphNode(animGraph, nullptr, TYPE_ID)
    {
        // allocate space for the variables
        CreateAttributeValues();
        RegisterPorts();
        InitInternalAttributesForAllInstances();
    }


    // destructor
    BlendTreeFinalNode::~BlendTreeFinalNode()
    {
    }


    // create
    BlendTreeFinalNode* BlendTreeFinalNode::Create(AnimGraph* animGraph)
    {
        return new BlendTreeFinalNode(animGraph);
    }


    // create the unique data
    AnimGraphObjectData* BlendTreeFinalNode::CreateObjectData()
    {
        return AnimGraphNodeData::Create(this, nullptr);
    }


    // register the ports
    void BlendTreeFinalNode::RegisterPorts()
    {
        // setup the input ports
        InitInputPorts(1);
        SetupInputPort("Input Pose", INPUTPORT_POSE, AttributePose::TYPE_ID, PORTID_INPUT_POSE);

        // setup the output ports
        InitOutputPorts(1);
        SetupOutputPortAsPose("Output", OUTPUTPORT_RESULT, PORTID_OUTPUT_POSE);
    }


    // register the parameters
    void BlendTreeFinalNode::RegisterAttributes()
    {
    }


    // get the palette name
    const char* BlendTreeFinalNode::GetPaletteName() const
    {
        return "Final Output";
    }


    // get the category
    AnimGraphObject::ECategory BlendTreeFinalNode::GetPaletteCategory() const
    {
        return AnimGraphObject::CATEGORY_MISC;
    }


    // create a clone of this node
    AnimGraphObject* BlendTreeFinalNode::Clone(AnimGraph* animGraph)
    {
        // create the clone
        BlendTreeFinalNode* clone = new BlendTreeFinalNode(animGraph);

        // copy base class settings such as parameter values to the new clone
        CopyBaseObjectTo(clone);

        // return a pointer to the clone
        return clone;
    }


    // init
    void BlendTreeFinalNode::Init(AnimGraphInstance* animGraphInstance)
    {
        MCORE_UNUSED(animGraphInstance);
        //  mOutputPose.Init( animGraphInstance->GetActorInstance() );
    }


    // the main process method of the final node
    void BlendTreeFinalNode::Output(AnimGraphInstance* animGraphInstance)
    {
        AnimGraphPose* outputPose;

        // if there is no input, just output a bind pose
        if (mConnections.GetLength() == 0)
        {
            RequestPoses(animGraphInstance);
            outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_RESULT)->GetValue();
            outputPose->InitFromBindPose(animGraphInstance->GetActorInstance());
            return;
        }

        // output the source node
        AnimGraphNode* sourceNode = mConnections[0]->GetSourceNode();
        OutputIncomingNode(animGraphInstance, sourceNode);

        RequestPoses(animGraphInstance);
        outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_RESULT)->GetValue();
        *outputPose = *sourceNode->GetMainOutputPose(animGraphInstance);
    }


    // update the node
    void BlendTreeFinalNode::Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        // if there are no connections, output nothing
        if (mConnections.GetLength() == 0)
        {
            AnimGraphNodeData* uniqueData = FindUniqueNodeData(animGraphInstance);
            uniqueData->Clear();
            return;
        }

        // update the source node
        AnimGraphNode* sourceNode = mConnections[0]->GetSourceNode();
        UpdateIncomingNode(animGraphInstance, sourceNode, timePassedInSeconds);

        // update the sync track
        AnimGraphNodeData* uniqueData = FindUniqueNodeData(animGraphInstance);
        uniqueData->Init(animGraphInstance, sourceNode);
    }


    // get the type string
    const char* BlendTreeFinalNode::GetTypeString() const
    {
        return "BlendTreeFinalNode";
    }
}   // namespace EMotionFX
