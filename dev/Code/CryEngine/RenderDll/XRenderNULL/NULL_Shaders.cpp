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

#include "StdAfx.h"
#include "NULL_Renderer.h"
#include "I3DEngine.h"

//============================================================================

bool CShader::FXSetTechnique(const CCryNameTSCRC& szName)
{
    return true;
}

bool CShader::FXSetPSFloat(const CCryNameR& NameParam, const Vec4* fParams, int nParams)
{
    return true;
}

bool CShader::FXSetPSFloat(const char* NameParam, const Vec4* fParams, int nParams)
{
    return true;
}

bool CShader::FXSetVSFloat(const CCryNameR& NameParam, const Vec4* fParams, int nParams)
{
    return true;
}

bool CShader::FXSetVSFloat(const char* NameParam, const Vec4* fParams, int nParams)
{
    return true;
}

bool CShader::FXSetGSFloat(const CCryNameR& NameParam, const Vec4* fParams, int nParams)
{
    return true;
}

bool CShader::FXSetGSFloat(const char* NameParam, const Vec4* fParams, int nParams)
{
    return true;
}

bool CShader::FXSetCSFloat(const CCryNameR& NameParam, const Vec4* fParams, int nParams)
{
    return true;
}

bool CShader::FXSetCSFloat(const char* NameParam, const Vec4* fParams, int nParams)
{
    return true;
}
bool CShader::FXBegin(uint32* uiPassCount, uint32 nFlags)
{
    return true;
}

bool CShader::FXBeginPass(uint32 uiPass)
{
    return true;
}

bool CShader::FXEndPass()
{
    return true;
}

bool CShader::FXEnd()
{
    return true;
}

bool CShader::FXCommit(const uint32 nFlags)
{
    return true;
}

//===================================================================================

FXShaderCache CHWShader::m_ShaderCache;
FXShaderCacheNames CHWShader::m_ShaderCacheList;

void CRenderer::RefreshSystemShaders()
{
}

SShaderCache::~SShaderCache()
{
    CHWShader::m_ShaderCache.erase(m_Name);
    SAFE_DELETE(m_pRes[CACHE_USER]);
    SAFE_DELETE(m_pRes[CACHE_READONLY]);
}

SShaderCache* CHWShader::mfInitCache(const char* name, CHWShader* pSH, bool bCheckValid, uint32 CRC32, bool bReadOnly, bool bAsync)
{
    return NULL;
}

#if !defined(CONSOLE)
bool CHWShader::mfOptimiseCacheFile(SShaderCache* pCache, bool bForce, SOptimiseStats* Stats)
{
    return true;
}
#endif

bool CHWShader::PreactivateShaders()
{
    bool bRes = true;
    return bRes;
}
void CHWShader::RT_PreactivateShaders()
{
}

const char* CHWShader::GetCurrentShaderCombinations(bool bLevel)
{
    return "";
}

void CHWShader::mfFlushPendedShadersWait(int nMaxAllowed)
{
}

void CShaderResources::Rebuild(IShader* pSH, AzRHI::ConstantBufferUsage usage)
{
}

void CShaderResources::CloneConstants(const IRenderShaderResources* pSrc)
{
}

void CShaderResources::ReleaseConstants()
{
}

void CShaderResources::UpdateConstants(IShader* pSH)
{
}

void CShader::mfFlushPendedShaders()
{
}

void SShaderCache::Cleanup(void)
{
}

void CShaderResources::AdjustForSpec()
{
}

