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

//! A glyph cache slot. Used to store glyph information read from FreeType.
typedef struct CCacheSlot
{
    unsigned int    dwUsage;
    int             iCacheSlot;
    int             iHoriAdvance;   //!< Advance width. See FT_Glyph_Metrics::horiAdvance.
    uint32          cCurrentChar;

    uint8           iCharWidth;     //!< Glyph width (in pixel)
    uint8           iCharHeight;    //!< Glyph height (in pixel)
    AZ::s8          iCharOffsetX;   //!< Glyph's left-side bearing (in pixels). See FT_GlyphSlotRec::bitmap_left.
    AZ::s8          iCharOffsetY;   //!< Glyph's top bearing (in pixels). See FT_GlyphSlotRec::bitmap_top.

    CGlyphBitmap    pGlyphBitmap;   //!< Contains a buffer storing a copy of the glyph from FreeType

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

//! The glyph cache maps UTF32 codepoints to their corresponding FreeType data.
//!
//! This cache is used to associate font glyph info (read from FreeType) with
//! UTF32 codepoints. Ultimately the glyph info will be read into a font texture
//! (CFontTexture) to avoid future FreeType lookups.
//!
//! \sa CFontTexture
class CGlyphCache
{
public:
    CGlyphCache();
    ~CGlyphCache();

    int Create(int iCacheSize, int iGlyphBitmapWidth, int iGlyphBitmapHeight, int iSmoothMethod, int iSmoothAmount);
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

    //! Obtains glyph information for the given UTF32 codepoint.
    //! This information is obtained from a CCacheSlot that corresponds to
    //! the given codepoint. If the codepoint doesn't exist within the cache
    //! table (m_pCacheTable), then the information is obtain from FreeType
    //! directly via CFontRenderer.
    //!
    //! Ultimately the glyph bitmap is copied into a font texture 
    //! (CFontTexture). Once the glyph is copied into the font texture then
    //! the font texture is referenced directly rather than relying on the
    //! glyph cache or FreeType.
    //!
    //! \sa CFontRenderer::GetGlyph, CFontTexture::UpdateSlot
    int GetGlyph(CGlyphBitmap** pGlyph, int* piHoriAdvance, int* piWidth, int* piHeight, AZ::s8& iCharOffsetX, AZ::s8& iCharOffsetY, uint32 cChar);

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

    int             m_iSmoothMethod;
    int             m_iSmoothAmount;

    CGlyphBitmap* m_pScaleBitmap;

    CFontRenderer   m_pFontRenderer;

    unsigned int    m_dwUsage;
};

#endif // CRYINCLUDE_CRYFONT_GLYPHCACHE_H
