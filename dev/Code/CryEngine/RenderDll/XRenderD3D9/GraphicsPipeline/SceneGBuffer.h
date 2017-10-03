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

#include "Common/GraphicsPipelinePass.h"
#include "Common/GraphicsPipelineStateSet.h"
#include "Common/FullscreenPass.h"

struct SGraphicsPipelineStateDescription;

class CSceneGBufferPass
    : public GraphicsPipelinePass
{
    enum EPerPassTexture
    {
        EPerPassTexture_TerrainBaseMap = 29,
        EPerPassTexture_NormalsFitting,
        EPerPassTexture_DissolveNoise,

        EPerPassTexture_Count,
        EPerPassTexture_First = EPerPassTexture_TerrainBaseMap,
    };

    static_assert(EPerPassTexture_Count <= 32, "Bind slot too high for platforms with only 32 shader slots");

    enum ESubPassId
    {
        eSubPassZPass = 0,
        eSubPassZPrePass = 1
    };

public:
    CSceneGBufferPass()
        : m_bValidResourceLayout(false)
    {}

    virtual ~CSceneGBufferPass() {}

    void Init() override;
    void Shutdown() override;
    void Prepare() override;
    void Reset() override;
    void Execute();

    virtual void PrepareCommandList(CDeviceGraphicsCommandListRef RESTRICT_REFERENCE commandList) const final;

    // Return number of created states, (uses fixed size array to avoid dynamic allocations).
    int CreatePipelineStates(DevicePipelineStatesArray& out, const SGraphicsPipelineStateDescription& stateDesc, CGraphicsPipelineStateLocalCache* pStateCache);

private:
    typedef std::array<CTexture*, 3> RenderTargetList;

    CDeviceGraphicsPSOPtr CreatePipelineState(const SGraphicsPipelineStateDescription& desc, int psoId = -1);

    void PreparePerPassResources();
    bool PrepareResourceLayout();

    void OnResolutionChanged();
    void RenderSceneOpaque();
    void RenderSceneOverlays();

    void ProcessRenderList(ERenderListID list, SGraphicsPiplinePassContext& passContext);

private:
    static uint64 GetPSOCacheKey(uint32 objFlags, int shaderID, int resID, bool bZPrePass);

    void DrawObject(CDeviceGraphicsCommandListPtr pCommandList, CRendElementBase* pRE, CShaderResources*& pPrevResources);
    void ProcessBatchesList(int nums, int nume, uint32 nBatchFilter, SGraphicsPiplinePassContext& passContext);
    void GetRenderTargets(RenderTargetList& targets, SDepthTexture& pDepthTarget) const;

private:
    CDeviceResourceSetPtr    m_pPerPassResources;
    CDeviceResourceLayoutPtr m_pResourceLayout;

    bool m_bValidResourceLayout;
};
