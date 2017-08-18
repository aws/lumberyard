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

// Description : Header file for the class implementing IAssetDisplayItem
//               interface  It declares the headers of the actual used
//               functions


#ifndef CRYINCLUDE_EDITOR_ASSET_BROWSER_ASSET_TYPES_TEXTURE_ASSETTEXTUREITEM_H
#define CRYINCLUDE_EDITOR_ASSET_BROWSER_ASSET_TYPES_TEXTURE_ASSETTEXTUREITEM_H
#pragma once
#include "AssetBrowser/AssetBrowserCommon.h"
#include "Util/GdiUtil.h"

class ITexture;

class CAssetTextureItem
    : public CAssetItem
{
public:
    friend class CAssetBrowserPreviewTextureDlg;

    enum EAssetTextureDrawingOption
    {
        eAssetTextureDrawing_RGBA = 0,
        eAssetTextureDrawing_Alpha,
        eAssetTextureDrawing_RGB,
    };

    enum EAssetTextureUsage
    {
        eAssetTextureUsage_DiffuseMap,
        eAssetTextureUsage_NormalMap,
        eAssetTextureUsage_SpecularMap,
        eAssetTextureUsage_DisplacementMap,
        eAssetTextureUsage_GlossMap,
        eAssetTextureUsage_DetailMap,
        eAssetTextureUsage_DecalMap
    };

    CAssetTextureItem();
    ~CAssetTextureItem();
    QVariant GetAssetFieldValue(const char* pFieldName) const;
    bool Cache();
    void OnBeginPreview(QWidget* hQuickPreviewWndC) override;
    void OnEndPreview();
    void PreviewRender(
        QWidget* hRenderWindow,
        const QRect& rstViewport,
        int aMouseX, int aMouseY,
        int aMouseDeltaX, int aMouseDeltaY,
        int aMouseWheelDelta, UINT aKeyFlags) override;
    void OnPreviewRenderKeyEvent(bool bKeyDown, UINT aChar, UINT aKeyFlags);
    bool Render(QWidget* hRenderWindow, const QRect& rstViewport, bool bCacheThumbnail = false);
    void CacheFieldsInfoForLoadedTex(const ITexture* pTexture);
    QWidget* GetCustomPreviewPanelHeader(QWidget* pParentWnd) override;
    void ToXML(XmlNodeRef& node) const;
    void FromXML(const XmlNodeRef& node);

    HRESULT STDMETHODCALLTYPE QueryInterface(const IID& riid, void** ppvObj);
    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();

protected:
    void DrawTextOnReportImage(QPaintDevice* abm) const;
    void PreviewCenterAndScaleToFit(const QRect& rViewport);

    QString m_strSurfaceTypeString;
    bool m_bHasAlphaChannel, m_bIsCubemap;
    UINT m_nTextureWidth, m_nTextureHeight, m_nMips;
    ETEX_Format m_format;
    EAssetTextureUsage m_textureUsage;
    CAlphaBitmap m_previewBmp, m_previewBmpAlpha, m_previewBmpRGB;
    EAssetTextureDrawingOption m_previewDrawingMode;
    static QImage s_uncachedTextureThumbBmp;
    //! current rendering zoom ( 0.0f .. 1.0f [100%] )
    float m_previewRenderZoom;
    QPoint m_previewOffset;
    QSize m_previewSize;
    QColor m_previewBackColor;
    bool m_bPreviewSmooth;
    int m_previewMipLevel;
};
#endif // CRYINCLUDE_EDITOR_ASSET_BROWSER_ASSET_TYPES_TEXTURE_ASSETTEXTUREITEM_H
