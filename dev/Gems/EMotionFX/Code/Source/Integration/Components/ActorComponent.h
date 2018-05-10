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
#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <Integration/Assets/ActorAsset.h>
#include <Integration/Assets/MotionSetAsset.h>
#include <Integration/Assets/AnimGraphAsset.h>
#include <Integration/ActorComponentBus.h>

#include <LmbrCentral/Rendering/MeshComponentBus.h>
#include <LmbrCentral/Rendering/RenderNodeBus.h>

namespace EMotionFX
{
    namespace Integration
    {
        class ActorRenderNode;

        class ActorComponent
            : public AZ::Component
            , private AZ::Data::AssetBus::Handler
            , private AZ::TransformNotificationBus::MultiHandler
            , private AZ::TickBus::Handler
            , private LmbrCentral::MeshComponentRequestBus::Handler
            , private LmbrCentral::RenderNodeRequestBus::Handler
            , private ActorComponentRequestBus::Handler
            , private ActorComponentNotificationBus::Handler
        {
        public:

            friend class EditorActorComponent;

            AZ_COMPONENT(ActorComponent, "{BDC97E7F-A054-448B-A26F-EA2B5D78E377}");

            /**
            * Configuration struct for procedural configuration of Actor Components.
            */
            struct Configuration
            {
                AZ_TYPE_INFO(Configuration, "{053BFBC0-ABAA-4F4E-911F-5320F941E1A8}")

                    Configuration();

                AZ::Data::Asset<ActorAsset>     m_actorAsset;               ///< Selected actor asset.
                ActorAsset::MaterialList        m_materialPerLOD;           ///< Material assignment per LOD.
                AZ::EntityId                    m_attachmentTarget;         ///< Target entity this actor should attach to.
                AZ::u32                         m_attachmentJointIndex;     ///< Index of joint on target skeleton for actor attachments.
                AttachmentType                  m_attachmentType;           ///< Type of attachment.
                bool                            m_renderSkeleton;           ///< Toggles debug rendering of the skeleton.
                bool                            m_renderCharacter;          ///< Toggles rendering of the character.
                SkinningMethod                  m_skinningMethod;           ///< The skinning method for this actor

                static void Reflect(AZ::ReflectContext* context);
            };

            ActorComponent(const Configuration* configuration = nullptr);
            ~ActorComponent() override;

            //////////////////////////////////////////////////////////////////////////
            // AZ::Component interface implementation
            void Activate() override;
            void Deactivate() override;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // LmbrCentral::MeshComponentRequestBus::Handler
            AZ::Aabb GetWorldBounds() override;
            AZ::Aabb GetLocalBounds() override;
            bool GetVisibility() override;
            void SetVisibility(bool isVisible) override;
            void SetMeshAsset(const AZ::Data::AssetId& id) override { (void)id; }
            AZ::Data::Asset<AZ::Data::AssetData> GetMeshAsset() override { return m_configuration.m_actorAsset; }
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // ActorComponentRequestBus::Handler
            EMotionFX::ActorInstance* GetActorInstance() override { return m_actorInstance ? m_actorInstance.get() : nullptr; }
            void AttachToEntity(AZ::EntityId targetEntityId, AttachmentType attachmentType) override;
            void DetachFromEntity() override;
            void DebugDrawRoot(bool enable) override;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // ActorComponentNotificationBus::Handler
            void OnActorInstanceCreated(EMotionFX::ActorInstance* actorInstance) override;
            void OnActorInstanceDestroyed(EMotionFX::ActorInstance* actorInstance) override;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // ActorComponentNotificationBus::Handler
            IRenderNode* GetRenderNode() override;
            float GetRenderNodeRequestBusOrder() const override;
            static const float s_renderNodeRequestBusOrder;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
            {
                provided.push_back(AZ_CRC("EMotionFXActorService", 0xd6e8f48d));
                provided.push_back(AZ_CRC("MeshService", 0x71d8a455));
            }

            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
            {
                incompatible.push_back(AZ_CRC("EMotionFXActorService", 0xd6e8f48d));
                incompatible.push_back(AZ_CRC("MeshService", 0x71d8a455));
            }

            static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& /*dependent*/)
            {
            }

            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
            {
                required.push_back(AZ_CRC("TransformService", 0x8ee22c50));
            }

            static void Reflect(AZ::ReflectContext* context);
            //////////////////////////////////////////////////////////////////////////

            static void DrawSkeleton(const EMotionFXPtr<EMotionFX::ActorInstance>& actorInstance);

        private:
            //vs 2013 build limitation
            //unique_ptr cannot be copied ->class cannot be copied
            ActorComponent(const ActorComponent&) = delete;

            // AZ::Data::AssetBus::Handler
            void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
            void OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset) override;

            // AZ::TransformNotificationBus::MultiHandler
            void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

            // AZ::TickBus::Handler
            void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

            void CheckActorCreation();
            void DestroyActor();
            void CheckAttachToEntity();

            Configuration                                   m_configuration;            ///< Component configuration.

                                                                                        /// Live state
            ActorAsset::ActorInstancePtr                    m_attachmentTargetActor;    ///< Target actor instance to attach to.
            AZ::EntityId                                    m_attachmentTargetEntityId; ///< Target actor entity ID
            ActorAsset::ActorInstancePtr                    m_actorInstance;            ///< Live actor instance.
            AnimGraphAsset::AnimGraphInstancePtr            m_animGraphInstance;        ///< Live anim graph instance.
            AZStd::unique_ptr<ActorRenderNode>              m_renderNode;               ///< Actor render node.
            bool                                            m_debugDrawRoot;            ///< Enables drawing of actor root and facing.
        };
    } //namespace Integration

} // namespace EMotionFX

