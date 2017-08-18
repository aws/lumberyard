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

#include "stdafx.h"

#if defined(OFFLINE_COMPUTATION)

#include <PRT/TransferParameters.h>


void NSH::NTransfer::STransferParameters::Log() const
{
    GetSHLog().Log("called with transfer parameters: \n");
    GetSHLog().Log("		backlightingColour: r=%.2f  g=%.2f  b=%.2f\n", backlightingColour.x, backlightingColour.y, backlightingColour.z);
    GetSHLog().Log("		groundPlaneBlockValue: %.2f\n", groundPlaneBlockValue);
    GetSHLog().Log("		rayTracingBias: %.2f\n", rayTracingBias);
    GetSHLog().Log("		minDirectBumpCoeffVisibility: %.2f\n", minDirectBumpCoeffVisibility);
    GetSHLog().Log("		transparencyShadowingFactor: %.2f\n", transparencyShadowingFactor);
    GetSHLog().Log("		sampleCountPerVertex: %d\n", sampleCountPerVertex);
    switch (matrixTargetCS)
    {
    case MATRIX_HEURISTIC_MAX:
        GetSHLog().Log("		matrixTargetCS: 3D Studio MAX\n");
        break;
    case MATRIX_HEURISTIC_SH:
        GetSHLog().Log("		matrixTargetCS: SH default\n");
        break;
    }
    switch (configType)
    {
    case TRANSFER_CONFIG_VEGETATION:
        GetSHLog().Log("		use transfer configuration VEGETATION\n");
        break;
    case TRANSFER_CONFIG_DEFAULT:
    default:
        GetSHLog().Log("		use transfer configuration DEFAULT\n");
    }
    GetSHLog().Log("		rayCastingThreads: %d\n", rayCastingThreads);
    GetSHLog().Log("		bumpGranularity: %s\n", bumpGranularity ? "true" : "false");
    GetSHLog().Log("		supportTransparency: %s\n", supportTransparency ? "true" : "false");
    GetSHLog().Log("		compressToByte: %s\n", compressToByte ? "true" : "false");
}

const bool NSH::NTransfer::STransferParameters::CheckForConsistency() const
{
    bool ret = true;
    if
    (
        backlightingColour.x < 0.f || backlightingColour.x > 1.f ||
        backlightingColour.y < 0.f || backlightingColour.y > 1.f ||
        backlightingColour.z < 0.f || backlightingColour.z > 1.f
    )
    {
        GetSHLog().LogError("		backlightingColour outside 0..1 range\n");
        ret = false;
    }
    if (groundPlaneBlockValue < 0.f || groundPlaneBlockValue > 1.f)
    {
        GetSHLog().LogError("		groundPlaneBlockValue outside 0..1 range\n");
        ret = false;
    }
    if (rayTracingBias < 0.f)
    {
        GetSHLog().LogError("		negative rayTracingBias\n");
        ret = false;
    }
    if (rayTracingBias > 0.2f)
    {
        GetSHLog().Log("		found large rayTracingBias\n");
    }
    if (minDirectBumpCoeffVisibility < 0.f || minDirectBumpCoeffVisibility >= 1.f)
    {
        GetSHLog().LogError("		minDirectBumpCoeffVisibility outside 0..1 range\n");
        ret = false;
    }
    if (transparencyShadowingFactor <= 0.f || transparencyShadowingFactor > 1.f)
    {
        GetSHLog().LogError("		transparencyShadowingFactor outside 0..1 range\n");
        ret = false;
    }
    if (sampleCountPerVertex > (2 << (sizeof(short) * 8)))
    {
        GetSHLog().LogError("		sampleCountPerVertex forces too much sampels to take per vertex\n");
        ret = false;
    }
    if (!ret)
    {
        GetSHLog().LogError("		CheckForConsistency on Transfer parameters failed!\n");
    }
    return ret;
}

#endif