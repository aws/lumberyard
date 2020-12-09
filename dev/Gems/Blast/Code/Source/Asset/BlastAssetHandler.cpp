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

#include "StdAfx.h"

#include <Asset/BlastAssetHandler.h>

#include <Asset/BlastAsset.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/GenericStreams.h>

namespace Blast
{
    BlastAssetHandler::~BlastAssetHandler()
    {
        Unregister();
    }

    AZ::Data::AssetPtr BlastAssetHandler::CreateAsset(
        const AZ::Data::AssetId& id, [[maybe_unused]] const AZ::Data::AssetType& type)
    {
        if (!CanHandleAsset(id) || type != GetAssetType())
        {
            AZ_Error("Blast", false, "Invalid asset type! BlastAssetHandler only handle 'BlastAsset'");
            return nullptr;
        }

        return aznew BlastAsset;
    }

    bool BlastAssetHandler::LoadAssetData(
        const AZ::Data::Asset<AZ::Data::AssetData>& asset, AZ::IO::GenericStream* stream,
        const AZ::Data::AssetFilterCB& assetLoadFilterCB)
    {
        if (!stream || asset.GetType() != AZ::AzTypeInfo<BlastAsset>::Uuid())
        {
            AZ_Error(
                "Blast", asset.GetType() == AZ::AzTypeInfo<BlastAsset>::Uuid(),
                "Invalid asset type! We only handle 'BlastAsset'");
            return false;
        }

        BlastAsset* data = asset.GetAs<BlastAsset>();

        const size_t sizeBytes = static_cast<size_t>(stream->GetLength());
        AZStd::vector<char> buffer;
        buffer.resize_no_construct(sizeBytes);
        stream->Read(sizeBytes, buffer.data());

        return data->LoadFromBuffer(buffer.data(), sizeBytes);
    }

    bool BlastAssetHandler::LoadAssetData(
        const AZ::Data::Asset<AZ::Data::AssetData>& asset, const char* assetPath,
        const AZ::Data::AssetFilterCB& /*assetLoadFilterCB*/)
    {
        if (asset.GetType() != AZ::AzTypeInfo<BlastAsset>::Uuid())
        {
            AZ_Error("Blast", false, "Invalid asset type! We only handle 'BlastAsset'");
            return false;
        }

        // Load from CryPak.
        AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
        if (!fileIO)
        {
            AZ_Error("Blast", false, "FileIO is not initialized.");
            return false;
        }

        AZ::IO::HandleType fileHandle = AZ::IO::InvalidHandle;
        const AZ::IO::Result result = fileIO->Open(assetPath, AZ::IO::OpenMode::ModeRead, fileHandle);

        if (!result)
        {
            AZ_Error("Blast", false, "Could not open file on the provided asset path %s", assetPath);
            return false;
        }

        BlastAsset* data = asset.GetAs<BlastAsset>();

        AZ::u64 sizeBytes = 0;

        if (!fileIO->Size(fileHandle, sizeBytes))
        {
            AZ_Error("Blast", false, "Failed to retrieve size of the asset file %s", assetPath);
            fileIO->Close(fileHandle);
            return false;
        }

        AZStd::vector<char> buffer;
        buffer.resize_no_construct(sizeBytes);

        bool loaded = false;
        if (fileIO->Read(fileHandle, buffer.data(), sizeBytes))
        {
            loaded = data->LoadFromBuffer(buffer.data(), sizeBytes);
        }
        else
        {
            AZ_Error("Blast", false, "Failed to read from the asset file %s into the buffer", assetPath);
            loaded = false;
        }

        fileIO->Close(fileHandle);

        return loaded;
    }

    void BlastAssetHandler::DestroyAsset(AZ::Data::AssetPtr ptr)
    {
        delete ptr;
    }

    void BlastAssetHandler::GetHandledAssetTypes(AZStd::vector<AZ::Data::AssetType>& assetTypes)
    {
        assetTypes.push_back(AZ::AzTypeInfo<BlastAsset>::Uuid());
    }

    void BlastAssetHandler::Register()
    {
        AZ_Assert(AZ::Data::AssetManager::IsReady(), "Asset manager isn't ready!");
        AZ::Data::AssetManager::Instance().RegisterHandler(this, AZ::AzTypeInfo<BlastAsset>::Uuid());

        AZ::AssetTypeInfoBus::Handler::BusConnect(AZ::AzTypeInfo<BlastAsset>::Uuid());
    }

    void BlastAssetHandler::Unregister()
    {
        AZ::AssetTypeInfoBus::Handler::BusDisconnect(AZ::AzTypeInfo<BlastAsset>::Uuid());

        if (AZ::Data::AssetManager::IsReady())
        {
            AZ::Data::AssetManager::Instance().UnregisterHandler(this);
        }
    }

    AZ::Data::AssetType BlastAssetHandler::GetAssetType() const
    {
        return AZ::AzTypeInfo<BlastAsset>::Uuid();
    }

    const char* BlastAssetHandler::GetAssetTypeDisplayName() const
    {
        return "Blast Asset";
    }

    const char* BlastAssetHandler::GetGroup() const
    {
        return "Blast";
    }

    const char* BlastAssetHandler::GetBrowserIcon() const
    {
        return "Editor/Icons/Components/Box.png";
    }

    void BlastAssetHandler::GetAssetTypeExtensions(AZStd::vector<AZStd::string>& extensions)
    {
        extensions.push_back("blast");
    }
} // namespace Blast
