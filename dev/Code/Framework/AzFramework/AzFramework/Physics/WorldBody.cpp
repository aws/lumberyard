
#include <AzFramework/Physics/World.h>
#include <AzFramework/Physics/WorldBody.h>

namespace Physics
{
    //////////////////////////////////////////////////////////////////////////
    WorldBody::WorldBody()
        : m_world(nullptr)
    {
        m_internalUserData.reset(aznew InternalUserData(this));
    }

    //////////////////////////////////////////////////////////////////////////
    WorldBody::WorldBody(const Ptr<WorldBodySettings>& settings)
        : m_world(nullptr)
    {
        m_internalUserData.reset(aznew InternalUserData(this, settings->m_customUserData));
        SetEntityId(settings->m_entityId);
    }

    //////////////////////////////////////////////////////////////////////////
    WorldBody::~WorldBody()
    {
    }

    //////////////////////////////////////////////////////////////////////////
    Ptr<World> WorldBody::GetWorld() const
    {
        return m_world;
    }

    //////////////////////////////////////////////////////////////////////////
    void WorldBody::SetWorld(const Ptr<World>& world)
    {
        if (m_world == world)
        {
            return;
        }

        if (m_world)
        {
            m_world->RemoveBody(this);
        }

        if (world)
        {
            world->AddBody(this);
        }
    }

    //////////////////////////////////////////////////////////////////////////
    AZ::EntityId WorldBody::GetEntityId() const
    {
        return m_entityId;
    }

    //////////////////////////////////////////////////////////////////////////
    void WorldBody::SetEntityId(AZ::EntityId entityId)
    {
        m_entityId = entityId;
    }

    //////////////////////////////////////////////////////////////////////////
    void WorldBody::SetUserData(void* userData)
    {
        m_internalUserData->m_customUserData = userData;
    }

    //////////////////////////////////////////////////////////////////////////
    void* WorldBody::GetUserData()
    {
        return m_internalUserData->m_customUserData;
    }

    //////////////////////////////////////////////////////////////////////////
    void WorldBody::RegisterCollisionEventHandler(const Ptr<WorldBodyCollisionEventHandler>& handler)
    {
        AZ_Assert(m_collisionEventHandlers.end() == AZStd::find(m_collisionEventHandlers.begin(), m_collisionEventHandlers.end(), handler), "Event handler is already registered.");

        m_collisionEventHandlers.emplace_back(handler);
    }

    //////////////////////////////////////////////////////////////////////////
    void WorldBody::UnregisterCollisionEventHandler(const Ptr<WorldBodyCollisionEventHandler>& handler)
    {
        auto iter = AZStd::find(m_collisionEventHandlers.begin(), m_collisionEventHandlers.end(), handler);
        if (iter != m_collisionEventHandlers.end())
        {
            m_collisionEventHandlers.erase(iter);
        }
    }

    //////////////////////////////////////////////////////////////////////////
    MaterialId WorldBody::GetMaterialIdForShapeHierarchy(const ShapeIdHierarchy& shapeIdHierarchy)
    {
        if (m_shapeHierarchyMaterialMap)
        {
            return m_shapeHierarchyMaterialMap->GetMaterialIdForShapeIdHierarchy(shapeIdHierarchy);
        }

        return kDefaultMaterialId;
    }

    //////////////////////////////////////////////////////////////////////////
    void WorldBody::SetShapeHierarchyMaterialMap(const Ptr<ShapeHierarchyMaterialMap>& map)
    {
        m_shapeHierarchyMaterialMap = map;
    }

    //////////////////////////////////////////////////////////////////////////
    const Ptr<ShapeHierarchyMaterialMap>& WorldBody::GetShapeHierarchyMaterialMap() const
    {
        return m_shapeHierarchyMaterialMap;
    }

} // namespace Physics
