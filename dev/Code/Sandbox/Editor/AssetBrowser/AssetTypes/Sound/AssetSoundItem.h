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

// Description : Header file for handling and rendering the the sound asset items
//               in the asset browser


#ifndef CRYINCLUDE_EDITOR_ASSET_BROWSER_ASSET_TYPES_SOUND_ASSETSOUNDITEM_H
#define CRYINCLUDE_EDITOR_ASSET_BROWSER_ASSET_TYPES_SOUND_ASSETSOUNDITEM_H
#pragma once
#include "AssetBrowser/AssetBrowserCommon.h"

class CAssetSoundItem
    : public CAssetItem
{
public:
    CAssetSoundItem();
    ~CAssetSoundItem();

    QVariant GetAssetFieldValue(const char* pFieldName) const;
    void LoadSound();
    bool Cache();
    void OnBeginPreview(QWidget* hQuickPreviewWnd);
    void OnEndPreview();
    void PreviewRender(QWidget* hRenderWindow, const QRect& rstViewport, int aMouseX, int aMouseY, int aMouseDeltaX, int aMouseDeltaY, UINT aKeyFlags);
    void* CreateInstanceInViewport(float aX, float aY, float aZ);
    bool MoveInstanceInViewport(const void* pDraggedObject, float aX, float aY, float aZ);
    void AbortCreateInstanceInViewport(const void* pDraggedObject);
    void OnPreviewRenderKeyEvent(bool bKeyDown, UINT aChar, UINT aKeyFlags);
    void ToXML(XmlNodeRef& node) const;
    void FromXML(const XmlNodeRef& node);

    HRESULT STDMETHODCALLTYPE QueryInterface(const IID& riid, void** ppvObj);
    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();

protected:
    UINT m_nSoundLengthMsec;
    UINT m_timeStamp;
    bool m_bLoopingSound;
    static QImage s_uncachedSoundThumbBmp;
};
#endif // CRYINCLUDE_EDITOR_ASSET_BROWSER_ASSET_TYPES_SOUND_ASSETSOUNDITEM_H
