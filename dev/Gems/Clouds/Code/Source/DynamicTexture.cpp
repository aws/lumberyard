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

// Description : Common dynamic texture manager implementation.


#include "StdAfx.h"
#include "DynamicTexture.h"

namespace CloudsGem
{
    static const int s_TexturePoolBlockLogSize = 5;
    static const int s_TexturePoolBlockSize = (1 << s_TexturePoolBlockLogSize); // 32

    uint32 DynamicTexture::s_CurTexAtlasSize = 0;
    uint32 DynamicTexture::s_SuggestedDynTexAtlasCloudsMaxsize = 0;
    uint32 DynamicTexture::s_nMemoryOccupied = 0;
    DynamicTexture::TextureSetMap DynamicTexture::s_TexturePool;

    void DynamicTexture::UnlinkGlobal()
    {
        // Remove this element from the doubly linked list
        if (m_Next)
        {
            m_Next->m_PrevLink = m_PrevLink;
        }
        if (m_PrevLink)
        {
            *m_PrevLink = m_Next;
        }
    }

    void DynamicTexture::LinkGlobal(DynamicTexture*& beforeTexture)
    {
        // Insertion before element specified
        if (beforeTexture)
        {
            beforeTexture->m_PrevLink = &m_Next;
        }
        m_Next = beforeTexture;
        m_PrevLink = &beforeTexture;
        beforeTexture = this;
    }

    void DynamicTexture::Link()
    {
        // Link to the root of the global list of all dynamic textures
        LinkGlobal(m_pOwner->m_pRoot);
    }

    void DynamicTexture::Unlink()
    {
        // Unlink current node from global list
        UnlinkGlobal();
        m_Next = nullptr;
        m_PrevLink = nullptr;
    }

    bool DynamicTexture::Remove()
    {
        if (m_pAllocator)
        {
            if (m_nBlockID != -1)
            {
                m_pAllocator->RemoveBlock(m_nBlockID);
            }

            m_nBlockID = ~0;
            m_pTexture = nullptr;
            m_nUpdateMask = 0;
            m_pAllocator = nullptr;
            Unlink();
            return true;
        }
        return false;
    }

    bool DynamicTexture::ClearRT()
    {
        gEnv->pRenderer->FX_ClearTarget(m_pTexture);
        return true;
    }

    bool DynamicTexture::SetRT(int nRT, bool bPush, SDepthTexture* pDepthSurf, bool bScreenVP)
    {
        Update(m_nWidth, m_nHeight);
        bool bRes = false;

        assert(m_pTexture);
        if (m_pTexture)
        {
            IRenderer* renderer = gEnv->pRenderer;
            bRes = bPush ? renderer->FX_PushRenderTarget(nRT, m_pTexture, pDepthSurf) : renderer->FX_SetRenderTarget(nRT, m_pTexture, pDepthSurf);
            SetRectStates();
            gEnv->pRenderer->FX_Commit();
        }
        return bRes;
    }

    bool DynamicTexture::SetRectStates()
    {
        gEnv->pRenderer->RT_SetViewport(m_nX, m_nY, m_nWidth, m_nHeight);
        gEnv->pRenderer->EF_Scissor(true, m_nX, m_nY, m_nWidth, m_nHeight);
        return true;
    }

    bool DynamicTexture::RestoreRT(int nRT, bool bPop)
    {
        gEnv->pRenderer->EF_Scissor(false, m_nX, m_nY, m_nWidth, m_nHeight);
        bool bRes = bPop ? gEnv->pRenderer->FX_PopRenderTarget(nRT) : gEnv->pRenderer->FX_RestoreRenderTarget(nRT);
        gEnv->pRenderer->FX_Commit();
        return bRes;
    }

    void DynamicTexture::GetImageRect(uint32& nX, uint32& nY, uint32& nWidth, uint32& nHeight)
    {
        nX = 0;
        nY = 0;

        if (m_pTexture)
        {
            if (m_pTexture->GetWidth() != DynamicTexture::s_CurTexAtlasSize || m_pTexture->GetHeight() != DynamicTexture::s_CurTexAtlasSize)
            {
                UpdateAtlasSize(DynamicTexture::s_CurTexAtlasSize, DynamicTexture::s_CurTexAtlasSize);
            }
            assert(m_pTexture->GetWidth() == DynamicTexture::s_CurTexAtlasSize || m_pTexture->GetHeight() == DynamicTexture::s_CurTexAtlasSize);
        }
        nWidth = DynamicTexture::s_CurTexAtlasSize;
        nHeight = DynamicTexture::s_CurTexAtlasSize;
    }


    DynamicTexture::DynamicTexture(const char* szSource)
    {
        m_name = AZStd::string(szSource);
        m_nFrameReset = gEnv->pRenderer->GetFrameReset();
        SetUpdateMask();
    }

    void DynamicTexture::SetUpdateMask()
    {
        // Update mask with current frame's bit enabled
        int nFrame = gEnv->pRenderer->RT_GetCurrGpuID();
        m_nUpdateMask |= 1 << nFrame;
    }

    void DynamicTexture::ResetUpdateMask()
    {
        m_nUpdateMask = 0;
        m_nFrameReset = gEnv->pRenderer->GetFrameReset();
    }

    DynamicTexture::DynamicTexture(uint32 nWidth, uint32 nHeight, uint32 nTexFlags, const char* szSource)
    {
        m_nWidth = nWidth;
        m_nHeight = nHeight;
        m_name = AZStd::string(szSource);

        // Find appropriate texture set for the given format
        ETEX_Format eTF = eTF_R8G8B8A8;
        auto textureSet = s_TexturePool.find(eTF);
        if (textureSet == s_TexturePool.end())
        {
            // Create new texture set for this texture
            m_pOwner = new TextureSetFormat(eTF, FT_NOMIPS | FT_USAGE_ATLAS);
            s_TexturePool.insert(TextureSetMap::value_type(eTF, m_pOwner));
        }
        else
        {
            m_pOwner = textureSet->second;
        }

        m_nFrameReset = gEnv->pRenderer->GetFrameReset();
        SetUpdateMask();
    }


    void DynamicTexture::Init()
    {
#if !defined(DEDICATED_SERVER)

        static ICVar* textureAtlasSizeVar = gEnv->pConsole->GetCVar("r_TexAtlasSize");
        AZ_Assert(textureAtlasSizeVar, "Unable to find r_TexAtlasSize cvar");

        static ICVar* dynTexAtlasCloudsMaxSizeVar = gEnv->pConsole->GetCVar("r_DynTexAtlasCloudsMaxSize");
        AZ_Assert(dynTexAtlasCloudsMaxSizeVar, "Unable to find r_DynTexAtlasCloudsMaxSize cvar");

        s_CurTexAtlasSize = textureAtlasSizeVar->GetIVal();
        s_SuggestedDynTexAtlasCloudsMaxsize = dynTexAtlasCloudsMaxSizeVar->GetIVal();

        int nSize = textureAtlasSizeVar->GetIVal();

        TArray<DynamicTexture*> textures;
        const char* szName = GetPoolName();
        while (true)
        {
            int nNeedSpace = CTexture::TextureDataSize(nSize, nSize, 1, 1, 1, eTF_R8G8B8A8);
            if (nNeedSpace + s_nMemoryOccupied > s_SuggestedDynTexAtlasCloudsMaxsize * 1024 * 1024)
            {
                break;
            }
            DynamicTexture* pTex = new DynamicTexture(nSize, nSize, FT_STATE_CLAMP | FT_NOMIPS, szName);
            pTex->Update(nSize, nSize);
            textures.AddElem(pTex);
        }

        // Free actual texture objects
        for (uint32 i = 0; i < textures.Num(); i++)
        {
            SAFE_DELETE(textures[i]);
        }
#endif
    }

    bool DynamicTexture::UpdateAtlasSize(int nNewWidth, int nNewHeight)
    {
        // Early out if invalid
        if (!m_pOwner)
        {
            return false;
        }
        if (!m_pTexture)
        {
            return false;
        }
        if (!m_pAllocator)
        {
            return false;
        }

        // Only update when the texture atlas dimensions have changed
        if (m_pTexture->GetWidth() != nNewWidth || m_pTexture->GetHeight() != nNewHeight)
        {
            // Remove all but this texture from the owning texture set
            DynamicTexture* pDT = nullptr, * pNext = nullptr;
            for (pDT = m_pOwner->m_pRoot; pDT; pDT = pNext)
            {
                pNext = pDT->m_Next;

                // Skip this texture
                if (pDT == this)
                {
                    continue;
                }

                // Remove from list. Block are still owned by allocator though
                assert(!pDT->m_bLocked);
                pDT->Remove();
                pDT->SetUpdateMask();
            }

            // Quantize the current block width and height
            int nBlockW = (m_nWidth + s_TexturePoolBlockSize - 1) / s_TexturePoolBlockSize;
            int nBlockH = (m_nHeight + s_TexturePoolBlockSize - 1) / s_TexturePoolBlockSize;

            // Remove this block from alloctor
            m_pAllocator->RemoveBlock(m_nBlockID);
            assert(m_pAllocator && m_pAllocator->GetNumUsedBlocks() == 0);

            // Compute the new block width and height
            int nW = (nNewWidth + s_TexturePoolBlockSize - 1) / s_TexturePoolBlockSize;
            int nH = (nNewHeight + s_TexturePoolBlockSize - 1) / s_TexturePoolBlockSize;

            // Update allocator and get new block
            m_pAllocator->UpdateSize(nW, nH);
            m_nBlockID = m_pAllocator->AddBlock(nBlockW, nBlockH);

            // Destroy current texture
            s_nMemoryOccupied -= CTexture::TextureDataSize(m_pTexture->GetWidth(), m_pTexture->GetHeight(), 1, 1, 1, eTF_R8G8B8A8);
            SAFE_RELEASE(m_pTexture);

            // Allocate new texture
            char name[256];
            sprintf_s(name, "$Dyn_2D_%s_%s_%d", CTexture::NameForTextureFormat(eTF_R8G8B8A8), GetPoolName(), gEnv->pRenderer->GenerateTextureId());
            m_pAllocator->m_pTexture = CTexture::CreateRenderTarget(name, nNewWidth, nNewHeight, Clr_Transparent, m_pOwner->m_eTT, m_pOwner->m_nTexFlags, eTF_R8G8B8A8);
            s_nMemoryOccupied += CTexture::TextureDataSize(nNewWidth, nNewHeight, 1, 1, 1, eTF_R8G8B8A8);
            m_pTexture = m_pAllocator->m_pTexture;
        }
        return true;
    }

    bool DynamicTexture::Update(int nNewWidth, int nNewHeight)
    {
#if !defined(DEDICATED_SERVER)
        int i = 0;
        bool bRecreate = false;
        DynamicTexture* pDT = nullptr;

        // Texture must have an owner (texture set)
        if (!m_pOwner)
        {
            return false;
        }

        // Recreate if there is no allocator
        if (!m_pAllocator)
        {
            bRecreate = true;
        }

        // Record frame in which the texture has been accessed
        SRenderPipeline* renderPipeline = gEnv->pRenderer->GetRenderPipeline();
        m_nAccessFrame = renderPipeline->m_TI[renderPipeline->m_nProcessThreadID].m_nFrameUpdateID;
        uint32 nFrame = m_nAccessFrame;

        // Recreate if dimensions have changed
        if (m_nWidth != nNewWidth || m_nHeight != nNewHeight)
        {
            bRecreate = true;
            m_nWidth = nNewWidth;
            m_nHeight = nNewHeight;
        }

        // Need to allocate space for this texture
        if (bRecreate)
        {
            // Restrict size to [512. 2048]
            static ICVar* textureAtlasSizeVar = gEnv->pConsole->GetCVar("r_TexAtlasSize");
            AZ_Assert(textureAtlasSizeVar, "Unable to find r_TexAtlasSize cvar");
            int nSize = textureAtlasSizeVar->GetIVal();
            nSize = nSize <= 512 ? 512 : nSize <= 1024 ? 1024 : nSize > 2048 ? 2048 : nSize;

            // Update atlas size cvar
            textureAtlasSizeVar->Set(nSize);

            // Determine how much space is needed for this texture and resize the pool as needed
            int nNeedSpace = CTexture::TextureDataSize(nSize, nSize, 1, 1, 1, eTF_R8G8B8A8);
            if (nNeedSpace > s_SuggestedDynTexAtlasCloudsMaxsize * 1024 * 1024)
            {
                SetPoolMaxSize(nNeedSpace / (1024 * 1024), true);
            }

            // Quantize to block size
            int nBlockW = (nNewWidth + s_TexturePoolBlockSize - 1) / s_TexturePoolBlockSize;
            int nBlockH = (nNewHeight + s_TexturePoolBlockSize - 1) / s_TexturePoolBlockSize;

            // Reset this node
            Remove();
            SetUpdateMask();

            // Add a block to the first texture pool that has space available
            uint32 nID = ~0;
            CPowerOf2BlockPacker* pPack = nullptr;
            for (i = 0; i < (int)m_pOwner->m_TexPools.size(); i++)
            {
                pPack = m_pOwner->m_TexPools[i];
                nID = pPack->AddBlock(LogBaseTwo(nBlockW), LogBaseTwo(nBlockH));
                if (nID != -1)
                {
                    break;
                }
            }

            // There was no packer with available space
            if (i == m_pOwner->m_TexPools.size())
            {
                // Do we need more space?
                if (nNeedSpace + s_nMemoryOccupied > s_SuggestedDynTexAtlasCloudsMaxsize * 1024 * 1024)
                {
                    DynamicTexture* pDTBest = nullptr;
                    DynamicTexture* pDTBestLarge = nullptr;

                    int nError = 1000000;
                    uint32 nFr = INT_MAX;
                    uint32 nFrLarge = INT_MAX;
                    int n = 0;

                    // Walk list of textures in the pool that manages this texture
                    for (pDT = m_pOwner->m_pRoot; pDT; pDT = pDT->m_Next)
                    {
                        // Skip us and locked textures (textures currently in use)
                        if (pDT == this || pDT->m_bLocked)
                        {
                            continue;
                        }

                        n++;
                        assert(pDT->m_pAllocator && pDT->m_pTexture && pDT->m_nBlockID != -1);

                        // Check for match against width and height
                        if (pDT->m_nWidth == m_nWidth && pDT->m_nHeight == m_nHeight)
                        {
                            // We want to pick the texture with the most recent access time
                            if (nFr > pDT->m_nAccessFrame)
                            {
                                // Update best access time and texture
                                nFr = pDT->m_nAccessFrame;
                                pDTBest = pDT;
                            }
                        }

                        // We failed to find an exact match for width and height
                        // so check if the texture is larger than what we require
                        else if (pDT->m_nWidth >= m_nWidth && pDT->m_nHeight >= m_nHeight)
                        {
                            // Quantify error in terms of difference in width and height from exact match
                            int nEr = pDT->m_nWidth - m_nWidth + pDT->m_nHeight - m_nHeight;

                            // Increase error term by number of frames since last access time
                            int fEr = nEr + (nFrame - pDT->m_nAccessFrame);

                            // Update if this match is the best so far.
                            if (fEr < nError)
                            {
                                nFrLarge = pDT->m_nAccessFrame;
                                nError = fEr;
                                pDTBestLarge = pDT;
                            }
                        }
                    }

                    pDT = nullptr;
                    if (pDTBest && nFr + 1 < nFrame && pDTBest->m_nBlockID != -1)
                    {
                        // Found an exact match
                        pDT = pDTBest;
                        nID = pDT->m_nBlockID;
                        pPack = pDT->m_pAllocator;

                        // Clear state for this entry
                        pDT->m_pAllocator = nullptr;
                        pDT->m_pTexture = nullptr;
                        pDT->m_nBlockID = ~0;
                        pDT->m_nUpdateMask = 0; // Clear update mask
                        pDT->SetUpdateMask();
                        pDT->Unlink();
                    }
                    else if (pDTBestLarge && nFrLarge + 1 < nFrame)
                    {
                        // Found a match that exceeds the size we need
                        CPowerOf2BlockPacker* pAllocator = pDTBestLarge->m_pAllocator;
                        pDT = pDTBestLarge;
                        pDT->Remove();
                        pDT->SetUpdateMask();

                        // Add new block
                        nID = pAllocator->AddBlock(LogBaseTwo(nBlockW), LogBaseTwo(nBlockH));
                        assert(nID != -1);

                        // If we were able to add the block then we can move on.
                        if (nID != -1)
                        {
                            pPack = pAllocator;
                        }
                        else
                        {
                            // Still no success...
                            pDT = nullptr;
                        }
                    }

                    // Still no matching texture
                    if (!pDT)
                    {
                        // Try to find oldest texture pool
                        float fTime = FLT_MAX;
                        CPowerOf2BlockPacker* pPackBest = nullptr;
                        for (auto pNextPack : m_pOwner->m_TexPools)
                        {
                            if (fTime > pNextPack->m_fLastUsed) // Older the better
                            {
                                fTime = pNextPack->m_fLastUsed;
                                pPackBest = pNextPack;
                            }
                        }

                        // Unable to find a pool or pool is too old?
                        if (!pPackBest || fTime + 0.5f > renderPipeline->m_TI[renderPipeline->m_nProcessThreadID].m_RealTime)
                        {
                            // Try to find most fragmented texture pool with least number of used blocks
                            uint32 nUsedBlocks = s_TexturePoolBlockSize * s_TexturePoolBlockSize + 1;
                            pPackBest = nullptr;
                            for (auto pNextPack : m_pOwner->m_TexPools)
                            {
                                int nBlocks = pNextPack->GetNumUsedBlocks();
                                if (nBlocks < (int)nUsedBlocks)
                                {
                                    nUsedBlocks = nBlocks;
                                    pPackBest = pNextPack;
                                }
                            }
                        }

                        // Did we find an appropriate allocator?
                        if (pPackBest)
                        {
                            // Traverse nodes of the texture set removing any node with the same packer
                            DynamicTexture* pNext = nullptr;
                            for (pDT = m_pOwner->m_pRoot; pDT; pDT = pNext)
                            {
                                pNext = pDT->m_Next;

                                // Skip this node and locked nodes
                                if (pDT == this || pDT->m_bLocked)
                                {
                                    continue;
                                }

                                if (pDT->m_pAllocator == pPackBest)
                                {
                                    pDT->Remove();
                                }
                            }

                            // Our packer should have zero blocks left now
                            assert(pPackBest->GetNumUsedBlocks() == 0);

                            // Create new block
                            pPack = pPackBest;
                            nID = pPack->AddBlock(LogBaseTwo(nBlockW), LogBaseTwo(nBlockH));
                            pPack->m_fLastUsed = renderPipeline->m_TI[renderPipeline->m_nProcessThreadID].m_RealTime;

                            // Check allocation
                            if (nID != -1)
                            {
                                pDT = this;
                                pDT->m_nUpdateMask = 0; // clear update mask
                            }
                            else
                            {
                                // fail
                                assert(0);
                                pDT = nullptr;
                            }
                        }
                    }
                }

                // How about now?
                if (!pDT)
                {
                    // Add a brand new allocator, that is large enough to handle the required number of blocks, to the texture set
                    int n = (nSize + s_TexturePoolBlockSize - 1) / s_TexturePoolBlockSize;
                    pPack = new CPowerOf2BlockPacker(LogBaseTwo(n), LogBaseTwo(n));
                    m_pOwner->m_TexPools.push_back(pPack);

                    // Add new blocks
                    nID = pPack->AddBlock(LogBaseTwo(nBlockW), LogBaseTwo(nBlockH));

                    // Create a new render target
                    char name[256];
                    sprintf_s(name, "$Dyn_2D_%s_%s_%d", CTexture::NameForTextureFormat(eTF_R8G8B8A8), GetPoolName(), gEnv->pRenderer->GenerateTextureId());
                    pPack->m_pTexture = CTexture::CreateRenderTarget(name, nSize, nSize, Clr_Transparent, m_pOwner->m_eTT, m_pOwner->m_nTexFlags, eTF_R8G8B8A8);

                    // Update memory usage
                    s_nMemoryOccupied += nNeedSpace;

                    // Verify that block for valid
                    if (nID == -1)
                    {
                        assert(0);
                        nID = (uint32) - 2;
                    }
                }
            }

            // Check that everything worked as expected i
            assert(nID != -1 && nID != -2);

            m_nBlockID = nID;
            m_pAllocator = pPack;
            if (pPack)
            {
                m_pTexture = pPack->m_pTexture;
                uint32 nX1, nX2, nY1, nY2;
                pPack->GetBlockInfo(nID, nX1, nY1, nX2, nY2);
                m_nX = nX1 << s_TexturePoolBlockLogSize;
                m_nY = nY1 << s_TexturePoolBlockLogSize;
                m_nWidth = (nX2 - nX1) << s_TexturePoolBlockLogSize;
                m_nHeight = (nY2 - nY1) << s_TexturePoolBlockLogSize;
                m_nUpdateMask = 0;
                SetUpdateMask();
            }
        }

        m_pAllocator->m_fLastUsed = renderPipeline->m_TI[renderPipeline->m_nProcessThreadID].m_RealTime;
        Unlink();

        if (m_pTexture)
        {
            Link();
            return true;
        }
#endif
        return false;
    }


    void DynamicTexture::Apply(int nTUnit, int nTS)
    {
        if (m_pAllocator)
        {
            // Update the allocate last used time used by free node search criteria
            SRenderPipeline* renderPipeline = gEnv->pRenderer->GetRenderPipeline();
            m_pAllocator->m_fLastUsed = renderPipeline->m_TI[renderPipeline->m_nProcessThreadID].m_RealTime;

            // Allocate / update texture if needed
            if (!m_pTexture)
            {
                Update(m_nWidth, m_nHeight);
            }

            // If the update resulted in a valid texture then apply it
            if (m_pTexture)
            {
                m_pTexture->Apply(nTUnit, nTS);
            }

            // Set render target rectangle on shader manager
            CShaderMan* shaderManager = gEnv->pRenderer->GetShaderManager();
            float width = float(m_pTexture->GetWidth());
            float height = float(m_pTexture->GetHeight());
            shaderManager->m_RTRect.x = float(m_nX) / width;
            shaderManager->m_RTRect.y = float(m_nY) / height;
            shaderManager->m_RTRect.z = float(m_nWidth) / width;
            shaderManager->m_RTRect.w = float(m_nHeight) / height;
        }
    }

    bool DynamicTexture::IsValid()
    {
        if (m_pTexture)
        {
            // Update last access frame
            SRenderPipeline* renderPipeline = gEnv->pRenderer->GetRenderPipeline();
            m_nAccessFrame = renderPipeline->m_TI[renderPipeline->m_nProcessThreadID].m_nFrameUpdateID;

            if (m_nFrameReset != gEnv->pRenderer->GetFrameReset())
            {
                m_nFrameReset = gEnv->pRenderer->GetFrameReset();
                m_nUpdateMask = 0;
                return false;
            }

            // If we have more than one gpu
            if (gEnv->pRenderer->GetActiveGPUCount() > 1)
            {
                // If this is an ATI card then verify image rect is valid
                if ((gEnv->pRenderer->GetFeatures() & RFT_HW_MASK) == RFT_HW_ATI)
                {
                    // Make sure the image rect is large enough
                    uint32 nX, nY, nW, nH;
                    GetImageRect(nX, nY, nW, nH);
                    if (nW < 1024 && nH < 1024)
                    {
                        return true;
                    }
                }

                // Check if the update mask has been has the current GPU frame enabled
                int nFrame = gEnv->pRenderer->RT_GetCurrGpuID();
                if (!((1 << nFrame) & m_nUpdateMask))
                {
                    return false;
                }
            }
        }
        return true;
    }

    void DynamicTexture::ShutDown()
    {
        for (auto entry : s_TexturePool)
        {
            TextureSetFormat* pF = entry.second;
            DynamicTexture* pDT, * pNext;
            for (pDT = pF->m_pRoot; pDT; pDT = pNext)
            {
                pNext = pDT->m_Next;
                SAFE_RELEASE_FORCE(pDT);
            }
            SAFE_DELETE(pF);
            s_TexturePool.clear();
        }
    }

    void DynamicTexture::GetSubImageRect(uint32& nX, uint32& nY, uint32& nWidth, uint32& nHeight)
    {
        nX = m_nX;
        nY = m_nY;
        nWidth = m_nWidth;
        nHeight = m_nHeight;
    }

    TextureSetFormat::~TextureSetFormat()
    {
        for (uint32 i = 0; i < m_TexPools.size(); i++)
        {
            CPowerOf2BlockPacker* pP = m_TexPools[i];
            if (pP->m_pTexture)
            {
                uint32 nSize = CTexture::TextureDataSize(pP->m_pTexture->GetWidth(), pP->m_pTexture->GetHeight(), 1, 1, 1, pP->m_pTexture->GetTextureDstFormat());
                DynamicTexture::s_nMemoryOccupied = max<uint32>(0, DynamicTexture::s_nMemoryOccupied - nSize);
            }
            SAFE_DELETE(pP);
        }
        m_TexPools.clear();
    }
}