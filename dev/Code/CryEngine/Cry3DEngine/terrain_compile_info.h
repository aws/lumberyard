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

#ifndef CRYINCLUDE_CRY3DENGINE_TERRAIN_COMPILE_INFO_H
#define CRYINCLUDE_CRY3DENGINE_TERRAIN_COMPILE_INFO_H
#pragma once

STRUCT_INFO_BEGIN(StatInstGroupChunk)
STRUCT_VAR_INFO(szFileName, TYPE_ARRAY(256, TYPE_INFO(char)))
STRUCT_VAR_INFO(fBending, TYPE_INFO(float))
STRUCT_VAR_INFO(fSpriteDistRatio, TYPE_INFO(float))
STRUCT_VAR_INFO(fShadowDistRatio, TYPE_INFO(float))
STRUCT_VAR_INFO(fMaxViewDistRatio, TYPE_INFO(float))
STRUCT_VAR_INFO(fBrightness, TYPE_INFO(float))
STRUCT_VAR_INFO(nRotationRangeToTerrainNormal, TYPE_INFO(int32))
STRUCT_VAR_INFO(fAlignToTerrainCoefficient, TYPE_INFO(float))
STRUCT_VAR_INFO(nMaterialLayers, TYPE_INFO(uint32))
STRUCT_VAR_INFO(fDensity, TYPE_INFO(float))
STRUCT_VAR_INFO(fElevationMax, TYPE_INFO(float))
STRUCT_VAR_INFO(fElevationMin, TYPE_INFO(float))
STRUCT_VAR_INFO(fSize, TYPE_INFO(float))
STRUCT_VAR_INFO(fSizeVar, TYPE_INFO(float))
STRUCT_VAR_INFO(fSlopeMax, TYPE_INFO(float))
STRUCT_VAR_INFO(fSlopeMin, TYPE_INFO(float))
STRUCT_VAR_INFO(fStatObjRadius_NotUsed, TYPE_INFO(float))
STRUCT_VAR_INFO(nIDPlusOne, TYPE_INFO(int))
STRUCT_VAR_INFO(fLodDistRatio, TYPE_INFO(float))
STRUCT_VAR_INFO(nReserved, TYPE_INFO(uint32))
STRUCT_VAR_INFO(nFlags, TYPE_INFO(int))
STRUCT_VAR_INFO(nMaterialId, TYPE_INFO(int))
STRUCT_VAR_INFO(m_dwRndFlags, TYPE_INFO(int))
STRUCT_VAR_INFO(fStiffness, TYPE_INFO(float))
STRUCT_VAR_INFO(fDamping, TYPE_INFO(float))
STRUCT_VAR_INFO(fVariance, TYPE_INFO(float))
STRUCT_VAR_INFO(fAirResistance, TYPE_INFO(float))
STRUCT_INFO_END(StatInstGroupChunk)

STRUCT_INFO_BEGIN(SNameChunk)
STRUCT_VAR_INFO(szFileName, TYPE_ARRAY(256, TYPE_INFO(char)))
STRUCT_INFO_END(SNameChunk)

#endif // CRYINCLUDE_CRY3DENGINE_TERRAIN_COMPILE_INFO_H
