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
#include "CREParticleGPU.h"
#include "FrameProfiler.h"
#include "HeapContainer.h"
#include <iterator>
#include <I3DEngine.h>
#include <IParticles.h>


//////////////////////////////////////////////////////////////////////////
//
// CREParticleGPU implementation.
//

CREParticleGPU::CREParticleGPU()
{
    mfSetType(eDATA_GPUParticle);
    m_pass = EGPUParticlePass::Main;
}

CRendElementBase* CREParticleGPU::mfCopyConstruct()
{
    return new CREParticleGPU(*this);
}

int CREParticleGPU::Size()
{
    return sizeof(*this);
}

void* CREParticleGPU::GetInstance() const
{
    return m_instance;
}

void CREParticleGPU::mfPrepare(bool bCheckOverflow)
{
    FUNCTION_PROFILER_SYS(PARTICLE);

    // notify the renderer that we need to be rendered.
    gRenDev->m_RP.m_pRE = this;
    gRenDev->m_RP.m_RendNumIndices = 0;
    gRenDev->m_RP.m_RendNumVerts = 0;

    if (m_pass != EGPUParticlePass::Shadow)
    {
        // add lighting information
        CRenderer* rd = gRenDev;
        SRenderPipeline& rRP = rd->m_RP;
        if ((rd->m_nNumVols > 0) && ((rRP.m_pCurObject->m_ObjFlags & FOB_LIGHTVOLUME) > 0))
        {
            SRenderObjData* pOD = rd->FX_GetObjData(rRP.m_pCurObject, rRP.m_nProcessThreadID);

            const int32 lightVolumeId = static_cast<int32>(pOD->m_LightVolumeId);
            if (lightVolumeId > 0)
            {
                const int32 numLightVolumes = static_cast<int32>(rd->m_nNumVols);
                const int32 modifiedLightVolumeId = static_cast<int32>(lightVolumeId - 1);

#ifndef _RELEASE
                CRY_ASSERT(modifiedLightVolumeId < numLightVolumes);
#endif
                rd->RT_SetLightVolumeShaderFlags(rd->m_pLightVols[modifiedLightVolumeId].pData.size());
            }
        }
    }
}

// NOTE: Render implementation (mfDraw/mfPreDraw) in D3DRenderRE.cpp.
//       This is specific for each renderer.

