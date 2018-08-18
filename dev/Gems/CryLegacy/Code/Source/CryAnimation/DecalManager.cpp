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

#include "CryEngineDecalInfo.h"
#include "DecalManager.h"
#include "Model.h"
#include "CharacterInstance.h"




// the array of transformed vertices (where 0,0,0 is the hit point, and 0,0,1 is the direction of the bullet)
// in other words, it's the Vertices in Bullet Coordinate System
std::vector<Vec3> g_arrVerticesBS;
// the mapping from the character vertices (in the external indexation) to the decal vertices
// -1 means the vertex has not yet been mapped
// It is always true that an element is >= -1 and < m_arrDecalVertices.size()
std::vector<int32> g_arrDecalVertexMapping;
std::vector<int32> g_arrDecalFaceMapping;



//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

size_t CAnimDecalManager::SizeOfThis ()
{
    size_t DecalInfoMem = sizeofVector(m_arrDecalRequests);
    size_t DecalsMem = 0;
    size_t numDecals = m_arrDecals.size();
    for (uint32 i = 0; i < numDecals; i++)
    {
        DecalsMem   +=  m_arrDecals[i].GetSize();
    }

    return DecalInfoMem + DecalsMem;
}

void CAnimDecalManager::GetMemoryUsage(ICrySizer* pSizer) const
{
    pSizer->AddObject(m_arrDecalRequests);
    pSizer->AddObject(m_arrDecals);
}

//----------------------------------------------------------------------------------
//-------     Spawn decal on the base-model and on all slave-model           -------
//----------------------------------------------------------------------------------
void CAnimDecalManager::CreateDecalsOnInstance(DualQuat* parrRemapSkinQuat, CDefaultSkeleton* pDefaultSkeleton, CryEngineDecalInfo& DecalLCS, const QuatTS& rLocation)
{
    FUNCTION_PROFILER(gEnv->pSystem, PROFILE_ANIMATION);

    // The decal info in the local coordinate system
    CryEngineDecalInfo DecalLocal = DecalLCS;
    DecalLocal.fSize *= Console::GetInst().ca_DecalSizeMultiplier * float(0.4 + (cry_random(0, 127)) * 0.001953125);
    DecalLocal.fAngle = (cry_random(0, 255)) * float(2 * gf_PI / 256.0f);
    m_arrDecalRequests.push_back(DecalLocal);

    OBB obb;
    /*
        uint32 count = pDefaultSkeleton->m_arrCollisions.size();
        for (uint32 i=0; i<count; i++)
        {
            int32 id    =   pDefaultSkeleton->m_arrCollisions[i].m_iBoneId;
            AABB aabb   =   pDefaultSkeleton->m_arrCollisions[i].m_aABB;
            if( !aabb.IsReset() )
            {
                const DualQuat& qd=parrRemapSkinQuat[id];
                QuatTS wjoint = rLocation * QuatTS(qd);

                QuatTS wjoint_scale=rLocation*QuatTS(parrRemapSkinQuat[id]);
                AABB scaled_aabb = aabb;
                scaled_aabb.min*=wjoint_scale.s;
                scaled_aabb.max*=wjoint_scale.s;

                obb.SetOBBfromAABB(wjoint.q, scaled_aabb);
                pDefaultSkeleton->m_arrCollisions[i].m_OBB = obb;
                //  g_pAuxGeom->DrawOBB(obb,wjoint.t,0,RGBA8(0xff,0xff,0xff,0xff),eBBD_Extremes_Color_Encoded);
            }
        }
    */

    // if there are decal requests, then converts them into the decal objects
    // reserves the vertex/updates the index buffers, if need to be
    // sets m_bNeedUpdateIndices to true if it has added something (and therefore refresh of indices is required)
    uint32 numDecalRequests = m_arrDecalRequests.size();
    if (numDecalRequests == 0)
    {
        CryFatalError("Decals Zero2");
    }

    IMaterialManager* pMatMan = g_pI3DEngine->GetMaterialManager();
    CModelMesh* pMesh = pDefaultSkeleton->GetModelMesh();
    if (pMesh == 0)
    {
        return;
    }
    IRenderMesh* pIRenderMesh = pDefaultSkeleton->GetIRenderMesh();
    if (pIRenderMesh == 0)
    {
        return;
    }

    uint32 numIntVertices   = pIRenderMesh->GetVerticesCount();
    uint32 numTriangles     = pIRenderMesh->GetIndicesCount() / 3;
    uint32 numVMappings = g_arrDecalVertexMapping.size();
    if (numVMappings < numIntVertices)
    {
        g_arrDecalVertexMapping.resize(numIntVertices);
    }

    uint32 numVerticesBCS = g_arrVerticesBS.size();
    if (numVerticesBCS < numIntVertices)
    {
        g_arrVerticesBS.resize(numIntVertices);
    }

    if (g_arrDecalFaceMapping.size() < numTriangles * 3)
    {
        g_arrDecalFaceMapping.resize(numTriangles * 3);
    }

    bool bFirstLock = true;

    // realize each unrealized decal, then clean up the array of unrealized decals
    for (uint32 nDecal = 0; nDecal < numDecalRequests; ++nDecal)
    {
        CryEngineDecalInfo& rDecalInfo = m_arrDecalRequests[nDecal];
        CAnimDecal NewDecal;
        bool bFirstMaterial = true;
        CRY_ASSERT((fabs_tpl(1 - (rDecalInfo.vHitDirection | rDecalInfo.vHitDirection))) < 0.0001);       //check if unit-vector

        // transform the bullet position to the local coordinates
        NewDecal.m_ptSource = rDecalInfo.vPos - rLocation.t;

        // Z axis looks in the direction of the hit
        Matrix34 m_matBullet34 = Matrix33::CreateRotationV0V1(Vec3(0, 0, -1), rDecalInfo.vHitDirection);
        m_matBullet34.SetTranslation(NewDecal.m_ptSource);

        //m_matBullet34 = Matrix33::CreaterRotationVDir(m_ptHitDirectionLCS, 0);
        //m_matBullet34.SetTranslation(m_ptSourceLCS);
        m_matBullet34.m02 *= 4.0f;
        m_matBullet34.m12 *= 4.0f;
        m_matBullet34.m22 *= 4.0f;

        Matrix34 m_matInvBullet34 = m_matBullet34.GetInverted();

        memset(&g_arrDecalVertexMapping[0], -1, g_arrDecalVertexMapping.size() * sizeof(int));
        memset(&g_arrDecalFaceMapping[0], -1, g_arrDecalFaceMapping.size() * sizeof(int));

        //Vec3 vec;

        /*  uint32 numCollisions = pDefaultSkeleton->m_arrCollisions.size();
            for (uint32 bb=0; bb<numCollisions; ++bb)
            {
                //inline uint8 Ray_OBB( const Ray& ray,const Vec3& pos,const OBB& obb, Vec3& output1 )
                if (Intersect::Ray_OBB(Ray(rDecalInfo.vPos, rDecalInfo.vHitDirection), pDefaultSkeleton->m_arrCollisions[bb].m_Pos, pDefaultSkeleton->m_arrCollisions[bb].m_OBB, vec) != 0)
                {
                    if (bFirstLock)
                    {
                        bFirstLock = false;
                    }

                    if (bFirstMaterial)
                    {
                        if (rDecalInfo.szMaterialName[0] != 0)
                        {
                            NewDecal.m_pMaterial =pMatMan->FindMaterial(rDecalInfo.szMaterialName);
                            if (!NewDecal.m_pMaterial)
                                NewDecal.m_pMaterial = pMatMan->LoadMaterial( rDecalInfo.szMaterialName, true, true );
                        }
                        else
                        {
                            _smart_ptr<IMaterial> pDefaultDecalMaterial( pMatMan->FindMaterial("Materials/Decals/CharacterDecal"));
                            if( !pDefaultDecalMaterial )
                            {
                                pDefaultDecalMaterial = pMatMan->LoadMaterial( "Materials/Decals/CharacterDecal", true, true );
                                if (!pDefaultDecalMaterial )
                                    pDefaultDecalMaterial = g_pI3DEngine->GetMaterialManager()->GetDefaultMaterial();
                            }

                            NewDecal.m_pMaterial = pDefaultDecalMaterial;
                        }
                    }

                    Matrix34 m34=m_matInvBullet34*Matrix33(rLocation.q);
                    Vec3 tmp, tmpn;

                    if(pMesh->m_arrSrcBoneIDs.size()==0)
                        CryFatalError("CryAnimation: arrExtBoneIDs not initialized");
                    pIRenderMesh->LockForThreadAccess();
                    uint32  numIndices      = pIRenderMesh->GetIndicesCount();
                    vtx_idx* pIndices   = pIRenderMesh->GetIndexPtr(FSL_READ);
                    if (pIndices==0)
                        return;
                    int32       nPositionStride;
                    uint8*  pPositions          = pIRenderMesh->GetPosPtr(nPositionStride, FSL_READ);
                    if (pPositions==0)
                        return;
                    int32       nQTangentStride;
                    uint8*  pQTangents          = pIRenderMesh->GetTangentPtr(nQTangentStride, FSL_READ);
                    if (pQTangents==0)
                        return;
                    int32       nSkinningStride;
                    uint8 * pSkinningInfo       = pIRenderMesh->GetHWSkinPtr(nSkinningStride, FSL_READ, 0, false); //pointer to weights and bone-id
                    if (pSkinningInfo==0)
                        return;
                    ++pMesh->m_iThreadMeshAccessCounter;

                    for (uint32 v=0, end = pDefaultSkeleton->m_arrCollisions[bb].m_arrIndexes.size(); v< end; v++)
                    {
                        uint32 ind = pDefaultSkeleton->m_arrCollisions[bb].m_arrIndexes[v];
                        if((ind*3+2) >= numIndices)
                        {
                            CryFatalError("Out of bounds index - %d of %d in %s", ind*3 + 2, numIndices, pDefaultSkeleton->GetModelFilePath());
                        }

                        {
                            int e = pIndices[ind*3+0];
                            //g_arrVerticesBS[i0] = m34* pMesh->GetDQSkinnedPosition(parrRemapSkinQuat, i0 );//arrSkinnedVertices[i].pos;
                            Vec3        hwPosition  = *(Vec3*)(pPositions + e*nPositionStride);
                            Vec4sf  hwQTangent  = *(Vec4sf*)(pQTangents + e*nQTangentStride);
                            ColorB  hwBoneIDs       = *(ColorB*)&((SVF_W4B_I4B*)(pSkinningInfo + e*nSkinningStride))->indices;
                            ColorB  hwWeights       = *(ColorB*)&((SVF_W4B_I4B*)(pSkinningInfo + e*nSkinningStride))->weights;
                            uint32 id0 = pMesh->m_arrSrcBoneIDs[e].idx0;
                            uint32 id1 = pMesh->m_arrSrcBoneIDs[e].idx1;
                            uint32 id2 = pMesh->m_arrSrcBoneIDs[e].idx2;
                            uint32 id3 = pMesh->m_arrSrcBoneIDs[e].idx3;
                            f32 w0 = hwWeights[0]/255.0f;
                            f32 w1 = hwWeights[1]/255.0f;
                            f32 w2 = hwWeights[2]/255.0f;
                            f32 w3 = hwWeights[3]/255.0f;
                            assert(fabsf((w0+w1+w2+w3)-1.0f)<0.0001f);
                            const DualQuat& q0=parrRemapSkinQuat[id0];
                            const DualQuat& q1=parrRemapSkinQuat[id1];
                            const DualQuat& q2=parrRemapSkinQuat[id2];
                            const DualQuat& q3=parrRemapSkinQuat[id3];
                            DualQuat wquat      =   (q0*w0 +  q1*w1 + q2*w2 +   q3*w3);
                            f32 l=1.0f/wquat.nq.GetLength();
                            wquat.nq*=l;
                            wquat.dq*=l;
                            g_arrVerticesBS[e] = m34*(wquat*hwPosition);
                        }

                        {
                            int e = pIndices[ind*3+1];
                            //g_arrVerticesBS[i0] = m34* pMesh->GetDQSkinnedPosition(parrRemapSkinQuat, i0 );//arrSkinnedVertices[i].pos;
                            Vec3        hwPosition  = *(Vec3*)(pPositions + e*nPositionStride);
                            Vec4sf  hwQTangent  = *(Vec4sf*)(pQTangents + e*nQTangentStride);
                            ColorB  hwBoneIDs       = *(ColorB*)&((SVF_W4B_I4B*)(pSkinningInfo + e*nSkinningStride))->indices;
                            ColorB  hwWeights       = *(ColorB*)&((SVF_W4B_I4B*)(pSkinningInfo + e*nSkinningStride))->weights;
                            uint32 id0 = pMesh->m_arrSrcBoneIDs[e].idx0;
                            uint32 id1 = pMesh->m_arrSrcBoneIDs[e].idx1;
                            uint32 id2 = pMesh->m_arrSrcBoneIDs[e].idx2;
                            uint32 id3 = pMesh->m_arrSrcBoneIDs[e].idx3;
                            f32 w0 = hwWeights[0]/255.0f;
                            f32 w1 = hwWeights[1]/255.0f;
                            f32 w2 = hwWeights[2]/255.0f;
                            f32 w3 = hwWeights[3]/255.0f;
                            assert(fabsf((w0+w1+w2+w3)-1.0f)<0.0001f);
                            const DualQuat& q0=parrRemapSkinQuat[id0];
                            const DualQuat& q1=parrRemapSkinQuat[id1];
                            const DualQuat& q2=parrRemapSkinQuat[id2];
                            const DualQuat& q3=parrRemapSkinQuat[id3];
                            DualQuat wquat      =   (q0*w0 +  q1*w1 + q2*w2 +   q3*w3);
                            f32 l=1.0f/wquat.nq.GetLength();
                            wquat.nq*=l;
                            wquat.dq*=l;
                            g_arrVerticesBS[e] = m34*(wquat*hwPosition);
                        }

                        {
                            int e = pIndices[ind*3+2];
                            //g_arrVerticesBS[i0] = m34* pMesh->GetDQSkinnedPosition(parrRemapSkinQuat, i0 );//arrSkinnedVertices[i].pos;
                            Vec3        hwPosition  = *(Vec3*)(pPositions + e*nPositionStride);
                            Vec4sf  hwQTangent  = *(Vec4sf*)(pQTangents + e*nQTangentStride);
                            ColorB  hwBoneIDs       = *(ColorB*)&((SVF_W4B_I4B*)(pSkinningInfo + e*nSkinningStride))->indices;
                            ColorB  hwWeights       = *(ColorB*)&((SVF_W4B_I4B*)(pSkinningInfo + e*nSkinningStride))->weights;
                            uint32 id0 = pMesh->m_arrSrcBoneIDs[e].idx0;
                            uint32 id1 = pMesh->m_arrSrcBoneIDs[e].idx1;
                            uint32 id2 = pMesh->m_arrSrcBoneIDs[e].idx2;
                            uint32 id3 = pMesh->m_arrSrcBoneIDs[e].idx3;
                            f32 w0 = hwWeights[0]/255.0f;
                            f32 w1 = hwWeights[1]/255.0f;
                            f32 w2 = hwWeights[2]/255.0f;
                            f32 w3 = hwWeights[3]/255.0f;
                            assert(fabsf((w0+w1+w2+w3)-1.0f)<0.0001f);
                            const DualQuat& q0=parrRemapSkinQuat[id0];
                            const DualQuat& q1=parrRemapSkinQuat[id1];
                            const DualQuat& q2=parrRemapSkinQuat[id2];
                            const DualQuat& q3=parrRemapSkinQuat[id3];
                            DualQuat wquat      =   (q0*w0 +  q1*w1 + q2*w2 +   q3*w3);
                            f32 l=1.0f/wquat.nq.GetLength();
                            wquat.nq*=l;
                            wquat.dq*=l;
                            g_arrVerticesBS[e] = m34*(wquat*hwPosition);
                        }

                    }

                    pIRenderMesh->UnLockForThreadAccess();
                    --pMesh->m_iThreadMeshAccessCounter;
                    if (pMesh->m_iThreadMeshAccessCounter==0)
                    {
                        pIRenderMesh->UnlockStream(VSF_GENERAL);
                        pIRenderMesh->UnlockStream(VSF_TANGENTS);
                        pIRenderMesh->UnlockStream(VSF_HWSKIN_INFO);
                    }

                    // find the vertices that participate in decal generation and add them to the array of vertices
                    //for (uint32 i=0; i<numIntVertices; i++)
                    //  g_arrDecalVertexMapping[i]=-1;


                    // FIXME: [CarstenW & MichaelR]: TexScale was always set to 0.030f regardless of fSize.
                    // Changed this to allow decals on large characters (on hunter the Overlap tests failed),
                    // without messing up appearance of currently used smaller ones (e.g. bullet decals)
                    f32 TexScale = (rDecalInfo.fSize < 0.25f) ? 0.030f : rDecalInfo.fSize;
                    //TexScale = 0.030f;

                    // find the faces to which the distance near enough
                    for (uint32 nFace=0, end = pDefaultSkeleton->m_arrCollisions[bb].m_arrIndexes.size() ; nFace<end; ++nFace)
                    {
                        // vertices belonging to the face, in internal indexation
                        TFace Face;// = arrFaces[nFace];
                        int ind = pDefaultSkeleton->m_arrCollisions[bb].m_arrIndexes[nFace];
                        if (g_arrDecalFaceMapping[ind] != -1)
                            continue;
                        //if (ind > g_arrDecalFaceMapping.size())
                        //{

                        //  g_arrDecalFaceMapping.resize(ind+1);
                        //}
                        g_arrDecalFaceMapping[ind] = 0;
                        Face.i0 = pIndices[ind*3+0];;
                        Face.i1 = pIndices[ind*3+1];;
                        Face.i2 = pIndices[ind*3+2];

                        Triangle tri;
                        tri.v0=g_arrVerticesBS[Face.i0];
                        tri.v1=g_arrVerticesBS[Face.i1];
                        tri.v2=g_arrVerticesBS[Face.i2];

                        //backface test
                        f32 b0 =  (tri.v1.x-tri.v0.x) * (tri.v2.y-tri.v0.y) - (tri.v2.x-tri.v0.x) * (tri.v1.y-tri.v0.y);
                        if (b0>0)
                        {
                            uint32 t = Overlap::Sphere_Triangle( Sphere(Vec3(ZERO),TexScale), tri );
                            if (t)
                            {
                                CDecalVertex DecalVertex;
                                f32 ts = 1.0f/TexScale;
                                uint32 i0,i1,i2;

                                if (g_arrDecalVertexMapping[Face.i0] != -1) {   i0=g_arrDecalVertexMapping[Face.i0]; }
                                else
                                {
                                    DecalVertex.nVertex = Face.i0;
                                    DecalVertex.u   = 0.5f+(g_arrVerticesBS[Face.i0].x*ts);
                                    DecalVertex.v   =   0.5f+(g_arrVerticesBS[Face.i0].y*ts);
                                    //DecalVertex.u = Clamp(DecalVertex.u, 0.0f, 1.0f);
                                    //DecalVertex.v = Clamp(DecalVertex.v, 0.0f, 1.0f);
                                    NewDecal.m_arrVertices.push_back(DecalVertex);
                                    g_arrDecalVertexMapping[Face.i0] = int(NewDecal.m_arrVertices.size()-1);
                                    i0=g_arrDecalVertexMapping[Face.i0];
                                }

                                if (g_arrDecalVertexMapping[Face.i1] != -1) { i1=g_arrDecalVertexMapping[Face.i1];  }
                                else
                                {
                                    DecalVertex.nVertex = Face.i1;
                                    DecalVertex.u   = 0.5f+(g_arrVerticesBS[Face.i1].x*ts);
                                    DecalVertex.v   =   0.5f+(g_arrVerticesBS[Face.i1].y*ts);
                                    //DecalVertex.u = Clamp(DecalVertex.u, 0.0f, 1.0f);
                                    //DecalVertex.v = Clamp(DecalVertex.v, 0.0f, 1.0f);

                                    NewDecal.m_arrVertices.push_back(DecalVertex);
                                    g_arrDecalVertexMapping[Face.i1] = int(NewDecal.m_arrVertices.size()-1);
                                    i1=g_arrDecalVertexMapping[Face.i1];
                                }

                                if (g_arrDecalVertexMapping[Face.i2] != -1) { i2=g_arrDecalVertexMapping[Face.i2];}
                                else
                                {
                                    DecalVertex.nVertex = Face.i2;
                                    DecalVertex.u   = 0.5f+(g_arrVerticesBS[Face.i2].x*ts);
                                    DecalVertex.v   =   0.5f+(g_arrVerticesBS[Face.i2].y*ts);
                                    //DecalVertex.u = Clamp(DecalVertex.u, 0.0f, 1.0f);
                                    //DecalVertex.v = Clamp(DecalVertex.v, 0.0f, 1.0f);

                                    NewDecal.m_arrVertices.push_back(DecalVertex);
                                    g_arrDecalVertexMapping[Face.i2] = int(NewDecal.m_arrVertices.size()-1);
                                    i2=g_arrDecalVertexMapping[Face.i2];
                                }

                                NewDecal.m_arrFaces.push_back( TFace(i0,i1,i2) );
                            }
                        }
                    }
                }
            }*/

        uint32 num = NewDecal.m_arrFaces.size();
        if (num == 0)
        {
            continue; // we're not interested in this decal: we don't have any decals
        }
        NewDecal.m_fBuildTime       = g_fCurrTime;
        NewDecal.m_fFadeInTime  = rDecalInfo.fSize / cry_random(0.04f, 0.06f); // suppose the speed is like this
        NewDecal.m_fDeathTime       = NewDecal.m_fBuildTime + rDecalInfo.fLifeTime; //remove deacal after 180 seconds
        NewDecal.m_fFadeOutTime = -1; // no fade out
        m_arrDecals.push_back(NewDecal);
    }


    // after we realized the decal request, we don't need it anymore
    m_arrDecalRequests.clear();
}



// starts fading out all decals that are close enough to the given point
// NOTE: the radius is m^2 - it's the square of the radius of the sphere
void CAnimDecalManager::fadeOutCloseDecals (const Vec3& ptCenter, float fRadius2)
{
    uint32 numDecals = m_arrDecals.size();
    for (uint32 i = 0; i < numDecals; ++i)
    {
        if ((m_arrDecals[i].m_ptSource - ptCenter).GetLengthSquared() < fRadius2)
        {
            m_arrDecals[i].startFadeOut (2);
        }
    }
}

// if we delete one of the decals
void CAnimDecalManager::RemoveObsoleteDecals()
{
    uint32 numDecals = m_arrDecals.size();
    for (uint32 i = 0; i < numDecals; i++)
    {
        if (m_arrDecals[i].isDead())
        {
            m_arrDecals[i] = m_arrDecals[numDecals - 1];
            m_arrDecals.pop_back();
            numDecals--;
        }
    }
}



// cleans up all decals, destroys the vertex buffer
void CAnimDecalManager::clear()
{
    m_arrDecalRequests.clear();
    m_arrDecals.clear();
}




// returns the decal multiplier: 0 - no decal, 1 - full decal size
f32 CAnimDecal::getIntensity() const
{
    if (g_fCurrTime >= m_fDeathTime)
    {
        // we've faded out
        return 0;
    }

    if (g_fCurrTime > m_fDeathTime - m_fFadeOutTime)
    {
        // we're fading out
        return 1 - sqr(1 - (m_fDeathTime - g_fCurrTime) / m_fFadeOutTime);
    }


    f32 fLifeTime = (g_fCurrTime - m_fBuildTime);
    if (fLifeTime > m_fFadeInTime)
    {
        // we've faded in
        return 1;
    }
    else
    {
        // we're fading in
        return 1 - sqr(1 - fLifeTime / m_fFadeInTime);
    }
}

// returns true if this is a dead/empty decal and should be discarded
bool CAnimDecal::isDead()
{
    return g_fCurrTime >= m_fDeathTime;
}

// starts fading out the decal from this moment on
void CAnimDecal::startFadeOut(f32 fTimeout)
{
    m_fFadeOutTime = fTimeout;
    m_fDeathTime = g_fCurrTime + fTimeout;
}

