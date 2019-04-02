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

#include <AzCore/Component/Entity.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Math/Transform.h>

#include <AzFramework/Physics/Ragdoll.h>
#include <AzFramework/Physics/RagdollPhysicsBus.h>

#include <LmbrCentral/Animation/AttachmentComponentBus.h>

#include <Integration/Components/ActorComponent.h>

#include <EMotionFX/Source/Transform.h>
#include <EMotionFX/Source/RagdollInstance.h>

#include <MathConversion.h>
#include <IRenderAuxGeom.h>


namespace EMotionFX
{
    namespace Integration
    {
        //////////////////////////////////////////////////////////////////////////
        void ActorComponent::Configuration::Reflect(AZ::ReflectContext* context)
        {
            auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (serializeContext)
            {
                serializeContext->Class<Configuration>()
                    ->Version(3)
                    ->Field("ActorAsset", &Configuration::m_actorAsset)
                    ->Field("MaterialPerLOD", &Configuration::m_materialPerLOD)
                    ->Field("RenderSkeleton", &Configuration::m_renderSkeleton)
                    ->Field("RenderCharacter", &Configuration::m_renderCharacter)
                    ->Field("RenderBounds", &Configuration::m_renderBounds)
                    ->Field("AttachmentType", &Configuration::m_attachmentType)
                    ->Field("AttachmentTarget", &Configuration::m_attachmentTarget)
                    ->Field("SkinningMethod", &Configuration::m_skinningMethod)
                    ->Field("LODLevel", &Configuration::m_lodLevel)
                ;
            }
        }

        //////////////////////////////////////////////////////////////////////////
        void ActorComponent::Reflect(AZ::ReflectContext* context)
        {
            auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (serializeContext)
            {
                Configuration::Reflect(context);

                serializeContext->Class<ActorComponent, AZ::Component>()
                    ->Version(1)
                    ->Field("Configuration", &ActorComponent::m_configuration)
                ;

                AZ::EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    editContext->Enum<EMotionFX::Integration::Space>("Space", "The transformation space.")
                        ->Value("Local Space", Space::LocalSpace)
                        ->Value("Model Space", Space::ModelSpace)
                        ->Value("World Space", Space::WorldSpace);
                }
            }

            auto* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
            if (behaviorContext)
            {
                behaviorContext->EBus<ActorComponentRequestBus>("ActorComponentRequestBus")
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::Preview)
                    ->Event("GetJointIndexByName", &ActorComponentRequestBus::Events::GetJointIndexByName)
                    ->Event("GetJointTransform", &ActorComponentRequestBus::Events::GetJointTransform)
                    ->Event("AttachToEntity", &ActorComponentRequestBus::Events::AttachToEntity)
                    ->Event("DetachFromEntity", &ActorComponentRequestBus::Events::DetachFromEntity)
                    ->Event("DebugDrawRoot", &ActorComponentRequestBus::Events::DebugDrawRoot)
                    ->Event("GetRenderCharacter", &ActorComponentRequestBus::Events::GetRenderCharacter)
                    ->Event("SetRenderCharacter", &ActorComponentRequestBus::Events::SetRenderCharacter)
                    ->VirtualProperty("RenderCharacter", "GetRenderCharacter", "SetRenderCharacter")
                ;

                behaviorContext->Class<ActorComponent>()->RequestBus("ActorComponentRequestBus");

                behaviorContext->EBus<ActorComponentNotificationBus>("ActorComponentNotificationBus")
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::List)
                    ->Event("OnActorInstanceCreated", &ActorComponentNotificationBus::Events::OnActorInstanceCreated)
                    ->Event("OnActorInstanceDestroyed", &ActorComponentNotificationBus::Events::OnActorInstanceDestroyed)
                ;
            }
        }

        //////////////////////////////////////////////////////////////////////////
        ActorComponent::Configuration::Configuration()
            : m_attachmentType(AttachmentType::None)
            , m_renderSkeleton(false)
            , m_renderCharacter(true)
            , m_renderBounds(false)
            , m_skinningMethod(SkinningMethod::DualQuat)
            , m_lodLevel(0)
            , m_actorAsset(AZ::Data::AssetLoadBehavior::NoLoad)
        {
        }

        //////////////////////////////////////////////////////////////////////////
        ActorComponent::ActorComponent(const Configuration* configuration)
            : m_debugDrawRoot(false)
            , m_materialReadyEventSent(false)
        {
            if (configuration)
            {
                m_configuration = *configuration;
            }
        }

        //////////////////////////////////////////////////////////////////////////
        ActorComponent::~ActorComponent()
        {
        }

        //////////////////////////////////////////////////////////////////////////
        void ActorComponent::Activate()
        {
            m_actorInstance.reset();

            AZ::Data::AssetBus::Handler::BusDisconnect();

            auto& cfg = m_configuration;

            if (cfg.m_actorAsset.GetId().IsValid())
            {
                AZ::Data::AssetBus::Handler::BusConnect(cfg.m_actorAsset.GetId());
                cfg.m_actorAsset.QueueLoad();
            }

            AZ::TickBus::Handler::BusConnect();

            LmbrCentral::MeshComponentRequestBus::Handler::BusConnect(GetEntityId());
            LmbrCentral::RenderNodeRequestBus::Handler::BusConnect(GetEntityId());
            LmbrCentral::MaterialOwnerRequestBus::Handler::BusConnect(GetEntityId());
            m_materialReadyEventSent = false;
            LmbrCentral::AttachmentComponentNotificationBus::Handler::BusConnect(GetEntityId());
            Physics::SystemNotificationBus::Handler::BusConnect();
            AzFramework::CharacterPhysicsDataRequestBus::Handler::BusConnect(GetEntityId());
            AzFramework::RagdollPhysicsNotificationBus::Handler::BusConnect(GetEntityId());

            if (cfg.m_attachmentTarget.IsValid())
            {
                AttachToEntity(cfg.m_attachmentTarget, cfg.m_attachmentType);
            }
        }

        //////////////////////////////////////////////////////////////////////////
        void ActorComponent::Deactivate()
        {
            AzFramework::RagdollPhysicsNotificationBus::Handler::BusDisconnect();
            AzFramework::CharacterPhysicsDataRequestBus::Handler::BusDisconnect();
            Physics::SystemNotificationBus::Handler::BusDisconnect();
            ActorComponentRequestBus::Handler::BusDisconnect();
            AZ::TickBus::Handler::BusDisconnect();
            ActorComponentNotificationBus::Handler::BusDisconnect();
            LmbrCentral::MeshComponentRequestBus::Handler::BusDisconnect();
            LmbrCentral::RenderNodeRequestBus::Handler::BusDisconnect();
            LmbrCentral::MaterialOwnerRequestBus::Handler::BusDisconnect();
            LmbrCentral::AttachmentComponentNotificationBus::Handler::BusDisconnect();
            AZ::TransformNotificationBus::MultiHandler::BusDisconnect();
            AZ::Data::AssetBus::Handler::BusDisconnect();

            m_renderNode = nullptr;

            DestroyActor();
            m_configuration.m_actorAsset.Release();
        }

        //////////////////////////////////////////////////////////////////////////
        void ActorComponent::AttachToEntity(AZ::EntityId targetEntityId, AttachmentType attachmentType)
        {
            if (targetEntityId.IsValid() && targetEntityId != GetEntityId())
            {
                ActorComponentNotificationBus::Handler::BusDisconnect();
                ActorComponentNotificationBus::Handler::BusConnect(targetEntityId);

                AZ::TransformNotificationBus::MultiHandler::BusConnect(targetEntityId);
                m_attachmentTargetEntityId = targetEntityId;

                // There's no guarantee that we will receive a on transform change call for the target entity because of the entity activate order.
                // Enforce a transform query on target to get the correct initial transform.
                AZ::Transform transform;
                AZ::TransformBus::EventResult(transform, GetEntityId(), &AZ::TransformBus::Events::GetWorldTM); // default to using our own TM
                AZ::TransformBus::EventResult(transform, targetEntityId, &AZ::TransformBus::Events::GetWorldTM); // attempt to get target's TM
                AZ::TransformBus::Event(GetEntityId(), &AZ::TransformBus::Events::SetWorldTM, transform); // set our TM
            }
            else
            {
                DetachFromEntity();
            }
        }

        //////////////////////////////////////////////////////////////////////////
        void ActorComponent::DetachFromEntity()
        {
            if (m_attachmentTargetActor)
            {
                m_attachmentTargetActor->RemoveAttachment(m_actorInstance.get());
                AZ::TransformBus::Event(GetEntityId(), &AZ::TransformBus::Events::SetParent, AZ::EntityId());
                AZ::TransformBus::Event(GetEntityId(), &AZ::TransformBus::Events::SetLocalTM, AZ::Transform::CreateIdentity());

                AZ::TransformNotificationBus::MultiHandler::BusDisconnect(m_attachmentTargetEntityId);
                m_attachmentTargetEntityId.SetInvalid();
            }
        }

        //////////////////////////////////////////////////////////////////////////
        void ActorComponent::DebugDrawRoot(bool enable)
        {
            m_debugDrawRoot = enable;
        }

        //////////////////////////////////////////////////////////////////////////
        bool ActorComponent::GetRenderCharacter() const
        {
            return m_configuration.m_renderCharacter;
        }

        //////////////////////////////////////////////////////////////////////////
        void ActorComponent::SetRenderCharacter(bool enable)
        {
            if (m_configuration.m_renderCharacter != enable)
            {
                m_configuration.m_renderCharacter = enable;
                if (m_renderNode)
                {
                    m_renderNode->Hide(!m_configuration.m_renderCharacter);
                }
            }
        }

        //////////////////////////////////////////////////////////////////////////
        void ActorComponent::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
        {
            OnAssetReady(asset);
        }

        //////////////////////////////////////////////////////////////////////////
        void ActorComponent::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
        {
            m_configuration.m_actorAsset = asset;

            CheckActorCreation();
        }

        //////////////////////////////////////////////////////////////////////////
        void ActorComponent::CheckActorCreation()
        {
            if (m_configuration.m_actorAsset.IsReady())
            {
                // Create actor instance.
                auto* actorAsset = m_configuration.m_actorAsset.GetAs<ActorAsset>();
                AZ_Error("EMotionFX", actorAsset, "Actor asset is not valid.");
                if (!actorAsset)
                {
                    return;
                }

                DestroyActor();

                m_actorInstance = actorAsset->CreateInstance(GetEntity());
                if (!m_actorInstance)
                {
                    AZ_Error("EMotionFX", actorAsset, "Failed to create actor instance.");
                    return;
                }

                ActorComponentRequestBus::Handler::BusConnect(GetEntityId());

                ActorComponentNotificationBus::Event(
                    GetEntityId(),
                    &ActorComponentNotificationBus::Events::OnActorInstanceCreated,
                    m_actorInstance.get());

                m_actorInstance->SetLODLevel(m_configuration.m_lodLevel);

                // Setup initial transform and listen for transform changes.
                AZ::Transform transform;
                AZ::TransformBus::EventResult(transform, GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);
                OnTransformChanged(transform, transform);
                AZ::TransformNotificationBus::MultiHandler::BusConnect(GetEntityId());

                m_actorInstance->UpdateWorldTransform();
                m_actorInstance->UpdateBounds(0, EMotionFX::ActorInstance::EBoundsType::BOUNDS_STATIC_BASED);

                m_renderNode = AZStd::make_unique<ActorRenderNode>(GetEntityId(), m_actorInstance, m_configuration.m_actorAsset, transform);
                m_renderNode->SetMaterials(m_configuration.m_materialPerLOD);
                m_renderNode->RegisterWithRenderer();
                m_renderNode->SetSkinningMethod(m_configuration.m_skinningMethod);
                m_renderNode->Hide(!m_configuration.m_renderCharacter);

                // Reattach all attachments
                for (AZ::EntityId& attachment : m_attachments)
                {
                    LmbrCentral::AttachmentComponentRequestBus::Event(attachment, &LmbrCentral::AttachmentComponentRequestBus::Events::Reattach, true);
                }

                LmbrCentral::AttachmentComponentRequestBus::Event(GetEntityId(), &LmbrCentral::AttachmentComponentRequestBus::Events::Reattach, true);

                CheckAttachToEntity();

                // Send general mesh creation notification to interested parties.
                LmbrCentral::MeshComponentNotificationBus::Event(GetEntityId(), &LmbrCentral::MeshComponentNotifications::OnMeshCreated, actorAsset);

                AzFramework::CharacterPhysicsDataNotificationBus::Event(GetEntityId(), &AzFramework::CharacterPhysicsDataNotifications::OnRagdollConfigurationReady);
            }
        }

        //////////////////////////////////////////////////////////////////////////
        void ActorComponent::CheckAttachToEntity()
        {
            // Attach to the target actor if we're both ready.
            // Note that m_attachmentTargetActor will always be null if we're not configured to attach to anything.
            if (m_actorInstance && m_attachmentTargetActor)
            {
                DetachFromEntity();

                // Make sure we don't generate some circular loop by attaching to each other.
                if (!m_attachmentTargetActor.get()->CheckIfCanHandleAttachment(m_actorInstance.get()))
                {
                    AZ_Error("EMotionFX", false, "You cannot attach to yourself or create circular dependencies!\n");
                    return;
                }

                // Create the attachment.
                AZ_Assert(m_configuration.m_attachmentType == AttachmentType::SkinAttachment, "Expected a skin attachment.");
                EMotionFX::Attachment* attachment = EMotionFX::AttachmentSkin::Create(m_attachmentTargetActor.get(), m_actorInstance.get());;
                m_actorInstance->SetLocalSpaceTransform(EMotionFX::Transform());
                m_attachmentTargetActor->AddAttachment(attachment);
                AZ::TransformBus::Event(GetEntityId(), &AZ::TransformBus::Events::SetParent, m_attachmentTargetActor->GetEntityId());
                AZ::TransformBus::Event(GetEntityId(), &AZ::TransformBus::Events::SetLocalTM, AZ::Transform::CreateIdentity());
            }
        }

        //////////////////////////////////////////////////////////////////////////
        void ActorComponent::DestroyActor()
        {
            if (m_actorInstance)
            {
                DetachFromEntity();

                ActorComponentNotificationBus::Event(
                    GetEntityId(),
                    &ActorComponentNotificationBus::Events::OnActorInstanceDestroyed,
                    m_actorInstance.get());

                m_actorInstance.reset();
            }
        }

        //////////////////////////////////////////////////////////////////////////
        void ActorComponent::OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world)
        {
            (void)local;

            const AZ::EntityId* busIdPtr = AZ::TransformNotificationBus::GetCurrentBusId();
            if (!busIdPtr || *busIdPtr == GetEntityId()) // Our own entity has moved.
            {
                // If we're not attached to another actor, keep the EMFX root in sync with any external changes to the entity's transform.
                if (m_actorInstance)
                {
                    const Transform localTransform = m_actorInstance->GetParentWorldSpaceTransform().Inversed() * MCore::AzTransformToEmfxTransform(world);
                    m_actorInstance->SetLocalSpacePosition(localTransform.mPosition);
                    m_actorInstance->SetLocalSpaceRotation(localTransform.mRotation);

                    // Disable updating the scale to prevent feedback from adding up.
                    // We need to find a better way to handle this or to prevent this feedback loop.
                    // m_actorInstance->SetLocalScale(localTransform.mScale);
                }
            }
        }

        //////////////////////////////////////////////////////////////////////////
        void ActorComponent::OnTick(float deltaTime, AZ::ScriptTimePoint time)
        {
            if (!m_actorInstance || !m_actorInstance->GetIsEnabled())
            {
                return;
            }

            if (m_renderNode)
            {
                m_renderNode->UpdateWorldBoundingBox();
            }

            if (m_configuration.m_renderSkeleton)
            {
                DrawSkeleton(m_actorInstance);
            }

            if (m_configuration.m_renderBounds)
            {
                gEnv->pRenderer->GetIRenderAuxGeom()->DrawAABB(m_renderNode->GetBBox(), false, Col_Cyan, eBBD_Faceted);
            }

            if (m_debugDrawRoot)
            {
                const AZ::Transform& worldTransform = GetEntity()->GetTransform()->GetWorldTM();
                gEnv->pRenderer->GetIRenderAuxGeom()->DrawCone(AZVec3ToLYVec3(worldTransform.GetTranslation() + AZ::Vector3(0.0f, 0.0f, 0.1f)), AZVec3ToLYVec3(worldTransform.GetColumn(1)), 0.05f, 0.5f, Col_Green);
            }

            if (!m_materialReadyEventSent && IsMaterialOwnerReady())
            {
                LmbrCentral::MaterialOwnerNotificationBus::Event(GetEntityId(), &LmbrCentral::MaterialOwnerNotifications::OnMaterialOwnerReady);
                m_materialReadyEventSent = true;
            }
        }

        int ActorComponent::GetTickOrder()
        {
            return AZ::TICK_PRE_RENDER;
        }

        void ActorComponent::OnPostPhysicsUpdate(float fixedDeltaTime, Physics::World* physicsWorld)
        {
            if (m_actorInstance &&
                m_actorInstance->GetRagdollInstance() &&
                m_actorInstance->GetRagdollInstance()->GetRagdollWorld() == physicsWorld)
            {
                m_actorInstance->PostPhysicsUpdate(fixedDeltaTime);
            }
        }

        //////////////////////////////////////////////////////////////////////////
        void ActorComponent::OnActorInstanceCreated(EMotionFX::ActorInstance* actorInstance)
        {
            auto it = AZStd::find(m_attachments.begin(), m_attachments.end(), actorInstance->GetEntityId());
            if (it != m_attachments.end())
            {
                if (m_actorInstance)
                {
                    LmbrCentral::AttachmentComponentRequestBus::Event(actorInstance->GetEntityId(), &LmbrCentral::AttachmentComponentRequestBus::Events::Reattach, true);
                }
            }
            else
            {
                m_attachmentTargetActor.reset(actorInstance);

                CheckAttachToEntity();
            }
        }

        //////////////////////////////////////////////////////////////////////////
        void ActorComponent::OnActorInstanceDestroyed(EMotionFX::ActorInstance* actorInstance)
        {
            DetachFromEntity();

            m_attachmentTargetActor = nullptr;
        }

        //////////////////////////////////////////////////////////////////////////
        IRenderNode* ActorComponent::GetRenderNode()
        {
            return m_renderNode.get();
        }

        //////////////////////////////////////////////////////////////////////////
        const float ActorComponent::s_renderNodeRequestBusOrder = 100.f;
        float ActorComponent::GetRenderNodeRequestBusOrder() const
        {
            return s_renderNodeRequestBusOrder;
        }

        //////////////////////////////////////////////////////////////////////////
        bool ActorComponent::IsMaterialOwnerReady()
        {
            return m_renderNode && m_renderNode->IsReady();
        }

        //////////////////////////////////////////////////////////////////////////
        void ActorComponent::SetMaterial(_smart_ptr<IMaterial> material)
        {
            if (m_renderNode)
            {
                if (material && material->IsSubMaterial())
                {
                    AZ_Error("MaterialOwnerRequestBus", false, "Material Owner cannot be given a Sub-Material.");
                }
                else
                {
                    m_renderNode->SetMaterial(material);
                }
            }
        }

        //////////////////////////////////////////////////////////////////////////
        _smart_ptr<IMaterial> ActorComponent::GetMaterial()
        {
            _smart_ptr<IMaterial> material = nullptr;

            if (m_renderNode)
            {
                material = m_renderNode->GetMaterial();

                if (!m_renderNode->IsReady())
                {
                    if (material)
                    {
                        AZ_Warning("MaterialOwnerRequestBus", false, "A Material was found, but Material Owner is not ready. May have unexpected results. (Try using MaterialOwnerNotificationBus.OnMaterialOwnerReady or MaterialOwnerRequestBus.IsMaterialOwnerReady)");
                    }
                    else
                    {
                        AZ_Error("MaterialOwnerRequestBus", false, "Material Owner is not ready and no Material was found. Assets probably have not finished loading yet. (Try using MaterialOwnerNotificationBus.OnMaterialOwnerReady or MaterialOwnerRequestBus.IsMaterialOwnerReady)");
                    }
                }
            }

            return material;
        }


        //////////////////////////////////////////////////////////////////////////
        bool ActorComponent::GetRagdollConfiguration(Physics::RagdollConfiguration& ragdollConfiguration) const
        {
            if (!m_actorInstance)
            {
                return false;
            }

            const AZStd::shared_ptr<PhysicsSetup>& physicsSetup = m_actorInstance->GetActor()->GetPhysicsSetup();
            ragdollConfiguration = physicsSetup->GetRagdollConfig();
            return true;
        }

        //////////////////////////////////////////////////////////////////////////
        AZStd::string ActorComponent::GetParentNodeName(const AZStd::string& childName) const
        {
            if (!m_actorInstance)
            {
                return AZStd::string();
            }

            const Skeleton* skeleton = m_actorInstance->GetActor()->GetSkeleton();
            Node* childNode = skeleton->FindNodeByName(childName);
            if (childNode)
            {
                const Node* parentNode = childNode->GetParentNode();
                if (parentNode)
                {
                    return parentNode->GetNameString();
                }
            }

            return AZStd::string();
        }

        //////////////////////////////////////////////////////////////////////////
        Physics::RagdollState ActorComponent::GetBindPose(const Physics::RagdollConfiguration& config) const
        {
            Physics::RagdollState physicsPose;

            if (!m_actorInstance)
            {
                return physicsPose;
            }

            const Actor* actor = m_actorInstance->GetActor();
            const Skeleton* skeleton = actor->GetSkeleton();
            const Pose* emfxPose = actor->GetBindPose();

            size_t numNodes = config.m_nodes.size();
            physicsPose.resize(numNodes);

            for (size_t nodeIndex = 0; nodeIndex < numNodes; nodeIndex++)
            {
                const char* nodeName = config.m_nodes[nodeIndex].m_debugName.data();
                Node* emfxNode = skeleton->FindNodeByName(nodeName);
                AZ_Error("EMotionFX", emfxNode, "Could not find bind pose for node %s", nodeName);
                if (emfxNode)
                {
                    const Transform& nodeTransform = emfxPose->GetModelSpaceTransform(emfxNode->GetNodeIndex());
                    physicsPose[nodeIndex].m_position = nodeTransform.mPosition;
                    physicsPose[nodeIndex].m_orientation = EmfxQuatToAzQuat(nodeTransform.mRotation);
                }
            }

            return physicsPose;
        }

        void ActorComponent::OnRagdollActivated()
        {
            AZStd::shared_ptr<Physics::Ragdoll> ragdoll;
            AzFramework::RagdollPhysicsRequestBus::EventResult(ragdoll, m_entity->GetId(), &AzFramework::RagdollPhysicsRequestBus::Events::GetRagdoll);
            if (ragdoll && m_actorInstance)
            {
                m_actorInstance->SetRagdoll(ragdoll);
            }
        }

        void ActorComponent::OnRagdollDeactivated()
        {
            if (m_actorInstance)
            {
                m_actorInstance->SetRagdoll(nullptr);
            }
        }

        //////////////////////////////////////////////////////////////////////////
        AZ::Aabb ActorComponent::GetWorldBounds()
        {
            if (m_actorInstance)
            {
                // The bounding box is moving with the actor instance. It is static in the way that it does not change shape.
                // The entity and actor transforms are kept in sync already.
                return AZ::Aabb::CreateFromMinMax(m_actorInstance->GetAABB().GetMin(), m_actorInstance->GetAABB().GetMax());
            }

            return AZ::Aabb::CreateNull();
        }

        //////////////////////////////////////////////////////////////////////////
        AZ::Aabb ActorComponent::GetLocalBounds()
        {
            if (m_actorInstance)
            {
                return AZ::Aabb::CreateFromMinMax(m_actorInstance->GetStaticBasedAABB().GetMin(), m_actorInstance->GetStaticBasedAABB().GetMax());
            }

            return AZ::Aabb::CreateNull();
        }

        //////////////////////////////////////////////////////////////////////////
        bool ActorComponent::GetVisibility()
        {
            return (m_renderNode ? !m_renderNode->IsHidden() : false);
        }

        //////////////////////////////////////////////////////////////////////////
        void ActorComponent::SetVisibility(bool isVisible)
        {
            if (m_renderNode)
            {
                m_renderNode->Hide(!isVisible);
            }
        }

        //////////////////////////////////////////////////////////////////////////
        void ActorComponent::DrawBounds(const EMotionFXPtr<EMotionFX::ActorInstance>& actorInstance)
        {
            if (!actorInstance)
            {
                return;
            }

            const MCore::AABB& actorBox = actorInstance->GetAABB();
            const AABB box(Vec3(actorBox.GetMin()), Vec3(actorBox.GetMax()));
            gEnv->pRenderer->GetIRenderAuxGeom()->DrawAABB(box, false, Col_Cyan, eBBD_Faceted);
        }

        //////////////////////////////////////////////////////////////////////////
        void ActorComponent::DrawSkeleton(const EMotionFXPtr<EMotionFX::ActorInstance>& actorInstance)
        {
            if (!actorInstance)
            {
                return;
            }

            // Draw the joints.
            const EMotionFX::TransformData* transformData = actorInstance->GetTransformData();
            const EMotionFX::Skeleton* skeleton = actorInstance->GetActor()->GetSkeleton();
            const EMotionFX::Pose* pose = transformData->GetCurrentPose();

            const AZ::u32 transformCount = transformData->GetNumTransforms();
            const AZ::u32 lodLevel = actorInstance->GetLODLevel();

            for (AZ::u32 index = 0; index < skeleton->GetNumNodes(); ++index)
            {
                const EMotionFX::Node* node = skeleton->GetNode(index);
                const AZ::u32 parentIndex = node->GetParentIndex();
                if (parentIndex == MCORE_INVALIDINDEX32)
                {
                    continue;
                }

                if (!node->GetSkeletalLODStatus(lodLevel))
                {
                    continue;
                }

                const AZ::Vector3 bonePos = pose->GetWorldSpaceTransform(index).mPosition;
                const AZ::Vector3 parentPos = pose->GetWorldSpaceTransform(parentIndex).mPosition;
                gEnv->pRenderer->GetIRenderAuxGeom()->DrawBone(AZVec3ToLYVec3(parentPos), AZVec3ToLYVec3(bonePos), Col_YellowGreen);
            }
        }


        size_t ActorComponent::GetJointIndexByName(const char* name) const
        {
            AZ_Assert(m_actorInstance, "The actor instance needs to be valid.");

            EMotionFX::Node* node = m_actorInstance->GetActor()->GetSkeleton()->FindNodeByNameNoCase(name);
            if (node)
            {
                return static_cast<size_t>(node->GetNodeIndex());
            }

            return ActorComponentRequests::s_invalidJointIndex;
        }


        AZ::Transform ActorComponent::GetJointTransform(size_t jointIndex, Space space) const
        {
            AZ_Assert(m_actorInstance, "The actor instance needs to be valid.");

            const AZ::u32 index = static_cast<AZ::u32>(jointIndex);
            const AZ::u32 numNodes = m_actorInstance->GetActor()->GetNumNodes();

            AZ_Error("EMotionFX", index < numNodes, "GetJointTransform: The joint index %d is out of bounds [0;%d]. Entity: %s",
                index, numNodes, GetEntity()->GetName());

            if (index >= numNodes)
            {
                return AZ::Transform::CreateIdentity();
            }

            EMotionFX::Pose* currentPose = m_actorInstance->GetTransformData()->GetCurrentPose();
            switch (space)
            {
            case Space::LocalSpace:
            {
                return MCore::EmfxTransformToAzTransform(currentPose->GetLocalSpaceTransform(index));
            }

            case Space::ModelSpace:
            {
                return MCore::EmfxTransformToAzTransform(currentPose->GetModelSpaceTransform(index));
            }

            case Space::WorldSpace:
            {
                return MCore::EmfxTransformToAzTransform(currentPose->GetWorldSpaceTransform(index));
            }

            default:
                AZ_Assert(false, "Unsupported space in GetJointTransform!");
            }

            return AZ::Transform::CreateIdentity();
        }

        void ActorComponent::GetJointTransformComponents(size_t jointIndex, Space space, AZ::Vector3& outPosition, AZ::Quaternion& outRotation, AZ::Vector3& outScale) const
        {
            AZ_Assert(m_actorInstance, "The actor instance needs to be valid.");

            const AZ::u32 index = static_cast<AZ::u32>(jointIndex);
            const AZ::u32 numNodes = m_actorInstance->GetActor()->GetNumNodes();

            AZ_Error("EMotionFX", index < numNodes, "GetJointTransformComponents: The joint index %d is out of bounds [0;%d]. Entity: %s",
                index, numNodes, GetEntity()->GetName());

            if (index >= numNodes)
            {
                return;
            }

            EMotionFX::Pose* currentPose = m_actorInstance->GetTransformData()->GetCurrentPose();

            switch (space)
            {
            case Space::LocalSpace:
            {
                const Transform& localTransform = currentPose->GetLocalSpaceTransform(index);
                outPosition = localTransform.mPosition;
                outRotation = MCore::EmfxQuatToAzQuat(localTransform.mRotation);
                outScale = localTransform.mScale;
                return;
            }

            case Space::ModelSpace:
            {
                const Transform& modelTransform = currentPose->GetModelSpaceTransform(index);
                outPosition = modelTransform.mPosition;
                outRotation = MCore::EmfxQuatToAzQuat(modelTransform.mRotation);
                outScale = modelTransform.mScale;
                return;
            }

            case Space::WorldSpace:
            {
                const Transform worldTransform = currentPose->GetWorldSpaceTransform(index);
                outPosition = worldTransform.mPosition;
                outRotation = MCore::EmfxQuatToAzQuat(worldTransform.mRotation);
                outScale = worldTransform.mScale;
                return;
            }

            default:
            {
                AZ_Assert(false, "Unsupported space in GetJointTransform!");
                outPosition = AZ::Vector3::CreateZero();
                outRotation = AZ::Quaternion::CreateIdentity();
                outScale = AZ::Vector3::CreateOne();
            }
            }
        }

        Physics::AnimationConfiguration* ActorComponent::GetPhysicsConfig() const
        {
            if (m_actorInstance)
            {
                Actor* actor = m_actorInstance->GetActor();
                const AZStd::shared_ptr<PhysicsSetup>& physicsSetup = actor->GetPhysicsSetup();
                if (physicsSetup)
                {
                    return &physicsSetup->GetConfig();
                }
            }

            return nullptr;
        }

        // The entity has attached to the target.
        void ActorComponent::OnAttached(AZ::EntityId attachedEntityId)
        {
            const AZ::EntityId* busIdPtr = LmbrCentral::AttachmentComponentNotificationBus::GetCurrentBusId();
            if (busIdPtr)
            {
                const auto result = AZStd::find(m_attachments.begin(), m_attachments.end(), attachedEntityId);
                if (result == m_attachments.end())
                {
                    m_attachments.emplace_back(attachedEntityId);
                }
            }

            if (!m_actorInstance)
            {
                return;
            }

            EMotionFX::ActorInstance* targetActorInstance = nullptr;
            ActorComponentRequestBus::EventResult(targetActorInstance, attachedEntityId, &ActorComponentRequestBus::Events::GetActorInstance);

            const char* jointName = nullptr;
            LmbrCentral::AttachmentComponentRequestBus::EventResult(jointName, attachedEntityId, &LmbrCentral::AttachmentComponentRequestBus::Events::GetJointName);
            if (targetActorInstance)
            {
                EMotionFX::Node* node = jointName ? m_actorInstance->GetActor()->GetSkeleton()->FindNodeByName(jointName) : m_actorInstance->GetActor()->GetSkeleton()->GetNode(0);
                if (node)
                {
                    const AZ::u32 jointIndex = node->GetNodeIndex();
                    EMotionFX::Attachment* attachment = EMotionFX::AttachmentNode::Create(m_actorInstance.get(), jointIndex, targetActorInstance, true /* Managed externally, by this component. */);
                    m_actorInstance->AddAttachment(attachment);
                }
            }
        }


        // The entity is detaching from the target.
        void ActorComponent::OnDetached(AZ::EntityId targetId)
        {
            // Remove the targetId from the attachment list
            const AZ::EntityId* busIdPtr = LmbrCentral::AttachmentComponentNotificationBus::GetCurrentBusId();
            if (busIdPtr)
            {
                m_attachments.erase(AZStd::remove(m_attachments.begin(), m_attachments.end(), targetId), m_attachments.end());
            }

            if (!m_actorInstance)
            {
                return;
            }

            EMotionFX::ActorInstance* targetActorInstance = nullptr;
            ActorComponentRequestBus::EventResult(targetActorInstance, targetId, &ActorComponentRequestBus::Events::GetActorInstance);
            if (targetActorInstance)
            {
                m_actorInstance->RemoveAttachment(targetActorInstance);
            }
        }
    } // namespace Integration
} // namespace EMotionFX
