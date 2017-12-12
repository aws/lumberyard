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

#ifndef CRYINCLUDE_CRY3DENGINE_WATERWAVERENDERNODE_H
#define CRYINCLUDE_CRY3DENGINE_WATERWAVERENDERNODE_H
#pragma once

#include "VertexFormats.h"

struct SWaterWaveSerialize
{
    uint64 m_nID;
    _smart_ptr<IMaterial> m_pMaterial;

    Matrix34 m_pWorldTM;

    uint32 m_nVertexCount;
    std::vector< Vec3 > m_pVertices;

    f32 m_fUScale;
    f32 m_fVScale;

    SWaterWaveParams m_pParams;

    size_t GetMemoryUsage() const
    {
        size_t dynSize(sizeof(*this));
        dynSize += m_pVertices.capacity() * sizeof(std::vector< Vec3 >::value_type);
        return dynSize;
    }
};

class CWaterWaveRenderNode
    : public IWaterWaveRenderNode
    , public Cry3DEngineBase
{
public:
    CWaterWaveRenderNode();
    virtual void Create(uint64 nID, const Vec3* pVertices, uint32 nVertexCount, const Vec2& pUVScale, const Matrix34& pWorldTM);
    void Update(float fDistanceToCamera);

    virtual void SetParams(const SWaterWaveParams& pParams);
    virtual const SWaterWaveParams& GetParams() const;

    void SetRenderMesh(IRenderMesh* pRenderMesh);

    const SWaterWaveSerialize* GetSerializationParams() const
    {
        return m_pSerializeParams;
    }

    uint64 GetID() const
    {
        return m_nID;
    }

    f32 GetWaveKey() const
    {
        return m_fWaveKey;
    }

public:

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    // IRenderNode implementation

    virtual EERType GetRenderNodeType();
    virtual const char* GetEntityClassName() const;
    virtual const char* GetName() const;
    virtual Vec3 GetPos(bool bWorldOnly = true) const;
    virtual void Render(const SRendParams& rParam, const SRenderingPassInfo& passInfo);
    virtual IPhysicalEntity* GetPhysics() const;
    virtual void SetPhysics(IPhysicalEntity*);
    virtual void SetMaterial(_smart_ptr<IMaterial> pMat);
    virtual _smart_ptr<IMaterial> GetMaterial(Vec3* pHitPos);
    virtual _smart_ptr<IMaterial> GetMaterialOverride() { return m_pMaterial; }
    virtual float GetMaxViewDist();
    virtual void GetMemoryUsage(ICrySizer* pSizer) const;
    virtual void Precache();
    virtual const AABB GetBBox() const { return m_WSBBox; }
    virtual void SetBBox(const AABB& WSBBox) { m_WSBBox = WSBBox; }
    virtual void FillBBox(AABB& aabb);
    virtual void OffsetPosition(const Vec3& delta);

private:

    ~CWaterWaveRenderNode();

    // Get wave boundings
    void ComputeMinMax(const Vec3* pVertices, unsigned int nVertexCount);

    // Update bounding box
    void UpdateBoundingBox();

    // Copy serialization parameters
    void CopySerializationParams(uint64 nID, const Vec3* pVertices, unsigned int nVertexCount, const Vec2& pUVScale, const Matrix34& pWorldTM);

    // Spawn again
    void Spawn();

private:

    uint64 m_nID;
    f32 m_fWaveKey;

    // Render node serialization data
    SWaterWaveSerialize* m_pSerializeParams;

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    // Renderer data

    _smart_ptr< IMaterial > m_pMaterial;

    IRenderMesh* m_pRenderMesh;

    float m_fRECustomData[16]; // used for passing data to renderer

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    // Animation data

    SWaterWaveParams m_pParams;

    float m_fCurrTerrainDepth;

    Vec3 m_pOrigPos;
    Matrix34 m_pWorldTM;
    Vec3 m_pMin, m_pMax; // Wave volume bounding

    AABB m_WSBBox;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
// Wave manager

class CWaterWaveManager
    : public Cry3DEngineBase
{
public:
    CWaterWaveManager();
    ~CWaterWaveManager();

    // Updates all nearby and visible waves
    void Update(const SRenderingPassInfo& passInfo);

    // Free all resources
    void Release();

    // Adds wave to manager list, also creates/instances it's render mesh
    void Register(CWaterWaveRenderNode* pWave);

    // Removes from manager and if no more instances sharing render mesh, removes it
    void Unregister(CWaterWaveRenderNode* pWave);

private:

    // Make wave render mesh instance
    IRenderMesh* CreateRenderMeshInstance(CWaterWaveRenderNode* pWave);

    typedef std::map<uint64, CWaterWaveRenderNode*> GlobalWavesMap;
    typedef GlobalWavesMap::iterator GlobalWavesMapIt;

    GlobalWavesMap m_pWaves;

    typedef std::map<f32, IRenderMesh*> InstancedWavesMap;
    typedef InstancedWavesMap::iterator InstancedWavesMapIt;

    InstancedWavesMap m_pInstancedWaves;
};

#endif // CRYINCLUDE_CRY3DENGINE_WATERWAVERENDERNODE_H
