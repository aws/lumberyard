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
// Purpose:
//  - Manage and cache glyphs, retrieving them from the renderer as needed

#ifndef CRYINCLUDE_CRYFONT_GLYPHCACHE_H
#define CRYINCLUDE_CRYFONT_GLYPHCACHE_H
#pragma once

#include <vector>
#include "GlyphBitmap.h"
#include "FontRenderer.h"
#include <StlUtils.h>


typedef struct CCacheSlot
{
    unsigned int    dwUsage;
    int             iCacheSlot;
    int             iHoriAdvance;
    uint32          cCurrentChar;

    uint8                   iCharWidth;                 // size in pixel
    uint8                   iCharHeight;                // size in pixel
    char                    iCharOffsetX;
    char                    iCharOffsetY;

    CGlyphBitmap    pGlyphBitmap;

    void            Reset()
    {
        dwUsage = 0;
        cCurrentChar = ~0;

        iCharWidth = 0;
        iCharHeight = 0;
        iCharOffsetX = 0;
        iCharOffsetY = 0;

        pGlyphBitmap.Clear();
    }

    void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(this, sizeof(*this));
        pSizer->AddObject(pGlyphBitmap);
    }
} CCacheSlot;


typedef AZStd::unordered_map<uint32, CCacheSlot*>          CCacheTable;

typedef std::vector<CCacheSlot*>                       CCacheSlotList;
typedef std::vector<CCacheSlot*>::iterator             CCacheSlotListItor;


#ifdef WIN64
#undef GetCharWidth
#undef GetCharHeight
#endif

class CGlyphCache
{
public:
    CGlyphCache();
    ~CGlyphCache();

    int Create(int iCacheSize, int iGlyphBitmapWidth, int iGlyphBitmapHeight, int iSmoothMethod, int iSmoothAmount, float fSizeRatio = 0.8f);
    int Release();

    int LoadFontFromFile(const string& szFileName);
    int LoadFontFromMemory(unsigned char* pFileBuffer, int iDataSize);
    int ReleaseFont();

    int SetEncoding(FT_Encoding pEncoding) { return m_pFontRenderer.SetEncoding(pEncoding); };
    FT_Encoding GetEncoding() { return m_pFontRenderer.GetEncoding(); };

    int GetGlyphBitmapSize(int* pWidth, int* pHeight);

    int PreCacheGlyph(uint32 cChar);
    int UnCacheGlyph(uint32 cChar);
    int GlyphCached(uint32 cChar);

    CCacheSlot* GetLRUSlot();
    CCacheSlot* GetMRUSlot();

    int GetGlyph(CGlyphBitmap** pGlyph, int* piHoriAdvance, int* piWidth, int* piHeight, char& iCharOffsetX, char& iCharOffsetY, uint32 cChar);

    void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(m_pSlotList);
        //pSizer->AddContainer(m_pCacheTable);
        pSizer->AddObject(m_pScaleBitmap);
        pSizer->AddObject(m_pFontRenderer);
    }

    bool GetMonospaced() const { return m_pFontRenderer.GetMonospaced(); }

    Vec2 GetKerning(uint32_t leftGlyph, uint32_t rightGlyph);

private:

    int             CreateSlotList(int iListSize);
    int             ReleaseSlotList();

    CCacheSlotList  m_pSlotList;
    CCacheTable     m_pCacheTable;

    int             m_iGlyphBitmapWidth;
    int             m_iGlyphBitmapHeight;
    float           m_fSizeRatio;

    int             m_iSmoothMethod;
    int             m_iSmoothAmount;

    CGlyphBitmap* m_pScaleBitmap;

    CFontRenderer   m_pFontRenderer;

    unsigned int    m_dwUsage;
};

#endif // CRYINCLUDE_CRYFONT_GLYPHCACHE_H
