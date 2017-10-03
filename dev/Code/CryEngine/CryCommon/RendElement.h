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

#ifndef CRYINCLUDE_CRYCOMMON_RENDELEMENT_H
#define CRYINCLUDE_CRYCOMMON_RENDELEMENT_H
#pragma once


//=============================================================
#include <CryEngineAPI.h>

#include "VertexFormats.h"

class CRendElementBase;
struct CRenderChunk;
struct PrimitiveGroup;
class CShader;
struct SShaderTechnique;
class CParserBin;
struct SParserFrame;

namespace AZ
{
    namespace Vertex
    {
        class Format;
    }
}

enum EDataType
{
    eDATA_Unknown = 0,
    eDATA_Sky,
    eDATA_Beam,
    eDATA_ClientPoly,
    eDATA_Flare,
    eDATA_Terrain,
    eDATA_SkyZone,
    eDATA_Mesh,
    eDATA_Imposter,
    eDATA_LensOptics,
    eDATA_FarTreeSprites_Deprecated,
    eDATA_OcclusionQuery,
    eDATA_Particle,
    eDATA_GPUParticle,
    eDATA_PostProcess,
    eDATA_HDRProcess,
    eDATA_Cloud,
    eDATA_HDRSky,
    eDATA_FogVolume,
    eDATA_WaterVolume,
    eDATA_WaterOcean,
    eDATA_VolumeObject,
    eDATA_PrismObject,              // normally this would be #if !defined(EXCLUDE_DOCUMENTATION_PURPOSE) but we keep it to get consistent numbers for serialization
    eDATA_DeferredShading,
    eDATA_GameEffect,
    eDATA_BreakableGlass,
    eDATA_GeomCache,
    eDATA_Gem,
};

#include <Cry_Color.h>

//=======================================================

#define FCEF_TRANSFORM 1
#define FCEF_DIRTY     2
#define FCEF_NODEL     4
#define FCEF_DELETED   8

#define FCEF_MODIF_TC   0x10
#define FCEF_MODIF_VERT 0x20
#define FCEF_MODIF_COL  0x40
#define FCEF_MODIF_MASK 0xf0

#define FCEF_UPDATEALWAYS 0x100
#define FCEF_ALLOC_CUST_FLOAT_DATA 0x200
#define FCEF_MERGABLE    0x400

#define FCEF_SKINNED    0x800
#define FCEF_PRE_DRAW_DONE  0x1000

#define FGP_NOCALC 1
#define FGP_SRC    2
#define FGP_REAL   4
#define FGP_WAIT   8

#define FGP_STAGE_SHIFT 0x10

#define MAX_CUSTOM_TEX_BINDS_NUM 2

struct SGeometryInfo;
class CRendElement;
struct IRenderElement
{
    virtual int mfGetMatId() = 0;
    virtual uint16 mfGetFlags() = 0;
    virtual void mfSetFlags(uint16 fl) = 0;
    virtual void mfUpdateFlags(uint16 fl) = 0;
    virtual void mfClearFlags(uint16 fl) = 0;
    virtual void mfPrepare(bool bCheckOverflow) = 0;
    virtual void mfCenter(Vec3& centr, CRenderObject* pObj) = 0;
    virtual void mfGetBBox(Vec3& vMins, Vec3& vMaxs) = 0;
    virtual void mfReset() = 0;
    virtual void mfGetPlane(Plane& pl) = 0;
    virtual void mfExport(struct SShaderSerializeContext& SC) = 0;
    virtual void mfImport(struct SShaderSerializeContext& SC, uint32& offset) = 0;
    virtual void mfPrecache(const SShaderItem& SH) = 0;
    virtual bool mfIsHWSkinned() = 0;
    virtual bool mfCheckUpdate(int Flags, uint16 nFrame, bool bTessellation = false) = 0;
    virtual bool mfUpdate(int Flags, bool bTessellation = false) = 0;
    virtual bool mfCompile(CParserBin& Parser, SParserFrame& Frame) = 0;
    virtual bool mfDraw(CShader* ef, SShaderPass* sfm) = 0;
    virtual bool mfPreDraw(SShaderPass* sl) = 0;

    virtual CRendElementBase* mfCopyConstruct() = 0;
    virtual CRenderChunk* mfGetMatInfo() = 0;
    virtual TRenderChunkArray* mfGetMatInfoList() = 0;

    virtual void* mfGetPointer(ESrcPointer ePT, int* Stride, EParamType Type, ESrcPointer Dst, int Flags) = 0;

    virtual AZ::Vertex::Format GetVertexFormat() const = 0;
    //virtual bool GetGeometryInfo(SGeometryInfo& streams) = 0;
    virtual void Draw(CRenderObject* pObj, const struct SGraphicsPiplinePassContext& ctx) = 0;
};

class CRendElement
{
public:
    static CRendElement m_RootGlobal;
    static CRendElement m_RootRelease[];
    CRendElement* m_NextGlobal;
    CRendElement* m_PrevGlobal;

    EDataType m_Type;

protected:
    virtual void UnlinkGlobal()
    {
        if (!m_NextGlobal || !m_PrevGlobal)
        {
            return;
        }
        m_NextGlobal->m_PrevGlobal = m_PrevGlobal;
        m_PrevGlobal->m_NextGlobal = m_NextGlobal;
        m_NextGlobal = m_PrevGlobal = NULL;
    }

    virtual void LinkGlobal(CRendElement* Before)
    {
        if (m_NextGlobal || m_PrevGlobal)
        {
            return;
        }
        m_NextGlobal = Before->m_NextGlobal;
        Before->m_NextGlobal->m_PrevGlobal = this;
        Before->m_NextGlobal = this;
        m_PrevGlobal = Before;
    }

public:
    ENGINE_API CRendElement();
    ENGINE_API virtual ~CRendElement();
    ENGINE_API virtual void Release(bool bForce = false);
    ENGINE_API virtual const char* mfTypeString();

    virtual EDataType mfGetType() { return m_Type; }
    virtual void mfSetType(EDataType t) { m_Type = t; }

    virtual void GetMemoryUsage(ICrySizer* pSizer) const {}
    virtual int Size() { return 0; }


    static void ShutDown();
    static void Tick();
    static void Cleanup();
};

class CRendElementBase
    : public CRendElement
    , public IRenderElement
{
public:
    uint16 m_Flags;
    uint16 m_nFrameUpdated;

public:
    void* m_CustomData;
    int m_CustomTexBind[MAX_CUSTOM_TEX_BINDS_NUM];

    struct SGeometryStreamInfo
    {
        const void* pStream;
        int nOffset;
        int nStride;
    };
    struct SGeometryInfo
    {
        uint32        bonesRemapGUID; // Input paremeter to fetch correct skinning stream.

        int           primitiveType; //!< \see eRenderPrimitiveType
        AZ::Vertex::Format vertexFormat;
        uint32        streamMask;

        int32  nFirstIndex;
        int32  nNumIndices;
        uint32 nFirstVertex;
        uint32 nNumVertices;

        uint32 nMaxVertexStreams;

        SGeometryStreamInfo indexStream;
        SGeometryStreamInfo vertexStream[VSF_NUM];

        void* pTessellationAdjacencyBuffer;
        void* pSkinningExtraBonesBuffer;
    };

public:
    ENGINE_API CRendElementBase();
    ENGINE_API virtual ~CRendElementBase();

    ENGINE_API virtual void mfPrepare(bool bCheckOverflow) override; // False - mergable, True - static mesh
    ENGINE_API virtual CRenderChunk* mfGetMatInfo() override;
    ENGINE_API virtual TRenderChunkArray* mfGetMatInfoList() override;
    ENGINE_API virtual int mfGetMatId() override;
    ENGINE_API virtual void mfReset() override;
    ENGINE_API virtual CRendElementBase* mfCopyConstruct() override;
    ENGINE_API virtual void mfCenter(Vec3& centr, CRenderObject* pObj) override;

    ENGINE_API virtual bool mfCompile(CParserBin& Parser, SParserFrame& Frame) override { return false; }
    ENGINE_API virtual bool mfPreDraw(SShaderPass* sl) override { return true; }
    ENGINE_API virtual bool mfUpdate(int Flags, bool bTessellation = false) override { return true; }
    ENGINE_API virtual void mfPrecache(const SShaderItem& SH) override {}
    ENGINE_API virtual void mfExport(struct SShaderSerializeContext& SC) override { CryFatalError("mfExport has not been implemented for this render element type"); }
    ENGINE_API virtual void mfImport(struct SShaderSerializeContext& SC, uint32& offset) override { CryFatalError("mfImport has not been implemented for this render element type"); }

    ENGINE_API virtual void mfGetPlane(Plane& pl) override;
    ENGINE_API virtual bool mfDraw(CShader* ef, SShaderPass* sfm) override;
    ENGINE_API virtual void* mfGetPointer(ESrcPointer ePT, int* Stride, EParamType Type, ESrcPointer Dst, int Flags) override;

    virtual uint16 mfGetFlags()  override { return m_Flags; }
    virtual void mfSetFlags(uint16 fl)  override { m_Flags = fl; }
    virtual void mfUpdateFlags(uint16 fl)  override { m_Flags |= fl; }
    virtual void mfClearFlags(uint16 fl)  override { m_Flags &= ~fl; }
    virtual bool mfCheckUpdate(int Flags, uint16 nFrame, bool bTessellation = false) override
    {
        if (nFrame != m_nFrameUpdated || (m_Flags & (FCEF_DIRTY | FCEF_SKINNED | FCEF_UPDATEALWAYS)))
        {
            m_nFrameUpdated = nFrame;
            return mfUpdate(Flags, bTessellation);
        }
        return true;
    }
    virtual void mfGetBBox(Vec3& vMins, Vec3& vMaxs) override
    {
        vMins.Set(0, 0, 0);
        vMaxs.Set(0, 0, 0);
    }
    virtual bool mfIsHWSkinned() override { return false; }
    virtual int Size() override { return 0; }
    virtual void GetMemoryUsage(ICrySizer* pSizer) const  override {}
    virtual AZ::Vertex::Format GetVertexFormat() const override { return AZ::Vertex::Format(eVF_Unknown); };
    virtual bool GetGeometryInfo(SGeometryInfo& streams) { return false; }
    virtual void Draw(CRenderObject* pObj, const struct SGraphicsPiplinePassContext& ctx) override {};
};

#include "CREMesh.h"
#include "CRESky.h"
#include "CREOcclusionQuery.h"
#include "CREImposter.h"
#include "CREBaseCloud.h"
#include "CREPostProcess.h"
#include "CREFogVolume.h"
#include "CREWaterVolume.h"
#include "CREWaterOcean.h"
#include "CREVolumeObject.h"
#include "CREGameEffect.h"
#include "CREBreakableGlass.h"
#include "CREGeomCache.h"

#if !defined(EXCLUDE_DOCUMENTATION_PURPOSE)
#include "CREPrismObject.h"
#endif // EXCLUDE_DOCUMENTATION_PURPOSE

//==========================================================

#endif // CRYINCLUDE_CRYCOMMON_RENDELEMENT_H
