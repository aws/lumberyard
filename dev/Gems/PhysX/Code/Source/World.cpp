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
#include <Source/World.h>
#include <PhysX/MathConversion.h>
#include <Source/SystemComponent.h>
#include <PhysX/SystemComponentBus.h>
#include <PhysX/NativeTypeIdentifiers.h>
#include <AzFramework/Physics/PhysicsComponentBus.h>
#include <AzFramework/Physics/WorldBody.h>
#include <AzFramework/Physics/WorldEventhandler.h>
#include <Source/RigidBody.h>
#include <Source/Collision.h>
#include <Source/Shape.h>
#include <PhysX/Utils.h>


namespace PhysX
{
    //Helper class, responsible for filtering invalid collision candidates prior to more expensive narrow phase checks
    class PhysXQueryFilterCallback
        : public physx::PxQueryFilterCallback
    {
    public:
        explicit PhysXQueryFilterCallback(const Physics::CollisionGroup& collisionGroup, Physics::CustomFilterCallback filterCallback, physx::PxQueryHitType::Enum hitType)
            : m_filterCallback(filterCallback)
            , m_collisionGroup(collisionGroup)
            , m_hitType(hitType)
        {
        }

        //Performs game specific entity filtering
        physx::PxQueryHitType::Enum preFilter(const physx::PxFilterData& queryFilterData, const physx::PxShape* pxShape,
            const physx::PxRigidActor* actor, physx::PxHitFlags& queryTypes) override
        {
            auto shapeFilterData = pxShape->getQueryFilterData();
            
            if (m_collisionGroup.GetMask() & Collision::Combine(shapeFilterData.word0, shapeFilterData.word1))
            {
                if (m_filterCallback)
                {
                    auto userData = Utils::GetUserData(actor);
                    auto shape = Utils::GetUserData(pxShape);
                    if (userData != nullptr && userData->GetEntityId().IsValid() && m_filterCallback(userData->GetWorldBody(), shape))
                    {
                        return m_hitType;
                    }
                }
                else
                {
                    return m_hitType;
                }
            }
            return physx::PxQueryHitType::eNONE;
        }

        // Unused, we're only prefiltering at this time
        physx::PxQueryHitType::Enum postFilter(const physx::PxFilterData&, const physx::PxQueryHit&) override
        {
            return physx::PxQueryHitType::eNONE;
        }

    private:
        Physics::CustomFilterCallback m_filterCallback;
        Physics::CollisionGroup m_collisionGroup;
        physx::PxQueryHitType::Enum m_hitType;
    };

    World::World(AZ::Crc32 id, const Physics::WorldConfiguration& settings)
        : m_worldId(id)
        , m_maxDeltaTime(settings.m_maxTimeStep)
        , m_fixedDeltaTime(settings.m_fixedTimeStep)
    {
        m_raycastBuffer.resize(settings.m_raycastBufferSize);
        m_sweepBuffer.resize(settings.m_sweepBufferSize);
        m_overlapBuffer.resize(settings.m_overlapBufferSize);

        Physics::WorldRequestBus::Handler::BusConnect(id);

        physx::PxTolerancesScale tolerancesScale = physx::PxTolerancesScale();
        physx::PxSceneDesc sceneDesc(tolerancesScale);
        sceneDesc.gravity = PxMathConvert(settings.m_gravity);
        if (settings.m_enableCcd)
        {
            sceneDesc.flags |= physx::PxSceneFlag::eENABLE_CCD;
            sceneDesc.filterShader = Collision::DefaultFilterShaderCCD;
        }
        else
        {
            sceneDesc.filterShader = Collision::DefaultFilterShader;
        }

        if (settings.m_enableActiveActors)
        {
            sceneDesc.flags |= physx::PxSceneFlag::eENABLE_ACTIVE_ACTORS;
        }

        if (settings.m_enablePcm)
        {
            sceneDesc.flags |= physx::PxSceneFlag::eENABLE_PCM;
        }
        else
        {
            sceneDesc.flags &= ~physx::PxSceneFlag::eENABLE_PCM;
        }

        sceneDesc.filterCallback = this;

        SystemRequestsBus::BroadcastResult(m_world, &SystemRequests::CreateScene, sceneDesc);
        m_world->userData = this;

        physx::PxPvdSceneClient* pvdClient = m_world->getScenePvdClient();
        if (pvdClient)
        {
            pvdClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
            pvdClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
            pvdClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
        }
    }

    World::~World()
    {
        Physics::WorldRequestBus::Handler::BusDisconnect();
        Physics::SystemNotificationBus::Broadcast(&Physics::SystemNotificationBus::Events::OnPreWorldDestroy, this);
        if (m_world)
        {
            m_world->release();
            m_world = nullptr;
        }
    }

    static Physics::RayCastHit GetHitFromPxHit(const physx::PxLocationHit& pxHit)
    {
        Physics::RayCastHit hit;

        hit.m_distance = pxHit.distance;
        hit.m_position = PxMathConvert(pxHit.position);
        hit.m_normal = PxMathConvert(pxHit.normal);
        hit.m_body = Utils::GetUserData(pxHit.actor)->GetWorldBody();
        hit.m_shape = Utils::GetUserData(pxHit.shape);

        if (pxHit.faceIndex != 0xFFFFffff)
        {
            hit.m_material = Utils::GetUserData(pxHit.shape->getMaterialFromInternalFaceIndex(pxHit.faceIndex));
        }
        else
        {
            hit.m_material = hit.m_shape->GetMaterial().get();
        }

        return hit;
    }

    static physx::PxQueryFlags GetPxQueryFlags(const Physics::QueryType& queryType)
    {
        physx::PxQueryFlags queryFlags = physx::PxQueryFlag::ePREFILTER;
        switch (queryType)
        {
        case Physics::QueryType::StaticAndDynamic:
            queryFlags |= physx::PxQueryFlag::eSTATIC | physx::PxQueryFlag::eDYNAMIC;
            break;
        case Physics::QueryType::Dynamic:
            queryFlags |= physx::PxQueryFlag::eDYNAMIC;
            break;
        case Physics::QueryType::Static:
            queryFlags |= physx::PxQueryFlag::eSTATIC;
            break;
        default:
            AZ_Warning("Physics::World", false, "Unhandled queryType");
            break;
        }
        return queryFlags;
    }

    Physics::RayCastHit World::RayCast(const Physics::RayCastRequest& request)
    {
        const auto orig = PxMathConvert(request.m_start);
        const auto dir = PxMathConvert(request.m_direction);
        
        // Query flags. 
        // Note: we specify eBLOCK here as we're only interested in the closest object. The touches field in the result will be invalid
        const physx::PxQueryFlags queryFlags = GetPxQueryFlags(request.m_queryType);
        const physx::PxQueryFilterData queryData(queryFlags);
        const physx::PxHitFlags outputFlags = physx::PxHitFlag::eDEFAULT | physx::PxHitFlag::eMESH_BOTH_SIDES;
        PhysXQueryFilterCallback queryFilterCallback(request.m_collisionGroup, request.m_customFilterCallback, physx::PxQueryHitType::eBLOCK);
        
        // Raycast
        physx::PxRaycastBuffer castResult;
        bool status = m_world->raycast(orig, dir, request.m_distance, castResult, outputFlags, queryData, &queryFilterCallback);

        // Convert to generic API
        Physics::RayCastHit hit;
        if (status)
        {
            hit = GetHitFromPxHit(castResult.block);
        }
        return hit;
    }

    AZStd::vector<Physics::RayCastHit> World::RayCastMultiple(const Physics::RayCastRequest& request)
    {
        const auto orig = PxMathConvert(request.m_start);
        const auto dir = PxMathConvert(request.m_direction);

        // Query flags. 
        // Note: we specify eTOUCH here as we're interested in all hits that intersect the ray. The block field in result will be invalid.
        const physx::PxQueryFlags queryFlags = GetPxQueryFlags(request.m_queryType);
        const physx::PxQueryFilterData queryData(queryFlags);
        const physx::PxHitFlags outputFlags = physx::PxHitFlag::eDEFAULT | physx::PxHitFlag::eMESH_BOTH_SIDES;
        PhysXQueryFilterCallback queryFilterCallback(request.m_collisionGroup, request.m_customFilterCallback, physx::PxQueryHitType::eTOUCH);

        // Raycast
        physx::PxRaycastBuffer castResult(m_raycastBuffer.begin(), (physx::PxU32)m_raycastBuffer.size());
        bool status = m_world->raycast(orig, dir, request.m_distance, castResult, outputFlags, queryData, &queryFilterCallback);

        // Convert to generic API
        AZStd::vector<Physics::RayCastHit> hits;
        if (status)
        {
            for (auto i = 0u; i < castResult.getNbTouches(); ++i)
            {
                const auto& pxHit = castResult.getTouch(i);
                hits.push_back(GetHitFromPxHit(pxHit));
            }
        }
        return hits;
    }

    Physics::RayCastHit World::ShapeCast(const Physics::ShapeCastRequest& request)
    {
        const physx::PxTransform pose = PxMathConvert(request.m_start);
        const physx::PxVec3 dir = PxMathConvert(request.m_direction);

        const physx::PxQueryFlags queryFlags = GetPxQueryFlags(request.m_queryType);
        const physx::PxQueryFilterData queryData(queryFlags);
        const physx::PxHitFlags outputFlags = physx::PxHitFlag::eDEFAULT | physx::PxHitFlag::eMESH_BOTH_SIDES;
        PhysXQueryFilterCallback queryFilterCallback(request.m_collisionGroup, request.m_customFilterCallback, physx::PxQueryHitType::eBLOCK);

        // Buffer to store results in.
        physx::PxSweepBuffer pxResult;
        physx::PxGeometryHolder pxGeometry;
        Utils::CreatePxGeometryFromConfig(*request.m_shapeConfiguration, pxGeometry);

        Physics::RayCastHit hit;
        if (pxGeometry.any().getType() == physx::PxGeometryType::eSPHERE ||
            pxGeometry.any().getType() == physx::PxGeometryType::eBOX ||
            pxGeometry.any().getType() == physx::PxGeometryType::eCAPSULE ||
            pxGeometry.any().getType() == physx::PxGeometryType::eCONVEXMESH)
        {
            bool status = m_world->sweep(pxGeometry.any(), pose, dir, request.m_distance, pxResult, outputFlags, queryData, &queryFilterCallback);
            if (status)
            {
                hit = GetHitFromPxHit(pxResult.block);
            }
        }
        else
        {
            AZ_Warning("World", false, "Invalid geometry type passed to shape cast. Only sphere, box, capsule or convex mesh is supported");
        }

        return hit;
    }

    AZStd::vector<Physics::RayCastHit> World::ShapeCastMultiple(const Physics::ShapeCastRequest& request)
    {
        const physx::PxTransform pose = PxMathConvert(request.m_start);
        const physx::PxVec3 dir = PxMathConvert(request.m_direction);

        const physx::PxQueryFlags queryFlags = GetPxQueryFlags(request.m_queryType);
        const physx::PxQueryFilterData queryData(queryFlags);
        const physx::PxHitFlags outputFlags = physx::PxHitFlag::eDEFAULT | physx::PxHitFlag::eMESH_BOTH_SIDES;
        PhysXQueryFilterCallback queryFilterCallback(request.m_collisionGroup, request.m_customFilterCallback, physx::PxQueryHitType::eTOUCH);

        // Buffer to store results in.
        physx::PxSweepBuffer pxResult(m_sweepBuffer.begin(), (physx::PxU32)m_sweepBuffer.size());
        physx::PxGeometryHolder pxGeometry;
        Utils::CreatePxGeometryFromConfig(*request.m_shapeConfiguration, pxGeometry);

        AZStd::vector<Physics::RayCastHit> hits;
        if (pxGeometry.any().getType() == physx::PxGeometryType::eSPHERE ||
            pxGeometry.any().getType() == physx::PxGeometryType::eBOX ||
            pxGeometry.any().getType() == physx::PxGeometryType::eCAPSULE ||
            pxGeometry.any().getType() == physx::PxGeometryType::eCONVEXMESH)
        {
            bool status = m_world->sweep(pxGeometry.any(), pose, dir, request.m_distance, pxResult, outputFlags, queryData, &queryFilterCallback);
            if (status)
            {
                for (auto i = 0u; i < pxResult.getNbTouches(); ++i)
                {
                    const auto& pxHit = pxResult.getTouch(i);
                    hits.push_back(GetHitFromPxHit(pxHit));
                }
            }
        }
        else
        {
            AZ_Warning("World", false, "Invalid geometry type passed to shape cast. Only sphere, box, capsule or convex mesh is supported");
        }

        return hits;
    }

    AZStd::vector<Physics::OverlapHit> World::Overlap(const Physics::OverlapRequest& request)
    {
        // Prepare overlap data
        const physx::PxTransform pose = PxMathConvert(request.m_pose);
        physx::PxGeometryHolder pxGeometry;
        Utils::CreatePxGeometryFromConfig(*request.m_shapeConfiguration, pxGeometry);

        const physx::PxQueryFlags queryFlags = GetPxQueryFlags(request.m_queryType);
        const physx::PxQueryFilterData defaultFilterData(queryFlags);
        PhysXQueryFilterCallback filterCallback(request.m_collisionGroup, request.m_customFilterCallback, physx::PxQueryHitType::eTOUCH);

        // Buffer to store results in.
        physx::PxOverlapBuffer queryHits(m_overlapBuffer.begin(), (physx::PxU32)m_overlapBuffer.size());
        bool status = m_world->overlap(pxGeometry.any(), pose, queryHits, defaultFilterData, &filterCallback);
        AZStd::vector<Physics::OverlapHit> hits;
        if (status)
        {
            // Process results
            AZ::u32 hitNum = queryHits.getNbAnyHits();
            for (AZ::u32 i = 0; i < hitNum; ++i)
            {
                const physx::PxOverlapHit& hit = queryHits.getAnyHit(i);
                if (auto userData = Utils::GetUserData(hit.actor))
                {
                    Physics::OverlapHit resultHit;
                    resultHit.m_body = userData->GetWorldBody();
                    resultHit.m_shape = static_cast<PhysX::Shape*>(hit.shape->userData);
                    hits.push_back(resultHit);
                }
            }
        }
        return hits;
    }

    physx::PxActor* GetPxActor(const AZStd::shared_ptr<Physics::WorldBody>& worldBody)
    {
        if (worldBody->GetNativeType() != NativeTypeIdentifiers::RigidBody &&
            worldBody->GetNativeType() != NativeTypeIdentifiers::RigidBodyStatic)
        {
            return nullptr;
        }

        return static_cast<physx::PxActor*>(worldBody->GetNativePointer());
    }

    AZStd::unordered_set<World::ActorPair>::iterator World::FindSuppressedPair(const physx::PxActor* actor0, const physx::PxActor* actor1)
    {
        auto iterator = m_suppressedCollisionPairs.find(AZStd::make_pair(actor0, actor1));
        if (iterator != m_suppressedCollisionPairs.end())
        {
            return iterator;
        }

        // also check for the pair with the actors in the other order
        return m_suppressedCollisionPairs.find(AZStd::make_pair(actor1, actor0));
    }

    void World::RegisterSuppressedCollision(const AZStd::shared_ptr<Physics::WorldBody>& body0,
        const AZStd::shared_ptr<Physics::WorldBody>& body1)
    {
        physx::PxActor* actor0 = GetPxActor(body0);
        physx::PxActor* actor1 = GetPxActor(body1);
        if (actor0 && actor1)
        {
            if (FindSuppressedPair(actor0, actor1) == m_suppressedCollisionPairs.end())
            {
                m_suppressedCollisionPairs.insert(AZStd::make_pair(actor0, actor1));
            }
        }
    }

    void World::UnregisterSuppressedCollision(const AZStd::shared_ptr<Physics::WorldBody>& body0,
        const AZStd::shared_ptr<Physics::WorldBody>& body1)
    {
        physx::PxActor* actor0 = GetPxActor(body0);
        physx::PxActor* actor1 = GetPxActor(body1);
        if (actor0 && actor1)
        {
            auto iterator = FindSuppressedPair(actor0, actor1);
            if (iterator != m_suppressedCollisionPairs.end())
            {
                m_suppressedCollisionPairs.erase(*iterator);
            }
        }
    }

    void World::AddBody(Physics::WorldBody& body)
    {
        physx::PxRigidActor* rigidActor = static_cast<physx::PxRigidActor*>(body.GetNativePointer());
        AddBody(rigidActor);
        body.AddedToWorld();
    }

    void World::SetSimFunc(std::function<void(void*)> func)
    {
        m_simFunc = func;
    }

    void World::AddBody(physx::PxActor* body)
    {
        if (!m_world)
        {
            AZ_Error("PhysX World", false, "Tried to add body to invalid world.");
            return;
        }

        if (!body)
        {
            AZ_Error("PhysX World", false, "Tried to add invalid PhysX body to world.");
            return;
        }

        m_world->addActor(*body);
    }

    void World::RemoveBody(const Physics::WorldBody& body)
    {
        physx::PxRigidActor* actor = static_cast<physx::PxRigidActor*>(body.GetNativePointer());
        if (!m_world)
        {
            AZ_Error("PhysX World", false, "Tried to remove body from invalid world.");
            return;
        }

        if (!actor)
        {
            AZ_Error("PhysX World", false, "Tried to remove invalid PhysX body from world.");
            return;
        }

        m_world->removeActor(*actor);
    }

    void World::Update(float deltaTime)
    {
        
        auto simulateFetch = [this](float simDeltaTime, std::function<void(void * activeAct)> activeActorsLambda)
        {
            m_world->simulate(simDeltaTime);
            m_world->fetchResults(true);
            AzFramework::PhysicsComponentNotificationBus::ExecuteQueuedEvents();

            if (m_world->getFlags() & physx::PxSceneFlag::eENABLE_ACTIVE_ACTORS)
            {
                physx::PxU32 numActiveActors = 0;
                physx::PxActor** activeActors = m_world->getActiveActors(numActiveActors);

                for (physx::PxU32 i = 0; i < numActiveActors; ++i)
                {
                    if (activeActorsLambda)
                    {
                        activeActorsLambda(activeActors[i]);
                    }
                }
            }
        };

        deltaTime = AZ::GetClamp(deltaTime, 0.0f, m_maxDeltaTime);

        if (m_fixedDeltaTime != 0.0f)
        {
            m_accumulatedTime += deltaTime;

            while (m_accumulatedTime >= m_fixedDeltaTime)
            {
                Physics::SystemNotificationBus::Broadcast(&Physics::SystemNotificationBus::Events::OnPrePhysicsUpdate, m_fixedDeltaTime, this);

                simulateFetch(m_fixedDeltaTime, m_simFunc);
                m_accumulatedTime -= m_fixedDeltaTime;

                Physics::SystemNotificationBus::Broadcast(&Physics::SystemNotificationBus::Events::OnPostPhysicsUpdate, m_fixedDeltaTime, this);
            }
        }
        else
        {
            simulateFetch(deltaTime, m_simFunc);
        }
    }

    AZ::Crc32 World::GetNativeType() const
    {
        return PhysX::NativeTypeIdentifiers::World;
    }

    void* World::GetNativePointer() const
    {
        return m_world;
    }

    void World::SetEventHandler(Physics::WorldEventHandler* eventHandler)
    {
        m_eventHandler = eventHandler;
        if (m_eventHandler == nullptr)
        {
            m_world->setSimulationEventCallback(nullptr);
        }
        else
        {
            m_world->setSimulationEventCallback(this);
        }
    }

    // physx::PxSimulationFilterCallback
    physx::PxFilterFlags World::pairFound(physx::PxU32 pairId, physx::PxFilterObjectAttributes attributes0,
        physx::PxFilterData filterData0, const physx::PxActor* actor0, const physx::PxShape* shape0,
        physx::PxFilterObjectAttributes attributes1, physx::PxFilterData filterData1, const physx::PxActor* actor1,
        const physx::PxShape* shape1, physx::PxPairFlags& pairFlags)
    {
        if (FindSuppressedPair(actor0, actor1) != m_suppressedCollisionPairs.end())
        {
            return physx::PxFilterFlag::eSUPPRESS;
        }

        return physx::PxFilterFlag::eDEFAULT;
    }

    void World::pairLost(physx::PxU32 pairId, physx::PxFilterObjectAttributes attributes0, physx::PxFilterData filterData0,
        physx::PxFilterObjectAttributes attributes1, physx::PxFilterData filterData1, bool objectRemoved)
    {

    }

    bool World::statusChange(physx::PxU32& pairId, physx::PxPairFlags& pairFlags, physx::PxFilterFlags& filterFlags)
    {
        return false;
    }

    // physx::PxSimulationEventCallback
    void World::onConstraintBreak(physx::PxConstraintInfo* constraints, physx::PxU32 count)
    {
    }

    void World::onWake(physx::PxActor** actors, physx::PxU32 count)
    {
    }

    void World::onSleep(physx::PxActor** actors, physx::PxU32 count)
    {
    }

    void World::onContact(const physx::PxContactPairHeader& pairHeader, const physx::PxContactPair* pairs, physx::PxU32 nbPairs)
    {
        AZ_Assert(m_eventHandler != nullptr, "Event handler has not been set. This should not be possible, see World::SetEventHandler");

        const bool body01Destroyed = pairHeader.flags & physx::PxContactPairHeaderFlag::eREMOVED_ACTOR_0;
        const bool body02Destroyed = pairHeader.flags & physx::PxContactPairHeaderFlag::eREMOVED_ACTOR_1;
        if (body01Destroyed || body02Destroyed)
        {
            // We can't report destroyed bodies at the moment.
            return;
        }

        static const physx::PxU32 MaxPointsToReport = 10;
        for (physx::PxU32 i = 0; i < nbPairs; i++)
        {
            auto contactPair = pairs[i];
            auto flagsToNotify =
                physx::PxPairFlag::eNOTIFY_TOUCH_FOUND |
                physx::PxPairFlag::eNOTIFY_TOUCH_PERSISTS |
                physx::PxPairFlag::eNOTIFY_TOUCH_LOST;

            if (contactPair.events & flagsToNotify)
            {
                auto userData01 = Utils::GetUserData(pairHeader.actors[0]);
                auto userData02 = Utils::GetUserData(pairHeader.actors[1]);

                // Missing user data, or user data was invalid
                if (!userData01 || !userData02)
                {
                    AZ_Warning("PhysX::World", false, "Invalid user data set for objects Obj0:%p Obj1:%p", userData01, userData02);
                    continue;
                }

                Physics::WorldBody* body01 = userData01->GetWorldBody();
                Physics::WorldBody* body02 = userData02->GetWorldBody();

                if (!body01 || !body02)
                {
                    AZ_Warning("PhysX::World", false, "Invalid body data set for objects Obj0:%p Obj1:%p", body01, body02);
                    continue;
                }

                Physics::Shape* shape01 = Utils::GetUserData(contactPair.shapes[0]);
                Physics::Shape* shape02 = Utils::GetUserData(contactPair.shapes[1]);

                if (!shape01 || !shape02)
                {
                    AZ_Warning("PhysX::World", false, "Invalid shape userdata set for objects Obj0:%p Obj1:%p", shape01, shape02);
                    continue;
                }

                // Collision Event
                Physics::CollisionEvent collision;
                collision.m_body1 = body01;
                collision.m_body2 = body02;
                collision.m_shape1 = shape01;
                collision.m_shape2 = shape02;

                // Extract contacts for collision event
                physx::PxContactPairPoint extractedPoints[MaxPointsToReport];
                physx::PxU32 contactPointCount = contactPair.extractContacts(extractedPoints, MaxPointsToReport);
                collision.m_contacts.resize(contactPointCount);
                for (physx::PxU8 j = 0; j < contactPointCount; ++j)
                {
                    auto point = extractedPoints[j];

                    collision.m_contacts[j].m_position = PxMathConvert(point.position);
                    collision.m_contacts[j].m_normal = PxMathConvert(point.normal);
                    collision.m_contacts[j].m_impulse = PxMathConvert(point.impulse);
                    collision.m_contacts[j].m_separation = point.separation;
                    collision.m_contacts[j].m_internalFaceIndex01 = point.internalFaceIndex0;
                    collision.m_contacts[j].m_internalFaceIndex02 = point.internalFaceIndex1;
                }

                if (contactPair.events & physx::PxPairFlag::eNOTIFY_TOUCH_FOUND)
                {
                    m_eventHandler->OnCollisionBegin(collision);
                }
                else if (contactPair.events & physx::PxPairFlag::eNOTIFY_TOUCH_PERSISTS)
                {
                    m_eventHandler->OnCollisionPersist(collision);
                }
                else if (contactPair.events & physx::PxPairFlag::eNOTIFY_TOUCH_LOST)
                {
                    m_eventHandler->OnCollisionEnd(collision);
                }
            }
        }
    }

    void World::onTrigger(physx::PxTriggerPair* pairs, physx::PxU32 count)
    {
        AZ_Assert(m_eventHandler != nullptr, "Invalid event handler");

        for (physx::PxU32 i = 0; i < count; ++i)
        {
            physx::PxTriggerPair& triggerPair = pairs[i];

            if (triggerPair.triggerActor->userData && triggerPair.otherActor->userData)
            {
                auto triggerBody = Utils::GetUserData(triggerPair.triggerActor)->GetWorldBody();
                auto triggerShape = static_cast<PhysX::Shape*>(triggerPair.triggerShape->userData);

                if( !triggerBody )
                {
                    AZ_Error( "PhysX World", false, "onTrigger:: trigger body was invalid" );
                    continue;
                }

                auto otherBody = Utils::GetUserData(triggerPair.otherActor)->GetWorldBody();
                auto otherShape = static_cast<PhysX::Shape*>(triggerPair.otherShape->userData);

                if( !otherBody )
                {
                    AZ_Error( "PhysX World", false, "onTrigger:: otherBody was invalid" );
                    continue;
                }

                if (triggerBody->GetEntityId().IsValid() && otherBody->GetEntityId().IsValid())
                {
                    Physics::TriggerEvent triggerEvent;
                    triggerEvent.m_triggerBody = triggerBody;
                    triggerEvent.m_triggerShape = triggerShape;
                    triggerEvent.m_otherBody = otherBody;
                    triggerEvent.m_otherShape = otherShape;

                    if (triggerPair.status == physx::PxPairFlag::eNOTIFY_TOUCH_FOUND)
                    {
                        m_eventHandler->OnTriggerEnter(triggerEvent);
                    }
                    else if (triggerPair.status == physx::PxPairFlag::eNOTIFY_TOUCH_LOST)
                    {
                        m_eventHandler->OnTriggerExit(triggerEvent);
                    }
                    else
                    {
                        AZ_Warning("PhysX World", false, "onTrigger with status different from TOUCH_FOUND and TOUCH_LOST.");
                    }
                }
                else
                {
                    AZ_Warning("PhysX World", false, "onTrigger received invalid actors.");
                }
            }
        }
    }

    void World::onAdvance(const physx::PxRigidBody*const* bodyBuffer, const physx::PxTransform* poseBuffer, const physx::PxU32 count)
    {
    }

    AZ::Vector3 World::GetGravity()
    {
        if (m_world)
        {
            return PxMathConvert(m_world->getGravity());
        }
        return AZ::Vector3::CreateZero();
    }

    void World::SetGravity(const AZ::Vector3& gravity) 
    {
        if (m_world)
        {
            m_world->setGravity(PxMathConvert(gravity));
        }
    }
    
} // namespace PhysX
