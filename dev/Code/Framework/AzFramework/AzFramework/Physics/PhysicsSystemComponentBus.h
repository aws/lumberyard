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

#include <AzCore/EBus/EBus.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Memory/SystemAllocator.h>

namespace AZ
{
    class Aabb;
}

namespace AzFramework
{
    /// Flags representing the various types of physical entities.
    enum PhysicalEntityTypes
    {
        Static      = 1 << 0,   ///< Static entities
        Dynamic     = 1 << 1,   ///< Rigid bodies and sleeping rigid bodies
        Living      = 1 << 2,   ///< Living entities (characters, etc.)
        Independent = 1 << 3,   ///< Independent entities
        Terrain     = 1 << 4,   ///< Terrain

        All = Static | Dynamic | Living | Independent | Terrain,  ///< Represents all entities
    };

    /// Requests for the physics system.
    class PhysicsSystemRequests
        : public AZ::EBusTraits
    {
    public:
        // EBusTraits
        // singleton pattern
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        virtual ~PhysicsSystemRequests() = default;

        // RayCast

        /// A \ref RayCast hit.
        /// If no hit occurred then IsValid() returns false.
        struct RayCastHit
        {
            AZ_TYPE_INFO(RayCastHit, "{3D8FA68C-A145-44B4-BA18-F3405D83A9DF}");
            AZ_CLASS_ALLOCATOR(RayCastHit, AZ::SystemAllocator, 0);

            /// Returns whether anything was actually hit.
            bool IsValid() const { return m_distance >= 0.f; }

            float m_distance = -1.0f; ///< The distance from \ref begin to the hit
            AZ::Vector3 m_position;  ///< The position of the hit in world space
            AZ::Vector3 m_normal;    ///< The normal of the surface hit
            AZ::EntityId m_entityId; ///< The id of the AZ::Entity hit, or AZ::InvalidEntityId if hit object is not an AZ::Entity
        };

        /// Configuration for performing a \ref RayCast.
        /// Default configuration tells RayCast to report only the first hit.
        struct RayCastConfiguration
        {
            AZ_TYPE_INFO(RayCastConfiguration, "{FC4E13C6-33D7-4015-91C4-ECBE08F7C5BE}");
            AZ_CLASS_ALLOCATOR(RayCastConfiguration, AZ::SystemAllocator, 0);

            /// The origin of the ray.
            AZ::Vector3 m_origin = AZ::Vector3::CreateZero();

            /// The direction of the ray.
            /// This should be a normalized unit vector.
            AZ::Vector3 m_direction = AZ::Vector3::CreateAxisY();

            /// The furthest distance that the ray will search for hits.
            float m_maxDistance = 100.f;

            /// The ray will ignore these specific entities.
            AZStd::vector<AZ::EntityId> m_ignoreEntityIds;

            /// The maximum number of hits to return.
            /// The ray can pass through multiple entities if pierceability is properly configured.
            size_t m_maxHits = 1;

            enum 
            {
                MaxSurfacePierceability = 15,
            };

            /// Physical surfaces have a numeric value for pierceability.
            /// The ray will pierce through surfaces whose pierceability is higher than this.
            /// Surfaces with pierceability less than or equal to this will block the ray.
            int m_piercesSurfacesGreaterThan = MaxSurfacePierceability;

            /// \ref PhysicalEntityTypes that the ray can hit.
            int m_physicalEntityTypes = PhysicalEntityTypes::All;
        };

        /// Results of a \ref RayCast.
        /// A blocking hit is one that stops the ray from searching any further.
        /// A piercing hit is one whose surface pierceability allowed the ray to pass through.
        class RayCastResult
        {
        public:
            AZ_TYPE_INFO(RayCastResult, "{9A32A294-0BC5-4931-B594-9EAAF95A6B78}");
            AZ_CLASS_ALLOCATOR(RayCastResult, AZ::SystemAllocator, 0);

            /// Get total number of blocking and piercing hits.
            inline size_t GetHitCount() const;

            /// Get a blocking or piercing hit.
            /// If a blocking hit exists, it is located at the final index.
            inline const RayCastHit* GetHit(size_t index) const;

            inline bool HasBlockingHit() const;
            inline const RayCastHit* GetBlockingHit() const;

            inline size_t GetPiercingHitCount() const;
            inline const RayCastHit* GetPiercingHit(size_t index) const;

            inline void SetBlockingHit(const RayCastHit& blockingHit);
            inline void AddPiercingHit(const RayCastHit& piercingHit);

        private:
            RayCastHit m_blockingHit;
            AZStd::vector<RayCastHit> m_piercingHits;
        };

        /// Cast a ray, and retrieve all hits.
        /// \param rayCastConfiguration A \ref RayCastConfiguration controlling how the ray is cast.
        /// \return A \ref RayCastResult describing what was hit.
        virtual RayCastResult RayCast(const RayCastConfiguration& rayCastConfiguration) { return RayCastResult(); }

        /// Gather entities within a specified AABB.
        /// \param aabb The world space axis-aligned box in which to gather entities.
        /// \param query The physical entity types to gather (see \ref PhysicalEntityTypes).
        virtual AZStd::vector<AZ::EntityId> GatherPhysicalEntitiesInAABB(const AZ::Aabb& aabb, AZ::u32 query) { return AZStd::vector<AZ::EntityId>(); }

        /// Gather entities within a radius around a specified point.
        /// \param center World-space point around which to gather entities.
        /// \param radius Radius (in meters) around center in which entities will be considered.
        /// \param query The physical entity types to gather (see \ref PhysicalEntityTypes).
        virtual AZStd::vector<AZ::EntityId> GatherPhysicalEntitiesAroundPoint(const AZ::Vector3& center, float radius, AZ::u32 query) { return AZStd::vector<AZ::EntityId>(); }
    };

    using PhysicsSystemRequestBus = AZ::EBus<PhysicsSystemRequests>;

    /// Broadcast physics system events.
    class PhysicsSystemEvents
        : public AZ::EBusTraits
    {
    public:
        virtual ~PhysicsSystemEvents() {}

        virtual void OnPrePhysicsUpdate() {};
        virtual void OnPostPhysicsUpdate() {};
    };

    using PhysicsSystemEventBus = AZ::EBus<PhysicsSystemEvents>;

} // namespace AzFramework

#include "PhysicsSystemComponentBus.inl"
