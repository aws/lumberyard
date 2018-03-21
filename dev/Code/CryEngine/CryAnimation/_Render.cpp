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
#include <I3DEngine.h>
#include <IShader.h>

#include "ModelMesh.h"
#include "CharacterInstance.h"
#include "IRenderAuxGeom.h"
#include "CharacterManager.h"

CLodValue CCharInstance::ComputeLod(int wantedLod, const SRenderingPassInfo& passInfo)
{
    // If the character instance has software vertex animation and ca_vaSkipVertexAnimationLod is set
    // we skip the first LOD to disable software skinning
    if (m_bHasVertexAnimation && wantedLod == 0 && Console::GetInst().ca_vaSkipVertexAnimationLOD == 1)
    {
        return CLodValue(1);
    }

    return CLodValue(wantedLod);
}

//! Render object ( register render elements into renderer )
void CCharInstance::Render(const struct SRendParams& RendParams, const QuatTS& Offset, const SRenderingPassInfo& passInfo)
{
    if (GetFlags() & CS_FLAG_COMPOUND_BASE)
    {
        if (Console::GetInst().ca_DrawCC == 0)
        {
            return;
        }
    }

    if (m_pDefaultSkeleton->m_ObjectType == CGA)
    {
        if (Console::GetInst().ca_DrawCGA == 0)
        {
            return;
        }
    }

    if (m_pDefaultSkeleton->m_ObjectType == CHR)
    {
        if (Console::GetInst().ca_DrawCHR == 0)
        {
            return;
        }
    }

    if (!(m_rpFlags & CS_FLAG_DRAW_MODEL))
    {
        return;
    }

    if (!RendParams.pMatrix)
    {
        return;
    }

    Vec3 position;
    position = RendParams.pMatrix->GetTranslation();
    if (m_SkeletonAnim.m_AnimationDrivenMotion == 0)
    {
        position += Offset.t;
    }

    Matrix33 orientation;
    orientation = Matrix33(*RendParams.pMatrix);
    if (m_SkeletonPose.m_physics.m_bPhysicsRelinquished)
    {
        orientation = Matrix33(m_location.q);
    }

    Matrix34 RenderMat34(orientation, position);

    //f32 axisX = RenderMat34.GetColumn0().GetLength();
    //f32 axisY = RenderMat34.GetColumn1().GetLength();
    //f32 axisZ = RenderMat34.GetColumn2().GetLength();
    //f32 fScaling = 0.333333333f*(axisX+axisY+axisZ);
    //RenderMat34.OrthonormalizeFast();


    uint32 nFrameID = g_pCharacterManager->m_nUpdateCounter;
    if (m_LastRenderedFrameID != nFrameID)
    {
        m_LastRenderedFrameID = nFrameID;
    }

    g_pAuxGeom->SetRenderFlags(e_Def3DPublicRenderflags);

    if (!passInfo.IsShadowPass())
    {
        m_nAnimationLOD = RendParams.lodValue.LodA();
    }

    //  float fColor[4] = {1,0,1,1};
    //  g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.2f, fColor, false,"fDistance: %f m_nAnimationLOD: %d   m_nRenderLOD: %d   numLODs: %d  m_pDefaultSkeleton->m_nBaseLOD: %d  Model: %s",RendParams.fDistance,m_nAnimationLOD,m_nRenderLOD,numLODs,m_pDefaultSkeleton->m_nBaseLOD,m_pDefaultSkeleton->GetFilePath().c_str() );
    //  g_YLine+=16.0f;

    //------------------------------------------------------------------------
    //------------   Debug-Draw of the final Render Location     -------------
    //------------------------------------------------------------------------
    if (Console::GetInst().ca_DrawPositionPost)
    {
        Vec3 wpos = RenderMat34.GetTranslation();
        g_pAuxGeom->SetRenderFlags(e_Def3DPublicRenderflags);
        static Ang3 angle(0, 0, 0);
        angle += Ang3(0.01f, 0.02f, 0.03f);
        AABB aabb = AABB(Vec3(-0.055f, -0.055f, -0.055f), Vec3(+0.055f, +0.055f, +0.055f));
        OBB obb = OBB::CreateOBBfromAABB(Matrix33::CreateRotationXYZ(angle), aabb);
        g_pAuxGeom->DrawOBB(obb, wpos, 0, RGBA8(0x00, 0xff, 0x00, 0xff), eBBD_Extremes_Color_Encoded);

        Vec3 axisX = RenderMat34.GetColumn0();
        Vec3 axisY = RenderMat34.GetColumn1();
        Vec3 axisZ = RenderMat34.GetColumn2();
        g_pAuxGeom->DrawLine(wpos, RGBA8(0x7f, 0x00, 0x00, 0x00), wpos + axisX, RGBA8(0xff, 0x00, 0x00, 0x00));
        g_pAuxGeom->DrawLine(wpos, RGBA8(0x00, 0x7f, 0x00, 0x00), wpos + axisY, RGBA8(0x00, 0xff, 0x00, 0x00));
        g_pAuxGeom->DrawLine(wpos, RGBA8(0x00, 0x00, 0x7f, 0x00), wpos + axisZ, RGBA8(0x00, 0x00, 0xff, 0x00));
    }

    //------------------------------------------------------------------------

    if (m_pDefaultSkeleton->m_ObjectType == CGA)
    {
        Matrix34 mRendMat34 = RenderMat34 * Matrix34(Offset);
        RenderCGA (RendParams, mRendMat34, passInfo);
    }
    else
    {
        pe_params_flags pf;
        IPhysicalEntity* pCharPhys = m_SkeletonPose.GetCharacterPhysics();
        if (pCharPhys && pCharPhys->GetType() == PE_ARTICULATED && pCharPhys->GetParams(&pf) && pf.flags & aef_recorded_physics)
        {
            RenderMat34 = RenderMat34 * Matrix34(Offset);
        }
        RenderCHR (RendParams, RenderMat34, passInfo);
    }

    // draw weapon and binded objects
    m_AttachmentManager.DrawAttachments(RendParams, RenderMat34, passInfo);
}



void CCharInstance::RenderCGA(const struct SRendParams& RendParams, const Matrix34& RenderMat34, const SRenderingPassInfo& passInfo)
{
    int nList = (int)CharacterManager::GetRendererMainThreadId();


    if (GetSkinningTransformationCount())
    {
        CryFatalError("CryAnimation: CGA should not have Dual-Quaternions for Skinning");
    }

    SRendParams nodeRP = RendParams;

    Matrix34 ortho_tm34 = RenderMat34;
    _smart_ptr<IMaterial> pMaterial = nodeRP.pMaterial;
    if (Console::GetInst().ca_DrawBaseMesh)
    {
        for (uint32 i = 0; i < m_SkeletonPose.GetPoseData().GetJointCount(); i++)
        {
            //SJoint* joint = &m_SkeletonPose.m_arrJoints[i];
            if (m_SkeletonPose.m_arrCGAJoints.size() &&  m_SkeletonPose.m_arrCGAJoints[i].m_CGAObjectInstance)
            {
                Matrix34 tm34 = ortho_tm34 * Matrix34(m_SkeletonPose.GetPoseData().GetJointAbsolute(i));
                nodeRP.pMatrix = &tm34;
                nodeRP.ppRNTmpData = &m_SkeletonPose.m_arrCGAJoints[i].m_pRNTmpData;
                nodeRP.pInstance = nodeRP.ppRNTmpData;

                // apply custom joint material, if set
                nodeRP.pMaterial = m_SkeletonPose.m_arrCGAJoints[i].m_pMaterial ? m_SkeletonPose.m_arrCGAJoints[i].m_pMaterial : pMaterial;

                if (tm34.IsValid())
                {
                    m_SkeletonPose.m_arrCGAJoints[i].m_CGAObjectInstance->Render(nodeRP, passInfo);
                }
                else
                {
                    gEnv->pLog->LogError("CCharInstance::RenderCGA: object has invalid matrix: %s", m_pDefaultSkeleton->GetModelFilePath());
                }
            }
        }
    }

    if (Console::GetInst().ca_DrawSkeleton || Console::GetInst().ca_DrawBBox)
    {
        Matrix34 wsRenderMat34(RenderMat34);

        // Convert "Camera Space" to "World Space"
        if (RendParams.dwFObjFlags & FOB_NEAREST)
        {
            wsRenderMat34.AddTranslation(gEnv->pRenderer->GetCamera().GetPosition());
        }

        if (Console::GetInst().ca_DrawSkeleton)
        {
            m_SkeletonPose.DrawSkeleton(wsRenderMat34);
        }

        if (Console::GetInst().ca_DrawBBox)
        {
            m_SkeletonPose.DrawBBox(wsRenderMat34);
        }
    }
}



void CCharInstance::RenderCHR(const SRendParams& RendParams, const Matrix34& rRenderMat34, const SRenderingPassInfo& passInfo)
{
    CRenderObject* pObj = g_pIRenderer->EF_GetObject_Temp(passInfo.ThreadID());
    if (!pObj)
    {
        return;
    }

    pObj->m_fSort = RendParams.fCustomSortOffset;

    //check if it should be drawn close to the player
    // For nearest geometry (weapons/arms) - make sure its rendered really at beginning (before water passes)
    if ((RendParams.dwFObjFlags & FOB_NEAREST) || (m_rpFlags & CS_FLAG_DRAW_NEAR))
    {
        pObj->m_ObjFlags |= FOB_NEAREST;
        ((SRendParams&)RendParams).nAfterWater = 1;
    }
    else
    {
        pObj->m_ObjFlags &= ~FOB_NEAREST;
    }

    pObj->m_fAlpha = RendParams.fAlpha;
    pObj->m_fDistance = RendParams.fDistance;

    pObj->m_II.m_AmbColor = RendParams.AmbientColor;


    pObj->m_ObjFlags |= RendParams.dwFObjFlags;
    SRenderObjData* pD = g_pIRenderer->EF_GetObjData(pObj, true, passInfo.ThreadID());

    // copy the shaderparams into the render object data from the render params
    if (RendParams.pShaderParams && RendParams.pShaderParams->size() > 0)
    {
        pD->SetShaderParams(RendParams.pShaderParams);
    }

    pObj->m_II.m_Matrix = rRenderMat34;

    pObj->m_nClipVolumeStencilRef = RendParams.nClipVolumeStencilRef;
    pObj->m_nTextureID = RendParams.nTextureID;

    bool bCheckMotion = MotionBlurMotionCheck(pObj->m_ObjFlags);
    pD->m_uniqueObjectId = reinterpret_cast<uintptr_t>(RendParams.pInstance);
    if (bCheckMotion)
    {
        pObj->m_ObjFlags |= FOB_MOTION_BLUR;
    }

    pD->m_nHUDSilhouetteParams = RendParams.nHUDSilhouettesParams;

    pObj->m_nMaterialLayers = RendParams.nMaterialLayersBlend;

    Vec3 skinOffset(ZERO);
    if (m_pDefaultSkeleton->GetModelMesh())
    {
        skinOffset = m_pDefaultSkeleton->GetModelMesh()->m_vRenderMeshOffset;
    }
    pD->m_fTempVars[0] = skinOffset.x;
    pD->m_fTempVars[1] = skinOffset.y;
    pD->m_fTempVars[2] = skinOffset.z;

    pD->m_nCustomData = RendParams.nCustomData;
    pD->m_nCustomFlags = RendParams.nCustomFlags;

    pObj->m_DissolveRef = RendParams.nDissolveRef;

    pD->m_pSkinningData = GetSkinningData();
    pObj->m_ObjFlags |= FOB_SKINNED;

    if (g_pI3DEngine->IsTessellationAllowed(pObj, passInfo))
    {
        // Allow this RO to be tessellated, however actual tessellation will be applied if enabled in material
        pObj->m_ObjFlags |= FOB_ALLOW_TESSELLATION;
    }

    pObj->m_nSort = fastround_positive(RendParams.fDistance * 2.0f);

    if (m_HideMaster == 0)
    {
        //  AddCurrentRenderData (pObj, RendParams);
        m_pDefaultSkeleton->m_ModelMesh.m_stream.nFrameId = passInfo.GetMainFrameID();

        if (Console::GetInst().ca_DrawSkeleton)
        {
            m_SkeletonPose.DrawSkeleton(rRenderMat34);
        }
        if (Console::GetInst().ca_DrawBBox)
        {
            m_SkeletonPose.DrawBBox(rRenderMat34);
        }

        IRenderMesh* pIRenderMesh = m_pDefaultSkeleton->m_ModelMesh.m_pIRenderMesh;
        if (pIRenderMesh)
        {
            // MichaelS - use the instance's material if there is one, and if no override material has
            // already been specified.
            _smart_ptr<IMaterial> pMaterial = RendParams.pMaterial;
            if (pMaterial == 0)
            {
                pMaterial = this->GetIMaterial_Instance();
            }
            if (pMaterial == 0)
            {
                pMaterial = m_pDefaultSkeleton->GetIMaterial();
            }
#ifndef _RELEASE
            CModelMesh* pModelMesh = m_pDefaultSkeleton->GetModelMesh();
            static ICVar* p_e_debug_draw = gEnv->pConsole->GetCVar("e_DebugDraw");
            if (p_e_debug_draw && p_e_debug_draw->GetIVal() != 0)
            {
                pModelMesh->DrawDebugInfo(this->m_pDefaultSkeleton, 0, rRenderMat34, p_e_debug_draw->GetIVal(), pMaterial, pObj, RendParams, passInfo.IsGeneralPass(), (IRenderNode*)RendParams.pRenderNode, m_SkeletonPose.GetAABB());
            }
#endif
            //  float fColor[4] = {1,0,1,1};
            //  extern f32 g_YLine;
            //  g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"m_nRenderLOD: %d  %d",RendParams.nLodLevel, pObj->m_nLod  ); g_YLine+=0x10;
            if (Console::GetInst().ca_DrawBaseMesh)
            {
                pIRenderMesh->Render(RendParams, pObj, pMaterial, passInfo);
                if (!passInfo.IsShadowPass())
                {
                    AddDecalsToRenderer(this, 0, rRenderMat34, passInfo);
                }
            }

            if (Console::GetInst().ca_DrawDecalsBBoxes)
            {
                Matrix34 wsRenderMat34(rRenderMat34);
                // Convert "Camera Space" to "World Space"
                if (pObj->m_ObjFlags & FOB_NEAREST)
                {
                    wsRenderMat34.AddTranslation(gEnv->pRenderer->GetCamera().GetPosition());
                }
                g_pAuxGeom->SetRenderFlags(e_Def3DPublicRenderflags);
                DrawDecalsBBoxes(this, 0, wsRenderMat34);
            }
        }
    }
}

//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------

bool CCharInstance::MotionBlurMotionCheck(uint64 nObjFlags)
{
    if  (m_skinningTransformationsMovement > Console::GetInst().ca_MotionBlurMovementThreshold)
    {
        return true;
    }

    return false;
}



SSkinningData* CCharInstance::GetSkinningData()
{
    DEFINE_PROFILER_FUNCTION();

    uint32 nNumBones = GetSkinningTransformationCount();

    bool bNeedJobSyncVar = true;

    // get data to fill
    int nFrameID = gEnv->pRenderer->EF_GetSkinningPoolID();
    int nPrevFrameID = nFrameID - 1;

    int nList = nFrameID % tripleBufferSize;
    int nPrevList = nPrevFrameID % tripleBufferSize;

    // before allocating new skinning date, check if we already have for this frame
    if (arrSkinningRendererData[nList].nFrameID == nFrameID && arrSkinningRendererData[nList].pSkinningData)
    {
        return arrSkinningRendererData[nList].pSkinningData;
    }

    SSkinningData* pSkinningData = gEnv->pRenderer->EF_CreateSkinningData(nNumBones, bNeedJobSyncVar, m_bUseMatrixSkinning);
    arrSkinningRendererData[nList].pSkinningData = pSkinningData;
    arrSkinningRendererData[nList].nNumBones = nNumBones;
    arrSkinningRendererData[nList].nFrameID = nFrameID;

    //clear obsolete data
    int nPrevPrevList = (nFrameID - 2) % tripleBufferSize;
    if (nPrevPrevList >= 0)
    {
        if (arrSkinningRendererData[nPrevPrevList].nFrameID == (nFrameID - 2) && arrSkinningRendererData[nPrevPrevList].pSkinningData)
        {
            arrSkinningRendererData[nPrevPrevList].pSkinningData->pPreviousSkinningRenderData = nullptr;
        }
        else
        {
            // If nFrameID was off by more than 2 frames old, then this data is guaranteed to be stale if it exists.  Clear it to be safe.
            // The triple-buffered pool allocator in EF_CreateSkinningData will have already freed this data if the frame count/IDs mismatch.
            arrSkinningRendererData[nPrevPrevList].pSkinningData = nullptr;
        }
    }

    if (!pSkinningData)
    {
        return nullptr;
    }

    // set data for motion blur
    if (arrSkinningRendererData[nPrevList].nFrameID == nPrevFrameID && arrSkinningRendererData[nPrevList].pSkinningData)
    {
        pSkinningData->nHWSkinningFlags |= eHWS_MotionBlured;
        pSkinningData->pPreviousSkinningRenderData = arrSkinningRendererData[nPrevList].pSkinningData;
        if (pSkinningData->pPreviousSkinningRenderData->pAsyncJobExecutor)
        {
            pSkinningData->pPreviousSkinningRenderData->pAsyncJobExecutor->WaitForCompletion();
        }
    }
    else
    {
        // if we don't have motion blur data, use the some as for the current frame
        pSkinningData->pPreviousSkinningRenderData = pSkinningData;
    }

    BeginSkinningTransformationsComputation(pSkinningData);
    return pSkinningData;
}
