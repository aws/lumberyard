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

#ifndef CRYINCLUDE_CRYFONT_FONTTEXTURE_H
#define CRYINCLUDE_CRYFONT_FONTTEXTURE_H
#pragma once

#include "GlyphCache.h"
#include "GlyphBitmap.h"


typedef uint8 FONT_TEXTURE_TYPE;

// the number of slots in the glyph cache
// each slot ocupies ((glyph_bitmap_width * glyph_bitmap_height) + 24) bytes
#define FONT_GLYPH_CACHE_SIZE           (1)

// the glyph spacing in font texels between characters in proportional font mode (more correct would be to take the value in the character)
#define FONT_GLYPH_PROP_SPACING     (1)

// the size of a rendered space, this value gets multiplied by the default characted width
#define FONT_SPACE_SIZE                     (0.5f)

// don't draw this char (used to avoid drawing color codes)
#define FONT_NOT_DRAWABLE_CHAR      (0xffff)

// smoothing methods
#define FONT_SMOOTH_NONE            0
#define FONT_SMOOTH_BLUR            1
#define FONT_SMOOTH_SUPERSAMPLE     2

// smoothing amounts
#define FONT_SMOOTH_AMOUNT_NONE     0
#define FONT_SMOOTH_AMOUNT_2X       1
#define FONT_SMOOTH_AMOUNT_4X       2

//! Stores glyph meta-data read from the font (FreeType).
//!
//! \sa CCacheSlot
typedef struct CTextureSlot
{
    uint16              wSlotUsage;                 //!< For LRU strategy, 0xffff is never released
    uint32              cCurrentChar;               //!< ~0 if not used for characters
    int                 iTextureSlot;
    int                 iHoriAdvance;               //!< Advance width. See FT_Glyph_Metrics::horiAdvance.
    float               vTexCoord[2];               //!< Character position in the texture (not yet half texel corrected)
    uint8               iCharWidth;                 //!< Glyph width (in pixel)
    uint8               iCharHeight;                //!< Glyph height (in pixel)
    AZ::s8              iCharOffsetX;               //!< Glyph's left-side bearing (in pixels). See FT_GlyphSlotRec::bitmap_left.
    AZ::s8              iCharOffsetY;               //!< Glyph's top bearing (in pixels). See FT_GlyphSlotRec::bitmap_top.

    void Reset()
    {
        wSlotUsage = 0;
        cCurrentChar = ~0;
        iHoriAdvance = 0;
        iCharWidth = 0;
        iCharHeight = 0;
        iCharOffsetX = 0;
        iCharOffsetY = 0;
    }

    void SetNotReusable()
    {
        wSlotUsage = 0xffff;
    }

    void GetMemoryUsage(ICrySizer* pSizer) const {}
} CTextureSlot;


typedef std::vector<CTextureSlot*>                                     CTextureSlotList;
typedef std::vector<CTextureSlot*>::iterator                           CTextureSlotListItor;

typedef AZStd::unordered_map<uint32, CTextureSlot*>                    CTextureSlotTable;
typedef AZStd::unordered_map<uint32, CTextureSlot*>::iterator          CTextureSlotTableItor;
typedef AZStd::unordered_map<uint32, CTextureSlot*>::const_iterator    CTextureSlotTableItorConst;

#ifdef WIN64
#undef GetCharWidth
#undef GetCharHeight
#endif

//! Stores the glyphs of a font within a single texture.
//!
//! The texture resolution is configurable, as is the number of slots within
//! the texture. 
//!
//! A texture slot contains a single glyph within the font and are uniform
//! size throughout the font texture (each slot occupies the same size 
//! regardless of the size of a glyph being stored, so a '.' occupies the 
//! same amount of space as a 'W', for example).
//!
//! Font glyph buffers are read from FreeType and copied into the texture.
//!
//! \sa CTextureSlot, CFontRenderer
class CFontTexture
{
public:
    CFontTexture();
    ~CFontTexture();

    int CreateFromFile(const string& szFileName, int iWidth, int iHeight, int iSmoothMethod, int iSmoothAmount, int iWidthCharCount = 16, int iHeightCharCount = 16);

    //! Default texture slot width/height is 16x8 slots, allowing for 128 glyphs to be stored in the font texture. This was
    //! previously 16x16, allowing 256 glyphs to be stored. For reference, there are 95 printable ASCII characters, so by
    //! reducing the number of slots, the height of the font texture can be halved (for some nice memory savings). We may
    //! want to make this configurable in the font XML (especially for languages with a large number of printable chars).
    int CreateFromMemory(unsigned char* pFileData, int iDataSize, int iWidth, int iHeight, int iSmoothMethod, int iSmoothAmount, int iWidthCharCount, int iHeightCharCount);

    int Create(int iWidth, int iHeight, int iSmoothMethod, int iSmoothAmount, int iWidthCharCount = 16, int iHeightCharCount = 16);
    int Release();

    int SetEncoding(FT_Encoding pEncoding) { return m_pGlyphCache.SetEncoding(pEncoding); }
    FT_Encoding GetEncoding() { return m_pGlyphCache.GetEncoding(); }

    int GetCellWidth() { return m_iCellWidth; }
    int GetCellHeight() { return m_iCellHeight; }

    int GetWidth() { return m_iWidth; }
    int GetHeight() { return m_iHeight; }

    int GetWidthCellCount() { return m_iWidthCellCount; }
    int GetHeightCellCount() { return m_iHeightCellCount; }

    float GetTextureCellWidth() { return m_fTextureCellWidth; }
    float GetTextureCellHeight() { return m_fTextureCellHeight; }

    FONT_TEXTURE_TYPE* GetBuffer() { return m_pBuffer; }

    uint32 GetSlotChar(int iSlot) const;
    CTextureSlot* GetCharSlot(uint32 cChar);
    CTextureSlot* GetGradientSlot();

    CTextureSlot* GetLRUSlot();
    CTextureSlot* GetMRUSlot();

    // returns 1 if texture updated, returns 2 if texture not updated, returns 0 on error
    // pUpdated is the number of slots updated
    int PreCacheString(const char* szString, int* pUpdated = 0);
    // Arguments:
    //   pSlot - function does nothing if this pointer is 0
    void GetTextureCoord(CTextureSlot * pSlot, float texCoords[4], int& iCharSizeX, int& iCharSizeY, int& iCharOffsetX, int& iCharOffsetY) const;
    int GetCharacterWidth(uint32 cChar) const;
    int GetHorizontalAdvance(uint32 cChar) const;
    //  int GetCharHeightByChar(wchar_t cChar);

    // useful for special feature rendering interleaved with fonts (e.g. box behind the text)
    void CreateGradientSlot();

    int WriteToFile(const string& szFileName);

    void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(this, sizeof(*this));
        pSizer->AddObject(m_pGlyphCache);
        pSizer->AddObject(m_pSlotList);
        //pSizer->AddContainer(m_pSlotTable);
        pSizer->AddObject(m_pBuffer, m_iWidth * m_iHeight * sizeof(FONT_TEXTURE_TYPE));
    }

    bool GetMonospaced() const { return m_pGlyphCache.GetMonospaced(); }

    Vec2 GetKerning(uint32_t leftGlyph, uint32_t rightGlyph);

private: // ---------------------------------------------------------------

    int CreateSlotList(int iListSize);
    int ReleaseSlotList();
    int UpdateSlot(int iSlot, uint16 wSlotUsage, uint32 cChar);

    // --------------------------------

    int                                     m_iWidth;                               // whole texture cache width
    int                                     m_iHeight;                          // whole texture cache height

    float                                   m_fInvWidth;
    float                                   m_fInvHeight;

    int                                     m_iCellWidth;
    int                                     m_iCellHeight;

    float                                   m_fTextureCellWidth;
    float                                   m_fTextureCellHeight;

    int                                     m_iWidthCellCount;
    int                                     m_iHeightCellCount;

    int                                     m_nTextureSlotCount;

    int                                     m_iSmoothMethod;
    int                                     m_iSmoothAmount;

    CGlyphCache                     m_pGlyphCache;
    CTextureSlotList            m_pSlotList;
    CTextureSlotTable           m_pSlotTable;

    FONT_TEXTURE_TYPE*     m_pBuffer;                           // [y*iWidth * x] x=0..iWidth-1, y=0..iHeight-1

    uint16                              m_wSlotUsage;
};


#endif // CRYINCLUDE_CRYFONT_FONTTEXTURE_H
