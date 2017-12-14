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


#include <StdAfx.h>

#include <Pipeline/PhysXMeshAsset.h>
#include <AzCore/IO/GenericStreams.h>
#include <AzCore/IO/FileIO.h>

namespace PhysX
{
    namespace Pipeline
    {
        const char* PhysXMeshAsset::m_assetFileExtention = "pxmesh";

        PhysXMeshAssetHandler::PhysXMeshAssetHandler()
        {
            Register();
        }

        PhysXMeshAssetHandler::~PhysXMeshAssetHandler()
        {
            Unregister();
        }

        void PhysXMeshAssetHandler::Register()
        {
            AZ_Assert(AZ::Data::AssetManager::IsReady(), "Asset manager isn't ready!");
            AZ::Data::AssetManager::Instance().RegisterHandler(this, AZ::AzTypeInfo<PhysXMeshAsset>::Uuid());

            AZ::AssetTypeInfoBus::Handler::BusConnect(AZ::AzTypeInfo<PhysXMeshAsset>::Uuid());
        }

        void PhysXMeshAssetHandler::Unregister()
        {
            AZ::AssetTypeInfoBus::Handler::BusDisconnect();

            if (AZ::Data::AssetManager::IsReady())
            {
                AZ::Data::AssetManager::Instance().UnregisterHandler(this);
            }
        }

        //////////////////////////////////////////////////////////////////////////////////////////////
        // AZ::AssetTypeInfoBus
        AZ::Data::AssetType PhysXMeshAssetHandler::GetAssetType() const
        {
            return AZ::AzTypeInfo<PhysXMeshAsset>::Uuid();
        }

        void PhysXMeshAssetHandler::GetAssetTypeExtensions(AZStd::vector<AZStd::string>& extensions)
        {
            extensions.push_back(PhysXMeshAsset::m_assetFileExtention);
        }

        const char* PhysXMeshAssetHandler::GetAssetTypeDisplayName() const
        {
            return "PhysX Collision Mesh";
        }

        const char* PhysXMeshAssetHandler::GetBrowserIcon() const
        {
            return AZ::AssetTypeInfoBus::Handler::GetBrowserIcon();
        }

        const char* PhysXMeshAssetHandler::GetGroup() const
        {
            return "Physics";
        }

        AZ::Uuid PhysXMeshAssetHandler::GetComponentTypeId() const
        {
            return AZ::Uuid("{87A02711-8D7F-4966-87E1-77001EB6B29E}"); // PhysXMeshShapeComponent
        }
        //////////////////////////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////////////////////////
        // AZ::Data::AssetHandler
        
        AZ::Data::AssetPtr PhysXMeshAssetHandler::CreateAsset(const AZ::Data::AssetId& id, const AZ::Data::AssetType& type)
        {
            AZ_Assert(type == AZ::AzTypeInfo<PhysXMeshAsset>::Uuid(), "This handler deals only with PhysXMeshAsset type!");
            if (type == AZ::AzTypeInfo<PhysXMeshAsset>::Uuid())
            {
                return aznew PhysXMeshAsset();
            }

            return nullptr;
        }
        
        bool PhysXMeshAssetHandler::LoadAssetData(const AZ::Data::Asset<AZ::Data::AssetData>& asset, AZ::IO::GenericStream* stream, const AZ::Data::AssetFilterCB& assetLoadFilterCB)
        {
            PhysXMeshAsset* physXMeshAsset = asset.GetAs<PhysXMeshAsset>();
            AZ_Assert(physXMeshAsset, "This should be a PhysXMeshAsset, as this is the only type we process!");

            AZ::u32 assetVersion = 0;
            stream->Read(sizeof(assetVersion), &assetVersion);
            
            AZ::u8 isConvexMesh = 0;
            stream->Read(sizeof(isConvexMesh), &isConvexMesh);
            
            AZ::u32 assetDataSize = 0;
            stream->Read(sizeof(assetDataSize), &assetDataSize);

            if (assetVersion == 1 && assetDataSize > 0)
            {
                void* assetDataBuffer = azmalloc(assetDataSize);

                stream->Read(assetDataSize, assetDataBuffer);

                if (isConvexMesh == 1)
                {
                    PhysX::PhysXSystemRequestBus::BroadcastResult(physXMeshAsset->m_meshData, &PhysX::PhysXSystemRequests::CreateConvexMeshFromCooked, assetDataBuffer, assetDataSize);
                }
                else
                {
                    PhysX::PhysXSystemRequestBus::BroadcastResult(physXMeshAsset->m_meshData, &PhysX::PhysXSystemRequests::CreateTriangleMeshFromCooked, assetDataBuffer, assetDataSize);
                }
                
                AZ_Error("PhysX", physXMeshAsset->m_meshData != nullptr, "Failed to construct PhysX mesh from the cooked data. Possible data corruption.");

                azfree(assetDataBuffer);

                return (physXMeshAsset->m_meshData != nullptr);
            }

            return false;
        }

        bool PhysXMeshAssetHandler::LoadAssetData(const AZ::Data::Asset<AZ::Data::AssetData>& asset, const char* assetPath, const AZ::Data::AssetFilterCB& assetLoadFilterCB)
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

        void PhysXMeshAssetHandler::DestroyAsset(AZ::Data::AssetPtr ptr)
        {
            delete ptr;
        }

        void PhysXMeshAssetHandler::GetHandledAssetTypes(AZStd::vector<AZ::Data::AssetType>& assetTypes)
        {
            assetTypes.push_back(AZ::AzTypeInfo<PhysXMeshAsset>::Uuid());
        }
        //////////////////////////////////////////////////////////////////////////////////////////////
    } //namespace Integration
} // namespace EMotionFX