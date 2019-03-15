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
#include "LmbrCentral_precompiled.h"
#include "PhysicsSystemComponent.h"

#include <AzFramework/Physics/PhysicsComponentBus.h>
#include <LmbrCentral/Physics/CryPhysicsComponentRequestBus.h>
#include <LmbrCentral/Rendering/MeshComponentBus.h>
#include <LmbrCentral/Rendering/MaterialOwnerBus.h>
#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Script/ScriptContextAttributes.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/MathUtils.h>

#include <MathConversion.h>
#include <IEntity.h>
#include <IEntityRenderState.h>
#include <IMaterial.h>
#include <IPhysics.h>
#include <ISurfaceType.h>
#include <physinterface.h>


namespace LmbrCentral
{
    using AzFramework::PhysicsComponentNotificationBus;
    using AzFramework::PhysicsSystemRequestBus;
    using AzFramework::PhysicsSystemEventBus;
    using AzFramework::PhysicalEntityTypes;

    // Class for putting ent_* flags on
    class PhysicalEntityTypesHolder
    {
    public:
        AZ_TYPE_INFO(PhysicalEntityTypesHolder, "{0D6FF6AB-3C2B-4D44-A6CA-B3C41478EF94}");
        AZ_CLASS_ALLOCATOR(PhysicalEntityTypesHolder, AZ::SystemAllocator, 0);

        PhysicalEntityTypesHolder() = default;
        ~PhysicalEntityTypesHolder() = default;

        static PhysicalEntityTypes ToggleEntityTypeMask(PhysicalEntityTypes currentEntityType, PhysicalEntityTypes entityTypeToggleValue)
        {
            return static_cast<PhysicalEntityTypes>(currentEntityType ^ entityTypeToggleValue);
        }
    };

    AZ::u32 EntFromEntityTypes(AZ::u32 types)
    {
        // Shortcut when requesting all entities
        if (types == PhysicalEntityTypes::All)
        {
            return ent_all;
        }

        AZ::u32 result = 0;

        if (types & PhysicalEntityTypes::Static)
        {
            result |= ent_static;
        }
        if (types & PhysicalEntityTypes::Dynamic)
        {
            result |= ent_rigid | ent_sleeping_rigid;
        }
        if (types & PhysicalEntityTypes::Living)
        {
            result |= ent_living;
        }
        if (types & PhysicalEntityTypes::Independent)
        {
            result |= ent_independent;
        }
        if (types & PhysicalEntityTypes::Terrain)
        {
            result |= ent_terrain;
        }

        return result;
    }

    /**
    * Behavior handler for PhysicsSystemEventBus
    */
    class PhysicsSystemEventBusBehaviorHandler
        : public PhysicsSystemEventBus::Handler
        , public AZ::BehaviorEBusHandler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER(PhysicsSystemEventBusBehaviorHandler, "{BA278D21-0BB0-43B9-8FFA-08BE25316BC4}", AZ::SystemAllocator
            , OnPrePhysicsUpdate
            , OnPostPhysicsUpdate
            );

        void OnPrePhysicsUpdate() override
        {
            Call(FN_OnPrePhysicsUpdate);
        }

        void OnPostPhysicsUpdate() override
        {
            Call(FN_OnPostPhysicsUpdate);
        }
    };

    // Overrides for functions that take indices, to account for Lua's 1-based indexing.
    namespace RayCastResultScriptOverrides
    {
        static const AzFramework::PhysicsSystemRequests::RayCastHit* GetHit(AzFramework::PhysicsSystemRequests::RayCastResult* self, int index)
        {
            return self->GetHit(index - 1);
        }

        static const AzFramework::PhysicsSystemRequests::RayCastHit* GetPiercingHit(AzFramework::PhysicsSystemRequests::RayCastResult* self, int index)
        {
            return self->GetPiercingHit(index - 1);
        }
    };

    void PhysicsSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<PhysicsSystemComponent, AZ::Component>()
                ->Version(1)
            ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<PhysicsSystemComponent>(
                    "CryPhysics Manager", "Allows Component Entities to work with CryPhysics")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Game")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b))
                    ;
            }

            serializeContext->Class<RayCastHit>()
                ->Version(1)
                ->Field("distance", &RayCastHit::m_distance)
                ->Field("position", &RayCastHit::m_position)
                ->Field("normal", &RayCastHit::m_normal)
                ->Field("entityId", &RayCastHit::m_entityId)
                ;

            serializeContext->Class<RayCastResult>()
                ->Version(1);

            serializeContext->Class<RayCastConfiguration>()
                ->Version(1)
                ->Field("origin", &RayCastConfiguration::m_origin)
                ->Field("direction", &RayCastConfiguration::m_direction)
                ->Field("maxDistance", &RayCastConfiguration::m_maxDistance)
                ->Field("ignoreEntityIds", &RayCastConfiguration::m_ignoreEntityIds)
                ->Field("maxHits", &RayCastConfiguration::m_maxHits)
                ->Field("piercesSurfacesGreaterThan", &RayCastConfiguration::m_piercesSurfacesGreaterThan)
                ->Field("physicalEntityTypes", &RayCastConfiguration::m_physicalEntityTypes)
                ;
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            // RayCast return type
            behaviorContext->Class<RayCastHit>()
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Property("distance", BehaviorValueGetter(&RayCastHit::m_distance), nullptr)
                ->Property("position", BehaviorValueGetter(&RayCastHit::m_position), nullptr)
                ->Property("normal", BehaviorValueGetter(&RayCastHit::m_normal), nullptr)
                ->Property("entityId", BehaviorValueGetter(&RayCastHit::m_entityId), nullptr)
                ->Method("IsValid", &RayCastHit::IsValid)
                ;

            // RayCastConfiguration
            behaviorContext->Class<RayCastConfiguration>()
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                ->Property("origin", BehaviorValueProperty(&RayCastConfiguration::m_origin))
                ->Property("direction", BehaviorValueProperty(&RayCastConfiguration::m_direction))
                ->Property("maxDistance", BehaviorValueProperty(&RayCastConfiguration::m_maxDistance))
                ->Property("ignoreEntityIds", BehaviorValueProperty(&RayCastConfiguration::m_ignoreEntityIds))
                ->Property("maxHits", BehaviorValueProperty(&RayCastConfiguration::m_maxHits))
                ->Property("piercesSurfacesGreaterThan", BehaviorValueProperty(&RayCastConfiguration::m_piercesSurfacesGreaterThan))
                ->Property("physicalEntityTypes", BehaviorValueProperty(&RayCastConfiguration::m_physicalEntityTypes))
                ;

            // RayCastResult
            behaviorContext->Class<RayCastResult>()
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                ->Method("GetHitCount", &RayCastResult::GetHitCount)
                ->Method("GetHit", &RayCastResult::GetHit)
                    ->Attribute(AZ::Script::Attributes::MethodOverride, &RayCastResultScriptOverrides::GetHit)
                ->Method("HasBlockingHit", &RayCastResult::HasBlockingHit)
                ->Method("GetBlockingHit", &RayCastResult::GetBlockingHit)
                ->Method("GetPiercingHitCount", &RayCastResult::GetPiercingHitCount)
                ->Method("GetPiercingHit", &RayCastResult::GetPiercingHit)
                    ->Attribute(AZ::Script::Attributes::MethodOverride, &RayCastResultScriptOverrides::GetPiercingHit)
                // script operators
                ->Method("ScriptLength", [](RayCastResult* self) { return aznumeric_cast<int>(self->GetHitCount()); })
                    ->Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Length)
                ->Method("ScriptIndexRead", &RayCastResult::GetHit)
                    ->Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::IndexRead)
                    ->Attribute(AZ::Script::Attributes::MethodOverride, &RayCastResultScriptOverrides::GetHit)

                ;

            // Entity query flags
            behaviorContext->Class<PhysicalEntityTypesHolder>("PhysicalEntityTypes")
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::Preview)
                ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                ->Constant("Static", BehaviorConstant(AzFramework::PhysicalEntityTypes::Static))
                ->Constant("Dynamic", BehaviorConstant(AzFramework::PhysicalEntityTypes::Dynamic))
                ->Constant("Living", BehaviorConstant(AzFramework::PhysicalEntityTypes::Living))
                ->Constant("Independent", BehaviorConstant(AzFramework::PhysicalEntityTypes::Independent))
                ->Constant("Terrain", BehaviorConstant(AzFramework::PhysicalEntityTypes::Terrain))
                ->Constant("All", BehaviorConstant(AzFramework::PhysicalEntityTypes::All))
                ;

            // Global function for bitwise toggle to configure ray casting entity types.
            // Bit manipulation for Lua is disabled for security purposes.
            behaviorContext->Method("TogglePhysicalEntityTypeMask", &PhysicalEntityTypesHolder::ToggleEntityTypeMask);

            behaviorContext->EBus<PhysicsSystemRequestBus>("PhysicsSystemRequestBus")
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Event("RayCast", &PhysicsSystemRequestBus::Events::RayCast)
                ->Event("GatherPhysicalEntitiesInAABB", &PhysicsSystemRequestBus::Events::GatherPhysicalEntitiesInAABB)
                ->Event("GatherPhysicalEntitiesAroundPoint", &PhysicsSystemRequestBus::Events::GatherPhysicalEntitiesAroundPoint)
                ;

            behaviorContext->EBus<PhysicsSystemEventBus>("PhysicsSystemEventBus")
                ->Handler<PhysicsSystemEventBusBehaviorHandler>()
                ;
        }
    }

    // private namespace for helper functions
    namespace PhysicalWorldEventClient
    {
        // return whether object involved in event was an AZ::Entity
        static bool InvolvesAzEntity(const EventPhysMono& event)
        {
            return event.iForeignData == PHYS_FOREIGN_ID_COMPONENT_ENTITY;
        }

        // return whether either object involved in event was an AZ::Entity
        static bool InvolvesAzEntity(const EventPhysStereo& event)
        {
            return event.iForeignData[0] == PHYS_FOREIGN_ID_COMPONENT_ENTITY
                   || event.iForeignData[1] == PHYS_FOREIGN_ID_COMPONENT_ENTITY;
        }

        static AZ::EntityId GetEntityId(const EventPhysMono& event)
        {
            if (event.iForeignData == PHYS_FOREIGN_ID_COMPONENT_ENTITY)
            {
                return static_cast<AZ::EntityId>(event.pForeignData);
            }

            return AZ::EntityId();
        }

        static AZ::EntityId GetEntityId(const EventPhysStereo& event, int entityIndex)
        {
            AZ_Assert(entityIndex >= 0 && entityIndex <= 1, "invalid entityI");

            if (event.iForeignData[entityIndex] == PHYS_FOREIGN_ID_COMPONENT_ENTITY)
            {
                return static_cast<AZ::EntityId>(event.pForeignData[entityIndex]);
            }

            return AZ::EntityId();
        }

        static int GetSurfaceId(const EventPhysStereo& event, int entityIndex)
        {
            AZ_Assert(entityIndex >= 0 && entityIndex <= 1, "invalid entityI");

            _smart_ptr<IMaterial> mat = nullptr;
            int type = event.iForeignData[entityIndex];

            switch (type)
            {
                case PHYS_FOREIGN_ID_COMPONENT_ENTITY:
                {
                    AZ::EntityId id = static_cast<AZ::EntityId>(event.pForeignData[entityIndex]);
                    LmbrCentral::MaterialOwnerRequestBus::EventResult(mat, id, &LmbrCentral::MaterialOwnerRequestBus::Events::GetMaterial);
                    break;
                }
                case PHYS_FOREIGN_ID_ENTITY:
                {
                    IEntity* entity = static_cast<IEntity*>(event.pForeignData[entityIndex]);
                    if (entity != nullptr)
                    {
                        mat = entity->GetMaterial();
                    }
                    break;
                }
                case PHYS_FOREIGN_ID_STATIC:
                {
                    IRenderNode* pRenderNode = static_cast<IRenderNode*>(event.pForeignData[entityIndex]);
                    if (pRenderNode != nullptr)
                    {
                        mat = pRenderNode->GetMaterial();
                    }
                    break;
                }
            }

            if (mat == nullptr || mat->GetSurfaceType() == nullptr)
            {
                return 0;
            }

            return mat->GetSurfaceType()->GetId();
        }

        // An IPhysicalWorld event client can prevent the event
        // from propagating any further
        enum EventClientReturnValues
        {
            StopEventProcessing = 0,
            ContinueEventProcessing = 1,
        };

        static int OnCollision(const EventPhys* event)
        {
            const EventPhysCollision& collisionIn = static_cast<const EventPhysCollision&>(*event);
            if (!InvolvesAzEntity(collisionIn))
            {
                return ContinueEventProcessing;
            }

            AzFramework::PhysicsComponentNotifications::Collision collision;

            collision.m_position = LYVec3ToAZVec3(collisionIn.pt);
            collision.m_normal = LYVec3ToAZVec3(collisionIn.n);
            collision.m_impulse = collisionIn.normImpulse;

            for (int senderI : {0, 1})
            {
                AZ::EntityId sender = GetEntityId(collisionIn, senderI);
                if (sender.IsValid())
                {
                    int otherI = 1 - senderI;
                    collision.m_entity = GetEntityId(collisionIn, otherI);

                    collision.m_velocities[0] = LYVec3ToAZVec3(collisionIn.vloc[senderI]);
                    collision.m_velocities[1] = LYVec3ToAZVec3(collisionIn.vloc[otherI]);

                    collision.m_masses[0] = collisionIn.mass[senderI];
                    collision.m_masses[1] = collisionIn.mass[otherI];

                    collision.m_surfaces[0] = GetSurfaceId(collisionIn, senderI);
                    collision.m_surfaces[1] = GetSurfaceId(collisionIn, otherI);

                    using CollisionPoint = AzFramework::PhysicsComponentNotifications::CollisionPoint;

                    CollisionPoint point;
                    point.m_position = collision.m_position;
                    point.m_normal = collision.m_normal;
                    point.m_impulse = collision.m_impulse * collision.m_normal;
                    point.m_separation = 0.0f;
                    point.m_internalFaceIndex01 = collision.m_surfaces[0];
                    point.m_internalFaceIndex02 = collision.m_surfaces[1];

                    collision.m_collisionPoints = { point };

                    EBUS_EVENT_ID(sender, PhysicsComponentNotificationBus, OnCollision, collision);
                }
            }

            return ContinueEventProcessing;
        }

        static int OnPostStep(const EventPhys* event)
        {
            auto& eventIn = static_cast<const EventPhysPostStep&>(*event);
            if (!InvolvesAzEntity(eventIn))
            {
                return ContinueEventProcessing;
            }

            EntityPhysicsEvents::PostStep eventOut;
            eventOut.m_entity = GetEntityId(eventIn);
            eventOut.m_entityPosition = LYVec3ToAZVec3(eventIn.pos);
            eventOut.m_entityRotation = LYQuaternionToAZQuaternion(eventIn.q);
            eventOut.m_stepTimeDelta = eventIn.dt;
            eventOut.m_stepId = eventIn.idStep;

            EBUS_EVENT_ID(eventOut.m_entity, EntityPhysicsEventBus, OnPostStep, eventOut);

            return ContinueEventProcessing;
        }
    } // namespace PhysicalWorldEventClient

    void PhysicsSystemComponent::Activate()
    {
        PhysicsSystemRequestBus::Handler::BusConnect();
        CrySystemEventBus::Handler::BusConnect();
    }

    void PhysicsSystemComponent::Deactivate()
    {
        PhysicsSystemRequestBus::Handler::BusDisconnect();
        CrySystemEventBus::Handler::BusDisconnect();
        SetEnabled(false);
    }

    PhysicsSystemComponent::RayCastResult PhysicsSystemComponent::RayCast(const RayCastConfiguration& configuration)
    {
        AZ_Warning("Physics", configuration.m_direction.IsNormalized(), "RayCast direction should be normalized.");

        // Destination for CryPhysics hits.
        // CryPhysics always reserves index 0 for the blocking hit.
        // We ask for 1 extra hit to avoid situation where user asks for two hits,
        // and no blocking surfaces are encountered, but only 1 piercing hit is
        // returned because index 0 was reserved for a blocking hit.
        AZStd::vector<ray_hit> cryHits(configuration.m_maxHits + 1);

        // Fill out SWRIParams based on RayCastConfiguration
        IPhysicalWorld::SRWIParams cryParams;
        cryParams.org = AZVec3ToLYVec3(configuration.m_origin);
        cryParams.dir = AZVec3ToLYVec3(configuration.m_direction * configuration.m_maxDistance);
        cryParams.objtypes = EntFromEntityTypes(configuration.m_physicalEntityTypes);
        cryParams.hits = cryHits.data();
        cryParams.nMaxHits = cryHits.size();
        cryParams.flags = rwi_pierceability(AZ::GetClamp<decltype(configuration.m_piercesSurfacesGreaterThan)>(configuration.m_piercesSurfacesGreaterThan, 0, sf_max_pierceable));

        // Set ignored entities
        AZStd::vector<IPhysicalEntity*> ignorePhysicalEntities;
        ignorePhysicalEntities.reserve(configuration.m_ignoreEntityIds.size());
        for (const AZ::EntityId& entityId : configuration.m_ignoreEntityIds)
        {
            IPhysicalEntity* physicalEntity = nullptr;
            EBUS_EVENT_ID_RESULT(physicalEntity, entityId, LmbrCentral::CryPhysicsComponentRequestBus, GetPhysicalEntity);
            if (physicalEntity)
            {
                ignorePhysicalEntities.push_back(physicalEntity);
            }
        }
        cryParams.pSkipEnts = ignorePhysicalEntities.data();
        cryParams.nSkipEnts = ignorePhysicalEntities.size();

        // Perform raycast
        const int hitCount = m_physicalWorld->RayWorldIntersection(cryParams);

        // We asked CryPhysics for 1 more hit than user actually wants.
        // Trim off the extra if necessary.
        if (hitCount > configuration.m_maxHits)
        {
            cryHits.pop_back();
        }

        // Fill out RayCastResults based on cryHits
        RayCastResult results;

        for (size_t cryHitsIndex = 0; cryHitsIndex < cryHits.size(); ++cryHitsIndex)
        {
            const ray_hit& cryHit = cryHits[cryHitsIndex];

            // Note that CryPhysics always puts the blocking hit at index 0.
            // If no blocking hits occurred then index 0 holds a dummy entry with distance of -1.
            if (cryHit.dist < 0.f)
            {
                if (cryHitsIndex == 0)
                {
                    continue;
                }
                else
                {
                    break;
                }
            }

            RayCastHit hit;
            hit.m_distance = cryHit.dist;
            hit.m_position = LYVec3ToAZVec3(cryHit.pt);
            hit.m_normal = LYVec3ToAZVec3(cryHit.n);

            if (cryHit.pCollider && cryHit.pCollider->GetiForeignData() == PHYS_FOREIGN_ID_COMPONENT_ENTITY)
            {
                hit.m_entityId = static_cast<AZ::EntityId>(cryHit.pCollider->GetForeignData(PHYS_FOREIGN_ID_COMPONENT_ENTITY));
            }

            if (cryHitsIndex == 0)
            {
                results.SetBlockingHit(hit);
            }
            else
            {
                results.AddPiercingHit(hit);
            }
        }

        return results;
    }

    AZStd::vector<AZ::EntityId> PhysicsSystemComponent::GatherPhysicalEntitiesInAABB(const AZ::Aabb& aabb, AZ::u32 query)
    {
        AZStd::vector<AZ::EntityId> gatheredEntityIds;

        IPhysicalEntity** results = nullptr;
        const int resultCount = m_physicalWorld->GetEntitiesInBox(
            AZVec3ToLYVec3(aabb.GetMin()), AZVec3ToLYVec3(aabb.GetMax()),
            results,
            EntFromEntityTypes(query));

        if (resultCount > 0)
        {
            AZ_Assert(results != nullptr, "Invalid result list returned with positive count.");

            for (int i = 0; i < resultCount; ++i)
            {
                IPhysicalEntity* physicalEntity = results[i];
                AZ_Error("Physics", physicalEntity, "Invalid entity in GetPhysicalEntities query results.");

                if (physicalEntity && physicalEntity->GetiForeignData() == PHYS_FOREIGN_ID_COMPONENT_ENTITY)
                {
                    const AZ::EntityId id = static_cast<AZ::EntityId>(physicalEntity->GetForeignData(PHYS_FOREIGN_ID_COMPONENT_ENTITY));
                    gatheredEntityIds.push_back(id);
                }
            }
        }

        return gatheredEntityIds;
    }

    AZStd::vector<AZ::EntityId> PhysicsSystemComponent::GatherPhysicalEntitiesAroundPoint(const AZ::Vector3& center, float radius, AZ::u32 query)
    {
        const float radiusSq = radius * radius;

        const AZ::Aabb aabb = AZ::Aabb::CreateCenterRadius(center, radius);
        AZStd::vector<AZ::EntityId> gatheredEntityIds = GatherPhysicalEntitiesInAABB(aabb, query);
        for (auto iter = gatheredEntityIds.begin(); iter != gatheredEntityIds.end(); )
        {
            AZ::Transform transform = AZ::Transform::CreateIdentity();
            AZ::TransformBus::EventResult(transform, *iter, &AZ::TransformInterface::GetWorldTM);
            if ((center - transform.GetTranslation()).GetLengthSq() > radiusSq)
            {
                iter = gatheredEntityIds.erase(iter);
            }
            else
            {
                ++iter;
            }
        }

        return gatheredEntityIds;
    }

    void PhysicsSystemComponent::OnCrySystemPrePhysicsUpdate()
    {
        EBUS_EVENT(PhysicsSystemEventBus, OnPrePhysicsUpdate);
    }

    void PhysicsSystemComponent::OnCrySystemPostPhysicsUpdate()
    {
        EBUS_EVENT(PhysicsSystemEventBus, OnPostPhysicsUpdate);
    }

    void PhysicsSystemComponent::OnCrySystemInitialized(ISystem& system, const SSystemInitParams&)
    {
        m_physicalWorld = system.GetGlobalEnvironment()->pPhysicalWorld;
        SetEnabled(true);
    }

    void PhysicsSystemComponent::OnCrySystemShutdown(ISystem& system)
    {
        // Purposely clearing m_physicalWorld before SetEnabled(false).
        // The IPhysicalWorld is going away, it doesn't matter if we carefully unregister from it.
        m_physicalWorld = nullptr;
        SetEnabled(false);
    }

    void PhysicsSystemComponent::SetEnabled(bool enable)
    {
        // can't be enabled if physical world doesn't exist
        if (!m_physicalWorld)
        {
            m_enabled = false;
            return;
        }

        if (enable == m_enabled)
        {
            return;
        }

        // Params for calls to IPhysicalWorld::AddEventClient and RemoveEventClient.
        enum
        {
            PARAM_ID, // id of the IPhysicalWorld event type
            PARAM_HANDLER, // handler function
            PARAM_LOGGED, // 1 for a logged event version, 0 for immediate
            PARAM_PRIORITY, // priority (higher means handled first)
        };
        std::tuple<int, int(*)(const EventPhys*), int, float> physicalWorldEventClientParams[] = {
            std::make_tuple(EventPhysCollision::id, PhysicalWorldEventClient::OnCollision,  1,  1.f),
            std::make_tuple(EventPhysPostStep::id,  PhysicalWorldEventClient::OnPostStep,   1,  1.f),
        };

        // start/stop listening for events
        for (auto& i : physicalWorldEventClientParams)
        {
            if (enable)
            {
                m_physicalWorld->AddEventClient(std::get<PARAM_ID>(i), std::get<PARAM_HANDLER>(i), std::get<PARAM_LOGGED>(i), std::get<PARAM_PRIORITY>(i));
            }
            else
            {
                m_physicalWorld->RemoveEventClient(std::get<PARAM_ID>(i), std::get<PARAM_HANDLER>(i), std::get<PARAM_LOGGED>(i));
            }
        }

        m_enabled = enable;
    }
} // namespace LmbrCentral
