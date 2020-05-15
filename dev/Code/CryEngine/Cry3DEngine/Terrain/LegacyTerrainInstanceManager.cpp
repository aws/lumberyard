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

//From CryCommon
#include <IRenderer.h>

#include "terrain.h"
#include "LegacyTerrainInstanceManager.h"
#include "LegacyTerrainCVars.h"
#include <HeightmapUpdateNotificationBus.h>

namespace LegacyTerrain
{
    LegacyTerrainInstanceManager::LegacyTerrainInstanceManager()
    {
        CrySystemEventBus::Handler::BusConnect();
        LegacyTerrainInstanceRequestBus::Handler::BusConnect();
    }

    LegacyTerrainInstanceManager::~LegacyTerrainInstanceManager()
    {
        LegacyTerrainInstanceRequestBus::Handler::BusDisconnect();
        CrySystemEventBus::Handler::BusDisconnect();
    }

    ///////////////////////////////////////////////////////////////////////
    // CrySystemEventBus START
    void LegacyTerrainInstanceManager::OnCrySystemInitialized(ISystem& system, const SSystemInitParams& initParams)
    {
        LegacyTerrainBase::SetSystem(&system);
        LegacyTerrainBase::GetCVars()->RegisterCVars();
    }
    // CrySystemEventBus END
    ///////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////
    // LegacyTerrainInstanceRequestBus START
    bool LegacyTerrainInstanceManager::CreateTerrainSystem(uint8* octreeData, size_t octreeDataSize)
    {
        if (CTerrain::GetTerrain() != nullptr)
        {
            return false;
        }

        STerrainInfo terrainInfo;
        XmlNodeRef surfaceTypes = nullptr;
        AZ::IO::HandleType octreeFileHandle = AZ::IO::InvalidHandle;
        bool bSectorPalettes = false;
        EEndian eEndian = eLittleEndian;
        int nBytesLeft = 0;

        if (octreeData)
        {
            AZ_Assert(octreeDataSize, "Invalid octree buffer data size");
            nBytesLeft = gEnv->p3DEngine->GetLegacyTerrainLevelData(octreeData, terrainInfo, bSectorPalettes, eEndian);
        }
        else
        {
            nBytesLeft = gEnv->p3DEngine->GetLegacyTerrainLevelData(octreeFileHandle, terrainInfo, bSectorPalettes, eEndian, surfaceTypes);
        }
        if ((nBytesLeft < 1) || (terrainInfo.TerrainSize() == 0))
        {
            GetPak()->FClose(octreeFileHandle);
            return false;
        }

        if (!CTerrain::CreateTerrain(terrainInfo))
        {
            AZ_Error("LegacyTerrain", false, "Failed to allocate the terrain instance.");
            GetPak()->FClose(octreeFileHandle);
            return false;
        }


        int nNodesLoaded = 0;

        if (octreeData)
        {
            //Loading terrain data from a buffer in memory is an operation that occurs only within the Editor. The Editor manages the terrain
            //layers and the terrain macro texture.
            AZ_Assert(surfaceTypes == nullptr, "When the editor is enabled the terrain doesn't own the terrain surface layers");
            const bool loadMacroTexture = false;
            nNodesLoaded = CTerrain::GetTerrain()->Load_T(surfaceTypes, octreeData, nBytesLeft, terrainInfo, false, true, bSectorPalettes, eEndian, nullptr, loadMacroTexture);
        }
        else
        {
            //Loading terrain data from a file handle is an operation that occurs during game client runtime.
            AZ_Assert(surfaceTypes != nullptr, "Missing terrain surface layers");
            const bool loadMacroTexture = true;
            nNodesLoaded = CTerrain::GetTerrain()->Load_T(surfaceTypes, octreeFileHandle, nBytesLeft, terrainInfo, false, true, bSectorPalettes, eEndian, nullptr, loadMacroTexture);
        }
        AZ_TracePrintf("LegacyTerrain", "Terrain loaded with %d nodes", nNodesLoaded);
        GetPak()->FClose(octreeFileHandle);

        if (nNodesLoaded > 0)
        {
            AZ::HeightmapUpdateNotificationBus::Broadcast(&AZ::HeightmapUpdateNotificationBus::Events::HeightmapModified, AZ::Aabb::CreateNull());
            return true;
        }

        return false;
    }

    bool LegacyTerrainInstanceManager::CreateUninitializedTerrainSystem(const STerrainInfo& terrainInfo)
    {
        if (CTerrain::GetTerrain() != nullptr)
        {
            return false;
        }

        const bool success = CTerrain::CreateTerrain(terrainInfo);
        AZ_Error("LegacyTerrain", success, "Failed to allocate the terrain instance.");
        return success;
    }

    bool LegacyTerrainInstanceManager::IsTerrainSystemInstantiated() const
    {
        return (CTerrain::GetTerrain() != nullptr);
    }

    void LegacyTerrainInstanceManager::DestroyTerrainSystem()
    {
        if (CTerrain::GetTerrain())
        {
            CTerrain::DestroyTerrain();

            // Make sure the Octree doesn't retain any pointers into the terrain structures
            if (Get3DEngine()->IsObjectTreeReady())
            {
                Get3DEngine()->GetIObjectTree()->UpdateTerrainNodes();
            }

            AZ::HeightmapUpdateNotificationBus::Broadcast(&AZ::HeightmapUpdateNotificationBus::Events::HeightmapModified, AZ::Aabb::CreateNull());
        }
    }
    // LegacyTerrainInstanceRequestBus END
    ///////////////////////////////////////////////////////////////////////
}//namespace LegacyTerrain
