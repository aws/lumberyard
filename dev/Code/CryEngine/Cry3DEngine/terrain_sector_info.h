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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#ifndef CRYINCLUDE_CRY3DENGINE_TERRAIN_SECTOR_INFO_H
#define CRYINCLUDE_CRY3DENGINE_TERRAIN_SECTOR_INFO_H
#pragma once

#include "terrain_sector.h"

STRUCT_INFO_BEGIN(STerrainNodeChunk)
STRUCT_VAR_INFO(nChunkVersion, TYPE_INFO(int16))
STRUCT_VAR_INFO(bHasHoles, TYPE_INFO(int16))
STRUCT_VAR_INFO(boxHeightmap, TYPE_INFO(AABB))
STRUCT_VAR_INFO(fOffset, TYPE_INFO(float))
STRUCT_VAR_INFO(fRange, TYPE_INFO(float))
STRUCT_VAR_INFO(nSize, TYPE_INFO(int))
STRUCT_VAR_INFO(nSurfaceTypesNum, TYPE_INFO(int))
STRUCT_INFO_END(STerrainNodeChunk)


#endif // CRYINCLUDE_CRY3DENGINE_TERRAIN_SECTOR_INFO_H
