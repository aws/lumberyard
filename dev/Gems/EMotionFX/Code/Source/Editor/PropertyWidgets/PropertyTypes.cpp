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

#include <Editor/PropertyWidgets/ActorMorphTargetHandler.h>
#include <Editor/PropertyWidgets/ActorGoalNodeHandler.h>
#include <Editor/PropertyWidgets/ActorNodeHandler.h>
#include <Editor/PropertyWidgets/AnimGraphNodeHandler.h>
#include <Editor/PropertyWidgets/AnimGraphNodeNameHandler.h>
#include <Editor/PropertyWidgets/AnimGraphParameterHandler.h>
#include <Editor/PropertyWidgets/AnimGraphTagHandler.h>
#include <Editor/PropertyWidgets/AnimGraphParameterHandler.h>
#include <Editor/PropertyWidgets/AnimGraphParameterMaskHandler.h>
#include <Editor/PropertyWidgets/BlendSpaceEvaluatorHandler.h>
#include <Editor/PropertyWidgets/BlendSpaceMotionContainerHandler.h>
#include <Editor/PropertyWidgets/BlendSpaceMotionHandler.h>
#include <Editor/PropertyWidgets/MotionSetMotionIdHandler.h>
#include <Editor/PropertyWidgets/MotionSetNameHandler.h>
#include <Editor/PropertyWidgets/TransitionStateFilterLocalHandler.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>


namespace EMotionFX
{
    void RegisterPropertyTypes()
    {
#if defined(EMOTIONFXANIMATION_EDITOR)
        AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Broadcast(&AzToolsFramework::PropertyTypeRegistrationMessages::RegisterPropertyType, aznew EMotionFX::ActorSingleNodeHandler());
        AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Broadcast(&AzToolsFramework::PropertyTypeRegistrationMessages::RegisterPropertyType, aznew EMotionFX::ActorMultiNodeHandler());
        AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Broadcast(&AzToolsFramework::PropertyTypeRegistrationMessages::RegisterPropertyType, aznew EMotionFX::ActorMultiWeightedNodeHandler());
        AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Broadcast(&AzToolsFramework::PropertyTypeRegistrationMessages::RegisterPropertyType, aznew EMotionFX::ActorSingleMorphTargetHandler());
        AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Broadcast(&AzToolsFramework::PropertyTypeRegistrationMessages::RegisterPropertyType, aznew EMotionFX::ActorMultiMorphTargetHandler());
        AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Broadcast(&AzToolsFramework::PropertyTypeRegistrationMessages::RegisterPropertyType, aznew EMotionFX::ActorGoalNodeHandler());
        AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Broadcast(&AzToolsFramework::PropertyTypeRegistrationMessages::RegisterPropertyType, aznew EMotionFX::AnimGraphNodeIdHandler());
        AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Broadcast(&AzToolsFramework::PropertyTypeRegistrationMessages::RegisterPropertyType, aznew EMotionFX::AnimGraphNodeNameHandler());
        AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Broadcast(&AzToolsFramework::PropertyTypeRegistrationMessages::RegisterPropertyType, aznew EMotionFX::AnimGraphMotionNodeIdHandler());
        AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Broadcast(&AzToolsFramework::PropertyTypeRegistrationMessages::RegisterPropertyType, aznew EMotionFX::AnimGraphSingleParameterHandler());
        AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Broadcast(&AzToolsFramework::PropertyTypeRegistrationMessages::RegisterPropertyType, aznew EMotionFX::AnimGraphMultipleParameterHandler());
        AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Broadcast(&AzToolsFramework::PropertyTypeRegistrationMessages::RegisterPropertyType, aznew EMotionFX::AnimGraphParameterMaskHandler());
        AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Broadcast(&AzToolsFramework::PropertyTypeRegistrationMessages::RegisterPropertyType, aznew EMotionFX::AnimGraphStateIdHandler());
        AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Broadcast(&AzToolsFramework::PropertyTypeRegistrationMessages::RegisterPropertyType, aznew EMotionFX::AnimGraphTagHandler());
        AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Broadcast(&AzToolsFramework::PropertyTypeRegistrationMessages::RegisterPropertyType, aznew EMotionFX::BlendSpaceEvaluatorHandler());
        AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Broadcast(&AzToolsFramework::PropertyTypeRegistrationMessages::RegisterPropertyType, aznew EMotionFX::BlendSpaceMotionContainerHandler());
        AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Broadcast(&AzToolsFramework::PropertyTypeRegistrationMessages::RegisterPropertyType, aznew EMotionFX::BlendSpaceMotionHandler());
        AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Broadcast(&AzToolsFramework::PropertyTypeRegistrationMessages::RegisterPropertyType, aznew EMotionFX::TransitionStateFilterLocalHandler());
        AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Broadcast(&AzToolsFramework::PropertyTypeRegistrationMessages::RegisterPropertyType, aznew EMotionFX::MotionSetMultiMotionIdHandler());
        AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Broadcast(&AzToolsFramework::PropertyTypeRegistrationMessages::RegisterPropertyType, aznew EMotionFX::MotionSetNameHandler());
#endif
    }


    void UnregisterPropertyTypes()
    {
#if defined(EMOTIONFXANIMATION_EDITOR)
        AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Broadcast(&AzToolsFramework::PropertyTypeRegistrationMessages::UnregisterPropertyType, aznew EMotionFX::ActorSingleNodeHandler());
        AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Broadcast(&AzToolsFramework::PropertyTypeRegistrationMessages::UnregisterPropertyType, aznew EMotionFX::ActorMultiNodeHandler());
        AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Broadcast(&AzToolsFramework::PropertyTypeRegistrationMessages::UnregisterPropertyType, aznew EMotionFX::ActorMultiWeightedNodeHandler());
        AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Broadcast(&AzToolsFramework::PropertyTypeRegistrationMessages::UnregisterPropertyType, aznew EMotionFX::ActorSingleMorphTargetHandler());
        AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Broadcast(&AzToolsFramework::PropertyTypeRegistrationMessages::UnregisterPropertyType, aznew EMotionFX::ActorMultiMorphTargetHandler());
        AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Broadcast(&AzToolsFramework::PropertyTypeRegistrationMessages::UnregisterPropertyType, aznew EMotionFX::ActorGoalNodeHandler());
        AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Broadcast(&AzToolsFramework::PropertyTypeRegistrationMessages::UnregisterPropertyType, aznew EMotionFX::AnimGraphNodeIdHandler());
        AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Broadcast(&AzToolsFramework::PropertyTypeRegistrationMessages::UnregisterPropertyType, aznew EMotionFX::AnimGraphNodeNameHandler());
        AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Broadcast(&AzToolsFramework::PropertyTypeRegistrationMessages::UnregisterPropertyType, aznew EMotionFX::AnimGraphMotionNodeIdHandler());
        AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Broadcast(&AzToolsFramework::PropertyTypeRegistrationMessages::UnregisterPropertyType, aznew EMotionFX::AnimGraphSingleParameterHandler());
        AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Broadcast(&AzToolsFramework::PropertyTypeRegistrationMessages::UnregisterPropertyType, aznew EMotionFX::AnimGraphMultipleParameterHandler());
        AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Broadcast(&AzToolsFramework::PropertyTypeRegistrationMessages::UnregisterPropertyType, aznew EMotionFX::AnimGraphParameterMaskHandler());
        AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Broadcast(&AzToolsFramework::PropertyTypeRegistrationMessages::UnregisterPropertyType, aznew EMotionFX::AnimGraphStateIdHandler());
        AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Broadcast(&AzToolsFramework::PropertyTypeRegistrationMessages::UnregisterPropertyType, aznew EMotionFX::AnimGraphTagHandler());
        AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Broadcast(&AzToolsFramework::PropertyTypeRegistrationMessages::UnregisterPropertyType, aznew EMotionFX::BlendSpaceEvaluatorHandler());
        AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Broadcast(&AzToolsFramework::PropertyTypeRegistrationMessages::UnregisterPropertyType, aznew EMotionFX::BlendSpaceMotionContainerHandler());
        AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Broadcast(&AzToolsFramework::PropertyTypeRegistrationMessages::UnregisterPropertyType, aznew EMotionFX::BlendSpaceMotionHandler());
        AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Broadcast(&AzToolsFramework::PropertyTypeRegistrationMessages::UnregisterPropertyType, aznew EMotionFX::TransitionStateFilterLocalHandler());
        AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Broadcast(&AzToolsFramework::PropertyTypeRegistrationMessages::UnregisterPropertyType, aznew EMotionFX::MotionSetMultiMotionIdHandler());
        AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Broadcast(&AzToolsFramework::PropertyTypeRegistrationMessages::UnregisterPropertyType, aznew EMotionFX::MotionSetNameHandler());
#endif
    }
} // namespace EMotionFX