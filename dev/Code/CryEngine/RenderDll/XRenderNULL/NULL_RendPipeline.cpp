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

// Description : NULL device specific implementation using shaders pipeline.


#include "StdAfx.h"
#include "NULL_Renderer.h"
#include "Common/RenderView.h"

//============================================================================================
// Init Shaders rendering

void CNULLRenderer::EF_Init()
{
    bool nv = 0;

    m_RP.m_MaxVerts = 600;
    m_RP.m_MaxTris = 300;

    //==================================================
    // Init RenderObjects
    {
        MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_D3D, 0, "Renderer RenderObjects");
        m_RP.m_nNumObjectsInPool = 384; // magic number set by Cry.  The regular pipe uses a constant set to 1024

        if (m_RP.m_ObjectsPool != nullptr)
        {
            for (int j = 0; j < (int)(m_RP.m_nNumObjectsInPool * RT_COMMAND_BUF_COUNT); j++)
            {
                CRenderObject* pRendObj = &m_RP.m_ObjectsPool[j];
                pRendObj->~CRenderObject();
            }
            CryModuleMemalignFree(m_RP.m_ObjectsPool);
        }

        // we use a plain allocation and placement new here to garantee the alignment, when using array new, the compiler can store it's size and break the alignment
        m_RP.m_ObjectsPool = (CRenderObject*)CryModuleMemalign(sizeof(CRenderObject) * (m_RP.m_nNumObjectsInPool * RT_COMMAND_BUF_COUNT), 16);
        for (int j = 0; j < (int)(m_RP.m_nNumObjectsInPool * RT_COMMAND_BUF_COUNT); j++)
        {
            CRenderObject* pRendObj = new(&m_RP.m_ObjectsPool[j])CRenderObject();
        }


        CRenderObject** arrPrefill = (CRenderObject**)(alloca(m_RP.m_nNumObjectsInPool * sizeof(CRenderObject*)));
        for (int j = 0; j < RT_COMMAND_BUF_COUNT; j++)
        {
            for (int k = 0; k < m_RP.m_nNumObjectsInPool; ++k)
            {
                arrPrefill[k] = &m_RP.m_ObjectsPool[j * m_RP.m_nNumObjectsInPool + k];
            }

            m_RP.m_TempObjects[j].PrefillContainer(arrPrefill, m_RP.m_nNumObjectsInPool);
            m_RP.m_TempObjects[j].resize(0);
        }
    }
    // Init identity RenderObject
    SAFE_DELETE(m_RP.m_pIdendityRenderObject);
    m_RP.m_pIdendityRenderObject = new CRenderObject();
    m_RP.m_pIdendityRenderObject->Init();
    m_RP.m_pIdendityRenderObject->m_II.m_AmbColor = Col_White;
    m_RP.m_pIdendityRenderObject->m_II.m_Matrix.SetIdentity();
    m_RP.m_pIdendityRenderObject->m_RState = 0;
    m_RP.m_pIdendityRenderObject->m_ObjFlags |= FOB_RENDERER_IDENDITY_OBJECT;

}

void CNULLRenderer::FX_SetClipPlane (bool bEnable, float* pPlane, bool bRefract)
{
}

void CNULLRenderer::FX_PipelineShutdown(bool bFastShutdown)
{
    uint32 i, j;

    for (int n = 0; n < 2; n++)
    {
        for (j = 0; j < 2; j++)
        {
            for (i = 0; i < CREClientPoly::m_PolysStorage[n][j].Num(); i++)
            {
                CREClientPoly::m_PolysStorage[n][j][i]->Release(false);
            }
            CREClientPoly::m_PolysStorage[n][j].Free();
        }
    }
}

void CNULLRenderer::EF_Release(int nFlags)
{
}

//==========================================================================

void CNULLRenderer::FX_SetState(int st, int AlphaRef, int RestoreState)
{
    m_RP.m_CurState = st;
    m_RP.m_CurAlphaRef = AlphaRef;
}
void CRenderer::FX_SetStencilState(int st, uint32 nStencRef, uint32 nStencMask, uint32 nStencWriteMask, bool bForceFullReadMask)
{
}

//=================================================================================

// Initialize of the new shader pipeline (only 2d)
void CRenderer::FX_Start(CShader* ef, int nTech, CShaderResources* Res, IRenderElement* re)
{
    m_RP.m_Frame++;
}

void CRenderer::FX_CheckOverflow(int nVerts, int nInds, IRenderElement* re, int* nNewVerts, int* nNewInds)
{
}

uint32 CRenderer::EF_GetDeferredLightsNum(const eDeferredLightType eLightType)
{
    return 0;
}

int CRenderer::EF_AddDeferredLight(const CDLight& pLight, float, const SRenderingPassInfo& passInfo, const SRendItemSorter& rendItemSorter)
{
    return 0;
}

void CRenderer::EF_ClearDeferredLightsList()
{
}

void CRenderer::EF_ReleaseDeferredData()
{
}

uint8 CRenderer::EF_AddDeferredClipVolume(const IClipVolume* pClipVolume)
{
    return 0;
}


bool CRenderer::EF_SetDeferredClipVolumeBlendData(const IClipVolume* pClipVolume, const SClipVolumeBlendInfo& blendInfo)
{
    return false;
}

void CRenderer::EF_ClearDeferredClipVolumesList()
{
}

//========================================================================================

void CNULLRenderer::EF_EndEf3D(const int nFlags, const int nPrecacheUpdateId, const int nNearPrecacheUpdateId, const SRenderingPassInfo& passInfo)
{
    //m_RP.m_TI[m_RP.m_nFillThreadID].m_RealTime = iTimer->GetCurrTime();
    EF_RemovePolysFromScene();

    int nThreadID = m_pRT->GetThreadList();
    SRendItem::m_RecurseLevel[nThreadID]--;
}

//double timeFtoI, timeFtoL, timeQRound;
//int sSome;
void CNULLRenderer::EF_EndEf2D(const bool bSort)
{
}

void CRenderView::PrepareForRendering() {}

void CRenderView::PrepareForWriting() {}

void CRenderView::ClearRenderItems() {}

void CRenderView::FreeRenderItems() {}

CRenderView::CRenderView() {}

CRenderView::~CRenderView() {}

