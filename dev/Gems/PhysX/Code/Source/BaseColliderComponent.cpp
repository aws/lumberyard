
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
    // ShapeInfoCache
    AZ::Aabb BaseColliderComponent::ShapeInfoCache::GetAabb(const AZStd::vector<AZStd::shared_ptr<Physics::Shape>>& shapes)
    { 
        if (m_cacheOutdated)
        {
            UpdateCache(shapes);
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

    void BaseColliderComponent::ShapeInfoCache::UpdateCache(const AZStd::vector<AZStd::shared_ptr<Physics::Shape>>& shapes)
    {
        size_t numShapes = shapes.size();

        if (numShapes > 0)
        {
            auto pxShape = static_cast<physx::PxShape*>(shapes[0]->GetNativePointer());
            physx::PxTransform pxWorldTransform = PxMathConvert(m_worldTransform);
            physx::PxBounds3 bounds = physx::PxGeometryQuery::getWorldBounds(pxShape->getGeometry().any(),
                pxWorldTransform * pxShape->getLocalPose(), 1.0f);

            for (size_t shapeIndex = 1; shapeIndex < numShapes; ++shapeIndex)
            {
                pxShape = static_cast<physx::PxShape*>(shapes[0]->GetNativePointer());
                bounds.include(physx::PxGeometryQuery::getWorldBounds(pxShape->getGeometry().any(),
                    pxWorldTransform * pxShape->getLocalPose(), 1.0f));
            }

            m_aabb = PxMathConvert(bounds);
        }

        else
        {
            m_aabb = AZ::Aabb::CreateFromPoint(m_worldTransform.GetPosition());
        }

        m_cacheOutdated = false;
    }

    // BaseColliderComponent
    const Physics::ColliderConfiguration BaseColliderComponent::s_defaultColliderConfig = Physics::ColliderConfiguration();

    void BaseColliderComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<BaseColliderComponent, AZ::Component>()
                ->Version(1)
                ->Field("ShapeConfigList", &BaseColliderComponent::m_shapeConfigList)
                ;
        }
    }

    void BaseColliderComponent::SetShapeConfigurationList(const Physics::ShapeConfigurationList& shapeConfigList)
    {
        if (GetEntity()->GetState() == AZ::Entity::State::ES_ACTIVE)
        {
            AZ_Warning("PhysX", false, "Trying to call SetShapeConfigurationList for entity \"%s\" while entity is active.",
                GetEntity()->GetName().c_str());
            return;
        }
        m_shapeConfigList = shapeConfigList;
    }

    // ColliderComponentRequestBus
    AZStd::shared_ptr<Physics::ShapeConfiguration> BaseColliderComponent::GetShapeConfigFromEntity()
    {
        AZ_Warning("PhysX", false, "Colliders now support multiple shapes. GetShapeConfigFromEntity is deprecated, "
            "but will return the shape configuration of only the first shape for compatibility. "
            "Please use GetShapeConfigurations instead.");

        if (!m_shapeConfigList.empty())
        {
            return m_shapeConfigList[0].second;
        }

        AZ_Warning("PhysX", false, "Collider has no associated shape/collider configurations, returning nullptr "
            "for entity \"%s\".", GetEntity()->GetName().c_str());
        return nullptr;
    }

    const Physics::ColliderConfiguration& BaseColliderComponent::GetColliderConfig()
    {
        AZ_Warning("PhysX", false, "Colliders now support multiple shapes. GetColliderConfig is deprecated, but for "
            "compatibility will return the collider configuration of the first shape only. Please use "
            "GetShapeConfigurations instead.");

        if (!m_shapeConfigList.empty())
        {
            return *m_shapeConfigList[0].first;
        }

        AZ_Warning("PhysX", false, "Collider has no associated shape/collider configurations, returning default "
            "collider configuration for entity \"%s\".", GetEntity()->GetName().c_str());
        return s_defaultColliderConfig;
    }

    AZStd::shared_ptr<Physics::Shape> BaseColliderComponent::GetShape()
    {
        AZ_Warning("PhysX", false, "Colliders now support multiple shapes. GetShape is deprecated, but for "
            "compatibility will return the first shape only. Please use GetShapes instead.");

        if (!m_shapes.empty())
        {
            return m_shapes[0];
        }

        AZ_Warning("PhysX", false, "No shapes found for entity \"%s\".", GetEntity()->GetName().c_str());
        return nullptr;
    }

    void* BaseColliderComponent::GetNativePointer()
    {
        AZ_Warning("PhysX", false, "Colliders now support multiple shapes. GetNativePointer is deprecated, but for "
            "compatibility will return the native pointer for the first shape only. Please use GetShapes instead.");

        if (!m_shapes.empty())
        {
            return m_shapes[0]->GetNativePointer();
        }

        AZ_Warning("PhysX", false, "No shapes found for entity \"%s\".", GetEntity()->GetName().c_str());
        return nullptr;
    }

    Physics::ShapeConfigurationList BaseColliderComponent::GetShapeConfigurations()
    {
        return m_shapeConfigList;
    }

    AZStd::vector<AZStd::shared_ptr<Physics::Shape>> BaseColliderComponent::GetShapes()
    {
        return m_shapes;
    }

    bool BaseColliderComponent::IsStaticRigidBody()
    {
        return m_staticRigidBody != nullptr;
    }

    PhysX::RigidBodyStatic* BaseColliderComponent::GetStaticRigidBody()
    {
        return m_staticRigidBody.get();
    }

    // TransformNotificationsBus
    void BaseColliderComponent::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& world)
    {
        m_shapeInfoCache.SetWorldTransform(world);
        if (m_staticRigidBody)
        {
            m_staticRigidBody->SetTransform(world);
        }
        m_shapeInfoCache.InvalidateCache();
    }

    // PhysX::ColliderShapeBus
    AZ::Aabb BaseColliderComponent::GetColliderShapeAabb()
    {
        if (m_shapes.empty())
        {
            return AZ::Aabb::CreateFromPoint(m_shapeInfoCache.GetWorldTransform().GetPosition());
        }

        return m_shapeInfoCache.GetAabb(m_shapes);
    }

    bool BaseColliderComponent::IsTrigger()
    {
        AZ_Error("PhysX", !m_shapes.empty(), "Tried to call IsTrigger before any shapes were initialized for entity %s.",
            GetEntity()->GetName().c_str());

        // Colliders now support multiple shapes. This will return true if any of the shapes is a trigger.
        for (const auto& shapeConfigPair : m_shapeConfigList)
        {
            if (shapeConfigPair.first->m_isTrigger)
            {
                return true;
            }
        }

        return false;
    }

    // AZ::Component
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

        InitShapes();
    }

    void BaseColliderComponent::Deactivate()
    {
        Physics::Utils::DeferDelete(AZStd::move(m_staticRigidBody));
        ColliderShapeRequestBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::Handler::BusDisconnect();
        AZ::EntityBus::Handler::BusDisconnect();
        ColliderComponentRequestBus::Handler::BusDisconnect();
    }

    // AZ::EntityBus
    void BaseColliderComponent::OnEntityActivated(const AZ::EntityId& parentEntityId)
    {
        InitStaticRigidBody();
    }

    // BaseColliderComponent
    AZ::Vector3 BaseColliderComponent::GetNonUniformScale() const
    {
        AZ::Vector3 localScale = AZ::Vector3::CreateOne();
        AZ::TransformBus::EventResult(localScale, GetEntityId(), &AZ::TransformBus::Events::GetLocalScale);
        return localScale;
    }

    float BaseColliderComponent::GetUniformScale() const
    {
        return GetNonUniformScale().GetMaxElement();
    }

    void BaseColliderComponent::UpdateScaleForShapeConfigs()
    {
        // Overridden by each collider component
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
                    const AZStd::vector<AZStd::shared_ptr<Physics::Shape>>& shapes = handler->GetShapes();
                    for (auto& shape : shapes)
                    {
                        m_staticRigidBody->AddShape(shape);
                    }

                    return true;
                });

                auto world = Utils::GetDefaultWorld();
                world->AddBody(*m_staticRigidBody);
            }
        }
    }

    void BaseColliderComponent::InitShapes()
    {
        UpdateScaleForShapeConfigs();
        m_shapes.reserve(m_shapeConfigList.size());

        for (const auto& shapeConfigPair : m_shapeConfigList)
        {
            const AZStd::shared_ptr<Physics::ShapeConfiguration>& shapeConfiguration = shapeConfigPair.second;
            Physics::ColliderConfiguration colliderConfiguration = *shapeConfigPair.first;

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

            m_shapes.push_back(shape);
        }
    }
}
