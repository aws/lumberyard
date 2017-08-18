/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution(the "License").All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file.Do not
* remove or modify any license notices.This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#pragma once

#include <PoolAllocator.h>
#include "GraphicsPipeline/Common/GraphicsPipelineStateSet.h"
#include "GraphicsPipeline/StandardGraphicsPipeline.h"

// Collection of pools used by CompiledObjects
class CRenderObjectsPools
{
public:
    CRenderObjectsPools();
    ~CRenderObjectsPools();
    AzRHI::ConstantBufferPtr AllocatePerInstanceConstantBuffer();
    void FreePerInstanceConstantBuffer(const AzRHI::ConstantBufferPtr& buffer);

public:
    // Global pool for compiled objects, overridden class new/delete operators use it.
    stl::TPoolAllocator<CCompiledRenderObject> m_compiledObjectsPool;

    // Pool used to allocate permanent render objects
    stl::TPoolAllocator<CRenderObjectImpl> m_permanentRenderObjectsPool;

private:
    // Used to pool per instance constant buffers.
    std::vector<AzRHI::ConstantBufferPtr> m_freeConstantBuffers;
};

// Used by Graphics Pass pipeline
class CCompiledRenderObject
{
public:
    struct SInstancingData
    {
        AzRHI::ConstantBufferPtr m_pConstBuffer;
        int m_nInstances;
    };
    struct SRootConstants
    {
        float dissolve;
        float dissolveOut;
    };
    struct SDrawParams
    {
        int32 m_nVerticesCount;
        int32 m_nStartIndex;
        int32 m_nNumIndices;

        SDrawParams()
            : m_nVerticesCount(0)
            , m_nStartIndex(0)
            , m_nNumIndices(0)
        {}
    };

public:
    // Used to link compiled render objects, within CRenderObject for each mesh chunk
    CCompiledRenderObject* m_pNext;

    // These 2 members must be initialized prior to calling Compile
    CRendElementBase*      m_pRenderElement;
    SShaderItem            m_shaderItem;
    //! see EBatchFlags, batch flags describe on what passes this object will be used (transparent,z-prepass,etc...)
    uint32                 m_nBatchFlags;

    /////////////////////////////////////////////////////////////////////////////
    // Packed parameters
    uint32 m_StencilRef        : 8; //!< Stencil ref value
    uint32 m_nMaxVertexStream  : 8; //!< Highest vertex stream index
    uint32 m_nRenderList       : 8; //!< Defines in which render list this compiled object belongs to, see ERenderListID
    uint32 m_nAfterWater       : 1; //!< Drawn before or after water
    uint32 m_bCompiled         : 1; //!< True if object compiled correctly
    uint32 m_bOwnPerInstanceCB : 1; //!< True if object owns its own per instance constant buffer, and is not sharing it with other compiled object
    /////////////////////////////////////////////////////////////////////////////

    uint32 m_nInstances;           //!< Number of instances.

    // From Material
    CDeviceResourceSetPtr   m_materialResourceSet;

    // Array of the PSOs for every ERenderableTechnique.
    // Sub array of PSO is defined inside the pass, and indexed by an integer passed inside the SGraphicsPiplinePassContext
    DevicePipelineStatesArray m_pso[RS_TECH_NUM];

    // Per instance constant buffer contain transformation matrix and other potentially not often changed data.
    AzRHI::ConstantBufferPtr m_perInstanceCB;

    // Per instance extra data
    CDeviceResourceSetPtr m_perInstanceExtraResources;

    // Skinning constant buffers, primary and previous
    AzRHI::ConstantBufferPtr m_skinningCB[2];

    std::vector<SInstancingData> m_InstancingCBs;

    // DrawCall parameters
    SDrawParams m_DIP;

    // Streams data.
    SStreamInfo m_indexStream;
    SStreamInfo m_vertexStream[VSF_NUM];

    // Optimized access to constants that can be directly bound to the root signature.
    SRootConstants m_rootConstants;

    //////////////////////////////////////////////////////////////////////////
public:
    CCompiledRenderObject()
        : m_pNext(nullptr)
        , m_StencilRef(0)
        , m_nMaxVertexStream(0)
        , m_nAfterWater(0)
        , m_bCompiled(false)
        , m_bOwnPerInstanceCB(false)
        , m_nInstances(0)
        , m_nRenderList(0)
    {}
    ~CCompiledRenderObject() { Release(); }

    void Release();

    //bool Compile(CRenderObject *pRenderObject, bool bInstanceDataUpdateOnly);
    bool Compile(CRenderObject* pRenderObject, float realTime);
    void CompilePerInstanceConstantBuffer(CRenderObject* pRenderObject, float realTime);
    void CompilePerInstanceExtraResources(CRenderObject* pRenderObject);
    void CompileInstancingData(CRenderObject* pRenderObject);

    void DrawAsync(CDeviceGraphicsCommandListRef RESTRICT_REFERENCE commandList, const SGraphicsPiplinePassContext& passContext) const;

public:
    static CCompiledRenderObject* AllocateFromPool()
    {
        return s_pPools->m_compiledObjectsPool.New();
    }
    static void FreeToPool(CCompiledRenderObject* ptr)
    {
        s_pPools->m_compiledObjectsPool.Delete(ptr);
    }
    static void SetStaticPools(CRenderObjectsPools* pools) { s_pPools = pools; }

private:
    static CRenderObjectsPools* s_pPools;
};
