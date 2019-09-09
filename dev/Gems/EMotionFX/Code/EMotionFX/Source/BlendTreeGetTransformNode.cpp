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
#include <AzCore/Math/Transform.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzFramework/Math/MathUtils.h>
#include <EMotionFX/Source/EMotionFXConfig.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphInstance.h>
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/BlendTreeGetTransformNode.h>
#include <EMotionFX/Source/Node.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/TransformData.h>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(BlendTreeGetTransformNode, AnimGraphAllocator, 0)
    AZ_CLASS_ALLOCATOR_IMPL(BlendTreeGetTransformNode::UniqueData, AnimGraphObjectUniqueDataAllocator, 0)


    BlendTreeGetTransformNode::BlendTreeGetTransformNode()
        : AnimGraphNode()
        , m_transformSpace(TRANSFORM_SPACE_LOCAL)
    {
        // setup the input ports
        InitInputPorts(1);
        SetupInputPort("Input Pose", INPUTPORT_POSE, AttributePose::TYPE_ID, PORTID_INPUT_POSE);

        // setup the output ports
        InitOutputPorts(3);
        SetupOutputPort("Output Translation", OUTPUTPORT_TRANSLATION, MCore::AttributeVector3::TYPE_ID, PORTID_OUTPUT_TRANSLATION);
        SetupOutputPort("Output Rotation", OUTPUTPORT_ROTATION, MCore::AttributeQuaternion::TYPE_ID, PORTID_OUTPUT_ROTATION);
        SetupOutputPort("Output Scale", OUTPUTPORT_SCALE, MCore::AttributeVector3::TYPE_ID, PORTID_OUTPUT_SCALE);
    }


    BlendTreeGetTransformNode::~BlendTreeGetTransformNode()
    {
    }


    void BlendTreeGetTransformNode::Reinit()
    {
        AnimGraphNode::Reinit();

        const size_t numAnimGraphInstances = mAnimGraph->GetNumAnimGraphInstances();
        for (size_t i = 0; i < numAnimGraphInstances; ++i)
        {
            AnimGraphInstance* animGraphInstance = mAnimGraph->GetAnimGraphInstance(i);

            UniqueData* uniqueData = static_cast<UniqueData*>(FindUniqueNodeData(animGraphInstance));
            if (uniqueData)
            {
                uniqueData->m_mustUpdate = true;
                OnUpdateUniqueData(animGraphInstance);
            }
        }
    }


    bool BlendTreeGetTransformNode::InitAfterLoading(AnimGraph* animGraph)
    {
        if (!AnimGraphNode::InitAfterLoading(animGraph))
        {
            return false;
        }

        InitInternalAttributesForAllInstances();

        Reinit();
        return true;
    }


    const char* BlendTreeGetTransformNode::GetPaletteName() const
    {
        return "Get Transform";
    }


    AnimGraphObject::ECategory BlendTreeGetTransformNode::GetPaletteCategory() const
    {
        return AnimGraphObject::CATEGORY_CONTROLLERS;
    }


    void BlendTreeGetTransformNode::Output(AnimGraphInstance* animGraphInstance)
    {
        ActorInstance* actorInstance = animGraphInstance->GetActorInstance();
        AnimGraphPose* inputPose = nullptr;

        // get the unique
        UniqueData* uniqueData = static_cast<UniqueData*>(FindUniqueNodeData(animGraphInstance));
        UpdateUniqueData(animGraphInstance, uniqueData);

        if (GetEMotionFX().GetIsInEditorMode())
        {
            SetHasError(animGraphInstance, uniqueData->m_nodeIndex == MCORE_INVALIDINDEX32);
        }

        // make sure we have at least an input pose, otherwise output the bind pose
        if (GetInputPort(INPUTPORT_POSE).mConnection)
        {
            OutputIncomingNode(animGraphInstance, GetInputNode(INPUTPORT_POSE));
            inputPose = GetInputPose(animGraphInstance, INPUTPORT_POSE)->GetValue();
        }

        Pose* pose = nullptr;
        if (uniqueData->m_nodeIndex != MCORE_INVALIDINDEX32)
        {
            if (m_actorNode.second == 0)
            {
                // We operate over the input pose
                if (inputPose)
                {
                    pose = &inputPose->GetPose();
                }
            }
            else
            {
                const ActorInstance* alignInstance = animGraphInstance->FindActorInstanceFromParentDepth(m_actorNode.second);
                if (alignInstance)
                {
                    pose = alignInstance->GetTransformData()->GetCurrentPose();
                }
            }
        }

        Transform inputTransform;
        if (pose)
        {
            switch (m_transformSpace)
            {
            case TRANSFORM_SPACE_LOCAL:
                pose->GetLocalSpaceTransform(uniqueData->m_nodeIndex, &inputTransform);
                break;
            case TRANSFORM_SPACE_WORLD:
                pose->GetWorldSpaceTransform(uniqueData->m_nodeIndex, &inputTransform);
                break;
            case TRANSFORM_SPACE_MODEL:
                pose->GetModelSpaceTransform(uniqueData->m_nodeIndex, &inputTransform);
                break;
            default:
                AZ_Assert(false, "Unhandled transform space");
                inputTransform.Identity();
                break;
            }
        }
        else
        {
            inputTransform.Identity();
        }

        GetOutputVector3(animGraphInstance, OUTPUTPORT_TRANSLATION)->SetValue(AZ::PackedVector3f(inputTransform.mPosition.GetX(), inputTransform.mPosition.GetY(), inputTransform.mPosition.GetZ()));
        GetOutputQuaternion(animGraphInstance, OUTPUTPORT_ROTATION)->SetValue(inputTransform.mRotation);
        GetOutputVector3(animGraphInstance, OUTPUTPORT_SCALE)->SetValue(AZ::PackedVector3f(inputTransform.mScale.GetX(), inputTransform.mScale.GetY(), inputTransform.mScale.GetZ()));
    }


    void BlendTreeGetTransformNode::UpdateUniqueData(AnimGraphInstance* animGraphInstance, UniqueData* uniqueData)
    {
        // update the unique data if needed
        if (uniqueData->m_mustUpdate || uniqueData->m_nodeIndex == MCORE_INVALIDINDEX32)
        {
            ActorInstance* actorInstance = animGraphInstance->GetActorInstance();
            Actor* actor = actorInstance->GetActor();

            uniqueData->m_mustUpdate = false;
            uniqueData->m_nodeIndex  = MCORE_INVALIDINDEX32;

            if (m_actorNode.first.empty())
            {
                return;
            }

            // lookup the actor instance to get the node from
            const ActorInstance* alignInstance = animGraphInstance->FindActorInstanceFromParentDepth(m_actorNode.second);
            if (alignInstance)
            {
                const Node* alignNode = alignInstance->GetActor()->GetSkeleton()->FindNodeByName(m_actorNode.first);
                if (alignNode)
                {
                    uniqueData->m_nodeIndex = alignNode->GetNodeIndex();
                }
            }
        }
    }


    void BlendTreeGetTransformNode::OnUpdateUniqueData(AnimGraphInstance* animGraphInstance)
    {
        // find our unique data
        UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->FindUniqueObjectData(this));
        if (uniqueData == nullptr)
        {
            uniqueData = aznew UniqueData(this, animGraphInstance);
            animGraphInstance->RegisterUniqueObjectData(uniqueData);
        }

        UpdateUniqueData(animGraphInstance, uniqueData);
    }


    void BlendTreeGetTransformNode::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<BlendTreeGetTransformNode, AnimGraphNode>()
            ->Version(1)
            ->Field("actorNode", &BlendTreeGetTransformNode::m_actorNode)
            ->Field("transformSpace", &BlendTreeGetTransformNode::m_transformSpace)
        ;

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<BlendTreeGetTransformNode>("Get Transform Node", "Get Transform node attributes")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
            ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
            ->DataElement(AZ_CRC("ActorGoalNode", 0xaf1e8a3a), &BlendTreeGetTransformNode::m_actorNode, "Node", "The node to get the transform from.")
            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::HideChildren)
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &BlendTreeGetTransformNode::Reinit)
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
            ->DataElement(AZ::Edit::UIHandlers::ComboBox, &BlendTreeGetTransformNode::m_transformSpace)
        ;
    }
} // namespace EMotionFX