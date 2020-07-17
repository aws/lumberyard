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
#include <PhysX/TriggerEventCallback.h>
#include <AzFramework/Physics/CollisionNotificationBus.h>
#include <AzFramework/Physics/TriggerBus.h>
#include <PhysX/PhysXLocks.h>


namespace PhysX
{
    /*static*/ thread_local AZStd::vector<physx::PxRaycastHit>   World::s_raycastBuffer;
    /*static*/ thread_local AZStd::vector<physx::PxSweepHit>     World::s_sweepBuffer;
    /*static*/ thread_local AZStd::vector<physx::PxOverlapHit>   World::s_overlapBuffer;

    // Helper function to convert AZ hit type to PhysX one.
    static physx::PxQueryHitType::Enum GetPxHitType(Physics::QueryHitType hitType)
    {
        static_assert(static_cast<int>(Physics::QueryHitType::None) == static_cast<int>(physx::PxQueryHitType::eNONE) &&
            static_cast<int>(Physics::QueryHitType::Touch) == static_cast<int>(physx::PxQueryHitType::eTOUCH) &&
            static_cast<int>(Physics::QueryHitType::Block) == static_cast<int>(physx::PxQueryHitType::eBLOCK),
            "PhysX hit types do not match QueryHitTypes");
        return static_cast<physx::PxQueryHitType::Enum>(hitType);
    }

    //Helper class, responsible for filtering invalid collision candidates prior to more expensive narrow phase checks
    class PhysXQueryFilterCallback
        : public physx::PxQueryFilterCallback
    {
    public:
        explicit PhysXQueryFilterCallback(const Physics::CollisionGroup& collisionGroup, Physics::FilterCallback filterCallback, physx::PxQueryHitType::Enum hitType)
            : m_filterCallback(AZStd::move(filterCallback))
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
                    if (userData != nullptr && userData->GetEntityId().IsValid())
                    {
                        return GetPxHitType(m_filterCallback(userData->GetWorldBody(), shape));
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
        Physics::FilterCallback m_filterCallback;
        Physics::CollisionGroup m_collisionGroup;
        physx::PxQueryHitType::Enum m_hitType;
    };

    World::World(AZ::Crc32 id, const Physics::WorldConfiguration& settings)
        : m_worldId(id)
        , m_maxDeltaTime(settings.m_maxTimeStep)
        , m_fixedDeltaTime(settings.m_fixedTimeStep)
        , m_maxRaycastBufferSize(settings.m_raycastBufferSize)
        , m_maxSweepBufferSize(settings.m_sweepBufferSize)
        , m_maxOverlapBufferSize(settings.m_overlapBufferSize)
    {
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
        
        if (settings.m_kinematicFiltering)
        {
            sceneDesc.kineKineFilteringMode = physx::PxPairFilteringMode::eKEEP;
        }

        if (settings.m_kinematicStaticFiltering)
        {
            sceneDesc.staticKineFilteringMode = physx::PxPairFilteringMode::eKEEP;
        }

        sceneDesc.filterCallback = this;
#ifdef ENABLE_TGS_SOLVER
        // Use Temporal Gauss-Seidel solver by default
        sceneDesc.solverType = physx::PxSolverType::eTGS;
#endif
        SystemRequestsBus::BroadcastResult(m_world, &SystemRequests::CreateScene, sceneDesc);
        m_world->userData = this;

        physx::PxPvdSceneClient* pvdClient = m_world->getScenePvdClient();
        if (pvdClient)
        {
            pvdClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
            pvdClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
            pvdClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
        }

        World::s_raycastBuffer = {};
        World::s_sweepBuffer   = {};
        World::s_overlapBuffer = {};

        Physics::SystemNotificationBus::Broadcast(&Physics::SystemNotificationBus::Events::OnWorldCreated, this);
    }

    World::~World()
    {
        Physics::WorldRequestBus::Handler::BusDisconnect();
        m_deferredDeletions.clear();
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
            PHYSX_SCENE_READ_LOCK(pxHit.actor->getScene());
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

    // Helper function to make the filter callback always return Block unless the result is None. 
    // This is needed for queries where we only need the single closest result.
    static Physics::FilterCallback GetBlockFilterCallback(const Physics::FilterCallback& filterCallback)
    {
        if (!filterCallback)
        {
            return nullptr;
        }

        return [filterCallback](const Physics::WorldBody* body, const Physics::Shape* shape)
        {
            if (filterCallback(body, shape) != Physics::QueryHitType::None)
            {
                return Physics::QueryHitType::Block;
            }
            else
            {
                return Physics::QueryHitType::None;
            }
        };
    }

    // Helper function to convert the Overlap Filter Callback returning bool to a standard Filter Callback returning QueryHitType
    static Physics::FilterCallback GetFilterCallbackFromOverlap(const Physics::OverlapFilterCallback& overlapFilterCallback)
    {
        if (!overlapFilterCallback)
        {
            return nullptr;
        }

        return [overlapFilterCallback](const Physics::WorldBody* body, const Physics::Shape* shape)
        {
            if (overlapFilterCallback(body, shape))
            {
                return Physics::QueryHitType::Touch;
            }
            else
            {
                return Physics::QueryHitType::None;
            }
        };
    }

    Physics::RayCastHit World::RayCast(const Physics::RayCastRequest& request)
    {
        const auto orig = PxMathConvert(request.m_start);
        const auto dir = PxMathConvert(request.m_direction);
        
        // Query flags. 
        // Note: we specify eBLOCK here as we're only interested in the closest object. The touches field in the result will be invalid
        const physx::PxQueryFlags queryFlags = GetPxQueryFlags(request.m_queryType);
        const physx::PxQueryFilterData queryData(queryFlags);
        const physx::PxHitFlags hitFlags = Utils::RayCast::GetPxHitFlags(request.m_hitFlags);
        PhysXQueryFilterCallback queryFilterCallback(request.m_collisionGroup,
            GetBlockFilterCallback(request.m_filterCallback), physx::PxQueryHitType::eBLOCK);
        
        // Raycast
        physx::PxRaycastBuffer castResult;
        bool status = false;
        {
            PHYSX_SCENE_READ_LOCK(*m_world);
            status = m_world->raycast(orig, dir, request.m_distance, castResult, hitFlags, queryData, &queryFilterCallback);
        }

        // Convert to generic API
        Physics::RayCastHit hit;
        if (status)
        {
            hit = Utils::RayCast::GetHitFromPxHit(castResult.block);
        }
        return hit;
    }

    AZStd::vector<Physics::RayCastHit> World::RayCastMultiple(const Physics::RayCastRequest& request)
    {
        const auto orig = PxMathConvert(request.m_start);
        const auto dir = PxMathConvert(request.m_direction);

        // Query flags. 
        // Note: we specify eTOUCH here as we're interested in all hits that intersect the ray.
        const physx::PxQueryFlags queryFlags = GetPxQueryFlags(request.m_queryType);
        const physx::PxQueryFilterData queryData(queryFlags);
        const physx::PxHitFlags hitFlags = Utils::RayCast::GetPxHitFlags(request.m_hitFlags);

        PhysXQueryFilterCallback queryFilterCallback(request.m_collisionGroup, request.m_filterCallback, physx::PxQueryHitType::eTOUCH);

        //resize if needed
        const AZ::u64 maxResults = AZ::GetMin(m_maxRaycastBufferSize, request.m_maxResults);
        AZ_Warning("World", request.m_maxResults == maxResults, "Raycast request exceeded maximum set in PhysX Configuration. Max[%u] Requested[%u]", m_maxRaycastBufferSize, request.m_maxResults);
        if (s_raycastBuffer.size() < maxResults)
        {
            s_raycastBuffer.resize(maxResults);
        }
        // Raycast
        physx::PxRaycastBuffer castResult(s_raycastBuffer.begin(), aznumeric_cast<physx::PxU32>(request.m_maxResults));
        bool status = false;
        {
            PHYSX_SCENE_READ_LOCK(*m_world);
            status = m_world->raycast(orig, dir, request.m_distance, castResult, hitFlags, queryData, &queryFilterCallback);
        }

        // Convert to generic API
        AZStd::vector<Physics::RayCastHit> hits;
        if (status)
        {
            PHYSX_SCENE_READ_LOCK(*m_world);
            if (castResult.hasBlock)
            {
                hits.push_back(Utils::RayCast::GetHitFromPxHit(castResult.block));
            }

            for (auto i = 0u; i < castResult.getNbTouches(); ++i)
            {
                const auto& pxHit = castResult.getTouch(i);
                hits.push_back(Utils::RayCast::GetHitFromPxHit(pxHit));
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
        const physx::PxHitFlags hitFlags = Utils::RayCast::GetPxHitFlags(request.m_hitFlags);
        PhysXQueryFilterCallback queryFilterCallback(request.m_collisionGroup,
            GetBlockFilterCallback(request.m_filterCallback), physx::PxQueryHitType::eBLOCK);

        physx::PxGeometryHolder pxGeometry;
        Utils::CreatePxGeometryFromConfig(*request.m_shapeConfiguration, pxGeometry);

        Physics::RayCastHit hit;
        if (pxGeometry.any().getType() == physx::PxGeometryType::eSPHERE ||
            pxGeometry.any().getType() == physx::PxGeometryType::eBOX ||
            pxGeometry.any().getType() == physx::PxGeometryType::eCAPSULE ||
            pxGeometry.any().getType() == physx::PxGeometryType::eCONVEXMESH)
        {
            // Buffer to store results in.
            physx::PxSweepBuffer pxResult;
            bool status = false;
            {
                PHYSX_SCENE_READ_LOCK(*m_world);
                status = m_world->sweep(pxGeometry.any(), pose, dir, request.m_distance, pxResult, hitFlags, queryData, &queryFilterCallback);
            }
            if (status)
            {
                hit = Utils::RayCast::GetHitFromPxHit(pxResult.block);
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
        const physx::PxHitFlags hitFlags = Utils::RayCast::GetPxHitFlags(request.m_hitFlags);
        PhysXQueryFilterCallback queryFilterCallback(request.m_collisionGroup, request.m_filterCallback, physx::PxQueryHitType::eTOUCH);

        physx::PxGeometryHolder pxGeometry;
        Utils::CreatePxGeometryFromConfig(*request.m_shapeConfiguration, pxGeometry);

        AZStd::vector<Physics::RayCastHit> hits;
        if (pxGeometry.any().getType() == physx::PxGeometryType::eSPHERE ||
            pxGeometry.any().getType() == physx::PxGeometryType::eBOX ||
            pxGeometry.any().getType() == physx::PxGeometryType::eCAPSULE ||
            pxGeometry.any().getType() == physx::PxGeometryType::eCONVEXMESH)
        {
            //resize if needed
            const AZ::u64 maxResults = AZ::GetMin(m_maxSweepBufferSize, request.m_maxResults);
            AZ_Warning("World", request.m_maxResults == maxResults, "Shape cast request exceeded maximum set in PhysX Configuration. Max[%u] Requested[%u]", m_maxSweepBufferSize, request.m_maxResults);
            if (s_sweepBuffer.size() < maxResults)
            {
                s_sweepBuffer.resize(maxResults);
            }

            // Buffer to store results
            physx::PxSweepBuffer pxResult(s_sweepBuffer.begin(), aznumeric_cast<physx::PxU32>(request.m_maxResults));

            bool status = false;
            {
                PHYSX_SCENE_READ_LOCK(*m_world);
                status = m_world->sweep(pxGeometry.any(), pose, dir, request.m_distance, pxResult, hitFlags, queryData, &queryFilterCallback);
            }
            if (status)
            {
                if (pxResult.hasBlock)
                {
                    hits.push_back(Utils::RayCast::GetHitFromPxHit(pxResult.block));
                }

                for (auto i = 0u; i < pxResult.getNbTouches(); ++i)
                {
                    const auto& pxHit = pxResult.getTouch(i);
                    hits.push_back(Utils::RayCast::GetHitFromPxHit(pxHit));
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
        PhysXQueryFilterCallback filterCallback(request.m_collisionGroup, GetFilterCallbackFromOverlap(request.m_filterCallback), physx::PxQueryHitType::eTOUCH);

        //resize if needed
        const AZ::u64 maxResults = AZ::GetMin(m_maxOverlapBufferSize, request.m_maxResults);
        AZ_Warning("World", request.m_maxResults == maxResults, "Overlap request exceeded maximum set in PhysX Configuration. Max[%u] Requested[%u]", m_maxOverlapBufferSize, request.m_maxResults);
        if (s_overlapBuffer.size() < maxResults)
        {
            s_overlapBuffer.resize(maxResults);
        }
        // Buffer to store results
        physx::PxOverlapBuffer queryHits(s_overlapBuffer.begin(), aznumeric_cast<physx::PxU32>(request.m_maxResults));
        bool status = false;
        {
            PHYSX_SCENE_READ_LOCK(*m_world);
            status = m_world->overlap(pxGeometry.any(), pose, queryHits, defaultFilterData, &filterCallback);
        }
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

    physx::PxActor* GetPxActor(const Physics::WorldBody& worldBody)
    {
        if (worldBody.GetNativeType() != NativeTypeIdentifiers::RigidBody &&
            worldBody.GetNativeType() != NativeTypeIdentifiers::RigidBodyStatic)
        {
            return nullptr;
        }

        return static_cast<physx::PxActor*>(worldBody.GetNativePointer());
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

    void World::RegisterSuppressedCollision(const Physics::WorldBody& body0,
        const Physics::WorldBody& body1)
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

    void World::UnregisterSuppressedCollision(const Physics::WorldBody& body0,
        const Physics::WorldBody& body1)
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
        body.AddToWorld(*this);
    }

    void World::RemoveBody(Physics::WorldBody& body)
    {
        body.RemoveFromWorld(*this);
    }

    void World::SetSimFunc(std::function<void(void*)> func)
    {
        m_simFunc = func;
    }

    void World::StartSimulation(float deltaTime)
    {
        AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::Physics, "World::StartSimulation");

        {
            AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::Physics, "OnPrePhysicsUpdate");
            Physics::WorldNotificationBus::Event(m_worldId, &Physics::WorldNotifications::OnPrePhysicsUpdate, deltaTime);
        }

        {
            AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::Physics, "PhysX::Collide");

            PHYSX_SCENE_WRITE_LOCK(*m_world);

            // Performs collision detection for the scene
            m_world->collide(deltaTime);
        }

        m_currentDeltaTime = deltaTime;
    }

    void World::FinishSimulation()
    {
        AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::Physics, "World::FinishSimulation");

        bool activeActorsEnabled = false;

        {
            PHYSX_SCENE_WRITE_LOCK(*m_world);

            activeActorsEnabled = m_world->getFlags() & physx::PxSceneFlag::eENABLE_ACTIVE_ACTORS;

            {
                AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::Physics, "PhysX::FetchCollision");

                // Wait for the collision phase to finish.
                m_world->fetchCollision(true);
            }

            {
                AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::Physics, "PhysX::Advance");

                // Performs dynamics phase of the simulation pipeline.
                m_world->advance();
            }
        }

        {
            AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::Physics, "PhysX::CheckResults");

            // Wait for the simulation to complete.
            // In the multithreaded environment we need to make sure we don't lock the scene for write here.
            // This is because contact modification callbacks can be issued from the job threads and cause deadlock
            // due to the callback code locking the scene.
            // https://devtalk.nvidia.com/default/topic/1024408/pxcontactmodifycallback-and-pxscene-locking/
            m_world->checkResults(true);
        }

        {
            AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::Physics, "PhysX::FetchResults");
            PHYSX_SCENE_WRITE_LOCK(*m_world);

            // Swap the buffers, invoke callbacks, build the list of active actors.
            m_world->fetchResults(true);
        }

        {
            AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::Physics, "PhysX::ExecuteCollisionNotifications");
            Physics::CollisionNotificationBus::ExecuteQueuedEvents();
        }

        {
            AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::Physics, "PhysX::ExecuteTriggerNotifications");
            Physics::TriggerNotificationBus::ExecuteQueuedEvents();
        }

        if (activeActorsEnabled && m_simFunc)
        {
            AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::Physics, "PhysX::ActiveActors");

            PHYSX_SCENE_READ_LOCK(*m_world);

            physx::PxU32 numActiveActors = 0;
            physx::PxActor** activeActors = m_world->getActiveActors(numActiveActors);

            for (physx::PxU32 i = 0; i < numActiveActors; ++i)
            {
                m_simFunc(activeActors[i]);
            }
        }

        {
            AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::Physics, "PhysX::OnPostPhysicsUpdate");
            Physics::WorldNotificationBus::Event(m_worldId, &Physics::WorldNotifications::OnPostPhysicsUpdate, m_currentDeltaTime);
        }

        m_deferredDeletions.clear();
    }

    void World::Update(float deltaTime)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Physics);

        auto simulateFetch = [this](float simDeltaTime)
        {
            StartSimulation(simDeltaTime);
            FinishSimulation();
        };

        deltaTime = AZ::GetClamp(deltaTime, 0.0f, m_maxDeltaTime);

        if (m_fixedDeltaTime != 0.0f)
        {
            m_accumulatedTime += deltaTime;

            while (m_accumulatedTime >= m_fixedDeltaTime)
            {

                simulateFetch(m_fixedDeltaTime);
                m_accumulatedTime -= m_fixedDeltaTime;

            }
        }
        else
        {
            simulateFetch(deltaTime);
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
        PHYSX_SCENE_WRITE_LOCK(*m_world);
         m_eventHandler = eventHandler;
         if (m_eventHandler == nullptr && m_triggerCallback == nullptr)
         {
             m_world->setSimulationEventCallback(nullptr);
         }
         else if (m_triggerCallback == nullptr)
         {
             m_world->setSimulationEventCallback(this);
         }
    }

    void World::SetTriggerEventCallback(Physics::ITriggerEventCallback* callback)
    {
        PHYSX_SCENE_WRITE_LOCK(*m_world);
        m_triggerCallback = static_cast<IPhysxTriggerEventCallback*>(callback);
        if (m_triggerCallback == nullptr && m_eventHandler == nullptr )
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
        AZ_Assert( (m_eventHandler != nullptr) || (m_triggerCallback != nullptr), "Invalid event handlers");

        for (physx::PxU32 i = 0; i < count; ++i)
        {
            physx::PxTriggerPair& triggerPair = pairs[i];

            if (triggerPair.triggerActor->userData && triggerPair.otherActor->userData)
            {
                if (m_triggerCallback && m_triggerCallback->OnTriggerCallback(&triggerPair))
                {
                    continue;
                }

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
            PHYSX_SCENE_READ_LOCK(*m_world);
            return PxMathConvert(m_world->getGravity());
        }
        return AZ::Vector3::CreateZero();
    }

    void World::SetGravity(const AZ::Vector3& gravity) 
    {
        if (m_world)
        {
            PHYSX_SCENE_WRITE_LOCK(*m_world);
            m_world->setGravity(PxMathConvert(gravity));
        }
    }

    void World::SetMaxDeltaTime(float maxDeltaTime)
    {
        m_maxDeltaTime = maxDeltaTime;
    }


    void World::SetFixedDeltaTime(float fixedDeltaTime)
    {
        m_fixedDeltaTime = fixedDeltaTime;
    }

    void World::DeferDelete(AZStd::unique_ptr<Physics::WorldBody> worldBody)
    {
        m_deferredDeletions.push_back(AZStd::move(worldBody));
    }
} // namespace PhysX
