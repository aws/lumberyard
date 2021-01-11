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

#include <AzCore/Serialization/SerializeContext.h>
#include <Components/BlastMeshDataComponent.h>

namespace Blast
{
    BlastMeshDataComponent::BlastMeshDataComponent(
        const AZStd::vector<AZ::Data::Asset<LmbrCentral::MeshAsset>>& meshAssets,
        AzFramework::SimpleAssetReference<LmbrCentral::MaterialAsset> material)
        : m_meshAssets(meshAssets)
        , m_material(material)
    {
        for (auto& meshAsset : m_meshAssets)
        {
            meshAsset.SetAutoLoadBehavior(AZ::Data::AssetLoadBehavior::QueueLoad);
        }
    }

    void BlastMeshDataComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<BlastMeshDataComponent, AZ::Component>()
                ->Version(1)
                ->Field("MeshAssets", &BlastMeshDataComponent::m_meshAssets)
                ->Field("Material", &BlastMeshDataComponent::m_material);
        }
    }

    void BlastMeshDataComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("BlastMeshDataService"));
    }

    void BlastMeshDataComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("BlastMeshDataService"));
    }

    const AzFramework::SimpleAssetReference<LmbrCentral::MaterialAsset>& BlastMeshDataComponent::GetMaterial() const
    {
        return m_material;
    }

    const AZ::Data::Asset<LmbrCentral::MeshAsset>& BlastMeshDataComponent::GetMeshAsset(size_t index) const
    {
        if (index < m_meshAssets.size())
        {
            return m_meshAssets[index];
        }

        AZ_Warning(
            "Blast", false, "Tried to get mesh asset at index %zu outside of Blast mesh data asset array.", index);
        static AZ::Data::Asset<LmbrCentral::MeshAsset> invalidAsset;
        return invalidAsset;
    }

    const AZStd::vector<AZ::Data::Asset<LmbrCentral::MeshAsset>>& BlastMeshDataComponent::GetMeshAssets() const
    {
        return m_meshAssets;
    }
} // namespace Blast
