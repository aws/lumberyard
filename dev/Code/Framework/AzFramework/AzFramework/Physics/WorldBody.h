
#pragma once

#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/Component/Entity.h>

#include <AzFramework/Physics/Base.h>
#include <AzFramework/Physics/ShapeIdHierarchy.h>
#include <AzFramework/Physics/CollisionFilter.h>

namespace Physics
{
    class ObjectCollisionFilter;
    class WorldBody;
    class World;
    struct RayCastRequest;
    struct RayCastResult;

    /**
     * 
     */
    class CollisionContactInfo
    {
    public:
        AZ::Vector3                     m_worldPointA;
        AZ::Vector3                     m_worldPointB;
        AZ::Vector3                     m_normal;
        float                           m_distance;
        ShapeIdHierarchy                m_shapeIdHierarchyA;
        ShapeIdHierarchy                m_shapeIdHierarchyB;
    };

    /**
     * 
     */
    class CollisionEventInfo
    {
    public:
        Ptr<WorldBody> m_bodyA;
        Ptr<WorldBody> m_bodyB;
            
        AZStd::vector<CollisionContactInfo> m_contacts;

        bool m_ignoreCollision = false; ///< Set to ignore the collision.
    };

    /**
     * 
     */
    class WorldBodyCollisionEventHandler : public ReferenceBase
    {
    public:

        friend class WorldBody;

        AZ_CLASS_ALLOCATOR(WorldBodyCollisionEventHandler, AZ::SystemAllocator, 0);

        virtual void OnCollision(const Ptr<WorldBody>& worldBody, CollisionEventInfo& info) = 0;
    };

    class WorldBodySettings : public ReferenceBase
    {
    public:

        AZ_CLASS_ALLOCATOR(WorldBodySettings, AZ::SystemAllocator, 0);
        AZ_RTTI(WorldBodySettings, "{6EEB377C-DC60-4E10-AF12-9626C0763B2D}", ReferenceBase);

        // Basic initial settings.
        AZ::Vector3                     m_position                  = AZ::Vector3::CreateZero();
        AZ::Quaternion                  m_orientation               = AZ::Quaternion::CreateIdentity();
        ObjectCollisionFilter           m_collisionFilter           = ObjectCollisionFilter();

        // Entity/object association.
        AZ::EntityId                    m_entityId;
        void*                           m_customUserData            = nullptr;
    };

    class WorldBody : public ReferenceBase
    {
    public:

        AZ_CLASS_ALLOCATOR(WorldBody, AZ::SystemAllocator, 0);
        AZ_RTTI(WorldBody, "{4F1D9B44-FC21-4E93-83F0-41B6A78D9B4B}", ReferenceBase);

        friend class World;

    public:

        WorldBody();
        WorldBody(const Ptr<WorldBodySettings>& settings);

        ~WorldBody() override;

        Ptr<World> GetWorld() const;

        virtual void SetWorld(const Ptr<World>& world);

        AZ::EntityId GetEntityId() const;
        void SetEntityId(AZ::EntityId entityId);

        void SetUserData(void* userData);
        void* GetUserData();

        const AZStd::unordered_set<WorldBodyCollisionEventHandler*>& GetCollisionEventHandlers() const;

        MaterialId GetMaterialIdForShapeHierarchy(const ShapeIdHierarchy& shapeIdHierarchy);

        void SetShapeHierarchyMaterialMap(const Ptr<ShapeHierarchyMaterialMap>& map);
        const Ptr<ShapeHierarchyMaterialMap>& GetShapeHierarchyMaterialMap() const;

        virtual AZ::Transform GetTransform() const = 0;
        virtual void SetTransform(const AZ::Transform& transform) = 0;
        virtual AZ::Vector3 GetPosition() const = 0;
        virtual AZ::Quaternion GetOrientation() const = 0;

        virtual ObjectCollisionFilter GetCollisionFilter() const = 0;
        virtual void SetCollisionFilter(const ObjectCollisionFilter& filter) = 0;

        virtual AZ::Aabb GetAabb() const = 0;

        virtual void RayCast(const RayCastRequest& request, RayCastResult& result) const = 0;

        virtual void RegisterCollisionEventHandler(const Ptr<WorldBodyCollisionEventHandler>& handler);
        virtual void UnregisterCollisionEventHandler(const Ptr<WorldBodyCollisionEventHandler>& handler);

    protected:

        AZ::EntityId                                        m_entityId;
        World*                                              m_world;
        AZStd::unique_ptr<InternalUserData>                 m_internalUserData;
        Ptr<ShapeHierarchyMaterialMap>                      m_shapeHierarchyMaterialMap;

        AZStd::vector<Ptr<WorldBodyCollisionEventHandler>>  m_collisionEventHandlers;

    private:
        AZ_DISABLE_COPY_MOVE(WorldBody);
    };
    
} // namespace Physics
