
#include <AzCore/IO/FileIO.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

#include <AzFramework/Physics/Material.h>

namespace Physics
{
    //////////////////////////////////////////////////////////////////////////
    class MaterialSetSerializationEvents
        : public AZ::SerializeContext::IEventHandler
    {
        // Serializer has completed writing to the object (after patching data, serializing from disk, etc).
        void OnWriteEnd(void* classPtr) 
        {
            MaterialSet* materialSet = reinterpret_cast<MaterialSet*>(classPtr);

            // Material Ids are indices into m_materials. Ensure we're not growing the material list such that it'd cause material Ids to wrap.
            AZ_Assert(materialSet->m_materials.size() < static_cast<MaterialId>(~0), "MaterialId overflow - too many materials");

            // Build a map of material Id -> entry.
            materialSet->m_materialMap.clear();
            materialSet->m_materialNameMap.clear();
            materialSet->m_surfaceTypeToPhysicsMaterialId.clear();

            MaterialId materialId = 0; // Material Id is an index into m_materials.
            for (MaterialSet::MaterialEntry& material : materialSet->m_materials)
            {
                // Compute material name and surface type Crcs.
                material.m_materialNameCrc = AZ::Crc32(material.m_properties.m_name.c_str());
                material.m_surfaceTypeCrc = AZ::Crc32(material.m_properties.m_surfaceType.c_str());

                // Populate map of game surface type Crcs to material Ids.
                if (material.m_surfaceTypeCrc)
                {
                    materialSet->m_surfaceTypeToPhysicsMaterialId[material.m_surfaceTypeCrc] = materialId;
                }

                // Add to MaterialId->Entry, and name Crc->MaterialId maps.
                materialSet->m_materialMap[materialId] = &material;
                materialSet->m_materialNameMap[material.m_materialNameCrc] = materialId;

                ++materialId;
            }
        }
    };

    //////////////////////////////////////////////////////////////////////////
    EditableCollisionFilter::EditableCollisionFilter()
    {
        for (AZ::u32 i = 0; i < m_groupMask.size(); ++i)
        {
            m_groupMask[i] = false;
        }
    }

    //////////////////////////////////////////////////////////////////////////
    ObjectCollisionFilter EditableCollisionFilter::MakeCollisionFilter() const
    {
        AZ::u32 groupMask = 0;
        for (AZ::u32 i = 0; i < m_groupMask.size(); ++i)
        {
            if (m_groupMask[i])
            {
                groupMask |= (1 << i);
            }
        }

        return ObjectCollisionFilter(m_layerIndex, groupMask);
    }

    //////////////////////////////////////////////////////////////////////////
    void MaterialSet::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<EditableCollisionFilter>()
                ->Version(1)
                ->Field("LayerIndex", &EditableCollisionFilter::m_layerIndex)
                ->Field("GroupMask", &EditableCollisionFilter::m_groupMask)
                ;

            serializeContext->Class<MaterialProperties>()
                ->Version(1)
                ->Field("Name", &MaterialProperties::m_name)
                ->Field("Friction", &MaterialProperties::m_friction)
                ->Field("Restitution", &MaterialProperties::m_restitution)
                ->Field("SurfaceType", &MaterialProperties::m_surfaceType)
                ;

            serializeContext->Class<MaterialEntry>()
                ->Version(1)
                ->Field("Configuration", &MaterialEntry::m_properties)
                ;

            serializeContext->Class<MaterialSet>()
                ->EventHandler<MaterialSetSerializationEvents>()
                ->Version(1)
                ->Field("DefaultMaterial", &MaterialSet::m_defaultMaterial)
                ->Field("Materials", &MaterialSet::m_materials)
                ;

            serializeContext->Class<MaterialSetAsset, MaterialSet>()
                ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<EditableCollisionFilter>("Collision Filter", "Defines object collision filter settings")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditableCollisionFilter::m_layerIndex, "Layer index", "Index of collision layer for the object")
                        ->Attribute(AZ::Edit::Attributes::Min, 0)
                        ->Attribute(AZ::Edit::Attributes::Max, 31)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditableCollisionFilter::m_groupMask, "Group mask", "Defines the object's collision group. Objects will never collide with others with the same group mask")
                    ;

                editContext->Class<MaterialProperties>("", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &MaterialProperties::m_name, "Name", "Material name")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::AttributesAndValues)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &MaterialProperties::m_friction, "Friction", "Friction coefficient")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.f)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &MaterialProperties::m_restitution, "Restitution", "Restitution coefficient")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.f)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &MaterialProperties::m_surfaceType, "Surface type", "Game surface type")
                    ;

                editContext->Class<MaterialEntry>("", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
                        ->Attribute(AZ::Edit::Attributes::NameLabelOverride, &MaterialEntry::GetName)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &MaterialEntry::m_properties, "Material", "Material configuration")
                    ;

                editContext->Class<MaterialSet>("Physics Material Set", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &MaterialSet::m_defaultMaterial, "Default Material", "Default physics material")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &MaterialSet::m_materials, "Materials", "Physics materials")
                    ;

                editContext->Class<MaterialSetAsset>("Physics Material Set", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ;
            }
        }
    }

    //////////////////////////////////////////////////////////////////////////
    MaterialSet::MaterialSet()
    {
    }

    //////////////////////////////////////////////////////////////////////////
    void MaterialSet::AddMaterial(const MaterialProperties& materialProperties)
    {
        // Material Ids are indices into m_materials. Ensure we're not growing the material list such that it'd cause material Ids to wrap. -1 because MaterialId(~0) is reserved for kDefaultMaterialId.
        AZ_Assert(m_materialMap.size() < static_cast<MaterialId>(~0) - 1, "MaterialId overflow - too many materials");

        const AZ::Crc32 materialNameCrc(materialProperties.m_name.c_str());
        const AZ::Crc32 surfaceTypeCrc(materialProperties.m_surfaceType.c_str());

        AZ_Error("Physics Materials", GetMaterialId(materialNameCrc) == kDefaultMaterialId, "Material \"%s\" is already registered.", materialProperties.m_name.c_str());

        const MaterialId materialId = static_cast<MaterialId>(m_materialMap.size());
        m_materials.emplace_back(materialNameCrc, surfaceTypeCrc, materialProperties);
        m_materialMap[materialId] = &m_materials.back();
        m_materialNameMap[materialNameCrc] = materialId;
    }
        
    //////////////////////////////////////////////////////////////////////////
    MaterialProperties* MaterialSet::GetDefaultMaterial()
    {
        return &m_defaultMaterial;
    }

    //////////////////////////////////////////////////////////////////////////
    MaterialId MaterialSet::GetMaterialId(const char* materialName) const
    {
        const AZ::Crc32 materialNameCrc(materialName);
        return GetMaterialId(materialNameCrc);
    }

    //////////////////////////////////////////////////////////////////////////
    MaterialId MaterialSet::GetMaterialId(const AZ::Crc32& materialNameCrc) const
    {
        auto foundIter = m_materialNameMap.find(materialNameCrc);
        if (foundIter != m_materialNameMap.end())
        {
            return foundIter->second;
        }

        return kDefaultMaterialId;
    }

    //////////////////////////////////////////////////////////////////////////
    MaterialProperties* MaterialSet::GetMaterialProperties(MaterialId materialId)
    {
        if (materialId == kDefaultMaterialId)
        {
            return &m_defaultMaterial;
        }

        auto foundIter = m_materialMap.find(materialId);
        if (foundIter != m_materialMap.end())
        {
            return &(foundIter->second->m_properties);
        }

        return &m_defaultMaterial;
    }

    //////////////////////////////////////////////////////////////////////////
    MaterialId MaterialSet::MapLegacySurfaceTypeToPhysicsMaterial(const AZ::Crc32& surfaceTypeCrc) const
    {
        auto iter = m_surfaceTypeToPhysicsMaterialId.find(surfaceTypeCrc);
        if (iter != m_surfaceTypeToPhysicsMaterialId.end())
        {
            return iter->second;
        }

        return kDefaultMaterialId;
    }

    //////////////////////////////////////////////////////////////////////////
    void MaterialSet::GetMaterialNames(AZStd::vector<AZStd::string>& materialNames) const
    {
        materialNames.clear();
        materialNames.reserve(m_materials.size() + 1);

        materialNames.push_back("Default");

        for (const MaterialEntry& material : m_materials)
        {
            materialNames.push_back(material.m_properties.m_name);
        }
    }

    //////////////////////////////////////////////////////////////////////////
    MaterialSetAsset::MaterialSetAsset()
    {
    }

} // namespace Physics
