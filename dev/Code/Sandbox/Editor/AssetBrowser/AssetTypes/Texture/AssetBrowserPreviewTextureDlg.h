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

// Description : The asset texture dialog panel, used for previewing textures
//               in the asset preview dialog


#ifndef CRYINCLUDE_EDITOR_ASSET_BROWSER_ASSET_TYPES_TEXTURE_ASSETBROWSERPREVIEWTEXTUREDLG_H
#define CRYINCLUDE_EDITOR_ASSET_BROWSER_ASSET_TYPES_TEXTURE_ASSETBROWSERPREVIEWTEXTUREDLG_H
#pragma once
#include "Controls/ImageHistogramCtrl.h"

#include <QWidget>

namespace Ui
{
    class AssetBrowserPreviewTextureDlg;
}

class CAssetBrowserPreviewTextureDlg
    : public QWidget
{
    Q_OBJECT
public:
    class CAssetTextureItem* m_pTexture;

    void Init();
    void ComputeHistogram();

    CAssetBrowserPreviewTextureDlg(QWidget* pParent = nullptr);   // standard constructor
    virtual ~CAssetBrowserPreviewTextureDlg();

protected:
    void OnBnClickedButtonShowAlpha();
    void OnBnClickedButtonShowRgb();
    void OnBnClickedButtonShowRgba();
    void OnBnClickedButtonZoom1to1();

public:
    QColor m_customBackColor;

    void OnInitDialog();
    void OnBnClickedCheckSmoothTexture();
    void OnCbnSelchangeComboTextureMipLevel();
    void OnCbnSelendokComboTexturePreviewBackcolor();
    void OnBnClickedCheckMoreTextureInfo();

private:
    QScopedPointer<Ui::AssetBrowserPreviewTextureDlg> m_ui;
};
#endif // CRYINCLUDE_EDITOR_ASSET_BROWSER_ASSET_TYPES_TEXTURE_ASSETBROWSERPREVIEWTEXTUREDLG_H
