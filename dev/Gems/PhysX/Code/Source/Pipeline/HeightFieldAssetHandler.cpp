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


#include <PhysX_precompiled.h>

#include <Pipeline/HeightFieldAssetHandler.h>
#include <AzCore/IO/GenericStreams.h>
#include <AzCore/IO/FileIO.h>
#include <PhysX/HeightFieldAsset.h>
#include <PhysX/SystemComponentBus.h>
#include <PhysX/ComponentTypeIds.h>
#include <Source/Pipeline/HeightFieldAssetHandler.h>


namespace PhysX
{
    namespace Pipeline
    {
        const char* HeightFieldAssetHandler::s_assetFileExtension = "pxheightfield";

        HeightFieldAssetHandler::HeightFieldAssetHandler()
        {
            Register();
        }

        HeightFieldAssetHandler::~HeightFieldAssetHandler()
        {
            Unregister();
        }

        void HeightFieldAssetHandler::Register()
        {
            bool assetManagerReady = AZ::Data::AssetManager::IsReady();
            AZ_Error("PhysX HeightField Asset", assetManagerReady, "Asset manager isn't ready.");
            if (assetManagerReady)
            {
                AZ::Data::AssetManager::Instance().RegisterHandler(this, AZ::AzTypeInfo<HeightFieldAsset>::Uuid());
            }

            AZ::AssetTypeInfoBus::Handler::BusConnect(AZ::AzTypeInfo<HeightFieldAsset>::Uuid());
        }

        void HeightFieldAssetHandler::Unregister()
        {
            AZ::AssetTypeInfoBus::Handler::BusDisconnect();

            if (AZ::Data::AssetManager::IsReady())
            {
                AZ::Data::AssetManager::Instance().UnregisterHandler(this);
            }
        }

        // AZ::AssetTypeInfoBus
        AZ::Data::AssetType HeightFieldAssetHandler::GetAssetType() const
        {
            return AZ::AzTypeInfo<HeightFieldAsset>::Uuid();
        }

        void HeightFieldAssetHandler::GetAssetTypeExtensions(AZStd::vector<AZStd::string>& extensions)
        {
            extensions.push_back(HeightFieldAssetHandler::s_assetFileExtension);
        }

        const char* HeightFieldAssetHandler::GetAssetTypeDisplayName() const
        {
            return "PhysX HeightField Mesh";
        }

        const char* HeightFieldAssetHandler::GetBrowserIcon() const
        {
            return "Editor/Icons/Components/ColliderMesh.png";
        }

        const char* HeightFieldAssetHandler::GetGroup() const
        {
            return "Physics";
        }

        AZ::Uuid HeightFieldAssetHandler::GetComponentTypeId() const
        {
            return PhysX::EditorTerrainComponentTypeId;
        }

        // AZ::Data::AssetHandler
        AZ::Data::AssetPtr HeightFieldAssetHandler::CreateAsset(const AZ::Data::AssetId& id, const AZ::Data::AssetType& type)
        {
            if (type == AZ::AzTypeInfo<HeightFieldAsset>::Uuid())
            {
                return aznew HeightFieldAsset();
            }

            AZ_Error("PhysX HeightField Asset", false, "This handler deals only with PhysXHeightFieldAsset type.");
            return nullptr;
        }

        bool HeightFieldAssetHandler::LoadAssetData(const AZ::Data::Asset<AZ::Data::AssetData>& asset, AZ::IO::GenericStream* stream, const AZ::Data::AssetFilterCB& assetLoadFilterCB)
        {
            HeightFieldAsset* physXHeightFieldAsset = asset.GetAs<HeightFieldAsset>();
            if (!physXHeightFieldAsset)
            {
                AZ_Error("PhysX HeightField Asset", false, "This should be a PhysX HeightField Asset, as this is the only type we process.");
                return false;
            }

            // Read the file header
            HeightFieldAssetHeader header;
            stream->Read(sizeof(header), &header);

            // Parse version 1.
            if (header.m_assetVersion == 1 && header.m_assetDataSize > 0)
            {
                // Read heightFieldData
                AZStd::vector<uint8_t> assetDataBuffer;
                assetDataBuffer.resize(header.m_assetDataSize);
                stream->Read(header.m_assetDataSize, &assetDataBuffer[0]);
                
                // Deserialise buffer into heightfield
                physx::PxDefaultMemoryInputData inputStream(assetDataBuffer.begin(), header.m_assetDataSize);
                physx::PxPhysics& physx = PxGetPhysics();
                physXHeightFieldAsset->m_heightField = physx.createHeightField(inputStream);
        
                AZ_Error("PhysX HeightField Asset", physXHeightFieldAsset->m_heightField != nullptr, "Failed to construct PhysX mesh from the cooked data. Possible data corruption.");
                return physXHeightFieldAsset->m_heightField != nullptr;
            }

            return false;
        }

        bool HeightFieldAssetHandler::LoadAssetData(const AZ::Data::Asset<AZ::Data::AssetData>& asset, const char* assetPath, const AZ::Data::AssetFilterCB& assetLoadFilterCB)
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
            return true;
        }

        void HeightFieldAssetHandler::DestroyAsset(AZ::Data::AssetPtr ptr)
        {
            delete ptr;
        }

        void HeightFieldAssetHandler::GetHandledAssetTypes(AZStd::vector<AZ::Data::AssetType>& assetTypes)
        {
            assetTypes.push_back(AZ::AzTypeInfo<HeightFieldAsset>::Uuid());
        }
    } //namespace Pipeline
} // namespace PhysX
