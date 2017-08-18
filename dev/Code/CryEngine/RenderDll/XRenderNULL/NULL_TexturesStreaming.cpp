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
#include "../Common/Textures/TextureStreamPool.h"

//===============================================================================

bool STexPoolItem::IsStillUsedByGPU(uint32 nCurTick)
{
    return false;
}

STexPoolItem::~STexPoolItem()
{
}

void CTexture::InitStreamingDev()
{
}

void CTexture::StreamExpandMip(const void* pRawData, int nMip, int nBaseMipOffset, int nSideDelta)
{
}

void CTexture::StreamCopyMipsTexToTex(STexPoolItem* pSrcItem, int nMipSrc, STexPoolItem* pDestItem, int nMipDest, int nNumMips)
{
}

bool CTexture::StreamPrepare_Platform()
{
    return true;
}

int CTexture::StreamTrim(int nToMip)
{
    return 0;
}

// Just remove item from the texture object and keep Item in Pool list for future use
// This function doesn't release API texture
void CTexture::StreamRemoveFromPool()
{
}

void CTexture::StreamCopyMipsTexToMem(int nStartMip, int nEndMip, bool bToDevice, STexPoolItem* pNewPoolItem)
{
}

STexPoolItem* CTexture::StreamGetPoolItem(int nStartMip, int nMips, bool bShouldBeCreated, bool bCreateFromMipData, bool bCanCreate, bool bForStreamOut)
{
    return NULL;
}

void CTexture::StreamAssignPoolItem(STexPoolItem* pItem, int nMinMip)
{
}


CTextureStreamPoolMgr::CTextureStreamPoolMgr()
{
}

CTextureStreamPoolMgr::~CTextureStreamPoolMgr()
{
}

void CTextureStreamPoolMgr::Flush()
{
}

STexPoolItem* CTextureStreamPoolMgr::GetPoolItem(int nWidth, int nHeight, int nArraySize, int nMips, ETEX_Format eTF, bool bIsSRGB, ETEX_Type eTT, bool bShouldBeCreated, const char* sName, STextureInfo* pTI, bool bCanCreate, bool bWaitForIdle)
{
    return NULL;
}

void CTextureStreamPoolMgr::ReleaseItem(STexPoolItem* pItem)
{
}

void CTextureStreamPoolMgr::GarbageCollect(size_t* nCurTexPoolSize, size_t nLowerPoolLimit, int nMaxItemsToFree)
{
}

void CTextureStreamPoolMgr::GetMemoryUsage(ICrySizer* pSizer)
{
}
