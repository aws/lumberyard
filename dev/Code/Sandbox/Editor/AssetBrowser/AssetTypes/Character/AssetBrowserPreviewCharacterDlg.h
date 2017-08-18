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

#ifndef CRYINCLUDE_EDITOR_ASSET_BROWSER_ASSET_TYPES_CHARACTER_ASSETBROWSERPREVIEWCHARACTERDLG_H
#define CRYINCLUDE_EDITOR_ASSET_BROWSER_ASSET_TYPES_CHARACTER_ASSETBROWSERPREVIEWCHARACTERDLG_H
#pragma once

class CAssetBrowserPreviewCharacterDlgFooter;

#include <QWidget>

namespace Ui
{
    class AssetBrowserPreviewCharacterDlg;
}

class CAssetBrowserPreviewCharacterDlg
    : public QWidget
{
    Q_OBJECT
public:
    class CAssetCharacterItem* m_pModel;

    void Init();
    CAssetBrowserPreviewCharacterDlg(QWidget* pParent = nullptr);   // standard constructor
    virtual ~CAssetBrowserPreviewCharacterDlg();
    void SetFooter(CAssetBrowserPreviewCharacterDlgFooter* footer)
    {
        m_pFooter = footer;
    }

protected:
    void DockViewPane();
    void OnInitDialog();
    void OnBnClickedButtonWireframe();
    void OnBnClickedButtonPhysics();
    void OnBnClickedButtonNormals();
    void OnBnClickedButtonResetView();
    void OnBnClickedButtonFullscreen();
    void OnLightingChanged();
    void OnBnClickedButtonSaveThumbAngle();

private:
    CAssetBrowserPreviewCharacterDlgFooter* m_pFooter;
    std::map<QString, XmlNodeRef> m_lightingMap;
    XmlNodeRef m_oldTod;
    QRect m_originalRect;

    QScopedPointer<Ui::AssetBrowserPreviewCharacterDlg> m_ui;
};
#endif // CRYINCLUDE_EDITOR_ASSET_BROWSER_ASSET_TYPES_CHARACTER_ASSETBROWSERPREVIEWCHARACTERDLG_H
