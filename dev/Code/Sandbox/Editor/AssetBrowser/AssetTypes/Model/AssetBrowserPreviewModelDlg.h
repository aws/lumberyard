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

// Description : Defines the preview model dialog panel, used for previewing
//               the current selected model asset


#ifndef CRYINCLUDE_EDITOR_ASSET_BROWSER_ASSET_TYPES_MODEL_ASSETBROWSERPREVIEWMODELDLG_H
#define CRYINCLUDE_EDITOR_ASSET_BROWSER_ASSET_TYPES_MODEL_ASSETBROWSERPREVIEWMODELDLG_H
#pragma once
class CAssetBrowserPreviewModelDlgFooter;

#include <QWidget>

namespace Ui
{
    class AssetBrowserPreviewCharacterDlg;
}

class CAssetBrowserPreviewModelDlg
    : public QWidget
{
    Q_OBJECT
public:
    class CAssetModelItem* m_pModel;

    void Init();
    CAssetBrowserPreviewModelDlg(QWidget* pParent = nullptr);   // standard constructor
    virtual ~CAssetBrowserPreviewModelDlg();
    void SetFooter(CAssetBrowserPreviewModelDlgFooter* footer)
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

private:
    CAssetBrowserPreviewModelDlgFooter* m_pFooter;
    std::map<QString, XmlNodeRef> m_lightingMap;
    XmlNodeRef m_oldTod;
    QRect m_originalRect;

public:
    void OnBnClickedButtonSaveThumbAngle();

private:
    QScopedPointer<Ui::AssetBrowserPreviewCharacterDlg> m_ui;
};
#endif // CRYINCLUDE_EDITOR_ASSET_BROWSER_ASSET_TYPES_MODEL_ASSETBROWSERPREVIEWMODELDLG_H
