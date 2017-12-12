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

enum EShadowRender
{
    ESR_DIRECTIONAL              = 0,
    ESR_PointLight = ESR_DIRECTIONAL + 1,
    ESR_RSM              = ESR_PointLight + 1,
    ESR_Count            = ESR_RSM + 1,
    ESR_First            = ESR_DIRECTIONAL
};

class CShadowMapPass
    :  public GraphicsPipelinePass
{
public:
    virtual ~CShadowMapPass() {}

    void Init() override;
    void Shutdown() override;
    void Prepare() override;
    void Reset() override;

    void Execute();

    // Return number of created states, (uses fixed size array to avoid dynamic allocations).
    int CreatePipelineStates(DevicePipelineStatesArray& out, const SGraphicsPipelineStateDescription& stateDesc, CGraphicsPipelineStateLocalCache* pStateCache);

private:
    void Render();

    CDeviceGraphicsPSOPtr CreatePipelineState(const SGraphicsPipelineStateDescription& description, int psoId = -1);

    // TODO: remove
    void PreparePerPassConstantBuffer();

private:
    static uint64 GetPSOCacheKey(uint32 objFlags, int shaderID, int resID, bool bZPrePass);

    static void DrawBatch(CShader* pShader, SShaderPass* pPass, bool bObjSkinned, CDeviceGraphicsPSOPtr pPSO);
    static void FlushShader();

    typedef std::unordered_map<uint64, CDeviceGraphicsPSOPtr> PSOCache;
    static PSOCache  m_cachePSO;

private:
    AzRHI::ConstantBufferPtr m_pPerPassConstantBuffer;
    CDeviceResourceLayoutPtr m_pResourceLayout;
};
