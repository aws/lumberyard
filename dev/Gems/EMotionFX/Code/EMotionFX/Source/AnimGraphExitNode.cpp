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
#include "AnimGraphExitNode.h"
#include "AnimGraphInstance.h"
#include "AnimGraphAttributeTypes.h"
#include "AnimGraphStateMachine.h"
#include "AnimGraphManager.h"
#include "EMotionFXManager.h"


namespace EMotionFX
{
    // constructor
    AnimGraphExitNode::AnimGraphExitNode(AnimGraph* animGraph)
        : AnimGraphNode(animGraph, nullptr, TYPE_ID)
    {
        // allocate space for the variables
        CreateAttributeValues();
        RegisterPorts();
        InitInternalAttributesForAllInstances();
    }


    // destructor
    AnimGraphExitNode::~AnimGraphExitNode()
    {
    }


    // create
    AnimGraphExitNode* AnimGraphExitNode::Create(AnimGraph* animGraph)
    {
        return new AnimGraphExitNode(animGraph);
    }


    AnimGraphObjectData* AnimGraphExitNode::CreateObjectData()
    {
        return new UniqueData(this, nullptr);
    }


    // register the ports
    void AnimGraphExitNode::RegisterPorts()
    {
        // setup the output ports
        InitOutputPorts(1);
        SetupOutputPortAsPose("Output", OUTPUTPORT_RESULT, PORTID_OUTPUT_POSE);
    }


    // register the parameters
    void AnimGraphExitNode::RegisterAttributes()
    {
    }


    // get the palette name
    const char* AnimGraphExitNode::GetPaletteName() const
    {
        return "Exit Node";
    }


    // get the category
    AnimGraphObject::ECategory AnimGraphExitNode::GetPaletteCategory() const
    {
        return AnimGraphObject::CATEGORY_SOURCES;
    }


    // create a clone of this node
    AnimGraphObject* AnimGraphExitNode::Clone(AnimGraph* animGraph)
    {
        // create the clone
        AnimGraphExitNode* clone = new AnimGraphExitNode(animGraph);

        // copy base class settings such as parameter values to the new clone
        CopyBaseObjectTo(clone);

        // return a pointer to the clone
        return clone;
    }


    // pre-create unique data object
    void AnimGraphExitNode::OnUpdateUniqueData(AnimGraphInstance* animGraphInstance)
    {
        UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->FindUniqueObjectData(this));
        if (uniqueData == nullptr)
        {
            //uniqueData = new UniqueData(this, animGraphInstance);
            uniqueData = (UniqueData*)GetEMotionFX().GetAnimGraphManager()->GetObjectDataPool().RequestNew(TYPE_ID, this, animGraphInstance);
            animGraphInstance->RegisterUniqueObjectData(uniqueData);
        }
    }


    // when entering this state/node
    void AnimGraphExitNode::OnStateEntering(AnimGraphInstance* animGraphInstance, AnimGraphNode* previousState, AnimGraphStateTransition* usedTransition)
    {
        MCORE_UNUSED(usedTransition);
        UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->FindUniqueObjectData(this));
        uniqueData->mPreviousNode = previousState;
    }


    // rewind the node
    void AnimGraphExitNode::Rewind(AnimGraphInstance* animGraphInstance)
    {
        UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->FindUniqueObjectData(this));
        uniqueData->mPreviousNode = nullptr;
    }


    // update
    void AnimGraphExitNode::Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        // if the previous node is not set, do nothing
        UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->FindUniqueObjectData(this));
        if (uniqueData->mPreviousNode == nullptr || uniqueData->mPreviousNode == this)
        {
            uniqueData->Clear();
            return;
        }

        //
        UpdateIncomingNode(animGraphInstance, uniqueData->mPreviousNode, timePassedInSeconds);
        uniqueData->mPreviousNode->IncreasePoseRefCount(animGraphInstance);
        uniqueData->mPreviousNode->IncreaseRefDataRefCount(animGraphInstance);
        uniqueData->Init(animGraphInstance, uniqueData->mPreviousNode);
    }


    // perform the calculations / actions
    void AnimGraphExitNode::Output(AnimGraphInstance* animGraphInstance)
    {
        AnimGraphPose* outputPose;

        // if the previous node is not set, output a bind pose
        ActorInstance* actorInstance = animGraphInstance->GetActorInstance();
        UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->FindUniqueObjectData(this));
        if (uniqueData->mPreviousNode == nullptr || uniqueData->mPreviousNode == this)
        {
            RequestPoses(animGraphInstance);
            outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_RESULT)->GetValue();
            outputPose->InitFromBindPose(actorInstance);

            // visualize it
        #ifdef EMFX_EMSTUDIOBUILD
            if (GetCanVisualize(animGraphInstance))
            {
                actorInstance->DrawSkeleton(outputPose->GetPose(), mVisualizeColor);
            }
        #endif

            return;
        }

        // everything seems fine with the previous node, so just sample that one
        OutputIncomingNode(animGraphInstance, uniqueData->mPreviousNode);
        RequestPoses(animGraphInstance);

        outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_RESULT)->GetValue();

        if (uniqueData->mPreviousNode != this)
        {
            *outputPose = *uniqueData->mPreviousNode->GetMainOutputPose(animGraphInstance);
        }
        else
        {
            outputPose->InitFromBindPose(actorInstance);
        }

        uniqueData->mPreviousNode->DecreaseRef(animGraphInstance);

        // visualize it
    #ifdef EMFX_EMSTUDIOBUILD
        if (GetCanVisualize(animGraphInstance))
        {
            actorInstance->DrawSkeleton(outputPose->GetPose(), mVisualizeColor);
        }
    #endif
    }


    // top down update
    void AnimGraphExitNode::TopDownUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        // if there is no previous node, do nothing
        UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->FindUniqueObjectData(this));
        if (uniqueData->mPreviousNode == nullptr || uniqueData->mPreviousNode == this)
        {
            return;
        }

        // sync the previous node to this exit node
        HierarchicalSyncInputNode(animGraphInstance, uniqueData->mPreviousNode, uniqueData);

        // call the top-down update of the previous node
        uniqueData->mPreviousNode->PerformTopDownUpdate(animGraphInstance, timePassedInSeconds);
    }


    // post update
    void AnimGraphExitNode::PostUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        // if there is no previous node, do nothing
        UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->FindUniqueObjectData(this));
        if (uniqueData->mPreviousNode == nullptr || uniqueData->mPreviousNode == this)
        {
            RequestRefDatas(animGraphInstance);
            AnimGraphRefCountedData* data = uniqueData->GetRefCountedData();
            data->ClearEventBuffer();
            data->ZeroTrajectoryDelta();
            return;
        }

        // post update the previous node, so that its event buffer is filled
        uniqueData->mPreviousNode->PerformPostUpdate(animGraphInstance, timePassedInSeconds);

        RequestRefDatas(animGraphInstance);
        AnimGraphRefCountedData* data = uniqueData->GetRefCountedData();

        AnimGraphRefCountedData* sourceData = uniqueData->mPreviousNode->FindUniqueNodeData(animGraphInstance)->GetRefCountedData();
        data->SetEventBuffer(sourceData->GetEventBuffer());
        data->SetTrajectoryDelta(sourceData->GetTrajectoryDelta());
        data->SetTrajectoryDeltaMirrored(sourceData->GetTrajectoryDeltaMirrored());

        uniqueData->mPreviousNode->DecreaseRefDataRef(animGraphInstance);
    }


    // get the blend node type string
    const char* AnimGraphExitNode::GetTypeString() const
    {
        return "AnimGraphExitNode";
    }


    // reset flags
    void AnimGraphExitNode::RecursiveResetFlags(AnimGraphInstance* animGraphInstance, uint32 flagsToDisable)
    {
        UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->FindUniqueObjectData(this));
        animGraphInstance->DisableObjectFlags(mObjectIndex, flagsToDisable);

        // forward it to the node we came from
        if (uniqueData->mPreviousNode && uniqueData->mPreviousNode != this)
        {
            uniqueData->mPreviousNode->RecursiveResetFlags(animGraphInstance, flagsToDisable);
        }
    }
}   // namespace EMotionFX

