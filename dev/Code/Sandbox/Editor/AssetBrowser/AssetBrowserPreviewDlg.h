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

// Description : The asset browser preview dialog window

#ifndef CRYINCLUDE_EDITOR_ASSET_BROWSER_ASSETBROWSERPREVIEWDLG_H
#define CRYINCLUDE_EDITOR_ASSET_BROWSER_ASSETBROWSERPREVIEWDLG_H
#pragma once
#include "Include/IAssetItem.h"
#include "Util/GdiUtil.h"

#include <QWidget>

namespace Ui
{
    class AssetBrowserPreviewDialog;
}

class CAssetBrowserPreviewDlg
    : public QWidget
{
    Q_OBJECT
public:
    CAssetBrowserPreviewDlg(QWidget* pParent = nullptr);   // standard constructor
    virtual ~CAssetBrowserPreviewDlg();

    void StartPreviewAsset(IAssetItem* pAsset);
    void EndPreviewAsset();
    void SetAssetPreviewHeaderDlg(QWidget* pDlg);
    void SetAssetPreviewFooterDlg(QWidget* pDlg);

    bool eventFilter(QObject* object, QEvent* event) override;

protected slots:
    void Render();

protected:
    IAssetItem* m_pAssetItem;
    QPoint m_lastPanDragPt;
    IRenderer* m_piRenderer;
    CGdiCanvas m_canvas;
    QWidget* m_pAssetPreviewHeaderDlg;
    QWidget* m_pAssetPreviewFooterDlg;
    QFont m_noPreviewTextFont;

    virtual void OnInitDialog();
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void OnDestroy();

private:
    QScopedPointer<Ui::AssetBrowserPreviewDialog> m_ui;
};

#endif // CRYINCLUDE_EDITOR_ASSET_BROWSER_ASSETBROWSERPREVIEWDLG_H
