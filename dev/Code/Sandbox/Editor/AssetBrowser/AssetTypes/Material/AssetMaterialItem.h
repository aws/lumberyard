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

// Description : Header file for handling and rendering the the Material asset items
//               in the asset browser


#ifndef CRYINCLUDE_EDITOR_ASSET_BROWSER_ASSET_TYPES_MATERIAL_ASSETMATERIALITEM_H
#define CRYINCLUDE_EDITOR_ASSET_BROWSER_ASSET_TYPES_MATERIAL_ASSETMATERIALITEM_H
#pragma once
#include "AssetBrowser/AssetBrowserCommon.h"

class CPreviewModelCtrl;

class CAssetMaterialItem
    : public CAssetItem
{
public:
    CAssetMaterialItem();
    ~CAssetMaterialItem();

    QVariant GetAssetFieldValue(const char* pFieldName) const;
    void LoadMaterial();
    bool Cache();
    void OnBeginPreview(QWidget* hQuickPreviewWnd);
    void OnEndPreview();
    void PreviewRender(QWidget* hRenderWindow, const QRect& rstViewport, int aMouseX, int aMouseY, int aMouseDeltaX, int aMouseDeltaY, UINT aKeyFlags);
    bool Render(QWidget* hRenderWindow, const QRect& rstViewport, bool bCacheThumbnail);
    void CacheThumbnail(QWidget* hRenderWindow, const QRect& rc);
    void OnPreviewRenderKeyEvent(bool bKeyDown, UINT aChar, UINT aKeyFlags);
    void ToXML(XmlNodeRef& node) const;
    void FromXML(const XmlNodeRef& node);

    HRESULT STDMETHODCALLTYPE QueryInterface(const IID& riid, void** ppvObj);
    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();

private:
    static QImage s_uncachedMaterialThumbBmp;
    static CPreviewModelCtrl* s_pPreviewCtrl;
    _smart_ptr<CMaterial> m_pMaterial;
};
#endif // CRYINCLUDE_EDITOR_ASSET_BROWSER_ASSET_TYPES_MATERIAL_ASSETMATERIALITEM_H
