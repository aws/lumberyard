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
#include "BlendTreeMaskNode.h"
#include "AnimGraphInstance.h"
#include "Actor.h"
#include "ActorInstance.h"
#include "AnimGraphAttributeTypes.h"
#include "AnimGraphManager.h"
#include "AnimGraph.h"
#include "EMotionFXManager.h"
#include "Node.h"
#include <MCore/Source/AttributeSettings.h>

namespace EMotionFX
{
    // constructor
    BlendTreeMaskNode::BlendTreeMaskNode(AnimGraph* animGraph)
        : AnimGraphNode(animGraph, nullptr, TYPE_ID)
    {
        // allocate space for the variables
        CreateAttributeValues();
        RegisterPorts();
        InitInternalAttributesForAllInstances();
    }


    // destructor
    BlendTreeMaskNode::~BlendTreeMaskNode()
    {
    }


    // create
    BlendTreeMaskNode* BlendTreeMaskNode::Create(AnimGraph* animGraph)
    {
        return new BlendTreeMaskNode(animGraph);
    }


    // create unique data
    AnimGraphObjectData* BlendTreeMaskNode::CreateObjectData()
    {
        return new UniqueData(this, nullptr);
    }


    // register the ports
    void BlendTreeMaskNode::RegisterPorts()
    {
        // setup the input ports
        InitInputPorts(4);
        SetupInputPort("Pose 0", INPUTPORT_POSE_0, AttributePose::TYPE_ID, PORTID_INPUT_POSE_0);
        SetupInputPort("Pose 1", INPUTPORT_POSE_1, AttributePose::TYPE_ID, PORTID_INPUT_POSE_1);
        SetupInputPort("Pose 2", INPUTPORT_POSE_2, AttributePose::TYPE_ID, PORTID_INPUT_POSE_2);
        SetupInputPort("Pose 3", INPUTPORT_POSE_3, AttributePose::TYPE_ID, PORTID_INPUT_POSE_3);

        // setup the output ports
        InitOutputPorts(1);
        SetupOutputPortAsPose("Output Pose", OUTPUTPORT_RESULT, PORTID_OUTPUT_RESULT);
    }


    // register the parameters
    void BlendTreeMaskNode::RegisterAttributes()
    {
        MCore::AttributeSettings* attrib;

        attrib = RegisterAttribute("Mask 0", "mask0", "The mask to apply on the Pose 0 input port.", ATTRIBUTE_INTERFACETYPE_NODENAMES);
        attrib->SetDefaultValue(AttributeNodeMask::Create());

        attrib = RegisterAttribute("Mask 1", "mask1", "The mask to apply on the Pose 1 input port.", ATTRIBUTE_INTERFACETYPE_NODENAMES);
        attrib->SetDefaultValue(AttributeNodeMask::Create());

        attrib = RegisterAttribute("Mask 2", "mask2", "The mask to apply on the Pose 2 input port.", ATTRIBUTE_INTERFACETYPE_NODENAMES);
        attrib->SetDefaultValue(AttributeNodeMask::Create());

        attrib = RegisterAttribute("Mask 3", "mask3", "The mask to apply on the Pose 3 input port.", ATTRIBUTE_INTERFACETYPE_NODENAMES);
        attrib->SetDefaultValue(AttributeNodeMask::Create());

        attrib = RegisterAttribute("Output Events 1", "events1", "Output events of the first input port?", MCore::ATTRIBUTE_INTERFACETYPE_CHECKBOX);
        attrib->SetDefaultValue(MCore::AttributeFloat::Create(1));

        attrib = RegisterAttribute("Output Events 2", "events2", "Output events of the second input port?", MCore::ATTRIBUTE_INTERFACETYPE_CHECKBOX);
        attrib->SetDefaultValue(MCore::AttributeFloat::Create(1));

        attrib = RegisterAttribute("Output Events 3", "events3", "Output events of the third input port?", MCore::ATTRIBUTE_INTERFACETYPE_CHECKBOX);
        attrib->SetDefaultValue(MCore::AttributeFloat::Create(1));

        attrib = RegisterAttribute("Output Events 4", "events4", "Output events of the fourth input port?", MCore::ATTRIBUTE_INTERFACETYPE_CHECKBOX);
        attrib->SetDefaultValue(MCore::AttributeFloat::Create(1));
    }


    // get the palette name
    const char* BlendTreeMaskNode::GetPaletteName() const
    {
        return "Pose Mask";
    }


    // get the category
    AnimGraphObject::ECategory BlendTreeMaskNode::GetPaletteCategory() const
    {
        return AnimGraphObject::CATEGORY_BLENDING;
    }


    // create a clone of this node
    AnimGraphObject* BlendTreeMaskNode::Clone(AnimGraph* animGraph)
    {
        // create the clone
        BlendTreeMaskNode* clone = new BlendTreeMaskNode(animGraph);

        // copy base class settings such as parameter values to the new clone
        CopyBaseObjectTo(clone);

        // return a pointer to the clone
        return clone;
    }


    // init and prealloc data
    void BlendTreeMaskNode::Init(AnimGraphInstance* animGraphInstance)
    {
        MCORE_UNUSED(animGraphInstance);
        //mOutputPose.Init( animGraphInstance->GetActorInstance() );
    }


    // precreate the unique data object
    void BlendTreeMaskNode::OnUpdateUniqueData(AnimGraphInstance* animGraphInstance)
    {
        // find the unique data for this node, if it doesn't exist yet, create it
        UniqueData* uniqueData = static_cast<BlendTreeMaskNode::UniqueData*>(animGraphInstance->FindUniqueObjectData(this));
        if (uniqueData == nullptr)
        {
            //uniqueData = new UniqueData(this, animGraphInstance);
            uniqueData = (UniqueData*)GetEMotionFX().GetAnimGraphManager()->GetObjectDataPool().RequestNew(TYPE_ID, this, animGraphInstance);
            animGraphInstance->RegisterUniqueObjectData(uniqueData);
        }

        uniqueData->mMasks.Resize(4);
        uniqueData->mMustUpdate = true;
        UpdateUniqueData(animGraphInstance, uniqueData);
    }


    // perform the calculations / actions
    void BlendTreeMaskNode::Output(AnimGraphInstance* animGraphInstance)
    {
        AnimGraphPose* outputPose;

        // update the unique data if needed
        UniqueData* uniqueData = static_cast<UniqueData*>(FindUniqueNodeData(animGraphInstance));
        UpdateUniqueData(animGraphInstance, uniqueData);

        // for all input ports
        for (uint32 i = 0; i < 4; ++i)
        {
            // if there is no connection plugged in
            if (mInputPorts[INPUTPORT_POSE_0 + i].mConnection == nullptr)
            {
                continue;
            }

            // calculate output
            OutputIncomingNode(animGraphInstance, GetInputNode(INPUTPORT_POSE_0 + i));
        }

        // init the output pose with the bind pose for safety
        RequestPoses(animGraphInstance);
        outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_RESULT)->GetValue();
        outputPose->InitFromBindPose(animGraphInstance->GetActorInstance());

        for (uint32 i = 0; i < 4; ++i)
        {
            // if there is no connection plugged in
            if (mInputPorts[INPUTPORT_POSE_0 + i].mConnection == nullptr)
            {
                continue;
            }

            const AnimGraphPose* pose = GetInputPose(animGraphInstance, INPUTPORT_POSE_0 + i)->GetValue();

            Pose& outputLocalPose = outputPose->GetPose();
            const Pose& localPose = pose->GetPose();

            // get the number of nodes inside the mask and default them to all nodes in the local pose in case there aren't any selected
            uint32 numNodes = uniqueData->mMasks[i].GetLength();
            if (numNodes > 0)
            {
                // for all nodes in the mask, output their transforms
                for (uint32 n = 0; n < numNodes; ++n)
                {
                    const uint32 nodeIndex = uniqueData->mMasks[i][n];
                    outputLocalPose.SetLocalTransform(nodeIndex, localPose.GetLocalTransform(nodeIndex));
                }
            }
            else
            {
                outputLocalPose = localPose;
            }
        }

        // visualize it
    #ifdef EMFX_EMSTUDIOBUILD
        if (GetCanVisualize(animGraphInstance))
        {
            animGraphInstance->GetActorInstance()->DrawSkeleton(outputPose->GetPose(), mVisualizeColor);
        }
    #endif
    }


    // update
    void BlendTreeMaskNode::Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        // update all incoming nodes
        UpdateAllIncomingNodes(animGraphInstance, timePassedInSeconds);

        // init the sync track etc to the first input
        AnimGraphNodeData* uniqueData = FindUniqueNodeData(animGraphInstance);
        AnimGraphNode* inputNode = GetInputNode(INPUTPORT_POSE_0);
        if (inputNode)
        {
            uniqueData->Init(animGraphInstance, inputNode);
        }
        else
        {
            uniqueData->Clear();
        }
    }


    // post update
    void BlendTreeMaskNode::PostUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        // post update all incoming nodes
        for (uint32 i = 0; i < 4; ++i)
        {
            // if the port has no input, skip it
            AnimGraphNode* inputNode = GetInputNode(INPUTPORT_POSE_0 + i);
            if (inputNode == nullptr)
            {
                continue;
            }

            // post update the input node first
            inputNode->PerformPostUpdate(animGraphInstance, timePassedInSeconds);
        }

        // request the reference counted data inside the unique data
        RequestRefDatas(animGraphInstance);
        UniqueData* uniqueData = static_cast<UniqueData*>(FindUniqueNodeData(animGraphInstance));
        AnimGraphRefCountedData* data = uniqueData->GetRefCountedData();
        data->ClearEventBuffer();
        data->ZeroTrajectoryDelta();

        for (uint32 i = 0; i < 4; ++i)
        {
            // if the port has no input, skip it
            AnimGraphNode* inputNode = GetInputNode(INPUTPORT_POSE_0 + i);
            if (inputNode == nullptr)
            {
                continue;
            }

            // get the number of nodes inside the mask and default them to all nodes in the local pose in case there aren't any selected
            const uint32 numNodes = uniqueData->mMasks[i].GetLength();
            if (numNodes > 0)
            {
                // for all nodes in the mask, output their transforms
                for (uint32 n = 0; n < numNodes; ++n)
                {
                    const uint32 nodeIndex = uniqueData->mMasks[i][n];
                    if (nodeIndex == animGraphInstance->GetActorInstance()->GetActor()->GetMotionExtractionNodeIndex())
                    {
                        AnimGraphRefCountedData* sourceData = inputNode->FindUniqueNodeData(animGraphInstance)->GetRefCountedData();

                        data->SetTrajectoryDelta(sourceData->GetTrajectoryDelta());
                        data->SetTrajectoryDeltaMirrored(sourceData->GetTrajectoryDeltaMirrored());
                        break;
                    }
                }
            }
            else
            {
                AnimGraphRefCountedData* sourceData = inputNode->FindUniqueNodeData(animGraphInstance)->GetRefCountedData();
                data->SetTrajectoryDelta(sourceData->GetTrajectoryDelta());
                data->SetTrajectoryDeltaMirrored(sourceData->GetTrajectoryDeltaMirrored());
            }

            // if we want to output events for this input
            // basically add the incoming events to the output event buffer
            if (GetAttributeFloatAsBool(ATTRIB_EVENTS_0 + i))
            {
                // get the input event buffer
                const AnimGraphEventBuffer& inputEventBuffer = inputNode->FindUniqueNodeData(animGraphInstance)->GetRefCountedData()->GetEventBuffer();
                AnimGraphEventBuffer& outputEventBuffer = data->GetEventBuffer();
                const uint32 startIndex = outputEventBuffer.GetNumEvents();

                // resize the buffer already, so that we don't do this for every event
                outputEventBuffer.Resize(outputEventBuffer.GetNumEvents() + inputEventBuffer.GetNumEvents());

                // copy over all the events
                const uint32 numInputEvents = inputEventBuffer.GetNumEvents();
                for (uint32 e = 0; e < numInputEvents; ++e)
                {
                    outputEventBuffer.SetEvent(startIndex + e, inputEventBuffer.GetEvent(e));
                }
            }
        }
    }


    // get the blend node type string
    const char* BlendTreeMaskNode::GetTypeString() const
    {
        return "BlendTreeMaskNode";
    }


    // when the attributes change
    void BlendTreeMaskNode::OnUpdateAttributes()
    {
        const uint32 numAttribValues = mAttributeValues.GetLength();
        if (numAttribValues == 0)
        {
            return;
        }

        // get the number of anim graph instances and iterate through them
        const uint32 numAnimGraphInstances = mAnimGraph->GetNumAnimGraphInstances();
        for (uint32 b = 0; b < numAnimGraphInstances; ++b)
        {
            // get the current anim graph instance
            AnimGraphInstance* animGraphInstance = mAnimGraph->GetAnimGraphInstance(b);

            UniqueData* uniqueData = reinterpret_cast<UniqueData*>(animGraphInstance->FindUniqueObjectData(this));
            if (uniqueData)
            {
                uniqueData->mMustUpdate = true;
            }
        }
    }


    // update the unique data
    void BlendTreeMaskNode::UpdateUniqueData(AnimGraphInstance* animGraphInstance, UniqueData* uniqueData)
    {
        // update the unique data if needed
        if (uniqueData->mMustUpdate)
        {
            // for all mask ports
            Actor* actor = animGraphInstance->GetActorInstance()->GetActor();
            for (uint32 i = 0; i < 4; ++i)
            {
                // get the array of strings with node names
                const MCore::Attribute* attrib = GetAttribute(ATTRIB_MASK_0 + i);
                const AttributeNodeMask* nodeMask = static_cast<const AttributeNodeMask*>(attrib);

                // update the unique data
                const uint32 numNodes = nodeMask->GetNumNodes();
                MCore::Array<uint32>& nodeIndices = uniqueData->mMasks[i];
                nodeIndices.Clear();
                nodeIndices.Reserve(numNodes);

                // for all nodes in the mask, try to find the related nodes inside the actor
                Skeleton* skeleton = actor->GetSkeleton();
                for (uint32 a = 0; a < numNodes; ++a)
                {
                    Node* node = skeleton->FindNodeByName(nodeMask->GetNode(a).mName.AsChar());
                    if (node)
                    {
                        nodeIndices.Add(node->GetNodeIndex());
                    }
                }
            }

            // don't update the next time again
            uniqueData->mMustUpdate = false;
        }
    }
}   // namespace EMotionFX

