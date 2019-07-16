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

#include <functional>

#include <AzCore/std/containers/vector.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Component/EntityId.h>

#include <AzFramework/Physics/WorldBody.h>
#include <AzFramework/Physics/Collision.h>
#include <AzFramework/Physics/Casts.h>
#include <AzFramework/Physics/Material.h>


namespace Physics
{
    class RigidBody;
    class WorldBody;
    class WorldEventHandler;
    class ITriggerEventCallback;

    /// Default world configuration.
    class WorldConfiguration
    {
    public:
        AZ_CLASS_ALLOCATOR(WorldConfiguration, AZ::SystemAllocator, 0);
        AZ_RTTI(WorldConfiguration, "{3C87DF50-AD02-4746-B19F-8B7453A86243}")
        static void Reflect(AZ::ReflectContext* context);

        virtual ~WorldConfiguration() = default;

        AZ::Aabb m_worldBounds = AZ::Aabb::CreateFromMinMax(-AZ::Vector3(1000.f, 1000.f, 1000.f), AZ::Vector3(1000.f, 1000.f, 1000.f));
        float m_maxTimeStep = 1.f / 20.f;
        float m_fixedTimeStep = 1 / 60.f;
        AZ::Vector3 m_gravity = AZ::Vector3(0.f, 0.f, -9.81f);
        void* m_customUserData = nullptr;
        AZ::u64 m_raycastBufferSize = 32; ///< Maximum number of hits that will be returned from a raycast
        AZ::u64 m_sweepBufferSize = 32; ///< Maximum number of hits that can be returned from a shapecast
        AZ::u64 m_overlapBufferSize = 32; ///< Maximum number of overlaps that can be returned from an overlap query
        bool m_enableCcd = false; ///< Enables continuous collision detection in the world
        bool m_enableActiveActors = false; ///< Enables pxScene::getActiveActors method
        bool m_enablePcm = true; ///< Enables the persistent contact manifold algorithm to be used as the narrow phase algorithm
        bool m_kinematicFiltering = true; ///< Enables filtering between kinematic/kinematic  objects.
        bool m_kinematicStaticFiltering = true; ///< Enables filtering between kinematic/static objects.

    private:
        static bool VersionConverter(AZ::SerializeContext& context,
            AZ::SerializeContext::DataElementNode& classElement);
    };

    /// Physics world.
    class World
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        typedef AZ::Crc32 BusIdType;

        AZ_CLASS_ALLOCATOR(World, AZ::SystemAllocator, 0);
        AZ_RTTI(World, "{61832612-9F5C-4A2E-8E11-00655A6DDDD2}");

        virtual ~World() = default;

        virtual void Update(float deltaTime) = 0;

        /// Perform a raycast in the world returning the closest object that intersected.
        virtual RayCastHit RayCast(const RayCastRequest& request) = 0;

        /// Perform a raycast in the world returning all objects that intersected. 
        virtual AZStd::vector<Physics::RayCastHit> RayCastMultiple(const RayCastRequest& request) = 0;

        /// Perform a shapecast in the world returning the closest object that intersected.
        virtual RayCastHit ShapeCast(const ShapeCastRequest& request) = 0;

        /// Perform a shapecast in the world returning all objects that intersected.
        virtual AZStd::vector<RayCastHit> ShapeCastMultiple(const ShapeCastRequest& request) = 0;

        /// Perform a spherecast in the world returning the closest object that intersected.
        Physics::RayCastHit SphereCast(float radius, 
            const AZ::Transform& startPose, const AZ::Vector3& direction, float distance, 
            QueryType queryType = QueryType::StaticAndDynamic,
            CollisionGroup collisionGroup = CollisionGroup::All, 
            CustomFilterCallback filterCallback = nullptr);

        /// Perform a spherecast in the world returning all objects that intersected.
        AZStd::vector<Physics::RayCastHit> SphereCastMultiple(float radius,
            const AZ::Transform& startPose, const AZ::Vector3& direction, float distance,
            QueryType queryType = QueryType::StaticAndDynamic,
            CollisionGroup collisionGroup = CollisionGroup::All,
            CustomFilterCallback filterCallback = nullptr);

        /// Perform a boxcast in the world returning the closest object that intersected.
        Physics::RayCastHit BoxCast(const AZ::Vector3& boxDimensions,
            const AZ::Transform& startPose, const AZ::Vector3& direction, float distance,
            QueryType queryType = QueryType::StaticAndDynamic,
            CollisionGroup collisionGroup = CollisionGroup::All,
            CustomFilterCallback filterCallback = nullptr);

        /// Perform a boxcast in the world returning all objects that intersected.
        AZStd::vector<Physics::RayCastHit> BoxCastMultiple(const AZ::Vector3& boxDimensions,
            const AZ::Transform& startPose, const AZ::Vector3& direction, float distance,
            QueryType queryType = QueryType::StaticAndDynamic,
            CollisionGroup collisionGroup = CollisionGroup::All,
            CustomFilterCallback filterCallback = nullptr);

        /// Perform a capsule in the world returning all objects that intersected.
        Physics::RayCastHit CapsuleCast(float capsuleRadius, float capsuleHeight,
            const AZ::Transform& startPose, const AZ::Vector3& direction, float distance,
            QueryType queryType = QueryType::StaticAndDynamic,
            CollisionGroup collisionGroup = CollisionGroup::All,
            CustomFilterCallback filterCallback = nullptr);

        /// Perform a capsule in the world returning all objects that intersected.
        AZStd::vector<Physics::RayCastHit> CapsuleCastMultiple(float capsuleRadius, float capsuleHeight,
            const AZ::Transform& startPose, const AZ::Vector3& direction, float distance,
            QueryType queryType = QueryType::StaticAndDynamic,
            CollisionGroup collisionGroup = CollisionGroup::All,
            CustomFilterCallback filterCallback = nullptr);

        /// Perform an overlap query returning all objects that overlapped.
        virtual AZStd::vector<OverlapHit> Overlap(const OverlapRequest& request) = 0;

        /// Perform an overlap sphere query returning all objects that overlapped.
        AZStd::vector<OverlapHit> OverlapSphere(float radius, const AZ::Transform& pose, CustomFilterCallback customFilterCallback = nullptr);

        /// Perform an overlap box query returning all objects that overlapped.
        AZStd::vector<OverlapHit> OverlapBox(const AZ::Vector3& dimensions, const AZ::Transform& pose, CustomFilterCallback customFilterCallback = nullptr);

        /// Perform an overlap capsule query returning all objects that overlapped.
        AZStd::vector<OverlapHit> OverlapCapsule(float height, float radius, const AZ::Transform& pose, CustomFilterCallback customFilterCallback = nullptr);

        /// Registers a pair of world bodies for which collisions should be suppressed.
        virtual void RegisterSuppressedCollision(const WorldBody& body0, const WorldBody& body1) = 0;

        /// Unregisters a pair of world bodies for which collisions should be suppressed.
        virtual void UnregisterSuppressedCollision(const WorldBody& body0, const WorldBody& body1) = 0;

        virtual void AddBody(WorldBody& body) = 0;
        virtual void RemoveBody(WorldBody& body) = 0;

        virtual AZ::Crc32 GetNativeType() const { return AZ::Crc32(); }
        virtual void* GetNativePointer() const { return nullptr; }

        virtual void SetSimFunc(std::function<void(void*)> func) = 0;

        virtual void SetEventHandler(WorldEventHandler* eventHandler) = 0;

        virtual AZ::Vector3 GetGravity() = 0;
        virtual void SetGravity(const AZ::Vector3& gravity) = 0;

        virtual void DeferDelete(AZStd::unique_ptr<WorldBody> worldBody) = 0;

        /*! @brief Similar to SetEventHandler, relevant for Touch Bending
         *
         *  SetEventHandler is useful to catch onTrigger events when the bodies
         *  involved were created with the standard physics Components attached to
         *  entities. On the other hand, this method was added since Touch Bending, and it is useful
         *  for the touch bending simulator to catch onTrigger events of Actors that
         *  don't have valid AZ:EntityId.
         *
         *  @param triggerCallback Pointer to the callback object that will get the On
         *  @returns Nothing.
         *
         */
        virtual void SetTriggerEventCallback(ITriggerEventCallback* triggerCallback) = 0;
    };

    typedef AZ::EBus<World> WorldRequestBus;
    using WorldRequests = World;

} // namespace Physics
