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

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <Source/MeshColliderComponent.h>

namespace PhysX
{
    void MeshColliderComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->ClassDeprecate("MeshColliderComponent", "{87A02711-8D7F-4966-87E1-77001EB6B29E}");
            serializeContext->Class<MeshColliderComponent, BaseColliderComponent>()
                ->Version(1)
                ->Field("Configuration", &MeshColliderComponent::m_shapeConfiguration)
            ;
        }
    }

    MeshColliderComponent::MeshColliderComponent(const Physics::ColliderConfiguration& colliderConfiguration, 
        const Physics::PhysicsAssetShapeConfiguration& configuration)
        : BaseColliderComponent(colliderConfiguration)
        , m_shapeConfiguration(configuration)
    {

    }

    void MeshColliderComponent::Activate()
    {
        UpdateMeshAsset();
        MeshColliderComponentRequestsBus::Handler::BusConnect(GetEntityId());
        BaseColliderComponent::Activate();
    }

    void MeshColliderComponent::Deactivate()
    {
        BaseColliderComponent::Deactivate();
        MeshColliderComponentRequestsBus::Handler::BusDisconnect();
    }

    AZ::Data::Asset<Pipeline::MeshAsset> MeshColliderComponent::GetMeshAsset() const
    {
        return m_shapeConfiguration.m_asset.GetAs<Pipeline::MeshAsset>();
    }

    void MeshColliderComponent::GetStaticWorldSpaceMeshTriangles(AZStd::vector<AZ::Vector3>& verts, AZStd::vector<AZ::u32>& indices) const
    {
        AZ_Assert(false, "Run-time support for this function is not available");
    }

    Physics::MaterialId MeshColliderComponent::GetMaterialId() const
    {
        return m_configuration.m_materialSelection.GetMaterialId();
    }

    void MeshColliderComponent::SetMeshAsset(const AZ::Data::AssetId& id)
    {
        m_shapeConfiguration.m_asset.Create(id);
        UpdateMeshAsset();
    }

    void MeshColliderComponent::SetMaterialAsset(const AZ::Data::AssetId& id)
    {
        m_configuration.m_materialSelection.CreateMaterialLibrary(id);
    }

    void MeshColliderComponent::SetMaterialId(const Physics::MaterialId& id)
    {
        m_configuration.m_materialSelection.SetMaterialId(id);
    }

    AZStd::shared_ptr<Physics::ShapeConfiguration> MeshColliderComponent::CreateScaledShapeConfig()
    {
        if (m_shapeConfiguration.m_asset)
        {
            physx::PxBase* meshData = m_shapeConfiguration.m_asset.GetAs<Pipeline::MeshAsset>()->GetMeshData();
            AZ_Error("PhysX", meshData != nullptr, "Error. Invalid mesh collider asset. Name: %s", GetEntity()->GetName().c_str());

            if (meshData)
            {
                auto scaledShape = AZStd::make_shared<Physics::PhysicsAssetShapeConfiguration>(m_shapeConfiguration);
                scaledShape->m_scale = GetNonUniformScale();
                return scaledShape;
            }
        }
        // It is a valid case to have PhysXMesh shape but we don't create any configurations for this
        return nullptr;
    }

    void MeshColliderComponent::UpdateMeshAsset()
    {
        if (m_shapeConfiguration.m_asset.GetId().IsValid())
        {
            AZ::Data::AssetBus::MultiHandler::BusConnect(m_shapeConfiguration.m_asset.GetId());
            m_shapeConfiguration.m_asset.QueueLoad();
        }
    }
   

    void MeshColliderComponent::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        if (asset == m_shapeConfiguration.m_asset)
        {
            m_shapeConfiguration.m_asset = asset;
        }
    }

    void MeshColliderComponent::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        if (asset == m_shapeConfiguration.m_asset)
        {
            m_shapeConfiguration.m_asset = asset;
        }
    }

} // namespace PhysX
