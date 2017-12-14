
#pragma once

#include <AzCore/std/containers/unordered_map.h>

#include <AzFramework/Physics/Base.h>

namespace Physics
{        
    using ShapeId = AZ::u32;

    static const AZ::u32 kMaxShapeHierarcyLevels = 3;

    /**
        * Shapes can have hierarchies of Ids. For example, a compound shape has child shapes. A mesh shape has child faces.
        * A compound shape can contain mesh shapes; an instance where a two-level hierarchy is involved.
        *
        * ShapeIdHierarchy is a simple structure that stores indexes at hierarchy levels, allowing one to resolve collision
        * information against a hierarchy of shapes.
        */
    class ShapeIdHierarchy
    {
    public:

        ShapeIdHierarchy()
            : m_numLevels(0)
        { 
            for (ShapeId i = 0; i < kMaxShapeHierarcyLevels; ++i)
            {
                m_ids[i] = 0;
            }
        }

        AZ_FORCE_INLINE void AddId(ShapeId id)
        {
            AZ_Assert(m_numLevels < kMaxShapeHierarcyLevels, "ShapeIDHierarchy: Too many ID levels, increase s_maxLevels!");
            m_ids[m_numLevels++] = id;
        }

        AZ_FORCE_INLINE void RemoveId()
        {
            AZ_Assert(m_numLevels > 0, "ShapeIDHierarchy: We don't have any ID to remove!");
            --m_numLevels;
        }

        AZ_FORCE_INLINE bool operator!=(const ShapeIdHierarchy& rhs) const
        {
            if (m_numLevels != rhs.m_numLevels)
            {
                return true;
            }

            for (ShapeId i = 0; i < m_numLevels; ++i)
            {
                if (m_ids[i] != rhs.m_ids[i])
                {
                    return true;
                }
            }
            return false;
        }

        AZ_FORCE_INLINE operator size_t() const
        {
            AZStd::size_t hash = static_cast<AZStd::size_t>(m_numLevels);
            for (ShapeId i = 0; i < m_numLevels; ++i)
            {
                AZStd::hash_combine(hash, static_cast<size_t>(m_ids[i]));
            }
            return hash;
        }

        AZ::u32 m_numLevels;                        ///< Number of shape ID levels.
        ShapeId m_ids[kMaxShapeHierarcyLevels];     ///< Shape IDs sorted in levels (0 is first, m_numLevels-1 is last)
    };

    /**
        * Provides necessary information for retrieving physics material information
        * for a given shape within a shape hierarchy level.
        */
    class ShapeIndexMaterialMap : public ReferenceBase
    {
        friend class ReferenceBase;

    public:

        AZ_CLASS_ALLOCATOR(ShapeIndexMaterialMap, AZ::SystemAllocator, 0);

        /// Given a shape index, the corresponding resolved material Id will be returned.
        /// \param shapeIndex index of the shape.
        /// \return material Id that has been set for the specified shape. If none has been specified, kDefualtMaterialId is returned.
        MaterialId GetMaterialIdForShapeIndex(AZ::u32 shapeIndex) const
        {
            if (shapeIndex < m_shapeIndexToMaterialIndex.size())
            {
                char materialIndex = m_shapeIndexToMaterialIndex[shapeIndex];
                AZ_Assert(materialIndex < m_materials.size(), "Invalid material index stored in ShapeIndexMaterialMap.");
                return m_materials[materialIndex];
            }

            return kDefaultMaterialId;
        }

        /// Retrieves the first valid material at this hierarchy level.
        /// \return the first material registered at this hierarchy level. If none are registered, kDefaultMaterialId is returned.
        MaterialId GetFirstMaterialId() const
        {
            if (!m_materials.empty())
            {
                return m_materials.front();
            }

            return kDefaultMaterialId;
        }

        /// Sets the expected number of shapes at this hierarchy level.
        /// This must be called prior to calling SetMaterialForShapeIndex.
        /// \param shapeCount expected shape count.
        void SetShapeCount(AZ::u32 shapeCount)
        {
            m_shapeIndexToMaterialIndex.resize(shapeCount);
        }

        /// Sets the material Id associated with a given shape index.
        /// If the material is not already registered, it will be registered automatically.
        /// \param shapeIndex index of the shape.
        /// \param materialId id of the material the specified shape should reference.
        void SetMaterialForShapeIndex(AZ::u32 shapeIndex, MaterialId materialId)
        {
            AZ_Assert(shapeIndex < m_shapeIndexToMaterialIndex.size(), "Shape index %u is out of range %llu. Call ShapeIndexMaterialMap::SetShapeCount() to reserve shape space.");
            m_shapeIndexToMaterialIndex[shapeIndex] = GetMaterialIndex(materialId);
        }

    private:

        AZ::u8 GetMaterialIndex(const MaterialId& materialId)
        {
            AZ::u8 materialIndex;
            auto materialIter = AZStd::find(m_materials.begin(), m_materials.end(), materialId);
            if (materialIter == m_materials.end())
            {
                AZ_Assert(m_materials.size() < size_t((AZ::u8)~0), "More than 255 materials for a given shape hierarchy level is not supported.");
                materialIndex = static_cast<AZ::u8>(m_materials.size());
                m_materials.push_back(materialId);
            }
            else
            {
                materialIndex = static_cast<AZ::u8>(AZStd::distance(m_materials.begin(), materialIter));
            }
            return materialIndex;
        }

        ~ShapeIndexMaterialMap() = default;

        AZStd::vector<MaterialId>   m_materials;                    ///< Unique materials used at this hierarchy level.
        AZStd::vector<AZ::u8>       m_shapeIndexToMaterialIndex;    ///< Per-shape index into the material array.
    };

    /**
        * Relates shape id hierarchies to material information.
        * This data is shared, and passed along with shape information when creating physics bodies.
        * Collision event handlers can query this structure to retrieve material information for a given
        * shapeIdHierarchy.
        */
    class ShapeHierarchyMaterialMap : public ReferenceBase
    {
        friend class ReferenceBase;

    public:

        AZ_CLASS_ALLOCATOR(ShapeHierarchyMaterialMap, AZ::SystemAllocator, 0);

        /// Retrieves the material Id for the specified shape Id hierarchy.
        /// If no material is registered for this specific Id hierarchy, first valid material at this level is returned.
        /// \param hierarchy shapeId hierarchy
        MaterialId GetMaterialIdForShapeIdHierarchy(const ShapeIdHierarchy& hierarchy) const
        {
            ShapeIdHierarchy lookupHierarchy;

            if (hierarchy.m_numLevels > 0)
            {
                for (AZ::u32 levelIndex = 0; levelIndex < hierarchy.m_numLevels - 1; ++levelIndex)
                {
                    const ShapeId shapeId = hierarchy.m_ids[levelIndex];
                    lookupHierarchy.AddId(shapeId);
                }

                auto iter = m_hierarchyToMaterialMap.find(lookupHierarchy);
                if (iter != m_hierarchyToMaterialMap.end())
                {
                    return iter->second->GetMaterialIdForShapeIndex(hierarchy.m_ids[hierarchy.m_numLevels - 1]);
                }
            }

            return GetFirstMaterialId();
        }

        /// Retrieves the first valid material in the data set.
        /// This is useful when acquiring material information at a top level when material-per-shape is not possible.
        /// \return materialId first valid Id. If no materials are registered, kDefaultMaterialId is returned.
        MaterialId GetFirstMaterialId() const
        {
            if (!m_hierarchyToMaterialMap.empty())
            {
                const Ptr<ShapeIndexMaterialMap>& map = m_hierarchyToMaterialMap.begin()->second;
                if (map)
                {
                    return map->GetFirstMaterialId();
                }
            }

            return kDefaultMaterialId;
        }

        /// Helper for setting a single material Id for this hierarchy level.
        void SetSingleMaterial(MaterialId materialId)
        {
            Ptr<ShapeIndexMaterialMap> defaultMap = MakeMaterialMapForHieararchy(ShapeIdHierarchy());
            defaultMap->SetShapeCount(1);
            defaultMap->SetMaterialForShapeIndex(0, materialId);
        }

        /// Creates if necessary, and returns a shape material map for the specified hierarchy,
        /// so it can be populated.
        /// \return ShapeIndexMaterialMap instance. This is guaranteed to be non-null.
        Ptr<ShapeIndexMaterialMap> MakeMaterialMapForHieararchy(const ShapeIdHierarchy& hierarchy)
        {
            Ptr<ShapeIndexMaterialMap>& materialMap = m_hierarchyToMaterialMap[hierarchy];

            if (!materialMap)
            {
                materialMap = aznew ShapeIndexMaterialMap();
            }

            return materialMap;
        }

    private:

        AZStd::unordered_map<ShapeIdHierarchy, Ptr<ShapeIndexMaterialMap>> m_hierarchyToMaterialMap; ///< Map of specific shape Id hierarchies to shape index material map.
    };

} // namespace Physics
