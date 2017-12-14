
#pragma once

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetTypeInfoBus.h>
#include <AzCore/std/containers/list.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/IO/FileIO.h>
#include <AzFramework/Asset/GenericAssetHandler.h>
#include <AzFramework/Physics/CollisionFilter.h>

namespace AZ
{
    class ReflectContext;
}

namespace Physics
{
    /**
     * Provides a property-grid-editable structure for collision filters.
     */
    struct EditableCollisionFilter
    {
        AZ_TYPE_INFO(EditableCollisionFilter, "{2EC1E83B-7C67-474A-A9B9-CE467AACB2B7}");

        AZ::u32                     m_layerIndex    = 0;
        AZStd::array<bool, 32>      m_groupMask;

        EditableCollisionFilter();

        ObjectCollisionFilter MakeCollisionFilter() const;
    };

    /**
     * Defines material-based physics surface properties.
     */
    struct MaterialProperties
    {
        AZ_TYPE_INFO(MaterialProperties, "{8807CAA1-AD08-4238-8FDB-2154ADD084A1}");

        AZStd::string           m_name;
        AZStd::string           m_surfaceType;
        float                   m_friction = 0.5f;
        float                   m_restitution = 0.5f;
    };

    /**
     * Owns a set of materials, as well as a cache of surface type mappings.
     */
    class MaterialSet
    {
    public:

        friend class MaterialSetSerializationEvents;

        AZ_CLASS_ALLOCATOR(MaterialSet, AZ::SystemAllocator, 0);
        AZ_RTTI(MaterialSet, "{84399E75-18AB-4000-8DCA-07B9D4E0F8E8}");

        MaterialSet();
        virtual ~MaterialSet() {};

        void AddMaterial(const MaterialProperties& materialProperties);

        MaterialProperties* GetDefaultMaterial();

        MaterialId GetMaterialId(const char* materialName) const;
        MaterialId GetMaterialId(const AZ::Crc32& materialNameCrc) const;

        MaterialProperties* GetMaterialProperties(MaterialId materialId);

        MaterialId MapLegacySurfaceTypeToPhysicsMaterial(const AZ::Crc32& surfaceTypeCrc) const;

        void GetMaterialNames(AZStd::vector<AZStd::string>& materialNames) const;

        static void Reflect(AZ::ReflectContext* context);

        struct MaterialEntry
        {
            AZ_TYPE_INFO(MaterialEntry, "{C5207CC2-EF1B-4A11-BC8F-F1898282FBE5}");

            MaterialEntry()
            {}

            MaterialEntry(const AZ::Crc32 materialNameCrc, const AZ::Crc32 surfaceTypeCrc, const MaterialProperties& properties)
                : m_materialNameCrc(materialNameCrc)
                , m_surfaceTypeCrc(surfaceTypeCrc)
                , m_properties(properties)
            {}

            const char* GetName() const { return m_properties.m_name.c_str(); }

            AZ::Crc32               m_materialNameCrc;  // Physics material logical name
            AZ::Crc32               m_surfaceTypeCrc;   // Corresponding game surface type
            MaterialProperties      m_properties;       // Material physics properties
        };

    protected:

        // Reflected/serialized structures.
        MaterialProperties                                  m_defaultMaterial;
        AZStd::list<MaterialEntry>                          m_materials;

        // Populated/optimized post-load.
        AZStd::unordered_map<MaterialId, MaterialEntry*>    m_materialMap;
        AZStd::unordered_map<AZ::Crc32, MaterialId>         m_materialNameMap;
        AZStd::unordered_map<AZ::Crc32, MaterialId>         m_surfaceTypeToPhysicsMaterialId;
    };

    /**
     * Asset-managed material set.
     */
    class MaterialSetAsset
        : public AZ::Data::AssetData
        , public MaterialSet
    {
    public:
        MaterialSetAsset();

        friend class MaterialSetAssetManager;

        AZ_CLASS_ALLOCATOR(MaterialSetAsset, AZ::SystemAllocator, 0);
        AZ_RTTI(MaterialSetAsset, "{9E366D8C-33BB-4825-9A1F-FA3ADBE11D0F}", AZ::Data::AssetData, MaterialSet);
    private:
        AZ_DISABLE_COPY_MOVE(MaterialSetAsset);
    };

    /**
     *
     */
    class MaterialSetAssetHandler
        : public AzFramework::GenericAssetHandler<MaterialSetAsset>
    {
    public:
        AZ_CLASS_ALLOCATOR(MaterialSetAssetHandler, AZ::SystemAllocator, 0);

        MaterialSetAssetHandler()
            : AzFramework::GenericAssetHandler<MaterialSetAsset>("Physics Material Set", "Other", "physicsmaterialset")
        {
        }
    };

    /**
     * Material request bus, serviced by the AzFramework::Physics::SystemComponent.
     * This bus can be used to query for physics material properties by name or material Id.
     */
    class MaterialRequests
        : public AZ::EBusTraits
    {
    public:

        ////////////////////////////////////////////////////////////////////////
        // EBusTraits
        // singleton pattern
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        ////////////////////////////////////////////////////////////////////////

        virtual ~MaterialRequests() = default;

        /// Look up a material Id by name.
        /// \param materialName name of material. If not found, kDefaultMaterialId is returned.
        /// \param material Id. If name was not found, kDefaultMaterialId is returned.
        virtual MaterialId GetMaterialIdByName(const char* materialName) = 0;

        /// Look up a material Id by name Crc.
        /// \param materialNameCrc crc32 of material name.
        /// \param material Id. If name was not found, kDefaultMaterialId is returned.
        virtual MaterialId GetMaterialIdByNameCrc(const AZ::Crc32& materialNameCrc) = 0;

        /// Given a Crc32 of a surface type name, the Id of the material referencing the surface type will be returned.
        /// \param surfaceTypeCrc Crc32 of surface type name.
        /// \return materialId If no material referencing the specified surface type is present, kDefaultMaterialId is returned.
        virtual MaterialId MapLegacySurfaceTypeToPhysicsMaterial(const AZ::Crc32& surfaceTypeCrc) = 0;

        /// Retrieve mutable material properties by material Id.
        /// Material Ids can be retrieved via GetMaterialId().
        /// \param materialId
        /// \return mutable pointer to material properties. Null will be returned if material id was not valid.
        virtual MaterialProperties* GetPhysicsMaterialProperties(MaterialId materialId) = 0;

        /// Retrieve the material set's default material properties.
        /// \return mutable pointer to material properties.
        virtual MaterialProperties* GetDefaultPhysicsMaterialProperties() = 0;

        /// Retrieve list of all registered material names.
        /// \param output vector of material names as AZStd::strings.
        virtual void GetMaterialNames(AZStd::vector<AZStd::string>& materialNames) = 0;
    };

    using MaterialRequestBus = AZ::EBus<MaterialRequests>;
} // namespace Physics
