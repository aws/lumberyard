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
#include "AnimGraphExitNode.h"
#include "AnimGraphInstance.h"
#include "AnimGraphAttributeTypes.h"
#include "AnimGraphStateMachine.h"
#include "AnimGraphManager.h"
#include "EMotionFXManager.h"
#include <EMotionFX/Source/AnimGraph.h>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(AnimGraphExitNode, AnimGraphAllocator, 0)
    AZ_CLASS_ALLOCATOR_IMPL(AnimGraphExitNode::UniqueData, AnimGraphObjectUniqueDataAllocator, 0)

    AnimGraphExitNode::AnimGraphExitNode()
        : AnimGraphNode()
    {
        // setup the output ports
        InitOutputPorts(1);
        SetupOutputPortAsPose("Output", OUTPUTPORT_RESULT, PORTID_OUTPUT_POSE);
    }


    AnimGraphExitNode::~AnimGraphExitNode()
    {
    }


    bool AnimGraphExitNode::InitAfterLoading(AnimGraph* animGraph)
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
    const char* AnimGraphExitNode::GetPaletteName() const
    {
        return "Exit Node";
    }


    // get the category
    AnimGraphObject::ECategory AnimGraphExitNode::GetPaletteCategory() const
    {
        return AnimGraphObject::CATEGORY_SOURCES;
    }


    // pre-create unique data object
    void AnimGraphExitNode::OnUpdateUniqueData(AnimGraphInstance* animGraphInstance)
    {
        UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->FindUniqueObjectData(this));
        if (uniqueData == nullptr)
        {
            uniqueData = aznew AnimGraphExitNode::UniqueData(this, animGraphInstance);
            animGraphInstance->RegisterUniqueObjectData(uniqueData);
        }
        else
        {
            if (uniqueData->mPreviousNode && FindChildNodeIndex(uniqueData->mPreviousNode) == MCORE_INVALIDINDEX32)
            {
                uniqueData->mPreviousNode = nullptr;
            }
        }

        OnUpdateTriggerActionsUniqueData(animGraphInstance);
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
            if (GetEMotionFX().GetIsInEditorMode() && GetCanVisualize(animGraphInstance))
            {
                actorInstance->DrawSkeleton(outputPose->GetPose(), mVisualizeColor);
            }

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
        if (GetEMotionFX().GetIsInEditorMode() && GetCanVisualize(animGraphInstance))
        {
            actorInstance->DrawSkeleton(outputPose->GetPose(), mVisualizeColor);
        }
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


    void AnimGraphExitNode::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<AnimGraphExitNode, AnimGraphNode>()
            ->Version(1);


        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<AnimGraphExitNode>("Exit Node", "Exit node attributes")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
            ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
            ;
    }
} // namespace EMotionFX