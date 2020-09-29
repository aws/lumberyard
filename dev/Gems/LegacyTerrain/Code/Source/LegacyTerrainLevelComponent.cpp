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

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>

#include <AzFramework/Terrain/TerrainDataRequestBus.h>

//From CryCommon
#include <Terrain/Bus/LegacyTerrainBus.h>

#include <LegacyTerrainLevelComponent.h>
#include "terrain.h"

namespace LegacyTerrain
{
    void LegacyTerrainLevelConfig::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<LegacyTerrainLevelConfig, AZ::ComponentConfig>()
                ->Version(1);

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
            {
                edit->Class<LegacyTerrainLevelConfig>(
                    "Legacy Terrain System Component", "Data required for the legacy terrain system to run")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZStd::vector<AZ::Crc32>({ AZ_CRC("Level", 0x9aeacc13) }))
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true);
            }
        }
    }

    void LegacyTerrainLevelComponent::Reflect(AZ::ReflectContext* context)
    {
        LegacyTerrainLevelConfig::Reflect(context);

        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<LegacyTerrainLevelComponent, AZ::Component>()
                ->Version(0)
                ->Field("Configuration", &LegacyTerrainLevelComponent::m_configuration)
                ;
        }
    }

    void LegacyTerrainLevelComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("TerrainService", 0x28ee7719));
    }

    void LegacyTerrainLevelComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("TerrainService", 0x28ee7719));
    }

    void LegacyTerrainLevelComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        AZ_UNUSED(required);
    }

    void LegacyTerrainLevelComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        AZ_UNUSED(dependent);
    }

    LegacyTerrainLevelComponent::LegacyTerrainLevelComponent(const LegacyTerrainLevelConfig& configuration)
        : m_configuration(configuration)
    {
    }

    void LegacyTerrainLevelComponent::Init()
    {
        if (!LegacyTerrainBase::GetSystem())
        {
            AZ_Assert(gEnv, "gEnv is required to initialize the Legacy Terrain level component");
            LegacyTerrainBase::SetSystem(gEnv->pSystem);
            LegacyTerrainBase::GetCVars()->RegisterCVars();
        }
    }

    void LegacyTerrainLevelComponent::Activate()
    {
        AZ_Assert(CTerrain::GetTerrain() == nullptr, "The Terrain System was already initialized");

        // If we're running in the Editor in Game Mode, make sure the Editor version of terrain data is used to instantiate our runtime terrain.
        // This allows us to see edited but unsaved data.  Otherwise, we'll see the last exported version of the runtime terrain.
        if (LegacyTerrainEditorDataRequestBus::HasHandlers())
        {
            ActivateTerrainSystemWithEditor();
            return;
        }

        AzFramework::Terrain::TerrainDataNotificationBus::Broadcast(&AzFramework::Terrain::TerrainDataNotifications::OnTerrainDataCreateBegin);

        STerrainInfo terrainInfo;
        XmlNodeRef surfaceTypes = nullptr;
        AZ::IO::HandleType octreeFileHandle = AZ::IO::InvalidHandle;
        bool bSectorPalettes = false;
        EEndian eEndian = eLittleEndian;
        int nBytesLeft = 0;

        nBytesLeft = Get3DEngine()->GetLegacyTerrainLevelData(octreeFileHandle, terrainInfo, bSectorPalettes, eEndian, surfaceTypes);

        if ((nBytesLeft < 1) || (terrainInfo.TerrainSize() == 0))
        {
            GetPak()->FClose(octreeFileHandle);
            AZ_Error("LegacyTerrain", false, "Failed to read legacy terrain info from the engine.");
            return;
        }

        if (!CTerrain::CreateTerrain(terrainInfo))
        {
            AZ_Error("LegacyTerrain", false, "Failed to allocate the terrain instance.");
            GetPak()->FClose(octreeFileHandle);
            return;
        }


        int nNodesLoaded = 0;

        //Loading terrain data from a file handle is an operation that occurs during game client runtime.
        AZ_Assert(surfaceTypes != nullptr, "Missing terrain surface layers");
        const bool loadMacroTexture = true;
        nNodesLoaded = CTerrain::GetTerrain()->Load_T(surfaceTypes, octreeFileHandle, nBytesLeft, terrainInfo, false, true, bSectorPalettes, eEndian, nullptr, loadMacroTexture);

        AZ_TracePrintf("LegacyTerrain", "Terrain loaded with %d nodes", nNodesLoaded);
        GetPak()->FClose(octreeFileHandle);

        if (nNodesLoaded > 0)
        {
            AZ::HeightmapUpdateNotificationBus::Broadcast(&AZ::HeightmapUpdateNotificationBus::Events::HeightmapModified, AZ::Aabb::CreateNull());
            AzFramework::Terrain::TerrainDataNotificationBus::Broadcast(&AzFramework::Terrain::TerrainDataNotifications::OnTerrainDataCreateEnd);
            return;
        }

        AZ_Error("LegacyTerrain", false, "Failed to load terrain heightmap data.");
        CTerrain::DestroyTerrain();
    }

    void LegacyTerrainLevelComponent::Deactivate()
    {
        if (CTerrain::GetTerrain() == nullptr)
        {
            return;
        }

        // If we're running in the Editor in Game Mode, the Editor terrain data needs to know that we're destroying the terrain system too.
        if (LegacyTerrainEditorDataRequestBus::HasHandlers())
        {
            DeactivateTerrainSystemWithEditor();
            return;
        }

        AzFramework::Terrain::TerrainDataNotificationBus::Broadcast(&AzFramework::Terrain::TerrainDataNotifications::OnTerrainDataDestroyBegin);

        //Before removing the terrain system from memory, it is important to make sure
        //there are no pending culling jobs because removing the terrain causes the Octree culling jobs
        //to recalculate and those jobs may access Octree nodes that don't exist anymore.
        Get3DEngine()->WaitForCullingJobsCompletion();

        CTerrain::DestroyTerrain();

        // Make sure the Octree doesn't retain any pointers into the terrain structures
        if (LegacyTerrainBase::Get3DEngine()->IsObjectTreeReady())
        {
            LegacyTerrainBase::Get3DEngine()->GetIObjectTree()->UpdateTerrainNodes();
        }

        AzFramework::Terrain::TerrainDataNotificationBus::Broadcast(&AzFramework::Terrain::TerrainDataNotifications::OnTerrainDataDestroyEnd);
    }

    bool LegacyTerrainLevelComponent::ReadInConfig(const AZ::ComponentConfig* baseConfig)
    {
        if (auto config = azrtti_cast<const LegacyTerrainLevelConfig*>(baseConfig))
        {
            m_configuration = *config;
            return true;
        }
        return false;
    }

    bool LegacyTerrainLevelComponent::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
    {
        if (auto config = azrtti_cast<LegacyTerrainLevelConfig*>(outBaseConfig))
        {
            *config = m_configuration;
            return true;
        }
        return false;
    }

    bool LegacyTerrainLevelComponent::ActivateTerrainSystemWithEditor()
    {
        //Let's get the terrain info from the Editor's TerrainManager
        STerrainInfo terrainInfo;
        bool success = false;
        LegacyTerrainEditorDataRequestBus::BroadcastResult(success, &LegacyTerrainEditorDataRequests::GetTerrainInfo, terrainInfo);
        AZ_Assert(success, "Failed to get terrain info.");

        // Announce to those who care that the terrain system will be created. 
        AzFramework::Terrain::TerrainDataNotificationBus::Broadcast(&AzFramework::Terrain::TerrainDataNotifications::OnTerrainDataCreateBegin);

        success = CTerrain::CreateTerrain(terrainInfo);
        AZ_Assert(success, "Failed to allocate the terrain instance.");

        LegacyTerrainEditorDataRequestBus::BroadcastResult(success, &LegacyTerrainEditorDataRequests::InitializeTerrainSystemFromEditorData);
        AZ_Error("LegacyTerrain", success, "Failed to initialize the legacy terrain system");

        if (success)
        {
            // Announce to those who care that the terrain system was created. 
            AzFramework::Terrain::TerrainDataNotificationBus::Broadcast(&AzFramework::Terrain::TerrainDataNotifications::OnTerrainDataCreateEnd);
        }
        else
        {
            CTerrain::DestroyTerrain();
        }

        return success;
    }

    void LegacyTerrainLevelComponent::DeactivateTerrainSystemWithEditor()
    {
        AzFramework::Terrain::TerrainDataNotificationBus::Broadcast(&AzFramework::Terrain::TerrainDataNotifications::OnTerrainDataDestroyBegin);

        //Before removing the terrain system from memory, it is important to make sure
        //there are no pending culling jobs because removing the terrain causes the Octree culling jobs
        //to recalculate and those jobs may access Octree nodes that don't exist anymore.
        LegacyTerrainBase::Get3DEngine()->WaitForCullingJobsCompletion();

        LegacyTerrainEditorDataRequestBus::Broadcast(&LegacyTerrainEditorDataRequests::DestroyTerrainSystem);

        CTerrain::DestroyTerrain();

        // Make sure the Octree doesn't retain any pointers into the terrain structures
        if (LegacyTerrainBase::Get3DEngine()->IsObjectTreeReady())
        {
            LegacyTerrainBase::Get3DEngine()->GetIObjectTree()->UpdateTerrainNodes();
        }

        AZ::HeightmapUpdateNotificationBus::Broadcast(&AZ::HeightmapUpdateNotificationBus::Events::HeightmapModified, AZ::Aabb::CreateNull());

        AzFramework::Terrain::TerrainDataNotificationBus::Broadcast(&AzFramework::Terrain::TerrainDataNotifications::OnTerrainDataDestroyEnd);
    }
}
