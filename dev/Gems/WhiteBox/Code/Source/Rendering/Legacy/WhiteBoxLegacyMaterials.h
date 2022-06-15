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

#include "WhiteBoxLegacyMaterialsRequestBus.h"

#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/hash.h>
#include <CryCommon/IMaterial.h>

namespace WhiteBox
{
    //! Cache of shared materials used by the White Box Component.
    class WhiteBoxLegacyMaterials : public WhiteBoxLegacyMaterialsRequestBus::Handler
    {
    public:
        void Connect();
        void Disconnect();

        // WhiteBoxLegacyMaterialsRequestBus overrides ...
        _smart_ptr<IMaterial> FindMaterial(const WhiteBoxLegacyMaterialKey& materialKey) override;
        void AddMaterial(const WhiteBoxLegacyMaterialKey& materialKey, _smart_ptr<IMaterial> material) override;

    private:
        AZStd::unordered_map<WhiteBoxLegacyMaterialKey, _smart_ptr<IMaterial>> m_materials;
    };

    void WhiteBoxLegacyMaterials::Connect()
    {
        WhiteBoxLegacyMaterialsRequestBus::Handler::BusConnect();
    }

    void WhiteBoxLegacyMaterials::Disconnect()
    {
        WhiteBoxLegacyMaterialsRequestBus::Handler::BusDisconnect();
    }

    _smart_ptr<IMaterial> WhiteBoxLegacyMaterials::FindMaterial(const WhiteBoxLegacyMaterialKey& materialKey)
    {
        if (auto material = m_materials.find(materialKey);
            material != m_materials.end())
        {
            return material->second;
        }

        return nullptr;
    }

    void WhiteBoxLegacyMaterials::AddMaterial(const WhiteBoxLegacyMaterialKey& materialKey, _smart_ptr<IMaterial> material)
    {
        m_materials[materialKey] = material;
    }
} // namespace WhiteBox
