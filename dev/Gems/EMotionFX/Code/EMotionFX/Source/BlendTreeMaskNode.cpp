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
#include <EMotionFX/Source/BlendTreeMaskNode.h>
#include <EMotionFX/Source/AnimGraphInstance.h>
#include <EMotionFX/Source/AnimGraphAttributeTypes.h>
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/Node.h>


namespace EMotionFX
{
    size_t BlendTreeMaskNode::m_numMasks = 4;

    AZ_CLASS_ALLOCATOR_IMPL(BlendTreeMaskNode, AnimGraphAllocator, 0)
    AZ_CLASS_ALLOCATOR_IMPL(BlendTreeMaskNode::UniqueData, AnimGraphObjectUniqueDataAllocator, 0)

    BlendTreeMaskNode::BlendTreeMaskNode()
        : AnimGraphNode()
        , m_outputEvents0(true)
        , m_outputEvents1(true)
        , m_outputEvents2(true)
        , m_outputEvents3(true)
    {
        // setup the input ports
        InitInputPorts(static_cast<AZ::u32>(m_numMasks));
        SetupInputPort("Pose 0", INPUTPORT_POSE_0, AttributePose::TYPE_ID, PORTID_INPUT_POSE_0);
        SetupInputPort("Pose 1", INPUTPORT_POSE_1, AttributePose::TYPE_ID, PORTID_INPUT_POSE_1);
        SetupInputPort("Pose 2", INPUTPORT_POSE_2, AttributePose::TYPE_ID, PORTID_INPUT_POSE_2);
        SetupInputPort("Pose 3", INPUTPORT_POSE_3, AttributePose::TYPE_ID, PORTID_INPUT_POSE_3);

        // setup the output ports
        InitOutputPorts(1);
        SetupOutputPortAsPose("Output Pose", OUTPUTPORT_RESULT, PORTID_OUTPUT_RESULT);
    }


    BlendTreeMaskNode::~BlendTreeMaskNode()
    {
    }


    void BlendTreeMaskNode::Reinit()
    {
        AnimGraphNode::Reinit();

        const size_t numAnimGraphInstances = mAnimGraph->GetNumAnimGraphInstances();
        for (size_t i = 0; i < numAnimGraphInstances; ++i)
        {
            AnimGraphInstance* animGraphInstance = mAnimGraph->GetAnimGraphInstance(i);

            UniqueData* uniqueData = reinterpret_cast<UniqueData*>(animGraphInstance->FindUniqueObjectData(this));
            if (uniqueData)
            {
                uniqueData->mMustUpdate = true;
            }
        }

        UpdateUniqueDatas();
    }


    bool BlendTreeMaskNode::InitAfterLoading(AnimGraph* animGraph)
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
    const char* BlendTreeMaskNode::GetPaletteName() const
    {
        return "Pose Mask";
    }


    // get the category
    AnimGraphObject::ECategory BlendTreeMaskNode::GetPaletteCategory() const
    {
        return AnimGraphObject::CATEGORY_BLENDING;
    }


    // precreate the unique data object
    void BlendTreeMaskNode::OnUpdateUniqueData(AnimGraphInstance* animGraphInstance)
    {
        // find the unique data for this node, if it doesn't exist yet, create it
        UniqueData* uniqueData = static_cast<BlendTreeMaskNode::UniqueData*>(animGraphInstance->FindUniqueObjectData(this));
        if (uniqueData == nullptr)
        {
            uniqueData = aznew UniqueData(this, animGraphInstance);
            animGraphInstance->RegisterUniqueObjectData(uniqueData);
        }

        uniqueData->mMasks.resize(m_numMasks);
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
        for (uint32 i = 0; i < m_numMasks; ++i)
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

        for (uint32 i = 0; i < m_numMasks; ++i)
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
            const size_t numNodes = uniqueData->mMasks[i].size();
            if (numNodes > 0)
            {
                // for all nodes in the mask, output their transforms
                for (size_t n = 0; n < numNodes; ++n)
                {
                    const uint32 nodeIndex = uniqueData->mMasks[i][n];
                    outputLocalPose.SetLocalSpaceTransform(nodeIndex, localPose.GetLocalSpaceTransform(nodeIndex));
                }
            }
            else
            {
                outputLocalPose = localPose;
            }
        }

        // visualize it
        if (GetEMotionFX().GetIsInEditorMode() && GetCanVisualize(animGraphInstance))
        {
            animGraphInstance->GetActorInstance()->DrawSkeleton(outputPose->GetPose(), mVisualizeColor);
        }
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
        for (uint32 i = 0; i < m_numMasks; ++i)
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

        for (uint32 i = 0; i < m_numMasks; ++i)
        {
            // if the port has no input, skip it
            AnimGraphNode* inputNode = GetInputNode(INPUTPORT_POSE_0 + i);
            if (inputNode == nullptr)
            {
                continue;
            }

            // get the number of nodes inside the mask and default them to all nodes in the local pose in case there aren't any selected
            const size_t numNodes = uniqueData->mMasks[i].size();
            if (numNodes > 0)
            {
                // for all nodes in the mask, output their transforms
                for (size_t n = 0; n < numNodes; ++n)
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
            if (GetOutputEvents(i))
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


    bool BlendTreeMaskNode::GetOutputEvents(size_t index) const
    {
        switch (index)
        {
        case 0:
            return m_outputEvents0;
        case 1:
            return m_outputEvents1;
        case 2:
            return m_outputEvents2;
        case 3:
            return m_outputEvents3;
        }

        return true;
    }


    void BlendTreeMaskNode::UpdateUniqueData(AnimGraphInstance* animGraphInstance, UniqueData* uniqueData)
    {
        // Update only if needed.
        if (uniqueData->mMustUpdate)
        {
            Actor* actor = animGraphInstance->GetActorInstance()->GetActor();

            AnimGraphPropertyUtils::ReinitJointIndices(actor, m_mask0, uniqueData->mMasks[0]);
            AnimGraphPropertyUtils::ReinitJointIndices(actor, m_mask1, uniqueData->mMasks[1]);
            AnimGraphPropertyUtils::ReinitJointIndices(actor, m_mask2, uniqueData->mMasks[2]);
            AnimGraphPropertyUtils::ReinitJointIndices(actor, m_mask3, uniqueData->mMasks[3]);

            // Don't update the next time again.
            uniqueData->mMustUpdate = false;
        }
    }



    void BlendTreeMaskNode::SetMask0(const AZStd::vector<AZStd::string>& mask0)
    {
        m_mask0 = mask0;
    }

    void BlendTreeMaskNode::SetMask1(const AZStd::vector<AZStd::string>& mask1)
    {
        m_mask1 = mask1;
    }

    void BlendTreeMaskNode::SetMask2(const AZStd::vector<AZStd::string>& mask2)
    {
        m_mask2 = mask2;
    }

    void BlendTreeMaskNode::SetMask3(const AZStd::vector<AZStd::string>& mask3)
    {
        m_mask3 = mask3;
    }

    void BlendTreeMaskNode::SetOutputEvents0(bool outputEvents0)
    {
        m_outputEvents0 = outputEvents0;
    }

    void BlendTreeMaskNode::SetOutputEvents1(bool outputEvents1)
    {
        m_outputEvents1 = outputEvents1;
    }

    void BlendTreeMaskNode::SetOutputEvents2(bool outputEvents2)
    {
        m_outputEvents2 = outputEvents2;
    }

    void BlendTreeMaskNode::SetOutputEvents3(bool outputEvents3)
    {
        m_outputEvents3 = outputEvents3;
    }

    void BlendTreeMaskNode::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<BlendTreeMaskNode, AnimGraphNode>()
            ->Version(1)
            ->Field("mask0", &BlendTreeMaskNode::m_mask0)
            ->Field("mask1", &BlendTreeMaskNode::m_mask1)
            ->Field("mask2", &BlendTreeMaskNode::m_mask2)
            ->Field("mask3", &BlendTreeMaskNode::m_mask3)
            ->Field("outputEvents0", &BlendTreeMaskNode::m_outputEvents0)
            ->Field("outputEvents1", &BlendTreeMaskNode::m_outputEvents1)
            ->Field("outputEvents2", &BlendTreeMaskNode::m_outputEvents2)
            ->Field("outputEvents3", &BlendTreeMaskNode::m_outputEvents3)
        ;


        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<BlendTreeMaskNode>("Pose Mask", "Pose mark attributes")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
            ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
            ->DataElement(AZ_CRC("ActorNodes", 0x70504714), &BlendTreeMaskNode::m_mask0, "Mask 1", "The mask to apply on the Pose 1 input port.")
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &BlendTreeMaskNode::Reinit)
            ->Attribute(AZ::Edit::Attributes::ContainerCanBeModified, false)
            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::HideChildren)
            ->DataElement(AZ_CRC("ActorNodes", 0x70504714), &BlendTreeMaskNode::m_mask1, "Mask 2", "The mask to apply on the Pose 2 input port.")
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &BlendTreeMaskNode::Reinit)
            ->Attribute(AZ::Edit::Attributes::ContainerCanBeModified, false)
            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::HideChildren)
            ->DataElement(AZ_CRC("ActorNodes", 0x70504714), &BlendTreeMaskNode::m_mask2, "Mask 3", "The mask to apply on the Pose 3 input port.")
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &BlendTreeMaskNode::Reinit)
            ->Attribute(AZ::Edit::Attributes::ContainerCanBeModified, false)
            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::HideChildren)
            ->DataElement(AZ_CRC("ActorNodes", 0x70504714), &BlendTreeMaskNode::m_mask3, "Mask 4", "The mask to apply on the Pose 4 input port.")
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &BlendTreeMaskNode::Reinit)
            ->Attribute(AZ::Edit::Attributes::ContainerCanBeModified, false)
            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::HideChildren)
            ->DataElement(AZ::Edit::UIHandlers::Default, &BlendTreeMaskNode::m_outputEvents0, "Output Events 1", "Output events of the first input port?")
            ->DataElement(AZ::Edit::UIHandlers::Default, &BlendTreeMaskNode::m_outputEvents1, "Output Events 2", "Output events of the second input port?")
            ->DataElement(AZ::Edit::UIHandlers::Default, &BlendTreeMaskNode::m_outputEvents2, "Output Events 3", "Output events of the third input port?")
            ->DataElement(AZ::Edit::UIHandlers::Default, &BlendTreeMaskNode::m_outputEvents3, "Output Events 4", "Output events of the forth input port?")
        ;
    }
} // namespace EMotionFX