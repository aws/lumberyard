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

#include <ISystem.h>

#include "LegacyTerrainCVars.h"

void LegacyTerrainCVars::RegisterCVars()
{
    if (m_isRegistered)
    {
        return;
    }

    REGISTER_CVAR(e_TerrainLog, 0, VF_CHEAT,
        "Debug");
    REGISTER_CVAR(e_TerrainBBoxes, 0, VF_CHEAT,
        "Show terrain nodes bboxes");
    REGISTER_CVAR(e_TerrainDrawThisSectorOnly, 0, VF_CHEAT,
        "1 - render only sector where camera is and objects registered in sector 00\n"
        "2 - render only sector where camera is");
    REGISTER_CVAR(e_TerrainTextureDebug, 0, VF_CHEAT,
        "Debug");
    REGISTER_CVAR(e_CoverageBufferTerrain, 0, VF_NULL,
        "Activates usage of coverage buffer for terrain");
    REGISTER_CVAR(e_TerrainDetailMaterialsDebug, 0, VF_CHEAT,
        "Shows number of materials in use per terrain sector");
    REGISTER_CVAR(e_TerrainTextureStreamingPoolItemsNum, 64, VF_REQUIRE_LEVEL_RELOAD,
        "Specifies number of textures in terrain base texture streaming pool");
    REGISTER_CVAR(e_TerrainLodRatio, 0.5f, VF_NULL,
        "Set heightmap LOD, this value is combined with sector error metrics and distance to camera");
    REGISTER_CVAR(e_TerrainLodDistRatio, 1.f, VF_NULL,
        "Set heightmap LOD, this value is combined only with sector distance to camera and ignores sector error metrics");
    REGISTER_CVAR(e_TerrainLodRatioHolesMin, 2.f, VF_NULL,
        "Rises LOD for distant terrain sectors with holes, prevents too strong distortions of holes on distance ");
    REGISTER_CVAR(e_TerrainDetailMaterialsViewDistZ, 128.f, VF_NULL,
        "Max view distance of terrain Z materials");
    REGISTER_CVAR(e_TerrainDetailMaterialsViewDistXY, 2048.f, VF_NULL,
        "Max view distance of terrain XY materials");
    REGISTER_CVAR(e_TerrainTextureLodRatio, 1.f, VF_NULL,
        "Adjust terrain base texture resolution on distance");

    m_isRegistered = true;
}

void LegacyTerrainCVars::UnregisterCVars()
{
    if (!m_isRegistered)
    {
        return;
    }

    UNREGISTER_CVAR("e_TerrainLog");
    UNREGISTER_CVAR("e_TerrainBBoxes");
    UNREGISTER_CVAR("e_TerrainDrawThisSectorOnly");
    UNREGISTER_CVAR("e_TerrainTextureDebug");
    UNREGISTER_CVAR("e_CoverageBufferTerrain");
    UNREGISTER_CVAR("e_TerrainDetailMaterialsDebug");
    UNREGISTER_CVAR("e_TerrainTextureStreamingPoolItemsNum");
    UNREGISTER_CVAR("e_TerrainLodRatio");
    UNREGISTER_CVAR("e_TerrainLodDistRatio");
    UNREGISTER_CVAR("e_TerrainLodRatioHolesMin");
    UNREGISTER_CVAR("e_TerrainDetailMaterialsViewDistZ");
    UNREGISTER_CVAR("e_TerrainDetailMaterialsViewDistXY");
    UNREGISTER_CVAR("e_TerrainTextureLodRatio");

    m_isRegistered = false;
}

