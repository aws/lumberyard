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
#include "GdiUtil.h"
#include "Image.h"
#include "StringUtils.h"

#include <QColor>
#include <QPainter>
#include <QMessageBox>

CGdiCanvas::CGdiCanvas()
{
    m_width = m_height = 0;
}

CGdiCanvas::~CGdiCanvas()
{
    Free();
}

bool CGdiCanvas::Create(QWidget* widget)
{
    if (!widget)
        return false;
    return Create(widget->width(), widget->height());
}

bool CGdiCanvas::Resize(QWidget* widget)
{
#ifdef KDAB_MAC_PORT
    return Resize(::GetDC(reinterpret_cast<HWND>(widget->winId())), widget->width(), widget->height());
#else
    return false;
#endif // KDAB_MAC_PORT
}

bool CGdiCanvas::BlitTo(QWidget* widget)
{
#ifdef KDAB_MAC_PORT
    return BlitTo(reinterpret_cast<HWND>(widget->winId()));
#else
    return false;
#endif
}

bool CGdiCanvas::Create(int aWidth, int aHeight)
{
    if (!aWidth || !aHeight)
    {
        return false;
    }

    Free();

    m_width = aWidth;
    m_height = aHeight;

    return true;
}

#ifdef KDAB_MAC_PORT
bool CGdiCanvas::Resize(HDC hCompatibleDC, UINT aWidth, UINT aHeight)
{
#ifdef KDAB_TEMPORARILY_REMOVED
    if (m_memBmp.GetSafeHandle())
    {
        m_memBmp.DeleteObject();
    }

    CDC dc;

    m_hCompatibleDC = hCompatibleDC;
    dc.Attach(m_hCompatibleDC);

    if (!m_memBmp.CreateCompatibleBitmap(&dc, aWidth, aHeight))
    {
        dc.Detach();
        return false;
    }

    dc.Detach();
    m_memDC.SelectObject(m_memBmp);
    m_width = aWidth;
    m_height = aHeight;
#endif
    return true;
}
#endif

#ifdef KDAB_TEMPORARILY_REMOVED
void CGdiCanvas::Clear(COLORREF aClearColor)
{
    m_memDC.FillSolidRect(0, 0, m_width, m_height, aClearColor);
}
#endif

UINT CGdiCanvas::GetWidth()
{
    return m_width;
}

UINT CGdiCanvas::GetHeight()
{
    return m_height;
}

#ifdef KDAB_MAC_PORT
bool CGdiCanvas::BlitTo(HDC hDestDC, int aDestX, int aDestY, int aDestWidth, int aDestHeight, int aSrcX, int aSrcY, int aRop)
{
#ifdef KDAB_TEMPORARILY_REMOVED
    CDC dc;
    BITMAP bmpInfo;

    dc.Attach(hDestDC);

    if (!m_memBmp.GetBitmap(&bmpInfo))
    {
        dc.Detach();
        return false;
    }

    aDestWidth = aDestWidth == -1 ?  bmpInfo.bmWidth : aDestWidth;
    aDestHeight = aDestHeight == -1 ?  bmpInfo.bmHeight : aDestHeight;
    dc.BitBlt(aDestX, aDestY, aDestWidth, aDestHeight, &m_memDC, aSrcX, aSrcY, aRop);
    dc.Detach();
#endif
    return true;
}
#endif

#ifdef KDAB_MAC_PORT
bool CGdiCanvas::BlitTo(HWND hDestWnd, int aDestX, int aDestY, int aDestWidth, int aDestHeight, int aSrcX, int aSrcY, int aRop)
{
#ifdef KDAB_MAC_PORT
    HDC hDC = ::GetDC(hDestWnd);
    bool bRet = BlitTo(hDC, aDestX, aDestY, aDestWidth, aDestHeight, aSrcX, aSrcY, aRop);
    ReleaseDC(hDestWnd, hDC);

    return bRet;
#else
    return false;
#endif // KDAB_MAC_PORT
}
#endif

void CGdiCanvas::Free()
{
    m_width = 0;
    m_height = 0;
}

bool CGdiCanvas::ComputeThumbsLayoutInfo(float aContainerWidth, float aThumbWidth, float aMargin, UINT aThumbCount, UINT& rThumbsPerRow, float& rNewMargin)
{
    rThumbsPerRow = 0;
    rNewMargin = 0;

    if (aThumbWidth <= 0 || aMargin <= 0 || (aThumbWidth + aMargin * 2) <= 0)
    {
        return false;
    }

    if (aContainerWidth <= 0)
    {
        return true;
    }

    rThumbsPerRow = (int) aContainerWidth / (aThumbWidth + aMargin * 2);

    if ((aThumbWidth + aMargin * 2) * aThumbCount < aContainerWidth)
    {
        rNewMargin = aMargin;
    }
    else
    {
        if (rThumbsPerRow > 0)
        {
            rNewMargin = (aContainerWidth - rThumbsPerRow * aThumbWidth);

            if (rNewMargin > 0)
            {
                rNewMargin = (float)rNewMargin / rThumbsPerRow / 2.0f;
            }
        }
    }

    return true;
}

#ifdef KDAB_MAC_PORT
void CGdiCanvas::MakeBlendFunc(BYTE aAlpha, BLENDFUNCTION& rBlendFunc)
{
    rBlendFunc.BlendOp = AC_SRC_OVER;
    rBlendFunc.BlendFlags = 0;
    rBlendFunc.SourceConstantAlpha = aAlpha;
    rBlendFunc.AlphaFormat = AC_SRC_ALPHA;
}
#endif

QColor CGdiCanvas::ScaleColor(const QColor& c, float aScale)
{
    QColor aColor = c;
    if (!aColor.isValid())
    {
        // help out scaling, by starting at very low black
        aColor = QColor(1, 1, 1);
    }

    int r = aColor.red();
    int g = aColor.green();
    int b = aColor.blue();

    r *= aScale;
    g *= aScale;
    b *= aScale;

    return QColor(CLAMP(r, 0, 255), CLAMP(g, 0, 255), CLAMP(b, 0, 255));
}

CAlphaBitmap::CAlphaBitmap()
{
    m_width = m_height = 0;
}

CAlphaBitmap::~CAlphaBitmap()
{
    Free();
}

#ifdef KDAB_TEMPORARILY_REMOVED
bool CAlphaBitmap::Load(const char* pFilename, bool bVerticalFlip)
{
    Free();
    WCHAR wzFilename[MAX_PATH];
    Unicode::Convert(wzFilename, pFilename);
    Gdiplus::Bitmap img(wzFilename);
    Gdiplus::BitmapData bmData;

    if (0 != img.GetLastStatus())
    {
        return false;
    }

    HDC hDC = ::GetDC(GetDesktopWindow());
    CDC dc;

    dc.Attach(hDC);

    if (!m_dcForBlitting.CreateCompatibleDC(&dc))
    {
        dc.Detach();
        ReleaseDC(GetDesktopWindow(), hDC);
        return false;
    }

    if (bVerticalFlip)
    {
        img.RotateFlip(Gdiplus::RotateNoneFlipY);
    }

    img.LockBits(NULL, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &bmData);

    if (!bmData.Scan0)
    {
        dc.Detach();
        ReleaseDC(GetDesktopWindow(), hDC);
        return false;
    }

    // premultiply alpha, AlphaBlend GDI expects it
    for (UINT y = 0; y < bmData.Height; ++y)
    {
        BYTE* pPixel = (BYTE*) bmData.Scan0 + bmData.Width * 4 * y;

        for (UINT x = 0; x < bmData.Width; ++x)
        {
            pPixel[0] = ((int)pPixel[0] * pPixel[3] + 127) >> 8;
            pPixel[1] = ((int)pPixel[1] * pPixel[3] + 127) >> 8;
            pPixel[2] = ((int)pPixel[2] * pPixel[3] + 127) >> 8;
            pPixel += 4;
        }
    }

    BITMAPINFO bmi;

    ZeroMemory(&bmi, sizeof(BITMAPINFO));
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = bmData.Width;
    bmi.bmiHeader.biHeight = bmData.Height;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    bmi.bmiHeader.biSizeImage = bmData.Width * bmData.Height * 4;
    void* pBits = NULL;
    HBITMAP hBmp = CreateDIBSection(hDC, &bmi, DIB_RGB_COLORS, &pBits, 0, 0);

    if (!hBmp)
    {
        dc.Detach();
        ReleaseDC(GetDesktopWindow(), hDC);
        img.UnlockBits(&bmData);
        return false;
    }

    if (!pBits || !bmData.Scan0)
    {
        dc.Detach();
        ReleaseDC(GetDesktopWindow(), hDC);
        img.UnlockBits(&bmData);
        return false;
    }

    memcpy(pBits, bmData.Scan0, bmi.bmiHeader.biSizeImage);
    m_bmp.Attach(hBmp);
    m_width = bmData.Width;
    m_height = bmData.Height;
    img.UnlockBits(&bmData);
    m_dcForBlitting.SelectObject(m_bmp);
    dc.Detach();
    ReleaseDC(GetDesktopWindow(), hDC);

    return true;
}

bool    CAlphaBitmap::Save(const char* pFileName)
{
    BITMAP bp;

    if (m_bmp.GetBitmap(&bp) == 0)
    {
        return false;
    }

    if (bp.bmBitsPixel != 32)
    {
        return false;
    }

    BITMAPINFO bminfo;
    memset(&bminfo, 0, sizeof(bminfo));

    bminfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bminfo.bmiHeader.biWidth = bp.bmWidth;
    bminfo.bmiHeader.biHeight = bp.bmHeight;
    bminfo.bmiHeader.biPlanes = bp.bmPlanes;
    bminfo.bmiHeader.biBitCount = bp.bmBitsPixel;
    bminfo.bmiHeader.biCompression =  BI_RGB;
    bminfo.bmiHeader.biSizeImage = 0;
    bminfo.bmiHeader.biXPelsPerMeter = 0;
    bminfo.bmiHeader.biYPelsPerMeter = 0;
    bminfo.bmiHeader.biClrUsed = 0;
    bminfo.bmiHeader.biClrImportant = 0;

    int imgSize = bp.bmWidthBytes * bp.bmHeight;
    _smart_ptr<IMemoryBlock> imgBuffer = gEnv->pCryPak->PoolAllocMemoryBlock(imgSize, "");
    int returnedBytes = m_bmp.GetBitmapBits(imgSize, imgBuffer->GetData());
    if (returnedBytes == 0)
    {
        return false;
    }
    Gdiplus::Bitmap bitmap(&bminfo, imgBuffer->GetData());
    CLSID clsid;

    if (!GetCLSIDObjFromFileExt(pFileName, &clsid))
    {
        return false;
    }

    wstring wideFileName = CryStringUtils::UTF8ToWStr(pFileName);
    if (bitmap.Save(wideFileName.c_str(), &clsid, NULL) != Gdiplus::Ok)
    {
        return false;
    }

    return true;
}

bool CAlphaBitmap::GetCLSIDObjFromFileExt(const char* fileName, CLSID* pClsid) const
{
    if (pClsid == NULL)
    {
        return false;
    }

    if (strstr(fileName, ".png"))
    {
        if (GetEncoderClsid("image/png", pClsid) == -1)
        {
            return false;
        }
    }
    else if (strstr(fileName, ".jpg"))
    {
        if (GetEncoderClsid("image/jpeg", pClsid) == -1)
        {
            return false;
        }
    }
    else if (strstr(fileName, ".gif"))
    {
        if (GetEncoderClsid("image/gif", pClsid) == -1)
        {
            return false;
        }
    }
    else if (strstr(fileName, ".tif"))
    {
        if (GetEncoderClsid("image/tiff", pClsid) == -1)
        {
            return false;
        }
    }
    else if (strstr(fileName, ".bmp"))
    {
        if (GetEncoderClsid("image/bmp", pClsid) == -1)
        {
            return false;
        }
    }

    return true;
}
#endif

bool CAlphaBitmap::Create(void* pData, UINT aWidth, UINT aHeight, bool bVerticalFlip, bool bPremultiplyAlpha)
{
    if (!aWidth || !aHeight)
    {
        return false;
    }

    m_bmp = QImage(aWidth, aHeight, QImage::Format_RGBA8888);
    if (m_bmp.isNull())
    {
        return false;
    }

    std::vector<UINT> vBuffer;

    if (pData)
    {
        // copy over the raw 32bpp data
        bVerticalFlip = !bVerticalFlip; // in Qt, the flip is not required. Still, keep the API behaving the same
        if (bVerticalFlip)
        {
            UINT nBufLen = aWidth * aHeight;
            vBuffer.resize(nBufLen);

            if (IsBadReadPtr(pData, nBufLen * 4))
            {
                //TODO: remove after testing alot the browser, it doesnt happen anymore
                QMessageBox::critical(QApplication::activeWindow(), QString(), QObject::tr("Bad image data ptr!"));
                Free();
                return false;
            }

            assert(!vBuffer.empty());

            if (vBuffer.empty())
            {
                Free();
                return false;
            }

            UINT scanlineSize = aWidth * 4;

            for (UINT i = 0, iCount = aHeight; i < iCount; ++i)
            {
                // top scanline position
                UINT* pTopScanPos = (UINT*)&vBuffer[0] + i * aWidth;
                // bottom scanline position
                UINT* pBottomScanPos = (UINT*)pData + (aHeight - i - 1) * aWidth;

                // save a scanline from top
                memcpy(pTopScanPos, pBottomScanPos, scanlineSize);
            }

            pData = &vBuffer[0];
        }

        // premultiply alpha, AlphaBlend GDI expects it
        if (bPremultiplyAlpha)
        {
            for (UINT y = 0; y < aHeight; ++y)
            {
                BYTE* pPixel = (BYTE*) pData + aWidth * 4 * y;

                for (UINT x = 0; x < aWidth; ++x)
                {
                    pPixel[0] = ((int)pPixel[0] * pPixel[3] + 127) >> 8;
                    pPixel[1] = ((int)pPixel[1] * pPixel[3] + 127) >> 8;
                    pPixel[2] = ((int)pPixel[2] * pPixel[3] + 127) >> 8;
                    pPixel += 4;
                }
            }
        }

        memcpy(m_bmp.bits(), pData, aWidth * aHeight * 4);

        if (m_bmp.isNull())
        {
            return false;
        }
    }
    else
    {
        m_bmp.fill(Qt::transparent);
    }

    // we dont need this screen DC anymore
    m_width = aWidth;
    m_height = aHeight;

    return true;
}

QImage& CAlphaBitmap::GetBitmap()
{
    return m_bmp;
}

void CAlphaBitmap::Free()
{

}

UINT CAlphaBitmap::GetWidth()
{
    return m_width;
}

UINT CAlphaBitmap::GetHeight()
{
    return m_height;
}

void CheckerboardFillRect(QPainter* pGraphics, const QRect& rRect, int checkDiameter, const QColor& aColor1, const QColor& aColor2)
{
    pGraphics->save();
    pGraphics->setClipRect(rRect);
    // Create a checkerboard background for easier readability
    pGraphics->fillRect(rRect, aColor1);
    QBrush lightBrush(aColor2);

    // QRect bottom/right methods are short one unit for legacy reasons. Compute bottomr/right of the rectange ourselves to get the full size.
    const int rectRight = rRect.x() + rRect.width();
    const int rectBottom = rRect.y() + rRect.height();

    for (int i = rRect.left(); i < rectRight; i += checkDiameter)
    {
        for (int j = rRect.top(); j < rectBottom; j += checkDiameter)
        {
            if ((i / checkDiameter) % 2 ^ (j / checkDiameter) % 2)
            {
                pGraphics->fillRect(QRect(i, j, checkDiameter, checkDiameter), lightBrush);
            }
        }
    }
    pGraphics->restore();
}
