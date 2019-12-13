/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include "LmbrCentral_precompiled.h"

#include <AzFramework/Asset/SimpleAsset.h>
#include <AzFramework/Asset/AssetCatalogBus.h>
#include <AzFramework/StringFunc/StringFunc.h>

#include <LmbrCentral/Rendering/MeshAsset.h>
#include <LmbrCentral/Rendering/MaterialAsset.h>
#include <Rendering/MaterialAssetHandler.h>

namespace LmbrCentral
{   
    //////////////////////////////////////////////////////////////////////////
    // Material Asset Handler
    //////////////////////////////////////////////////////////////////////////

    MaterialAssetHandler::~MaterialAssetHandler()
    {
        Unregister();
    }

    AZ::Data::AssetPtr MaterialAssetHandler::CreateAsset(const AZ::Data::AssetId& id, const AZ::Data::AssetType& type)
    {
        (void)type;

        AZ_Assert(type == AZ::AzTypeInfo<MaterialDataAsset>::Uuid(), "Invalid asset type! We handle only 'MaterialDataAsset'.");

        return aznew MaterialDataAsset();
    }

    bool MaterialAssetHandler::LoadAssetData(const AZ::Data::Asset<AZ::Data::AssetData>& /*asset*/, AZ::IO::GenericStream* /*stream*/, const AZ::Data::AssetFilterCB& /*filterDescriptor*/)
    {
        // Load from preloaded stream.
        AZ_Assert(false, "Favor loading through custom stream override of LoadAssetData, in order to load through CryPak.");
        return false;
    }

    bool MaterialAssetHandler::LoadAssetData(const AZ::Data::Asset<AZ::Data::AssetData>& asset, const char* assetPath, const AZ::Data::AssetFilterCB& /*filterDescriptor*/)
    {
        AZ_Assert(asset.GetType() == AZ::AzTypeInfo<MaterialDataAsset>::Uuid(), "Invalid asset type! We only load 'MaterialDataAsset'.");
        if (MaterialDataAsset* materialAsset = asset.GetAs<MaterialDataAsset>())
        {
            AZ_Assert(!materialAsset->m_Material.get(), "Attempting to create material without cleaning up the old one.");

            // Strip the alias.  Material instances are stored in a dictionary by their path,
            // so to share instances with legacy cry entities, we need to use the same un-aliased format.
            StripAssetAlias(assetPath);

            if (GetISystem() && (GetISystem()->GetI3DEngine()) && GetISystem()->GetI3DEngine()->GetMaterialManager())
            {
                materialAsset->m_Material = GetISystem()->GetI3DEngine()->GetMaterialManager()->LoadMaterial(assetPath);

                if (!(materialAsset->m_Material))
                {
                    #if defined(AZ_ENABLE_TRACING)
                    AZStd::string assetDescription = asset.GetId().ToString<AZStd::string>();
                    AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetDescription, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetPathById, asset.GetId());
                    AZ_Error("MaterialAssetHandler", false, "Failed to load material asset \"%s\".", assetDescription.c_str());
                    #endif // AZ_ENABLE_TRACING
                    return false;
                }
            }

            return true;
        }
        return false;
    }

    void MaterialAssetHandler::DestroyAsset(AZ::Data::AssetPtr ptr)
    {
        delete ptr;
    }

    void MaterialAssetHandler::GetHandledAssetTypes(AZStd::vector<AZ::Data::AssetType>& assetTypes)
    {
        assetTypes.push_back(AZ::AzTypeInfo<MaterialDataAsset>::Uuid());
    }

    void MaterialAssetHandler::Register()
    {
        AZ_Assert(AZ::Data::AssetManager::IsReady(), "Asset manager isn't ready!");
        AZ::Data::AssetManager::Instance().RegisterHandler(this, AZ::AzTypeInfo<MaterialDataAsset>::Uuid());

        AZ::AssetTypeInfoBus::Handler::BusConnect(AZ::AzTypeInfo<MaterialDataAsset>::Uuid());
    }

    void MaterialAssetHandler::Unregister()
    {
        AZ::AssetTypeInfoBus::Handler::BusDisconnect(AZ::AzTypeInfo<MaterialDataAsset>::Uuid());

        if (AZ::Data::AssetManager::IsReady())
        {
            AZ::Data::AssetManager::Instance().UnregisterHandler(this);
        }
    }

    AZ::Data::AssetType MaterialAssetHandler::GetAssetType() const
    {
        return AZ::AzTypeInfo<MaterialDataAsset>::Uuid();
    }

    const char* MaterialAssetHandler::GetAssetTypeDisplayName() const
    {
        return "Material";
    }

    const char * MaterialAssetHandler::GetGroup() const
    {
        return "Material";
    }

    const char* MaterialAssetHandler::GetBrowserIcon() const
    {
        return "Editor/Icons/AssetBrowser/Material_16.png";
    }

    void MaterialAssetHandler::GetAssetTypeExtensions(AZStd::vector<AZStd::string>& extensions)
    {
        extensions.push_back("mtl");
    }
    //////////////////////////////////////////////////////////////////////////

} // namespace LmbrCentral
