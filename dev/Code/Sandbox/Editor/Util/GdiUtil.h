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

// Description : Utilitarian classes for double buffer GDI rendering and 32bit bitmaps


#ifndef CRYINCLUDE_EDITOR_UTIL_GDIUTIL_H
#define CRYINCLUDE_EDITOR_UTIL_GDIUTIL_H
#pragma once

#ifdef KDAB_MAC_PORT
#include <gdiplus.h>
#else
typedef struct _BLENDFUNCTION {
    BYTE BlendOp;
    BYTE BlendFlags;
    BYTE SourceConstantAlpha;
    BYTE AlphaFormat;
} BLENDFUNCTION, *PBLENDFUNCTION, *LPBLENDFUNCTION;

const DWORD SRCCOPY = 0;
const BYTE AC_SRC_OVER = 0;
const BYTE AC_SRC_ALPHA = 0;
typedef GUID CLSID;
namespace Gdiplus
{
    class Rect;
    class Graphics;
}
#endif // KDAB_MAC_PORT

const COLORREF kDrawBoxSameColorAsFill = 0xFFFFFFFF;

//! This is a class that manages a doublebuffer in GDI, so you can have flicker-free drawing
class CRYEDIT_API CGdiCanvas
{
public:

    CGdiCanvas();
    ~CGdiCanvas();

    bool Create(QWidget* widget);
    bool Resize(QWidget* widget);

    //! create the canvas from a compatible DC
    bool    Create(int aWidth, int aHeight);

#ifdef KDAB_MAC_PORT
    //! resize the canvas, without destroying DCs
    bool    Resize(HDC hCompatibleDC, UINT aWidth, UINT aHeight);
#endif
    //! clear the canvas with a color
    void    Clear(const QColor& aClearColor = QColor( 255, 255, 255 ));
    //! \return canvas width
    UINT    GetWidth();
    //! \return canvas height
    UINT    GetHeight();

    bool BlitTo(QWidget* widget);
#ifdef KDAB_MAC_PORT
    //! blit/copy the image from the canvas to other destination DC
    bool    BlitTo(HDC hDestDC, int aDestX = 0, int aDestY = 0, int aDestWidth = -1, int aDestHeight = -1, int aSrcX = 0, int aSrcY = 0, int aRop = SRCCOPY);
    //! blit/copy the image from the canvas to other destination window
    bool    BlitTo(HWND hDestWnd, int aDestX = 0, int aDestY = 0, int aDestWidth = -1, int aDestHeight = -1, int aSrcX = 0, int aSrcY = 0, int aRop = SRCCOPY);
#endif
    //! free the canvas data
    void    Free();
#ifdef KDAB_TEMPORARILY_REMOVED
    //! this will blit/copy a bitmap with alpha channel, using the given blend mode
    //! \param rBmp the bitmap to blit
    //! \param aDestX destination X on the canvas
    //! \param aDestY destination Y on the canvas
    //! \param aDestWidth destination width on the canvas, if -1 then the bitmap width will be used
    //! \param aDestHeight destination height on the canvas, if -1 then the bitmap height will be used
    //! \param pBlendFunc the blending function, if NULL then the normal alpha blend will be used
    bool    BitBltWithAlpha(CDC& rBmpDC, int aDestX, int aDestY, int aDestWidth = -1, int aDestHeight = -1, int aSrcX = 0, int aSrcY = 0, int aSrcWidth = -1, int aSrcHeight = -1, BLENDFUNCTION* pBlendFunc = NULL);
    //! this function breaks a text enclosed in a rectangle, on a character basis, because GDI is not capable of such feature, only on a word-break basis
    //! it will insert \r chars in the text
    //! \remark it will use the current font selected in the canvas' DC
    void    BreakTextByChars(const char* pText, CString& rOutStr, const CRect& rRect);
#endif
    //! function used to compute thumbs per row and spacing, used in asset browser and other tools where thumb layout is needed and maybe GDI canvas used
    //! \param aContainerWidth the thumbs' container width
    //! \param aThumbWidth the thumb image width
    //! \param aMargin the thumb default minimum horizontal margin
    //! \param aThumbCount the thumb count
    //! \param rThumbsPerRow returned thumb count per single row
    //! \param rNewMargin returned new computed margin between thumbs
    //! \note The margin between thumbs will grow/shrink dynamically to keep up with the thumb count per row
    static bool ComputeThumbsLayoutInfo(float aContainerWidth, float aThumbWidth, float aMargin, UINT aThumbCount, UINT& rThumbsPerRow, float& rNewMargin);
    //! this function will setup a blendfunc struct with a specified alpha value
#ifdef KDAB_MAC_PORT
    static void MakeBlendFunc(BYTE aAlpha, /*out*/ BLENDFUNCTION& rBlendFunc);
#endif
    static QColor ScaleColor(const QColor& coor, float aScale);

protected:

    UINT        m_width, m_height;
};

//! This class loads alpha-channel bitmaps and holds a DC for use with AlphaBlend function
class CRYEDIT_API CAlphaBitmap
{
public:

    CAlphaBitmap();
    ~CAlphaBitmap();

    //! load a bitmap file from a PNG or other alpha-capable format
    bool            Load(const char* pFilename, bool bVerticalFlip = false);
    //! save a bitmap to a file
    bool            Save(const char* pFileName);
    //! creates the bitmap from raw 32bpp data
    //! \param pData the 32bpp raw image data, RGBA, can be NULL and it would create just an empty bitmap
    //! \param aWidth the bitmap width
    //! \param aHeight the bitmap height
    bool            Create(void* pData, UINT aWidth, UINT aHeight, bool bVerticalFlip = false, bool bPremultiplyAlpha = false);
    //! \return the actual bitmap
    QImage&    GetBitmap();
    //! free the bitmap and DC
    void            Free();
    //! \return bitmap width
    UINT            GetWidth();
    //! \return bitmap height
    UINT            GetHeight();

protected:

    QImage m_bmp;
    UINT        m_width, m_height;
};

//! Fill a rectangle with a checkerboard pattern.
//! \param pGraphics The Graphics object used for drawing
//! \param rRect The rectangle to be filled
//! \param checkDiameter the diameter of the check squares
//! \param aColor1 the color that starts in the top left corner check square
//! \param aColor2 the second color used for check squares
#ifdef KDAB_MAC_PORT
void CheckerboardFillRect(Gdiplus::Graphics* pGraphics, Gdiplus::Rect& rRect, int checkDiameter, COLORREF aColor1, COLORREF aColor2);
#endif
#endif // CRYINCLUDE_EDITOR_UTIL_GDIUTIL_H
