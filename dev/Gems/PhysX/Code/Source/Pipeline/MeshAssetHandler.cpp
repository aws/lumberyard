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

#include <Pipeline/MeshAssetHandler.h>
#include <AzCore/IO/GenericStreams.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/Serialization/Utils.h>
#include <PhysX/MeshAsset.h>
#include <PhysX/SystemComponentBus.h>
#include <Source/Pipeline/MeshAssetHandler.h>

namespace PhysX
{
    namespace Pipeline
    {
        const char* MeshAssetHandler::s_assetFileExtension = "pxmesh";

        MeshAssetHandler::MeshAssetHandler()
        {
            Register();
        }

        MeshAssetHandler::~MeshAssetHandler()
        {
            Unregister();
        }

        void MeshAssetHandler::Register()
        {
            bool assetManagerReady = AZ::Data::AssetManager::IsReady();
            AZ_Error("PhysX Mesh Asset", assetManagerReady, "Asset manager isn't ready.");
            if (assetManagerReady)
            {
                AZ::Data::AssetManager::Instance().RegisterHandler(this, AZ::AzTypeInfo<MeshAsset>::Uuid());
            }

            AZ::AssetTypeInfoBus::Handler::BusConnect(AZ::AzTypeInfo<MeshAsset>::Uuid());
        }

        void MeshAssetHandler::Unregister()
        {
            AZ::AssetTypeInfoBus::Handler::BusDisconnect();

            if (AZ::Data::AssetManager::IsReady())
            {
                AZ::Data::AssetManager::Instance().UnregisterHandler(this);
            }
        }

        // AZ::AssetTypeInfoBus
        AZ::Data::AssetType MeshAssetHandler::GetAssetType() const
        {
            return AZ::AzTypeInfo<MeshAsset>::Uuid();
        }

        void MeshAssetHandler::GetAssetTypeExtensions(AZStd::vector<AZStd::string>& extensions)
        {
            extensions.push_back(s_assetFileExtension);
        }

        const char* MeshAssetHandler::GetAssetTypeDisplayName() const
        {
            return "PhysX Collision Mesh (PhysX Gem)";
        }

        const char* MeshAssetHandler::GetBrowserIcon() const
        {
            return "Editor/Icons/Components/ColliderMesh.png";
        }

        const char* MeshAssetHandler::GetGroup() const
        {
            return "Physics";
        }

        // Disable spawning of physics asset entities on drag and drop
        AZ::Uuid MeshAssetHandler::GetComponentTypeId() const
        {
            // NOTE: This doesn't do anything when CanCreateComponent returns false
            return AZ::Uuid("{FD429282-A075-4966-857F-D0BBF186CFE6}"); // EditorColliderComponent
        }

        bool MeshAssetHandler::CanCreateComponent(const AZ::Data::AssetId& assetId) const
        {
            return false;
        }

        // AZ::Data::AssetHandler
        AZ::Data::AssetPtr MeshAssetHandler::CreateAsset(const AZ::Data::AssetId& id, const AZ::Data::AssetType& type)
        {
            if (type == AZ::AzTypeInfo<MeshAsset>::Uuid())
            {
                return aznew MeshAsset();
            }

            AZ_Error("PhysX Mesh Asset", false, "This handler deals only with PhysXMeshAsset type.");
            return nullptr;
        }

        bool MeshAssetHandler::LoadAssetData(const AZ::Data::Asset<AZ::Data::AssetData>& asset, AZ::IO::GenericStream* stream, const AZ::Data::AssetFilterCB& assetLoadFilterCB)
        {
            MeshAsset* meshAsset = asset.GetAs<MeshAsset>();
            if (!meshAsset)
            {
                AZ_Error("PhysX Mesh Asset", false, "This should be a PhysXMeshAsset, as this is the only type we process.");
                return false;
            }

            AZ::SerializeContext* serializeContext = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);

            MeshAssetCookedData cookedMeshAsset;
            if (!AZ::Utils::LoadObjectFromStreamInPlace(*stream, cookedMeshAsset, serializeContext))
            {
                return false;
            }

            if (cookedMeshAsset.m_isConvexMesh)
            {
                PhysX::SystemRequestsBus::BroadcastResult(
                    meshAsset->m_meshData,
                    &PhysX::SystemRequests::CreateConvexMeshFromCooked, 
                    static_cast<void*>(cookedMeshAsset.m_cookedPxMeshData.data()),
                    static_cast<AZ::u32>(cookedMeshAsset.m_cookedPxMeshData.size())
                );
            }
            else
            {
                PhysX::SystemRequestsBus::BroadcastResult(
                    meshAsset->m_meshData,
                    &PhysX::SystemRequests::CreateTriangleMeshFromCooked, 
                    static_cast<void*>(cookedMeshAsset.m_cookedPxMeshData.data()),
                    static_cast<AZ::u32>(cookedMeshAsset.m_cookedPxMeshData.size())
                );
            }

            meshAsset->m_materials = AZStd::move(cookedMeshAsset.m_materialsData);
            meshAsset->m_materialSlots = AZStd::move(cookedMeshAsset.m_materialSlots);

            return meshAsset->m_meshData != nullptr;
        }

        bool MeshAssetHandler::LoadAssetData(const AZ::Data::Asset<AZ::Data::AssetData>& asset, const char* assetPath, const AZ::Data::AssetFilterCB& assetLoadFilterCB)
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

        void MeshAssetHandler::DestroyAsset(AZ::Data::AssetPtr ptr)
        {
            delete ptr;
        }

        void MeshAssetHandler::GetHandledAssetTypes(AZStd::vector<AZ::Data::AssetType>& assetTypes)
        {
            assetTypes.push_back(AZ::AzTypeInfo<MeshAsset>::Uuid());
        }

        MeshAssetCookedData::MeshAssetCookedData(const physx::PxDefaultMemoryOutputStream& cookedStream)
        {
            m_cookedPxMeshData.insert( m_cookedPxMeshData.begin(), cookedStream.getData(), cookedStream.getData() + cookedStream.getSize());
        }

        void MeshAssetCookedData::Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (serializeContext)
            {
                serializeContext->Class<MeshAssetCookedData>()
                    ->Version(2)
                    ->Field("Convexity", &MeshAssetCookedData::m_isConvexMesh)
                    ->Field("Materials", &MeshAssetCookedData::m_materialsData)
                    ->Field("MaterialSlots", &MeshAssetCookedData::m_materialSlots)
                    ->Field("CookedPxData", &MeshAssetCookedData::m_cookedPxMeshData)
                    ;
            }
        }
} //namespace Pipeline
} // namespace PhysX
