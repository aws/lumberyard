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

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <Editor/EditorBlastMeshDataComponent.h>

namespace Blast
{
    void EditorBlastMeshDataComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("BlastMeshDataService"));
    }

    void EditorBlastMeshDataComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC("TransformService", 0x8ee22c50));
    }

    void EditorBlastMeshDataComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        dependent.push_back(AZ_CRC("EditorVisibilityService", 0x90888caf));
    }

    void EditorBlastMeshDataComponent::GetIncompatibleServices(
        AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("BlastMeshDataService"));
    }

    void EditorBlastMeshDataComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<EditorBlastMeshDataComponent, AzToolsFramework::Components::EditorComponentBase>()
                ->Version(3)
                ->Field("Show Mesh Assets", &EditorBlastMeshDataComponent::m_showMeshAssets)
                ->Field("Mesh Assets", &EditorBlastMeshDataComponent::m_meshAssets)
                ->Field("Material", &EditorBlastMeshDataComponent::m_material)
                ->Field("Blast Slice", &EditorBlastMeshDataComponent::m_blastSliceAsset);

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<EditorBlastMeshDataComponent>(
                      "Blast Family Mesh Data", "Used to keep track of mesh assets for a Blast family")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Destruction")
                    ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/Box.png")
                    ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/Box.png")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                    ->Attribute(
                        AZ::Edit::Attributes::HelpPageURL,
                        "https://docs.aws.amazon.com/lumberyard/latest/userguide/component-blast-actor.html")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(
                        AZ::Edit::UIHandlers::CheckBox, &EditorBlastMeshDataComponent::m_showMeshAssets,
                        "Show mesh assets", "Allows manual editing of mesh assets")
                    ->Attribute(
                        AZ::Edit::Attributes::ChangeNotify,
                        &EditorBlastMeshDataComponent::OnMeshAssetsVisibilityChanged)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &EditorBlastMeshDataComponent::m_meshAssets, "Mesh assets",
                        "Mesh assets needed for each Blast chunk")
                    ->Attribute(
                        AZ::Edit::Attributes::Visibility, &EditorBlastMeshDataComponent::GetMeshAssetsVisibility)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, false)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorBlastMeshDataComponent::OnMeshAssetsChanged)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &EditorBlastMeshDataComponent::m_material, "Material",
                        "Material to be applied to all Blast chunks")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorBlastMeshDataComponent::OnMaterialChanged)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &EditorBlastMeshDataComponent::m_blastSliceAsset, "Blast Slice",
                        "Slice override to fill out meshes and material")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorBlastMeshDataComponent::OnSliceAssetChanged);
            }
        }
    }

    void EditorBlastMeshDataComponent::Activate()
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::System);
        OnMeshAssetsChanged();
        OnMaterialChanged();

        EditorMeshComponent::Activate();
    }

    void EditorBlastMeshDataComponent::Deactivate()
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::System);
        EditorMeshComponent::Deactivate();

        m_materialOverride = nullptr;
    }

    void EditorBlastMeshDataComponent::OnSliceAssetChanged()
    {
        if (!m_blastSliceAsset.GetId().IsValid())
        {
            return;
        }

        using namespace AZ::Data;
        using namespace LmbrCentral;

        const bool queueLoadData = true;
        const bool loadBlocking = true;
        const AssetId blastAssetId = m_blastSliceAsset.GetId();
        m_blastSliceAsset =
            AssetManager::Instance().GetAsset<BlastSliceAsset>(blastAssetId, queueLoadData, nullptr, loadBlocking);

        // load up the new mesh list
        m_meshAssets.clear();
        for (const auto& meshId : m_blastSliceAsset.Get()->GetMeshIdList())
        {
            auto meshAsset = AssetManager::Instance().GetAsset<MeshAsset>(meshId, queueLoadData, nullptr, loadBlocking);
            if (meshAsset)
            {
                m_meshAssets.push_back(meshAsset);
            }
        }

        if (!m_meshAssets.empty() && m_meshAssets[0].GetId().IsValid())
        {
            SetMeshAsset(m_meshAssets[0].GetId());
        }

        OnMaterialChanged();

        AzToolsFramework::ToolsApplicationEvents::Bus::Broadcast(
            &AzToolsFramework::ToolsApplicationEvents::InvalidatePropertyDisplay, AzToolsFramework::Refresh_EntireTree);
    }

    void EditorBlastMeshDataComponent::OnMeshAssetsChanged()
    {
        if (!m_meshAssets.empty())
        {
            if (m_meshAssets[0].GetId().IsValid())
            {
                SetMeshAsset(m_meshAssets[0].GetId());
            }

            for (auto& meshAsset : m_meshAssets)
            {
                meshAsset.SetAutoLoadBehavior(AZ::Data::AssetLoadBehavior::QueueLoad);
            }
        }
    }

    void EditorBlastMeshDataComponent::OnMaterialChanged()
    {
        const AZStd::string& materialOverridePath = m_material.GetAssetPath();
        if (!materialOverridePath.empty())
        {
            m_materialOverride = gEnv->p3DEngine->GetMaterialManager()->LoadMaterial(materialOverridePath.c_str());

            AZ_Warning(
                "BlastMeshDataComponent",
                m_materialOverride != gEnv->p3DEngine->GetMaterialManager()->GetDefaultMaterial(),
                "Failed to load override Material \"%s\".", materialOverridePath.c_str());
        }
        else
        {
            m_materialOverride = nullptr;
        }

        SetMaterial(m_materialOverride);

        // update each chunk in the slice asset
        for (auto& meshAsset : m_meshAssets)
        {
            if (meshAsset.IsReady())
            {
                meshAsset.Get()->m_statObj->SetMaterial(m_materialOverride);
            }
        }
    }

    AZ::Crc32 EditorBlastMeshDataComponent::GetMeshAssetsVisibility() const
    {
        return m_showMeshAssets ? AZ::Edit::PropertyVisibility::Show : AZ::Edit::PropertyVisibility::Hide;
    }

    void EditorBlastMeshDataComponent::OnMeshAssetsVisibilityChanged()
    {
        AzToolsFramework::ToolsApplicationEvents::Bus::Broadcast(
            &AzToolsFramework::ToolsApplicationEvents::InvalidatePropertyDisplay, AzToolsFramework::Refresh_EntireTree);
    }

    void EditorBlastMeshDataComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        gameEntity->CreateComponent<BlastMeshDataComponent>(m_meshAssets, m_material);
    }

    const AZ::Data::Asset<BlastSliceAsset>& EditorBlastMeshDataComponent::GetBlastSliceAsset() const
    {
        return m_blastSliceAsset;
    }

    const AZStd::vector<AZ::Data::Asset<LmbrCentral::MeshAsset>>& EditorBlastMeshDataComponent::GetMeshAssets() const
    {
        return m_meshAssets;
    }

    const AzFramework::SimpleAssetReference<LmbrCentral::MaterialAsset>& EditorBlastMeshDataComponent::GetMeshMaterial()
        const
    {
        return m_material;
    }
} // namespace Blast
