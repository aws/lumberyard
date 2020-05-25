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

class LegacyTerrainCVars
{
public:
    LegacyTerrainCVars() : m_isRegistered(false) {}
    ~LegacyTerrainCVars() { UnregisterCVars(); }

    int e_TerrainLog = 0;
    int e_TerrainBBoxes = 0;
    int e_TerrainDrawThisSectorOnly = 0;
    int e_TerrainTextureDebug = 0;
    int e_CoverageBufferTerrain = 0;
    int e_TerrainTextureStreamingPoolItemsNum = 64;
    int e_TerrainDetailMaterialsDebug = 0;
    float e_TerrainLodRatio;
    float e_TerrainLodDistRatio;
    float e_TerrainLodRatioHolesMin;
    float e_TerrainDetailMaterialsViewDistZ;
    float e_TerrainDetailMaterialsViewDistXY;
    float e_TerrainTextureLodRatio;

    void RegisterCVars();
    void UnregisterCVars();

private:
    bool m_isRegistered;
};
