
#include <AzFramework/Physics/CollisionFilter.h>

namespace Physics
{
    ObjectCollisionFilter::ObjectCollisionFilter()
        : m_filter(0)
    {
    }

    ObjectCollisionFilter::ObjectCollisionFilter(const ObjectCollisionFilter& rhs)
        : m_filter(rhs.m_filter)
    {
    }

    ObjectCollisionFilter::ObjectCollisionFilter(AZ::u64 filter)
        : m_filter(filter)
    {
    }

    ObjectCollisionFilter::ObjectCollisionFilter(void* filter)
    {
        *this = ObjectCollisionFilter(reinterpret_cast<AZ::u64>(filter));
    }

    ObjectCollisionFilter::ObjectCollisionFilter(AZ::u32 layerIndex, AZ::u32 groupMask /*= 0*/)
    {
        SetLayer(layerIndex);
        SetGroupMask(groupMask);
    }

    ObjectCollisionFilter::ObjectCollisionFilter(StandardObjectLayers layer, AZ::u32 groupMask /*= 0*/)
    {
        SetLayer(layer);
        SetGroupMask(groupMask);
    }

    void ObjectCollisionFilter::SetLayer(AZ::u32 layerIndex)
    {
        AZ_Error("CollisionFilter", layerIndex < kMaxObjectFilterLayers, "Layer index must be < 32.");
        Clear();
        m_filter |= static_cast<AZ::u64>(GetObjectLayerBit(layerIndex));
    }

    void ObjectCollisionFilter::ClearLayer(AZ::u32 layerIndex)
    {
        AZ_Error("CollisionFilter", layerIndex < kMaxObjectFilterLayers, "Layer index must be < 32.");
        m_filter &= ~static_cast<AZ::u64>(GetObjectLayerBit(layerIndex));
    }

    void ObjectCollisionFilter::SetLayer(StandardObjectLayers layer)
    {
        return SetLayer(static_cast<AZ::u32>(layer));
    }

    void ObjectCollisionFilter::ClearLayer(StandardObjectLayers layer)
    {
        return ClearLayer(static_cast<AZ::u32>(layer));
    }

    void ObjectCollisionFilter::Clear()
    {
        m_filter = 0;
    }

    StandardObjectLayers ObjectCollisionFilter::GetLayer() const
    {
        return static_cast<StandardObjectLayers>(m_filter & kLayerSegmentMask);
    }
    AZ::u32 ObjectCollisionFilter::GetGroupMask() const
    {
        return static_cast<AZ::u32>(m_filter >> 32);
    }

    void ObjectCollisionFilter::SetGroupMask(AZ::u32 groupMask)
    {
        // Clear group half and set.
        m_filter &= kLayerSegmentMask;
        m_filter |= (static_cast<AZ::u64>(groupMask) << 32);
    }

    void* ObjectCollisionFilter::GetRawValue() const
    {
        return reinterpret_cast<void*>(m_filter);
    }

    WorldCollisionFilter::WorldCollisionFilter()
    {
        ClearAllCollisions();

        EnableCollision(StandardObjectLayers::Static, StandardObjectLayers::Dynamic);
        EnableCollision(StandardObjectLayers::Static, StandardObjectLayers::Debris);
        EnableCollision(StandardObjectLayers::Static, StandardObjectLayers::Ragdoll);
        EnableCollision(StandardObjectLayers::Static, StandardObjectLayers::Vehicle);
        EnableCollision(StandardObjectLayers::Static, StandardObjectLayers::Particle);
        EnableCollision(StandardObjectLayers::Static, StandardObjectLayers::Character);
        EnableCollision(StandardObjectLayers::Terrain, StandardObjectLayers::Dynamic);
        EnableCollision(StandardObjectLayers::Terrain, StandardObjectLayers::Debris);
        EnableCollision(StandardObjectLayers::Terrain, StandardObjectLayers::Ragdoll);
        EnableCollision(StandardObjectLayers::Terrain, StandardObjectLayers::Vehicle);
        EnableCollision(StandardObjectLayers::Terrain, StandardObjectLayers::Particle);
        EnableCollision(StandardObjectLayers::Terrain, StandardObjectLayers::Character);
        EnableCollision(StandardObjectLayers::Dynamic, StandardObjectLayers::Dynamic);
        EnableCollision(StandardObjectLayers::Dynamic, StandardObjectLayers::Debris);
        EnableCollision(StandardObjectLayers::Dynamic, StandardObjectLayers::Ragdoll);
        EnableCollision(StandardObjectLayers::Dynamic, StandardObjectLayers::Vehicle);
        EnableCollision(StandardObjectLayers::Dynamic, StandardObjectLayers::Particle);
        EnableCollision(StandardObjectLayers::Dynamic, StandardObjectLayers::Water);
        EnableCollision(StandardObjectLayers::Dynamic, StandardObjectLayers::Character);
        EnableCollision(StandardObjectLayers::Ragdoll, StandardObjectLayers::Debris);
        EnableCollision(StandardObjectLayers::Ragdoll, StandardObjectLayers::Vehicle);
        EnableCollision(StandardObjectLayers::Ragdoll, StandardObjectLayers::Water);
        EnableCollision(StandardObjectLayers::Ragdoll, StandardObjectLayers::Character);
        EnableCollision(StandardObjectLayers::Debris, StandardObjectLayers::Vehicle);
        EnableCollision(StandardObjectLayers::Debris, StandardObjectLayers::Water);
        EnableCollision(StandardObjectLayers::Debris, StandardObjectLayers::Character);
        EnableCollision(StandardObjectLayers::Vehicle, StandardObjectLayers::Water);
        EnableCollision(StandardObjectLayers::Vehicle, StandardObjectLayers::Particle);
        EnableCollision(StandardObjectLayers::Vehicle, StandardObjectLayers::Character);
        EnableCollision(StandardObjectLayers::Particle, StandardObjectLayers::Water);
        EnableCollision(StandardObjectLayers::Character, StandardObjectLayers::Character);
    }

    void WorldCollisionFilter::ClearAllCollisions()
    {
        for (size_t i = 0; i < kMaxObjectFilterLayers; ++i)
        {
            m_enabledCollisionMask[i] = 0;
        }
    }

    void WorldCollisionFilter::EnableCollision(AZ::u32 layerIndexA, AZ::u32 layerIndexB)
    {
        AZ_Assert(layerIndexA < kMaxObjectFilterLayers, "Layer index is out of range: %u", layerIndexA);
        AZ_Assert(layerIndexB < kMaxObjectFilterLayers, "Layer index is out of range: %u", layerIndexB);
        AZ::u32 bitA = GetObjectLayerBit(layerIndexA);
        AZ::u32 bitB = GetObjectLayerBit(layerIndexB);
        m_enabledCollisionMask[layerIndexA] |= bitB;
        m_enabledCollisionMask[layerIndexB] |= bitA;
    }

    void WorldCollisionFilter::EnableCollision(StandardObjectLayers layerA, StandardObjectLayers layerB)
    {
        return EnableCollision(static_cast<AZ::u32>(layerA), static_cast<AZ::u32>(layerB));
    }

    void WorldCollisionFilter::DisableCollision(AZ::u32 layerIndexA, AZ::u32 layerIndexB)
    {
        AZ_Assert(layerIndexA < kMaxObjectFilterLayers, "Layer index is out of range: %u", layerIndexA);
        AZ_Assert(layerIndexB < kMaxObjectFilterLayers, "Layer index is out of range: %u", layerIndexB);
        m_enabledCollisionMask[layerIndexA] &= ~GetObjectLayerBit(layerIndexB);
        m_enabledCollisionMask[layerIndexB] &= ~GetObjectLayerBit(layerIndexA);
    }

    void WorldCollisionFilter::DisableCollision(StandardObjectLayers layerA, StandardObjectLayers layerB)
    {
        return DisableCollision(static_cast<AZ::u32>(layerA), static_cast<AZ::u32>(layerB));
    }

    bool WorldCollisionFilter::IsCollisionEnabled(void* filterA, void* filterB) const
    {
        return TestCollision(ObjectCollisionFilter(filterA), ObjectCollisionFilter(filterB));
    }

    bool WorldCollisionFilter::TestCollision(const ObjectCollisionFilter& filterA, const ObjectCollisionFilter& filterB) const
    {
        AZ::u32 groupMaskLhs = filterA.GetGroupMask();
        AZ::u32 groupMaskRhs = filterB.GetGroupMask();

        // Same groups - don't self-collide.
        if (groupMaskLhs && (groupMaskLhs == groupMaskRhs))
        {
            return false;
        }

        AZ::u32 layerIndexLhs = GetObjectLayerIndex(filterA.GetLayer());
        AZ::u32 layerIndexRhsBit = static_cast<AZ::u32>(filterB.GetLayer());
        AZ::u32 mask = m_enabledCollisionMask[layerIndexLhs];
        if (!!(mask & layerIndexRhsBit))
        {
            return true;
        }

        if (filterA.GetLayer() == StandardObjectLayers::QueryOnly)
        {
            // Groupmask is a layer mask for Query filters.
            return !!(filterA.GetGroupMask() & GetObjectLayerBit(filterB.GetLayer()));
        }

        if (filterB.GetLayer() == StandardObjectLayers::QueryOnly)
        {
            // Groupmask is a layer mask for Query filters.
            return !!(filterB.GetGroupMask() & GetObjectLayerBit(filterA.GetLayer()));
        }

        return false;
    }

} // namespace Physics
