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
#include "StatoscopeTextureStreaming.h"

#if ENABLE_STATOSCOPE

#include "TextureStreamPool.h"

#include "I3DEngine.h"

static void Normalize(char* p)
{
    for (; *p; ++p)
    {
        char c = *p;
        *p = c == '\\' ? '/' : tolower(c);
    }
}

void SStatoscopeTextureStreamingDG::Enable()
{
    CTexture::s_bStatsComputeStreamPoolWanted = true;
}

void SStatoscopeTextureStreamingDG::Disable()
{
    CTexture::s_bStatsComputeStreamPoolWanted = false;
}

IStatoscopeDataGroup::SDescription SStatoscopeTextureStreamingDG::GetDescription() const
{
    return SDescription(
        'x',
        "streaming textures",
        "['/StreamingTextures/' "
        "(float bandwidthActualKBsecond) "
        "(float bandwidthRequestedKBsecond) "
        "(float numUpdatedPerSecond) "
        "(float numRequestedPerSecond) "
        "(float numRenderedPerSecond) "
        "(int numRequestedJobs) "
        "(float amtRequestedJobsMB) "
        "(float streamPoolMemReservedMB) "
        "(float streamPoolMemInUseMB) "
        "(float streamPoolMemBoundMB) "
        "(int streamPoolSoftCreates) "
        "(int streamPoolSoftFrees) "
        "(int streamPoolHardCreates) "
        "(int streamPoolHardFrees) "
        "(float bias) "
        "(int streamAllocFails) "
        "(float poolMemUsedMB) "
        "(float poolMemWantedMB)]");
}

void SStatoscopeTextureStreamingDG::Write(IStatoscopeFrameRecord& fr)
{
    I3DEngine::SStremaingBandwidthData subsystemStreamingData;
    gEnv->p3DEngine->GetStreamingSubsystemData(eStreamTaskTypeTexture, subsystemStreamingData);

    CTextureStreamPoolMgr::SFrameStats poolStats;

#if !defined(_RELEASE)
    poolStats = CTexture::s_pPoolMgr->FetchFrameStats();
#endif

    float fInUseMem = CTexture::s_pPoolMgr->GetInUseSize() / (1024.0f * 1024.0f);

    fr.AddValue(subsystemStreamingData.fBandwidthActual);
    fr.AddValue(subsystemStreamingData.fBandwidthRequested);
    fr.AddValue(GetTextureUpdates());
    fr.AddValue(GetTextureRequests());
    fr.AddValue(GetTextureRenders());
    fr.AddValue((int)CTexture::s_nMipsSubmittedToStreaming.val);
    fr.AddValue(CTexture::s_nBytesSubmittedToStreaming.val / (1024.0f * 1024.0f));
    fr.AddValue(CTexture::s_pPoolMgr->GetReservedSize() / (1024.0f * 1024.0f));
    fr.AddValue(fInUseMem);
    fr.AddValue(CTexture::s_nStatsStreamPoolBoundMem / (1024.0f * 1024.0f));
    fr.AddValue((int)poolStats.nSoftCreates);
    fr.AddValue((int)poolStats.nSoftFrees);
    fr.AddValue((int)poolStats.nHardCreates);
    fr.AddValue((int)poolStats.nHardFrees);
    fr.AddValue(CTexture::s_pTextureStreamer->GetBias());
    fr.AddValue(CTexture::s_nStatsAllocFails);
    fr.AddValue(GetTexturePoolUsage());
    fr.AddValue(GetTexturePoolWanted());
}

float SStatoscopeTextureStreamingDG::GetTextureRequests()
{
    float res = 0.0f;

#if !defined (_RELEASE) || defined(ENABLE_STATOSCOPE_RELEASE)
    float time = gEnv->pTimer->GetAsyncCurTime() - CTexture::s_StreamingRequestsTime;
    res = ((float)CTexture::s_StreamingRequestsCount / time);
    CTexture::s_StreamingRequestsTime = gEnv->pTimer->GetAsyncCurTime();
    CTexture::s_StreamingRequestsCount = 0;
#endif

    return res;
}

float SStatoscopeTextureStreamingDG::GetTextureRenders()
{
    float res = 0.0f;

#if !defined (_RELEASE) || defined(ENABLE_STATOSCOPE_RELEASE)
    float time = gEnv->pTimer->GetAsyncCurTime() - CTexture::s_TextureUpdatedRenderedTime;
    res = ((float)CTexture::s_TexturesUpdatedRendered / time);
    CTexture::s_TextureUpdatedRenderedTime = gEnv->pTimer->GetAsyncCurTime();
    CTexture::s_TexturesUpdatedRendered = 0;
#endif

    return res;
}

float SStatoscopeTextureStreamingDG::GetTexturePoolUsage()
{
    return (float)(CTexture::s_nStatsStreamPoolInUseMem / (1024.0 * 1024.0));
}

float SStatoscopeTextureStreamingDG::GetTexturePoolWanted()
{
    return (float)(CTexture::s_nStatsStreamPoolWanted / (1024.0 * 1024.0));
}

float SStatoscopeTextureStreamingDG::GetTextureUpdates()
{
    float res = 0.0f;

#if !defined (_RELEASE) || defined(ENABLE_STATOSCOPE_RELEASE)
    float time = gEnv->pTimer->GetAsyncCurTime() - CTexture::s_TextureUpdatesTime;
    res = ((float)CTexture::s_TextureUpdates / time);
    CTexture::s_TextureUpdatesTime = gEnv->pTimer->GetAsyncCurTime();
    CTexture::s_TextureUpdates = 0;
#endif

    return res;
}

void SStatoscopeTextureStreamingItemsDG::Enable()
{
    CTexture::s_pStatsTexWantedLists = m_statsTexWantedLists;
}

void SStatoscopeTextureStreamingItemsDG::Disable()
{
    CTexture::s_pStatsTexWantedLists = NULL;
}

IStatoscopeDataGroup::SDescription SStatoscopeTextureStreamingItemsDG::GetDescription() const
{
    return SDescription('S', "Texture Streaming Items", "['/TexStrm/$/' (float requiredMB)]");
}

uint32 SStatoscopeTextureStreamingItemsDG::PrepareToWrite()
{
#ifdef STRIP_RENDER_THREAD
    int nThreadId = m_nCurThreadFill;
#else
    int nThreadId = gRenDev->m_pRT->m_nCurThreadFill;
#endif
    return (uint32)m_statsTexWantedLists[nThreadId].size();
}

void SStatoscopeTextureStreamingItemsDG::Write(IStatoscopeFrameRecord& fr)
{
#ifdef STRIP_RENDER_THREAD
    int nThreadId = m_nCurThreadFill;
#else
    int nThreadId = gRenDev->m_pRT->m_nCurThreadFill;
#endif
    std::vector<CTexture::WantedStat>& list = m_statsTexWantedLists[nThreadId];

    char name[MAX_PATH];

    for (size_t i = 0, c = list.size(); i != c; ++i)
    {
        azstrcpy(name, AZ_ARRAY_SIZE(name), list[i].pTex->GetName());
        Normalize(name);
        fr.AddValue(name);
        fr.AddValue(list[i].nWanted / (1024 * 1024.f));
    }

    list.clear();
}

#ifndef _RELEASE

void SStatoscopeTextureStreamingPoolDG::Enable()
{
    CTexture::s_pPoolMgr->EnableStatsComputation(true);
}

void SStatoscopeTextureStreamingPoolDG::Disable()
{
    CTexture::s_pPoolMgr->EnableStatsComputation(false);
}

IStatoscopeDataGroup::SDescription SStatoscopeTextureStreamingPoolDG::GetDescription() const
{
    return SDescription('P', "Texture Stream Pools", "['/TexPools/$/' (int n)]");
}

uint32 SStatoscopeTextureStreamingPoolDG::PrepareToWrite()
{
    CTexture::s_pPoolMgr->FetchPoolStats(m_ps);

    return m_ps.size() * 3;
}

void SStatoscopeTextureStreamingPoolDG::Write(IStatoscopeFrameRecord& fr)
{
    for (std::vector<CTextureStreamPoolMgr::SPoolStats>::const_iterator it = m_ps.begin(), itEnd = m_ps.end(); it != itEnd; ++it)
    {
        char name[128];
        sprintf_s(name, "Total/%ix%ix%ix%08xx%s", it->nWidth, it->nHeight, it->nMips, it->nFormat, it->eTT == eTT_2D ? "2D" : "Cube");

        fr.AddValue(name);
        fr.AddValue(it->nInUse + it->nFree);
    }

    for (std::vector<CTextureStreamPoolMgr::SPoolStats>::const_iterator it = m_ps.begin(), itEnd = m_ps.end(); it != itEnd; ++it)
    {
        char name[128];
        sprintf_s(name, "InUse/%ix%ix%ix%08xx%s", it->nWidth, it->nHeight, it->nMips, it->nFormat, it->eTT == eTT_2D ? "2D" : "Cube");

        fr.AddValue(name);
        fr.AddValue(it->nInUse);
    }

    for (std::vector<CTextureStreamPoolMgr::SPoolStats>::const_iterator it = m_ps.begin(), itEnd = m_ps.end(); it != itEnd; ++it)
    {
        char name[128];
        sprintf_s(name, "Free/%ix%ix%ix%08xx%s", it->nWidth, it->nHeight, it->nMips, it->nFormat, it->eTT == eTT_2D ? "2D" : "Cube");

        fr.AddValue(name);
        fr.AddValue(it->nFree);
    }
}

#endif

#endif
