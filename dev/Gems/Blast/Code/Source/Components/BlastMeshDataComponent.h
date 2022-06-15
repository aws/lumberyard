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
#pragma once

#include <AzCore/std/containers/vector.h>
#include <AzCore/Component/Component.h>

#include <AzCore/Asset/AssetCommon.h>
#include <LmbrCentral/Rendering/MaterialAsset.h>
#include <LmbrCentral/Rendering/MeshAsset.h>

namespace Blast
{
    class EditorBlastMeshDataComponent;

    //! An interface that is responsible for providing meshes of chunks.
    class BlastMeshData
    {
    public:
        virtual ~BlastMeshData() = default;

        virtual const AZ::Data::Asset<LmbrCentral::MeshAsset>& GetMeshAsset(size_t index) const = 0;
        virtual const AZStd::vector<AZ::Data::Asset<LmbrCentral::MeshAsset>>& GetMeshAssets() const = 0;
    };

    //! Component that stores meshes for the blast family to use during game time.
    class BlastMeshDataComponent
        : public AZ::Component
        , public BlastMeshData
    {
    public:
        AZ_COMPONENT(BlastMeshDataComponent, "{8910FB8D-D474-443B-93EC-84E4A595ADDF}", AZ::Component);

        BlastMeshDataComponent() = default;
        BlastMeshDataComponent(
            const AZStd::vector<AZ::Data::Asset<LmbrCentral::MeshAsset>>& meshAssets,
            AzFramework::SimpleAssetReference<LmbrCentral::MaterialAsset> material);
        ~BlastMeshDataComponent() = default;

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

        // AZ::Component interface implementation
        void Activate() override {}
        void Deactivate() override {}

        const AzFramework::SimpleAssetReference<LmbrCentral::MaterialAsset>& GetMaterial() const;
        const AZ::Data::Asset<LmbrCentral::MeshAsset>& GetMeshAsset(size_t index) const override;

        const AZStd::vector<AZ::Data::Asset<LmbrCentral::MeshAsset>>& GetMeshAssets() const override;

    private:
        // Reflected data
        AZStd::vector<AZ::Data::Asset<LmbrCentral::MeshAsset>> m_meshAssets;
        AzFramework::SimpleAssetReference<LmbrCentral::MaterialAsset> m_material;
    };
} // namespace Blast
