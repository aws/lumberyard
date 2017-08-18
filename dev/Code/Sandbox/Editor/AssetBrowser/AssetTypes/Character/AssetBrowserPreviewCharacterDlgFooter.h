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

#ifndef CRYINCLUDE_EDITOR_ASSET_BROWSER_ASSET_TYPES_CHARACTER_ASSETBROWSERPREVIEWCHARACTERDLGFOOTER_H
#define CRYINCLUDE_EDITOR_ASSET_BROWSER_ASSET_TYPES_CHARACTER_ASSETBROWSERPREVIEWCHARACTERDLGFOOTER_H
#pragma once

#include <QWidget>

namespace Ui
{
    class AssetBrowserPreviewCharacterDlgFooter;
}

class CAssetBrowserPreviewCharacterDlgFooter
    : public QWidget
{
    Q_OBJECT
public:
    CAssetBrowserPreviewCharacterDlgFooter(QWidget* pParent = nullptr);
    virtual ~CAssetBrowserPreviewCharacterDlgFooter();

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
#endif // CRYINCLUDE_EDITOR_ASSET_BROWSER_ASSET_TYPES_CHARACTER_ASSETBROWSERPREVIEWCHARACTERDLGFOOTER_H
