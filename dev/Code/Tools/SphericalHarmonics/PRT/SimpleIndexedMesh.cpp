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

#include <PRT/SimpleIndexedMesh.h>
#include <PRT/SHFrameworkBasis.h>
#include <PRT/MaterialFactory.h>
#include <PRT/TransferParameters.h>

void CSimpleIndexedMesh::Log() const
{
    LogName();
    GetSHLog().Log("	faces: %d	    vertices: %d	    material count: %d\n", m_FaceCount, m_VertCount, m_MaterialCount);
}

void CSimpleIndexedMesh::LogName() const
{
    GetSHLog().Log("mesh %s\n", m_MeshFilename);
}

const bool CSimpleIndexedMesh::HasTransparentMaterials()
{
    for (int i = 0; i < m_MaterialCount; ++i)
    {
        const SAddMaterialProperty& crMatProp = m_Materials[i];
        if (crMatProp.considerForRayCasting && crMatProp.pSHMaterial->HasTransparencyTransfer())//is transparent and to consider for ray casting
        {
            return true;
        }
    }
    return false;
}

void CSimpleIndexedMesh::RetrieveFacedata(const uint32 cFaceIndex, Vec3& rV0, Vec3& rV1, Vec3& rV2) const
{
    assert((int32)cFaceIndex < m_FaceCount);
    const CObjFace& f = m_pFaces[cFaceIndex];
    //set vertices
    rV0.x = m_pVerts[f.v[0]].x;
    rV0.y = m_pVerts[f.v[0]].y;
    rV0.z = m_pVerts[f.v[0]].z;
    rV1.x = m_pVerts[f.v[1]].x;
    rV1.y = m_pVerts[f.v[1]].y;
    rV1.z = m_pVerts[f.v[1]].z;
    rV2.x = m_pVerts[f.v[2]].x;
    rV2.y = m_pVerts[f.v[2]].y;
    rV2.z = m_pVerts[f.v[2]].z;
}

void CSimpleIndexedMesh::RetrieveVertexDataBary(const CObjFace& rFace, const Vec3& rBary, Vec3& rV) const
{
    //set vertices
    rV.x = m_pVerts[rFace.v[0]].x * rBary.x +   m_pVerts[rFace.v[1]].x * rBary.y + m_pVerts[rFace.v[2]].x * rBary.z;
    rV.y = m_pVerts[rFace.v[0]].y * rBary.x +   m_pVerts[rFace.v[1]].y * rBary.y + m_pVerts[rFace.v[2]].y * rBary.z;
    rV.z = m_pVerts[rFace.v[0]].z * rBary.x +   m_pVerts[rFace.v[1]].z * rBary.y + m_pVerts[rFace.v[2]].z * rBary.z;
}

//! free data
void CSimpleIndexedMesh::FreeData()
{
    static CSHAllocator<Vec3> sAllocator;

    delete [] m_pFaces;
    delete [] m_pCoors;

    sAllocator.delete_mem_array(m_pVerts, sizeof(Vec3) * m_VertCount);
    sAllocator.delete_mem_array(m_pNorms, sizeof(Vec3) * m_NormCount);
    sAllocator.delete_mem_array(m_pWSNorms, sizeof(Vec3) * m_NormCount);
    sAllocator.delete_mem_array(m_pBiNorms, sizeof(Vec3) * m_NormCount);
    sAllocator.delete_mem_array(m_pTangentNorms, sizeof(Vec3) * m_NormCount);

    m_pFaces = NULL;
    m_pVerts = NULL;
    m_pCoors = NULL;
    m_pNorms = NULL;
    m_pWSNorms = NULL;
    m_pBiNorms = NULL;
    m_pTangentNorms = NULL;
    m_FaceCount = 0;
    m_VertCount = 0;
    m_CoorCount = 0;
    m_NormCount = 0;
    m_Min.x = m_Min.y = m_Min.z = 0;
    m_Max.x = m_Max.y = m_Max.z = 0;
}

void CSimpleIndexedMesh::AllocateNormals(const uint32 cCount)
{
    static CSHAllocator<Vec3> sAllocator;
    m_NormCount = cCount;
    assert(cCount);
    if (m_pWSNorms)
    {
        sAllocator.delete_mem_array(m_pWSNorms, sizeof(Vec3) * m_NormCount);
        m_pWSNorms = NULL;
    }
    if (m_pNorms)
    {
        sAllocator.delete_mem_array(m_pNorms, sizeof(Vec3) * m_NormCount);
        m_pNorms = NULL;
    }
    if (m_pBiNorms)
    {
        sAllocator.delete_mem_array(m_pBiNorms, sizeof(Vec3) * m_NormCount);
        m_pBiNorms = NULL;
    }
    if (m_pTangentNorms)
    {
        sAllocator.delete_mem_array(m_pTangentNorms, sizeof(Vec3) * m_NormCount);
        m_pTangentNorms = NULL;
    }
    m_pNorms                = (Vec3*)(sAllocator.new_mem_array(sizeof(Vec3) * m_NormCount));
    m_pWSNorms          = (Vec3*)(sAllocator.new_mem_array(sizeof(Vec3) * m_NormCount));
    m_pBiNorms          = (Vec3*)(sAllocator.new_mem_array(sizeof(Vec3) * m_NormCount));
    m_pTangentNorms = (Vec3*)(sAllocator.new_mem_array(sizeof(Vec3) * m_NormCount));

    assert(m_pWSNorms);
    assert(m_pTangentNorms);
    assert(m_pBiNorms);
    assert(m_pNorms);
}

#endif