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

#include "CryLegacy_precompiled.h"
#include "AttachmentBase.h"
#include "CharacterManager.h"
#include "CharacterInstance.h"
#include "Vertex/VertexData.h"
#include "Vertex/VertexCommand.h"
#include "Vertex/VertexCommandBuffer.h"
#include "AttachmentManager.h"
#include "AttachmentVCloth.h"
#include <IRenderAuxGeom.h>
#include <IShader.h>
#include "ModelMesh.h"
#include "QTangent.h"

void CAttachmentVCLOTH::ReleaseSoftwareRenderMeshes()
{
    m_pRenderMeshsSW[0] = NULL;
    m_pRenderMeshsSW[1] = NULL;
}

uint32 CAttachmentVCLOTH::AddBinding(IAttachmentObject* pIAttachmentObject, _smart_ptr<ISkin> pISkinRender /*= nullptr*/, uint32 nLoadingFlags /*= 0*/)
{
    if (pIAttachmentObject == 0)
    {
        return 0; //no attachment objects
    }
    if (pISkinRender == 0)
    {
        CryFatalError("CryAnimation: if you create the binding for a Skin-Attachment, then you have to pass the pointer to an ISkin as well");
    }

    //On reload we need to clear out the motion blur pose array of old data
    //we need to wait for async jobs to be done using the data before clearing

    for (uint32 i = 0; i < tripleBufferSize; i++)
    {
        SSkinningData* pSkinningData = m_arrSkinningRendererData[i].pSkinningData;
        int expectedNumBones = m_arrSkinningRendererData[i].nNumBones;
        if (pSkinningData
            && pSkinningData->nNumBones == expectedNumBones
            && pSkinningData->pPreviousSkinningRenderData
            && pSkinningData->pPreviousSkinningRenderData->nNumBones == expectedNumBones)
        {
            AZ::LegacyJobExecutor* pAsyncJobExecutor = pSkinningData->pPreviousSkinningRenderData->pAsyncJobExecutor;
            if (pAsyncJobExecutor)
            {
                pAsyncJobExecutor->WaitForCompletion();
            }
        }
    }
    memset(m_arrSkinningRendererData, 0, sizeof(m_arrSkinningRendererData));

    uint32 nLogWarnings = (nLoadingFlags & CA_DisableLogWarnings) == 0;
    _smart_ptr<CSkin> pCSkinRenderModel = _smart_ptr<CSkin>((CSkin*)pISkinRender.get());

    //only the SKIN-Instance is allowed to keep a smart-ptr CSkin object
    //this should prevent ptr-hijacking in the future
    if (m_pRenderSkin != pCSkinRenderModel)
    {
        ReleaseRenderSkin();

        {
            AZStd::unique_lock<AZStd::mutex> lock(m_remapMutex);
            m_hasRemappedRenderMesh.resize(pCSkinRenderModel->GetNumLODs()); // this has to happen before calling RegisterInstance
            m_pRenderSkin = pCSkinRenderModel;            //increase the Ref-Counter
        }

        g_pCharacterManager->RegisterInstanceVCloth(pCSkinRenderModel, this); //register this CAttachmentVCLOTH instance in CSkin.
    }

    CCharInstance* pInstanceSkel = m_pAttachmentManager->m_pSkelInstance;
    CDefaultSkeleton* pDefaultSkeleton = pInstanceSkel->m_pDefaultSkeleton;

    const char* pSkelFilePath = pDefaultSkeleton->GetModelFilePath();
    const char* pSkinFilePath = m_pRenderSkin->GetModelFilePath();

    uint32 numJointsSkel = pDefaultSkeleton->m_arrModelJoints.size();
    uint32 numJointsSkin = m_pRenderSkin->m_arrModelJoints.size();

    uint32 NotMatchingNames = 0;
    m_arrRemapTable.resize(numJointsSkin, 0);
    for (uint32 js = 0; js < numJointsSkin; js++)
    {
        const int32 nID = pDefaultSkeleton->GetJointIDByCRC32(m_pRenderSkin->m_arrModelJoints[js].m_nJointCRC32Lower);
        if (nID >= 0)
        {
            m_arrRemapTable[js] = nID;
        }
#ifdef EDITOR_PCDEBUGCODE
        else
        {
            NotMatchingNames++, g_pILog->LogError ("The joint-name (%s) of SKIN (%s) was not found in SKEL:  %s", m_pRenderSkin->m_arrModelJoints[js].m_NameModelSkin.c_str(), pSkinFilePath, pSkelFilePath);
        }
#endif
    } //for loop

    if (NotMatchingNames)
    {
        if (pInstanceSkel->GetCharEditMode())
        {
            //for now limited to CharEdit
            RecreateDefaultSkeleton(pInstanceSkel, nLoadingFlags);
        }
        else
        {
            if (nLogWarnings)
            {
                CryLogAlways("SKEL: %s", pDefaultSkeleton->GetModelFilePath());
                CryLogAlways("SKIN: %s", m_pRenderSkin->GetModelFilePath());
                uint32 numJointCount = pDefaultSkeleton->GetJointCount();
                for (uint32 i = 0; i < numJointCount; i++)
                {
                    const char* pJointName = pDefaultSkeleton->GetJointNameByID(i);
                    CryLogAlways("%03d JointName: %s", i, pJointName);
                }
            }

            // Free the new attachment as we cannot use it
            SAFE_RELEASE(pIAttachmentObject);
            return 0; //critical! incompatible skeletons. cant create skin-attachment
        }
    }

    {
        AZStd::unique_lock<AZStd::mutex> lock(m_remapMutex);
        PatchRemapping(pDefaultSkeleton);
    }

    SAFE_RELEASE(m_pIAttachmentObject);
    m_pIAttachmentObject = pIAttachmentObject;
    return 1;
}

void CAttachmentVCLOTH::ComputeClothCacheKey()
{
    uint32 keySimMesh = CCrc32::ComputeLowercase(m_pSimSkin->GetModelFilePath());
    uint32 keyRenderMesh = CCrc32::ComputeLowercase(m_pRenderSkin->GetModelFilePath());
    m_clothCacheKey = (((uint64)keySimMesh) << 32) | keyRenderMesh;
}

void CAttachmentVCLOTH::AddClothParams(const SVClothParams& clothParams)
{
    m_clothPiece.SetClothParams(clothParams);
}

bool CAttachmentVCLOTH::InitializeCloth()
{
    return m_clothPiece.Initialize(this);
}

const SVClothParams& CAttachmentVCLOTH::GetClothParams()
{
    return m_clothPiece.GetClothParams();
}

uint32 CAttachmentVCLOTH::AddSimBinding(const ISkin& pISkinRender, uint32 nLoadingFlags)
{
    uint32 nLogWarnings = (nLoadingFlags & CA_DisableLogWarnings) == 0;
    CSkin* pCSkinRenderModel = (CSkin*)&pISkinRender;

    //only the VCLOTH-Instance is allowed to keep a smart-ptr CSkin object
    //this should prevent ptr-hijacking in the future
    if (m_pSimSkin != pCSkinRenderModel)
    {
        ReleaseSimSkin();

        {
            AZStd::unique_lock<AZStd::mutex> lock(m_remapMutex);
            m_hasRemappedSimMesh = false;
            m_pSimSkin = pCSkinRenderModel;            //increase the Ref-Counter
        }

        g_pCharacterManager->RegisterInstanceVCloth(pCSkinRenderModel, this); //register this CAttachmentVCLOTH instance in CSkin.
    }

    CCharInstance* pInstanceSkel = m_pAttachmentManager->m_pSkelInstance;
    CDefaultSkeleton* pDefaultSkeleton = pInstanceSkel->m_pDefaultSkeleton;

    const char* pSkelFilePath = pDefaultSkeleton->GetModelFilePath();
    const char* pSkinFilePath = m_pSimSkin->GetModelFilePath();

    uint32 numJointsSkel = pDefaultSkeleton->m_arrModelJoints.size();
    uint32 numJointsSkin = m_pSimSkin->m_arrModelJoints.size();

    uint32 NotMatchingNames = 0;
    m_arrSimRemapTable.resize(numJointsSkin, 0);
    for (uint32 js = 0; js < numJointsSkin; js++)
    {
        const int32 nID = pDefaultSkeleton->GetJointIDByCRC32(m_pSimSkin->m_arrModelJoints[js].m_nJointCRC32Lower);
        if (nID >= 0)
        {
            m_arrSimRemapTable[js] = nID;
        }
#ifdef EDITOR_PCDEBUGCODE
        else
        {
            NotMatchingNames++, g_pILog->LogError ("The joint-name (%s) of SKIN (%s) was not found in SKEL:  %s", m_pSimSkin->m_arrModelJoints[js].m_NameModelSkin.c_str(), pSkinFilePath, pSkelFilePath);
        }
#endif
    } //for loop

    if (NotMatchingNames)
    {
        CryFatalError("CryAnimation: the simulation-attachment is supposed to have the same skeleton as the render-attachment");
        return 0; //critical! incompatible skeletons. cant create skin-attachment
    }

    {
        AZStd::unique_lock<AZStd::mutex> lock(m_remapMutex);
        PatchSimRemapping(pDefaultSkeleton);
    }

    return 1;
}


void CAttachmentVCLOTH::ClearBinding(uint32 nLoadingFlags)
{
    if (m_pIAttachmentObject)
    {
        m_pIAttachmentObject->Release();
        m_pIAttachmentObject = 0;

        ReleaseRenderSkin();
        ReleaseSimSkin();

        if (nLoadingFlags & CA_SkipSkelRecreation)
        {
            return;
        }
        //for now limited to CharEdit
        //      CCharInstance* pInstanceSkel = m_pAttachmentManager->m_pSkelInstance;
        //      if (pInstanceSkel->GetCharEditMode())
        //          RecreateDefaultSkeleton(pInstanceSkel,CA_CharEditModel|nLoadingFlags);
    }
};

void CAttachmentVCLOTH::RecreateDefaultSkeleton(CCharInstance* pInstanceSkel, uint32 nLoadingFlags)
{
    CDefaultSkeleton* pDefaultSkeleton = pInstanceSkel->m_pDefaultSkeleton;
    const char* pOriginalFilePath = pDefaultSkeleton->GetModelFilePath();
    if (pDefaultSkeleton->GetModelFilePathCRC64() && pOriginalFilePath[0] == '_')
    {
        pOriginalFilePath++; //all extended skeletons have an '_' in front of the filepath to not confuse them with regular skeletons.
    }
    _smart_ptr<CDefaultSkeleton> pOrigDefaultSkeleton = g_pCharacterManager->CheckIfModelSKELLoadedAutoRef(pOriginalFilePath, nLoadingFlags);
    if (pOrigDefaultSkeleton == 0)
    {
        return;
    }
    pOrigDefaultSkeleton->SetKeepInMemory(true);
    uint64 nExtendedCRC64 = CCrc32::ComputeLowercase(pOriginalFilePath);

    static DynArray<const char*> arrNotMatchingSkins;
    arrNotMatchingSkins.resize(0);
    uint32 numAttachments = m_pAttachmentManager->GetAttachmentCount();
    for (uint32 i = 0; i < numAttachments; i++)
    {
        IAttachment* pIAttachment = m_pAttachmentManager->m_arrAttachments[i];
        if (pIAttachment->GetType() != CA_VCLOTH)
        {
            continue;
        }
        CAttachmentVCLOTH* pAttachmentSkin = (CAttachmentVCLOTH*)pIAttachment;
        CSkin* pSkin = pAttachmentSkin->m_pRenderSkin;
        if (pSkin == 0)
        {
            continue;
        }
        const char* pname = pSkin->GetModelFilePath();
        arrNotMatchingSkins.push_back(pname);
        nExtendedCRC64 += CCrc32::ComputeLowercase(pname);
    }

    CDefaultSkeleton* pExtDefaultSkeleton = g_pCharacterManager->CreateExtendedSkel(pInstanceSkel, pOrigDefaultSkeleton, nExtendedCRC64, arrNotMatchingSkins, nLoadingFlags);
    arrNotMatchingSkins.resize(0);
    if (pExtDefaultSkeleton)
    {
        pInstanceSkel->RuntimeInit(pExtDefaultSkeleton);
        m_pAttachmentManager->m_TypeSortingRequired++;
        m_pAttachmentManager->UpdateAllRemapTables();
    }
}

void CAttachmentVCLOTH::UpdateRemapTable()
{
    if (m_pRenderSkin == 0)
    {
        return;
    }

    ReleaseRenderRemapTablePair();
    ReleaseSimRemapTablePair();
    CCharInstance* pInstanceSkel = m_pAttachmentManager->m_pSkelInstance;
    CDefaultSkeleton* pDefaultSkeleton = pInstanceSkel->m_pDefaultSkeleton;

    const char* pSkelFilePath = pDefaultSkeleton->GetModelFilePath();
    const char* pSkinFilePath = m_pRenderSkin->GetModelFilePath();

    uint32 numJointsSkel = pDefaultSkeleton->m_arrModelJoints.size();
    uint32 numJointsSkin = m_pRenderSkin->m_arrModelJoints.size();

    m_arrRemapTable.resize(numJointsSkin, 0);
    for (uint32 js = 0; js < numJointsSkin; js++)
    {
        const int32 nID = pDefaultSkeleton->GetJointIDByCRC32(m_pRenderSkin->m_arrModelJoints[js].m_nJointCRC32Lower);
        if (nID >= 0)
        {
            m_arrRemapTable[js] = nID;
        }
        else
        {
            CryFatalError("ModelError: data-corruption when executing UpdateRemapTable for SKEL (%s) and SKIN (%s) ", pSkelFilePath, pSkinFilePath); //a fail in this case is virtually impossible, because the SKINs are already attached
        }
    }

    {
        AZStd::unique_lock<AZStd::mutex> lock(m_remapMutex);
        PatchRemapping(pDefaultSkeleton);
        PatchSimRemapping(pDefaultSkeleton);
    }

}

void CAttachmentVCLOTH::PatchRemapping(CDefaultSkeleton* pDefaultSkeleton)
{
    for (size_t i = 0; i < m_pRenderSkin->GetNumLODs(); i++)
    {
        CModelMesh* pModelMesh = m_pRenderSkin->GetModelMesh(i);
        IRenderMesh* pRenderMesh = pModelMesh->m_pIRenderMesh;

        if (pRenderMesh)
        {
            pRenderMesh->CreateRemappedBoneIndicesPair(m_arrRemapTable, pDefaultSkeleton->GetGuid());
            m_hasRemappedRenderMesh[i] = true;
        }
    }
}

void CAttachmentVCLOTH::PatchSimRemapping(CDefaultSkeleton* pDefaultSkeleton)
{
    CModelMesh* pSimModelMesh = m_pSimSkin->GetModelMesh(0);
    IRenderMesh* pSimRenderMesh = pSimModelMesh->m_pIRenderMesh;

    if (pSimRenderMesh)
    {
        pSimRenderMesh->CreateRemappedBoneIndicesPair(m_arrSimRemapTable, pDefaultSkeleton->GetGuid());
        m_hasRemappedSimMesh = true;
    }
}

void CAttachmentVCLOTH::ReleaseSimRemapTablePair()
{
    if (!m_pSimSkin)
    {
        return;
    }

    AZStd::unique_lock<AZStd::mutex> lock(m_remapMutex);

    CCharInstance* pInstanceSkel = m_pAttachmentManager->m_pSkelInstance;
    CDefaultSkeleton* pModelSkel = pInstanceSkel->m_pDefaultSkeleton;
    const uint skeletonGuid = pModelSkel->GetGuid();

    CModelMesh* pModelSimMesh = m_pSimSkin->GetModelMesh(0);
    IRenderMesh* pSimRenderMesh = pModelSimMesh->m_pIRenderMesh;
    if (pSimRenderMesh)
    {
        m_hasRemappedSimMesh = false;
        pSimRenderMesh->ReleaseRemappedBoneIndicesPair(skeletonGuid);
    }
}

void CAttachmentVCLOTH::ReleaseRenderRemapTablePair()
{
    if (!m_pRenderSkin)
    {
        return;
    }

    AZStd::unique_lock<AZStd::mutex> lock(m_remapMutex);

    CCharInstance* pInstanceSkel = m_pAttachmentManager->m_pSkelInstance;
    CDefaultSkeleton* pModelSkel = pInstanceSkel->m_pDefaultSkeleton;
    const uint skeletonGuid = pModelSkel->GetGuid();
    for (size_t i = 0; i < m_pRenderSkin->GetNumLODs(); i++)
    {
        CModelMesh* pModelMesh = m_pRenderSkin->GetModelMesh(i);
        IRenderMesh* pRenderMesh = pModelMesh->m_pIRenderMesh;

        if (pRenderMesh)
        {
            m_hasRemappedRenderMesh[i] = false;
            pRenderMesh->ReleaseRemappedBoneIndicesPair(skeletonGuid);
        }
        
    }
}

uint32 CAttachmentVCLOTH::SwapBinding(IAttachment* pNewAttachment)
{
    CryWarning(VALIDATOR_MODULE_ANIMATION, VALIDATOR_WARNING, "CAttachmentVCLOTH::SwapBinding attempting to swap skin attachment bindings this is not supported");
    return 0;
}

float CAttachmentVCLOTH::GetExtent(EGeomForm eForm)
{
    if (m_pRenderSkin)
    {
        int nLOD = m_pRenderSkin->SelectNearestLoadedLOD(0);
        if (IRenderMesh* pMesh = m_pRenderSkin->GetIRenderMesh(nLOD))
        {
            return pMesh->GetExtent(eForm);
        }
    }
    return 0.f;
}

void CAttachmentVCLOTH::GetRandomPos(PosNorm& ran, EGeomForm eForm) const
{
    int nLOD = m_pRenderSkin->SelectNearestLoadedLOD(0);
    IRenderMesh* pMesh = m_pRenderSkin->GetIRenderMesh(nLOD);

    SSkinningData* pSkinningData = NULL;
    int nFrameID = gEnv->pRenderer->EF_GetSkinningPoolID();
    for (int n = 0; n < 3; n++)
    {
        int nList = (nFrameID - n) % 3;
        if (m_arrSkinningRendererData[nList].nFrameID == nFrameID - n)
        {
            pSkinningData = m_arrSkinningRendererData[nList].pSkinningData;
            break;
        }
    }

    pMesh->GetRandomPos(ran, eForm, pSkinningData);
}

const QuatTS CAttachmentVCLOTH::GetAttWorldAbsolute() const
{
    QuatTS rPhysLocation = m_pAttachmentManager->m_pSkelInstance->m_location;
    return rPhysLocation;
};

void CAttachmentVCLOTH::UpdateAttModelRelative()
{
}

int CAttachmentVCLOTH::GetGuid() const
{
    return m_pAttachmentManager->m_pSkelInstance->m_pDefaultSkeleton->GetGuid();
}

void CAttachmentVCLOTH::ReleaseRenderSkin()
{
    if (m_pRenderSkin)
    {
        g_pCharacterManager->UnregisterInstanceVCloth(m_pRenderSkin, this);
        ReleaseRenderRemapTablePair();
        m_pRenderSkin = 0;
    }
}

void CAttachmentVCLOTH::ReleaseSimSkin()
{
    if (m_pSimSkin)
    {
        g_pCharacterManager->UnregisterInstanceVCloth(m_pSimSkin, this);
        ReleaseSimRemapTablePair();
        m_pSimSkin = 0;
    }
}

CAttachmentVCLOTH::~CAttachmentVCLOTH()
{
    int nFrameID = gEnv->pRenderer->EF_GetSkinningPoolID();
    int nList = nFrameID % 3;
    if (m_arrSkinningRendererData[nList].nFrameID == nFrameID && m_arrSkinningRendererData[nList].pSkinningData)
    {
        if (m_arrSkinningRendererData[nList].pSkinningData->pAsyncJobExecutor)
        {
            m_arrSkinningRendererData[nList].pSkinningData->pAsyncJobExecutor->WaitForCompletion();
        }
    }

    m_vertexAnimation.SetClothData(NULL);
    ReleaseRenderSkin();
    ReleaseSimSkin();

    CVertexAnimation::RemoveSoftwareRenderMesh(this);
    for (uint32 j = 0; j < 2; ++j)
    {
        m_pRenderMeshsSW[j] = NULL;
    }
}


void CAttachmentVCLOTH::Serialize(TSerialize ser)
{
    if (ser.GetSerializationTarget() != eST_Network)
    {
        ser.BeginGroup("CAttachmentVCLOTH");
        bool bHideInMainPass;
        if (ser.IsWriting())
        {
            bHideInMainPass = (m_AttFlags & FLAGS_ATTACH_HIDE_MAIN_PASS) == FLAGS_ATTACH_HIDE_MAIN_PASS;
        }

        ser.Value("HideInMainPass", bHideInMainPass);

        if (ser.IsReading())
        {
            HideAttachment(bHideInMainPass);
        }
        ser.EndGroup();
    }
}

size_t CAttachmentVCLOTH::SizeOfThis()
{
    size_t nSize = sizeof(CAttachmentVCLOTH) + sizeofVector(m_strSocketName);
    nSize += m_arrRemapTable.get_alloc_size();
    return nSize;
}

void CAttachmentVCLOTH::GetMemoryUsage(ICrySizer* pSizer) const
{
    pSizer->AddObject(this, sizeof(*this));
    pSizer->AddObject(m_strSocketName);
    pSizer->AddObject(m_arrRemapTable);
}


ILINE void DrawVertexDebug(
    IRenderMesh* pRenderMesh, const QuatT& location,
    const SVertexAnimationJob* pVertexAnimation,
    const SVertexSkinData& vertexSkinData)
{
    static const ColorB SKINNING_COLORS[8] =
    {
        ColorB(0x40, 0x40, 0xff, 0xff),
        ColorB(0x40, 0x80, 0xff, 0xff),
        ColorB(0x40, 0xff, 0x40, 0xff),
        ColorB(0x80, 0xff, 0x40, 0xff),

        ColorB(0xff, 0x80, 0x40, 0xff),
        ColorB(0xff, 0x80, 0x80, 0xff),
        ColorB(0xff, 0xc0, 0xc0, 0xff),
        ColorB(0xff, 0xff, 0xff, 0xff),
    };

    // wait till the SW-Skinning jobs have finished
    while (*pVertexAnimation->pRenderMeshSyncVariable)
    {
        CrySleep(1);
    }

    IRenderMesh* pIRenderMesh = pRenderMesh;
    strided_pointer<Vec3> parrDstPositions = pVertexAnimation->vertexData.pPositions;
    strided_pointer<SPipTangents> parrDstTangents;
    parrDstTangents.data = (SPipTangents*)pVertexAnimation->vertexData.pTangents.data;
    parrDstTangents.iStride = sizeof(SPipTangents);

    uint32 numExtVertices = pIRenderMesh->GetVerticesCount();
    if (parrDstPositions && parrDstTangents)
    {
        static DynArray<Vec3>       arrDstPositions;
        static DynArray<ColorB> arrDstColors;
        uint32 numDstPositions = arrDstPositions.size();
        if (numDstPositions < numExtVertices)
        {
            arrDstPositions.resize(numExtVertices);
            arrDstColors.resize(numExtVertices);
        }

        //transform vertices by world-matrix
        for (uint32 i = 0; i < numExtVertices; ++i)
        {
            arrDstPositions[i]  = location * parrDstPositions[i];
        }

        //render faces as wireframe
        if (Console::GetInst().ca_DebugSWSkinning == 1)
        {
            for (uint i = 0; i < sizeof(SKINNING_COLORS) / sizeof(SKINNING_COLORS[0]); ++i)
            {
                g_pIRenderer->Draw2dLabel(32.0f + float(i * 16), 32.0f, 2.0f, ColorF(SKINNING_COLORS[i].r / 255.0f, SKINNING_COLORS[i].g / 255.0f, SKINNING_COLORS[i].b / 255.0f, 1.0f), false, "%d", i + 1);
            }

            for (uint32 e = 0; e < numExtVertices; e++)
            {
                uint32 w = 0;
                const SoftwareVertexBlendWeight* pBlendWeights = &vertexSkinData.pVertexTransformWeights[e];
                for (uint c = 0; c < vertexSkinData.vertexTransformCount; ++c)
                {
                    if (pBlendWeights[c] > 0.0f)
                    {
                        w++;
                    }
                }

                if (w)
                {
                    --w;
                }
                arrDstColors[e] = w < 8 ? SKINNING_COLORS[w] : ColorB(0x00, 0x00, 0x00, 0xff);
            }

            pIRenderMesh->LockForThreadAccess();
            uint32  numIndices = pIRenderMesh->GetIndicesCount();
            vtx_idx* pIndices = pIRenderMesh->GetIndexPtr(FSL_READ);

            IRenderAuxGeom* pAuxGeom =  gEnv->pRenderer->GetIRenderAuxGeom();
            SAuxGeomRenderFlags renderFlags(e_Def3DPublicRenderflags);
            renderFlags.SetFillMode(e_FillModeWireframe);
            //      renderFlags.SetAlphaBlendMode(e_AlphaAdditive);
            renderFlags.SetDrawInFrontMode(e_DrawInFrontOn);
            pAuxGeom->SetRenderFlags(renderFlags);
            //  pAuxGeom->DrawTriangles(&arrDstPositions[0],numExtVertices, pIndices,numIndices,RGBA8(0x00,0x17,0x00,0x00));
            pAuxGeom->DrawTriangles(&arrDstPositions[0], numExtVertices, pIndices, numIndices, &arrDstColors[0]);

            pIRenderMesh->UnLockForThreadAccess();
        }

        //render the Normals
        if (Console::GetInst().ca_DebugSWSkinning == 2)
        {
            IRenderAuxGeom* pAuxGeom =  gEnv->pRenderer->GetIRenderAuxGeom();
            static std::vector<ColorB> arrExtVColors;
            uint32 csize = arrExtVColors.size();
            if (csize < (numExtVertices * 2))
            {
                arrExtVColors.resize(numExtVertices * 2);
            }
            for (uint32 i = 0; i < numExtVertices * 2; i = i + 2)
            {
                arrExtVColors[i + 0] = RGBA8(0x00, 0x00, 0x3f, 0x1f);
                arrExtVColors[i + 1] = RGBA8(0x7f, 0x7f, 0xff, 0xff);
            }

            Matrix33 WMat33 = Matrix33(location.q);
            static std::vector<Vec3> arrExtSkinnedStream;
            uint32 numExtSkinnedStream = arrExtSkinnedStream.size();
            if (numExtSkinnedStream < (numExtVertices * 2))
            {
                arrExtSkinnedStream.resize(numExtVertices * 2);
            }
            for (uint32 i = 0, t = 0; i < numExtVertices; i++)
            {
                Vec3 vNormal = parrDstTangents[i].GetN().GetNormalized() * 0.03f;

                arrExtSkinnedStream[t + 0] = arrDstPositions[i];
                arrExtSkinnedStream[t + 1] = WMat33 * vNormal + arrExtSkinnedStream[t];
                t = t + 2;
            }
            SAuxGeomRenderFlags renderFlags(e_Def3DPublicRenderflags);
            pAuxGeom->SetRenderFlags(renderFlags);
            pAuxGeom->DrawLines(&arrExtSkinnedStream[0], numExtVertices * 2, &arrExtVColors[0]);
        }
    }
}

_smart_ptr<IRenderMesh> CAttachmentVCLOTH::CreateVertexAnimationRenderMesh(uint lod, uint id)
{
    m_pRenderMeshsSW[id] = NULL; // smart pointer release

    if (m_pRenderSkin == 0)
    {
        return _smart_ptr<IRenderMesh>(NULL);
    }

    uint32 numModelMeshes = m_pRenderSkin->m_arrModelMeshes.size();
    if (lod >= numModelMeshes)
    {
        return _smart_ptr<IRenderMesh>(NULL);
    }

    _smart_ptr<IRenderMesh> pIStaticRenderMesh = m_pRenderSkin->m_arrModelMeshes[lod].m_pIRenderMesh;
    if (pIStaticRenderMesh == 0)
    {
        return _smart_ptr<IRenderMesh>(NULL);
    }
    ;

    CModelMesh* pModelMesh = m_pRenderSkin->GetModelMesh(lod);
    uint32 success = pModelMesh->InitSWSkinBuffer();
    if (success == 0)
    {
        return _smart_ptr<IRenderMesh>(NULL);
    }
    ;

    if (m_sSoftwareMeshName.empty() && pIStaticRenderMesh->GetSourceName() != NULL)
    {
        m_sSoftwareMeshName.reserve(strlen(pIStaticRenderMesh->GetSourceName()) + 3);
        m_sSoftwareMeshName = pIStaticRenderMesh->GetSourceName();
        m_sSoftwareMeshName += "_SW";
    }

    m_pRenderMeshsSW[id] = g_pIRenderer->CreateRenderMeshInitialized(
            NULL
            , pIStaticRenderMesh->GetVerticesCount()
            , eVF_P3F_C4B_T2F
            , NULL
            , pIStaticRenderMesh->GetIndicesCount()
            , prtTriangleList
            , "Character"
            , m_sSoftwareMeshName.c_str()
            , eRMT_Transient);

    m_pRenderMeshsSW[id]->SetMeshLod(lod);

    TRenderChunkArray& chunks = pIStaticRenderMesh->GetChunks();
    TRenderChunkArray nchunks;
    nchunks.resize(chunks.size());
    for (size_t i = 0; i < size_t(chunks.size()); ++i)
    {
        nchunks[i].m_vertexFormat = chunks[i].m_vertexFormat;
        nchunks[i].m_texelAreaDensity = chunks[i].m_texelAreaDensity;
        nchunks[i].nFirstIndexId = chunks[i].nFirstIndexId;
        nchunks[i].nNumIndices = chunks[i].nNumIndices;
        nchunks[i].nFirstVertId = chunks[i].nFirstVertId;
        nchunks[i].nNumVerts = chunks[i].nNumVerts;
#ifdef SUBDIVISION_ACC_ENGINE
        nchunks[i].nFirstFaceId = chunks[i].nFirstFaceId;
        nchunks[i].nNumFaces = chunks[i].nNumFaces;
        nchunks[i].nPrimitiveType = chunks[i].nPrimitiveType;
#endif
        nchunks[i].m_nMatFlags = chunks[i].m_nMatFlags;
        nchunks[i].m_nMatID = chunks[i].m_nMatID;
        nchunks[i].nSubObjectIndex = chunks[i].nSubObjectIndex;
    }
    m_pRenderMeshsSW[id]->SetRenderChunks(&nchunks[0], nchunks.size(), false);

#ifndef _RELEASE
    m_vertexAnimation.m_vertexAnimationStats.sCharInstanceName = m_pAttachmentManager->m_pSkelInstance->GetFilePath();
    m_vertexAnimation.m_vertexAnimationStats.sAttachmentName = "";//m_Name; TODO fix
    m_vertexAnimation.m_vertexAnimationStats.pCharInstance = m_pAttachmentManager->m_pSkelInstance;
#endif
    return m_pRenderMeshsSW[id];
}

void CAttachmentVCLOTH::DrawAttachment(SRendParams& RendParams, const SRenderingPassInfo& passInfo, const Matrix34& rWorldMat34, f32 fZoomFactor)
{
    //-----------------------------------------------------------------------------
    //---              map logical LOD to render LOD                            ---
    //-----------------------------------------------------------------------------
    uint32 ddd = GetType();
    int32 numLODs = m_pRenderSkin->m_arrModelMeshes.size();
    if (numLODs == 0)
    {
        return;
    }
    int nDesiredRenderLOD = RendParams.lodValue.LodA();
    if (nDesiredRenderLOD >= numLODs)
    {
        if (m_AttFlags & FLAGS_ATTACH_RENDER_ONLY_EXISTING_LOD)
        {
            return;  //early exit, if LOD-file doesn't exist
        }
        nDesiredRenderLOD = numLODs - 1; //render the last existing LOD-file
    }
    int nRenderLOD = m_pRenderSkin->SelectNearestLoadedLOD(nDesiredRenderLOD);  //we can render only loaded LODs

    m_pRenderSkin->m_arrModelMeshes[nRenderLOD].m_stream.nFrameId = passInfo.GetMainFrameID();
    m_pSimSkin->m_arrModelMeshes[nRenderLOD].m_stream.nFrameId = passInfo.GetMainFrameID();

    Matrix34 FinalMat34 = rWorldMat34;
    RendParams.pMatrix = &FinalMat34;
    RendParams.pInstance = this;
    RendParams.pMaterial = (_smart_ptr<IMaterial>)m_pIAttachmentObject->GetReplacementMaterial(nRenderLOD); //the Replacement-Material has priority
    if (RendParams.pMaterial == 0)
    {
        RendParams.pMaterial = (_smart_ptr<IMaterial>)m_pRenderSkin->GetIMaterial(nRenderLOD); //as a fall back take the Base-Material from the model
    }
    bool bNewFrame = false;
    if (m_vertexAnimation.m_skinningPoolID != gEnv->pRenderer->EF_GetSkinningPoolID())
    {
        m_vertexAnimation.m_skinningPoolID = gEnv->pRenderer->EF_GetSkinningPoolID();
        ++m_vertexAnimation.m_RenderMeshId;
        bNewFrame = true;
    }

    if (bNewFrame && nRenderLOD < m_clothPiece.GetNumLods())
    {
        bool visible = m_clothPiece.IsAlwaysVisible();
        if (m_clothPiece.PrepareCloth(m_pAttachmentManager->m_pSkelInstance->m_SkeletonPose, rWorldMat34, visible, nRenderLOD))
        {
            m_AttFlags |= FLAGS_ATTACH_SW_SKINNING;
            m_vertexAnimation.SetClothData(&m_clothPiece);
        }
        else
        {
            m_AttFlags &= ~FLAGS_ATTACH_SW_SKINNING;
            m_vertexAnimation.SetClothData(NULL);
        }
    }

    //---------------------------------------------------------------------------------------------
    //---------------------------------------------------------------------------------------------
    //---------------------------------------------------------------------------------------------
    CRenderObject* pObj = g_pIRenderer->EF_GetObject_Temp(passInfo.ThreadID());
    if (!pObj)
    {
        return;
    }

    pObj->m_fSort = RendParams.fCustomSortOffset;
    uint64 uLocalObjFlags = pObj->m_ObjFlags;

    //check if it should be drawn close to the player
    CCharInstance* pMaster = m_pAttachmentManager->m_pSkelInstance;
    if (pMaster->m_rpFlags & CS_FLAG_DRAW_NEAR)
    {
        uLocalObjFlags |= FOB_NEAREST;
    }
    else
    {
        uLocalObjFlags &= ~FOB_NEAREST;
    }

    pObj->m_fAlpha = RendParams.fAlpha;
    pObj->m_fDistance = RendParams.fDistance;
    pObj->m_II.m_AmbColor = RendParams.AmbientColor;

    uLocalObjFlags |= RendParams.dwFObjFlags;

    SRenderObjData* pD = g_pIRenderer->EF_GetObjData(pObj, true, passInfo.ThreadID());

    // copy the shaderparams into the render object data from the render params
    if (RendParams.pShaderParams && RendParams.pShaderParams->size() > 0)
    {
        pD->SetShaderParams(RendParams.pShaderParams);
    }

    pD->m_uniqueObjectId = reinterpret_cast<uintptr_t>(RendParams.pInstance);

    bool bCheckMotion = pMaster->MotionBlurMotionCheck(pObj->m_ObjFlags);
    if (bCheckMotion)
    {
        uLocalObjFlags |= FOB_MOTION_BLUR;
    }

    Matrix34 RenderMat34 = (*RendParams.pMatrix);
    pObj->m_II.m_Matrix = RenderMat34;
    pObj->m_nClipVolumeStencilRef = RendParams.nClipVolumeStencilRef;
    pObj->m_nTextureID = RendParams.nTextureID;

    pObj->m_nMaterialLayers = RendParams.nMaterialLayersBlend;

    pD->m_nHUDSilhouetteParams = RendParams.nHUDSilhouettesParams;

    if (RendParams.pTerrainTexInfo && (RendParams.dwFObjFlags & (FOB_BLEND_WITH_TERRAIN_COLOR /* | FOB_AMBIENT_OCCLUSION*/)))
    {
        pObj->m_nTextureID = RendParams.pTerrainTexInfo->nTex0;
        pD->m_fTempVars[0] = RendParams.pTerrainTexInfo->fTexOffsetX;
        pD->m_fTempVars[1] = RendParams.pTerrainTexInfo->fTexOffsetY;
        pD->m_fTempVars[2] = RendParams.pTerrainTexInfo->fTexScale;
    }
    else
    {
        Vec3 skinOffset = m_pRenderSkin->m_arrModelMeshes[nRenderLOD].m_vRenderMeshOffset;
        pD->m_fTempVars[0] = skinOffset.x;
        pD->m_fTempVars[1] = skinOffset.y;
        pD->m_fTempVars[2] = skinOffset.z;
    }

    pD->m_nCustomData = RendParams.nCustomData;
    pD->m_nCustomFlags = RendParams.nCustomFlags;

    pObj->m_DissolveRef = RendParams.nDissolveRef;

    pObj->m_ObjFlags = uLocalObjFlags;

    if (g_pI3DEngine->IsTessellationAllowed(pObj, passInfo))
    {
        // Allow this RO to be tessellated, however actual tessellation will be applied if enabled in material
        pObj->m_ObjFlags |= FOB_ALLOW_TESSELLATION;
    }

    pObj->m_nSort = fastround_positive(RendParams.fDistance * 2.0f);

    const float SORT_BIAS_AMOUNT = 1.f;
    if (pMaster->m_rpFlags & CS_FLAG_BIAS_SKIN_SORT_DIST)
    {
        pObj->m_fSort = SORT_BIAS_AMOUNT;
    }


    // get skinning data
    const int numClothLods = m_clothPiece.GetNumLods();
    const bool swSkin = (nRenderLOD == 0) && (Console::GetInst().ca_vaEnable != 0) && (nRenderLOD < numClothLods);
    IF (!swSkin, 1)
    {
        pObj->m_ObjFlags |= FOB_SKINNED;
    }

    pD->m_pSkinningData = GetVertexTransformationData(swSkin, nRenderLOD);
    m_pRenderSkin->m_arrModelMeshes[nRenderLOD].m_stream.nFrameId = passInfo.GetMainFrameID();
    m_pSimSkin->m_arrModelMeshes[nRenderLOD].m_stream.nFrameId = passInfo.GetMainFrameID();

    SVertexAnimationJob* pVertexAnimation = static_cast<SVertexAnimationJob*>(pD->m_pSkinningData->pCustomData);

    IRenderMesh* pRenderMesh = m_pRenderSkin->GetIRenderMesh(nRenderLOD);
    if (swSkin && pRenderMesh)
    {
        pObj->m_ObjFlags |= FOB_VERTEX_VELOCITY;

        uint iCurrentRenderMeshID = m_vertexAnimation.m_RenderMeshId & 1;

        if (bNewFrame)
        {
            CreateVertexAnimationRenderMesh(nRenderLOD, iCurrentRenderMeshID);
            CVertexAnimation::RegisterSoftwareRenderMesh((CAttachmentVCLOTH*)this);
        }

        pRenderMesh = m_pRenderMeshsSW[iCurrentRenderMeshID];

        IF (pRenderMesh && bNewFrame, 1)
        {
            CModelMesh* pModelMesh = m_pRenderSkin->GetModelMesh(nRenderLOD);
            CSoftwareMesh& geometry = pModelMesh->m_softwareMesh;

            SVertexSkinData vertexSkinData(pD, geometry);
            CRY_ASSERT(pRenderMesh->GetVerticesCount() == geometry.GetVertexCount());

#if defined(AZ_RESTRICTED_PLATFORM)
#include AZ_RESTRICTED_FILE(AttachmentVCloth_cpp, AZ_RESTRICTED_PLATFORM)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
            const uint fslCreate = FSL_SYSTEM_CREATE;
            const uint fslRead = FSL_READ;
#endif

            vertexSkinData.pVertexPositionsPrevious = strided_pointer<const Vec3>(NULL);
            if (pD->m_pSkinningData->pPreviousSkinningRenderData)
            {
                pD->m_pSkinningData->pPreviousSkinningRenderData->pAsyncJobExecutor->WaitForCompletion();
            }
            _smart_ptr<IRenderMesh>& pRenderMeshPrevious = m_pRenderMeshsSW[1 - iCurrentRenderMeshID];
            if (pRenderMeshPrevious != NULL)
            {
                pRenderMeshPrevious->LockForThreadAccess();
                Vec3* pPrevPositions = (Vec3*)pRenderMeshPrevious->GetPosPtrNoCache(vertexSkinData.pVertexPositionsPrevious.iStride, fslRead);
                if (pPrevPositions)
                {
                    vertexSkinData.pVertexPositionsPrevious.data = pPrevPositions;
                    pVertexAnimation->m_previousRenderMesh = pRenderMeshPrevious;
                }
                else
                {
                    pRenderMeshPrevious->UnlockStream(VSF_GENERAL);
                    pRenderMeshPrevious->UnLockForThreadAccess();
                }
            }
            m_vertexAnimation.SetSkinData(vertexSkinData);

            pVertexAnimation->vertexData.m_vertexCount = pRenderMesh->GetVerticesCount();

            pRenderMesh->LockForThreadAccess();

            SVF_P3F_C4B_T2F* pGeneral = (SVF_P3F_C4B_T2F*)pRenderMesh->GetPosPtrNoCache(pVertexAnimation->vertexData.pPositions.iStride, fslCreate);

            pVertexAnimation->vertexData.pPositions.data = (Vec3*)(&pGeneral[0].xyz);
            pVertexAnimation->vertexData.pPositions.iStride = sizeof(pGeneral[0]);
            pVertexAnimation->vertexData.pColors.data = (uint32*)(&pGeneral[0].color);
            pVertexAnimation->vertexData.pColors.iStride = sizeof(pGeneral[0]);
            pVertexAnimation->vertexData.pCoords.data = (Vec2*)(&pGeneral[0].st);
            pVertexAnimation->vertexData.pCoords.iStride = sizeof(pGeneral[0]);
            pVertexAnimation->vertexData.pVelocities.data = (Vec3*)pRenderMesh->GetVelocityPtr(pVertexAnimation->vertexData.pVelocities.iStride, fslCreate);
            pVertexAnimation->vertexData.pTangents.data = (SPipTangents*)pRenderMesh->GetTangentPtr(pVertexAnimation->vertexData.pTangents.iStride, fslCreate);
            pVertexAnimation->vertexData.pIndices = pRenderMesh->GetIndexPtr(fslCreate);
            pVertexAnimation->vertexData.m_indexCount = geometry.GetIndexCount();

            if (!pVertexAnimation->vertexData.pPositions ||
                !pVertexAnimation->vertexData.pVelocities ||
                !pVertexAnimation->vertexData.pTangents)
            {
                pRenderMesh->UnlockStream(VSF_GENERAL);
                pRenderMesh->UnlockStream(VSF_TANGENTS);
                pRenderMesh->UnlockStream(VSF_VERTEX_VELOCITY);
#if ENABLE_NORMALSTREAM_SUPPORT
                pRenderMesh->UnlockStream(VSF_NORMALS);
#endif
                pRenderMesh->UnLockForThreadAccess();
                return;
            }

            // If memory was pre-allocated for the command buffer, recompile into it.
            if (pVertexAnimation->commandBufferLength)
            {
                CVertexCommandBufferAllocatorStatic commandBufferAllocator(
                    (uint8*)pD->m_pSkinningData->pCustomData + sizeof(SVertexAnimationJob),
                    pVertexAnimation->commandBufferLength);
                pVertexAnimation->commandBuffer.Initialize(commandBufferAllocator);
                m_vertexAnimation.CompileCommands(pVertexAnimation->commandBuffer);
            }

            pVertexAnimation->pRenderMeshSyncVariable = pRenderMesh->SetAsyncUpdateState();

            SSkinningData* pCurrentJobSkinningData = *pD->m_pSkinningData->pMasterSkinningDataList;
            if (pCurrentJobSkinningData == NULL)
            {
                pVertexAnimation->Begin(pD->m_pSkinningData->pAsyncJobExecutor);
            }
            else
            {
                // try to append to list
                pD->m_pSkinningData->pNextSkinningData = pCurrentJobSkinningData;
                void* pUpdatedJobSkinningData = CryInterlockedCompareExchangePointer(alias_cast<void* volatile*>(pD->m_pSkinningData->pMasterSkinningDataList), pD->m_pSkinningData, pCurrentJobSkinningData);

                // in case we failed (job has finished in the meantime), we need to start the job from the main thread
                if (pUpdatedJobSkinningData == NULL)
                {
                    pVertexAnimation->Begin(pD->m_pSkinningData->pAsyncJobExecutor);
                }
            }

            if (Console::GetInst().ca_DebugSWSkinning)
            {
                DrawVertexDebug(pRenderMesh, QuatT(RenderMat34), pVertexAnimation, vertexSkinData);
            }

            pRenderMesh->UnLockForThreadAccess();
            if (m_clothPiece.NeedsDebugDraw())
            {
                m_clothPiece.DrawDebug(pVertexAnimation);
            }
            if (!(Console::GetInst().ca_DrawCloth & 1))
            {
                return;
            }
        }
    }

    //  CryFatalError("CryAnimation: pMaster is zero");
    if (pRenderMesh)
    {
        //g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, ColorF(0,1,0,1), false,"VCloth name: %s",GetName()),g_YLine+=16.0f;

        _smart_ptr<IMaterial> pMaterial = RendParams.pMaterial;
        if (pMaterial == 0)
        {
            pMaterial = m_pRenderSkin->GetIMaterial(0);
        }
#ifndef _RELEASE
        CModelMesh* pModelMesh = m_pRenderSkin->GetModelMesh(nRenderLOD);
        static ICVar* p_e_debug_draw = gEnv->pConsole->GetCVar("e_DebugDraw");
        if (p_e_debug_draw && p_e_debug_draw->GetIVal() > 0)
        {
            pModelMesh->DrawDebugInfo(pMaster->m_pDefaultSkeleton, nRenderLOD, RenderMat34, p_e_debug_draw->GetIVal(), pMaterial, pObj, RendParams, passInfo.IsGeneralPass(), (IRenderNode*)RendParams.pRenderNode, m_pAttachmentManager->m_pSkelInstance->m_SkeletonPose.GetAABB());
        }
#endif
        pRenderMesh->Render(RendParams, pObj, pMaterial, passInfo);

        //------------------------------------------------------------------
        //---       render debug-output (only PC in CharEdit mode)       ---
        //------------------------------------------------------------------
#if EDITOR_PCDEBUGCODE
        if (pMaster->m_CharEditMode & CA_CharacterTool)
        {
            const Console& rConsole = Console::GetInst();

            uint32 tang = rConsole.ca_DrawTangents;
            uint32 bitang = rConsole.ca_DrawBinormals;
            uint32 norm = rConsole.ca_DrawNormals;
            uint32 wire = rConsole.ca_DrawWireframe;
            if (tang || bitang || norm || wire)
            {
                CModelMesh* pModelMesh = m_pRenderSkin->GetModelMesh(nRenderLOD);
                pD->m_pSkinningData->pAsyncJobExecutor->WaitForCompletion();
                //SoftwareSkinningDQ(pModelMesh, pObj->m_II.m_Matrix,   tang,bitang,norm,wire, pD->m_pSkinningData->pBoneQuatsS);
                SoftwareSkinningDQ_VS_Emulator(pModelMesh, pObj->m_II.m_Matrix, tang, bitang, norm, wire, pD->m_pSkinningData->pBoneQuatsS);
            }
        }
#endif
    }
}




//-----------------------------------------------------------------------------
//---     trigger streaming of desired LODs (only need in CharEdit)         ---
//-----------------------------------------------------------------------------
void CAttachmentVCLOTH::TriggerMeshStreaming(uint32 nDesiredRenderLOD, const SRenderingPassInfo& passInfo)
{
    if (!m_pRenderSkin)
    {
        return;
    }

    uint32 numLODs = m_pRenderSkin->m_arrModelMeshes.size();
    if (numLODs == 0)
    {
        return;
    }
    if (m_AttFlags & FLAGS_ATTACH_HIDE_ATTACHMENT)
    {
        return;  //mesh not visible
    }
    if (nDesiredRenderLOD >= numLODs)
    {
        if (m_AttFlags & FLAGS_ATTACH_RENDER_ONLY_EXISTING_LOD)
        {
            return;  //early exit, if LOD-file doesn't exist
        }
        nDesiredRenderLOD = numLODs - 1; //render the last existing LOD-file
    }
    m_pRenderSkin->m_arrModelMeshes[nDesiredRenderLOD].m_stream.nFrameId = passInfo.GetMainFrameID();
    m_pSimSkin->m_arrModelMeshes[nDesiredRenderLOD].m_stream.nFrameId = passInfo.GetMainFrameID();
}

SSkinningData* CAttachmentVCLOTH::GetVertexTransformationData(bool bVertexAnimation, uint8 nRenderLOD)
{
    DEFINE_PROFILER_FUNCTION();
    CCharInstance* pMaster = m_pAttachmentManager->m_pSkelInstance;
    if (pMaster == 0)
    {
        CryFatalError("CryAnimation: pMaster is zero");
        return NULL;
    }


    uint32 skinningQuatCount = m_arrRemapTable.size();
    uint32 skinningQuatCountMax = pMaster->GetSkinningTransformationCount();
    uint32 nNumBones = min(skinningQuatCount, skinningQuatCountMax);

    // get data to fill
    int nFrameID = gEnv->pRenderer->EF_GetSkinningPoolID();
    int nList = nFrameID % tripleBufferSize;
    int nPrevList = (nFrameID - 1) % tripleBufferSize;
    
    // before allocating new skinning date, check if we already have for this frame
    if (m_arrSkinningRendererData[nList].nFrameID == nFrameID && m_arrSkinningRendererData[nList].pSkinningData)
    {
        return m_arrSkinningRendererData[nList].pSkinningData;
    }

    if (pMaster->arrSkinningRendererData[nList].nFrameID != nFrameID)
    {
        pMaster->GetSkinningData(); // force master to compute skinning data if not available
    }

    uint32 nCustomDataSize = 0;
    uint commandBufferLength = 0;
    if (bVertexAnimation)
    {
        // Make sure the software skinning command gets compiled.
        SVertexSkinData vertexSkinData;
        vertexSkinData.transformationCount = 1;
        vertexSkinData.vertexTransformCount = 4;
        vertexSkinData.tangetUpdateTriCount = 1;
        m_vertexAnimation.SetSkinData(vertexSkinData);

        // Compile the command buffer without allocating the commands to
        // compute the buffer length.
        CVertexCommandBufferAllocationCounter commandBufferAllocationCounter;
        CVertexCommandBuffer commandBuffer;
        commandBuffer.Initialize(commandBufferAllocationCounter);
        m_vertexAnimation.CompileCommands(commandBuffer);
        commandBufferLength = commandBufferAllocationCounter.GetCount();

        nCustomDataSize = sizeof(SVertexAnimationJob) + commandBufferLength;
    }

    SSkinningData* pSkinningData = gEnv->pRenderer->EF_CreateRemappedSkinningData(nNumBones, pMaster->arrSkinningRendererData[nList].pSkinningData, nCustomDataSize, pMaster->m_pDefaultSkeleton->GetGuid());
    if (nCustomDataSize)
    {
        SVertexAnimationJob* pVertexAnimation = new (pSkinningData->pCustomData)SVertexAnimationJob();
        pVertexAnimation->commandBufferLength = commandBufferLength;
    }
    m_arrSkinningRendererData[nList].pSkinningData = pSkinningData;
    m_arrSkinningRendererData[nList].nNumBones = nNumBones;
    m_arrSkinningRendererData[nList].nFrameID = nFrameID;

    //clear obsolete data
    int nPrevPrevList = (nFrameID - 2) % tripleBufferSize;
    if (nPrevPrevList >= 0)
    {
        if (m_arrSkinningRendererData[nPrevPrevList].nFrameID == (nFrameID - 2) && m_arrSkinningRendererData[nPrevPrevList].pSkinningData)
        {
            m_arrSkinningRendererData[nPrevPrevList].pSkinningData->pPreviousSkinningRenderData = nullptr;
        }
        else
        {
            // If nFrameID was off by more than 2 frames old, then this data is guaranteed to be stale if it exists.  Clear it to be safe.
            // The triple-buffered pool allocator in EF_CreateRemappedSkinningData will have already freed this data if the frame count/IDs mismatch.
            m_arrSkinningRendererData[nPrevPrevList].pSkinningData = nullptr;
        }
    }

    PREFAST_ASSUME(pSkinningData);

    // set data for motion blur
    if (m_arrSkinningRendererData[nPrevList].nFrameID == (nFrameID - 1) && m_arrSkinningRendererData[nPrevList].pSkinningData)
    {
        pSkinningData->nHWSkinningFlags |= eHWS_MotionBlured;
        pSkinningData->pPreviousSkinningRenderData = m_arrSkinningRendererData[nPrevList].pSkinningData;
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

    pSkinningData->pRemapTable = &m_arrRemapTable[0];

    return pSkinningData;
}

void CAttachmentVCLOTH::ComputeGeometricMean(SMeshLodInfo& lodInfo) const
{
    CModelMesh* pModelMesh = m_pRenderSkin->GetModelMesh(0);
    if (pModelMesh)
    {
        lodInfo.fGeometricMean = pModelMesh->m_geometricMeanFaceArea;
        lodInfo.nFaceCount = pModelMesh->m_faceCount;
    }
}










#ifdef EDITOR_PCDEBUGCODE
//These functions are need only for the Editor on PC

void CAttachmentVCLOTH::DrawWireframeStatic(const Matrix34& m34, int nLOD, uint32 color)
{
    CModelMesh* pModelMesh = m_pRenderSkin->GetModelMesh(nLOD);
    if (pModelMesh)
    {
        pModelMesh->DrawWireframeStatic(m34, color);
    }
}

//////////////////////////////////////////////////////////////////////////
// skinning of external Vertices and QTangents
//////////////////////////////////////////////////////////////////////////
void CAttachmentVCLOTH::SoftwareSkinningDQ_VS_Emulator(CModelMesh* pModelMesh, Matrix34 rRenderMat34, uint8 tang, uint8 binorm, uint8 norm, uint8 wire, const DualQuat* const pSkinningTransformations)
{
#ifdef DEFINE_PROFILER_FUNCTION
    DEFINE_PROFILER_FUNCTION();
#endif

    CRenderAuxGeomRenderFlagsRestore _renderFlagsRestore(g_pAuxGeom);

    if (pModelMesh->m_pIRenderMesh == 0)
    {
        return;
    }

    static DynArray<Vec3> g_arrExtSkinnedStream;
    static DynArray<Quat> g_arrQTangents;
    static DynArray<uint8> arrSkinned;

    uint32 numExtIndices    = pModelMesh->m_pIRenderMesh->GetIndicesCount();
    uint32 numExtVertices   = pModelMesh->m_pIRenderMesh->GetVerticesCount();

    uint32 ssize = g_arrExtSkinnedStream.size();
    if (ssize < numExtVertices)
    {
        g_arrExtSkinnedStream.resize(numExtVertices);
        g_arrQTangents.resize(numExtVertices);
        arrSkinned.resize(numExtVertices);
    }
    memset(&arrSkinned[0], 0, numExtVertices);

    pModelMesh->m_pIRenderMesh->LockForThreadAccess();
    ++pModelMesh->m_iThreadMeshAccessCounter;

    vtx_idx* pIndices               = pModelMesh->m_pIRenderMesh->GetIndexPtr(FSL_READ);
    if (pIndices == 0)
    {
        return;
    }
    int32       nPositionStride;
    uint8*  pPositions          = pModelMesh->m_pIRenderMesh->GetPosPtr(nPositionStride, FSL_READ);
    if (pPositions == 0)
    {
        return;
    }
    int32       nQTangentStride;
    uint8*  pQTangents          = pModelMesh->m_pIRenderMesh->GetQTangentPtr(nQTangentStride, FSL_READ);
    if (pQTangents == 0)
    {
        return;
    }
    int32       nSkinningStride;
    uint8* pSkinningInfo               = pModelMesh->m_pIRenderMesh->GetHWSkinPtr(nSkinningStride, FSL_READ);  //pointer to weights and bone-id
    if (pSkinningInfo == 0)
    {
        return;
    }

    DualQuat arrRemapSkinQuat[MAX_JOINT_AMOUNT];    //dual quaternions for skinning
    for (size_t b = 0; b < m_arrRemapTable.size(); ++b)
    {
        const uint16 sidx = m_arrRemapTable[b]; //is array can be different for every skin-attachment
        arrRemapSkinQuat[b] = pSkinningTransformations[sidx];
    }

    const uint32 numSubsets = pModelMesh->m_arrRenderChunks.size();
    for (uint32 s = 0; s < numSubsets; s++)
    {
        float fColor[4] = {1, 0, 1, 1};
        extern f32 g_YLine;
        //  g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"MODEL: %s",pDefaultSkeleton->GetFilePath()); g_YLine+=0x10;
        uint32 startIndex       = pModelMesh->m_arrRenderChunks[s].m_nFirstIndexId;
        uint32 endIndex         = pModelMesh->m_arrRenderChunks[s].m_nFirstIndexId + pModelMesh->m_arrRenderChunks[s].m_nNumIndices;
        g_pIRenderer->Draw2dLabel(1, g_YLine, 1.3f, fColor, false, "subset: %d  startIndex: %d  endIndex: %d", s, startIndex, endIndex);
        g_YLine += 0x10;

        for (uint32 idx = startIndex; idx < endIndex; ++idx)
        {
            uint32 e = pIndices[idx];
            if (arrSkinned[e])
            {
                continue;
            }

            arrSkinned[e] = 1;
            Vec3        hwPosition  = *(Vec3*)(pPositions + e * nPositionStride) + pModelMesh->m_vRenderMeshOffset;
            SPipQTangents   hwQTangent  = *(SPipQTangents*)(pQTangents + e * nQTangentStride);
            uint16* hwIndices   = ((SVF_W4B_I4S*)(pSkinningInfo + e * nSkinningStride))->indices;
            ColorB  hwWeights       = *(ColorB*)&((SVF_W4B_I4S*)(pSkinningInfo + e * nSkinningStride))->weights;

            //---------------------------------------------------------------------
            //---     this is CPU emulation of Dual-Quat skinning              ---
            //---------------------------------------------------------------------
            //get indices for bones (always 4 indices per vertex)
            uint32 id0 = hwIndices[0];
            uint32 id1 = hwIndices[1];
            uint32 id2 = hwIndices[2];
            uint32 id3 = hwIndices[3];
            CRY_ASSERT(id0 < m_arrRemapTable.size() && id1 < m_arrRemapTable.size() && id2 < m_arrRemapTable.size() && id3 < m_arrRemapTable.size());

            //get weights for vertices (always 4 weights per vertex)
            f32 w0 = hwWeights[0] / 255.0f;
            f32 w1 = hwWeights[1] / 255.0f;
            f32 w2 = hwWeights[2] / 255.0f;
            f32 w3 = hwWeights[3] / 255.0f;
            CRY_ASSERT(fcmp(w0 + w1 + w2 + w3, 1.0f, 0.0001f));

            const DualQuat& q0 = arrRemapSkinQuat[id0];
            const DualQuat& q1 = arrRemapSkinQuat[id1];
            const DualQuat& q2 = arrRemapSkinQuat[id2];
            const DualQuat& q3 = arrRemapSkinQuat[id3];
            DualQuat wquat      =   q0 * w0 +  q1 * w1 + q2 * w2 +    q3 * w3;

            f32 l = 1.0f / wquat.nq.GetLength();
            wquat.nq *= l;
            wquat.dq *= l;
            g_arrExtSkinnedStream[e] = rRenderMat34 * (wquat * hwPosition);  //transform position by dual-quaternion

            Quat QTangent = hwQTangent.GetQ();
            CRY_ASSERT(QTangent.IsUnit());

            g_arrQTangents[e] = wquat.nq * QTangent;
            if (g_arrQTangents[e].w < 0.0f)
            {
                g_arrQTangents[e] = -g_arrQTangents[e]; //make it positive
            }
            f32 flip = QTangent.w < 0 ? -1.0f : 1.0f;
            if (flip < 0)
            {
                g_arrQTangents[e] = -g_arrQTangents[e]; //make it negative
            }
        }
    }
    g_YLine += 0x10;



    pModelMesh->m_pIRenderMesh->UnLockForThreadAccess();
    --pModelMesh->m_iThreadMeshAccessCounter;
    if (pModelMesh->m_iThreadMeshAccessCounter == 0)
    {
        pModelMesh->m_pIRenderMesh->UnlockStream(VSF_GENERAL);
        pModelMesh->m_pIRenderMesh->UnlockStream(VSF_TANGENTS);
        pModelMesh->m_pIRenderMesh->UnlockStream(VSF_HWSKIN_INFO);
    }

    //---------------------------------------------------------------------------
    //---------------------------------------------------------------------------
    //---------------------------------------------------------------------------

    uint32 numBuffer    =   g_arrExtSkinnedStream.size();
    if (numBuffer < numExtVertices)
    {
        return;
    }
    const f32 VLENGTH = 0.03f;

    if (tang)
    {
        static std::vector<ColorB> arrExtVColors;
        uint32 csize = arrExtVColors.size();
        if (csize < (numExtVertices * 2))
        {
            arrExtVColors.resize(numExtVertices * 2);
        }
        for (uint32 i = 0; i < numExtVertices * 2; i = i + 2)
        {
            arrExtVColors[i + 0] = RGBA8(0x3f, 0x00, 0x00, 0x00);
            arrExtVColors[i + 1] = RGBA8(0xff, 0x7f, 0x7f, 0x00);
        }

        Matrix33 WMat33 = Matrix33(rRenderMat34);
        static std::vector<Vec3> arrExtSkinnedStream;
        uint32 vsize = arrExtSkinnedStream.size();
        if (vsize < (numExtVertices * 2))
        {
            arrExtSkinnedStream.resize(numExtVertices * 2);
        }
        for (uint32 i = 0, t = 0; i < numExtVertices; i++)
        {
            Vec3 vTangent  = g_arrQTangents[i].GetColumn0() * VLENGTH;
            arrExtSkinnedStream[t + 0] = g_arrExtSkinnedStream[i];
            arrExtSkinnedStream[t + 1] = WMat33 * vTangent + arrExtSkinnedStream[t];
            t = t + 2;
        }

        SAuxGeomRenderFlags renderFlags(e_Def3DPublicRenderflags);
        g_pAuxGeom->SetRenderFlags(renderFlags);
        g_pAuxGeom->DrawLines(&arrExtSkinnedStream[0], numExtVertices * 2, &arrExtVColors[0]);
    }

    if (binorm)
    {
        static std::vector<ColorB> arrExtVColors;
        uint32 csize = arrExtVColors.size();
        if (csize < (numExtVertices * 2))
        {
            arrExtVColors.resize(numExtVertices * 2);
        }
        for (uint32 i = 0; i < numExtVertices * 2; i = i + 2)
        {
            arrExtVColors[i + 0] = RGBA8(0x00, 0x3f, 0x00, 0x00);
            arrExtVColors[i + 1] = RGBA8(0x7f, 0xff, 0x7f, 0x00);
        }

        Matrix33 WMat33 = Matrix33(rRenderMat34);
        static std::vector<Vec3> arrExtSkinnedStream;
        uint32 vsize = arrExtSkinnedStream.size();
        if (vsize < (numExtVertices * 2))
        {
            arrExtSkinnedStream.resize(numExtVertices * 2);
        }
        for (uint32 i = 0, t = 0; i < numExtVertices; i++)
        {
            Vec3 vBitangent = g_arrQTangents[i].GetColumn1() * VLENGTH;
            arrExtSkinnedStream[t + 0] = g_arrExtSkinnedStream[i];
            arrExtSkinnedStream[t + 1] = WMat33 * vBitangent + arrExtSkinnedStream[t];
            t = t + 2;
        }

        SAuxGeomRenderFlags renderFlags(e_Def3DPublicRenderflags);
        g_pAuxGeom->SetRenderFlags(renderFlags);
        g_pAuxGeom->DrawLines(&arrExtSkinnedStream[0], numExtVertices * 2, &arrExtVColors[0]);
    }

    if (norm)
    {
        static std::vector<ColorB> arrExtVColors;
        uint32 csize = arrExtVColors.size();
        if (csize < (numExtVertices * 2))
        {
            arrExtVColors.resize(numExtVertices * 2);
        }
        for (uint32 i = 0; i < numExtVertices * 2; i = i + 2)
        {
            arrExtVColors[i + 0] = RGBA8(0x00, 0x00, 0x3f, 0x00);
            arrExtVColors[i + 1] = RGBA8(0x7f, 0x7f, 0xff, 0x00);
        }

        Matrix33 WMat33 = Matrix33(rRenderMat34);
        static std::vector<Vec3> arrExtSkinnedStream;
        uint32 vsize = arrExtSkinnedStream.size();
        if (vsize < (numExtVertices * 2))
        {
            arrExtSkinnedStream.resize(numExtVertices * 2);
        }
        for (uint32 i = 0, t = 0; i < numExtVertices; i++)
        {
            f32 flip = (g_arrQTangents[i].w < 0) ? -VLENGTH : VLENGTH;
            Vec3 vNormal = g_arrQTangents[i].GetColumn2() * flip;
            arrExtSkinnedStream[t + 0] = g_arrExtSkinnedStream[i];
            arrExtSkinnedStream[t + 1] = WMat33 * vNormal + arrExtSkinnedStream[t];
            t = t + 2;
        }
        SAuxGeomRenderFlags renderFlags(e_Def3DPublicRenderflags);
        g_pAuxGeom->SetRenderFlags(renderFlags);
        g_pAuxGeom->DrawLines(&arrExtSkinnedStream[0], numExtVertices * 2, &arrExtVColors[0]);
    }

    if (wire)
    {
        uint32 color = RGBA8(0x00, 0xff, 0x00, 0x00);
        SAuxGeomRenderFlags renderFlags(e_Def3DPublicRenderflags);
        renderFlags.SetFillMode(e_FillModeWireframe);
        renderFlags.SetDrawInFrontMode(e_DrawInFrontOn);
        renderFlags.SetAlphaBlendMode(e_AlphaAdditive);
        g_pAuxGeom->SetRenderFlags(renderFlags);
        g_pAuxGeom->DrawTriangles(&g_arrExtSkinnedStream[0], numExtVertices, pIndices, numExtIndices, color);
    }
}
#endif













extern f32 g_YLine;


struct STopology
{
    int iStartEdge, iEndEdge, iSorted;
    int bFullFan;

    STopology()
        : iStartEdge(0)
        , iEndEdge(0)
        , iSorted(0)
        , bFullFan(0) { }
};

struct SEdgeInfo
{
    int m_iEdge;
    float m_cost;
    bool m_skip;

    SEdgeInfo()
        : m_iEdge(-1)
        , m_cost(1e38f)
        , m_skip(false) { }
};

// copied form StatObjPhys.cpp
static inline int GetEdgeByBuddy(mesh_data* pmd, int itri, int itri_buddy)
{
    int iedge = 0, imask;
    imask = pmd->pTopology[itri].ibuddy[1] - itri_buddy;
    imask = (imask - 1) >> 31 ^ imask >> 31;
    iedge = 1 & imask;
    imask = pmd->pTopology[itri].ibuddy[2] - itri_buddy;
    imask = (imask - 1) >> 31 ^ imask >> 31;
    iedge = iedge & ~imask | 2 & imask;
    return iedge;
}

bool CClothSimulator::AddGeometry(phys_geometry* pgeom)
{
    if (!pgeom || pgeom->pGeom->GetType() != GEOM_TRIMESH)
    {
        return false;
    }

    mesh_data* pMesh = (mesh_data*)pgeom->pGeom->GetData();

    m_nVtx = pMesh->nVertices;
    m_particlesHot = new SParticleHot[m_nVtx];
    m_particlesCold = new SParticleCold[m_nVtx];
    for (int i = 0; i < m_nVtx; i++)
    {
        m_particlesHot[i].pos = Vector4(pMesh->pVertices[i]);
        m_particlesCold[i].prevPos = m_particlesHot[i].pos; // vel = 0
    }

    SEdgeInfo (*pInfo)[3] = new SEdgeInfo[pMesh->nTris][3];
    for (int i = 0; i < pMesh->nTris; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            int i1 = pMesh->pIndices[i * 3 + j];
            int i2 = pMesh->pIndices[i * 3 + inc_mod3[j]];
            const Vec3& v1 = pMesh->pVertices[i1];
            const Vec3& v2 = pMesh->pVertices[i2];
            // compute the cost of the edge (squareness of quad)
            int adjTri = pMesh->pTopology[i].ibuddy[j];
            if (adjTri >= 0)
            {
                float cost;
                int adjEdge = GetEdgeByBuddy(pMesh, adjTri, i);
                int i3 = pMesh->pIndices[i * 3 + dec_mod3[j]];
                const Vec3& v3 = pMesh->pVertices[i3];
                int i4 = pMesh->pIndices[adjTri * 3 + dec_mod3[adjEdge]];
                const Vec3& v4 = pMesh->pVertices[i4];
                cost = fabs((v1 - v2).len() - (v3 - v4).len()); // difference between diagonals
                // get the lowest value, although it should be the same
                if (pInfo[adjTri][adjEdge].m_cost < cost)
                {
                    cost = pInfo[adjTri][adjEdge].m_cost;
                }
                else
                {
                    pInfo[adjTri][adjEdge].m_cost = cost;
                }
                pInfo[i][j].m_cost = cost;
            }
        }
    }
    // count the number of edges - for each tri, mark each edge, unless buddy already did it
    m_nEdges = 0;
    for (int i = 0; i < pMesh->nTris; i++)
    {
        int bDegen = 0;
        float len[3];
        float minCost = 1e37f;
        int iedge = -1;
        for (int j = 0; j < 3; j++)
        {
            const Vec3& v1 = pMesh->pVertices[pMesh->pIndices[i * 3 + j]];
            const Vec3& v2 = pMesh->pVertices[pMesh->pIndices[i * 3 + inc_mod3[j]]];
            len[j] = (v1 - v2).len2();
            bDegen |= iszero(len[j]);
            int adjTri = pMesh->pTopology[i].ibuddy[j];
            if (adjTri >= 0)
            {
                int adjEdge = GetEdgeByBuddy(pMesh, adjTri, i);
                float cost = pInfo[i][j].m_cost;
                if (pInfo[adjTri][adjEdge].m_skip)
                {
                    cost = -1e36f;
                }
                bool otherSkipped = false;
                for (int k = 0; k < 3; k++)
                {
                    if (pInfo[adjTri][k].m_skip && k != adjEdge)
                    {
                        otherSkipped = true;
                        break;
                    }
                }
                if (cost < minCost && !otherSkipped)
                {
                    minCost = cost;
                    iedge = j;
                }
            }
        }

        int j = 0;
        if (iedge >= 0)
        {
            j = iedge & - bDegen;
            pInfo[i][iedge].m_skip = true;
        }
        do
        {
            if (pInfo[i][j].m_iEdge < 0 && !(j == iedge && !bDegen))
            {
                int adjTri = pMesh->pTopology[i].ibuddy[j];
                if (adjTri >= 0)
                {
                    int adjEdge = GetEdgeByBuddy(pMesh, adjTri, i);
                    pInfo[ adjTri ][ adjEdge ].m_iEdge = m_nEdges;
                }
                pInfo[i][j].m_iEdge = m_nEdges++;
            }
        } while (++j < 3 && !bDegen);
    }
    int num = ((m_nEdges / 2) + 1) * 2;
    m_links = new SLink[num];
    int* pVtxEdges = new int[m_nEdges * 2];
    for (int i = 0; i < pMesh->nTris; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            int iedge = pInfo[i][j].m_iEdge;
            if (iedge >= 0)
            {
                int i0 = m_links[iedge].i1 = pMesh->pIndices[i * 3 + j];
                int i1 = m_links[iedge].i2 = pMesh->pIndices[i * 3 + inc_mod3[j]];
                m_links[iedge].lenSqr = (pMesh->pVertices[i0] - pMesh->pVertices[i1]).len2();
            }
        }
    }
    if (m_nEdges & 1)
    {
        m_links[num - 1].skip = true;
    }

    // for each vertex, trace ccw fan around it and store in m_pVtxEdges
    STopology* pTopology = new STopology[m_nVtx];
    int nVtxEdges = 0;
    for (int i = 0; i < pMesh->nTris; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            int ivtx = pMesh->pIndices[i * 3 + j];
            if (!pTopology[ivtx].iSorted)
            {
                int itri = i;
                int iedge = j;
                pTopology[ivtx].iStartEdge = nVtxEdges;
                pTopology[ivtx].bFullFan = 1;
                int itrinew;
                do   // first, trace cw fan until we find an open edge (if any)
                {
                    itrinew = pMesh->pTopology[itri].ibuddy[iedge];
                    if (itrinew <= 0)
                    {
                        break;
                    }
                    iedge = inc_mod3[GetEdgeByBuddy(pMesh, itrinew, itri)];
                    itri = itrinew;
                } while (itri != i);
                int itri0 = itri;
                do   // now trace ccw fan
                {
                    CRY_ASSERT(itri < pMesh->nTris);
                    if (pInfo[itri][iedge].m_iEdge >= 0)
                    {
                        pVtxEdges[nVtxEdges++] = pInfo[itri][iedge].m_iEdge;
                    }
                    itrinew = pMesh->pTopology[itri].ibuddy[dec_mod3[iedge]];
                    if (itrinew < 0)
                    {
                        if (pInfo[itri][dec_mod3[iedge]].m_iEdge >= 0)
                        {
                            pVtxEdges[nVtxEdges++] = pInfo[itri][dec_mod3[iedge]].m_iEdge;
                        }
                        pTopology[ivtx].bFullFan = 0;
                        break;
                    }
                    iedge = GetEdgeByBuddy(pMesh, itrinew, itri);
                    itri = itrinew;
                } while (itri != itri0);
                pTopology[ivtx].iEndEdge = nVtxEdges - 1;
                pTopology[ivtx].iSorted = 1;
            }
        }
    }
    delete[] pInfo;

    // add shear and bending edges
    m_shearLinks.clear();
    m_bendLinks.clear();
    for (int i = 0; i < m_nEdges; i++)
    {
        int i1 = m_links[i].i1;
        int i2 = m_links[i].i2;
        // first point 1
        float maxLen = 0;
        int maxIdx = -1;
        bool maxAdded = false;
        for (int j = pTopology[i1].iStartEdge; j < pTopology[i1].iEndEdge + pTopology[i1].bFullFan; j++)
        {
            int i3 = m_links[pVtxEdges[j]].i2 + m_links[pVtxEdges[j]].i1 - i1;
            float len = (m_particlesHot[i3].pos - m_particlesHot[i2].pos).len2();
            bool valid = i3 != i1 && i2 < i3;
            if (len > maxLen)
            {
                maxLen = len;
                maxIdx = m_shearLinks.size();
                maxAdded = valid;
            }
            if (valid)
            {
                SLink link;
                link.i1 = i2;
                link.i2 = i3;
                link.lenSqr = len;
                m_shearLinks.push_back(link);
            }
        }
        // we make the assumption that the longest edge of all is a bending one
        if (maxIdx >= 0 && maxAdded && pTopology[i1].bFullFan)
        {
            m_bendLinks.push_back(m_shearLinks[maxIdx]);
            m_shearLinks.erase(m_shearLinks.begin() + maxIdx);
        }
        // then point 2
        maxLen = 0;
        maxIdx = -1;
        maxAdded = false;
        for (int j = pTopology[i2].iStartEdge; j < pTopology[i2].iEndEdge + pTopology[i2].bFullFan; j++)
        {
            int i3 = m_links[pVtxEdges[j]].i2 + m_links[pVtxEdges[j]].i1 - i2;
            float len = (m_particlesHot[i3].pos - m_particlesHot[i1].pos).len2();
            bool valid = i3 != i2 && i1 < i3;
            if (len > maxLen)
            {
                maxLen = len;
                maxIdx = m_shearLinks.size();
                maxAdded = valid;
            }
            if (valid)
            {
                SLink link;
                link.i1 = i1;
                link.i2 = i3;
                link.lenSqr = len;
                m_shearLinks.push_back(link);
            }
        }
        if (maxIdx >= 0 && maxAdded && pTopology[i2].bFullFan)
        {
            m_bendLinks.push_back(m_shearLinks[maxIdx]);
            m_shearLinks.erase(m_shearLinks.begin() + maxIdx);
        }
    }

    delete[] pTopology;
    delete[] pVtxEdges;

    // allocate contacts
    m_contacts = new SContact[m_nVtx];

    return true;
}

int CClothSimulator::SetParams(const SVClothParams& params, float* weights)
{
    m_config = params;
    m_config.weights = weights;

    for (int i = 0; i < m_nVtx && m_config.weights; i++)
    {
        m_particlesHot[i].alpha = m_config.weights[i];
        m_particlesCold[i].bAttached = m_particlesHot[i].alpha == 1.f;
    }
    // refresh the edges for newly attached vertices
    for (int i = 0; i < m_nEdges; i++)
    {
        PrepareEdge(m_links[i]);
    }
    for (size_t i = 0; i < m_shearLinks.size(); i++)
    {
        PrepareEdge(m_shearLinks[i]);
    }
    for (size_t i = 0; i < m_bendLinks.size(); i++)
    {
        PrepareEdge(m_bendLinks[i]);
    }
    return 1;
}

void CClothSimulator::SetSkinnedPositions(const Vector4* points)
{
    if (!points)
    {
        return;
    }
    for (int i = 0; i < m_nVtx; i++)
    {
        m_particlesCold[i].skinnedPos = points[i];
    }
}

void CClothSimulator::GetVertices(Vector4* pWorldCoords)
{
    // TODO: skip conversion, use Vector4 for skinning
    for (int i = 0; i < m_nVtx; i++)
    {
        pWorldCoords[i] = m_particlesHot[i].pos;
    }
}


void CClothSimulator::StartStep(float time_interval, const QuatT& location)
{
    m_time = time_interval;
    m_steps = 0;

    uint32 numClothProxie = 0;
    uint32 numProxies =  m_pAttachmentManager->GetProxyCount();
    for (uint32 i = 0; i < numProxies; i++)
    {
        if (m_pAttachmentManager->m_arrProxies[i].m_nPurpose == 1)
        {
            numClothProxie++;
        }
    }
    //  g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, ColorF(0,1,0,1), false,"numProxies: %d",numClothProxie),g_YLine+=16.0f;

    uint32 numCollidibles = m_permCollidables.size();
    if (numClothProxie != numCollidibles)
    {
        m_permCollidables.resize(numClothProxie);
    }

    Quat rotf = location.q;
    Quaternion ssequat(location.q);
    uint32 c = 0;
    for (uint32 i = 0; i < numProxies; i++)
    {
        if (m_pAttachmentManager->m_arrProxies[i].m_nPurpose != 1)
        {
            continue;
        }
        m_permCollidables[c].oldOffset = m_permCollidables[c].offset;
        m_permCollidables[c].oldR = m_permCollidables[c].R;

        const CProxy* pProxy = &m_pAttachmentManager->m_arrProxies[i];
        m_permCollidables[c].offset = ssequat * Vector4(pProxy->m_ProxyModelRelative.t);
        m_permCollidables[c].R  = Matrix3(rotf * pProxy->m_ProxyModelRelative.q);
        m_permCollidables[c].cr = pProxy->m_params.w;
        m_permCollidables[c].ca = pProxy->m_params.x;
        c++;
    }

    for (int i = 0; i < m_nVtx; i++)
    {
        m_particlesCold[i].oldPos = m_particlesHot[i].pos;
        int j = m_particlesCold[i].permIdx;
    }

    // blend world space with local space movement
    const float rotationBlend = m_config.rotationBlend; //max(m_angModulator, m_config.rotationBlend);
    if ((m_config.rotationBlend > 0.f || m_config.translationBlend > 0.f)  && !m_permCollidables.empty())
    {
        Quaternion dq(IDENTITY);
        if (m_config.rotationBlend > 0.f)
        {
            Quaternion qi(m_permCollidables[0].R);
            Quaternion qf(m_permCollidables[0].oldR);
            if (m_config.rotationBlend < 1.f)
            {
                qi.SetNlerp(qf, qi, m_config.rotationBlend);
            }
            dq = qi * !qf;
        }
        Vector4 delta(ZERO);
        if (m_config.translationBlend > 0.f && m_permCollidables.size())
        {
            delta = m_config.translationBlend * (m_permCollidables[0].offset - m_permCollidables[0].oldOffset);
            delta += m_permCollidables[0].oldOffset - dq * m_permCollidables[0].oldOffset;
        }
        for (int i = 0; i < m_nVtx; i++)
        {
            if (!m_particlesCold[i].bAttached)
            {
                m_particlesHot[i].pos = dq * m_particlesHot[i].pos + delta;
                m_particlesCold[i].prevPos = dq * m_particlesCold[i].prevPos + delta;
            }
        }
    }
}

struct SRay
{
    Vector4 origin, dir;
};

struct SIntersection
{
    Vector4 pt, n;
};


struct Quotient
{
    float x, y;

    Quotient(float ax, float ay)
        : x(ax)
        , y(ay) { }
    float val() { return y != 0 ? x / y : 0; }
    Quotient& operator +=(float op) { x += op * y; return *this; }
};

inline float getmin(float) { return 1E-20f; }
static ILINE bool operator<(const Quotient& op1, const Quotient& op2)
{
    return op1.x * op2.y - op2.x * op1.y + getmin(op1.x) * (op1.x - op2.x) < 0;
}

static ILINE bool PrimitiveRayCast(const f32 cr, const f32 ca, const SRay& ray, SIntersection& inters)
{
    if (ca == 0)
    {
        Vector4 delta = ray.origin;
        float a = ray.dir.len2();
        float b = ray.dir * delta;
        float c = delta.len2() - sqr(cr);
        float d = b * b - a * c;
        if (d >= 0)
        {
            d = sqrt_tpl(d);
            Quotient t(-b - d, a);
            int bHit = isneg(fabs_tpl(t.x * 2 - t.y) - t.y);
            t.x += d * (bHit ^ 1) * 2;
            bHit = isneg(fabs_tpl(t.x * 2 - t.y) - t.y);
            if (!bHit)
            {
                return 0;
            }

            inters.pt = ray.origin + ray.dir * t.val();
            inters.n = (inters.pt).normalized();
            //inters.iFeature[0][0]=inters.iFeature[1][0] = 0x40;
            //inters.iFeature[0][1]=inters.iFeature[1][1] = 0x20;
            return true;
        }
    }

    if (ca)
    {
        //lozenges
        Vector4 axis;
#ifdef CLOTH_SSE
        axis.x = 1, axis.y = 0, axis.z = 0, axis.w = 0;
#else
        axis.x = 1, axis.y = 0, axis.z = 0;
#endif

        Quotient tmin(1, 1);
        int iFeature = -1; // TODO: remove or change
        float a = ray.dir.len2();
        for (int icap = 0; icap < 2; icap++)
        {
            Vector4 vec0 = axis * (ca * (icap * 2 - 1));
            Vector4 delta = ray.origin - vec0;
            float b = ray.dir * delta;
            float c = delta.len2() - sqr(cr);
            float axcdiff = (ray.origin) * axis;
            float axdir = ray.dir * axis;
            float d = b * b - a * c;
            if (d >= 0)
            {
                d = sqrt_tpl(d);
                Quotient t(-b - d, a);
                int bHit = inrange(t.x * tmin.y, 0.f, t.y * tmin.x) & isneg(ca * t.y - fabs_tpl(axcdiff * t.y + axdir * t.x));
                tmin.x += (t.x - tmin.x) * bHit;
                tmin.y += (t.y - tmin.y) * bHit;
                iFeature = 0x41 + icap & - bHit | iFeature & ~-bHit;
                t.x += d * 2;
                bHit = inrange(t.x * tmin.y, 0.f, t.y * tmin.x) & isneg(ca * t.y - fabs_tpl(axcdiff * t.y + axdir * t.x));
                tmin.x += (t.x - tmin.x) * bHit;
                tmin.y += (t.y - tmin.y) * bHit;
                iFeature = 0x41 + icap & - bHit | iFeature & ~-bHit;
            }
        }

        Vector4 vec0 = ray.origin;
        Vector4 vec1 = ray.dir ^ axis;
        a = vec1 * vec1;
        float b = vec0 * vec1;
        float c = vec0 * vec0 - sqr(cr);
        float d = b * b - a * c;
        if (d >= 0)
        {
            d = sqrt_tpl(d);
            Quotient t(-b - d, a);
            int bHit = inrange(t.x * tmin.y, 0.f, t.y * tmin.x) & isneg(fabs_tpl(((ray.origin) * t.y + ray.dir * t.x) * axis) - ca * t.y);
            tmin.x += (t.x - tmin.x) * bHit;
            tmin.y += (t.y - tmin.y) * bHit;
            iFeature = 0x40 & - bHit | iFeature & ~-bHit;
            t.x += d * 2;
            bHit = inrange(t.x * tmin.y, 0.f, t.y * tmin.x) & isneg(fabs_tpl(((ray.origin) * t.y + ray.dir * t.x) * axis) - ca * t.y);
            tmin.x += (t.x - tmin.x) * bHit;
            tmin.y += (t.y - tmin.y) * bHit;
            iFeature = 0x40 & - bHit | iFeature & ~-bHit;
        }

        if (iFeature < 0)
        {
            return 0;
        }

        inters.pt = ray.origin + ray.dir * tmin.val();
        if (iFeature == 0x40)
        {
            inters.n = inters.pt;
            inters.n -= axis * (axis * inters.n);
        }
        else
        {
            inters.n = inters.pt - axis * (ca * ((iFeature - 0x41) * 2 - 1));
        }
        inters.n.normalize();
        //inters.iFeature[0][0]=inters.iFeature[1][0] = iFeature;
        //inters.iFeature[0][1]=inters.iFeature[1][1] = 0x20;
        return true;
    }

    return false;
}



void CClothSimulator::DetectCollisions()
{
    std::vector<SCollidable>& collidables = m_permCollidables;
    const float radius = m_config.thickness * m_config.tolerance;
    m_nContacts = 0;
    for (int i = 0; i < m_nVtx; i++)
    {
        if (m_particlesCold[i].bAttached)
        {
            continue;
        }
        SContact* pContact = m_particlesCold[i].pContact;
        int j = m_particlesCold[i].permIdx;
        m_particlesCold[i].pContact = NULL;
        if (pContact && j >= 0)
        {
            Vector4 currPos = (m_particlesHot[i].pos - collidables[j].offset) * collidables[j].R; // TODO: move out
            Vector4 prevPos = (m_particlesCold[i].oldPos - collidables[j].oldOffset) * collidables[j].oldR;
            Vector4 delta = currPos - prevPos;
            SRay ray;
            ray.origin = prevPos;
            ray.dir = delta;
            SIntersection inters;
            if (PrimitiveRayCast(collidables[j].cr, collidables[j].ca, ray, inters) && delta.normalized() * inters.n < 0.f)
            {
                Vector4 ptContact = collidables[j].R * (inters.pt + inters.n * m_config.thickness) + collidables[j].offset;
                AddContact(i, collidables[j].R * inters.n, ptContact, j);
                continue;
            }
        }

        // static collision
        for (size_t k = 0; k < m_permCollidables.size(); k++)
        {
            Vector4 ipos = (m_particlesHot[i].pos - collidables[k].offset) * collidables[k].R;
            SIntersection aContact;
            const f32 cr = collidables[k].cr;
            const f32 ca = collidables[k].ca;

            if (ca == 0)
            {
                Vector4 dc = ipos;
                if (dc.len2() > sqr(cr + radius))
                {
                    continue;
                }
                aContact.n = dc.normalized();
                aContact.pt = aContact.n * cr;
                AddContact(i, collidables[k].R * aContact.n, collidables[k].R * (aContact.pt + aContact.n * m_config.thickness) + collidables[k].offset, k);
                continue;
            }

            if (ca)
            {
                //      if (ipos.len2() > sqr(cr + ca + radius))
                //          continue;
                if (ipos.y * ipos.y + ipos.z * ipos.z > sqr(cr + radius))
                {
                    continue;
                }
                if (fabs_tpl(ipos.x) < ca)
                {
                    #ifdef CLOTH_SSE
                    aContact.n = Vector4(0, ipos.y, ipos.z, 0);
                    #else
                    aContact.n = Vector4(0, ipos.y, ipos.z);
                    #endif
                    aContact.n  = aContact.n.normalize();
                    aContact.pt = aContact.n * cr, aContact.pt.x += ipos.x;
                    AddContact(i, collidables[k].R * aContact.n, collidables[k].R * (aContact.pt + aContact.n * m_config.thickness) + collidables[k].offset, k);
                    continue;
                }
                f32 capx = ca * (ipos.x < 0 ? -1 : 1);
                Vector4 d = ipos;
                d.x -= capx;
                if (d.len2() > sqr(cr + radius))
                {
                    continue;
                }
                aContact.n  = d.normalized();
                aContact.pt = aContact.n * cr, aContact.pt.x += capx;
                AddContact(i, collidables[k].R * aContact.n, collidables[k].R * (aContact.pt + aContact.n * m_config.thickness) + collidables[k].offset, k);
                continue;
            }
        }
    }
}

static inline Matrix3 SkewSymmetric(Vector4 v)
{
    Matrix3 mat;
#ifdef CLOTH_SSE
    mat.row1.Set(0, -v.z, v.y);
    mat.row2.Set(v.z, 0, -v.x);
    mat.row3.Set(-v.y, v.x, 0);
#else
    mat.m00 = 0.f;
    mat.m01 = -v.z;
    mat.m02 = v.y;
    mat.m10 = v.z;
    mat.m11 = 0.f;
    mat.m12 = -v.x;
    mat.m20 = -v.y;
    mat.m21 = v.x;
    mat.m22 = 0.f;
#endif
    return mat;
}

// Implemented after the damping method presented in "Position Based Dynamics" by Mueller et al.
void CClothSimulator::RigidBodyDamping()
{
    if (!m_config.rigidDamping)
    {
        return;
    }

    Vector4 xcm(ZERO);
    int num = 0;
    for (int i = 0; i < m_nVtx; i++)
    {
        if (m_particlesCold[i].bAttached)
        {
            continue;
        }
        xcm += m_particlesHot[i].pos;
        num++;
    }
    if (num == 0)
    {
        CryLog("[Character cloth] All vertices are attached");
        return;
    }
    xcm *= 1.f / num;

    Vector4 vcm(ZERO);
    Vector4 angularMomentum(ZERO);
    Matrix3 inertiaTensor;
    inertiaTensor.SetZero();
    for (int i = 0; i < m_nVtx; i++)
    {
        if (m_particlesCold[i].bAttached)
        {
            continue;
        }
        Vector4 r = m_particlesHot[i].pos - xcm;
        Matrix3 rMat = SkewSymmetric(r);
        inertiaTensor -= rMat * rMat;
        Vector4 vel = (m_particlesHot[i].pos - m_particlesCold[i].prevPos);
        vcm += vel;
        angularMomentum += r ^ vel;
    }
    vcm *= 1.f / num;
    vcm += m_config.timeStep * m_config.timeStep * m_gravity;

    Vector4 omega(ZERO);
    float det = inertiaTensor.Determinant();
    if (fabs(det) == 0.f)
    {
        CryLog("[Character cloth] Singular matrix");
        return;
    }
    omega = inertiaTensor.GetInverted() * angularMomentum;

    for (int i = 0; i < m_nVtx; i++)
    {
        if (m_particlesCold[i].bAttached || m_particlesCold[i].pContact)
        {
            continue;
        }
        Vector4 r = m_particlesHot[i].pos - xcm;
        Vector4 v = vcm + (omega ^ r);
        Vector4 vel = (m_particlesHot[i].pos - m_particlesCold[i].prevPos);
        Vector4 dv = v - vel;
        v = vel + m_config.rigidDamping * dv;
        if (vel.z < 0.f)
        {
            v.z = vel.z;
        }
        m_particlesCold[i].prevPos = m_particlesHot[i].pos - v;
    }
}

int CClothSimulator::Step(float animBlend)
{
    if (m_nVtx <= 0)
    {
        return 1;
    }

    // always update attached vertices even if not stepping
    for (int i = 0; i < m_nVtx; i++)
    {
        if (m_particlesCold[i].bAttached)
        {
            m_particlesHot[i].pos = m_particlesCold[i].skinnedPos;
        }
    }
    if (m_time < m_config.timeStep * 0.1f)
    {
        return 1;
    }

    //g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, ColorF(0,1,0,1), false,"SimulationStep -    m_time: %f  m_config.timeStep: %f ",m_time, m_config.timeStep),g_YLine+=16.0f;

    RigidBodyDamping();

    // integrate positions
    const Vector4 dg = m_config.timeStep * m_config.timeStep * m_gravity;
    const float collDamping = m_config.collisionDamping;
    for (int i = 0; i < m_nVtx; i++)
    {
        Vector4 pos0 = m_particlesHot[i].pos;
        Vector4 dp = m_particlesHot[i].pos - m_particlesCold[i].prevPos;
        float vel = dp.len() / m_config.timeStep;
        if (m_particlesHot[i].timer > 0 && m_particlesHot[i].timer < m_config.collDampingRange)
        {
            dp *= collDamping;
        }
        dp *= m_config.dragDamping;
        m_particlesHot[i].pos += dp + dg;
        m_particlesCold[i].prevPos = pos0;
        m_particlesHot[i].timer++;
    }

    DetectCollisions();

    // pull towards skinned positions
    if (m_config.pullStiffness || animBlend || m_config.maxAnimDistance)
    {
        const float minDist = m_config.maxAnimDistance;
        const float domain = 2 * minDist;
        const Vector4 up(0, 0, 1);
        for (int i = 0; i < m_nVtx; i++)
        {
            if ((m_particlesHot[i].alpha == 0.f && animBlend == 0.f && m_config.maxAnimDistance == 0.f) || m_particlesCold[i].bAttached)
            {
                continue;
            }
            Vector4 target = m_particlesCold[i].skinnedPos;
            Vector4 delta = target - m_particlesHot[i].pos;
            float alpha = animBlend * m_config.maxBlendWeight;
            float len = delta.len();
            if (m_config.maxAnimDistance && len > minDist && delta * up < 0.f)
            {
                alpha = min(1.f, max(alpha, (len - minDist) / domain));
            }
            float stiffness = max(alpha, m_config.pullStiffness * m_particlesHot[i].alpha);
            m_particlesHot[i].pos += stiffness * delta;
        }
    }

    // shear and bending
    if (m_config.bendStiffness)
    {
        for (size_t i = 0; i < m_bendLinks.size(); i++)
        {
            SolveEdge(m_bendLinks[i], m_config.bendStiffness);
        }
    }
    if (m_config.shearStiffness)
    {
        for (size_t i = 0; i < m_shearLinks.size(); i++)
        {
            SolveEdge(m_shearLinks[i], m_config.shearStiffness);
        }
    }

    // solver
    const float ss = 1.0f - min(1.f - animBlend, m_config.stretchStiffness);
    float stretchStiffness = 1.0f;
    const bool halve = m_config.halfStretchIterations;
    const int nHalfEdges = m_nEdges / 2;
    for (int iter = 0; iter < m_config.numIterations; iter++)
    {
        // solve stretch edge constraints
        if (!halve || (iter & 1) == 0)
        {
            stretchStiffness *= ss;
            const float s = 1.0f - stretchStiffness;
            for (int i = 0; i < nHalfEdges; i++)
            {
                SolveEdge(m_links[i], s);
                SolveEdge(m_links[nHalfEdges + i], s);
            }
        }

        // solve contacts
        for (int i = 0; i < m_nContacts; i++)
        {
            int idx = m_contacts[i].particleIdx;
            const Vector4& n = m_contacts[i].n;
            float depth = (m_particlesHot[idx].pos - m_contacts[i].pt) * n;
            if (depth < 0)
            {
                float stiffness = max(animBlend * m_config.maxBlendWeight, m_config.pullStiffness * m_particlesHot[i].alpha); // TODO: try and take this out
                m_particlesHot[idx].pos -= (1.f - stiffness) * depth * n;
                m_particlesHot[idx].timer = 0;
            }
        }
    }


    // friction
    for (int i = 0; i < m_nContacts && m_config.friction > 0; i++)
    {
        const Vector4& n = m_contacts[i].n;
        int idx = m_contacts[i].particleIdx;
        Vector4 v = m_particlesHot[idx].pos - m_particlesCold[idx].prevPos;
        float vn = (v * n);
        Vector4 vt = v - vn * n;
        float push = m_gravity * n;
        if (push < 0 && vn < 0)
        {
            // if the particle is pressing on the body and moving towards it
            float vf = -m_config.friction * m_config.timeStep * push;
            if (vt.len2() > vf * vf)
            {
                m_particlesCold[idx].prevPos += vf * vt.normalized();
            }
            else
            {
                m_particlesCold[idx].prevPos += vt;
            }
        }
    }

    m_time -= m_config.timeStep;
    m_steps++;
    if (m_steps >= m_maxSteps)
    {
        // reset time if steps exceeded
        while (m_time > m_config.timeStep)
        {
            m_time -= m_config.timeStep;
        }
        return 1;
    }
    return 0;
}

void CClothSimulator::SwitchToBlendSimMesh()
{
    m_bBlendSimMesh = true;
}

bool CClothSimulator::IsBlendSimMeshOn()
{
    return m_bBlendSimMesh;
}

void CClothSimulator::DrawHelperInformation()
{
    IRenderAuxGeom* pRendererAux = gEnv->pRenderer->GetIRenderAuxGeom();
    //  Vec3 offs = m_pClothProxiesQQQ->m_Wtranslation;
    Vec3 offs = m_pAttachmentManager->m_pSkelInstance->m_location.t;

    ColorB color(128, 128, 255);
    for (int i = 0; i < m_nVtx; i++)
    {
        ColorB color1((int)(255 * m_particlesHot[i].alpha), (int)(128 * m_particlesHot[i].alpha), (int)(128 * m_particlesHot[i].alpha));
        pRendererAux->DrawSphere(SseToVec3(m_particlesHot[i].pos) + offs, m_config.thickness, m_particlesCold[i].bAttached ? color : color1);
    }
    for (int i = 0; i < m_nEdges; i++)
    {
        pRendererAux->DrawLine(SseToVec3(m_particlesHot[m_links[i].i1].pos) + offs, color, SseToVec3(m_particlesHot[m_links[i].i2].pos) + offs, color);
    }
}



struct VertexCommandClothSkin
    : public VertexCommand
{
public:
    VertexCommandClothSkin()
        : VertexCommand((VertexCommandFunction)Execute) { }

public:
    static void Execute(VertexCommandClothSkin& command, CVertexData& vertexData)
    {
        FUNCTION_PROFILER(GetISystem(), PROFILE_ANIMATION);
        //      if (!command.pClothPiece->m_bSingleThreaded)
        command.pClothPiece->UpdateSimulation(command.pTransformations, command.transformationCount);

        if (command.pVertexPositionsPrevious)
        {
            command.pClothPiece->SkinSimulationToRenderMesh<true>(command.pClothPiece->m_currentLod, vertexData, command.pVertexPositionsPrevious);
        }
        else
        {
            command.pClothPiece->SkinSimulationToRenderMesh<false>(command.pClothPiece->m_currentLod, vertexData, NULL);
        }
    }

public:
    const DualQuat* pTransformations;
    uint transformationCount;
    CClothPiece* pClothPiece;
    strided_pointer<const Vec3> pVertexPositionsPrevious;
};

bool CClothPiece::Initialize(const CAttachmentVCLOTH* pVClothAttachment)
{
    if (m_initialized)
    {
        return true;
    }

    if (pVClothAttachment == 0)
    {
        return false;
    }

    if (pVClothAttachment->GetType() != CA_VCLOTH)
    {
        return false;
    }

    if (!pVClothAttachment->m_pRenderSkin)
    {
        CryWarning(VALIDATOR_MODULE_ANIMATION, VALIDATOR_ERROR, "[Character cloth] Skin render attachment '%s' has no model mesh.", pVClothAttachment->GetName());
        return false;
    }

    m_pVClothAttachment = (CAttachmentVCLOTH*)pVClothAttachment;
    m_simulator.m_pAttachmentManager = pVClothAttachment->m_pAttachmentManager;

    CSkin* pSimSkin = m_pVClothAttachment->m_pSimSkin;
    CSkin* pRenderSkin = m_pVClothAttachment->m_pRenderSkin;

    if (pSimSkin == 0 || pSimSkin->GetIRenderMesh(0) == 0 || pSimSkin->GetIRenderMesh(0)->GetVerticesCount() == 0)
    {
        return false;
    }

    if (pRenderSkin == 0 || pRenderSkin->GetIRenderMesh(0) == 0 || pRenderSkin->GetIRenderMesh(0)->GetVerticesCount() == 0)
    {
        return false;
    }

    m_numLods = 0;
    _smart_ptr<IRenderMesh> pRenderMeshes[2];
    for (int lod = 0; lod < SClothGeometry::MAX_LODS; lod++)
    {
        pRenderMeshes[lod] = m_pVClothAttachment->m_pRenderSkin->GetIRenderMesh(lod);
        if (pRenderMeshes[lod])
        {
            m_numLods++;
        }
    }
    if (!m_numLods)
    {
        return false;
    }

    //  m_bSingleThreaded = context.bSingleThreaded;
    m_bAlwaysVisible = m_clothParams.isMainCharacter;

    CModelMesh* pSimModelMesh = pSimSkin->GetModelMesh(0);
    if (!pSimModelMesh)
    {
        return false;
    }
    pSimModelMesh->InitSWSkinBuffer();

    m_clothGeom = g_pCharacterManager->LoadVClothGeometry(*m_pVClothAttachment, pRenderMeshes);
    if (!m_clothGeom)
    {
        return false;
    }

    // init simulator
    m_simulator.AddGeometry(m_clothGeom->pPhysGeom);
    m_simulator.SetParams(m_clothParams, &m_clothGeom->weights[0]);
    //m_simulator.m_pAttachmentManager = pAttachmentManager;

    m_initialized = true;

    return true;
}

void CClothPiece::Dettach()
{
    if (!m_pVClothAttachment)
    {
        return;
    }

    WaitForJob(false);

    // make attachment GPU skinned
    uint32 flags = m_pVClothAttachment->GetFlags();
    flags &= ~FLAGS_ATTACH_SW_SKINNING;
    m_pVClothAttachment->SetFlags(flags);
    m_pVClothAttachment->m_vertexAnimation.SetClothData(NULL);
    m_pVClothAttachment = NULL;
}

void CClothPiece::WaitForJob(bool bPrev)
{
    // wait till the SW-Skinning job has finished
    int nFrameID = gEnv->pRenderer->EF_GetSkinningPoolID();
    if (bPrev)
    {
        nFrameID--;
    }
    int nList = nFrameID % 3;
    if (m_pVClothAttachment->m_arrSkinningRendererData[nList].nFrameID == nFrameID && m_pVClothAttachment->m_arrSkinningRendererData[nList].pSkinningData)
    {
        if (m_pVClothAttachment->m_arrSkinningRendererData[nList].pSkinningData->pAsyncDataJobExecutor)
        {
            m_pVClothAttachment->m_arrSkinningRendererData[nList].pSkinningData->pAsyncDataJobExecutor->WaitForCompletion();
        }
    }
}


bool CClothPiece::PrepareCloth(CSkeletonPose& skeletonPose, const Matrix34& worldMat, bool visible, int lod)
{
    FUNCTION_PROFILER(GetISystem(), PROFILE_ANIMATION);

    m_pCharInstance = skeletonPose.m_pInstance;
    if (m_lastVisible != visible)
    {
        m_hideCounter = 0; // trigger a phase change
    }
    m_lastVisible = visible;

    if (m_hideCounter < HIDE_INTERVAL) // if during phase change
    {
        m_hideCounter++;
        float f = (float)m_hideCounter / (float)HIDE_INTERVAL;
        f = 2.f * sqrt(f) - f; // nice super-linear function
        m_animBlend = max(m_animBlend, visible ? 1 - f : f);
        visible = true; // force it to be still visible
    }
    if (!visible) // no need to do the rest if falling back to GPU skinning
    {
        return false;
    }

    WaitForJob(true);

    // get working buffers from the pool
    if (m_buffers != NULL)
    {
        CryLog("[Character Cloth] the previous job is not done: %s - %s", m_pVClothAttachment->GetName(), skeletonPose.m_pInstance->GetFilePath());
        return false;
    }
    if (m_poolIdx < 0)
    {
        m_poolIdx = m_clothGeom->GetBuffers();
        if (m_poolIdx < 0)
        {
            return false;
        }
    }

    m_currentLod = min(lod, m_numLods - 1);
    m_charLocation = QuatT(worldMat); // TODO: direct conversion

    //  if (m_bSingleThreaded)
    //  {
    //      // software skin the sim mesh
    //      SSkinningData* pSkinningData = m_pSimAttachment->GetVertexTransformationData(true,lod);
    //      pSkinningData->pAsyncJobExecutor->WaitForCompletion(); // stall still the skinning related jobs have finished
    //      UpdateSimulation(pSkinningData->pBoneQuatsS, pSkinningData->nNumBones);
    //  }
    //  m_pRenderAttachment->m_vertexAnimation.SetClothData(this);

    return true;
}

void CClothPiece::DrawDebug(const SVertexAnimationJob* pVertexAnimation)
{
    // wait till the SW-Skinning jobs have finished
    while (*pVertexAnimation->pRenderMeshSyncVariable)
    {
        CrySleep(1);
    }
    m_simulator.DrawHelperInformation();
}

void CClothPiece::SetClothParams(const SVClothParams& params)
{
    m_clothParams = params;
}

const SVClothParams& CClothPiece::GetClothParams()
{
    return m_clothParams;
}

bool CClothPiece::CompileCommand(SVertexSkinData& skinData, CVertexCommandBuffer& commandBuffer)
{
    VertexCommandClothSkin* pCommand = commandBuffer.AddCommand<VertexCommandClothSkin>();
    if (!pCommand)
    {
        return false;
    }

    pCommand->pTransformations = skinData.pTransformations;
    pCommand->transformationCount = skinData.transformationCount;
    pCommand->pVertexPositionsPrevious = skinData.pVertexPositionsPrevious;
    pCommand->pClothPiece = this;
    return true;
}


struct SMemVec
{
    Vec3 v;
    float w;
} _ALIGN(16);

void CClothPiece::UpdateSimulation(const DualQuat* pTransformations, const uint transformationCount)
{
    FUNCTION_PROFILER(GetISystem(), PROFILE_ANIMATION);

    if (m_clothGeom->weldMap == NULL)
    {
        return;
    }
    if (m_poolIdx >= 0)
    {
        m_buffers = m_clothGeom->GetBufferPtr(m_poolIdx);
    }
    if (m_buffers == NULL)
    {
        return;
    }


    DynArray<Vec3>& arrDstPositions = m_buffers->m_arrDstPositions;
    DynArray<Vector4>& tmpClothVtx = m_buffers->m_tmpClothVtx;

    CVertexData vertexData;
    vertexData.m_vertexCount = m_clothGeom->nVtx;
    vertexData.pPositions.data = &arrDstPositions[0];
    vertexData.pPositions.iStride = sizeof(Vec3);
    vertexData.pTangents.data = &m_buffers->m_arrDstTangents[0];
    vertexData.pTangents.iStride = sizeof(SPipTangents);

    uint8 commandBufferData[256];
    CVertexCommandBufferAllocatorStatic commandBufferAllocator(
        commandBufferData, sizeof(commandBufferData));
    CVertexCommandBuffer commandBuffer;
    commandBuffer.Initialize(commandBufferAllocator);

    if (VertexCommandSkin* pCommand = commandBuffer.AddCommand<VertexCommandSkin>())
    {
        CModelMesh* pModelMesh = m_pVClothAttachment->m_pSimSkin->GetModelMesh(0);
        const CSoftwareMesh& geometry = pModelMesh->m_softwareMesh;

        pCommand->pTransformations = pTransformations;
        pCommand->pTransformationRemapTable = &m_pVClothAttachment->m_arrSimRemapTable[0];
        pCommand->transformationCount = transformationCount;
        pCommand->pVertexPositions = geometry.GetPositions();
        pCommand->pVertexQTangents = geometry.GetTangents();
        pCommand->pVertexTransformIndices = geometry.GetBlendIndices();
        pCommand->pVertexTransformWeights = geometry.GetBlendWeights();
        pCommand->vertexTransformCount = geometry.GetBlendCount();
    }

    commandBuffer.Process(vertexData);

    if (!Console::GetInst().ca_ClothBypassSimulation)
    {
        // transform the resulting vertices into physics space
        for (int i = 0; i < arrDstPositions.size(); i++)
        {
#ifdef CLOTH_SSE
            Vector4 v;
            v.Load((const float*)&arrDstPositions[i]);
#else
            const Vec3& v = arrDstPositions[i];
#endif
            Quaternion ssequat(m_charLocation.q);
            tmpClothVtx[m_clothGeom->weldMap[i]] = ssequat * v;
        }

        // send the target pose to the cloth simulator
        m_simulator.SetSkinnedPositions(&tmpClothVtx[0]);

        // step the cloth
        float dt = m_pCharInstance ? g_AverageFrameTime* m_pCharInstance->GetPlaybackScale() : g_AverageFrameTime;
        dt = dt ? dt : g_AverageFrameTime;

        m_simulator.StartStep(dt, m_charLocation);
        while (!m_simulator.Step(m_animBlend))
        {
            ;
        }

        // get the result back
        m_simulator.GetVertices(&tmpClothVtx[0]);

        for (int i = 0; i < m_clothGeom->nUniqueVtx; i++)
        {
            Quaternion ssequat(m_charLocation.q);
            tmpClothVtx[i] = tmpClothVtx[i] * ssequat;
        }
        m_animBlend = 0.f;
    }
    else
    {
        for (int i = 0; i < arrDstPositions.size(); i++)
        {
#ifdef CLOTH_SSE
            tmpClothVtx[m_clothGeom->weldMap[i]].Load((const float*)&arrDstPositions[i]);
#else
            tmpClothVtx[m_clothGeom->weldMap[i]] = arrDstPositions[i];
#endif
        }
    }
}

template<bool PREVIOUS_POSITIONS>
void CClothPiece::SkinSimulationToRenderMesh(int lod, CVertexData& vertexData, const strided_pointer<const Vec3>& pVertexPositionsPrevious)
{
    FUNCTION_PROFILER(GetISystem(), PROFILE_ANIMATION);
    if (m_clothGeom->skinMap[lod] == NULL || m_buffers == NULL)
    {
        return;
    }

    const int nVtx = vertexData.GetVertexCount();
    strided_pointer<Vec3> pVtx = vertexData.GetPositions();

    const DynArray<Vector4>& tmpClothVtx = m_buffers->m_tmpClothVtx;
    std::vector<Vector4>& normals = m_buffers->m_normals;
    std::vector<STangents>& tangents = m_buffers->m_tangents;

    // compute sim normals
    mesh_data* md = (mesh_data*)m_clothGeom->pPhysGeom->pGeom->GetData();
    for (int i = 0; i < md->nVertices; i++)
    {
        normals[i].zero();
    }
    for (int i = 0; i < md->nTris; i++)
    {
        int base = i * 3;
        const int idx1 = md->pIndices[base++];
        const int idx2 = md->pIndices[base++];
        const int idx3 = md->pIndices[base];
        const Vector4& p1 = tmpClothVtx[idx1];
        const Vector4& p2 = tmpClothVtx[idx2];
        const Vector4& p3 = tmpClothVtx[idx3];
        Vector4 n = (p2 - p1) ^ (p3 - p1);
        normals[idx1] += n;
        normals[idx2] += n;
        normals[idx3] += n;
    }
    for (int i = 0; i < md->nVertices; i++)
    {
        normals[i].normalize();
    }

    // set the positions
    SMemVec newPos;
    for (int i = 0; i < nVtx; i++)
    {
        tangents[i].n.zero();
        tangents[i].t.zero();

        if (m_clothGeom->skinMap[lod][i].iMap < 0)
        {
            continue;
        }
#ifdef CLOTH_SSE
        Vector4 p = SkinByTriangle(i, pVtx, lod);
        p.StoreAligned((float*)&newPos);
#else
        newPos.v = SkinByTriangle(i, pVtx, lod);
#endif
        if (PREVIOUS_POSITIONS)
        {
            vertexData.pVelocities[i] = pVertexPositionsPrevious[i] - newPos.v;
        }
        else
        {
            memset(&vertexData.pVelocities[i], 0, sizeof(Vec3));
        }
        pVtx[i] = newPos.v;
    }


    strided_pointer<SPipTangents> pTangents = vertexData.GetTangents();

    // rebuild tangent frame
    int nTris = m_clothGeom->numIndices[lod] / 3;
    vtx_idx* pIndices = m_clothGeom->pIndices[lod];
    for (int i = 0; i < nTris; i++)
    {
        int base = i * 3;
        int i1 = pIndices[base++];
        int i2 = pIndices[base++];
        int i3 = pIndices[base];

#ifdef CLOTH_SSE
        Vector4 v1;
        v1.Load((const float*)&pVtx[i1]);
        Vector4 v2;
        v2.Load((const float*)&pVtx[i2]);
        Vector4 v3;
        v3.Load((const float*)&pVtx[i3]);
#else
        const Vec3& v1 = pVtx[i1];
        const Vec3& v2 = pVtx[i2];
        const Vec3& v3 = pVtx[i3];
#endif

        Vector4 u = v2 - v1;
        Vector4 v = v3 - v1;

        const float t1 = m_clothGeom->tangentData[lod][i].t1;
        const float t2 = m_clothGeom->tangentData[lod][i].t2;
        const float r = m_clothGeom->tangentData[lod][i].r;
        Vector4 sdir = (u * t2 - v * t1) * r;

        tangents[i1].t += sdir;
        tangents[i2].t += sdir;
        tangents[i3].t += sdir;

        // compute area averaged normals
        Vector4 n = u ^ v;
        tangents[i1].n += n;
        tangents[i2].n += n;
        tangents[i3].n += n;
    }

#ifdef CLOTH_SSE
    Vector4 minusOne(0.f, 0.f, 0.f, -1.f);
    Vector4 maxShort(32767.f);
    _MS_ALIGN(16) Vec4sf tangentBitangent[2];
#endif
    for (int i = 0; i < nVtx; i++)
    {
        // Gram-Schmidt ortho-normalization
        const Vector4& t = tangents[i].t;
        const Vector4 n = tangents[i].n.normalized();
        Vector4 tan = (t - n * (n * t)).normalized();
        Vector4 biTan = tan ^ n;
#ifdef CLOTH_SSE
        tan.q = _mm_or_ps(tan.q, minusOne.q);
        biTan.q = _mm_or_ps(biTan.q, minusOne.q);

        __m128i tangenti = _mm_cvtps_epi32(_mm_mul_ps(tan.q, maxShort.q));
        __m128i bitangenti = _mm_cvtps_epi32(_mm_mul_ps(biTan.q, maxShort.q));

        __m128i compressed = _mm_packs_epi32(tangenti, bitangenti);
        _mm_store_si128((__m128i*)&tangentBitangent[0], compressed);

        pTangents[i] = SPipTangents(tangentBitangent[0], tangentBitangent[1]);
#else
        pTangents[i] = SPipTangents(tan, biTan, -1);
#endif
    }

    m_clothGeom->ReleaseBuffers(m_poolIdx);
    m_buffers = NULL;
    m_poolIdx = -1;
}

void SClothGeometry::AllocateBuffer()
{
    size_t poolSize = pool.size();
    uint32 maxChars = Console::GetInst().ca_ClothMaxChars;
    if (maxChars == 0 || poolSize < maxChars)
    {
        pool.resize(poolSize + 1);
        freePoolSlots.push_back(poolSize);
        SBuffers* buff = &pool.back();

        buff->m_arrDstPositions.resize(nVtx);
        buff->m_arrDstTangents.resize(nVtx);

        buff->m_tmpClothVtx.resize(nUniqueVtx + 1); // leave room for unaligned store
        buff->m_normals.resize(nUniqueVtx);

        buff->m_tangents.resize(maxVertices);
    }
}

SBuffers* SClothGeometry::GetBufferPtr(int idx)
{
    uint32 numSize = pool.size();
    if (idx >= numSize)
    {
        return nullptr;
    }
    std::list<SBuffers>::iterator it = pool.begin();
    std::advance(it, idx);

    return &(*it);
}

int SClothGeometry::GetBuffers()
{
    WriteLock lock(poolLock);
    if (freePoolSlots.size())
    {
        int idx = freePoolSlots.back();
        freePoolSlots.pop_back();
        return idx;
    }

    return -1;
}

void SClothGeometry::ReleaseBuffers(int idx)
{
    WriteLock lock(poolLock);
    freePoolSlots.push_back(idx);
}

void SClothGeometry::Cleanup()
{
    if (pPhysGeom)
    {
        g_pIPhysicalWorld->GetGeomManager()->UnregisterGeometry(pPhysGeom);
        pPhysGeom = NULL;
    }
    SAFE_DELETE_ARRAY(weldMap);
    SAFE_DELETE_ARRAY(weights);
    for (int i = 0; i < MAX_LODS; i++)
    {
        SAFE_DELETE_ARRAY(skinMap[i]);
        SAFE_DELETE_ARRAY(pIndices[i]);
        SAFE_DELETE_ARRAY(tangentData[i]);
    }
}

bool CompareDistances(SClothInfo* a, SClothInfo* b)
{
    return a->distance < b->distance;
}
