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

#include <AzFramework/Physics/RagdollPhysicsBus.h>
#include <AzFramework/Physics/CharacterPhysicsDataBus.h>
#include <AzFramework/Physics/SystemBus.h>

#include <Integration/Assets/ActorAsset.h>
#include <Integration/Assets/MotionSetAsset.h>
#include <Integration/Assets/AnimGraphAsset.h>
#include <Integration/ActorComponentBus.h>

#include <LmbrCentral/Rendering/MeshComponentBus.h>
#include <LmbrCentral/Rendering/RenderNodeBus.h>
#include <LmbrCentral/Rendering/RenderBoundsBus.h>
#include <LmbrCentral/Rendering/MaterialOwnerBus.h>
#include <LmbrCentral/Animation/AttachmentComponentBus.h>


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
            , private LmbrCentral::RenderBoundsRequestBus::Handler
            , private ActorComponentRequestBus::Handler
            , private ActorComponentNotificationBus::Handler
            , private LmbrCentral::MaterialOwnerRequestBus::Handler
            , private LmbrCentral::AttachmentComponentNotificationBus::Handler
            , private AzFramework::CharacterPhysicsDataRequestBus::Handler
            , private AzFramework::RagdollPhysicsNotificationBus::Handler
            , protected Physics::SystemNotificationBus::Handler
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
                bool                            m_renderBounds;             ///< Toggles rendering of the character bounds used for visibility testing.
                SkinningMethod                  m_skinningMethod;           ///< The skinning method for this actor
                AZ::u32                         m_lodLevel;

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
            // RenderBoundsRequestBus interface implementation
            AZ::Aabb GetWorldBounds() override;
            AZ::Aabb GetLocalBounds() override;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // LmbrCentral::MeshComponentRequestBus::Handler
            bool GetVisibility() override;
            void SetVisibility(bool isVisible) override;
            void SetMeshAsset(const AZ::Data::AssetId& id) override { (void)id; }
            AZ::Data::Asset<AZ::Data::AssetData> GetMeshAsset() override { return m_configuration.m_actorAsset; }
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // ActorComponentRequestBus::Handler
            size_t GetJointIndexByName(const char* name) const override;
            AZ::Transform GetJointTransform(size_t jointIndex, Space space) const override;
            void GetJointTransformComponents(size_t jointIndex, Space space, AZ::Vector3& outPosition, AZ::Quaternion& outRotation, AZ::Vector3& outScale) const override;
            Physics::AnimationConfiguration* GetPhysicsConfig() const override;

            EMotionFX::ActorInstance* GetActorInstance() override { return m_actorInstance ? m_actorInstance.get() : nullptr; }
            void AttachToEntity(AZ::EntityId targetEntityId, AttachmentType attachmentType) override;
            void DetachFromEntity() override;
            void DebugDrawRoot(bool enable) override;
            bool GetRenderCharacter() const override;
            void SetRenderCharacter(bool enable) override;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // ActorComponentNotificationBus::Handler
            void OnActorInstanceCreated(EMotionFX::ActorInstance* actorInstance) override;
            void OnActorInstanceDestroyed(EMotionFX::ActorInstance* actorInstance) override;
            //////////////////////////////////////////////////////////////////////////

            // The entity has attached to the target.
            void OnAttached(AZ::EntityId targetId) override;

            // The entity is detaching from the target.
            void OnDetached(AZ::EntityId targetId) override;

            //////////////////////////////////////////////////////////////////////////
            // ActorComponentNotificationBus::Handler
            IRenderNode* GetRenderNode() override;
            float GetRenderNodeRequestBusOrder() const override;
            static const float s_renderNodeRequestBusOrder;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // MaterialOwnerRequestBus interface implementation
            bool IsMaterialOwnerReady() override;
            void SetMaterial(_smart_ptr<IMaterial>) override;
            _smart_ptr<IMaterial> GetMaterial() override;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // AzFramework::CharacterPhysicsDataBus::Handler
            bool GetRagdollConfiguration(Physics::RagdollConfiguration& config) const override;
            Physics::RagdollState GetBindPose(const Physics::RagdollConfiguration& config) const override;
            AZStd::string GetParentNodeName(const AZStd::string& childName) const override;

            //////////////////////////////////////////////////////////////////////////
            // AzFramework::RagdollPhysicsNotificationBus::Handler
            void OnRagdollActivated() override;
            void OnRagdollDeactivated() override;

            //////////////////////////////////////////////////////////////////////////
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
            {
                provided.push_back(AZ_CRC("EMotionFXActorService", 0xd6e8f48d));
                provided.push_back(AZ_CRC("MeshService", 0x71d8a455));
                provided.push_back(AZ_CRC("CharacterPhysicsDataService", 0x34757927));
            }

            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
            {
                incompatible.push_back(AZ_CRC("EMotionFXActorService", 0xd6e8f48d));
                incompatible.push_back(AZ_CRC("MeshService", 0x71d8a455));
            }

            static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
            {
                dependent.push_back(AZ_CRC("PhysicsService", 0xa7350d22));
            }

            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
            {
                required.push_back(AZ_CRC("TransformService", 0x8ee22c50));
            }

            static void Reflect(AZ::ReflectContext* context);
            //////////////////////////////////////////////////////////////////////////

            static void DrawSkeleton(const EMotionFXPtr<EMotionFX::ActorInstance>& actorInstance);
            static void DrawBounds(const EMotionFXPtr<EMotionFX::ActorInstance>& actorInstance);

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
            int GetTickOrder() override;

            // Physics::SystemNotifications::Handler
            void OnPostPhysicsUpdate(float fixedDeltaTime, Physics::World* physicsWorld) override;

            void CheckActorCreation();
            void DestroyActor();
            void CheckAttachToEntity();
            void RenderDebugDraw();

            Configuration                                   m_configuration;            ///< Component configuration.
                                                                                        /// Live state
            ActorAsset::ActorInstancePtr                    m_attachmentTargetActor;    ///< Target actor instance to attach to.
            AZ::EntityId                                    m_attachmentTargetEntityId; ///< Target actor entity ID
            ActorAsset::ActorInstancePtr                    m_actorInstance;            ///< Live actor instance.
            AnimGraphAsset::AnimGraphInstancePtr            m_animGraphInstance;        ///< Live anim graph instance.
            AZStd::vector<AZ::EntityId>                     m_attachments;
            AZStd::unique_ptr<ActorRenderNode>              m_renderNode;               ///< Actor render node.
            bool                                            m_debugDrawRoot;            ///< Enables drawing of actor root and facing.
            bool                                            m_materialReadyEventSent;   ///< Tracks whether OnMaterialOwnerReady has been sent yet. - TBD
        };
    } //namespace Integration
} // namespace EMotionFX

