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
#include "BlendTreePoseSwitchNode.h"
#include "AnimGraphInstance.h"
#include "AnimGraphAttributeTypes.h"
#include "AnimGraphManager.h"
#include "AnimGraphRefCountedData.h"
#include "EMotionFXManager.h"


namespace EMotionFX
{
    // constructor
    BlendTreePoseSwitchNode::BlendTreePoseSwitchNode(AnimGraph* animGraph)
        : AnimGraphNode(animGraph, nullptr, TYPE_ID)
    {
        // allocate space for the variables
        CreateAttributeValues();
        RegisterPorts();
        InitInternalAttributesForAllInstances();
    }


    // destructor
    BlendTreePoseSwitchNode::~BlendTreePoseSwitchNode()
    {
    }


    // create
    BlendTreePoseSwitchNode* BlendTreePoseSwitchNode::Create(AnimGraph* animGraph)
    {
        return new BlendTreePoseSwitchNode(animGraph);
    }


    // create unique data
    AnimGraphObjectData* BlendTreePoseSwitchNode::CreateObjectData()
    {
        return new UniqueData(this, nullptr, 0);
    }


    // register the ports
    void BlendTreePoseSwitchNode::RegisterPorts()
    {
        // setup input ports
        InitInputPorts(11);
        SetupInputPort("Pose 0", INPUTPORT_POSE_0, AttributePose::TYPE_ID,  PORTID_INPUT_POSE_0);
        SetupInputPort("Pose 1", INPUTPORT_POSE_1, AttributePose::TYPE_ID,  PORTID_INPUT_POSE_1);
        SetupInputPort("Pose 2", INPUTPORT_POSE_2, AttributePose::TYPE_ID,  PORTID_INPUT_POSE_2);
        SetupInputPort("Pose 3", INPUTPORT_POSE_3, AttributePose::TYPE_ID,  PORTID_INPUT_POSE_3);
        SetupInputPort("Pose 4", INPUTPORT_POSE_4, AttributePose::TYPE_ID,  PORTID_INPUT_POSE_4);
        SetupInputPort("Pose 5", INPUTPORT_POSE_5, AttributePose::TYPE_ID,  PORTID_INPUT_POSE_5);
        SetupInputPort("Pose 6", INPUTPORT_POSE_6, AttributePose::TYPE_ID,  PORTID_INPUT_POSE_6);
        SetupInputPort("Pose 7", INPUTPORT_POSE_7, AttributePose::TYPE_ID,  PORTID_INPUT_POSE_7);
        SetupInputPort("Pose 8", INPUTPORT_POSE_8, AttributePose::TYPE_ID,  PORTID_INPUT_POSE_8);
        SetupInputPort("Pose 9", INPUTPORT_POSE_9, AttributePose::TYPE_ID,  PORTID_INPUT_POSE_9);
        SetupInputPortAsNumber("Decision Value", INPUTPORT_DECISIONVALUE,  PORTID_INPUT_DECISIONVALUE); // accept float/int/bool values

        // setup output ports
        InitOutputPorts(1);
        SetupOutputPortAsPose("Output Pose", OUTPUTPORT_POSE, PORTID_OUTPUT_POSE);
    }


    // register the parameters
    void BlendTreePoseSwitchNode::RegisterAttributes()
    {
    }


    // get the palette name
    const char* BlendTreePoseSwitchNode::GetPaletteName() const
    {
        return "Pose Switch";
    }


    // get the category
    AnimGraphObject::ECategory BlendTreePoseSwitchNode::GetPaletteCategory() const
    {
        return AnimGraphObject::CATEGORY_LOGIC;
    }



    // create a clone of this node
    AnimGraphObject* BlendTreePoseSwitchNode::Clone(AnimGraph* animGraph)
    {
        // create the clone
        BlendTreePoseSwitchNode* clone = new BlendTreePoseSwitchNode(animGraph);

        // copy base class settings such as parameter values to the new clone
        CopyBaseObjectTo(clone);

        // return a pointer to the clone
        return clone;
    }


    // init the node
    void BlendTreePoseSwitchNode::Init(AnimGraphInstance* animGraphInstance)
    {
        MCORE_UNUSED(animGraphInstance);
        //mOutputPose.Init( animGraphInstance->GetActorInstance() );
    }


    // perform the calculations / actions
    void BlendTreePoseSwitchNode::Output(AnimGraphInstance* animGraphInstance)
    {
        AnimGraphPose* outputPose;

        // if the decision port has no incomming connection, there is nothing we can do
        if (mInputPorts[INPUTPORT_DECISIONVALUE].mConnection == nullptr)
        {
            RequestPoses(animGraphInstance);
            outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue();
            outputPose->InitFromBindPose(animGraphInstance->GetActorInstance());
            return;
        }

        // get the index we choose
        OutputIncomingNode(animGraphInstance, GetInputNode(INPUTPORT_DECISIONVALUE));
        const int32 decisionValue = MCore::Clamp<int32>(GetInputNumberAsInt32(animGraphInstance, INPUTPORT_DECISIONVALUE), 0, 9); // max 10 cases

        // check if there is an incoming connection from this port
        if (mInputPorts[INPUTPORT_POSE_0 + decisionValue].mConnection == nullptr)
        {
            RequestPoses(animGraphInstance);
            outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue();
            outputPose->InitFromBindPose(animGraphInstance->GetActorInstance());

            // visualize it
        #ifdef EMFX_EMSTUDIOBUILD
            if (GetCanVisualize(animGraphInstance))
            {
                animGraphInstance->GetActorInstance()->DrawSkeleton(outputPose->GetPose(), mVisualizeColor);
            }
        #endif

            return;
        }

        // get the local pose from the port
        OutputIncomingNode(animGraphInstance, GetInputNode(INPUTPORT_POSE_0 + decisionValue));
        const AnimGraphPose* pose = GetInputPose(animGraphInstance, INPUTPORT_POSE_0 + decisionValue)->GetValue();

        // output the pose
        RequestPoses(animGraphInstance);
        outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue();
        *outputPose = *pose;

        // visualize it
    #ifdef EMFX_EMSTUDIOBUILD
        if (GetCanVisualize(animGraphInstance))
        {
            animGraphInstance->GetActorInstance()->DrawSkeleton(outputPose->GetPose(), mVisualizeColor);
        }
    #endif
    }


    // update the blend tree node (update timer values etc)
    void BlendTreePoseSwitchNode::Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        // if the decision port has no incomming connection, there is nothing we can do
        if (mInputPorts[INPUTPORT_DECISIONVALUE].mConnection == nullptr)
        {
            UniqueData* uniqueData = static_cast<UniqueData*>(FindUniqueNodeData(animGraphInstance));
            uniqueData->Clear();
            return;
        }

        // update the node that plugs into the decision value port
        UpdateIncomingNode(animGraphInstance, mInputPorts[INPUTPORT_DECISIONVALUE].mConnection->GetSourceNode(), timePassedInSeconds);

        // get the index we choose
        const int32 decisionValue = MCore::Clamp<int32>(GetInputNumberAsInt32(animGraphInstance, INPUTPORT_DECISIONVALUE), 0, 9); // max 10 cases

        // check if there is an incoming connection from this port
        if (mInputPorts[INPUTPORT_POSE_0 + decisionValue].mConnection == nullptr)
        {
            UniqueData* uniqueData = static_cast<UniqueData*>(FindUniqueNodeData(animGraphInstance));
            uniqueData->Clear();
            return;
        }

        // pass through the motion extraction of the selected node
        AnimGraphNode* sourceNode = mInputPorts[INPUTPORT_POSE_0 + decisionValue].mConnection->GetSourceNode();

        // if our decision value changed since last time, specify that we want to resync
        // this basically means that the motion extraction delta will be zero for one frame
        UniqueData* uniqueData = static_cast<UniqueData*>(FindUniqueNodeData(animGraphInstance));
        if (uniqueData->mDecisionIndex != decisionValue)
        {
            uniqueData->mDecisionIndex = decisionValue;
            //sourceNode->RecursiveSetUniqueDataFlag(animGraphInstance, AnimGraphObjectData::FLAGS_RESYNC, true);
        }

        // update the source node and init the unique data
        UpdateIncomingNode(animGraphInstance, sourceNode, timePassedInSeconds);
        uniqueData->Init(animGraphInstance, sourceNode);
    }


    // get the blend node type string
    const char* BlendTreePoseSwitchNode::GetTypeString() const
    {
        return "BlendTreePoseSwitchNode";
    }


    // post update
    void BlendTreePoseSwitchNode::PostUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        // if the decision port has no incomming connection, there is nothing we can do
        if (mInputPorts[INPUTPORT_DECISIONVALUE].mConnection == nullptr)
        {
            RequestRefDatas(animGraphInstance);
            UniqueData* uniqueData = static_cast<UniqueData*>(FindUniqueNodeData(animGraphInstance));
            AnimGraphRefCountedData* data = uniqueData->GetRefCountedData();
            data->ClearEventBuffer();
            data->ZeroTrajectoryDelta();
            return;
        }

        // update the node that plugs into the decision value port
        mInputPorts[INPUTPORT_DECISIONVALUE].mConnection->GetSourceNode()->PerformPostUpdate(animGraphInstance, timePassedInSeconds);

        // get the index we choose
        const int32 decisionValue = MCore::Clamp<int32>(GetInputNumberAsInt32(animGraphInstance, INPUTPORT_DECISIONVALUE), 0, 9); // max 10 cases

        // check if there is an incoming connection from this port
        if (mInputPorts[INPUTPORT_POSE_0 + decisionValue].mConnection == nullptr)
        {
            RequestRefDatas(animGraphInstance);
            UniqueData* uniqueData = static_cast<UniqueData*>(FindUniqueNodeData(animGraphInstance));
            AnimGraphRefCountedData* data = uniqueData->GetRefCountedData();
            data->ClearEventBuffer();
            data->ZeroTrajectoryDelta();
            return;
        }

        // pass through the motion extraction of the selected node
        AnimGraphNode* sourceNode = mInputPorts[INPUTPORT_POSE_0 + decisionValue].mConnection->GetSourceNode();
        sourceNode->PerformPostUpdate(animGraphInstance, timePassedInSeconds);

        // output the events of the source node we picked
        RequestRefDatas(animGraphInstance);
        UniqueData* uniqueData = static_cast<UniqueData*>(FindUniqueNodeData(animGraphInstance));
        AnimGraphRefCountedData* data = uniqueData->GetRefCountedData();
        AnimGraphRefCountedData* sourceData = sourceNode->FindUniqueNodeData(animGraphInstance)->GetRefCountedData();
        data->SetEventBuffer(sourceData->GetEventBuffer());
        data->SetTrajectoryDelta(sourceData->GetTrajectoryDelta());
        data->SetTrajectoryDeltaMirrored(sourceData->GetTrajectoryDeltaMirrored());
    }



    // post update
    void BlendTreePoseSwitchNode::TopDownUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        // if the decision port has no incomming connection, there is nothing we can do
        if (mInputPorts[INPUTPORT_DECISIONVALUE].mConnection == nullptr)
        {
            return;
        }

        // get the index we choose
        const int32 decisionValue = MCore::Clamp<int32>(GetInputNumberAsInt32(animGraphInstance, INPUTPORT_DECISIONVALUE), 0, 9); // max 10 cases

        // check if there is an incoming connection from this port
        if (mInputPorts[INPUTPORT_POSE_0 + decisionValue].mConnection == nullptr)
        {
            return;
        }

        // sync all the incoming connections
        UniqueData* uniqueData = static_cast<UniqueData*>(FindUniqueNodeData(animGraphInstance));
        HierarchicalSyncAllInputNodes(animGraphInstance, uniqueData);

        // top down update all incoming connections
        const uint32 numConnections = mConnections.GetLength();
        for (uint32 i = 0; i < numConnections; ++i)
        {
            mConnections[i]->GetSourceNode()->PerformTopDownUpdate(animGraphInstance, timePassedInSeconds);
        }
    }


    // update the unique data
    void BlendTreePoseSwitchNode::OnUpdateUniqueData(AnimGraphInstance* animGraphInstance)
    {
        // get the unique data and if it doesn't exist create it
        UniqueData* uniqueData = static_cast<UniqueData*>(FindUniqueNodeData(animGraphInstance));
        if (uniqueData == nullptr)
        {
            //uniqueData = new UniqueData(this, animGraphInstance, MCORE_INVALIDINDEX32);
            uniqueData = (UniqueData*)GetEMotionFX().GetAnimGraphManager()->GetObjectDataPool().RequestNew(TYPE_ID, this, animGraphInstance);
            animGraphInstance->RegisterUniqueObjectData(uniqueData);
        }
    }
}   // namespace EMotionFX

