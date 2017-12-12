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

// Description : Tooltip that displays bitmap.


#include "StdAfx.h"
#include "BitmapToolTip.h"
#include "Util/PathUtil.h"

#include <QApplication>
#include <QDesktopWidget>
#include <QKeyEvent>

static const int STATIC_TEXT_C_HEIGHT = 42;
static const int HISTOGRAM_C_HEIGHT = 130;

/////////////////////////////////////////////////////////////////////////////
// CBitmapToolTip
CBitmapToolTip::CBitmapToolTip(QWidget* parent)
    : QWidget(parent, Qt::ToolTip)
    , m_staticBitmap(this)
    , m_staticText(this)
    , m_rgbaHistogram(this)
    , m_alphaChannelHistogram(this)
{
    m_nTimer = 0;
    m_hToolWnd = 0;
    m_bShowHistogram = true;
    m_bShowFullsize = false;

    connect(&m_timer, &QTimer::timeout, this, &CBitmapToolTip::OnTimer);
}

CBitmapToolTip::~CBitmapToolTip()
{
}

//////////////////////////////////////////////////////////////////////////
void CBitmapToolTip::GetShowMode(EShowMode& eShowMode, bool& bShowInOriginalSize) const
{
    bShowInOriginalSize = CheckVirtualKey(Qt::Key_Space);
    eShowMode = ESHOW_RGB;

    if (m_bHasAlpha)
    {
        if (CheckVirtualKey(Qt::Key_Control))
        {
            eShowMode = ESHOW_RGB_ALPHA;
        }
        else if (CheckVirtualKey(Qt::Key_Menu))
        {
            eShowMode = ESHOW_ALPHA;
        }
        else if (CheckVirtualKey(Qt::Key_Shift))
        {
            eShowMode = ESHOW_RGBA;
        }
    }
    else if (m_bIsLimitedHDR)
    {
        if (CheckVirtualKey(Qt::Key_Shift))
        {
            eShowMode = ESHOW_RGBE;
        }
    }
}

const char* CBitmapToolTip::GetShowModeDescription(EShowMode eShowMode, bool bShowInOriginalSize) const
{
    switch (eShowMode)
    {
    case ESHOW_RGB:
        return "RGB";
    case ESHOW_RGB_ALPHA:
        return "RGB+A";
    case ESHOW_ALPHA:
        return "Alpha";
    case ESHOW_RGBA:
        return "RGBA";
    case ESHOW_RGBE:
        return "RGBExp";
    }

    return "";
}

void CBitmapToolTip::RefreshLayout()
{
    QRect rcClient = rect();

        if (rcClient.width() <= 0)
        {
            return;
        }
        if (rcClient.height() <= 0)
        {
            return;
        }

        QRect rc(rcClient);
        int histoHeight = (m_bShowHistogram ? HISTOGRAM_C_HEIGHT : 0);

        rc.setBottom(rc.bottom() - STATIC_TEXT_C_HEIGHT - histoHeight);
        m_staticBitmap.setGeometry(rc);

        rc = rcClient;
        rc.setTop(rc.bottom() - STATIC_TEXT_C_HEIGHT - histoHeight);
        m_staticText.setGeometry(rc);

        if (m_bShowHistogram)
        {
            QRect rcHistoRgba, rcHistoAlpha;

            if (m_eShowMode == ESHOW_RGB_ALPHA || m_eShowMode == ESHOW_RGBA)
            {
                rc = rcClient;
                rc.setTop(rc.bottom() - histoHeight);
                rc.setRight(rc.width() / 2);
                rcHistoRgba = rc;

                rc = rcClient;
                rc.setTop(rc.bottom() - histoHeight);
                rc.setLeft(rcHistoRgba.right());
                rc.setRight(rcClient.width());
                rcHistoAlpha = rc;

                m_rgbaHistogram.setVisible(true);
                m_alphaChannelHistogram.setVisible(true);
            }
            else if (m_eShowMode == ESHOW_ALPHA)
            {
                rc = rcClient;
                rc.setTop(rc.bottom() - histoHeight);
                rc.setRight(rc.width());
                rcHistoAlpha = rc;

                m_rgbaHistogram.setVisible(false);
                m_alphaChannelHistogram.setVisible(true);
            }
            else
            {
                rc = rcClient;
                rc.setTop(rc.bottom() - histoHeight);
                rc.setRight(rc.width());
                rcHistoRgba = rc;

                m_rgbaHistogram.setVisible(true);
                m_alphaChannelHistogram.setVisible(false);
            }

            m_rgbaHistogram.setGeometry(rcHistoRgba);
            m_alphaChannelHistogram.setGeometry(rcHistoAlpha);
        }
}

void CBitmapToolTip::RefreshViewmode()
{
    EShowMode eShowMode = ESHOW_RGB;
    bool bShowInOriginalSize = false;

    GetShowMode(eShowMode, bShowInOriginalSize);

    if ((m_eShowMode != eShowMode) || (m_bShowFullsize != bShowInOriginalSize))
    {
        LoadImage(m_filename);

        RefreshLayout();
        CorrectPosition();
        update();
    }
}

bool CBitmapToolTip::LoadImage(const QString& imageFilename)
{
    // For max quality, let's try loading the source image if available:
    QString convertedFileName = Path::GamePathToFullPath(Path::ReplaceExtension(imageFilename, ".dds"));

    CCryFile fileCheck;
    if (!fileCheck.Open(convertedFileName.toLatin1().data(), "rb"))
    {
        // if we didn't find it, then default back to just using what we can find (if any)
        convertedFileName = imageFilename;
    }
    else
    {
        fileCheck.Close();
    }


    EShowMode eShowMode = ESHOW_RGB;
    const char* pShowModeDescription = "RGB";
    bool bShowInOriginalSize = false;

    GetShowMode(eShowMode, bShowInOriginalSize);
    pShowModeDescription = GetShowModeDescription(eShowMode, bShowInOriginalSize);

    if ((m_filename == convertedFileName) && (m_eShowMode == eShowMode) && (m_bShowFullsize == bShowInOriginalSize))
    {
        return true;
    }

    m_eShowMode = eShowMode;
    m_bShowFullsize = bShowInOriginalSize;

    CImageEx image;
    image.SetHistogramEqualization(CheckVirtualKey(Qt::Key_Shift));
    bool loadedRequestedAsset = true;
    if (!CImageUtil::LoadImage(imageFilename, image))
    {
        //Failed to load the requested asset,  let's try loading the source asset if available.
        loadedRequestedAsset = false;
        if (!CImageUtil::LoadImage(convertedFileName, image))
        {
            m_staticBitmap.clear();
            update();
            return false;
        }
    }

    QString imginfo;

    m_filename = loadedRequestedAsset ? imageFilename : convertedFileName;
    m_bHasAlpha = image.HasAlphaChannel();
    m_bIsLimitedHDR = image.IsLimitedHDR();

    GetShowMode(eShowMode, bShowInOriginalSize);
    pShowModeDescription = GetShowModeDescription(eShowMode, bShowInOriginalSize);

    if (m_bHasAlpha)
    {
        imginfo = tr("%1x%2 %3\nShowing %4 (ALT=Alpha, SHIFT=RGBA, CTRL=RGB+A, SPACE=see in original size)");
    }
    else if (m_bIsLimitedHDR)
    {
        imginfo = tr("%1x%2 %3\nShowing %4 (SHIFT=see hist.-equalized, SPACE=see in original size)");
    }
    else
    {
        imginfo = tr("%1x%2 %3\nShowing %4 (SPACE=see in original size)");
    }

    imginfo = imginfo.arg(image.GetWidth()).arg(image.GetHeight()).arg(image.GetFormatDescription()).arg(pShowModeDescription);

    m_staticText.setText(imginfo);

    int w = image.GetWidth();
    int h = image.GetHeight();
    int multiplier = (m_eShowMode == ESHOW_RGB_ALPHA ? 2 : 1);
    int originalW = w * multiplier;
    int originalH = h;

    if (!bShowInOriginalSize || (w == 0))
    {
        w = 256;
    }
    if (!bShowInOriginalSize || (h == 0))
    {
        h = 256;
    }

    w *= multiplier;

    resize(w + 4, h + 4 + STATIC_TEXT_C_HEIGHT + HISTOGRAM_C_HEIGHT);
    setVisible(true);

    CImageEx scaledImage;

    if (bShowInOriginalSize && (originalW < w))
    {
        w = originalW;
    }
    if (bShowInOriginalSize && (originalH < h))
    {
        h = originalH;
    }

    scaledImage.Allocate(w, h);

    if (m_eShowMode == ESHOW_RGB_ALPHA)
    {
        CImageUtil::ScaleToDoubleFit(image, scaledImage);
    }
    else
    {
        CImageUtil::ScaleToFit(image, scaledImage);
    }

    if (m_eShowMode == ESHOW_RGB || m_eShowMode == ESHOW_RGBE)
    {
        scaledImage.SwapRedAndBlue();
        scaledImage.FillAlpha();
    }
    else if (m_eShowMode == ESHOW_ALPHA)
    {
        for (int h = 0; h < scaledImage.GetHeight(); h++)
        {
            for (int w = 0; w < scaledImage.GetWidth(); w++)
            {
                int a = scaledImage.ValueAt(w, h) >> 24;
                scaledImage.ValueAt(w, h) = RGB(a, a, a);
            }
        }
    }
    else if (m_eShowMode == ESHOW_RGB_ALPHA)
    {
        int halfWidth = scaledImage.GetWidth() / 2;
        for (int h = 0; h < scaledImage.GetHeight(); h++)
        {
            for (int w = 0; w < halfWidth; w++)
            {
                int r = GetRValue(scaledImage.ValueAt(w, h));
                int g = GetGValue(scaledImage.ValueAt(w, h));
                int b = GetBValue(scaledImage.ValueAt(w, h));
                int a = scaledImage.ValueAt(w, h) >> 24;
                scaledImage.ValueAt(w, h) = RGB(b, g, r);
                scaledImage.ValueAt(w + halfWidth, h) = RGB(a, a, a);
            }
        }
    }
    else //if (m_showMode == ESHOW_RGBA)
    {
        scaledImage.SwapRedAndBlue();
    }

    update();

    QImage qImage(scaledImage.GetWidth(), scaledImage.GetHeight(), QImage::Format_RGB32);
    memcpy(qImage.bits(), scaledImage.GetData(), qImage.byteCount());
    m_staticBitmap.setPixmap(QPixmap::fromImage(qImage));

    if (m_bShowHistogram && scaledImage.GetData())
    {
        m_rgbaHistogram.ComputeHistogram((BYTE*)image.GetData(), image.GetWidth(), image.GetHeight(), CImageHistogramCtrl::eImageFormat_32BPP_BGRA);
        m_rgbaHistogram.m_drawMode = CImageHistogramCtrl::eDrawMode_OverlappedRGB;
        m_alphaChannelHistogram.CopyComputedDataFrom(&m_rgbaHistogram);
        m_alphaChannelHistogram.m_drawMode = CImageHistogramCtrl::eDrawMode_AlphaChannel;
    }

    CorrectPosition();
    update();
    return true;
}

void CBitmapToolTip::OnTimer()
{
    /*
    if (IsWindowVisible())
    {
        if (m_bHaveAnythingToRender)
            Invalidate();
    }
    */
    if (m_hToolWnd)
    {
        QRect toolRc(m_toolRect);
        QRect rc = geometry();
        QPoint cursorPos = QCursor::pos();
        toolRc.moveTopLeft(m_hToolWnd->mapToGlobal(toolRc.topLeft()));
        if (!toolRc.contains(cursorPos) && !rc.contains(cursorPos))
        {
            setVisible(false);
        }
        else
        {
            RefreshViewmode();
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CBitmapToolTip::showEvent(QShowEvent* event)
{
    QPoint cursorPos = QCursor::pos();
    move(cursorPos);
    m_timer.start(500);
}

//////////////////////////////////////////////////////////////////////////
void CBitmapToolTip::hideEvent(QHideEvent* event)
{
    m_timer.stop();
}

//////////////////////////////////////////////////////////////////////////

void CBitmapToolTip::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Control || event->key() == Qt::Key_Alt || event->key() == Qt::Key_Shift)
    {
        RefreshViewmode();
    }
}

void CBitmapToolTip::keyReleaseEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Control || event->key() == Qt::Key_Alt || event->key() == Qt::Key_Shift)
    {
        RefreshViewmode();
    }
}

//////////////////////////////////////////////////////////////////////////
void CBitmapToolTip::resizeEvent(QResizeEvent* event)
{
    RefreshLayout();
}

void CBitmapToolTip::CorrectPosition()
{
    // Move window inside monitor
    QRect rc = geometry();


    int hMon = qApp->desktop()->screenNumber(rc.bottomRight());
    if (hMon == -1)
    {
        hMon = qApp->desktop()->screenNumber(QCursor::pos());
        if (hMon == -1)
        {
            return;
        }
    }

    QRect rcMonitor = qApp->desktop()->screenGeometry(hMon);

    QRect toolRc(m_toolRect);
    if (m_hToolWnd)
    {
        toolRc.moveTopLeft(m_hToolWnd->mapToGlobal(toolRc.topLeft()));
    }

    bool bOffset = false;
    int dx, dy;
    QRect go = rc;
    QPoint cursorPos = QCursor::pos();

    // move to the default position
    go.setRight(cursorPos.x() + rc.width());
    go.setLeft(cursorPos.x());
    go.setBottom(toolRc.top() + rc.height() + m_toolRect.height());
    go.setTop(toolRc.top() + m_toolRect.height());

    // if the texture clips monitor's right, move it left
    if (go.right() > rcMonitor.right())
    {
        dx = rcMonitor.right() - go.right();
        go.translate(dx, 0);
    }

    // if the texture clips monitor's bottom, move it up
    if (go.bottom() > rcMonitor.bottom())
    {
        dy = rcMonitor.bottom() - go.bottom();
        go.translate(0, dy);
    }

    // if the texture clips monitor's top, move it down
    if (go.top() < rcMonitor.top())
    {
        dy = rcMonitor.bottom() - go.bottom();
        go.translate(0, dy);
    }

    // move window only if position changed
    if (go != rc)
    {
        setGeometry(go);
    }
}


//////////////////////////////////////////////////////////////////////////
void CBitmapToolTip::SetTool(QWidget* pWnd, const QRect& rect)
{
    assert(pWnd);
    m_hToolWnd = pWnd;
    m_toolRect = rect;
}

#include <Controls/BitmapToolTip.moc>
