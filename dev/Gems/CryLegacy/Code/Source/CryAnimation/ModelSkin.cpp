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
#include "ModelSkin.h"
#include "CharacterManager.h"




CSkin::CSkin(const string& strFileName, uint32 nLoadingFlags)
{
    m_strFilePath = strFileName;
    m_nKeepInMemory = 0;
    m_nRefCounter = 0;
    m_nInstanceCounter = 0;
    m_nLoadingFlags = nLoadingFlags;
}

CSkin::~CSkin()
{
    for (int i = 0, c = m_arrModelMeshes.size(); i != c; ++i)
    {
        m_arrModelMeshes[i].AbortStream();
    }

    if (m_nInstanceCounter)
    {
        CryFatalError("The model '%s' still has %d skin-instances. Something went wrong with the ref-counting", m_strFilePath.c_str(), m_nInstanceCounter);
    }
    if (m_nRefCounter)
    {
        CryFatalError("The model '%s' has the value %d in the m_nRefCounter, while calling the destructor. Something went wrong with the ref-counting", m_strFilePath.c_str(), m_nRefCounter);
    }
    g_pCharacterManager->UnregisterModelSKIN(this);
}

void CSkin::Release()
{
    int refCount = CryInterlockedDecrement(&m_nRefCounter);

    if (m_nKeepInMemory)
    {
        return;
    }
    if (refCount <= 0)
    {
        // this check will make sure that nobody can hijack a smart - pointer or manipulate the ref - counter through the interface
        if (m_nInstanceCounter != m_nRefCounter)
        {
            CryFatalError("CryAnimation: m_nRefCounter and m_nInstanceCounter must be identical");
        }

        g_pCharacterManager->MarkForGC(this);
    }
}

//////////////////////////////////////////////////////////////////////////
uint32 CSkin::GetTextureMemoryUsage2(ICrySizer* pSizer) const
{
    uint32 nSize = 0;
    if (pSizer)
    {
        uint32 numModelmeshes = m_arrModelMeshes.size();
        for (uint32 i = 0; i < numModelmeshes; i++)
        {
            if (m_arrModelMeshes[i].m_pIRenderMesh)
            {
                nSize += (uint32)m_arrModelMeshes[i].m_pIRenderMesh->GetTextureMemoryUsage(m_arrModelMeshes[i].m_pIDefaultMaterial, pSizer);
            }
        }
    }
    else
    {
        if (m_arrModelMeshes[0].m_pIRenderMesh)
        {
            nSize = (uint32)m_arrModelMeshes[0].m_pIRenderMesh->GetTextureMemoryUsage(m_arrModelMeshes[0].m_pIDefaultMaterial);
        }
    }
    return nSize;
}

//////////////////////////////////////////////////////////////////////////
uint32 CSkin::GetMeshMemoryUsage(ICrySizer* pSizer) const
{
    uint32 nSize = 0;
    uint32 numModelMeshes = m_arrModelMeshes.size();
    for (uint32 i = 0; i < numModelMeshes; i++)
    {
        if (m_arrModelMeshes[i].m_pIRenderMesh)
        {
            nSize += m_arrModelMeshes[i].m_pIRenderMesh->GetMemoryUsage(0, IRenderMesh::MEM_USAGE_ONLY_STREAMS);
        }
    }
    return nSize;
}

//////////////////////////////////////////////////////////////////////////
int CSkin::SelectNearestLoadedLOD(int nLod)
{
    int nMinLod = 0;
    int nMaxLod = (int)m_arrModelMeshes.size() - 1;

    int nLodUp = nLod;
    int nLodDown = nLod;

    while (nLodUp >= nMinLod || nLodDown <= nMaxLod)
    {
        if (nLodUp >= nMinLod && m_arrModelMeshes[nLodUp].m_pIRenderMesh)
        {
            return nLodUp;
        }
        if (nLodDown <= nMaxLod && m_arrModelMeshes[nLodDown].m_pIRenderMesh)
        {
            return nLodDown;
        }
        --nLodUp;
        ++nLodDown;
    }

    return nLod;
}

//////////////////////////////////////////////////////////////////////////
void CSkin::PrecacheMesh(bool bFullUpdate, int nRoundId, int nLod)
{
    nLod = max(nLod, 0);
    nLod = min(nLod, (int)m_arrModelMeshes.size() - 1);

    int nZoneIdx = bFullUpdate ? 0 : 1;

    MeshStreamInfo& si = m_arrModelMeshes[nLod].m_stream;
    si.nRoundIds[nZoneIdx] = nRoundId;
}

//////////////////////////////////////////////////////////////////////////
IRenderMesh* CSkin::GetIRenderMesh(uint32 nLOD) const
{
    uint32 numModelMeshes = m_arrModelMeshes.size();
    if (nLOD >= numModelMeshes)
    {
        return 0;
    }
    return m_arrModelMeshes[nLOD].m_pIRenderMesh;
}




uint32 CSkin::SizeOfModelData() const
{
    uint32 nSize = sizeof(CSkin);

    uint32 numMeshes = m_arrModelMeshes.size();
    for (uint32 i = 0; i < numMeshes; ++i)
    {
        nSize += m_arrModelMeshes[i].SizeOfModelMesh();
    }

    nSize += m_arrModelJoints.get_alloc_size();

    uint32 numModelJoints = m_arrModelJoints.size();
    for (uint32 i = 0; i < numModelJoints; i++)
    {
        nSize += m_arrModelJoints[i].m_NameModelSkin.capacity();
    }

    return nSize;
}



void CSkin::GetMemoryUsage(ICrySizer* pSizer) const
{
    pSizer->AddObject(this, sizeof(*this));

    {
        SIZER_COMPONENT_NAME(pSizer, "CModelMesh");
        pSizer->AddObject(m_arrModelMeshes);
    }

    {
        SIZER_SUBCOMPONENT_NAME(pSizer, "SkinSkeleton");
        //pSizer->AddObject(m_poseData);
        pSizer->AddObject(m_arrModelJoints);

        uint32 numModelJoints = m_arrModelJoints.size();
        for (uint32 i = 0; i < numModelJoints; i++)
        {
            pSizer->AddObject(m_arrModelJoints[i].m_NameModelSkin);
        }
    }
}



