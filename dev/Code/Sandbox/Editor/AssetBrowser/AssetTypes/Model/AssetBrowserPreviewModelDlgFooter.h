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


#ifndef CRYINCLUDE_EDITOR_ASSET_BROWSER_ASSET_TYPES_MODEL_ASSETBROWSERPREVIEWMODELDLGFOOTER_H
#define CRYINCLUDE_EDITOR_ASSET_BROWSER_ASSET_TYPES_MODEL_ASSETBROWSERPREVIEWMODELDLGFOOTER_H
#pragma once

#include <QWidget>

namespace Ui
{
    class AssetBrowserPreviewCharacterDlgFooter;
}

class CAssetBrowserPreviewModelDlgFooter
    : public QWidget
{
    Q_OBJECT
public:
    CAssetBrowserPreviewModelDlgFooter(QWidget* pParent = nullptr);
    virtual ~CAssetBrowserPreviewModelDlgFooter();

    void Init();
    void Reset();

protected:
    void OnInitDialog();
    void OnLodLevelChanged();
    void OnHScroll();

protected:
    int m_minLodDefault;

private:
    QScopedPointer<Ui::AssetBrowserPreviewCharacterDlgFooter> m_ui;
};
#endif // CRYINCLUDE_EDITOR_ASSET_BROWSER_ASSET_TYPES_MODEL_ASSETBROWSERPREVIEWMODELDLGFOOTER_H
