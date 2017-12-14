
#pragma once

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Component/EntityId.h>

#include <AzFramework/Physics/Base.h>
#include <AzFramework/Physics/WorldBody.h>
#include <AzFramework/Physics/Action.h>
#include <AzFramework/Physics/CollisionFilter.h>
#include <AzFramework/Physics/ShapeIdHierarchy.h>

namespace Physics
{
    class RigidBody;
    class WorldBody;
    class ShapeConfiguration;
    class InternalUserData;
    class Action;

    /**
     * 
     */
    struct RayCastRequest
    {
        RayCastRequest() {}

        RayCastRequest(const AZ::Vector3& start, const AZ::Vector3& end)
        {
            m_start = start;
            AZ::Vector3 dir = end - start;
            m_time = dir.NormalizeWithLength();
            m_dir = dir;
        }

        RayCastRequest(const AZ::Vector3& start, AZ::Vector3 dir, float time)
        {
            AZ::VectorFloat len = dir.NormalizeWithLength();
            time *= len;
            m_start = start;
            m_time = time;
            m_dir = dir;
        }

        AZ::Vector3 GetEndpoint() const
        {
            return m_start + m_dir * m_time;
        }

        float                   m_time                  = 1.0f;                             ///< The distance along m_dir direction.
        AZ::Vector3             m_start                 = AZ::Vector3::CreateZero();        ///< World space point where ray starts from.
        AZ::Vector3             m_dir                   = AZ::Vector3::CreateZero();        ///< World space direction (normalized).
        AZ::u32                 m_maxHits               = 1;                                ///< Max hits to be collected.
        ObjectCollisionFilter   m_collisionFilter       = ObjectCollisionFilter();          ///< Collision filter for raycast.
    };

    /**
     * 
     */
    struct RayCastHit
    {
        float                   m_hitTime               = 0.f;                              ///< The distance along the cast at which the hit occurred as given by Dot(m_normal, startPoint) - Dot(m_normal, m_point).
        AZ::Vector3             m_position              = AZ::Vector3::CreateZero();        ///< The position of the hit in world space
        AZ::Vector3             m_normal                = AZ::Vector3::CreateZero();        ///< The normal of the surface hit
        AZ::EntityId            m_entityId;                                                 ///< The id of the AZ::Entity hit, or AZ::InvalidEntityId if hit object is not an AZ::Entity
        Ptr<WorldBody>          m_hitBody;                                                  ///< World body that was hit.
        ShapeIdHierarchy        m_hitShapeIdHierarchy;                                      ///< Shape Id hierarchy of the hit body at the hit location.
    };

    /**
     * 
     */
    struct RayCastResult
    {
        inline operator bool() const { return !m_hits.empty(); }

        /**
         * Convenience function to find the earliest signed hitTime. Does no special handling
         * for the possibility of negative hit times.
         */
        inline size_t FindEarliestHit() const
        {
            size_t minIndex = 0;
            float min = FLT_MAX;

            for (size_t i = 0; i < m_hits.size(); ++i)
            {
                float t = m_hits[i].m_hitTime;
                if (t < min)
                {
                    min = t;
                    minIndex = i;
                }
            }

            return minIndex;
        }

        AZStd::vector<RayCastHit> m_hits;
    };

    /**
     * Shape cast for finding a time of impact along a shape sweep. The default is linear sweeping without any rotational
     * consideration. If \ref m_nonLinear is true a more expensive rotational + translational sweep can be performed.
     */
    struct ShapeCastRequest
    {
        AZ::Transform               m_start;                                                ///< World space start position. Assumes only rotation + translation (no scaling).
        AZ::Transform               m_end;                                                  ///< World space end position. Only translation is considered, unless \ref m_nonLinear is true to also consider rotation.
        Ptr<ShapeConfiguration>     m_shapeConfiguration;                                   ///< Shape information.
        AZ::u32                     m_maxHits               = 1;                            ///< Max hits to be collected.
        ObjectCollisionFilter       m_collisionFilter       = ObjectCollisionFilter();      ///< Collision filter for raycast.
        bool                        m_nonLinear = false;                                    ///< Perform translational + rotational nonlinear sweep if true.
    };

    /**
     * 
     */
    struct ShapeCastHit
    {
        float               m_hitTime                   = 0.f;                              ///< The normalized distance along the cast at which the hit occurred.
        AZ::Vector3         m_position                  = AZ::Vector3::CreateZero();        ///< The position of the hit in world space
        AZ::Vector3         m_normal                    = AZ::Vector3::CreateZero();        ///< The normal of the surface hit
        AZ::EntityId        m_entityId;                                                     ///< The id of the AZ::Entity hit, or AZ::InvalidEntityId if hit object is not an AZ::Entity
        Ptr<WorldBody>      m_hitBody;                                                      ///< World body that was hit.
        ShapeIdHierarchy    m_shapeIdHierarchySelf;                                         ///< Shape Id hierarchy of the casted body at the hit location.
        ShapeIdHierarchy    m_shapeIdHierarchyHit;                                          ///< Shape Id hierarchy of the hit body at the hit location.
    };

    /**
     * 
     */
    struct ShapeCastResult
    {
        inline operator bool() const { return !m_hits.empty(); }

        AZStd::vector<ShapeCastHit> m_hits;
    };

    /**
     * 
     */
    enum class WorldSimulationType : AZ::u8
    {
        Default,
        SingleThreaded,
        MultiThreaded,
    };

    /**
     * 
     */
    class WorldSettings : public ReferenceBase
    {
    public:
        AZ_CLASS_ALLOCATOR(WorldSettings, AZ::SystemAllocator, 0);
        AZ_RTTI(WorldSettings, "{3C87DF50-AD02-4746-B19F-8B7453A86243}", ReferenceBase)

        AZ::Aabb                    m_worldBounds               = AZ::Aabb::CreateFromMinMax(-AZ::Vector3(1000.f, 1000.f, 1000.f), AZ::Vector3(1000.f, 1000.f, 1000.f));
        WorldSimulationType         m_simulationType            = WorldSimulationType::Default;
        bool                        m_updateWorldOnSystemTick   = true;
        float                       m_maxTimeStep               = (1.f / 20.f);
        float                       m_fixedTimeStep             = 0.f;
        AZ::u32                     m_maxItersPerUpdate         = 2;
        AZ::Vector3                 m_gravity                   = AZ::Vector3(0.f, 0.f, -9.81f);
        Ptr<WorldCollisionFilter>   m_collisionFilter           = nullptr;
        void*                       m_customUserData            = nullptr;
    };

    using RayCastResultCallback = AZStd::function<void(const RayCastResult& result)>;
    using ShapeCastResultCallback = AZStd::function<void(const ShapeCastResult& result)>;

    /**
     * 
     */
    class World : public ReferenceBase
    {
    public:
            
        AZ_CLASS_ALLOCATOR(World, AZ::SystemAllocator, 0);
        AZ_RTTI(World, "{61832612-9F5C-4A2E-8E11-00655A6DDDD2}", ReferenceBase);

        World()
        {
            m_internalUserData.reset(aznew InternalUserData(this));
        }

        explicit World(const Ptr<WorldSettings>& settings)
            : m_worldCollisionFilter(settings->m_collisionFilter)
        {
            m_internalUserData.reset(aznew InternalUserData(this, settings->m_customUserData));
        }

        ~World() override
        {
            for (WorldBody* body : m_bodies)
            {
                body->release();
            }

            for (Action* action : m_actions)
            {
                action->release();
            }
        }

        virtual void Update(float deltaTime) = 0;

        virtual void RayCast(const RayCastRequest& request, RayCastResult& result) = 0;

        virtual void ShapeCast(const ShapeCastRequest& request, ShapeCastResult& result) = 0;

        virtual AZ::u32 QueueRayCast(const RayCastRequest& request, const RayCastResultCallback& callback) = 0;
        virtual void CancelQueuedRayCast(AZ::u32 queueHandle) = 0;

        virtual AZ::u32 QueueShapeCast(const ShapeCastRequest& request, const ShapeCastResultCallback& callback) = 0;
        virtual void CancelQueuedShapeCast(AZ::u32 queueHandle) = 0;

        virtual void AddBody(const Ptr<WorldBody>& body)
        {
            if (body)
            {
                if (body->m_world)
                {
                    body->m_world->RemoveBody(body);
                }

                body->add_ref();
                body->m_world = this;

                AZStd::lock_guard<AZStd::mutex> lock(m_worldMutex);
                m_bodies.insert(body.get());
                OnAddBody(body);
            }
        }

        virtual void RemoveBody(const Ptr<WorldBody>& body)
        {
            if (body)
            {
                AZStd::lock_guard<AZStd::mutex> lock(m_worldMutex);
                if (m_bodies.erase(body.get()) > 0)
                {
                    OnRemoveBody(body);
                    body->m_world = nullptr;
                    body->release();
                }
            }
        }

        virtual void AddAction(const Ptr<Action>& action)
        {
            if (action)
            {
                if (action->m_world)
                {
                    action->m_world->RemoveAction(action);
                }

                AZStd::lock_guard<AZStd::mutex> lock(m_worldMutex);
                action->add_ref();
                m_actions.insert(action.get());
                action->m_world = this;
                OnAddAction(action);
            }
        }

        virtual void RemoveAction(const Ptr<Action>& action)
        {
            if (action)
            {
                AZStd::lock_guard<AZStd::mutex> lock(m_worldMutex);
                if (m_actions.erase(action.get()) > 0)
                {
                    OnRemoveAction(action);
                    action->m_world = nullptr;
                    action->release();
                }
            }
        }

        virtual void SetCollisionFilter(const Ptr<WorldCollisionFilter>& collisionFilter)
        {
            AZStd::lock_guard<AZStd::mutex> lock(m_worldMutex);
            m_worldCollisionFilter = collisionFilter;
        }

        virtual Ptr<WorldCollisionFilter>& GetCollisionFilter()
        {
            return m_worldCollisionFilter;
        }

        virtual const Ptr<WorldCollisionFilter>& GetCollisionFilter() const
        {
            return m_worldCollisionFilter;
        }

        AZStd::mutex& GetMutex()
        {
            return m_worldMutex;
        }

        const AZStd::unordered_set<WorldBody*>& GetBodies() const
        {
            AZStd::lock_guard<AZStd::mutex> lock(m_worldMutex);
            return m_bodies;
        }

        virtual AZ::Crc32 GetNativeType() const { return AZ::Crc32(); }
        virtual void* GetNativePointer() const { return nullptr; }

    protected:

        /// Called when a body is added to the world. The underlying implementation should use this hook to do its own registration.
        virtual void OnAddBody(const Ptr<WorldBody>& body) = 0;

        /// Called when a body is removed from the world. The underlying implementation should use this hook to do its own un-registration.
        virtual void OnRemoveBody(const Ptr<WorldBody>& body) = 0;

        /// Called when an action is added to the world. The underlying implementation should use this hook to do its own registration.
        virtual void OnAddAction(const Ptr<Action>& action) = 0;

        /// Called when an action is removed from the world. The underlying implementation should use this hook to do its own un-registration.
        virtual void OnRemoveAction(const Ptr<Action>& action) = 0;

        Ptr<WorldCollisionFilter>               m_worldCollisionFilter;
        AZStd::unique_ptr<InternalUserData>     m_internalUserData;
        mutable AZStd::mutex                    m_worldMutex;

        AZStd::unordered_set<WorldBody*>        m_bodies;
        AZStd::unordered_set<Action*>           m_actions;
    };

} // namespace Physics
