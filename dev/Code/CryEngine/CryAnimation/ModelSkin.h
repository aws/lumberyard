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
// Contains:
//     stores the loaded SKIN-model and materials

#ifndef CRYINCLUDE_CRYANIMATION_MODELSKIN_H
#define CRYINCLUDE_CRYANIMATION_MODELSKIN_H
#pragma once

#include "ModelMesh.h"                  //embedded

//////////////////////////////////////////////////////////////////////////////////////////////////////
// This class contain data which can be shared between different several SKIN-models of same type.
// It loads and contains all geometry and materials.
//////////////////////////////////////////////////////////////////////////////////////////////////////
class CSkin
    : public ISkin
{
public:
    explicit CSkin(const string& strFileName, uint32 nLoadingFlags);
    virtual ~CSkin();

    //--------------------------------------------------------------------------------
    //------                  Instance Management        -----------------------------
    //--------------------------------------------------------------------------------
    void SetInstanceCounter(uint32 numInstances) { m_nInstanceCounter = numInstances; }
    void AddRef()
    {
        CryInterlockedIncrement(&m_nRefCounter);
    }
    void Release();
    void DeleteIfNotReferenced()
    {
        if (m_nRefCounter <= 0)
        {
            delete this;
        }
    }
    int GetRefCounter() const
    {
        return m_nRefCounter;
    }

    virtual void SetKeepInMemory(bool nKiM) { m_nKeepInMemory = nKiM; }
    uint32 GetKeepInMemory() const {return m_nKeepInMemory; }

    virtual void PrecacheMesh(bool bFullUpdate, int nRoundId, int nLod);

    virtual const char* GetModelFilePath() const {  return m_strFilePath.c_str(); }
    virtual uint32 GetNumLODs() const { return m_arrModelMeshes.size(); }
    virtual IRenderMesh* GetIRenderMesh(uint32 nLOD) const;
    virtual uint32 GetTextureMemoryUsage2(ICrySizer* pSizer = 0) const;
    virtual uint32 GetMeshMemoryUsage(ICrySizer* pSizer = 0) const;
    virtual Vec3 GetRenderMeshOffset(uint32 nLOD) const
    {
        uint32 numMeshes = m_arrModelMeshes.size();
        if (nLOD >= numMeshes)
        {
            return Vec3(ZERO);
        }
        return m_arrModelMeshes[nLOD].m_vRenderMeshOffset;
    };

    int SelectNearestLoadedLOD(int nLod);




    CModelMesh* GetModelMesh(uint32 nLOD) const
    {
        uint32 numMeshes = m_arrModelMeshes.size();
        if (numMeshes <= nLOD)
        {
            return 0;
        }
        return (CModelMesh*)&m_arrModelMeshes[nLOD];
    }
    // this is the array that's returned from the RenderMesh
    TRenderChunkArray* GetRenderMeshMaterials()
    {
        if (m_arrModelMeshes[0].m_pIRenderMesh)
        {
            return &m_arrModelMeshes[0].m_pIRenderMesh->GetChunks();
        }
        else
        {
            return NULL;
        }
    }
    void GetMemoryUsage(ICrySizer* pSizer) const;
    uint32 SizeOfModelData() const;

    _smart_ptr<IMaterial> GetIMaterial(uint32 nLOD) const
    {
        uint32 numModelMeshes = m_arrModelMeshes.size();
        if (nLOD >= numModelMeshes)
        {
            return 0;
        }
        return m_arrModelMeshes[nLOD].m_pIDefaultMaterial;
    }

    bool LoadNewSKIN(const char* szFilePath, uint32 nLoadingFlags);

    virtual const IVertexFrames* GetVertexFrames() const { return &m_arrModelMeshes[0].m_softwareMesh.GetVertexFrames(); }
    bool HasSameSkeletonTopology(CSkin& comparedSkin);

public:
    // this struct contains the minimal information to attach a SKIN to a base-SKEL
    struct SJointInfo
    {
        int32       m_idxParent;               //index of parent-joint. if the idx==-1 then this joint is the root. Usually this values are > 0
        uint32  m_nJointCRC32Lower;  //hash value for searching joint-names
        QuatT   m_DefaultAbsolute;
        string  m_NameModelSkin;    // the name of joint
        void GetMemoryUsage(ICrySizer* pSizer) const  {};
    };

    DynArray<SJointInfo> m_arrModelJoints;
    DynArray<CModelMesh> m_arrModelMeshes;
    uint32 m_nLoadingFlags;

private:
    int m_nKeepInMemory;
    int m_nRefCounter;
    int m_nInstanceCounter;
    string m_strFilePath;
};



#endif // CRYINCLUDE_CRYANIMATION_MODELSKIN_H
