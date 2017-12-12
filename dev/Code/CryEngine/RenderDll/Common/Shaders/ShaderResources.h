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

#include "DeviceManager/Enums.h"
#include <CryEngineAPI.h>

//! This class provide all necessary resources to the shader extracted from material definition.
class CShaderResources
    : public IRenderShaderResources
    , public SBaseShaderResources
{
private:
    std::vector<Vec4> m_Constants;

public:
    SEfResTexture* m_Textures[EFTT_MAX];        // 48 bytes
    SDeformInfo* m_pDeformInfo;                 // 4 bytes
    TArray<struct SHRenderTarget*> m_RTargets;  // 4
    CCamera* m_pCamera;                         // 4
    SSkyInfo* m_pSky;                           // 4
    AzRHI::ConstantBuffer*  m_ConstantBuffer;
    uint16 m_Id;                                // 2 bytes
    uint16 m_IdGroup;                           // 2 bytes

    /////////////////////////////////////////////////////

    float m_fMinMipFactorLoad;
    int m_nRefCounter;
    int m_nLastTexture;
    int m_nFrameLoad;

    // Compiled resource set.
    // For DX12 will prepare list of textures in the global heap.
    std::shared_ptr<class CDeviceResourceSet>               m_pCompiledResourceSet;
    std::shared_ptr<class CGraphicsPipelineStateLocalCache> m_pipelineStateCache;

    uint8 m_nMtlLayerNoDrawFlags;

public:
    void AddTextureMap(int Id)
    {
        assert(Id >= 0 && Id < EFTT_MAX);
        m_Textures[Id] = new SEfResTexture;
    }
    int Size() const
    {
        int nSize = sizeof(CShaderResources);
        for (int i = 0; i < EFTT_MAX; i++)
        {
            if (m_Textures[i])
            {
                nSize += m_Textures[i]->Size();
            }
        }
        nSize += sizeofVector(m_Constants);
        nSize += m_RTargets.GetMemoryUsage();
        if (m_pDeformInfo)
        {
            nSize += m_pDeformInfo->Size();
        }
        return nSize;
    }

    void GetMemoryUsage(ICrySizer* pSizer) const final
    {
        pSizer->AddObject(this, sizeof(*this));

        for (int i = 0; i < EFTT_MAX; i++)
        {
            pSizer->AddObject(m_Textures[i]);
        }
        pSizer->AddObject(m_Constants);
        pSizer->AddObject(m_RTargets);
        pSizer->AddObject(m_pDeformInfo);
        SBaseShaderResources::GetMemoryUsage(pSizer);
    }

    CShaderResources();
    CShaderResources& operator=(const CShaderResources& src);
    CShaderResources(struct SInputShaderResources* pSrc);

    void PostLoad(CShader* pSH);
    void AdjustForSpec();
    void CreateModifiers(SInputShaderResources* pInRes);
    virtual void UpdateConstants(IShader* pSH) final;
    virtual void CloneConstants(const IRenderShaderResources* pSrc) final;
    virtual int GetResFlags() final { return m_ResFlags; }
    virtual void SetMaterialName(const char* szName) final { m_szMaterialName = szName; }
    virtual CCamera* GetCamera() final { return m_pCamera; }
    virtual void SetCamera(CCamera* pCam) final { m_pCamera = pCam; }
    virtual SSkyInfo* GetSkyInfo() final { return m_pSky; }
    virtual const float& GetAlphaRef() const final { return m_AlphaRef; }
    virtual void SetAlphaRef(float alphaRef) final { m_AlphaRef = alphaRef; }
    virtual SEfResTexture* GetTexture(int nSlot) const final { return m_Textures[nSlot]; }
    virtual DynArrayRef<SShaderParam>& GetParameters() final { return m_ShaderParams; }
    virtual ColorF GetFinalEmittance() final
    {
        const float kKiloScale = 1000.0f;
        return GetColorValue(EFTT_EMITTANCE) * GetStrengthValue(EFTT_EMITTANCE) * (kKiloScale / RENDERER_LIGHT_UNIT_SCALE);
    }
    virtual float GetVoxelCoverage() final { return ((float)m_VoxelCoverage) * (1.0f / 255.0f); }

    virtual void SetMtlLayerNoDrawFlags(uint8 nFlags) final { m_nMtlLayerNoDrawFlags = nFlags; }
    virtual uint8 GetMtlLayerNoDrawFlags() const final { return m_nMtlLayerNoDrawFlags; }

    void Rebuild(IShader* pSH, AzRHI::ConstantBufferUsage usage = AzRHI::ConstantBufferUsage::Dynamic);
    void ReleaseConstants();

    void Reset();

    bool IsDeforming() const
    {
        return (m_pDeformInfo && m_pDeformInfo->m_fDividerX != 0);
    }

    bool IsEmpty(int nTSlot) const
    {
        if (!m_Textures[nTSlot])
        {
            return true;
        }
        return false;
    }
    bool HasLMConstants() const { return (m_Constants.size() > 0); }
    virtual void SetInputLM(const CInputLightMaterial& lm) final;
    virtual void ToInputLM(CInputLightMaterial& lm) final;

    virtual ColorF GetColorValue(EEfResTextures slot) const final;
    ENGINE_API virtual float GetStrengthValue(EEfResTextures slot) const final;

    virtual void SetColorValue(EEfResTextures slot, const ColorF& color) final;
    virtual void SetStrengthValue(EEfResTextures slot, float value) final;

    ~CShaderResources();
    virtual void Release() final;
    virtual void AddRef() final { CryInterlockedIncrement(&m_nRefCounter); }
    virtual void ConvertToInputResource(SInputShaderResources* pDst) final;
    virtual CShaderResources* Clone() const final;
    virtual void SetShaderParams(SInputShaderResources* pDst, IShader* pSH) final;

    virtual size_t GetResourceMemoryUsage(ICrySizer* pSizer) final;

    void Cleanup();
};
typedef _smart_ptr<CShaderResources> CShaderResourcesPtr;
