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
#include "BlendTreeBlend2NodeBase.h"
#include "Node.h"
#include "AnimGraph.h"


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(BlendTreeBlend2NodeBase, AnimGraphAllocator, 0)
    AZ_CLASS_ALLOCATOR_IMPL(BlendTreeBlend2NodeBase::UniqueData, AnimGraphObjectUniqueDataAllocator, 0)

    //-------------------------------------------

    BlendTreeBlend2NodeBase::UniqueData::UniqueData(AnimGraphNode* node, AnimGraphInstance* animGraphInstance)
        : AnimGraphNodeData(node, animGraphInstance) 
        , mSyncTrackNode(nullptr)
        , mMustUpdate(true)
    {
    }

    //-------------------------------------------

    BlendTreeBlend2NodeBase::BlendTreeBlend2NodeBase()
        : AnimGraphNode()
        , m_syncMode(SYNCMODE_DISABLED)
        , m_eventMode(EVENTMODE_MOSTACTIVE)
        , m_extractionMode(EXTRACTIONMODE_BLEND)
    {
        // Setup the input ports.
        InitInputPorts(3);
        SetupInputPort("Pose 1", INPUTPORT_POSE_A, AttributePose::TYPE_ID, PORTID_INPUT_POSE_A);
        SetupInputPort("Pose 2", INPUTPORT_POSE_B, AttributePose::TYPE_ID, PORTID_INPUT_POSE_B);
        SetupInputPortAsNumber("Weight", INPUTPORT_WEIGHT, PORTID_INPUT_WEIGHT);

        // Setup the output ports.
        InitOutputPorts(1);
        SetupOutputPortAsPose("Output Pose", OUTPUTPORT_POSE, PORTID_OUTPUT_POSE);
    }


    BlendTreeBlend2NodeBase::~BlendTreeBlend2NodeBase()
    {
    }


    AnimGraphObject::ECategory BlendTreeBlend2NodeBase::GetPaletteCategory() const
    {
        return AnimGraphObject::CATEGORY_BLENDING;
    }


    void BlendTreeBlend2NodeBase::SetWeightedNodeMask(const AZStd::vector<WeightedMaskEntry>& weightedNodeMask)
    {
        m_weightedNodeMask = weightedNodeMask;
    }


    void BlendTreeBlend2NodeBase::Reinit()
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


    bool BlendTreeBlend2NodeBase::InitAfterLoading(AnimGraph* animGraph)
    {
        if (!AnimGraphNode::InitAfterLoading(animGraph))
        {
            return false;
        }

        InitInternalAttributesForAllInstances();
        Reinit();
        return true;
    }


    void BlendTreeBlend2NodeBase::OnUpdateUniqueData(AnimGraphInstance* animGraphInstance)
    {
        UniqueData* uniqueData = static_cast<BlendTreeBlend2NodeBase::UniqueData*>(animGraphInstance->FindUniqueObjectData(this));
        if (!uniqueData)
        {
            uniqueData = aznew UniqueData(this, animGraphInstance);
            animGraphInstance->RegisterUniqueObjectData(uniqueData);
        }

        UpdateUniqueData(animGraphInstance, uniqueData);
    }


    void BlendTreeBlend2NodeBase::UpdateUniqueData(AnimGraphInstance* animGraphInstance, UniqueData* uniqueData)
    {
        if (uniqueData->mMustUpdate)
        {
            Actor* actor = animGraphInstance->GetActorInstance()->GetActor();
            if (m_weightedNodeMask.empty())
            {
                uniqueData->mMask.clear();
                uniqueData->mMustUpdate = false;
                return;
            }

            AZStd::vector<uint32>& nodeIndices = uniqueData->mMask;
            nodeIndices.clear();
            const size_t numNodes = m_weightedNodeMask.size();
            nodeIndices.reserve(numNodes);

            // Try to find the node indices by name for all masked nodes.
            const Skeleton* skeleton = actor->GetSkeleton();
            for (const WeightedMaskEntry& weightedNode : m_weightedNodeMask)
            {
                Node* node = skeleton->FindNodeByName(weightedNode.first.c_str());
                if (node)
                {
                    nodeIndices.emplace_back(node->GetNodeIndex());
                }
            }

            // Don't update the next time again
            uniqueData->mMustUpdate = false;
        }
    }


    void BlendTreeBlend2NodeBase::FindBlendNodes(AnimGraphInstance* animGraphInstance, AnimGraphNode** outBlendNodeA, AnimGraphNode** outBlendNodeB, float* outWeight, bool isAdditive, bool optimizeByWeight)
    {
        BlendTreeConnection* connectionA = mInputPorts[INPUTPORT_POSE_A].mConnection;
        BlendTreeConnection* connectionB = mInputPorts[INPUTPORT_POSE_B].mConnection;
        if (!connectionA && !connectionB)
        {
            *outBlendNodeA  = nullptr;
            *outBlendNodeB  = nullptr;
            *outWeight      = 0.0f;
            return;
        }

        if (connectionA && connectionB)
        {
            *outWeight = (mInputPorts[INPUTPORT_WEIGHT].mConnection) ? GetInputNumberAsFloat(animGraphInstance, INPUTPORT_WEIGHT) : 0.0f;
            *outWeight = MCore::Clamp<float>(*outWeight, 0.0f, 1.0f);

            UniqueData* uniqueData = static_cast<BlendTreeBlend2NodeBase::UniqueData*>(animGraphInstance->FindUniqueObjectData(this));
            if (uniqueData->mMask.size() > 0)
            {
                *outBlendNodeA  = connectionA->GetSourceNode();
                *outBlendNodeB  = connectionB->GetSourceNode();
                return;
            }

            if (optimizeByWeight)
            {
                if (*outWeight < MCore::Math::epsilon)
                {
                    *outBlendNodeA = connectionA->GetSourceNode();
                    *outBlendNodeB = nullptr;
                }
                else
                if ((*outWeight < 1.0f - MCore::Math::epsilon) || isAdditive)
                {
                    *outBlendNodeA = connectionA->GetSourceNode();
                    *outBlendNodeB = connectionB->GetSourceNode();
                }
                else
                {
                    *outBlendNodeA = connectionB->GetSourceNode();
                    *outBlendNodeB = nullptr;
                    *outWeight = 0.0f;
                }
            }
            else
            {
                *outBlendNodeA = connectionA->GetSourceNode();
                *outBlendNodeB = connectionB->GetSourceNode();
            }
            return;
        }

        *outBlendNodeA  = (connectionA) ? connectionA->GetSourceNode() : connectionB->GetSourceNode();
        *outBlendNodeB  = nullptr;
        *outWeight      = 1.0f;
    }


    void BlendTreeBlend2NodeBase::SetSyncMode(ESyncMode syncMode)
    { 
        m_syncMode = syncMode;
    }


    BlendTreeBlend2NodeBase::ESyncMode BlendTreeBlend2NodeBase::GetSyncMode() const
    { 
        return m_syncMode;
    }

    
    void BlendTreeBlend2NodeBase::SetEventMode(EEventMode eventMode)
    { 
        m_eventMode = eventMode;
    }


    BlendTreeBlend2NodeBase::EEventMode BlendTreeBlend2NodeBase::GetEventMode() const
    { 
        return m_eventMode;
    }

    
    void BlendTreeBlend2NodeBase::SetExtractionMode(EExtractionMode extractionMode)
    { 
        m_extractionMode = extractionMode;
    }


    BlendTreeBlend2NodeBase::EExtractionMode BlendTreeBlend2NodeBase::GetExtractionMode() const
    { 
        return m_extractionMode;
    }


    void BlendTreeBlend2NodeBase::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<BlendTreeBlend2NodeBase, AnimGraphNode>()
            ->Version(1)
            ->Field("syncMode", &BlendTreeBlend2NodeBase::m_syncMode)
            ->Field("eventMode", &BlendTreeBlend2NodeBase::m_eventMode)
            ->Field("extractionMode", &BlendTreeBlend2NodeBase::m_extractionMode)
            ->Field("mask", &BlendTreeBlend2NodeBase::m_weightedNodeMask)
        ;

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<BlendTreeBlend2NodeBase>("Blend 2 Base", "Blend 2 base attributes")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
            ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
            ->DataElement(AZ::Edit::UIHandlers::ComboBox, &BlendTreeBlend2NodeBase::m_syncMode)
            ->DataElement(AZ::Edit::UIHandlers::ComboBox, &BlendTreeBlend2NodeBase::m_eventMode)
            ->DataElement(AZ::Edit::UIHandlers::ComboBox, &BlendTreeBlend2NodeBase::m_extractionMode)
            ->DataElement(AZ_CRC("ActorWeightedNodes", 0x689c0537), &BlendTreeBlend2NodeBase::m_weightedNodeMask, "Mask", "The mask to apply on the Pose 1 input port.")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &BlendTreeBlend2NodeBase::Reinit)
                ->Attribute(AZ::Edit::Attributes::ContainerCanBeModified, false)
                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::HideChildren)
        ;
    }

}   // namespace EMotionFX
