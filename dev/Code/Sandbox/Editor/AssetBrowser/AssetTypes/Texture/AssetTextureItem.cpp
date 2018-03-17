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

// Description : Implementation of AssetTextureItem.h


#include "StdAfx.h"
#include "AssetTextureItem.h"
#include "Util/GdiUtil.h"
#include "Util/MemoryBlock.h"
#include "Util/Image.h"
#include "Util/ImageUtil.h"
#include "Util/PathUtil.h"
#include "Include/IAssetItemDatabase.h"
#include "ITexture.h"
#include "IRenderer.h"
#include "Include/IAssetViewer.h"
#include "ImageExtensionHelper.h"
#include "Controls/AssetViewer.h"
#include "AssetBrowserPreviewTextureDlg.h"

#include <QPointer>
#include <QFile>
#include <QPainter>
#include <QTextStream>
#include <QVariant>

namespace AssetBrowser
{
    const float kTexturePreviewZoomSpeedDamper = 2000.0f;
    const int   kMouseWheelDeltaStep = (WHEEL_DELTA == 0 ? 1 : WHEEL_DELTA);
    const float kTexturePreviewMinZoom = 0.01f;
    const float kTexturePreviewMaxZoom = 20.0f;
    const float kTexturePreviewGridScaleBackColor = 0.7f;
    const int   kTexturePreviewInfoTextMargin = 5;

    QPointer<CAssetBrowserPreviewTextureDlg> s_texturePreviewDlg;
};


QImage CAssetTextureItem::s_uncachedTextureThumbBmp;

CAssetTextureItem::CAssetTextureItem()
    : CAssetItem()
    , m_strSurfaceTypeString("")
    , m_bHasAlphaChannel(false)
    , m_bIsCubemap(false)
    , m_nTextureWidth(0)
    , m_nTextureHeight(0)
    , m_nMips(0)
    , m_format(eTF_Unknown)
    , m_textureUsage(eAssetTextureUsage_DiffuseMap)
    , m_previewDrawingMode(eAssetTextureDrawing_RGB)
    , m_previewRenderZoom(1.0f)
    , m_bPreviewSmooth(true)
    , m_previewMipLevel(0)
    , m_previewBackColor(QColor(100, 100, 100))
{
    m_flags |= eFlag_UseGdiRendering;

    if (s_uncachedTextureThumbBmp.isNull())
    {
        s_uncachedTextureThumbBmp.load("Editor/UI/Icons/asset_texture.png");
    }

    m_pUncachedThumbBmp = &s_uncachedTextureThumbBmp;
}

CAssetTextureItem::~CAssetTextureItem()
{
    // empty, call FreeData first
}

HRESULT STDMETHODCALLTYPE CAssetTextureItem::QueryInterface(const IID& riid, void** ppvObj)
{
    if (riid == __uuidof(IAssetItem))
    {
        *ppvObj = this;
        return S_OK;
    }

    return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE CAssetTextureItem::AddRef()
{
    return ++m_ref;
};

ULONG STDMETHODCALLTYPE CAssetTextureItem::Release()
{
    if ((--m_ref) == 0)
    {
        FreeData();
        delete this;
        return 0;
    }
    else
    {
        return m_ref;
    }
}

QVariant CAssetTextureItem::GetAssetFieldValue(const char* pFieldName) const
{
    if (AssetViewer::IsFieldName(pFieldName, "thumbToolTipText"))
    {
        const QString tagsText = CAssetItem::GetAssetFieldValue("tags").toString();
        return QString::fromLatin1("Path: %1\nWidth: %2\nHeight: %3\nMips: %4\nFilesize: %5 kB\nTags: %6\n")
                   .arg(m_strRelativePath)
                   .arg(m_nTextureWidth)
                   .arg(m_nTextureHeight)
                   .arg(m_nMips)
                   .arg(m_nFileSize / 1024.0f, 2, 'f')
                   .arg(tagsText);
    }
    else if (AssetViewer::IsFieldName(pFieldName, "thumbOneLineText") && m_nTextureWidth)
    {
        stack_string str;
        return QString::fromLatin1("[ %1 x %2 ]").arg(m_nTextureWidth).arg(m_nTextureHeight);
    }
    else if (AssetViewer::IsFieldName(pFieldName, "errors"))
    {
        QString str;

        if (m_nMips == 1)
        {
            str += QStringLiteral("WARNING: MipCount is zero\n");
        }

        return str;
    }
    else if (AssetViewer::IsFieldName(pFieldName, "width"))
    {
        return m_nTextureWidth;
    }
    else if (AssetViewer::IsFieldName(pFieldName, "height"))
    {
        return m_nTextureHeight;
    }
    else if (AssetViewer::IsFieldName(pFieldName, "mips"))
    {
        return m_nMips;
    }
    else if (AssetViewer::IsFieldName(pFieldName, "format"))
    {
        return QString::fromLatin1(CImageExtensionHelper::NameForTextureFormat(m_format));
        return true;
    }
    else if (AssetViewer::IsFieldName(pFieldName, "type"))
    {
        return m_bIsCubemap ? QStringLiteral("Cube") : QStringLiteral("2D");
    }
    else if (AssetViewer::IsFieldName(pFieldName, "texture_usage"))
    {
        if (m_textureUsage == eAssetTextureUsage_DiffuseMap)
        {
            return QStringLiteral("Diffuse");
        }
        else if (m_textureUsage == eAssetTextureUsage_NormalMap)
        {
            return QStringLiteral("Normal");
        }
        else if (m_textureUsage == eAssetTextureUsage_SpecularMap)
        {
            return QStringLiteral("Specular");
        }
        else if (m_textureUsage == eAssetTextureUsage_DisplacementMap)
        {
            return QStringLiteral("Displacement");
        }
        else if (m_textureUsage == eAssetTextureUsage_GlossMap)
        {
            return QStringLiteral("Gloss");
        }
        else if (m_textureUsage == eAssetTextureUsage_DecalMap)
        {
            return QStringLiteral("Decal");
        }
        else if (m_textureUsage == eAssetTextureUsage_DetailMap)
        {
            return QStringLiteral("Detail");
        }
    }

    // else, check if the common fields are requested
    return CAssetItem::GetAssetFieldValue(pFieldName);
}

bool CAssetTextureItem::Cache()
{
    if (m_flags & eFlag_Cached)
    {
        return true;
    }

    CImageEx img;

    QString str = m_strRelativePath;
    str += m_strFilename;

    if (!CImageUtil::LoadImage(str.toUtf8().data(), img))
    {
        SetFlag(eFlag_Invalid, true);
        return false;
    }

    unsigned int* pData = img.GetData();

    if (!pData)
    {
        img.Release();
        return false;
    }

    if (0 == img.GetWidth() || 0 == img.GetHeight())
    {
        img.Release();
        return false;
    }

    // compute correct ratio to scale the image right
    float ratioX = 1, ratioY = 1;
    UINT newWidth = img.GetWidth();
    UINT newHeight = img.GetHeight();

    if (newWidth > newHeight)
    {
        if (newWidth > 0)
        {
            ratioY = (float) newHeight / newWidth;
        }
    }
    else
    {
        if (newHeight > 0)
        {
            ratioX = (float) newWidth / newHeight;
        }
    }

    newWidth = ratioX * gSettings.sAssetBrowserSettings.nThumbSize;
    newHeight = ratioY * gSettings.sAssetBrowserSettings.nThumbSize;

    m_cachedThumbBmp = QImage(reinterpret_cast<uchar*>(pData),
            img.GetWidth(),
            img.GetHeight(),
            QImage::Format_ARGB32)
            .scaled(gSettings.sAssetBrowserSettings.nThumbSize,
            gSettings.sAssetBrowserSettings.nThumbSize,
            Qt::IgnoreAspectRatio);

    if (m_cachedThumbBmp.isNull())
    {
        img.Release();
        return false;
    }

    m_nTextureWidth = img.GetWidth();
    m_nTextureHeight = img.GetHeight();
    m_nMips = img.GetNumberOfMipMaps();
    m_format = img.GetFormat();
    m_bIsCubemap = img.IsCubemap();
    m_strDccFilename = img.GetDccFilename();

    if (m_strFilename.contains(QStringLiteral("_ddn.")))
    {
        m_textureUsage = eAssetTextureUsage_NormalMap;
    }
    else if (m_strFilename.contains(QStringLiteral("_spec.")))
    {
        m_textureUsage = eAssetTextureUsage_SpecularMap;
    }
    else if (m_strFilename.contains(QStringLiteral("_displ.")))
    {
        m_textureUsage = eAssetTextureUsage_DisplacementMap;
    }
    else if (m_strFilename.contains(QStringLiteral("_gloss.")))
    {
        m_textureUsage = eAssetTextureUsage_GlossMap;
    }
    else if (m_strFilename.contains(QStringLiteral("_decal.")))
    {
        m_textureUsage = eAssetTextureUsage_DecalMap;
    }
    else if (m_strFilename.contains(QStringLiteral("_detail.")))
    {
        m_textureUsage = eAssetTextureUsage_DetailMap;
    }
    else
    {
        m_textureUsage = eAssetTextureUsage_DiffuseMap;
    }

    if (0 == m_nMips)
    {
        m_flags |= eFlag_HasErrors;
    }

    SetFlag(eFlag_Cached, true);
    GetOwnerDatabase()->OnMetaDataChange(this);

    if (0 >= m_nTextureWidth || 0 >= m_nTextureHeight)
    {
        SetFlag(eFlag_Invalid, true);
    }

    img.Release();

    CAssetItem::Cache();
    return true;
}

void CAssetTextureItem::OnBeginPreview(QWidget* hQuickPreviewWnd)
{
    CImageEx img;
    QString str = m_strRelativePath;

    str += m_strFilename;

    if (!CImageUtil::LoadImage(str, img))
    {
        SetFlag(eFlag_Invalid, true);
        return;
    }

    unsigned int* pData = img.GetData();

    if (!pData)
    {
        img.Release();
        return;
    }

    if (0 == img.GetWidth() || 0 == img.GetHeight())
    {
        img.Release();
        return;
    }

    CAlphaBitmap bmp;

    if (!bmp.Create(pData, img.GetWidth(), img.GetHeight(), true, false))
    {
        img.Release();
        return;
    }

    if (!m_previewBmp.Create(pData, img.GetWidth(), img.GetHeight(), true, true))
    {
        img.Release();
        return;
    }

    // Create the alpha-only thumbnail.

    int byteCount = m_previewBmp.GetWidth() * m_previewBmp.GetHeight() * (m_previewBmp.GetBitmap().depth() / 8);
    std::vector<unsigned char> bits;

    bits.resize(byteCount);

    if (bits.empty())
    {
        img.Release();
        return;
    }

    std::copy(m_previewBmp.GetBitmap().bits(), m_previewBmp.GetBitmap().bits() + byteCount, bits.begin());

    for (UINT i = 0; i < byteCount; i += 4)
    {
        bits[i] = bits[i + 1] = bits[i + 2] = bits[i + 3];
        bits[i + 3] = 255;
    }

    m_previewBmpAlpha.Create(&bits[0], m_previewBmp.GetWidth(), m_previewBmp.GetHeight(), true, false);

    // Create the RGB-only thumbnail.
    if (m_previewBmpRGB.Create(NULL, m_previewBmp.GetWidth(), m_previewBmp.GetHeight(), false, false))
    {
        QPainter painter(&m_previewBmpRGB.GetBitmap());
        painter.fillRect(m_previewBmp.GetBitmap().rect(), Qt::black);
        painter.drawImage(QPoint(0, 0), m_previewBmp.GetBitmap());
    }

    const QRect rc = hQuickPreviewWnd->rect();
    PreviewCenterAndScaleToFit(rc);
    m_hPreviewWidget = hQuickPreviewWnd;
}

void CAssetTextureItem::OnEndPreview()
{
    m_hPreviewWidget = 0;
    m_previewBmp.Free();
    m_previewBmpRGB.Free();
    m_previewBmpAlpha.Free();
}

void CAssetTextureItem::PreviewCenterAndScaleToFit(const QRect& rViewport)
{
    float ratio = 1.0f;
    UINT newWidth = m_previewBmp.GetWidth();
    UINT newHeight = m_previewBmp.GetHeight();

    if (newWidth > rViewport.width())
    {
        if (newWidth > 0)
        {
            ratio = (float) rViewport.width() / newWidth;
        }

        newWidth = rViewport.width();
        newHeight *= ratio;
    }

    if (newHeight > rViewport.height())
    {
        if (newHeight > 0)
        {
            ratio = (float) rViewport.height() / newHeight;
        }

        newHeight = rViewport.height();
        newWidth *= ratio;
    }

    m_previewSize = QSize(newWidth, newHeight);
    m_previewOffset.rx() = rViewport.left() + rViewport.width() / 2 - newWidth / 2;
    m_previewOffset.ry() = rViewport.top() + rViewport.height() / 2 - newHeight / 2;
    m_previewRenderZoom = 1.0f;
}

void CAssetTextureItem::PreviewRender(
    QWidget* hRenderWindow,
    const QRect& rstViewport,
    int aMouseX, int aMouseY,
    int aMouseDeltaX, int aMouseDeltaY,
    int aMouseWheelDelta, UINT aKeyFlags)
{
    float zoomDelta = 0.1f * (float)aMouseWheelDelta / AssetBrowser::kMouseWheelDeltaStep;

    m_previewRenderZoom += zoomDelta;
    m_previewRenderZoom = CLAMP(m_previewRenderZoom, AssetBrowser::kTexturePreviewMinZoom, AssetBrowser::kTexturePreviewMaxZoom);

    if (aMouseWheelDelta < 0)
    {
        zoomDelta = 1.0f - fabs(zoomDelta);
    }
    else
    {
        zoomDelta = 1.0f + fabs(zoomDelta);
    }

    if (0 != aMouseWheelDelta)
    {
        float zoomPtx = 0;
        float zoomPty = 0;

        assert(m_previewRenderZoom != 0.0f);
        zoomPtx = ((float)aMouseX - m_previewOffset.x()) / m_previewRenderZoom;
        zoomPty = ((float)aMouseY - m_previewOffset.y()) / m_previewRenderZoom;
        m_previewRenderZoom += (float)aMouseWheelDelta / AssetBrowser::kTexturePreviewZoomSpeedDamper;

        QPoint newZoomCenter((float)(zoomPtx * m_previewRenderZoom + m_previewOffset.x()),
            (float)(zoomPty * m_previewRenderZoom + m_previewOffset.y()));

        m_previewOffset.rx() -= newZoomCenter.x() - aMouseX;
        m_previewOffset.ry() -= newZoomCenter.y() - aMouseY;
    }

    if (aKeyFlags & MK_LBUTTON)
    {
        m_previewOffset.rx() += aMouseDeltaX;
        m_previewOffset.ry() += aMouseDeltaY;
    }

    Render(hRenderWindow, rstViewport, false);
}

void CAssetTextureItem::OnPreviewRenderKeyEvent(bool bKeyDown, UINT aChar, UINT aKeyFlags)
{
    // empty
}

bool CAssetTextureItem::Render(QWidget* hRenderWindow, const QRect& r, bool bCacheThumbnail)
{
    QPainter painter(hRenderWindow);
    // if we are in preview mode
    if (m_hPreviewWidget)
    {
        QPainter painter(hRenderWindow);

        QBrush brush(ScaleColor(m_previewBackColor, AssetBrowser::kTexturePreviewGridScaleBackColor), Qt::DiagCrossPattern);
        painter.fillRect(r, m_previewBackColor);
        painter.fillRect(r, brush);

        int newWidth = m_previewSize.width() * m_previewRenderZoom;
        int newHeight = m_previewSize.height() * m_previewRenderZoom;
        QPoint pt = m_previewOffset;

        painter.setRenderHint(QPainter::SmoothPixmapTransform, m_bPreviewSmooth);

        if (m_previewDrawingMode == eAssetTextureDrawing_RGBA)
        {
            painter.drawImage(QRect(pt, QSize(newWidth, newHeight)), m_previewBmp.GetBitmap());
        }
        else if (m_previewDrawingMode == eAssetTextureDrawing_Alpha)
        {
            painter.drawImage(QRect(pt, QSize(newWidth, newHeight)), m_previewBmpAlpha.GetBitmap());
        }
        else if (m_previewDrawingMode == eAssetTextureDrawing_RGB)
        {
            painter.drawImage(QRect(pt, QSize(newWidth, newHeight)), m_previewBmpRGB.GetBitmap());
        }

        QFont fntInfo("Arial Bold", 9);

        painter.setFont(fntInfo);

        QString strTmp, strDrawingMode;

        if (m_previewDrawingMode == eAssetTextureDrawing_Alpha)
        {
            strDrawingMode = "ALPHA";
        }

        if (m_previewDrawingMode == eAssetTextureDrawing_RGB)
        {
            strDrawingMode = "RGB";
        }

        if (m_previewDrawingMode == eAssetTextureDrawing_RGBA)
        {
            strDrawingMode = "RGBA";
        }

        strTmp = QStringLiteral("%1 (%2%, %3x%4)").arg(strDrawingMode).arg(m_previewRenderZoom * 100.0f).arg(m_nTextureWidth).arg(m_nTextureHeight);

        QRect rc = r;

        rc.adjust(AssetBrowser::kTexturePreviewInfoTextMargin, AssetBrowser::kTexturePreviewInfoTextMargin, AssetBrowser::kTexturePreviewInfoTextMargin, AssetBrowser::kTexturePreviewInfoTextMargin);

        painter.setPen(QColor(0, 0, 0));
        rc.translate(1, 1);
        painter.drawText(rc, painter.fontMetrics().elidedText(strTmp, Qt::ElideRight, rc.width()));

        painter.setPen(QColor(255, 255, 0));
        rc.translate(-1, -1);
        painter.drawText(rc, painter.fontMetrics().elidedText(strTmp, Qt::ElideRight, rc.width()));
    }
    else
    {
        // normal render of texture thumb
        DrawThumbImage(&painter, r);
    }
    return true;
}

void CAssetTextureItem::CacheFieldsInfoForLoadedTex(const ITexture* pTexture)
{
    if (m_flags & eFlag_Cached)
    {
        return;
    }

    m_nTextureWidth = pTexture->GetWidth();
    m_nTextureHeight = pTexture->GetHeight();
    m_nMips = pTexture->GetNumMips();
    m_format = pTexture->GetTextureDstFormat();
    m_bIsCubemap = pTexture->GetTextureType() == eTT_Cube;

    if (0 == m_nMips)
    {
        m_flags |= eFlag_HasErrors;
    }

    SetFlag(eFlag_Cached, true);
    GetOwnerDatabase()->OnMetaDataChange(this);

    if (0 >= m_nTextureWidth || 0 >= m_nTextureHeight)
    {
        SetFlag(eFlag_Invalid, true);
    }
}

void CAssetTextureItem::DrawTextOnReportImage(QPaintDevice* pd) const
{
    const QColor filenameShadowColor(0, 0, 0);
    const QColor filenameColor(255, 255, 0);
    const QColor otherInfosColor(0, 0, 0);
    QFont fontInfoTitle(QStringLiteral("Arial"), 9, QFont::Bold);
    QFont fontInfo(QStringLiteral("Arial"), 8, QFont::Bold);

    QPainter painter(pd);
    painter.setFont(fontInfoTitle);
    painter.setPen(filenameShadowColor);
    painter.drawText(AssetViewer::kOverlayTextLeftMargin + 1,
        AssetViewer::kOverlayTextTopMargin + 1, m_strFilename);
    painter.setPen(filenameColor);
    painter.drawText(AssetViewer::kOverlayTextLeftMargin,
        AssetViewer::kOverlayTextTopMargin, m_strFilename);

    QString reportInfos = QString::fromLatin1("Path: %1\nRes: %2x%3\nFormat: %4\nType: %5\n")
            .arg(m_strRelativePath)
            .arg(m_nTextureWidth)
            .arg(m_nTextureHeight)
            .arg(CImageExtensionHelper::NameForTextureFormat(m_format))
            .arg(m_bIsCubemap ? "Cube" : "2D");

    const QSize titleSize = painter.fontMetrics().boundingRect(m_strFilename).size();
    const QRect rcTextInfo(QPoint(AssetViewer::kOverlayTextLeftMargin, titleSize.height() + 15), QPoint(pd->width(), pd->height()));
    painter.setPen(otherInfosColor);
    painter.setFont(fontInfo);
    painter.drawText(rcTextInfo, painter.fontMetrics().elidedText(reportInfos, Qt::ElideRight, rcTextInfo.width()));
}

QWidget* CAssetTextureItem::GetCustomPreviewPanelHeader(QWidget* pParentWnd)
{
    if (AssetBrowser::s_texturePreviewDlg == nullptr)
    {
        AssetBrowser::s_texturePreviewDlg = new CAssetBrowserPreviewTextureDlg(pParentWnd);
    }
    AssetBrowser::s_texturePreviewDlg->m_pTexture = this;

    AssetBrowser::s_texturePreviewDlg->Init();

    return AssetBrowser::s_texturePreviewDlg;
}

void CAssetTextureItem::ToXML(XmlNodeRef& node) const
{
    node->setTag("Texture");
    QString fileName = m_strRelativePath + m_strFilename;
    node->setAttr("fileName", fileName.toUtf8().data());
    node->setAttr("dccFilename", m_strDccFilename.toUtf8().data());
    node->setAttr("nTextureWidth", m_nTextureWidth);
    node->setAttr("nTextureHeight", m_nTextureHeight);
    node->setAttr("nMips", m_nMips);
    node->setAttr("format", (int)m_format);
    node->setAttr("textureUsage", (int)m_textureUsage);
    node->setAttr("bIsCubemap", m_bIsCubemap);
}

void CAssetTextureItem::FromXML(const XmlNodeRef& node)
{
    assert(node->isTag("Texture"));

    if (node->isTag("Texture") == false)
    {
        return;
    }

    node->getAttr("nTextureWidth", m_nTextureWidth);
    node->getAttr("nTextureHeight", m_nTextureHeight);
    node->getAttr("nMips", m_nMips);
    int format;
    node->getAttr("format", format);
    m_format = ETEX_Format(format);
    node->getAttr("bIsCubemap", m_bIsCubemap);
    node->getAttr("textureUsage", (int&)m_textureUsage);

    const char* dccstr = NULL;
    node->getAttr("dccFilename", &dccstr);

    if (dccstr)
    {
        m_strDccFilename = dccstr;
    }

    if (0 == m_nMips)
    {
        SetFlag(eFlag_HasErrors, true);
    }

    if (0 >= m_nTextureWidth || 0 >= m_nTextureHeight)
    {
        SetFlag(eFlag_Invalid, true);
    }

    SetFlag(eFlag_Cached);
}
