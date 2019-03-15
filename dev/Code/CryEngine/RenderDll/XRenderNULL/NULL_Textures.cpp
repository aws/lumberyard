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

// Description : NULL device specific texture manager implementation.


#include "StdAfx.h"
#include "NULL_Renderer.h"

//=================================================================================

///////////////////////////////////////////////////////////////////////////////////


void CNULLRenderer::MakeSprite(IDynTexture*& rTexturePtr, float _fSpriteDistance, int nTexSize, float angle, float angle2, IStatObj* pStatObj, const float fBrightnessMultiplier, SRendParams& rParms)
{
    rTexturePtr = NULL;
}

int CNULLRenderer::GenerateAlphaGlowTexture(float k)
{
    return 0;
}

bool CNULLRenderer::EF_SetLightHole(Vec3 vPos, Vec3 vNormal, int idTex, float fScale, bool bAdditive)
{
    return false;
}

bool CNULLRenderer::EF_PrecacheResource(ITexture* pTP, float fDist, float fTimeToReady, int Flags, int nUpdateId, int nCounter)
{
    return false;
}

#if AZ_RENDER_TO_TEXTURE_GEM_ENABLED
bool CTexture::RenderToTexture(int handle, const CCamera& camera, AzRTT::RenderContextId contextId)
{
    return true;
}
#endif // if AZ_RENDER_TO_TEXTURE_GEM_ENABLED

bool CTexture::RenderEnvironmentCMHDR(int size, Vec3& Pos, TArray<unsigned short>& vecData)
{
    return true;
}

void CTexture::Apply(int nTUnit, int nState, int nTMatSlot, int nSUnit, SResourceView::KeyType nResViewKey, EHWShaderClass eSHClass)
{
}

#if defined(TEXTURE_GET_SYSTEM_COPY_SUPPORT)
byte* CTexture::Convert(const byte* pSrc, int nWidth, int nHeight, int nMips, ETEX_Format eTFSrc, ETEX_Format eTFDst, int& nOutSize, bool bLinear)
{
    return NULL;
}
#endif

void CTexture::ReleaseDeviceTexture(bool bKeepLastMips, bool bFromUnload)
{
}

bool CTexture::Clear(const ColorF& color)
{
    return true;
}

void CTexture::SetTexStates()
{
}

bool CTexture::CreateDeviceTexture(const byte* pData[6])
{
    return true;
}

void* CTexture::CreateDeviceResourceView(const SResourceView& rv)
{
    return NULL;
}

ETEX_Format CTexture::ClosestFormatSupported(ETEX_Format eTFDst)
{
    return eTFDst;
}

bool CTexture::SetFilterMode(int nFilter)
{
    return s_sDefState.SetFilterMode(nFilter);
}

bool CTexture::CreateRenderTarget(ETEX_Format eTF, const ColorF& cClear)
{
    return true;
}

bool CTexture::SetClampingMode(int nAddressU, int nAddressV, int nAddressW)
{
    return s_sDefState.SetClampMode(nAddressU, nAddressV, nAddressW);
}

void CTexture::UpdateTexStates()
{
}

void CTexture::GenerateCachedShadowMaps()
{
}

void CTexture::Readback(AZ::u32 subresourceIndex, StagingHook callback)
{
}

//======================================================================================

void SEnvTexture::Release()
{
}

void SEnvTexture::RT_SetMatrix(void)
{
}

bool SDynTexture::RestoreRT(int nRT, bool bPop)
{
    return true;
}

bool SDynTexture::ClearRT()
{
    return true;
}

bool SDynTexture2::ClearRT()
{
    return true;
}

bool SDynTexture::SetRT(int nRT, bool bPush, SDepthTexture* pDepthSurf, bool bScreenVP)
{
    return true;
}

bool SDynTexture2::SetRT(int nRT, bool bPush, SDepthTexture* pDepthSurf, bool bScreenVP)
{
    return true;
}

bool SDynTexture2::RestoreRT(int nRT, bool bPop)
{
    return true;
}

bool SDynTexture2::SetRectStates()
{
    return true;
}

//===============================================================================

void STexState::PostCreate()
{
}

void STexState::Destroy()
{
}

void STexState::Init(const STexState& src)
{
    memcpy(this, &src, sizeof(src));
}

void STexState::SetComparisonFilter(bool bEnable)
{
}

bool STexState::SetClampMode(int nAddressU, int nAddressV, int nAddressW)
{
    m_nAddressU = 0;
    m_nAddressV = 0;
    m_nAddressW = 0;
    return true;
}

bool STexState::SetFilterMode(int nFilter)
{
    m_nMinFilter = 0;
    m_nMagFilter = 0;
    m_nMipFilter = 0;
    return true;
}

void STexState::SetBorderColor(DWORD dwColor)
{
    m_dwBorderColor = dwColor;
}


SDepthTexture::~SDepthTexture()
{
}

void SDepthTexture::Release(bool bReleaseTex)
{
}

ETEX_Format CTexture::TexFormatFromDeviceFormat(int nFormat)
{
    return eTF_Unknown;
}

bool CTexture::RT_CreateDeviceTexture(const byte* pData[6])
{
    return true;
}

void CTexture::UpdateTextureRegion(const uint8_t* data, int X, int Y, int Z, int USize, int VSize, int ZSize, ETEX_Format eTFSrc)
{
}
void CTexture::RT_UpdateTextureRegion(const uint8_t* data, int X, int Y, int Z, int USize, int VSize, int ZSize, ETEX_Format eTFSrc)
{
}

bool SDynTexture::RT_SetRT(int nRT, int nWidth, int nHeight, bool bPush, bool bScreenVP)
{
    return true;
}

bool SDynTexture::RT_Update(int nNewWidth, int nNewHeight)
{
    return true;
}

void CTexture::ReleaseSystemTargets(void) {}
void CTexture::ReleaseMiscTargets(void) {}
void CTexture::CreateSystemTargets(void) {}

//===============================================================================

namespace  TextureHelpers
{
    bool VerifyTexSuffix(EEfResTextures texSlot, const char* texPath)
    {
        return false;
    }

    bool VerifyTexSuffix(EEfResTextures texSlot, const string& texPath)
    {
        return false;
    }

    const char* LookupTexSuffix(EEfResTextures texSlot)
    {
        return nullptr;
    }

    int8 LookupTexPriority(EEfResTextures texSlot)
    {
        return 0;
    }

    CTexture* LookupTexDefault(EEfResTextures texSlot)
    {
        return nullptr;
    }

    CTexture* LookupTexBlank(EEfResTextures texSlot)
    {
        return nullptr;
    }
}

bool CTexture::Clear() { return true; }

uint32 CDeviceTexture::TextureDataSize(uint32 nWidth, uint32 nHeight, uint32 nDepth, uint32 nMips, uint32 nSlices, const ETEX_Format eTF)
{
    return 0;
}