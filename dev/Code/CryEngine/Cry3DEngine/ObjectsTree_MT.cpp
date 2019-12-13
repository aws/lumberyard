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

#include "StdAfx.h"
#include "CullBuffer.h"
#include "RoadRenderNode.h"
#include "Brush.h"
#include "RenderMeshMerger.h"
#include "DecalManager.h"
#include "VisAreas.h"
#include "ICryAnimation.h"
#include "LightEntity.h"
#include <CryProfileMarker.h>

#ifdef WIN32
#include <CryWindows.h>
#endif //WIN32

#ifndef PI
#define PI 3.14159f
#endif

//////////////////////////////////////////////////////////////////////////
void CObjManager::PrepareCullbufferAsync(const CCamera& rCamera)
{
    AZ_TRACE_METHOD();
    if (!(gEnv->IsDedicated()))
    {
        m_CullThread.PrepareCullbufferAsync(rCamera);
    }
}

//////////////////////////////////////////////////////////////////////////
void CObjManager::BeginOcclusionCulling(const SRenderingPassInfo& passInfo)
{
    AZ_TRACE_METHOD();
    if (!gEnv->IsDedicated())
    {
        m_CullThread.CullStart(passInfo);
    }
}

//////////////////////////////////////////////////////////////////////////
void CObjManager::EndOcclusionCulling()
{
    AZ_TRACE_METHOD();
    if (!gEnv->IsDedicated())
    {
        m_CullThread.CullEnd();
    }
}

//////////////////////////////////////////////////////////////////////////
void CObjManager::RenderBufferedRenderMeshes(const SRenderingPassInfo& passInfo)
{
    AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Renderer);

    CRYPROFILE_SCOPE_PROFILE_MARKER("RenderBufferedRenderMeshes");
    SCheckOcclusionOutput outputData;
    while (1)
    {
        {
            AZ_PROFILE_SCOPE_STALL(AZ::Debug::ProfileCategory::Renderer, "PopFromCullOutputQueue");
            // process till we know that no more procers are working
            if (!GetObjManager()->PopFromCullOutputQueue(&outputData))
            {
                break;
            }
        }

        switch (outputData.type)
        {
        case SCheckOcclusionOutput::VEGETATION:
            GetObjManager()->RenderVegetation(static_cast<CVegetation*>(outputData.common.pObj),
                outputData.objBox,
                outputData.common.fEntDistance,
                outputData.common.pTerrainTexInfo,
                outputData.common.bCheckPerObjectOcclusion,
                passInfo,
                outputData.rendItemSorter);
            break;
        case SCheckOcclusionOutput::ROAD_DECALS:
            GetObjManager()->RenderDecalAndRoad(outputData.common.pObj,
                outputData.objBox,
                outputData.common.fEntDistance,
                outputData.common.bCheckPerObjectOcclusion,
                passInfo,
                outputData.rendItemSorter);
            break;
        case SCheckOcclusionOutput::COMMON:
            GetObjManager()->RenderObject(outputData.common.pObj,
                outputData.objBox,
                outputData.common.fEntDistance,
                outputData.common.pObj->GetRenderNodeType(),
                passInfo,
                outputData.rendItemSorter);
            break;

#ifdef LY_TERRAIN_LEGACY_RUNTIME
        case SCheckOcclusionOutput::TERRAIN:
            GetTerrain()->AddVisSector(outputData.terrain.pTerrainNode);
            break;
#endif

        case SCheckOcclusionOutput::DEFORMABLE_BRUSH:
            outputData.deformable_brush.pBrush->m_pDeform->RenderInternalDeform(outputData.deformable_brush.pRenderObject,
                outputData.deformable_brush.nLod,
                outputData.deformable_brush.pBrush->CBrush::GetBBox(),
                passInfo,
                outputData.rendItemSorter);
            break;
        case SCheckOcclusionOutput::BRUSH:
            outputData.brush.pBrush->Render_JobEntry(outputData.brush.pRenderObject,
                CLodValue(outputData.brush.nLodA, outputData.brush.nDissolveRef, outputData.brush.nLodB),
                passInfo,
                outputData.rendItemSorter);
            break;
#ifdef LY_TERRAIN_RUNTIME
        case SCheckOcclusionOutput::TERRAIN_SYSTEM:
            {
                SRendParams rParams;
                rParams.rendItemSorter = outputData.rendItemSorter.GetValue();
                outputData.terrain_system.pTerrainNode->Render(rParams, passInfo);
            }
            break;
#endif
        default:
            CryFatalError("Got Unknown Output type from CheckOcclusion");
            break;
        }
    }
#if !defined(_RELEASE)
    if (GetCVars()->e_CoverageBufferDebug)
    {
        GetObjManager()->CoverageBufferDebugDraw();
    }
#endif
}

//////////////////////////////////////////////////////////////////////////
void CObjManager::BeginCulling()
{
    m_CheckOcclusionOutputQueue.SetRunningState();
}

