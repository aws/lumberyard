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

#ifndef CRYINCLUDE_CRY3DENGINE_ITERRAIN_INFO_H
#define CRYINCLUDE_CRY3DENGINE_ITERRAIN_INFO_H
#pragma once

#include <IShader_info.h>
#include <ITerrain.h>

STRUCT_INFO_BEGIN(STerrainInfo)
STRUCT_VAR_INFO(nHeightMapSize_InUnits, TYPE_INFO(int))
STRUCT_VAR_INFO(nUnitSize_InMeters, TYPE_INFO(int))
STRUCT_VAR_INFO(nSectorSize_InMeters, TYPE_INFO(int))
STRUCT_VAR_INFO(nSectorsTableSize_InSectors, TYPE_INFO(int))
STRUCT_VAR_INFO(fHeightmapZRatio, TYPE_INFO(float))
STRUCT_VAR_INFO(fOceanWaterLevel, TYPE_INFO(float))
STRUCT_INFO_END(STerrainInfo)

STRUCT_INFO_BEGIN(STerrainChunkHeader)
STRUCT_VAR_INFO(nVersion, TYPE_INFO(int8))
STRUCT_VAR_INFO(nDummy, TYPE_INFO(int8))
STRUCT_VAR_INFO(nFlags, TYPE_INFO(int8))
STRUCT_VAR_INFO(nFlags2, TYPE_INFO(int8))
STRUCT_VAR_INFO(nChunkSize, TYPE_INFO(int32))
STRUCT_VAR_INFO(TerrainInfo, TYPE_INFO(STerrainInfo))
STRUCT_INFO_END(STerrainChunkHeader)

STRUCT_INFO_BEGIN(SCommonFileHeader)
STRUCT_VAR_INFO(signature, TYPE_ARRAY(4, TYPE_INFO(char)))
STRUCT_VAR_INFO(file_type, TYPE_INFO(uint8))
STRUCT_VAR_INFO(flags, TYPE_INFO(uint8))
STRUCT_VAR_INFO(version, TYPE_INFO(uint16))
STRUCT_INFO_END(SCommonFileHeader)

STRUCT_INFO_BEGIN(STerrainTextureFileHeader)
STRUCT_VAR_INFO(LayerCount, TYPE_INFO(uint16))
STRUCT_VAR_INFO(Flags, TYPE_INFO(uint16))
STRUCT_VAR_INFO(ColorMultiplier_deprecated, TYPE_INFO(float))
STRUCT_INFO_END(STerrainTextureFileHeader)

STRUCT_INFO_BEGIN(STerrainTextureLayerFileHeader)
STRUCT_VAR_INFO(SectorSizeInPixels, TYPE_INFO(uint16))
STRUCT_VAR_INFO(nReserved, TYPE_INFO(uint16))
STRUCT_VAR_INFO(eTexFormat, TYPE_INFO(ETEX_Format))
STRUCT_VAR_INFO(SectorSizeInBytes, TYPE_INFO(uint32))
STRUCT_INFO_END(STerrainTextureLayerFileHeader)

STRUCT_INFO_BEGIN(ITerrain::SurfaceWeight)
STRUCT_VAR_INFO(Ids, TYPE_ARRAY(WeightCount, TYPE_INFO(uint8)))
STRUCT_VAR_INFO(Weights, TYPE_ARRAY(WeightCount, TYPE_INFO(uint8)))
STRUCT_INFO_END(ITerrain::SurfaceWeight)

#endif