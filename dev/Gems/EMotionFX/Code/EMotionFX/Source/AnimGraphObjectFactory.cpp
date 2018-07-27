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

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Serialization/SerializeContext.h>
#include "EMotionFXConfig.h"
#include "AnimGraphObjectFactory.h"
#include "AnimGraphObject.h"
#include "AnimGraphManager.h"

#include <MCore/Source/LogManager.h>

// default object types
#include "BlendTreeParameterNode.h"
#include "BlendSpace2DNode.h"
#include "BlendSpace1DNode.h"
#include "BlendTreeFinalNode.h"
#include "BlendTreeBlend2Node.h"
#include "BlendTreeBlend2NodeBase.h"
#include "BlendTreeBlend2AdditiveNode.h"
#include "BlendTreeBlend2LegacyNode.h"
#include "BlendTreeBlendNNode.h"
#include "BlendTreePoseSwitchNode.h"
#include "BlendTreeFloatConstantNode.h"
#include "BlendTreeFloatMath1Node.h"
#include "BlendTreeFloatMath2Node.h"
#include "BlendTreeFloatSwitchNode.h"
#include "BlendTreeFloatConditionNode.h"
#include "BlendTreeBoolLogicNode.h"
#include "BlendTreeSmoothingNode.h"
#include "BlendTreeMaskNode.h"
#include "BlendTreeMorphTargetNode.h"
#include "BlendTreeMotionFrameNode.h"
#include "BlendTreeVector3Math1Node.h"
#include "BlendTreeVector3Math2Node.h"
#include "BlendTreeVector2DecomposeNode.h"
#include "BlendTreeVector3DecomposeNode.h"
#include "BlendTreeVector4DecomposeNode.h"
#include "BlendTreeVector2ComposeNode.h"
#include "BlendTreeVector3ComposeNode.h"
#include "BlendTreeVector4ComposeNode.h"
#include "BlendTreeTwoLinkIKNode.h"
#include "BlendTreeLookAtNode.h"
#include "BlendTreeTransformNode.h"
#include "BlendTreeAccumTransformNode.h"
#include "BlendTreeRangeRemapperNode.h"
#include "BlendTreeDirectionToWeightNode.h"
#include "BlendTreeMirrorPoseNode.h"
#include "BlendTree.h"
#include "BlendTreeMirrorPoseNode.h"
#include "BlendTreePoseSubtractNode.h"

#include "AnimGraphBindPoseNode.h"
#include "AnimGraphMotionNode.h"
#include "AnimGraphStateMachine.h"
#include "AnimGraphExitNode.h"
#include "AnimGraphEntryNode.h"
#include "AnimGraphHubNode.h"

#include "AnimGraphTransitionCondition.h"
#include "AnimGraphParameterCondition.h"
#include "AnimGraphMotionCondition.h"
#include "AnimGraphStateCondition.h"
#include "AnimGraphTimeCondition.h"
#include "AnimGraphTagCondition.h"
#include "AnimGraphVector2Condition.h"
#include "AnimGraphPlayTimeCondition.h"

namespace EMotionFX
{
    void AnimGraphObjectFactory::ReflectTypes(AZ::ReflectContext* context)
    {
        AnimGraphNode::Reflect(context);
        AnimGraphStateMachine::Reflect(context);
        AnimGraphStateTransition::Reflect(context);
        AnimGraphExitNode::Reflect(context);
        AnimGraphEntryNode::Reflect(context);

        AnimGraphMotionNode::Reflect(context);
        BlendSpaceNode::Reflect(context);
        BlendSpace1DNode::Reflect(context);
        BlendSpace2DNode::Reflect(context);
        AnimGraphBindPoseNode::Reflect(context);
        AnimGraphHubNode::Reflect(context);

        AnimGraphParameterCondition::Reflect(context);
        AnimGraphVector2Condition::Reflect(context);
        AnimGraphMotionCondition::Reflect(context);
        AnimGraphStateCondition::Reflect(context);
        AnimGraphTimeCondition::Reflect(context);
        AnimGraphTransitionCondition::Reflect(context);
        AnimGraphPlayTimeCondition::Reflect(context);
        AnimGraphTagCondition::Reflect(context);

        // Blend tree
        BlendTree::Reflect(context);
        BlendTreeConnection::Reflect(context);
        BlendTreeFinalNode::Reflect(context);
        BlendTreeBlend2NodeBase::Reflect(context);
        BlendTreeBlend2Node::Reflect(context);
        BlendTreeBlend2AdditiveNode::Reflect(context);
        BlendTreeBlend2LegacyNode::Reflect(context);
        BlendTreeBlendNNode::Reflect(context);
        BlendTreeParameterNode::Reflect(context);
        BlendTreeFloatMath1Node::Reflect(context);
        BlendTreeFloatMath2Node::Reflect(context);
        BlendTreeFloatConditionNode::Reflect(context);
        BlendTreeFloatConstantNode::Reflect(context);
        BlendTreeFloatSwitchNode::Reflect(context);
        BlendTreeBoolLogicNode::Reflect(context);
        BlendTreePoseSwitchNode::Reflect(context);
        BlendTreeMaskNode::Reflect(context);
        BlendTreeMorphTargetNode::Reflect(context);
        BlendTreeMotionFrameNode::Reflect(context);
        BlendTreeVector3Math1Node::Reflect(context);
        BlendTreeVector3Math2Node::Reflect(context);
        BlendTreeVector2DecomposeNode::Reflect(context);
        BlendTreeVector3DecomposeNode::Reflect(context);
        BlendTreeVector4DecomposeNode::Reflect(context);
        BlendTreeVector2ComposeNode::Reflect(context);
        BlendTreeVector3ComposeNode::Reflect(context);
        BlendTreeVector4ComposeNode::Reflect(context);
        BlendTreeSmoothingNode::Reflect(context);
        BlendTreeRangeRemapperNode::Reflect(context);
        BlendTreeDirectionToWeightNode::Reflect(context);
        BlendTreeMirrorPoseNode::Reflect(context);
        BlendTreeTwoLinkIKNode::Reflect(context);
        BlendTreeLookAtNode::Reflect(context);
        BlendTreeTransformNode::Reflect(context);
        BlendTreeAccumTransformNode::Reflect(context);
        BlendTreePoseSubtractNode::Reflect(context);
    }

    AZStd::unordered_set<AZ::TypeId>& AnimGraphObjectFactory::GetUITypes()
    {
        static AZStd::unordered_set<AZ::TypeId> uitypes = {
            azrtti_typeid<AnimGraphBindPoseNode>(),
            azrtti_typeid<AnimGraphStateMachine>(),
            azrtti_typeid<AnimGraphMotionNode>(),
            azrtti_typeid<AnimGraphHubNode>(),
            azrtti_typeid<AnimGraphExitNode>(),
            azrtti_typeid<AnimGraphEntryNode>(),
            azrtti_typeid<BlendTree>(),
            azrtti_typeid<BlendTreeFinalNode>(),
            azrtti_typeid<BlendSpace1DNode>(),
            azrtti_typeid<BlendSpace2DNode>(),
            azrtti_typeid<BlendTreeBlend2Node>(),
            azrtti_typeid<BlendTreeBlend2AdditiveNode>(),
            azrtti_typeid<BlendTreeBlend2LegacyNode>(),
            azrtti_typeid<BlendTreeBlendNNode>(),
            azrtti_typeid<BlendTreeParameterNode>(),
            azrtti_typeid<BlendTreeFloatMath1Node>(),
            azrtti_typeid<BlendTreeFloatMath2Node>(),
            azrtti_typeid<BlendTreeFloatConditionNode>(),
            azrtti_typeid<BlendTreeFloatConstantNode>(),
            azrtti_typeid<BlendTreeFloatSwitchNode>(),
            azrtti_typeid<BlendTreeBoolLogicNode>(),
            azrtti_typeid<BlendTreePoseSwitchNode>(),
            azrtti_typeid<BlendTreeMaskNode>(),
            azrtti_typeid<BlendTreeMorphTargetNode>(),
            azrtti_typeid<BlendTreeMotionFrameNode>(),
            azrtti_typeid<BlendTreeVector3Math1Node>(),
            azrtti_typeid<BlendTreeVector3Math2Node>(),
            azrtti_typeid<BlendTreeVector2DecomposeNode>(),
            azrtti_typeid<BlendTreeVector3DecomposeNode>(),
            azrtti_typeid<BlendTreeVector4DecomposeNode>(),
            azrtti_typeid<BlendTreeVector2ComposeNode>(),
            azrtti_typeid<BlendTreeVector3ComposeNode>(),
            azrtti_typeid<BlendTreeVector4ComposeNode>(),
            azrtti_typeid<BlendTreeSmoothingNode>(),
            azrtti_typeid<BlendTreeRangeRemapperNode>(),
            azrtti_typeid<BlendTreeDirectionToWeightNode>(),
            azrtti_typeid<BlendTreeMirrorPoseNode>(),
            azrtti_typeid<BlendTreeTwoLinkIKNode>(),
            azrtti_typeid<BlendTreeLookAtNode>(),
            azrtti_typeid<BlendTreeTransformNode>(),
            azrtti_typeid<BlendTreeAccumTransformNode>(),
            azrtti_typeid<BlendTreePoseSubtractNode>(),
            azrtti_typeid<AnimGraphStateTransition>(),
            azrtti_typeid<AnimGraphParameterCondition>(),
            azrtti_typeid<AnimGraphVector2Condition>(),
            azrtti_typeid<AnimGraphMotionCondition>(),
            azrtti_typeid<AnimGraphStateCondition>(),
            azrtti_typeid<AnimGraphTimeCondition>(),
            azrtti_typeid<AnimGraphPlayTimeCondition>(),
            azrtti_typeid<AnimGraphTagCondition>()
        };

        return uitypes;
    }


    AnimGraphObject* AnimGraphObjectFactory::Create(const AZ::TypeId& type, AnimGraph* animGraph)
    {
        AZ::SerializeContext* context = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
        if (!context)
        {
            AZ_Error("EMotionFX", false, "Can't get serialize context from component application.");
            return nullptr;
        }

        const AZ::SerializeContext::ClassData* classData = context->FindClassData(type);
        if (!classData)
        {
            AZ_Warning("EMotionFX", false, "Can't find class data for this type.");
            return nullptr;
        }

        AnimGraphObject* animGraphObject = reinterpret_cast<AnimGraphObject*>(classData->m_factory->Create(classData->m_name));
        if (animGraph)
        {
            animGraphObject->InitAfterLoading(animGraph);
        }
        return animGraphObject;
    }
    
} // namespace EMotionFX