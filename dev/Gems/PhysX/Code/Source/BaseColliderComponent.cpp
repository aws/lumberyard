
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

#include <PhysX_precompiled.h>

#include <AzFramework/Physics/PhysicsComponentBus.h>
#include <AzFramework/Physics/CharacterBus.h>
#include <AzFramework/Physics/RigidBodyBus.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzFramework/Physics/Utils.h>

#include <Source/BaseColliderComponent.h>
#include <Source/RigidBodyComponent.h>
#include <Source/RigidBodyStatic.h>
#include <Source/SystemComponent.h>
#include <Source/Utils.h>

namespace PhysX
{
    AZ::Aabb BaseColliderComponent::ShapeInfoCache::GetAabb(PhysX::Shape& shape)
    { 
        if (m_cacheOutdated)
        {
            UpdateCache(shape);
        }
        return m_aabb; 
    }

    void BaseColliderComponent::ShapeInfoCache::InvalidateCache()
    { 
        m_cacheOutdated = true; 
    }

    const AZ::Transform& BaseColliderComponent::ShapeInfoCache::GetWorldTransform()
    {
        return m_worldTransform;
    }

    void BaseColliderComponent::ShapeInfoCache::SetWorldTransform(const AZ::Transform& worldTransform)
    {
        m_worldTransform = worldTransform;
    }

    void BaseColliderComponent::ShapeInfoCache::UpdateCache(PhysX::Shape& shape)
    {
        physx::PxShape* pxShape = shape.GetPxShape();
        physx::PxTransform pxTransform = pxShape->getLocalPose();
        physx::PxGeometryHolder pxGeomHolder = pxShape->getGeometry();
        physx::PxGeometryType::Enum pxGeomType = pxGeomHolder.getType();

        physx::PxBounds3 bounds = physx::PxGeometryQuery::getWorldBounds(pxShape->getGeometry().any()
            , physx::PxTransform(PxMathConvert(m_worldTransform)) * pxShape->getLocalPose());
        m_aabb = AZ::Aabb::CreateFromMinMax(AZ::Vector3(PxMathConvert(bounds.minimum))
            , AZ::Vector3(PxMathConvert(bounds.maximum)));
        m_cacheOutdated = false;
    }

    void BaseColliderComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<BaseColliderComponent, AZ::Component>()
                ->Version(1)
                ->Field("Configuration", &BaseColliderComponent::m_configuration)
                ;
        }
    }

    BaseColliderComponent::BaseColliderComponent(const Physics::ColliderConfiguration& colliderConfiguration)
        : m_configuration(colliderConfiguration)
    {

    }

    void BaseColliderComponent::Activate()
    {
        const AZ::EntityId entityId = GetEntityId();

        ColliderComponentRequestBus::Handler::BusConnect(entityId);
        AZ::EntityBus::Handler::BusConnect(entityId);
        AZ::TransformNotificationBus::Handler::BusConnect(entityId);
        ColliderShapeRequestBus::Handler::BusConnect(GetEntityId());

        AZ::Transform worldTransform = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(worldTransform, entityId, &AZ::TransformBus::Events::GetWorldTM);
        m_shapeInfoCache.SetWorldTransform(worldTransform);

        InitShape();
    }

    void BaseColliderComponent::Deactivate()
    {
        Physics::Utils::DeferDelete(AZStd::move(m_staticRigidBody));
        ColliderShapeRequestBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::Handler::BusDisconnect();
        AZ::EntityBus::Handler::BusDisconnect();
        ColliderComponentRequestBus::Handler::BusDisconnect();
    }

    void BaseColliderComponent::OnEntityActivated(const AZ::EntityId& parentEntityId)
    {
        InitStaticRigidBody();
    }

    AZStd::shared_ptr<Physics::ShapeConfiguration> BaseColliderComponent::GetShapeConfigFromEntity()
    {
        return CreateScaledShapeConfig();
    }

    const Physics::ColliderConfiguration& BaseColliderComponent::GetColliderConfig()
    {
        return m_configuration;
    }

    bool BaseColliderComponent::IsStaticRigidBody()
    {
        return m_staticRigidBody != nullptr;
    }

    PhysX::RigidBodyStatic* BaseColliderComponent::GetStaticRigidBody()
    {
        return m_staticRigidBody.get();
    }

    bool BaseColliderComponent::IsTrigger()
    {
        return m_shape->IsTrigger();
    }

    void BaseColliderComponent::InitStaticRigidBody()
    {
        bool hasRigidBodyComponent = GetEntity()->FindComponent<PhysX::RigidBodyComponent>() != nullptr;
        bool hasCharacterComponent = Physics::CharacterRequestBus::FindFirstHandler(GetEntityId()) != nullptr;
        
        if (!hasRigidBodyComponent && !hasCharacterComponent)
        {
            bool noOtherStaticBodies = true;

            ColliderComponentRequestBus::EnumerateHandlersId(GetEntityId(), [&noOtherStaticBodies](ColliderComponentRequests* handler)
            {
                noOtherStaticBodies = noOtherStaticBodies && !handler->IsStaticRigidBody();
                // if there are other bodies no need to continue iterating
                return noOtherStaticBodies;
            });

            if (noOtherStaticBodies)
            {
                AZ::Transform lyTransform = AZ::Transform::CreateIdentity();
                AZ::TransformBus::EventResult(lyTransform, GetEntityId(), &AZ::TransformInterface::GetWorldTM);
                lyTransform.ExtractScale();

                Physics::WorldBodyConfiguration configuration;
                configuration.m_orientation = AZ::Quaternion::CreateFromTransform(lyTransform);
                configuration.m_position = lyTransform.GetPosition();
                configuration.m_entityId = GetEntityId();
                configuration.m_debugName = GetEntity()->GetName();

                m_staticRigidBody = AZStd::make_unique<PhysX::RigidBodyStatic>(configuration);

                ColliderComponentRequestBus::EnumerateHandlersId(GetEntityId(), [this](ColliderComponentRequests* handler)
                {
                    m_staticRigidBody->AddShape(handler->GetShape());
                    return true;
                });

                auto world = Utils::GetDefaultWorld();
                world->AddBody(*m_staticRigidBody);
            }
        }
    }

    void BaseColliderComponent::InitShape()
    {
        auto shapeConfiguration = GetShapeConfigFromEntity();
        
        auto colliderConfiguration = m_configuration;
        colliderConfiguration.m_position *= GetNonUniformScale();
        
        if (!shapeConfiguration)
        {
            AZ_Error("PhysX", false, "Unable to create a physics shape because shape configuration is null. Entity: %s", 
                GetEntity()->GetName().c_str());
            return;
        }

        auto shape = AZStd::make_shared<Shape>(colliderConfiguration, *shapeConfiguration);

        if (!shape->GetPxShape())
        {
            AZ_Error("PhysX", false, "Failed to create a PhysX shape. Entity: %s", GetEntity()->GetName().c_str());
            return;
        }

        m_shape = shape;
    }

    float BaseColliderComponent::GetUniformScale()
    {
        return GetNonUniformScale().GetMaxElement();
    }

    AZ::Vector3 BaseColliderComponent::GetNonUniformScale()
    {
        AZ::Transform transform;
        AZ::TransformBus::EventResult(transform, GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);
        return transform.ExtractScale();
    }

    AZStd::shared_ptr<Physics::ShapeConfiguration> BaseColliderComponent::CreateScaledShapeConfig()
    {
        // Overridden by each collider component
        return nullptr;
    }

    AZStd::shared_ptr<Physics::Shape> BaseColliderComponent::GetShape()
    {
        return m_shape;
    }

    void* BaseColliderComponent::GetNativePointer()
    {
       return m_shape->GetPxShape();
    }

    void BaseColliderComponent::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& world)
    {
        m_shapeInfoCache.SetWorldTransform(world);
        if (m_staticRigidBody)
        {
            m_staticRigidBody->SetTransform(world);
        }
        m_shapeInfoCache.InvalidateCache();
    }

    AZ::Aabb BaseColliderComponent::GetColliderShapeAabb()
    {
        if (!m_shape)
        {
            return AZ::Aabb::CreateFromPoint(m_shapeInfoCache.GetWorldTransform().GetPosition());
        }
        return m_shapeInfoCache.GetAabb(*m_shape);
    }
}
