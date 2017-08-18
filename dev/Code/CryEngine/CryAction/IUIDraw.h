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

// Description : UI draw functions interface

#ifndef CRYINCLUDE_CRYACTION_IUIDRAW_H
#define CRYINCLUDE_CRYACTION_IUIDRAW_H
#pragma once

//-----------------------------------------------------------------------------------------------------

struct IFFont;

enum EUIDRAWHORIZONTAL
{
    UIDRAWHORIZONTAL_LEFT,
    UIDRAWHORIZONTAL_CENTER,
    UIDRAWHORIZONTAL_RIGHT
};

enum EUIDRAWVERTICAL
{
    UIDRAWVERTICAL_TOP,
    UIDRAWVERTICAL_CENTER,
    UIDRAWVERTICAL_BOTTOM
};

//-----------------------------------------------------------------------------------------------------

struct IUIDraw
{
    virtual ~IUIDraw(){}
    virtual void Release() = 0;

    virtual void PreRender() = 0;
    virtual void PostRender() = 0;

    // TODO: uintARGB or float,float,float,float ?

    virtual uint32 GetColorARGB(uint8 ucAlpha, uint8 ucRed, uint8 ucGreen, uint8 ucBlue) = 0;

    virtual int CreateTexture(const char* strName, bool dontRelease = true) = 0;

    virtual bool DeleteTexture(int iTextureID) = 0;

    virtual void GetTextureSize(int iTextureID, float& rfSizeX, float& rfSizeY) = 0;

    virtual void DrawTriangle(float fX0, float fY0, float fX1, float fY1, float fX2, float fY2, uint32 uiColor) = 0;

    virtual void DrawLine(float fX1, float fY1, float fX2, float fY2, uint32 uiDiffuse) = 0;

    virtual void DrawQuadSimple(float fX, float fY, float fSizeX, float fSizeY, uint32 uiDiffuse, int iTextureID) = 0;

    virtual void DrawQuad(float fX,
        float fY,
        float fSizeX,
        float fSizeY,
        uint32 uiDiffuse = 0,
        uint32 uiDiffuseTL = 0, uint32 uiDiffuseTR = 0, uint32 uiDiffuseDL = 0, uint32 uiDiffuseDR = 0,
        int iTextureID = -1,
        float fUTexCoordsTL = 0.0f, float fVTexCoordsTL = 0.0f,
        float fUTexCoordsTR = 1.0f, float fVTexCoordsTR = 0.0f,
        float fUTexCoordsDL = 0.0f, float fVTexCoordsDL = 1.0f,
        float fUTexCoordsDR = 1.0f, float fVTexCoordsDR = 1.0f,
        bool bUse169 = true) = 0;


    virtual void DrawImage(int iTextureID, float fX,
        float fY,
        float fSizeX,
        float fSizeY,
        float fAngleInDegrees,
        float fRed,
        float fGreen,
        float fBlue,
        float fAlpha,
        float fS0 = 0.0f,
        float fT0 = 0.0f,
        float fS1 = 1.0f,
        float fT1 = 1.0f) = 0;

    virtual void DrawImageCentered(int iTextureID, float fX,
        float fY,
        float fSizeX,
        float fSizeY,
        float fAngleInDegrees,
        float fRed,
        float fGreen,
        float fBlue,
        float fAlpha) = 0;


    virtual void DrawTextSimple(IFFont* pFont,
        float fX, float fY,
        float fSizeX, float fSizeY,
        const char* strText, ColorF color,
        EUIDRAWHORIZONTAL   eUIDrawHorizontal, EUIDRAWVERTICAL      eUIDrawVertical) = 0;

    virtual void DrawText(IFFont* pFont,
        float fX,
        float fY,
        float fSizeX,
        float fSizeY,
        const char* strText,
        float fAlpha,
        float fRed,
        float fGreen,
        float fBlue,
        EUIDRAWHORIZONTAL   eUIDrawHorizontalDocking,
        EUIDRAWVERTICAL     eUIDrawVerticalDocking,
        EUIDRAWHORIZONTAL   eUIDrawHorizontal,
        EUIDRAWVERTICAL     eUIDrawVertical) = 0;

    virtual void DrawWrappedText(IFFont* pFont,
        float fX,
        float fY,
        float fMaxWidth,
        float fSizeX,
        float fSizeY,
        const char* strText,
        float fAlpha,
        float fRed,
        float fGreen,
        float fBlue,
        EUIDRAWHORIZONTAL   eUIDrawHorizontalDocking,
        EUIDRAWVERTICAL     eUIDrawVerticalDocking,
        EUIDRAWHORIZONTAL   eUIDrawHorizontal,
        EUIDRAWVERTICAL     eUIDrawVertical) = 0;

    virtual void GetTextDim(IFFont* pFont,
        float* fWidth,
        float* fHeight,
        float fSizeX,
        float fSizeY,
        const char* strText) = 0;

    virtual void GetWrappedTextDim(IFFont* pFont,
        float* fWidth,
        float* fHeight,
        float fMaxWidth,
        float fSizeX,
        float fSizeY,
        const char* strText) = 0;
};

//-----------------------------------------------------------------------------------------------------

#endif // CRYINCLUDE_CRYACTION_IUIDRAW_H

//-----------------------------------------------------------------------------------------------------
