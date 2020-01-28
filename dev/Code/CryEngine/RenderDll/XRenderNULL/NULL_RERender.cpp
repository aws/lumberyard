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
#include "NULL_Renderer.h"
#include "I3DEngine.h"

#include "CREParticle.h"
#include "CREParticleGPU.h"

//=======================================================================

bool CRESky::mfDraw(CShader* ef, SShaderPass* sfm)
{
    return true;
}

bool CREHDRSky::mfDraw(CShader* ef, SShaderPass* sfm)
{
    return true;
}

bool CREFogVolume::mfDraw(CShader* ef, SShaderPass* sfm)
{
    return true;
}

bool CREWaterVolume::mfDraw(CShader* ef, SShaderPass* sfm)
{
    return true;
}

void CREWaterOcean::FrameUpdate()
{
}

void CREWaterOcean::Create(uint32 nVerticesCount, SVF_P3F_C4B_T2F* pVertices, uint32 nIndicesCount, const void* pIndices, uint32 nIndexSizeof)
{
}

void CREWaterOcean::ReleaseOcean()
{
}

bool CREWaterOcean::mfDraw(CShader* ef, SShaderPass* sfm)
{
    return true;
}

CREOcclusionQuery::~CREOcclusionQuery()
{
    mfReset();
}

void CREOcclusionQuery::mfReset()
{
    m_nOcclusionID = 0;
}

uint32 CREOcclusionQuery::m_nQueriesPerFrameCounter = 0;
uint32 CREOcclusionQuery::m_nReadResultNowCounter = 0;
uint32 CREOcclusionQuery::m_nReadResultTryCounter = 0;

bool CREOcclusionQuery::mfDraw(CShader* ef, SShaderPass* sfm)
{
    return true;
}
bool CREOcclusionQuery::mfReadResult_Now(void)
{
    return true;
}
bool CREOcclusionQuery::mfReadResult_Try(uint32 nDefaultNumSamples)
{
    return true;
}
bool CREOcclusionQuery::RT_ReadResult_Try(uint32 nDefaultNumSamples)
{
    return true;
}

bool CREMeshImpl::mfPreDraw(SShaderPass* sl)
{
    return true;
}

bool CREMeshImpl::mfDraw(CShader* ef, SShaderPass* sl)
{
    return true;
}

bool CREHDRProcess::mfDraw(CShader* ef, SShaderPass* sfm)
{
    return true;
}

bool CREDeferredShading::mfDraw(CShader* ef, SShaderPass* sfm)
{
    return true;
}

bool CREBeam::mfDraw(CShader* ef, SShaderPass* sl)
{
    return true;
}

bool CREImposter::mfDraw(CShader* ef, SShaderPass* pPass)
{
    return true;
}

bool CRECloud::mfDraw(CShader* ef, SShaderPass* pPass)
{
    return true;
}

bool CRECloud::UpdateImposter(CRenderObject* pObj)
{
    return true;
}

bool CRECloud::GenerateCloudImposter(CShader* pShader, CShaderResources* pRes, CRenderObject* pObject)
{
    return true;
}

bool CREImposter::UpdateImposter()
{
    return true;
}

bool CREParticle::mfPreDraw(SShaderPass* sl)
{
    return true;
}

bool CREParticle::mfDraw(CShader* ef, SShaderPass* sfm)
{
    return true;
}

// Confetti BEGIN:

bool CREParticleGPU::mfPreDraw(SShaderPass* sl)
{
    return true;
}

bool CREParticleGPU::mfDraw(CShader* ef, SShaderPass* sfm)
{
    return true;
}

// Confetti END:

bool CREVolumeObject::mfDraw(CShader* ef, SShaderPass* sfm)
{
    return true;
}

#if !defined(EXCLUDE_DOCUMENTATION_PURPOSE)
bool CREPrismObject::mfDraw(CShader* ef, SShaderPass* sfm)
{
    return true;
}
#endif // EXCLUDE_DOCUMENTATION_PURPOSE

bool CREGameEffect::mfDraw(CShader* ef, SShaderPass* sfm)
{
    return true;
}

void CREBreakableGlass::mfPrepare(bool bCheckOverflow)
{
}

bool CREBreakableGlass::mfDraw(CShader* pShader, SShaderPass* pShaderPass)
{
    return true;
}

void CRELensOptics::ClearResources()
{
}

#if defined(USE_GEOM_CACHES)
bool CREGeomCache::mfDraw(CShader* pShader, SShaderPass* pShaderPass)
{
    return true;
}
#endif
