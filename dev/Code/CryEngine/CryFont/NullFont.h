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

// Description : Dummy font implementation (dedicated server)


#ifndef CRYINCLUDE_CRYFONT_NULLFONT_H
#define CRYINCLUDE_CRYFONT_NULLFONT_H
#pragma once


#if defined(USE_NULLFONT)

#include <IFont.h>

class CNullFont
    : public IFFont
{
public:
    CNullFont() {}
    virtual ~CNullFont() {}

    virtual int32 AddRef() { return 0; };
    virtual int32 Release() { return 0; };

    virtual bool Load(const char* pFontFilePath, unsigned int width, unsigned int height, unsigned int widthNumSlots, unsigned int heightNumSlots, unsigned int flags, float sizeRatio) { return true; }
    virtual bool Load(const char* pXMLFile) { return true; }
    virtual void Free() {}

    virtual void DrawString(float x, float y, const char* pStr, const bool asciiMultiLine, const STextDrawContext& ctx) {}
    virtual void DrawString(float x, float y, float z, const char* pStr, const bool asciiMultiLine, const STextDrawContext& ctx) {}

    virtual void DrawStringW(float x, float y, const wchar_t* pStr, const bool asciiMultiLine, const STextDrawContext& ctx) {}
    virtual void DrawStringW(float x, float y, float z, const wchar_t* pStr, const bool asciiMultiLine, const STextDrawContext& ctx) {}

    virtual Vec2 GetTextSize(const char* pStr, const bool asciiMultiLine, const STextDrawContext& ctx) { return Vec2(0.0f, 0.0f); }
    virtual Vec2 GetTextSizeW(const wchar_t* pStr, const bool asciiMultiLine, const STextDrawContext& ctx) { return Vec2(0.0f, 0.0f); }

    virtual size_t GetTextLength(const char* pStr, const bool asciiMultiLine) const { return 0; }
    virtual size_t GetTextLengthW(const wchar_t* pStr, const bool asciiMultiLine) const { return 0; }

    virtual void WrapText(string& result, float maxWidth, const char* pStr, const STextDrawContext& ctx) { result = pStr; }
    virtual void WrapTextW(wstring& result, float maxWidth, const wchar_t* pStr, const STextDrawContext& ctx) { result = pStr; }

    virtual void GetMemoryUsage(ICrySizer* pSizer) const {}

    virtual void GetGradientTextureCoord(float& minU, float& minV, float& maxU, float& maxV) const {}

    virtual unsigned int GetEffectId(const char* pEffectName) const { return 0; }
    virtual unsigned int GetNumEffects() const { return 0; }
    virtual const char* GetEffectName(unsigned int effectId) const { return nullptr; }
    virtual Vec2 GetMaxEffectOffset(unsigned int effectId) const { return Vec2(); }
    virtual bool DoesEffectHaveTransparency(unsigned int effectId) const { return false; }

    virtual void AddCharsToFontTexture(const char* pChars, int glyphSizeX, int glyphSizeY) override {}
    virtual Vec2 GetKerning(uint32_t leftGlyph, uint32_t rightGlyph, const STextDrawContext& ctx) const override { return Vec2(); }
    virtual float GetAscender(const STextDrawContext& ctx) const override { return 0.0f; }
    virtual float GetBaseline(const STextDrawContext& ctx) const override { return 0.0f; }
    virtual float GetSizeRatio() const override { return IFFontConstants::defaultSizeRatio; }
    virtual uint32 GetNumQuadsForText(const char* pStr, const bool asciiMultiLine, const STextDrawContext& ctx) { return 0; }
    virtual uint32 WriteTextQuadsToBuffers(SVF_P2F_C4B_T2F_F4B* verts, uint16* indices, uint32 maxQuads, float x, float y, float z, const char* pStr, const bool asciiMultiLine, const STextDrawContext& ctx) { return 0; }
    virtual int GetFontTextureId() { return -1; }
    virtual uint32 GetFontTextureVersion() { return 0; }
};

class CCryNullFont
    : public ICryFont
{
public:
    virtual void Release() {}
    virtual IFFont* NewFont(const char* pFontName) { return &ms_nullFont; }
    virtual IFFont* GetFont(const char* pFontName) const { return &ms_nullFont; }
    virtual FontFamilyPtr LoadFontFamily(const char* pFontFamilyName) override { CRY_ASSERT(false); return nullptr; }
    virtual FontFamilyPtr GetFontFamily(const char* pFontFamilyName) override { CRY_ASSERT(false); return nullptr; }
    virtual void AddCharsToFontTextures(FontFamilyPtr pFontFamily, const char* pChars, int glyphSizeX, int glyphSizeY) override {};
    virtual void SetRendererProperties(IRenderer* pRenderer) {}
    virtual void GetMemoryUsage(ICrySizer* pSizer) const {}
    virtual string GetLoadedFontNames() const { return ""; }
    virtual void OnLanguageChanged() override { }
    virtual void ReloadAllFonts() override { } 

private:
    static CNullFont ms_nullFont;
};

#endif // USE_NULLFONT

#endif // CRYINCLUDE_CRYFONT_NULLFONT_H
