

#include <AzCore/Asset/AssetManagerBus.h>

#include <AzFramework/Physics/SystemComponent.h>
#include <AzFramework/Physics/Material.h>

namespace Physics
{
    //////////////////////////////////////////////////////////////////////////
    void SystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC("AssetDatabaseService", 0x3abf5601));
    }

    //////////////////////////////////////////////////////////////////////////
    void SystemComponent::Reflect(AZ::ReflectContext* context)
    {
        MaterialSet::Reflect(context);

        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<SystemComponent>()
                ->Version(1)
                ;
        }
    }

    //////////////////////////////////////////////////////////////////////////
    SystemComponent::SystemComponent()
        : m_materialSetAssetHandler(nullptr)
    {
    }

    //////////////////////////////////////////////////////////////////////////
    SystemComponent::~SystemComponent()
    {
    }

    //////////////////////////////////////////////////////////////////////////
    void SystemComponent::Activate()
    {
        MaterialSetAssetHandler* materialAssetHandler = aznew MaterialSetAssetHandler();
        materialAssetHandler->Register();
        m_materialSetAssetHandler.reset(materialAssetHandler);

        AzFramework::AssetCatalogEventBus::Handler::BusConnect();
        MaterialRequestBus::Handler::BusConnect();
    }

    //////////////////////////////////////////////////////////////////////////
    void SystemComponent::Deactivate()
    {
        AzFramework::AssetCatalogEventBus::Handler::BusDisconnect();
        MaterialRequestBus::Handler::BusDisconnect();

        {
            AZStd::lock_guard<AZStd::mutex> lock(m_dataLock);
            m_defaultMaterialSet = AZ::Data::Asset<AZ::Data::AssetData>();
        }

        m_materialSetAssetHandler = nullptr;
    }

    //////////////////////////////////////////////////////////////////////////
    MaterialId SystemComponent::GetMaterialIdByName(const char* materialName)
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_dataLock);

        if (m_defaultMaterialSet && m_defaultMaterialSet.IsReady())
        {
            return m_defaultMaterialSet.Get()->GetMaterialId(materialName);
        }

        return kDefaultMaterialId;
    }

    //////////////////////////////////////////////////////////////////////////
    MaterialId SystemComponent::GetMaterialIdByNameCrc(const AZ::Crc32& materialNameCrc)
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_dataLock);

        if (m_defaultMaterialSet && m_defaultMaterialSet.IsReady())
        {
            return m_defaultMaterialSet.Get()->GetMaterialId(materialNameCrc);
        }

        return kDefaultMaterialId;
    }
        
    //////////////////////////////////////////////////////////////////////////
    MaterialId SystemComponent::MapLegacySurfaceTypeToPhysicsMaterial(const AZ::Crc32& surfaceTypeNameCrc)
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_dataLock);

        if (m_defaultMaterialSet && m_defaultMaterialSet.IsReady())
        {
            return m_defaultMaterialSet.Get()->MapLegacySurfaceTypeToPhysicsMaterial(surfaceTypeNameCrc);
        }

        return kDefaultMaterialId;
    }
        
    //////////////////////////////////////////////////////////////////////////
    MaterialProperties* SystemComponent::GetPhysicsMaterialProperties(MaterialId materialId)
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_dataLock);

        if (m_defaultMaterialSet && m_defaultMaterialSet.IsReady())
        {
            return m_defaultMaterialSet.Get()->GetMaterialProperties(materialId);
        }

        return nullptr;
    }

    //////////////////////////////////////////////////////////////////////////
    MaterialProperties* SystemComponent::GetDefaultPhysicsMaterialProperties()
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_dataLock);

        if (m_defaultMaterialSet && m_defaultMaterialSet.IsReady())
        {
            return m_defaultMaterialSet.Get()->GetDefaultMaterial();
        }

        return nullptr;
    }
        
    //////////////////////////////////////////////////////////////////////////
    void SystemComponent::GetMaterialNames(AZStd::vector<AZStd::string>& materialNames)
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_dataLock);

        if (m_defaultMaterialSet && m_defaultMaterialSet.IsReady())
        {
            m_defaultMaterialSet.Get()->GetMaterialNames(materialNames);
        }
    }

    //////////////////////////////////////////////////////////////////////////
    void SystemComponent::OnCatalogAssetChanged(const AZ::Data::AssetId& assetId)
    {
        (void)assetId;

        const char* materialAssetFile = "default.physicsmaterialset";
        AZ::Data::AssetId materialAssetId;
        EBUS_EVENT_RESULT(materialAssetId, AZ::Data::AssetCatalogRequestBus, GetAssetIdByPath, materialAssetFile, AZ::Data::s_invalidAssetType, false);
        if (materialAssetId.IsValid())
        {
            AZStd::lock_guard<AZStd::mutex> lock(m_dataLock);

            AZ::Data::AssetBus::Handler::BusConnect(materialAssetId);
            m_defaultMaterialSet = AZ::Data::AssetManager::Instance().GetAsset<MaterialSetAsset>(materialAssetId, true);

            // This is a system level asset loaded during engine initialization, so block until loaded.
            // Note: \todo 1.7 has a better API for blocking loads.
            while (m_defaultMaterialSet.IsLoading())
            {
                AZ::Data::AssetManager::Instance().DispatchEvents();
                AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(10));
            }
        }
    }

    //////////////////////////////////////////////////////////////////////////
    //void SystemComponent::OnAssetReady(const AZ::Data::Asset<AZ::Data::AssetData>& asset)
	void SystemComponent::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
	{
        if (asset.GetId() == m_defaultMaterialSet.GetId())
        {
            m_defaultMaterialSet = asset;
        }
    }

} // namespace Physics
