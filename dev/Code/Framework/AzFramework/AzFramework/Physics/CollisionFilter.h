
#pragma once

#include <AzCore/base.h>

#include <AzFramework/Physics/Base.h>

namespace Physics
{
    enum Constants : AZ::u64
    {
        kMaxObjectFilterLayers  = 32,                                       ///< Maximum number of layers. Filter is 64-bit, of which 32-bits is devoted to layers.
        kGroupSegmentMask       = static_cast<AZ::u64>(0xffffffff) << 32,   ///< Upper 32-bit mask, used for group mask half.
        kLayerSegmentMask       = static_cast<AZ::u64>(0xffffffff),         ///< Lower 32-bit mask, used for layer half.
        kAllLayersMask          = static_cast<AZ::u32>(~0),                 ///< Sets all layer bits, for queries against all object layers.
    };

    AZ_STATIC_ASSERT(kMaxObjectFilterLayers <= 32, "Max layer count must remain under 32 bits to fit alongside group mask in a 64-bit value. To increase layers, adjust logic to steal bits from group mask.");

    /**
        * Common standard set of layers. These are logical definitions, and are not required to be used.
        */
    enum class StandardObjectLayers : AZ::u32
    {
        None        = 0,
        QueryOnly,  ///< Special layer designation for queries (raycasts and shapecasts). Groupmask is used as a mask of layers to cast against.

        Dynamic,
        Static,
        Terrain,
        Keyframed,
        Ragdoll,
        Debris,
        Vehicle,
        Water,
        Particle,
        Character,

        UserBegin, // Game-specific layers should start at this value.
                   // e.g.  MyGameLayer1 = StandardObjectLayers::UserBegin,
                   //       MyGameLayer2,
                   //       etc,

        //GAME layers
        CharacterDefensive = UserBegin,     //This doens't hit anything.  Other things hit it.


        // Full range from [0..kMaxObjectFilterLayers-1] is valid.

        Max         = 32,
    };

    inline AZ::u32 GetObjectLayerIndex(StandardObjectLayers layer)
    {
        return static_cast<AZ::u32>(layer) >> 1;
    }

    inline AZ::u32 GetObjectLayerBit(StandardObjectLayers layer)
    {
        return 1 << static_cast<AZ::u32>(layer);
    }

    inline AZ::u32 GetObjectLayerIndex(AZ::u32 layer)
    {
        return layer >> 1;
    }

    inline AZ::u32 GetObjectLayerBit(AZ::u32 layer)
    {
        return 1 << layer;
    }

    /**
     * 
     */
    class ObjectCollisionFilter
    {
    public:

        ObjectCollisionFilter();
        ObjectCollisionFilter(const ObjectCollisionFilter& rhs);

        explicit ObjectCollisionFilter(AZ::u64 filter);
        explicit ObjectCollisionFilter(void* filter);
        explicit ObjectCollisionFilter(AZ::u32 layerIndex, AZ::u32 groupMask = 0);
        explicit ObjectCollisionFilter(StandardObjectLayers layer, AZ::u32 groupMask = 0);

        void SetLayer(AZ::u32 layerIndex);
        void SetLayer(StandardObjectLayers layer);
        void ClearLayer(AZ::u32 layerIndex);
        void ClearLayer(StandardObjectLayers layer);
        void Clear();

        StandardObjectLayers GetLayer() const;
            
        AZ::u32 GetGroupMask() const;
        void SetGroupMask(AZ::u32 groupMask);

        void* GetRawValue() const;

        AZ::u64 m_filter;
    };

    /// Create an object collision filter for a specific layer and optional group mask.
    inline ObjectCollisionFilter CreateObjectFilterLayerAndGroup(StandardObjectLayers layer, AZ::u32 groupMask = 0)
    {
        return ObjectCollisionFilter(layer, groupMask);
    }

    /// Create an object collision filter specifically for a query (ray and shape casts), which will collide against all layers in the specified mask.
    inline ObjectCollisionFilter CreateObjectFilterQuery(AZ::u32 layerMask = kAllLayersMask)
    {
        return ObjectCollisionFilter(StandardObjectLayers::QueryOnly, layerMask);
    }

    /**
     * 
     */
    class WorldCollisionFilter : public ReferenceBase
    {
    public:

        AZ_CLASS_ALLOCATOR(WorldCollisionFilter, AZ::SystemAllocator, 0);
        AZ_RTTI(WorldCollisionFilter, "{A181D92B-992A-4D59-BDED-5C606785D58C}", ReferenceBase);

        WorldCollisionFilter();

        void ClearAllCollisions();

        void EnableCollision(AZ::u32 layerIndexA, AZ::u32 layerIndexB);
        void EnableCollision(StandardObjectLayers layerA, StandardObjectLayers layerB);

        void DisableCollision(AZ::u32 layerIndexA, AZ::u32 layerIndexB);
        void DisableCollision(StandardObjectLayers layerA, StandardObjectLayers layerB);

        bool IsCollisionEnabled(void* filterA, void* filterB) const;

        virtual bool TestCollision(const ObjectCollisionFilter& filterA, const ObjectCollisionFilter& filterB) const;

    protected:

        AZ::u32 m_enabledCollisionMask[kMaxObjectFilterLayers];
    };

} // namespace Physics
