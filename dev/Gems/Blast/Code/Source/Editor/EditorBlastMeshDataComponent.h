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

#include <Asset/BlastSliceAsset.h>
#include <Components/BlastMeshDataComponent.h>
#include <Rendering/EditorMeshComponent.h>

namespace Blast
{
    //! This class is a used for setting and storing meshes and material for chunks of an entity with
    //! Blast Family component during Editor time. It renders mesh of a root chunk in the viewport.
    class EditorBlastMeshDataComponent : public LmbrCentral::EditorMeshComponent
    {
    public:
        AZ_COMPONENT(
            EditorBlastMeshDataComponent, "{2DA6B11A-5091-423A-AC1D-7F03C46DBF43}", LmbrCentral::EditorMeshComponent);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

        static void Reflect(AZ::ReflectContext* context);

        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;

        // EditorComponentBase
        void BuildGameEntity(AZ::Entity* gameEntity) override;

        const AZ::Data::Asset<BlastSliceAsset>& GetBlastSliceAsset() const;

        const AZStd::vector<AZ::Data::Asset<LmbrCentral::MeshAsset>>& GetMeshAssets() const;

        const AzFramework::SimpleAssetReference<LmbrCentral::MaterialAsset>& GetMeshMaterial() const;

    private:
        void OnSliceAssetChanged();
        void OnMeshAssetsChanged();
        void OnMaterialChanged();
        AZ::Crc32 GetMeshAssetsVisibility() const;
        void OnMeshAssetsVisibilityChanged();

        //////////////////////////////////////////////////////////////////////////
        // Reflected data
        bool m_showMeshAssets = false;
        AZ::Data::Asset<BlastSliceAsset> m_blastSliceAsset;
        AZStd::vector<AZ::Data::Asset<LmbrCentral::MeshAsset>> m_meshAssets;
        AzFramework::SimpleAssetReference<LmbrCentral::MaterialAsset> m_material;
        //////////////////////////////////////////////////////////////////////////

        _smart_ptr<IMaterial> m_materialOverride;
    };
} // namespace Blast
