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


#include "EMotionFX_precompiled.h"

#include <platform_impl.h>

#include <Integration/System/SystemComponent.h>
#include <Integration/Components/ActorComponent.h>
#include <Integration/Components/AnimAudioComponent.h>
#include <Integration/Components/AnimGraphComponent.h>
#include <Integration/Components/SimpleMotionComponent.h>
#include <AzCore/Module/DynamicModuleHandle.h>

#include <IGem.h>

#if defined (EMOTIONFXANIMATION_EDITOR)
#   include <Integration/System/PipelineComponent.h>
#   include <Integration/Editor/Components/EditorActorComponent.h>
#   include <Integration/Editor/Components/EditorAnimAudioComponent.h>
#   include <Integration/Editor/Components/EditorAnimGraphComponent.h>
#   include <Integration/Editor/Components/EditorSimpleMotionComponent.h>
#   include <SceneAPIExt/Behaviors/ActorGroupBehavior.h>
#   include <SceneAPIExt/Behaviors/MeshRuleBehavior.h>
#   include <SceneAPIExt/Behaviors/MotionGroupBehavior.h>
#   include <SceneAPIExt/Behaviors/MotionRangeRuleBehavior.h>
#   include <SceneAPIExt/Behaviors/SkinRuleBehavior.h>
#   include <SceneAPIExt/Behaviors/MorphTargetRuleBehavior.h>
#   include <RCExt/Actor/ActorExporter.h>
#   include <RCExt/Actor/ActorGroupExporter.h>
#   include <RCExt/Actor/ActorBuilder.h>
#   include <RCExt/Actor/MorphTargetExporter.h>
#   include <RCExt/Motion/MotionExporter.h>
#   include <RCExt/Motion/MotionGroupExporter.h>
#   include <RCExt/Motion/MotionDataBuilder.h>
#endif // EMOTIONFXANIMATION_EDITOR

namespace EMotionFX
{
    namespace Integration
    {
        /**
         * Animation module class for EMotion FX animation gem.
         */
        class EMotionFXIntegrationModule
            : public CryHooksModule
        {
        public:
            AZ_RTTI(EMotionFXIntegrationModule, "{02533EDC-F2AA-4076-86E9-5E3702202E15}", CryHooksModule);

            EMotionFXIntegrationModule()
                : CryHooksModule()
            {
                // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
                m_descriptors.insert(m_descriptors.end(), {
                    // System components
                    SystemComponent::CreateDescriptor(),

                    // Runtime components
                    ActorComponent::CreateDescriptor(),
                    AnimAudioComponent::CreateDescriptor(),
                    AnimGraphComponent::CreateDescriptor(),
                    SimpleMotionComponent::CreateDescriptor(),

    #if defined(EMOTIONFXANIMATION_EDITOR)
                    // Pipeline components
                    EMotionFX::Pipeline::PipelineComponent::CreateDescriptor(),

                    // Editor components
                    EditorActorComponent::CreateDescriptor(),
                    EditorAnimAudioComponent::CreateDescriptor(),
                    EditorAnimGraphComponent::CreateDescriptor(),
                    EditorSimpleMotionComponent::CreateDescriptor(),

                    // Actor
                    EMotionFX::Pipeline::Behavior::ActorGroupBehavior::CreateDescriptor(),
                    EMotionFX::Pipeline::Behavior::MeshRuleBehavior::CreateDescriptor(),
                    EMotionFX::Pipeline::Behavior::MorphTargetRuleBehavior::CreateDescriptor(),
                    EMotionFX::Pipeline::ActorExporter::CreateDescriptor(),
                    EMotionFX::Pipeline::ActorGroupExporter::CreateDescriptor(),
                    EMotionFX::Pipeline::ActorBuilder::CreateDescriptor(),
                    EMotionFX::Pipeline::MorphTargetExporter::CreateDescriptor(),

                    // Motion
                    EMotionFX::Pipeline::Behavior::MotionGroupBehavior::CreateDescriptor(),
                    EMotionFX::Pipeline::Behavior::SkinRuleBehavior::CreateDescriptor(),
                    EMotionFX::Pipeline::Behavior::MotionRangeRuleBehavior::CreateDescriptor(),
                    EMotionFX::Pipeline::MotionExporter::CreateDescriptor(),
                    EMotionFX::Pipeline::MotionGroupExporter::CreateDescriptor(),
                    EMotionFX::Pipeline::MotionDataBuilder::CreateDescriptor()
    #endif // EMOTIONFXANIMATION_EDITOR
                });
            }

            ~EMotionFXIntegrationModule()
            {
            }

            /**
             * Add required SystemComponents to the SystemEntity.
             */
            AZ::ComponentTypeList GetRequiredSystemComponents() const override
            {
                return AZ::ComponentTypeList{
                    azrtti_typeid<SystemComponent>(),
                };
            }
        };
    }
}

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(EMotionFX_044a63ea67d04479aa5daf62ded9d9ca, EMotionFX::Integration::EMotionFXIntegrationModule)
