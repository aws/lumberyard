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


/*
	simple indexed mesh implementations
*/

inline SCoefficientExportPolicy::SCoefficientExportPolicy() : bytesPerComponent(4), coefficientsPerSet(9), compressionBytesPerComponent(4), swizzled(false), preMultiplied(false), compressed(false){}

/************************************************************************************************************************************************/

inline bool CObjCoor::operator==(CObjCoor& rOther)
{
	if(s == rOther.s)
		if(t == rOther.t)
			return true;
	return false;
}

inline CObjCoor::CObjCoor(const float cS, const float cT) : s(cS), t(cT){}
inline CObjCoor::CObjCoor() : s(0.f), t(0.f){}

/************************************************************************************************************************************************/

inline SAddMaterialProperty::SAddMaterialProperty
	() : pSHMaterial(NULL), considerForRayCasting(false), computeSHCoefficients(false), is2Sided(false)
{}

inline SAddMaterialProperty::SAddMaterialProperty
(
	NSH::CSmartPtr<NSH::NMaterial::ISHMaterial, CSHAllocator<> >& rSHMaterial,
	const bool cConsiderForRayCasting,
	const bool cComputeSHCoefficients,
	const bool cIs2Sided
) : pSHMaterial(rSHMaterial), considerForRayCasting(cConsiderForRayCasting), computeSHCoefficients(cComputeSHCoefficients), is2Sided(cIs2Sided){}

/************************************************************************************************************************************************/

inline CSimpleIndexedMesh::CSimpleIndexedMesh() : 
	m_pFaces(NULL), m_pVerts(NULL), m_pCoors(NULL), m_pNorms(NULL), m_pWSNorms(NULL), m_pBiNorms(NULL), m_pTangentNorms(NULL), m_FaceCount(0), m_VertCount(0), 
	m_CoorCount(0), m_NormCount(0), m_Min(0.,0.,0.), m_Max(0.,0.,0.), m_HasIdentityRotation(true), m_WorldToObjectSpaceSHMatrix(3 * 3/*3 bands supported for now*/),
	m_MaterialCount(0)
{
	m_WorldToObjectSpaceMatrix.SetIdentity();
	m_WorldToObjectSpaceSHMatrix.SetSHRotation(m_WorldToObjectSpaceMatrix);
}

inline CSimpleIndexedMesh::~CSimpleIndexedMesh()
{
	FreeData();
}

inline const char* CSimpleIndexedMesh::GetMeshName()const{return m_MeshFilename;}

inline void CSimpleIndexedMesh::SetMeshName(const char* cpName)
{
	const uint32 cStrLen = (uint32)strlen(cpName);
	assert(cStrLen < sizeof(m_MeshFilename)-1);
	memcpy(m_MeshFilename, cpName, cStrLen);
	m_MeshFilename[cStrLen] = '\0';
	if(strstr(&m_MeshFilename[cStrLen - 4], ".obj"))
		m_MeshFilename[cStrLen-4] = '\0';
}

inline const bool CSimpleIndexedMesh::ConsiderForRayCastingByFaceID(const uint32 cFaceID) const
{
	assert(cFaceID < (const uint32)m_FaceCount);
	const uint32 cMatIndex = m_pFaces[cFaceID].shaderID;
	assert(m_MaterialCount > cMatIndex);
	return m_Materials[cMatIndex].considerForRayCasting;
}

inline const bool CSimpleIndexedMesh::ComputeSHCoeffs(const uint32 cFaceID) const
{
	assert(cFaceID < (const uint32)m_FaceCount);
	const uint32 cMatIndex = m_pFaces[cFaceID].shaderID;
	assert(m_MaterialCount > cMatIndex);
	return m_Materials[cMatIndex].computeSHCoefficients;
}

inline const bool CSimpleIndexedMesh::Has2SidedMatByFaceID(const uint32 cFaceID) const
{
	assert(cFaceID < (const uint32)m_FaceCount);
	const uint32 cMatIndex = m_pFaces[cFaceID].shaderID;
	assert(m_MaterialCount > cMatIndex);
	return m_Materials[cMatIndex].is2Sided;
}

inline const NSH::NMaterial::ISHMaterial& CSimpleIndexedMesh::GetMaterialByFaceID(const uint32 cFaceID) const
{
	assert(cFaceID < (const uint32)m_FaceCount);
	const uint32 cMatIndex = m_pFaces[cFaceID].shaderID;
	assert(m_MaterialCount > cMatIndex);
	return *(m_Materials[cMatIndex].pSHMaterial);
}

inline const uint32 CSimpleIndexedMesh::GetMaterialIndexByFaceID(const uint32 cFaceID) const
{
	assert(cFaceID < (const uint32)m_FaceCount);
	const uint32 cMatIndex = m_pFaces[cFaceID].shaderID;
	assert(m_MaterialCount > cMatIndex);
	return cMatIndex;
}

inline const Matrix33& CSimpleIndexedMesh::GetWSOSRotation()const
{
	return m_WorldToObjectSpaceMatrix;
}

inline const bool CSimpleIndexedMesh::HasIdentityWSOSRotation()const
{
	return m_HasIdentityRotation;
}

inline void CSimpleIndexedMesh::SetWSOSRotation(const Matrix33& crMatrix)
{
	m_WorldToObjectSpaceMatrix = crMatrix;
	const bool cHadIdentityRotation = m_HasIdentityRotation;
	m_HasIdentityRotation = crMatrix.IsIdentity();
	if(!(cHadIdentityRotation && m_HasIdentityRotation))
		m_WorldToObjectSpaceSHMatrix.SetSHRotation(m_WorldToObjectSpaceMatrix);
}

inline const CSimpleIndexedMesh::TSHRotationMatrix& CSimpleIndexedMesh::GetSHWSOSRotation()const
{
	return m_WorldToObjectSpaceSHMatrix;
}

inline SCoefficientExportPolicy& CSimpleIndexedMesh::GetExportPolicy()
{
	return m_ExportPolicy;
}

inline const SCoefficientExportPolicy& CSimpleIndexedMesh::GetExportPolicy()const
{
	return m_ExportPolicy;
}

inline const CObjFace& CSimpleIndexedMesh::GetObjFace(const uint32 cIndex) const
{
	assert(cIndex < (uint32)m_FaceCount);
	return m_pFaces[cIndex];
}

inline CObjFace& CSimpleIndexedMesh::GetObjFace(const uint32 cIndex)
{
	assert(cIndex < (uint32)m_FaceCount);
	return m_pFaces[cIndex];
}

inline void CSimpleIndexedMesh::AllocateFaces(const uint32 cCount)
{
	m_FaceCount = cCount;
	assert(cCount);
	if(m_pFaces)
	{
		delete [] m_pFaces;
		m_pFaces = NULL;
	}
	m_pFaces = new CObjFace[m_FaceCount];
	assert(m_pFaces);
}

inline const uint32 CSimpleIndexedMesh::GetFaceCount()const
{
	return m_FaceCount;
}

inline void CSimpleIndexedMesh::AddMaterial(const SAddMaterialProperty& crMatToAdd)
{
	assert(m_MaterialCount < scMaxMats-1);
	m_Materials[m_MaterialCount++] = crMatToAdd;
}

inline const uint32 CSimpleIndexedMesh::MaterialCount()const
{
	return m_MaterialCount;
}

inline const bool CSimpleIndexedMesh::ComputeSHForMatFromIndex(const uint32 cIndex)
{
	assert(cIndex < m_MaterialCount);
	return m_Materials[cIndex].computeSHCoefficients;
}

inline const Vec3& CSimpleIndexedMesh::GetVertex(const uint32 cIndex) const
{
	assert(cIndex < (uint32)m_VertCount);
	return m_pVerts[cIndex];
}

inline Vec3& CSimpleIndexedMesh::GetVertex(const uint32 cIndex)
{
	assert(cIndex < (uint32)m_VertCount);
	return m_pVerts[cIndex];
}

inline void CSimpleIndexedMesh::AllocateVertices(const uint32 cCount)
{
	m_VertCount = cCount;
	assert(cCount);
	if(m_pVerts)
	{
		delete [] m_pVerts;
		m_pVerts = NULL;
	}
	static CSHAllocator<Vec3> sAllocator;
	m_pVerts = (Vec3*)sAllocator.new_mem_array(sizeof(Vec3) * m_VertCount);
	assert(m_pVerts);
}

inline const uint32 CSimpleIndexedMesh::GetVertexCount()const
{
	return m_VertCount;
}

inline const Vec3 CSimpleIndexedMesh::GetMinExt()const
{
	return m_Min;
}

inline void CSimpleIndexedMesh::SetMinExt(const Vec3& crMinExt)
{
	m_Min = crMinExt;
}
inline const Vec3 CSimpleIndexedMesh::GetMaxExt()const
{
	return m_Max;
}

inline void CSimpleIndexedMesh::SetMaxExt(const Vec3& crMaxExt)
{
	m_Max = crMaxExt;
}

inline const CObjCoor& CSimpleIndexedMesh::GetTexCoord(const uint32 cIndex) const
{
	assert(cIndex < (uint32)m_CoorCount);
	return m_pCoors[cIndex];
}

inline CObjCoor& CSimpleIndexedMesh::GetTexCoord(const uint32 cIndex)
{
	assert(cIndex < (uint32)m_CoorCount);
	return m_pCoors[cIndex];
}

inline void CSimpleIndexedMesh::AllocateTexCoords(const uint32 cCount)
{
	m_CoorCount = cCount;
	assert(cCount);
	if(m_pCoors)
	{
		delete [] m_pCoors;
		m_pCoors = NULL;
	}
	m_pCoors = new CObjCoor[m_CoorCount];
	assert(m_pCoors);
}

inline const uint32 CSimpleIndexedMesh::GetTexCoordCount()const
{
	return m_CoorCount;
}

inline const uint32 CSimpleIndexedMesh::GetNormalCount()const
{
	return m_NormCount;
}

inline const Vec3& CSimpleIndexedMesh::GetWSNormal(const uint32 cIndex) const
{
	assert(cIndex < (uint32)m_CoorCount);
	return m_pWSNorms[cIndex];
}

inline Vec3& CSimpleIndexedMesh::GetWSNormal(const uint32 cIndex)
{
	assert(cIndex < (uint32)m_CoorCount);
	return m_pWSNorms[cIndex];
}

inline const Vec3& CSimpleIndexedMesh::GetNormal(const uint32 cIndex) const
{
	assert(cIndex < (uint32)m_CoorCount);
	return m_pNorms[cIndex];
}

inline Vec3& CSimpleIndexedMesh::GetNormal(const uint32 cIndex)
{
	assert(cIndex < (uint32)m_CoorCount);
	return m_pNorms[cIndex];
}

inline const Vec3& CSimpleIndexedMesh::GetBiNormal(const uint32 cIndex) const
{
	assert(cIndex < (uint32)m_CoorCount);
	return m_pBiNorms[cIndex];
}

inline Vec3& CSimpleIndexedMesh::GetBiNormal(const uint32 cIndex)
{
	assert(cIndex < (uint32)m_CoorCount);
	return m_pBiNorms[cIndex];
}

inline const Vec3& CSimpleIndexedMesh::GetTNormal(const uint32 cIndex) const
{
	assert(cIndex < (uint32)m_CoorCount);
	return m_pTangentNorms[cIndex];
}

inline Vec3& CSimpleIndexedMesh::GetTNormal(const uint32 cIndex)
{
	assert(cIndex < (uint32)m_CoorCount);
	return m_pTangentNorms[cIndex];
}

