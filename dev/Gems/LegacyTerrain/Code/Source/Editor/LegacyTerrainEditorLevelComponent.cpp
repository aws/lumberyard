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

#include "LegacyTerrain_precompiled.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

#include "LegacyTerrainEditorLevelComponent.h"
#include "terrain.h"

//From EditorLib/Include
#include <EditorDefs.h>
#include <Terrain/Bus/LegacyTerrainBus.h>
#include <AzFramework/Terrain/TerrainDataRequestBus.h>

namespace LegacyTerrain
{
    void LegacyTerrainEditorLevelComponent::Reflect(AZ::ReflectContext* context)
    {
        BaseClassType::Reflect(context);

        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

        if (serializeContext)
        {
            serializeContext->Class<LegacyTerrainEditorLevelComponent, BaseClassType>()
                ->Version(0)
            ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<LegacyTerrainEditorLevelComponent>(
                    "Legacy Terrain", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Terrain")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZStd::vector<AZ::Crc32>({ AZ_CRC("Level", 0x9aeacc13) }))
                    ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/LegacyTerrain.svg")
                ;
            }
        }
    }


    void LegacyTerrainEditorLevelComponent::Init()
    {
        BaseClassType::Init();
    }

    void LegacyTerrainEditorLevelComponent::Activate()
    {
        AZ_Assert(CTerrain::GetTerrain() == nullptr, "The Terrain System was already initialized");
        m_component.ActivateTerrainSystemWithEditor();
    }

    void LegacyTerrainEditorLevelComponent::Deactivate()
    {
        if (CTerrain::GetTerrain() == nullptr)
        {
            return;
        }

        m_component.DeactivateTerrainSystemWithEditor();
    }

    AZ::u32 LegacyTerrainEditorLevelComponent::ConfigurationChanged()
    {
        return BaseClassType::ConfigurationChanged();
    }
}
