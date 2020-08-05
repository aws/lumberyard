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

#include "WhiteBox_precompiled.h"

#include "Asset/WhiteBoxMeshAsset.h"
#include "Asset/WhiteBoxMeshAssetHandler.h"

#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/GenericStreams.h>

namespace WhiteBox
{
    namespace Pipeline
    {
        const char* const WhiteBoxMeshAssetHandler::s_assetFileExtension = "wbm";

        WhiteBoxMeshAssetHandler::WhiteBoxMeshAssetHandler()
        {
            Register();
        }

        WhiteBoxMeshAssetHandler::~WhiteBoxMeshAssetHandler()
        {
            Unregister();
        }

        void WhiteBoxMeshAssetHandler::Register()
        {
            const bool assetManagerReady = AZ::Data::AssetManager::IsReady();
            AZ_Error("WhiteBoxMesh Asset", assetManagerReady, "Asset manager isn't ready.");

            if (assetManagerReady)
            {
                AZ::Data::AssetManager::Instance().RegisterHandler(this, AZ::AzTypeInfo<WhiteBoxMeshAsset>::Uuid());
            }

            AZ::AssetTypeInfoBus::Handler::BusConnect(AZ::AzTypeInfo<WhiteBoxMeshAsset>::Uuid());
        }

        void WhiteBoxMeshAssetHandler::Unregister()
        {
            AZ::AssetTypeInfoBus::Handler::BusDisconnect();

            if (AZ::Data::AssetManager::IsReady())
            {
                AZ::Data::AssetManager::Instance().UnregisterHandler(this);
            }
        }

        AZ::Data::AssetType WhiteBoxMeshAssetHandler::GetAssetType() const
        {
            return AZ::AzTypeInfo<WhiteBoxMeshAsset>::Uuid();
        }

        void WhiteBoxMeshAssetHandler::GetAssetTypeExtensions(AZStd::vector<AZStd::string>& extensions)
        {
            extensions.push_back(s_assetFileExtension);
        }

        const char* WhiteBoxMeshAssetHandler::GetAssetTypeDisplayName() const
        {
            return "WhiteBoxMesh";
        }

        const char* WhiteBoxMeshAssetHandler::GetBrowserIcon() const
        {
            return "Editor/Icons/Components/WhiteBoxMesh.png";
        }

        const char* WhiteBoxMeshAssetHandler::GetGroup() const
        {
            return "WhiteBox";
        }

        AZ::Uuid WhiteBoxMeshAssetHandler::GetComponentTypeId() const
        {
            return "{C9F2D913-E275-49BB-AB4F-2D221C16170A}"; // EditorWhiteBoxComponent
        }

        // AZ::Data::AssetHandler
        AZ::Data::AssetPtr WhiteBoxMeshAssetHandler::CreateAsset(
            const AZ::Data::AssetId& id, const AZ::Data::AssetType& type)
        {
            if (type == AZ::AzTypeInfo<WhiteBoxMeshAsset>::Uuid())
            {
                return aznew WhiteBoxMeshAsset();
            }

            AZ_Error("WhiteBoxMesh", false, "This handler deals only with WhiteBoxMeshAsset type.");
            return nullptr;
        }

        bool WhiteBoxMeshAssetHandler::LoadAssetData(
            const AZ::Data::Asset<AZ::Data::AssetData>& asset, AZ::IO::GenericStream* stream,
            [[maybe_unused]] const AZ::Data::AssetFilterCB& assetLoadFilterCB)
        {
            WhiteBoxMeshAsset* whiteBoxMeshAsset = asset.GetAs<WhiteBoxMeshAsset>();
            if (!whiteBoxMeshAsset)
            {
                AZ_Error(
                    "WhiteBoxMesh Asset", false,
                    "This should be a WhiteBoxMesh Asset, as this is the only type we process.");
                return false;
            }

            const auto size = stream->GetLength();

            AZStd::vector<AZ::u8> buffer(size);
            stream->Read(size, buffer.data());

            auto whiteBoxMesh = WhiteBox::Api::CreateWhiteBoxMesh();
            const bool success = WhiteBox::Api::ReadMesh(*whiteBoxMesh, buffer);

            if (success)
            {
                whiteBoxMeshAsset->SetMesh(AZStd::move(whiteBoxMesh));
                whiteBoxMeshAsset->SetUndoData(buffer);
            }

            return success;
        }

        bool WhiteBoxMeshAssetHandler::SaveAssetData(
            const AZ::Data::Asset<AZ::Data::AssetData>& asset, AZ::IO::GenericStream* stream)
        {
            WhiteBoxMeshAsset* whiteBoxMeshAsset = asset.GetAs<WhiteBoxMeshAsset>();
            if (!whiteBoxMeshAsset)
            {
                AZ_Error(
                    "WhiteBoxMesh Asset", false,
                    "This should be a WhiteBoxMesh Asset. WhiteBoxMeshAssetHandler doesn't handle any other asset "
                    "type.");
                return false;
            }

            WhiteBox::WhiteBoxMesh* mesh = whiteBoxMeshAsset->GetMesh();
            if (!mesh)
            {
                AZ_Warning("WhiteBoxMesh Asset", false, "There is no WhiteBoxMesh to save.");
                return false;
            }

            AZStd::vector<AZ::u8> buffer;
            WhiteBox::Api::WriteMesh(*mesh, buffer);

            const auto bytesWritten = stream->Write(buffer.size(), buffer.data());
            const bool success = bytesWritten == buffer.size();

            if (!success)
            {
                AZ_Warning("", false, "Failed to write white box mesh asset:", asset.GetHint().c_str());
            }

            return success;
        }

        bool WhiteBoxMeshAssetHandler::LoadAssetData(
            const AZ::Data::Asset<AZ::Data::AssetData>& asset, const char* assetPath,
            const AZ::Data::AssetFilterCB& assetLoadFilterCB)
        {
            if (AZ::IO::FileIOBase::GetInstance())
            {
                AZ::IO::FileIOStream stream(assetPath, AZ::IO::OpenMode::ModeRead);
                if (stream.IsOpen())
                {
                    return LoadAssetData(asset, &stream, assetLoadFilterCB);
                }
            }

            return true;
        }

        void WhiteBoxMeshAssetHandler::DestroyAsset(AZ::Data::AssetPtr ptr)
        {
            delete ptr;
        }

        void WhiteBoxMeshAssetHandler::GetHandledAssetTypes(AZStd::vector<AZ::Data::AssetType>& assetTypes)
        {
            assetTypes.push_back(AZ::AzTypeInfo<WhiteBoxMeshAsset>::Uuid());
        }
    } // namespace Pipeline
} // namespace WhiteBox
