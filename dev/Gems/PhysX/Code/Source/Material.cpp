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
#include <AzCore/std/smart_ptr/make_shared.h>
#include <Source/Material.h>
#include <PxPhysicsAPI.h>

namespace PhysX
{
    Material::Material(Material&& material) 
        : m_pxMaterial(AZStd::move(material.m_pxMaterial))
        , m_surfaceType(material.m_surfaceType)
        , m_surfaceString(AZStd::move(material.m_surfaceString))
    {
        m_pxMaterial->userData = this;
    }

    Material& Material::operator=(Material&& material)
    {
        m_pxMaterial = AZStd::move(material.m_pxMaterial);
        m_surfaceType = material.m_surfaceType;
        m_surfaceString = AZStd::move(material.m_surfaceString);

        return *this;
    }

    static Material::CombineMode FromPxCombineMode(physx::PxCombineMode::Enum pxMode)
    {
        switch (pxMode)
        {
        case physx::PxCombineMode::eAVERAGE: return Material::CombineMode::Average;
        case physx::PxCombineMode::eMULTIPLY: return Material::CombineMode::Multiply;
        case physx::PxCombineMode::eMAX: return Material::CombineMode::Maximum;
        case physx::PxCombineMode::eMIN: return Material::CombineMode::Minimum;
        default: return Material::CombineMode::Average;
        }
    }

    static physx::PxCombineMode::Enum ToPxCombineMode(Material::CombineMode mode)
    {
        switch (mode)
        {
        case Material::CombineMode::Average: return physx::PxCombineMode::eAVERAGE;
        case Material::CombineMode::Multiply: return physx::PxCombineMode::eMULTIPLY;
        case Material::CombineMode::Maximum: return physx::PxCombineMode::eMAX;
        case Material::CombineMode::Minimum: return physx::PxCombineMode::eMIN;
        default: return physx::PxCombineMode::eAVERAGE;
        }
    }

    Material::Material(const Physics::MaterialConfiguration& materialConfiguration)
    {
        auto pxMaterial = PxGetPhysics().createMaterial(
            materialConfiguration.m_staticFriction,
            materialConfiguration.m_dynamicFriction,
            materialConfiguration.m_restitution
        );

        auto materialDestructor = [](auto material)
        {
            material->release();
            material->userData = nullptr;
        };

        pxMaterial->setFrictionCombineMode(ToPxCombineMode(materialConfiguration.m_frictionCombine));
        pxMaterial->setRestitutionCombineMode(ToPxCombineMode(materialConfiguration.m_restitutionCombine));
        pxMaterial->userData = this;

        m_pxMaterial = PxMaterialUniquePtr(pxMaterial, materialDestructor);
        m_surfaceType = AZ::Crc32(materialConfiguration.m_surfaceType.c_str());
        m_surfaceString = materialConfiguration.m_surfaceType;

        Physics::LegacySurfaceTypeRequestsBus::BroadcastResult(
            m_cryEngineSurfaceId, 
            &Physics::LegacySurfaceTypeRequestsBus::Events::GetLegacySurfaceTypeFronName,
            m_surfaceString);
    }

    void Material::UpdateWithConfiguration(const Physics::MaterialConfiguration& configuration)
    {
        AZ_Assert(m_pxMaterial != nullptr, "Material can't be null!");

        m_pxMaterial->setRestitution(configuration.m_restitution);
        m_pxMaterial->setStaticFriction(configuration.m_staticFriction);
        m_pxMaterial->setDynamicFriction(configuration.m_dynamicFriction);

        m_pxMaterial->setFrictionCombineMode(ToPxCombineMode(configuration.m_frictionCombine));
        m_pxMaterial->setRestitutionCombineMode(ToPxCombineMode(configuration.m_restitutionCombine));

        m_surfaceType = AZ::Crc32(configuration.m_surfaceType.c_str());
        m_surfaceString = configuration.m_surfaceType;

        Physics::LegacySurfaceTypeRequestsBus::BroadcastResult(
            m_cryEngineSurfaceId,
            &Physics::LegacySurfaceTypeRequestsBus::Events::GetLegacySurfaceTypeFronName,
            m_surfaceString);
    }

    physx::PxMaterial* Material::GetPxMaterial()
    {
        return m_pxMaterial.get();
    }

    AZ::Crc32 Material::GetSurfaceType() const
    {
        return m_surfaceType;
    }

    void Material::SetSurfaceType(AZ::Crc32 surfaceType)
    {
        m_surfaceType = surfaceType;
    }

    float Material::GetDynamicFriction() const
    {
        return m_pxMaterial ? m_pxMaterial->getDynamicFriction() : 0.0f;
    }

    void Material::SetDynamicFriction(float dynamicFriction)
    {
        if (m_pxMaterial)
        {
            m_pxMaterial->setDynamicFriction(dynamicFriction);
        }
    }

    float Material::GetStaticFriction() const
    {
        return m_pxMaterial ? m_pxMaterial->getStaticFriction() : 0.0f;
    }

    void Material::SetStaticFriction(float staticFriction)
    {
        if (m_pxMaterial)
        {
            m_pxMaterial->setStaticFriction(AZStd::clamp(staticFriction, 0.0f, 1.0f));
        }
    }

    float Material::GetRestitution() const
    {
        return m_pxMaterial ? m_pxMaterial->getRestitution() : 0.0f;
    }

    void Material::SetRestitution(float restitution)
    {
        if (m_pxMaterial)
        {
            m_pxMaterial->setRestitution(AZStd::clamp(restitution, 0.0f, 1.0f));
        }
    }

    Material::CombineMode Material::GetFrictionCombineMode() const
    {
        return m_pxMaterial ? FromPxCombineMode(m_pxMaterial->getFrictionCombineMode()) : CombineMode::Average;
    }

    void Material::SetFrictionCombineMode(CombineMode mode)
    {
        if (m_pxMaterial)
        {
            m_pxMaterial->setFrictionCombineMode(ToPxCombineMode(mode));
        }
    }

    Material::CombineMode Material::GetRestitutionCombineMode() const
    {
        return m_pxMaterial ? FromPxCombineMode(m_pxMaterial->getRestitutionCombineMode()) : CombineMode::Average;
    }

    void Material::SetRestitutionCombineMode(CombineMode mode)
    {
        if (m_pxMaterial)
        {
            m_pxMaterial->setRestitutionCombineMode(ToPxCombineMode(mode));
        }
    }

    AZ::u32 Material::GetCryEngineSurfaceId() const
    {
        return m_cryEngineSurfaceId;
    }

    void* Material::GetNativePointer()
    {
        return m_pxMaterial.get();
    }

    MaterialsManager::MaterialsManager()
    {
    }

    MaterialsManager::~MaterialsManager()
    {
    }

    void MaterialsManager::Connect()
    {
        MaterialManagerRequestsBus::Handler::BusConnect();
    }

    void MaterialsManager::Disconnect()
    {
        MaterialManagerRequestsBus::Handler::BusDisconnect();
    }

    AZStd::vector<AZStd::shared_ptr<Material>> MaterialsManager::GetMaterials(const Physics::MaterialSelection& materialSelection)
    {
        AZStd::vector<AZStd::shared_ptr<Material>> materials;
        materials.reserve(materialSelection.GetMaterialIdsAssignedToSlots().size());

        AZStd::vector<physx::PxMaterial*> pxMaterials;
        GetPxMaterials(materialSelection, pxMaterials);

        for (auto pxMaterial : pxMaterials)
        {
            Material* material = static_cast<Material*>(pxMaterial->userData);
            materials.emplace_back(static_cast<Material*>(pxMaterial->userData)->shared_from_this());
        }

        return materials;
    }

    void MaterialsManager::GetPxMaterials(const Physics::MaterialSelection& materialSelection, AZStd::vector<physx::PxMaterial*>& outMaterials)
    {
        outMaterials.clear();
        if (materialSelection.GetMaterialIdsAssignedToSlots().empty())
        {
            // if the materialSelection is invalid we still
            // return a default material as a fallback behavior
            outMaterials.push_back(GetDefaultMaterial()->GetPxMaterial());
            return;
        }

        outMaterials.reserve(materialSelection.GetMaterialIdsAssignedToSlots().size());

        for (const auto& id : materialSelection.GetMaterialIdsAssignedToSlots())
        {
            Physics::MaterialFromAssetConfiguration configuration;
            if (materialSelection.GetMaterialConfiguration(configuration, id))
            {
                auto materialId = configuration.m_id;

                if (materialId.IsNull())
                {
                    materialId = Physics::MaterialId::Create();
                }

                auto iterator = m_materialsFromAssets.find(materialId.GetUuid());
                if (iterator != m_materialsFromAssets.end())
                {
                    iterator->second->UpdateWithConfiguration(configuration.m_configuration);
                    outMaterials.push_back(iterator->second->GetPxMaterial());
                }
                else
                {
                    auto newMaterial = AZStd::make_shared<Material>(configuration.m_configuration);
                    m_materialsFromAssets.emplace(materialId.GetUuid(), newMaterial);

                    outMaterials.push_back(newMaterial->GetPxMaterial());
                }
            }
            else
            {
                // It is important to return exactly the amount of materials specified in materialSelection
                // If a number of materials different to what was cooked is assigned on a physx mesh it will lead to undefined 
                // behavior and subtle bugs. Unfortunately, there's no warning or assertion on physx side at the shape creation time, 
                // nor mention of this in the documentation
                outMaterials.push_back(GetDefaultMaterial()->GetPxMaterial());
            }
        }
    }

    const AZStd::shared_ptr<Material>& MaterialsManager::GetDefaultMaterial()
    {
        if (!m_defaultMaterial)
        {
            m_defaultMaterial = AZStd::make_shared<Material>(Physics::MaterialConfiguration());
        }

        return m_defaultMaterial;
    }

    void MaterialsManager::ReleaseAllMaterials()
    {
        m_defaultMaterial = nullptr;
        m_materialsFromAssets.clear();
    }
}