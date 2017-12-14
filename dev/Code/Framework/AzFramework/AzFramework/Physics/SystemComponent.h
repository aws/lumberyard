
#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <AzFramework/Physics/Material.h>
#include <AzFramework/Asset/AssetCatalogBus.h>

namespace AZ
{
    namespace Data
    {
        class AssetHandler;
    }
}

namespace Physics
{
    class SystemComponent
        : public AZ::Component
        , private MaterialRequestBus::Handler
        , private AzFramework::AssetCatalogEventBus::Handler
        , private AZ::Data::AssetBus::Handler
    {
    public:
        AZ_COMPONENT(SystemComponent, "{9D91F3E4-DE62-48A5-AE98-4F88EDF4D0A3}");

        SystemComponent();
        ~SystemComponent() override;
        
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void Reflect(AZ::ReflectContext* context);

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override {}
        void Activate() override;
        void Deactivate() override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // MaterialRequestBus::Handler
        MaterialId GetMaterialIdByName(const char* materialName) override;
        MaterialId GetMaterialIdByNameCrc(const AZ::Crc32& materialNameCrc) override;
        MaterialProperties* GetPhysicsMaterialProperties(MaterialId materialId) override;
        MaterialProperties* GetDefaultPhysicsMaterialProperties() override;
        MaterialId MapLegacySurfaceTypeToPhysicsMaterial(const AZ::Crc32& surfaceTypeNameCrc) override;
        void GetMaterialNames(AZStd::vector<AZStd::string>& materialNames) override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // AzFramework::AssetCatalogEventBus::Handler
        void OnCatalogAssetChanged(const AZ::Data::AssetId& assetId) override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // AZ::Data::AssetBus::Handler
        void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        //////////////////////////////////////////////////////////////////////////

    protected:
        AZStd::unique_ptr<AZ::Data::AssetHandler>   m_materialSetAssetHandler;
        AZ::Data::Asset<MaterialSetAsset>           m_defaultMaterialSet;
        AZStd::mutex                                m_dataLock;

    private:
        AZ_DISABLE_COPY_MOVE(SystemComponent);
    };

} // namespace Physics
