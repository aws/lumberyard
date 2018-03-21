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

#include <AzCore/IO/GenericStreams.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/XML/rapidxml.h>
#include <AzFramework/Asset/SimpleAsset.h>
#include <AzFramework/Asset/AssetCatalogBus.h>

#include <LmbrCentral/Ai/BehaviorTreeAsset.h>
#include "BehaviorTreeAssetHandler.h"

#define BEHAVIOR_TREE_LIB_EXT "xml"

namespace LmbrCentral
{
    bool LoadFromBuffer(BehaviorTreeAsset* data, char* buffer, size_t bufferSize)
    {
        using namespace AZ::rapidxml;

        xml_document<char> xmlDoc;
        xmlDoc.parse<parse_no_data_nodes>(buffer);

        xml_node<char>* root = xmlDoc.first_node();
        if (root)
        {
            // This will be filled out as more data is used through the asset
            // For now we only care that it is well formed xml with a BehaviorTree node
            return true;
        }

        return false;
    }

    BehaviorTreeAssetHandler::~BehaviorTreeAssetHandler()
    {
        Unregister();
    }

    AZ::Data::AssetPtr BehaviorTreeAssetHandler::CreateAsset(const AZ::Data::AssetId& id, const AZ::Data::AssetType& type)
    {
        (void)type;
        AZ_Assert(type == GetAssetType(), "Invalid asset type! We handle only 'BehaviorTreeAsset'");

        // Look up the asset path to ensure it's actually a behavior tree library.
        if (!CanHandleAsset(id))
        {
            return nullptr;
        }

        return aznew BehaviorTreeAsset;
    }

    bool BehaviorTreeAssetHandler::LoadAssetData(const AZ::Data::Asset<AZ::Data::AssetData>& asset, AZ::IO::GenericStream* stream, const AZ::Data::AssetFilterCB& /*assetLoadFilterCB*/)
    {
        // Load from preloaded stream.
        AZ_Assert(asset.GetType() == AZ::AzTypeInfo<BehaviorTreeAsset>::Uuid(), "Invalid asset type! We handle only 'BehaviorTreeAsset'");
        if (stream)
        {
            BehaviorTreeAsset* data = asset.GetAs<BehaviorTreeAsset>();

            const size_t sizeBytes = static_cast<size_t>(stream->GetLength());
            char* buffer = new char[sizeBytes];
            stream->Read(sizeBytes, buffer);

            bool loaded = LoadFromBuffer(data, buffer, sizeBytes);

            delete[] buffer;

            return loaded;
        }

        return false;
    }

    AZ::Data::AssetType BehaviorTreeAssetHandler::GetAssetType() const
    {
        return AZ::AzTypeInfo<BehaviorTreeAsset>::Uuid();
    }

    const char* BehaviorTreeAssetHandler::GetAssetTypeDisplayName() const
    {
        return "BehaviorTree";
    }

    const char* BehaviorTreeAssetHandler::GetGroup() const
    {
        return "BehaviorTree";
    }

    const char* BehaviorTreeAssetHandler::GetBrowserIcon() const
    {
        return "Editor/Icons/Components/BehaviorTree.png";
    }

    AZ::Uuid BehaviorTreeAssetHandler::GetComponentTypeId() const
    {
        return AZ::Uuid::CreateNull();
    }

    void BehaviorTreeAssetHandler::GetAssetTypeExtensions(AZStd::vector<AZStd::string>& extensions)
    {
        extensions.push_back("xml");
    }

    bool BehaviorTreeAssetHandler::LoadAssetData(const AZ::Data::Asset<AZ::Data::AssetData>& asset, const char* assetPath, const AZ::Data::AssetFilterCB& /*assetLoadFilterCB*/)
    {
        bool loaded = false;

        // Load from CryPak.
        AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
        AZ_Assert(fileIO, "FileIO is not initialized.");
        AZ::IO::HandleType fileHandle = AZ::IO::InvalidHandle;
        const AZ::IO::Result result = fileIO->Open(assetPath, AZ::IO::OpenMode::ModeRead, fileHandle);
        if (result)
        {
            BehaviorTreeAsset* data = asset.GetAs<BehaviorTreeAsset>();

            AZ::u64 sizeBytes = 0;
            if (fileIO->Size(fileHandle, sizeBytes))
            {
                char* buffer = new char[sizeBytes];
                if (fileIO->Read(fileHandle, buffer, sizeBytes))
                {
                    loaded = LoadFromBuffer(data, buffer, sizeBytes);
                }

                delete[] buffer;
            }

            fileIO->Close(fileHandle);
        }

        return loaded;
    }

    void BehaviorTreeAssetHandler::DestroyAsset(AZ::Data::AssetPtr ptr)
    {
        delete ptr;
    }

    void BehaviorTreeAssetHandler::GetHandledAssetTypes(AZStd::vector<AZ::Data::AssetType>& assetTypes)
    {
        assetTypes.push_back(GetAssetType());
    }

    void BehaviorTreeAssetHandler::Register()
    {
        AZ_Assert(AZ::Data::AssetManager::IsReady(), "Asset manager isn't ready!");
        AZ::Data::AssetManager::Instance().RegisterHandler(this, GetAssetType());

        AZ::AssetTypeInfoBus::Handler::BusConnect(GetAssetType());
    }

    void BehaviorTreeAssetHandler::Unregister()
    {
        AZ::AssetTypeInfoBus::Handler::BusDisconnect();

        if (AZ::Data::AssetManager::IsReady())
        {
            AZ::Data::AssetManager::Instance().UnregisterHandler(this);
        }
    }

    bool BehaviorTreeAssetHandler::CanHandleAsset(const AZ::Data::AssetId& id) const
    {
        // Look up the asset path to ensure it's actually a behavior tree library.
        AZStd::string assetPath;
        EBUS_EVENT_RESULT(assetPath, AZ::Data::AssetCatalogRequestBus, GetAssetPathById, id);

        bool isBehaviorTree = false;

        if (!assetPath.empty())
        {
            AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
            AZ_Assert(fileIO, "FileIO is not initialized.");
            AZ::IO::HandleType fileHandle = AZ::IO::InvalidHandle;
            const AZ::IO::Result result = fileIO->Open(assetPath.c_str(), AZ::IO::OpenMode::ModeRead, fileHandle);
            if (result)
            {
                AZ::u64 fileSize = 0;
                if (fileIO->Size(fileHandle, fileSize) && fileSize)
                {
                    AZStd::vector<char> memoryBuffer;
                    memoryBuffer.resize_no_construct(fileSize);
                    if (fileIO->Read(fileHandle, memoryBuffer.begin(), fileSize, true))
                    {
                        isBehaviorTree = (nullptr != strstr(memoryBuffer.begin(), "BehaviorTree"));
                    }
                }

                fileIO->Close(fileHandle);
            }
        }

        return isBehaviorTree;
    }
} // namespace LmbrCentral

