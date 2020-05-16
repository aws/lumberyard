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
        bool isInstantiated = false;
        LegacyTerrainInstanceRequestBus::BroadcastResult(isInstantiated, &LegacyTerrainInstanceRequests::IsTerrainSystemInstantiated);
        if (isInstantiated)
        {
            AZ_Warning("LegacyTerrain", false, "The legacy terrain system was already instantiated");
            return;
        }

        AzFramework::Terrain::TerrainDataNotificationBus::Broadcast(&AzFramework::Terrain::TerrainDataNotifications::OnTerrainDataCreateBegin);

        // If we're running in the Editor with terrain editor code compiled in, make sure the Editor version of terrain data is used
        // to instantiate our runtime terrain.  This allows us to see edited but unsaved data.  Otherwise, we'll see the last exported
        // version of the runtime terrain.
        if (LegacyTerrainEditorDataRequestBus::HasHandlers())
        {
            LegacyTerrainEditorDataRequestBus::BroadcastResult(isInstantiated, &LegacyTerrainEditorDataRequests::CreateTerrainSystemFromEditorData);
        }
        else
        {
            LegacyTerrainInstanceRequestBus::BroadcastResult(isInstantiated, &LegacyTerrainInstanceRequests::CreateTerrainSystem, (uint8*)nullptr, 0);
        }

        AzFramework::Terrain::TerrainDataNotificationBus::Broadcast(&AzFramework::Terrain::TerrainDataNotifications::OnTerrainDataCreateEnd);

        AZ_Error("LegacyTerrain", isInstantiated, "Failed to initialize the legacy terrain system");
    }

    void LegacyTerrainEditorLevelComponent::Deactivate()
    {
        bool isInstantiated = false;
        LegacyTerrainInstanceRequestBus::BroadcastResult(isInstantiated, &LegacyTerrainInstanceRequests::IsTerrainSystemInstantiated);
        if (!isInstantiated)
        {
            return;
        }

        AzFramework::Terrain::TerrainDataNotificationBus::Broadcast(&AzFramework::Terrain::TerrainDataNotifications::OnTerrainDataDestroyBegin);

        //Before removing the terrain system from memory, it is important to make sure
        //there are no pending culling jobs because removing the terrain causes the Octree culling jobs
        //to recalculate and those jobs may access Octree nodes that don't exist anymore.
        GetIEditor()->Get3DEngine()->WaitForCullingJobsCompletion();

        // If we're running in the Editor with terrain editing compiled in, the Editor terrain data needs to know that
        // we're destroying the terrain system too.
        if (LegacyTerrainEditorDataRequestBus::HasHandlers())
        {
            LegacyTerrainEditorDataRequestBus::Broadcast(&LegacyTerrainEditorDataRequests::DestroyTerrainSystem);
        }
        else
        {
            LegacyTerrainInstanceRequestBus::Broadcast(&LegacyTerrainInstanceRequests::DestroyTerrainSystem);
        }

        AzFramework::Terrain::TerrainDataNotificationBus::Broadcast(&AzFramework::Terrain::TerrainDataNotifications::OnTerrainDataDestroyEnd);
    }

    AZ::u32 LegacyTerrainEditorLevelComponent::ConfigurationChanged()
    {
        return BaseClassType::ConfigurationChanged();
    }
}
