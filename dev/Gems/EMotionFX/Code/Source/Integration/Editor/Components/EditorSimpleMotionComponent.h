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

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Asset/AssetCommon.h>

#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <Integration/Components/SimpleMotionComponent.h>
#include <Integration/Components/ActorComponent.h>

namespace EMotionFX
{
    namespace Integration
    {
        class EditorSimpleMotionComponent
            : public AzToolsFramework::Components::EditorComponentBase
            , private AZ::Data::AssetBus::Handler
        {
        public:

            AZ_EDITOR_COMPONENT(EditorSimpleMotionComponent, "{0CF1ADF7-DA51-4183-89EC-BDD7D2E17D36}");

            EditorSimpleMotionComponent();
            ~EditorSimpleMotionComponent() override;

            // AZ::Component interface implementation
            void Activate() override;
            void Deactivate() override;

            // AZ::Data::AssetBus::Handler
            void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
            void OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset) override;

            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
            {
                SimpleMotionComponent::GetProvidedServices(provided);
            }

            static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
            {
                SimpleMotionComponent::GetDependentServices(dependent);
            }

            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
            {
                SimpleMotionComponent::GetRequiredServices(required);
            }

            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
            {
                SimpleMotionComponent::GetIncompatibleServices(incompatible);
            }

            static void Reflect(AZ::ReflectContext* context);

        private:
            EditorSimpleMotionComponent(const EditorSimpleMotionComponent&) = delete;

            void BuildGameEntity(AZ::Entity* gameEntity) override;
            void VerifyMotionAssetState();

            SimpleMotionComponent::Configuration m_configuration;
        };
    }
}