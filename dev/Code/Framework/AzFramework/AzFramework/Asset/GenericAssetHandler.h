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
#ifndef AZFRAMEWORK_ASSET_GENERICASSETHANDLER_H
#define AZFRAMEWORK_ASSET_GENERICASSETHANDLER_H

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/IO/GenericStreams.h>
#include <AzCore/Asset/AssetTypeInfoBus.h>

#include <AzFramework/Asset/AssetCatalogBus.h>
#include <AzFramework/StringFunc/StringFunc.h>

namespace AzFramework
{
    /**
     * Generic implementation of an asset handler for any arbitrary type that is reflected for serialization.
     *
     * Simple or game specific assets that wish to make use of automated loading and editing facilities
     * can use this handler with any asset type that's reflected for editing.
     *
     * Example:
     *
     * class MyAsset : public AZ::Data::AssetData
     * {
     * public:
     *  AZ_CLASS_ALLOCATOR(MyAsset, AZ::SystemAllocator, 0);
     *  AZ_RTTI(MyAsset, "{AAAAAAAA-BBBB-CCCC-DDDD-EEEEEEEEEEEE}", AZ::Data::AssetData);
     *
     *  static void Reflect(AZ::ReflectContext* context)
     *  {
     *      AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
     *      if (serialize)
     *      {
     *          serialize->Class<MyAsset>()
     *              ->Field("SomeField", &MyAsset::m_someField)
     *              ;
     *
     *          AZ::EditContext* edit = serialize->GetEditContext();
     *          if (edit)
     *          {
     *              edit->Class<MyAsset>("My Asset", "Asset for representing X, Y, and Z")
     *                  ->DataElement(0, &MyAsset::m_someField, "Some data field", "It's a float")
     *                  ;
     *          }
     *      }
     *  }
     *
     *  float m_someField;
     * };
     *
     *
     * using MyAssetHandler = GenericAssetHandler<MyAsset>;
     *
     */

    template <typename AssetType>
    class GenericAssetHandler
        : public AZ::Data::AssetHandler
        , private AZ::AssetTypeInfoBus::Handler
    {
    public:

        AZ_CLASS_ALLOCATOR(GenericAssetHandler<AssetType>, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(GenericAssetHandler<AssetType>, "{B153B8B5-25CC-4BB7-A2BD-9A47ECF4123C}");

        GenericAssetHandler(const char* extension, AZ::SerializeContext* serializeContext = nullptr)
            : m_serializeContext(serializeContext)
        {
            AZ_Assert(extension, "Extension is required.");
            if (extension[0] == '.')
            {
                ++extension;
            }

            m_extension = extension;
            AZ_Assert(!m_extension.empty(), "Invalid extension provided.");

            if (!m_serializeContext)
            {
                EBUS_EVENT_RESULT(m_serializeContext, AZ::ComponentApplicationBus, GetSerializeContext);
            }

            AZ::AssetTypeInfoBus::Handler::BusConnect(AZ::AzTypeInfo<AssetType>::Uuid());
        }

        AZ::Data::AssetPtr CreateAsset(const AZ::Data::AssetId& /*id*/, const AZ::Data::AssetType& /*type*/) override
        {
            return aznew AssetType();
        }

        bool LoadAssetData(const AZ::Data::Asset<AZ::Data::AssetData>& asset, AZ::IO::GenericStream* stream, const AZ::Data::AssetFilterCB& assetLoadFilterCB) override
        {
            AssetType* assetData = asset.GetAs<AssetType>();
            AZ_Assert(assetData, "Asset is of the wrong type.");
            AZ_Assert(m_serializeContext, "Unable to retrieve serialize context.");
            if (assetData)
            {
                return AZ::Utils::LoadObjectFromStreamInPlace<AssetType>(*stream, *assetData, m_serializeContext, AZ::ObjectStream::FilterDescriptor(assetLoadFilterCB));
            }

            return false;
        }

        bool LoadAssetData(const AZ::Data::Asset<AZ::Data::AssetData>& asset, const char* assetPath, const AZ::Data::AssetFilterCB& assetLoadFilterCB) override
        {
            AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
            if (fileIO)
            {
                AZ::IO::FileIOStream stream(assetPath, AZ::IO::OpenMode::ModeRead);
                if (stream.IsOpen())
                {
                    return LoadAssetData(asset, &stream, assetLoadFilterCB);
                }
            }

            return false;
        }

        bool SaveAssetData(const AZ::Data::Asset<AZ::Data::AssetData>& asset, AZ::IO::GenericStream* stream) override
        {
            AssetType* assetData = asset.GetAs<AssetType>();
            AZ_Assert(assetData, "Asset is of the wrong type.");
            if (assetData && m_serializeContext)
            {
                return AZ::Utils::SaveObjectToStream<AssetType>(*stream,
                    AZ::ObjectStream::ST_XML,
                    assetData,
                    m_serializeContext);
            }

            return false;
        }

        void DestroyAsset(AZ::Data::AssetPtr ptr) override
        {
            delete ptr;
        }

        void GetHandledAssetTypes(AZStd::vector<AZ::Data::AssetType>& assetTypes) override
        {
            assetTypes.push_back(AZ::AzTypeInfo<AssetType>::Uuid());
        }

        void Register()
        {
            EBUS_EVENT(AZ::Data::AssetCatalogRequestBus, EnableCatalogForAsset, AZ::AzTypeInfo<AssetType>::Uuid());
            EBUS_EVENT(AZ::Data::AssetCatalogRequestBus, AddExtension, m_extension.c_str());

            AZ_Assert(AZ::Data::AssetManager::IsReady(), "AssetManager isn't ready!");
            AZ::Data::AssetManager::Instance().RegisterHandler(this, AZ::AzTypeInfo<AssetType>::Uuid());
        }

        void Unregister()
        {
            if (AZ::Data::AssetManager::IsReady())
            {
                AZ::Data::AssetManager::Instance().UnregisterHandler(this);
            }
        }

        AZ::Data::AssetType GetAssetType() const override
        {
            return AZ::AzTypeInfo<AssetType>::Uuid();
        }

        void GetAssetTypeExtensions(AZStd::vector<AZStd::string>& extensions) override
        {
            extensions.push_back(m_extension);
        }

        bool CanHandleAsset(const AZ::Data::AssetId& id) const
        {
            AZStd::string assetPath;
            EBUS_EVENT_RESULT(assetPath, AZ::Data::AssetCatalogRequestBus, GetAssetPathById, id);
            if (!assetPath.empty())
            {
                AZStd::string assetExtension;
                if (AzFramework::StringFunc::Path::GetExtension(assetPath.c_str(), assetExtension, false))
                {
                    return assetExtension == m_extension;
                }
            }

            return false;
        }

        AZStd::string m_extension;
        AZ::SerializeContext* m_serializeContext;

        private:
            // Workaround for VS2013 - Delete the copy constructor and make it private
            // https://connect.microsoft.com/VisualStudio/feedback/details/800328/std-is-copy-constructible-is-broken
            GenericAssetHandler(const GenericAssetHandler&) = delete;
    };
}   // namespace AZ

#endif // AZFRAMEWORK_ASSET_GENERICASSETHANDLER_H
