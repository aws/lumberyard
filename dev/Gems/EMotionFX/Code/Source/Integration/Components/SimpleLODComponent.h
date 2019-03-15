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
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Script/ScriptProperty.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <Integration/Assets/MotionAsset.h>
#include <Integration/ActorComponentBus.h>


namespace EMotionFX
{
    namespace Integration
    {
        class SimpleLODComponent
            : public AZ::Component
            , private AZ::TickBus::Handler
            , private ActorComponentNotificationBus::Handler
        {
        public:

            friend class EditorSimpleLODComponent;

            AZ_COMPONENT(SimpleLODComponent, "{9380B039-EB03-4920-9F06-D90481E739E6}");

            /**
            * Configuration struct for procedural configuration of SimpleLODComponents.
            */
            struct Configuration
            {
                AZ_TYPE_INFO(Configuration, "{262470E5-57D8-4C45-8BB4-88EDFBC54D7E}");
                Configuration();

                AZStd::vector<float> m_lodDistances;         // Lod distances that decide which lod the actor should choose.

                static void Reflect(AZ::ReflectContext* context);
            };

            SimpleLODComponent(const Configuration* config = nullptr);
            ~SimpleLODComponent();

            // AZ::Component interface implementation
            void Init() override;
            void Activate() override;
            void Deactivate() override;

            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
            {
                provided.push_back(AZ_CRC("EMotionFXSimpleLODService", 0xa9b5f358));
            }
            static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
            {
            }
            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
            {
                required.push_back(AZ_CRC("EMotionFXActorService", 0xd6e8f48d));
            }
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
            {
                incompatible.push_back(AZ_CRC("EMotionFXSimpleLODService", 0xa9b5f358));
            }
            static void Reflect(AZ::ReflectContext* context);

        private:
            // ActorComponentNotificationBus::Handler
            void OnActorInstanceCreated(EMotionFX::ActorInstance* actorInstance) override;
            void OnActorInstanceDestroyed(EMotionFX::ActorInstance* actorInstance) override;

            // AZ::TickBus::Handler
            void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

            static AZ::u32 GetLODByDistance(const AZStd::vector<float>& distances, float distance);
            static void UpdateLODLevelByDistance(EMotionFX::ActorInstance* actorInstance, const AZStd::vector<float>& distances, AZ::EntityId entityId);

            Configuration                               m_configuration;        // Component configuration.
            EMotionFX::ActorInstance*                   m_actorInstance;        // Associated actor instance (retrieved from Actor Component).
        };

    } // namespace Integration
} // namespace EMotionFX

