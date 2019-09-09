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

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include "EMotionFXConfig.h"
#include "AnimGraphEntryNode.h"
#include "AnimGraphInstance.h"
#include "AnimGraphAttributeTypes.h"
#include "AnimGraphStateMachine.h"
#include "AnimGraphRefCountedData.h"
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/EMotionFXManager.h>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(AnimGraphEntryNode, AnimGraphAllocator, 0)

    AnimGraphEntryNode::AnimGraphEntryNode()
        : AnimGraphNode()
    {
        // setup the output ports
        InitOutputPorts(1);
        SetupOutputPortAsPose("Output Pose", OUTPUTPORT_RESULT, PORTID_OUTPUT_POSE);
    }

    AnimGraphEntryNode::~AnimGraphEntryNode()
    {
    }


    bool AnimGraphEntryNode::InitAfterLoading(AnimGraph* animGraph)
    {
        if (!AnimGraphNode::InitAfterLoading(animGraph))
        {
            return false;
        }

        InitInternalAttributesForAllInstances();

        Reinit();
        return true;
    }


    // get the palette name
    const char* AnimGraphEntryNode::GetPaletteName() const
    {
        return "Entry";
    }


    // get the category
    AnimGraphObject::ECategory AnimGraphEntryNode::GetPaletteCategory() const
    {
        return AnimGraphObject::CATEGORY_SOURCES;
    }


    // a helper function to get the source node
    AnimGraphNode* AnimGraphEntryNode::FindSourceNode(AnimGraphInstance* animGraphInstance)
    {
        // get the parent node and check if it is a state machine
        AnimGraphNode* parentNode = GetParentNode();
        MCORE_ASSERT(azrtti_typeid(parentNode) == azrtti_typeid<AnimGraphStateMachine>());

        // get the parent of the state machine where this pass-through node is in
        AnimGraphNode* stateMachineParentNode = parentNode->GetParentNode();
        if (azrtti_typeid(stateMachineParentNode) == azrtti_typeid<AnimGraphStateMachine>())
        {
            // cast the parent of the state machine where this pass-through node is in to a state machine
            AnimGraphStateMachine* stateMachineParent = static_cast<AnimGraphStateMachine*>(stateMachineParentNode);
            return stateMachineParent->GetCurrentState(animGraphInstance);
        }

        return nullptr;
    }


    // perform the calculations / actions
    void AnimGraphEntryNode::Output(AnimGraphInstance* animGraphInstance)
    {
        AnimGraphPose* outputPose;

        // we only need to get the pose from the source node for the cases where the source node is valid (not null) and 
        // is not the parent node (since the source of the parent state machine is the binding pose)
        AnimGraphNode* sourceNode = FindSourceNode(animGraphInstance);
        if (!sourceNode)
        {
            RequestPoses(animGraphInstance);
            outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_RESULT)->GetValue();
            outputPose->InitFromBindPose(animGraphInstance->GetActorInstance());

            if (GetEMotionFX().GetIsInEditorMode())
            {
                SetHasError(animGraphInstance, true);
            }
            return;
        }
        else if (sourceNode == GetParentNode())
        {
            RequestPoses(animGraphInstance);
            outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_RESULT)->GetValue();
            outputPose->InitFromBindPose(animGraphInstance->GetActorInstance());

            // visualize it
            if (GetEMotionFX().GetIsInEditorMode() && GetCanVisualize(animGraphInstance))
            {
                animGraphInstance->GetActorInstance()->DrawSkeleton(outputPose->GetPose(), mVisualizeColor);
            }
            return;
        }

        if (GetEMotionFX().GetIsInEditorMode())
        {
            SetHasError(animGraphInstance, false);
        }

        OutputIncomingNode(animGraphInstance, sourceNode);

        RequestPoses(animGraphInstance);
        outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_RESULT)->GetValue();
        *outputPose = *sourceNode->GetMainOutputPose(animGraphInstance);

        sourceNode->DecreaseRef(animGraphInstance);

        // visualize it
        if (GetEMotionFX().GetIsInEditorMode() && GetCanVisualize(animGraphInstance))
        {
            animGraphInstance->GetActorInstance()->DrawSkeleton(outputPose->GetPose(), mVisualizeColor);
        }
    }



    // update
    void AnimGraphEntryNode::Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        AnimGraphNodeData* uniqueData = FindUniqueNodeData(animGraphInstance);

        // find the source node
        AnimGraphNode* sourceNode = FindSourceNode(animGraphInstance);
        if (!sourceNode || sourceNode == GetParentNode())
        {
            uniqueData->Clear();
            return;
        }

        sourceNode->IncreasePoseRefCount(animGraphInstance);
        sourceNode->IncreaseRefDataRefCount(animGraphInstance);
        UpdateIncomingNode(animGraphInstance, sourceNode, timePassedInSeconds);
        uniqueData->Init(animGraphInstance, sourceNode);
    }


    // top down update
    void AnimGraphEntryNode::TopDownUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        // find the source node
        AnimGraphNode* sourceNode = FindSourceNode(animGraphInstance);
        if (!sourceNode || sourceNode == GetParentNode())
        {
            return;
        }

        // sync the previous node to this exit node
        //HierarchicalSyncInputNode(animGraphInstance, sourceNode, uniqueData);

        // call the top-down update of the previous node
        sourceNode->PerformTopDownUpdate(animGraphInstance, timePassedInSeconds);
    }


    // post update
    void AnimGraphEntryNode::PostUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        // find the source node
        AnimGraphNode* sourceNode = FindSourceNode(animGraphInstance);
        if (!sourceNode || sourceNode == GetParentNode())
        {
            AnimGraphNodeData* uniqueData = FindUniqueNodeData(animGraphInstance);
            RequestRefDatas(animGraphInstance);
            AnimGraphRefCountedData* data = uniqueData->GetRefCountedData();
            data->ClearEventBuffer();
            data->ZeroTrajectoryDelta();
            return;
        }

        // post update the source node, so that its event buffer is filled
        sourceNode->PerformPostUpdate(animGraphInstance, timePassedInSeconds);

        // copy over the event buffer
        AnimGraphNodeData* uniqueData = FindUniqueNodeData(animGraphInstance);
        RequestRefDatas(animGraphInstance);
        AnimGraphRefCountedData* data = uniqueData->GetRefCountedData();

        AnimGraphRefCountedData* sourceData = sourceNode->FindUniqueNodeData(animGraphInstance)->GetRefCountedData();
        data->SetEventBuffer(sourceData->GetEventBuffer());
        data->SetTrajectoryDelta(sourceData->GetTrajectoryDelta());
        data->SetTrajectoryDeltaMirrored(sourceData->GetTrajectoryDeltaMirrored());

        sourceNode->DecreaseRefDataRef(animGraphInstance);
    }


    void AnimGraphEntryNode::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<AnimGraphEntryNode, AnimGraphNode>()
            ->Version(1);


        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<AnimGraphEntryNode>("Entry Node", "Entry node attributes")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
            ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
            ;
    }
} // namespace EMotionFX