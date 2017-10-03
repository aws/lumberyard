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


#include "StdAfx.h"

#include <AzCore/Component/Entity.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Math/Transform.h>

#include <LmbrCentral/Physics/CryCharacterPhysicsBus.h>

#include <Integration/Components/ActorComponent.h>

#include <EMotionFX/Source/Transform.h>

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
                    ->Field("ActorAsset", &Configuration::m_actorAsset)
                    ->Field("MaterialPerLOD", &Configuration::m_materialPerLOD)
                    ->Field("RenderSkeleton", &Configuration::m_renderSkeleton)
                    ->Field("AttachmentType", &Configuration::m_attachmentType)
                    ->Field("AttachmentTarget", &Configuration::m_attachmentTarget)
                    ->Field("AttachmentJointIndex", &Configuration::m_attachmentJointIndex)
                    ->Field("SkinningMethod", &Configuration::m_skinningMethod)
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
            }

            auto* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
            if (behaviorContext)
            {
                behaviorContext->EBus<ActorComponentRequestBus>("ActorComponentRequestBus")
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::Preview)
                    ->Event("AttachToEntity", &ActorComponentRequestBus::Events::AttachToEntity)
                    ->Event("DetachFromEntity", &ActorComponentRequestBus::Events::DetachFromEntity)
                    ->Event("DebugDrawRoot", &ActorComponentRequestBus::Events::DebugDrawRoot)
                    ;

                behaviorContext->EBus<ActorComponentNotificationBus>("ActorComponentNotificationBus")
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::Preview)
                    ->Event("OnActorInstanceCreated", &ActorComponentNotificationBus::Events::OnActorInstanceCreated)
                    ->Event("OnActorInstanceDestroyed", &ActorComponentNotificationBus::Events::OnActorInstanceDestroyed)
                    ;
            }
        }

        //////////////////////////////////////////////////////////////////////////
        ActorComponent::Configuration::Configuration()
            : m_attachmentJointIndex(0)
            , m_attachmentType(AttachmentType::None)
            , m_renderSkeleton(false)
            , m_skinningMethod(SkinningMethod::DualQuat)
        {
        }

        //////////////////////////////////////////////////////////////////////////
        ActorComponent::ActorComponent(const Configuration* configuration)
            : m_debugDrawRoot(false)
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
            ActorComponentRequestBus::Handler::BusConnect(GetEntityId());

            if (cfg.m_attachmentTarget.IsValid())
            {
                AttachToEntity(cfg.m_attachmentTarget, cfg.m_attachmentType);
            }
        }

        //////////////////////////////////////////////////////////////////////////
        void ActorComponent::Deactivate()
        {
            ActorComponentRequestBus::Handler::BusDisconnect();
            AZ::TickBus::Handler::BusDisconnect();
            ActorComponentNotificationBus::Handler::BusDisconnect();
            LmbrCentral::MeshComponentRequestBus::Handler::BusDisconnect();
            LmbrCentral::RenderNodeRequestBus::Handler::BusDisconnect();
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

                m_actorInstance = actorAsset->CreateInstance();
                if (!m_actorInstance)
                {
                    AZ_Error("EMotionFX", actorAsset, "Failed to create actor instance.");
                    return;
                }

                m_actorInstance->SetCustomData(reinterpret_cast<void*>(static_cast<AZ::u64>(GetEntityId())));

                ActorComponentNotificationBus::Event(
                    GetEntityId(),
                    &ActorComponentNotificationBus::Events::OnActorInstanceCreated,
                    m_actorInstance.get());

                // Setup initial transform and listen for transform changes.
                AZ::Transform transform;
                AZ::TransformBus::EventResult(transform, GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);
                OnTransformChanged(transform, transform);
                AZ::TransformNotificationBus::MultiHandler::BusConnect(GetEntityId());

                m_renderNode = AZStd::make_unique<ActorRenderNode>(GetEntityId(), m_actorInstance, m_configuration.m_actorAsset, transform);
                m_renderNode->SetMaterials(m_configuration.m_materialPerLOD);
                m_renderNode->RegisterWithRenderer(true);
                m_renderNode->SetSkinningMethod(m_configuration.m_skinningMethod);

                CheckAttachToEntity();
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

                EMotionFX::Attachment* attachment = nullptr;

                switch (m_configuration.m_attachmentType)
                {
                case AttachmentType::ActorAttachment:
                {
                    attachment = EMotionFX::AttachmentNode::Create(
                        m_attachmentTargetActor.get(),
                        m_configuration.m_attachmentJointIndex,
                        m_actorInstance.get());
                }
                break;
                case AttachmentType::SkinAttachment:
                {
                    attachment = EMotionFX::AttachmentSkin::Create(
                        m_attachmentTargetActor.get(),
                        m_actorInstance.get());
                }
                break;
                }

                if (attachment)
                {
                    m_actorInstance->SetLocalTransform(EMotionFX::Transform());
                    m_attachmentTargetActor->AddAttachment(attachment);
                }
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
                if (m_actorInstance && !m_attachmentTargetActor)
                {
                    const AZ::Quaternion entityOrientation = AZ::Quaternion::CreateFromTransform(world);
                    const AZ::Vector3 entityPosition = world.GetTranslation();

                    AZ::Transform transformNoScale = AZ::Transform::CreateFromQuaternionAndTranslation(entityOrientation, entityPosition);
                    m_actorInstance->SetLocalTransform(MCore::AzTransformToEmfxTransform(transformNoScale));
                }
            }
            else if (busIdPtr && *busIdPtr == m_attachmentTargetEntityId) // The entity owning the actor we're attached to has moved.
            {
                // We're attached to another actor. Keep our transform in sync with the master actor's entity.
                // Note that our transform does not reflect the precise joint transform at this time.
                GetEntity()->GetTransform()->SetWorldTM(world);
            }
        }

        //////////////////////////////////////////////////////////////////////////
        void ActorComponent::OnTick(float deltaTime, AZ::ScriptTimePoint time)
        {
            if (m_actorInstance)
            {
                if (m_actorInstance->GetActor()->GetMotionExtractionNode())
                {
                    const float deltaTimeInv = (deltaTime > 0.0f) ? (1.0f / deltaTime) : 0.0f;

                    //get the current entity location and calculated motion extracted location
                    AZ::Transform currentTransform = GetEntity()->GetTransform()->GetWorldTM();
                    const AZ::Vector3 actorInstancePosition = MCore::EmfxVec3ToAzVec3(m_actorInstance->GetGlobalPosition());

                    //calculate the delta of the positions and apply that as velocity
                    const AZ::Vector3 currentPos = currentTransform.GetPosition();
                    const AZ::Vector3 positionDelta = actorInstancePosition - currentPos;
                    const AZ::Vector3 scale = currentTransform.ExtractScaleExact();
                    const AZ::Vector3 velocity = positionDelta * scale * deltaTimeInv;
                    EBUS_EVENT_ID(GetEntityId(), LmbrCentral::CryCharacterPhysicsRequestBus, RequestVelocity, velocity, 0);

                    //calculate the difference in rotation and apply that to the entity transform
                    const AZ::Quaternion actorInstanceRotation = MCore::EmfxQuatToAzQuat(m_actorInstance->GetGlobalRotation());
                    AZ::Quaternion rotationDelta = AZ::Quaternion::CreateFromTransform(currentTransform).GetInverseFull() * actorInstanceRotation;
                    if (!rotationDelta.IsIdentity(AZ::g_fltEps))
                    {
                        currentTransform = currentTransform * AZ::Transform::CreateFromQuaternion(rotationDelta);
                        currentTransform.Orthogonalize();
                        GetEntity()->GetTransform()->SetWorldTM(currentTransform);
                    }
                }

                if (m_configuration.m_renderSkeleton)
                {
                    DrawSkeleton(m_actorInstance);
                }

                if (m_debugDrawRoot)
                {
                    const AZ::Transform& worldTransform = GetEntity()->GetTransform()->GetWorldTM();
                    gEnv->pRenderer->GetIRenderAuxGeom()->DrawCone(AZVec3ToLYVec3(worldTransform.GetTranslation() + AZ::Vector3(0.0f, 0.0f, 0.1f)), AZVec3ToLYVec3(worldTransform.GetColumn(1)), 0.05f, 0.5f, Col_Green);
                }
            }
        }


        //////////////////////////////////////////////////////////////////////////
        void ActorComponent::OnActorInstanceCreated(EMotionFX::ActorInstance* actorInstance)
        {
            AZ_Assert(actorInstance != m_actorInstance.get(), "Attempting to attach to self");

            m_attachmentTargetActor.reset(actorInstance);

            CheckAttachToEntity();
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
        AZ::Aabb ActorComponent::GetWorldBounds()
        {
            if (m_actorInstance)
            {
                AZ::Aabb aabb = AZ::Aabb::CreateFromMinMax(
                    EmfxVec3ToAzVec3(m_actorInstance->GetStaticBasedAABB().GetMin()),
                    EmfxVec3ToAzVec3(m_actorInstance->GetStaticBasedAABB().GetMax()));

                if (GetEntity() && GetEntity()->GetTransform())
                {
                    aabb.ApplyTransform(GetEntity()->GetTransform()->GetWorldTM());
                }

                return aabb;
            }

            return AZ::Aabb::CreateNull();
        }

        //////////////////////////////////////////////////////////////////////////
        AZ::Aabb ActorComponent::GetLocalBounds()
        {
            if (m_actorInstance)
            {
                const AZ::Aabb aabb = AZ::Aabb::CreateFromMinMax(
                    EmfxVec3ToAzVec3(m_actorInstance->GetStaticBasedAABB().GetMin()),
                    EmfxVec3ToAzVec3(m_actorInstance->GetStaticBasedAABB().GetMax()));

                return aabb;
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
        void ActorComponent::DrawSkeleton(const EMotionFXPtr<EMotionFX::ActorInstance>& actorInstance)
        {
            if (actorInstance)
            {
                const EMotionFX::TransformData* transformData = actorInstance->GetTransformData();
                const EMotionFX::Skeleton* skeleton = actorInstance->GetActor()->GetSkeleton();

                const AZ::u32 transformCount = transformData->GetNumTransforms();

                for (AZ::u32 index = 0; index < skeleton->GetNumNodes(); ++index)
                {
                    const EMotionFX::Node* node = skeleton->GetNode(index);
                    const AZ::u32 parentIndex = node->GetParentIndex();
                    if (parentIndex == MCORE_INVALIDINDEX32)
                    {
                        continue;
                    }

                    const AZ::Vector3 bonePos = EmfxVec3ToAzVec3(transformData->GetGlobalPosition(index));
                    const AZ::Vector3 parentPos = EmfxVec3ToAzVec3(transformData->GetGlobalPosition(parentIndex));

                    gEnv->pRenderer->GetIRenderAuxGeom()->DrawBone(AZVec3ToLYVec3(parentPos), AZVec3ToLYVec3(bonePos), Col_YellowGreen);
                }
            }
        }
    } // namespace Integration
} // namespace EMotionFX

